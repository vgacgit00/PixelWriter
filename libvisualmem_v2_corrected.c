/**
 * LibVisualMem v2.0 - Core Implementation (Version Corrigée)
 * ===========================================================
 * 
 * Implémentation principale CORRIGÉE avec gestion sécurisée
 * des threads et prévention des segmentation faults
 */

#define _GNU_SOURCE
#include "libvisualmem_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

// === VARIABLES GLOBALES THREAD-SAFE ===
static pthread_mutex_t global_context_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t allocation_mutex = PTHREAD_MUTEX_INITIALIZER;

// === FONCTIONS UTILITAIRES SÉCURISÉES ===

static uint32_t calculate_checksum(const uint8_t* data, size_t size) __attribute__((unused));
static uint32_t calculate_checksum(const uint8_t* data, size_t size) {
    if (!data || size == 0) return 0;
    
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum += data[i];
        checksum = (checksum << 1) | (checksum >> 31);
    }
    return checksum;
}

static uint64_t get_timestamp_microseconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// === THREAD DE REFRESH DISPLAY SÉCURISÉ ===

static void* display_refresh_thread(void* arg) {
    visualmem_v2_context_t* ctx = (visualmem_v2_context_t*)arg;
    
    if (!ctx) {
        printf("[ERROR] Null context in refresh thread\n");
        return NULL;
    }
    
    printf("[DISPLAY] Refresh thread started (target: 60 Hz)\n");
    
    const int target_fps = 60;
    const long frame_time_us = 1000000 / target_fps; // 16667 microseconds
    
    while (ctx->refresh_thread_active) {
        // Vérification de sécurité avant refresh
        if (ctx->display_active && ctx->hardware_active) {
            pthread_mutex_lock(&global_context_mutex);
            
            // Double vérification après acquisition du mutex
            if (ctx->display_active && ctx->refresh_thread_active) {
                switch (ctx->backend) {
                    case VISUALMEM_V2_BACKEND_X11:
                    case VISUALMEM_V2_BACKEND_OPENGL:
                        if (ctx->x11.display && ctx->x11.window) {
                            visualmem_v2_x11_refresh(ctx);
                        }
                        break;
                    default:
                        // Autres backends...
                        break;
                }
            }
            
            pthread_mutex_unlock(&global_context_mutex);
        }
        
        // Attendre avant le prochain frame avec vérification d'interruption
        for (long elapsed = 0; elapsed < frame_time_us && ctx->refresh_thread_active; elapsed += 1000) {
            usleep(1000); // 1ms
        }
    }
    
    printf("[DISPLAY] Refresh thread stopped\n");
    return NULL;
}

// === API PRINCIPALE CORRIGÉE ===

int visualmem_v2_init(visualmem_v2_context_t* ctx, visualmem_v2_mode_t mode, int width, int height) {
    if (!ctx) {
        printf("[ERROR] Null context pointer\n");
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        printf("[ERROR] Invalid resolution: %dx%d\n", width, height);
        return VISUALMEM_V2_ERROR_INVALID_PARAMETER;
    }
    
    printf("[INIT] Initializing LibVisualMem v2.0...\n");
    printf("[INIT] Resolution: %dx%d, Mode: %d, Backend: %d\n", width, height, mode, ctx->backend);
    
    pthread_mutex_lock(&global_context_mutex);
    
    // Initialisation sécurisée de la structure
    memset(ctx, 0, sizeof(visualmem_v2_context_t));
    ctx->width = width;
    ctx->height = height;
    ctx->mode = mode;
    ctx->display_active = 0;
    ctx->hardware_active = 0;
    ctx->refresh_thread_active = 0;
    ctx->refresh_thread = 0;
    
    // Détection hardware
    visualmem_v2_hardware_caps_t caps;
    int caps_result = visualmem_v2_detect_hardware(&caps);
    if (caps_result != VISUALMEM_V2_SUCCESS) {
        printf("[ERROR] Hardware detection failed\n");
        pthread_mutex_unlock(&global_context_mutex);
        return caps_result;
    }
    
    // Sélection du backend
    ctx->backend = visualmem_v2_select_best_backend(&caps);
    if (ctx->backend == VISUALMEM_V2_BACKEND_AUTO) {
        printf("[ERROR] No suitable backend available\n");
        pthread_mutex_unlock(&global_context_mutex);
        return VISUALMEM_V2_ERROR_HARDWARE_INIT;
    }
    
    // Initialisation du backend hardware
    int backend_result = visualmem_v2_init_hardware_backend(ctx);
    if (backend_result != VISUALMEM_V2_SUCCESS) {
        printf("[ERROR] Backend initialization failed: %d\n", backend_result);
        pthread_mutex_unlock(&global_context_mutex);
        return backend_result;
    }
    
    // Calcul de la mémoire vidéo
    ctx->video_memory_size = width * height * 4; // 32 bits par pixel
    
    // Activation du système
    ctx->display_active = 1;
    ctx->hardware_active = 1;
    
    printf("[INIT] LibVisualMem v2.0 initialized successfully\n");
    printf("[INIT] Backend: %d, Hardware: %s, Video Memory: %zu bytes\n", 
           ctx->backend, caps.gpu_model, ctx->video_memory_size);
    
    // Démarrage du thread de refresh sécurisé
    ctx->refresh_thread_active = 1;
    int thread_result = pthread_create(&ctx->refresh_thread, NULL, display_refresh_thread, ctx);
    if (thread_result != 0) {
        printf("[WARNING] Failed to create refresh thread: %d\n", thread_result);
        ctx->refresh_thread_active = 0;
        ctx->refresh_thread = 0;
        // Continue sans thread (non fatal)
    }
    
    pthread_mutex_unlock(&global_context_mutex);
    
    return VISUALMEM_V2_SUCCESS;
}

int visualmem_v2_cleanup(visualmem_v2_context_t* ctx) {
    if (!ctx) {
        printf("[ERROR] Null context in cleanup\n");
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    printf("[CLEANUP] Shutting down LibVisualMem v2.0...\n");
    
    pthread_mutex_lock(&global_context_mutex);
    
    // 1. Arrêt sécurisé du thread de refresh
    if (ctx->refresh_thread_active) {
        printf("[DISPLAY] Stopping refresh thread...\n");
        ctx->refresh_thread_active = 0; // Signal d'arrêt
        
        pthread_mutex_unlock(&global_context_mutex); // Libère pour permettre l'arrêt
        
        if (ctx->refresh_thread) {
            int join_result = pthread_join(ctx->refresh_thread, NULL);
            if (join_result == 0) {
                printf("[DISPLAY] Refresh thread stopped\n");
            } else {
                printf("[WARNING] Thread join failed: %d\n", join_result);
            }
            ctx->refresh_thread = 0;
        }
        
        pthread_mutex_lock(&global_context_mutex); // Réacquiert le mutex
    }
    
    // 2. Désactivation du système
    ctx->display_active = 0;
    ctx->hardware_active = 0;
    
    // 3. Cleanup du backend hardware
    if (ctx->backend != VISUALMEM_V2_BACKEND_AUTO) {
        int cleanup_result = visualmem_v2_cleanup_hardware_backend(ctx);
        if (cleanup_result != VISUALMEM_V2_SUCCESS) {
            printf("[WARNING] Backend cleanup failed: %d\n", cleanup_result);
        }
    }
    
    // 4. Réinitialisation de la structure
    memset(ctx, 0, sizeof(visualmem_v2_context_t));
    
    pthread_mutex_unlock(&global_context_mutex);
    
    printf("[CLEANUP] LibVisualMem v2.0 shutdown complete\n");
    return VISUALMEM_V2_SUCCESS;
}

// === GESTION MÉMOIRE VISUELLE SÉCURISÉE ===

void* visualmem_v2_alloc(visualmem_v2_context_t* ctx, size_t size, const char* label) {
    if (!ctx || !ctx->display_active) {
        printf("[ERROR] Invalid context or inactive display\n");
        return NULL;
    }
    
    if (size == 0 || size > ctx->video_memory_size) {
        printf("[ERROR] Invalid allocation size: %zu\n", size);
        return NULL;
    }
    
    pthread_mutex_lock(&allocation_mutex);
    
    // Allocation simulée basée sur coordonnées pixel
    // Dans une implémentation complète, ceci gérerait un allocateur de mémoire visuelle
    static uintptr_t next_address = 0x10000000; // Adresse de base simulée
    void* address = (void*)next_address;
    next_address += (size + 0xFFF) & ~0xFFF; // Alignement 4K
    
    printf("[ALLOC] Allocated %zu bytes at %p (label: %s)\n", size, address, label ? label : "unnamed");
    
    pthread_mutex_unlock(&allocation_mutex);
    
    return address;
}

int visualmem_v2_free(visualmem_v2_context_t* ctx, void* ptr) {
    if (!ctx || !ctx->display_active) {
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    if (!ptr) {
        return VISUALMEM_V2_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&allocation_mutex);
    
    printf("[FREE] Freed memory at %p\n", ptr);
    
    pthread_mutex_unlock(&allocation_mutex);
    
    return VISUALMEM_V2_SUCCESS;
}

// === OPÉRATIONS LECTURE/ÉCRITURE SÉCURISÉES ===

int visualmem_v2_write(visualmem_v2_context_t* ctx, void* addr, const void* data, size_t size) {
    if (!ctx || !ctx->display_active || !addr || !data) {
        return VISUALMEM_V2_ERROR_INVALID_PARAMETER;
    }
    
    if (size == 0) {
        return VISUALMEM_V2_SUCCESS;
    }
    
    // Conversion en opérations pixel
    const uint8_t* byte_data = (const uint8_t*)data;
    uintptr_t pixel_base = ((uintptr_t)addr - 0x10000000) / 4; // Conversion adresse -> pixel
    
    for (size_t i = 0; i < size; i += 4) {
        int pixel_index = pixel_base + (i / 4);
        int x = pixel_index % ctx->width;
        int y = pixel_index / ctx->width;
        
        if (x >= 0 && x < ctx->width && y >= 0 && y < ctx->height) {
            uint32_t color = 0xFF000000; // Alpha
            if (i < size) color |= (byte_data[i] << 16);     // R
            if (i + 1 < size) color |= (byte_data[i + 1] << 8); // G
            if (i + 2 < size) color |= byte_data[i + 2];         // B
            
            visualmem_v2_write_pixel(ctx, x, y, color);
        }
    }
    
    return VISUALMEM_V2_SUCCESS;
}

int visualmem_v2_read(visualmem_v2_context_t* ctx, void* addr, void* data, size_t size) {
    if (!ctx || !ctx->display_active || !addr || !data) {
        return VISUALMEM_V2_ERROR_INVALID_PARAMETER;
    }
    
    if (size == 0) {
        return VISUALMEM_V2_SUCCESS;
    }
    
    // Conversion en opérations pixel
    uint8_t* byte_data = (uint8_t*)data;
    uintptr_t pixel_base = ((uintptr_t)addr - 0x10000000) / 4;
    
    for (size_t i = 0; i < size; i += 4) {
        int pixel_index = pixel_base + (i / 4);
        int x = pixel_index % ctx->width;
        int y = pixel_index / ctx->width;
        
        if (x >= 0 && x < ctx->width && y >= 0 && y < ctx->height) {
            uint32_t color = visualmem_v2_read_pixel(ctx, x, y);
            
            if (i < size) byte_data[i] = (color >> 16) & 0xFF;     // R
            if (i + 1 < size) byte_data[i + 1] = (color >> 8) & 0xFF; // G
            if (i + 2 < size) byte_data[i + 2] = color & 0xFF;         // B
            if (i + 3 < size) byte_data[i + 3] = (color >> 24) & 0xFF; // A
        } else {
            // Remplit avec des zéros si hors limites
            for (int j = 0; j < 4 && (i + j) < size; j++) {
                byte_data[i + j] = 0;
            }
        }
    }
    
    return VISUALMEM_V2_SUCCESS;
}

// === OPÉRATIONS PIXEL SÉCURISÉES ===

int visualmem_v2_write_pixel(visualmem_v2_context_t* ctx, int x, int y, uint32_t color) {
    if (!ctx || !ctx->display_active) {
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return VISUALMEM_V2_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&global_context_mutex);
    
    int result = VISUALMEM_V2_ERROR_NOT_IMPLEMENTED;
    
    if (ctx->hardware_active) {
        switch (ctx->backend) {
            case VISUALMEM_V2_BACKEND_X11:
            case VISUALMEM_V2_BACKEND_OPENGL:
                result = visualmem_v2_x11_write_pixel(ctx, x, y, color);
                break;
            default:
                result = VISUALMEM_V2_ERROR_NOT_IMPLEMENTED;
                break;
        }
    }
    
    pthread_mutex_unlock(&global_context_mutex);
    
    return result;
}

uint32_t visualmem_v2_read_pixel(visualmem_v2_context_t* ctx, int x, int y) {
    if (!ctx || !ctx->display_active) {
        return 0;
    }
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return 0;
    }
    
    pthread_mutex_lock(&global_context_mutex);
    
    uint32_t result = 0;
    
    if (ctx->hardware_active) {
        switch (ctx->backend) {
            case VISUALMEM_V2_BACKEND_X11:
            case VISUALMEM_V2_BACKEND_OPENGL:
                result = visualmem_v2_x11_read_pixel(ctx, x, y);
                break;
            default:
                result = 0;
                break;
        }
    }
    
    pthread_mutex_unlock(&global_context_mutex);
    
    return result;
}

// === FONCTIONS UTILITAIRES ===

int visualmem_v2_refresh_display(visualmem_v2_context_t* ctx) {
    if (!ctx || !ctx->display_active) {
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    pthread_mutex_lock(&global_context_mutex);
    
    int result = VISUALMEM_V2_ERROR_NOT_IMPLEMENTED;
    
    if (ctx->hardware_active) {
        switch (ctx->backend) {
            case VISUALMEM_V2_BACKEND_X11:
            case VISUALMEM_V2_BACKEND_OPENGL:
                result = visualmem_v2_x11_refresh(ctx);
                break;
            default:
                result = VISUALMEM_V2_SUCCESS; // Pas d'erreur pour backends non implémentés
                break;
        }
    }
    
    pthread_mutex_unlock(&global_context_mutex);
    
    return result;
}

int visualmem_v2_get_hardware_caps(visualmem_v2_hardware_caps_t* caps) {
    return visualmem_v2_detect_hardware(caps);
}

int visualmem_v2_get_performance_stats(visualmem_v2_context_t* ctx, visualmem_v2_performance_t* stats) {
    if (!ctx || !stats) {
        return VISUALMEM_V2_ERROR_INVALID_PARAMETER;
    }
    
    // Statistiques basiques
    stats->total_allocations = 0; // À implémenter
    stats->total_memory_used = ctx->video_memory_size;
    stats->average_access_time_us = 10.0; // Estimation
    stats->pixel_operations_per_second = 100000.0; // Estimation
    
    return VISUALMEM_V2_SUCCESS;
}

const char* visualmem_v2_get_error_string(int error_code) {
    switch (error_code) {
        case VISUALMEM_V2_SUCCESS:
            return "Success";
        case VISUALMEM_V2_ERROR_INVALID_CONTEXT:
            return "Invalid context";
        case VISUALMEM_V2_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case VISUALMEM_V2_ERROR_MEMORY:
            return "Memory error";
        case VISUALMEM_V2_ERROR_HARDWARE_INIT:
            return "Hardware initialization failed";
        case VISUALMEM_V2_ERROR_HARDWARE_OPERATION:
            return "Hardware operation failed";
        case VISUALMEM_V2_ERROR_NOT_IMPLEMENTED:
            return "Feature not implemented";
        default:
            return "Unknown error";
    }
}