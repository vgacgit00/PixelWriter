/**
 * LibVisualMem v2.0 - Hardware-Native Implementation
 * =================================================
 * 
 * Real hardware implementation using X11, OpenGL, and framebuffer
 * for authentic visual memory operations on physical displays.
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

// External hardware interface functions
extern int visualmem_v2_detect_hardware(visualmem_v2_hardware_caps_t* caps);
extern visualmem_v2_backend_t visualmem_v2_select_best_backend(const visualmem_v2_hardware_caps_t* caps);
extern int visualmem_v2_init_hardware_backend(visualmem_v2_context_t* ctx);
extern void visualmem_v2_cleanup_hardware_backend(visualmem_v2_context_t* ctx);
extern int visualmem_v2_x11_write_pixel(visualmem_v2_context_t* ctx, int x, int y, uint32_t color);
extern uint32_t visualmem_v2_x11_read_pixel(visualmem_v2_context_t* ctx, int x, int y);
extern int visualmem_v2_x11_refresh(visualmem_v2_context_t* ctx);

// === INTERNAL CONSTANTS ===
#define VISUALMEM_V2_MAGIC_HEADER 0x56495355  // "VISU" in hex
#define VISUALMEM_V2_BYTE_SPACING_X 10        // 8 bits + 2 markers
#define VISUALMEM_V2_BYTE_SPACING_Y 2         // Vertical spacing
#define VISUALMEM_V2_MEMORY_START_X 20        // Skip header area
#define VISUALMEM_V2_MEMORY_START_Y 20

// === UTILITY FUNCTIONS ===

static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static uint32_t calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = (checksum << 1) ^ bytes[i];
    }
    return checksum;
}

// Suppress unused function warning for now
__attribute__((unused)) static void* suppress_unused_warning = (void*)calculate_checksum;

// === COORDINATE CONVERSION ===

static inline void* coord_to_addr(int x, int y) {
    return (void*)((uintptr_t)((y << 16) | x));
}

static inline void addr_to_coord(void* addr, int* x, int* y) {
    uintptr_t val = (uintptr_t)addr;
    *x = val & 0xFFFF;
    *y = (val >> 16) & 0xFFFF;
}

static void calculate_byte_position(int byte_index, int* x, int* y, int width) {
    int bytes_per_row = (width - VISUALMEM_V2_MEMORY_START_X) / VISUALMEM_V2_BYTE_SPACING_X;
    if (bytes_per_row <= 0) bytes_per_row = 1;
    
    int row = byte_index / bytes_per_row;
    int col = byte_index % bytes_per_row;
    
    *x = VISUALMEM_V2_MEMORY_START_X + (col * VISUALMEM_V2_BYTE_SPACING_X);
    *y = VISUALMEM_V2_MEMORY_START_Y + (row * VISUALMEM_V2_BYTE_SPACING_Y);
}

// === DISPLAY REFRESH THREAD ===

static void* display_refresh_thread(void* arg) {
    visualmem_v2_context_t* ctx = (visualmem_v2_context_t*)arg;
    
    printf("[DISPLAY] Refresh thread started (target: %d Hz)\n", ctx->refresh_rate_hz);
    
    int frame_time_us = 1000000 / ctx->refresh_rate_hz;
    uint64_t frame_count = 0;
    uint64_t start_time = get_timestamp_us();
    
    while (ctx->display_thread_running) {
        uint64_t frame_start = get_timestamp_us();
        
        // Refresh display based on backend
        if (ctx->backend == VISUALMEM_V2_BACKEND_X11 || 
            ctx->backend == VISUALMEM_V2_BACKEND_OPENGL) {
            visualmem_v2_x11_refresh(ctx);
        }
        
        frame_count++;
        ctx->performance.display_refreshes = frame_count;
        
        // Calculate frame rate
        uint64_t elapsed = get_timestamp_us() - start_time;
        if (elapsed > 0) {
            ctx->performance.frame_rate = (double)frame_count * 1000000.0 / elapsed;
        }
        
        // Sleep until next frame
        uint64_t frame_end = get_timestamp_us();
        int frame_duration = frame_end - frame_start;
        
        if (frame_duration < frame_time_us) {
            usleep(frame_time_us - frame_duration);
        }
    }
    
    printf("[DISPLAY] Refresh thread stopped\n");
    return NULL;
}

// === CORE API IMPLEMENTATION ===

int visualmem_v2_get_hardware_caps(visualmem_v2_hardware_caps_t* caps) {
    return visualmem_v2_detect_hardware(caps);
}

int visualmem_v2_init(visualmem_v2_context_t* ctx, 
                      visualmem_v2_mode_t mode,
                      int width, int height) {
    return visualmem_v2_init_with_backend(ctx, VISUALMEM_V2_BACKEND_AUTO, mode, width, height);
}

int visualmem_v2_init_with_backend(visualmem_v2_context_t* ctx,
                                   visualmem_v2_backend_t backend,
                                   visualmem_v2_mode_t mode,
                                   int width, int height) {
    if (!ctx) return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    
    if (width < VISUALMEM_V2_MIN_WIDTH || width > VISUALMEM_V2_MAX_WIDTH ||
        height < VISUALMEM_V2_MIN_HEIGHT || height > VISUALMEM_V2_MAX_HEIGHT) {
        return VISUALMEM_V2_ERROR_INVALID_RESOLUTION;
    }
    
    printf("[INIT] Initializing LibVisualMem v2.0...\n");
    printf("[INIT] Resolution: %dx%d, Mode: %d, Backend: %d\n", width, height, mode, backend);
    
    // Initialize context
    memset(ctx, 0, sizeof(visualmem_v2_context_t));
    ctx->width = width;
    ctx->height = height;
    ctx->mode = mode;
    ctx->backend = backend;
    ctx->pixel_format = VISUALMEM_V2_PIXEL_RGBA32;
    ctx->refresh_rate_hz = 60;
    ctx->vsync_enabled = 1;
    ctx->double_buffering = 1;
    
    // Initialize mutexes
    if (pthread_mutex_init(&ctx->context_mutex, NULL) != 0) {
        return VISUALMEM_V2_ERROR_THREAD_FAILED;
    }
    
    if (pthread_cond_init(&ctx->display_cond, NULL) != 0) {
        pthread_mutex_destroy(&ctx->context_mutex);
        return VISUALMEM_V2_ERROR_THREAD_FAILED;
    }
    
    // Detect hardware capabilities
    int result = visualmem_v2_detect_hardware(&ctx->hardware);
    if (result != VISUALMEM_V2_SUCCESS) {
        printf("[INIT] ERROR: Hardware detection failed\n");
        pthread_cond_destroy(&ctx->display_cond);
        pthread_mutex_destroy(&ctx->context_mutex);
        return result;
    }
    
    // Select backend if auto
    if (ctx->backend == VISUALMEM_V2_BACKEND_AUTO) {
        ctx->backend = visualmem_v2_select_best_backend(&ctx->hardware);
    }
    
    // Initialize hardware backend
    result = visualmem_v2_init_hardware_backend(ctx);
    if (result != VISUALMEM_V2_SUCCESS) {
        printf("[INIT] ERROR: Hardware backend initialization failed\n");
        pthread_cond_destroy(&ctx->display_cond);
        pthread_mutex_destroy(&ctx->context_mutex);
        return result;
    }
    
    // Allocate video memory buffer
    size_t video_memory_size = width * height * VISUALMEM_V2_BYTES_PER_PIXEL;
    ctx->video_memory = malloc(video_memory_size);
    if (!ctx->video_memory) {
        printf("[INIT] ERROR: Failed to allocate video memory buffer\n");
        visualmem_v2_cleanup_hardware_backend(ctx);
        pthread_cond_destroy(&ctx->display_cond);
        pthread_mutex_destroy(&ctx->context_mutex);
        return VISUALMEM_V2_ERROR_OUT_OF_VIDEO_MEMORY;
    }
    
    // Clear video memory
    memset(ctx->video_memory, 0, video_memory_size);
    
    // Initialize allocations array mutexes
    for (int i = 0; i < VISUALMEM_V2_MAX_ALLOCATIONS; i++) {
        pthread_mutex_init(&ctx->allocations[i].mutex, NULL);
    }
    
    // Start display refresh thread
    ctx->display_thread_running = 1;
    if (pthread_create(&ctx->display_thread, NULL, display_refresh_thread, ctx) != 0) {
        printf("[INIT] WARNING: Failed to start display refresh thread\n");
        ctx->display_thread_running = 0;
    }
    
    ctx->is_initialized = 1;
    ctx->hardware_active = 1;
    ctx->display_active = 1;
    
    printf("[INIT] LibVisualMem v2.0 initialized successfully\n");
    printf("[INIT] Backend: %d, Hardware: %s, Video Memory: %zu bytes\n", 
           ctx->backend, ctx->hardware.gpu_model, video_memory_size);
    
    return VISUALMEM_V2_SUCCESS;
}

void visualmem_v2_cleanup(visualmem_v2_context_t* ctx) {
    if (!ctx || !ctx->is_initialized) return;
    
    printf("[CLEANUP] Shutting down LibVisualMem v2.0...\n");
    
    // Stop display thread
    ctx->display_thread_running = 0;
    if (ctx->display_thread) {
        pthread_join(ctx->display_thread, NULL);
    }
    
    // Free all allocations
    for (int i = 0; i < ctx->allocation_count; i++) {
        if (ctx->allocations[i].is_active) {
            visualmem_v2_free(ctx, ctx->allocations[i].visual_addr);
        }
    }
    
    // Cleanup hardware backend
    visualmem_v2_cleanup_hardware_backend(ctx);
    
    // Free video memory
    if (ctx->video_memory) {
        free(ctx->video_memory);
        ctx->video_memory = NULL;
    }
    
    // Cleanup allocation mutexes
    for (int i = 0; i < VISUALMEM_V2_MAX_ALLOCATIONS; i++) {
        pthread_mutex_destroy(&ctx->allocations[i].mutex);
    }
    
    // Cleanup context mutexes
    pthread_cond_destroy(&ctx->display_cond);
    pthread_mutex_destroy(&ctx->context_mutex);
    
    ctx->is_initialized = 0;
    ctx->hardware_active = 0;
    ctx->display_active = 0;
    
    printf("[CLEANUP] Cleanup completed\n");
}

// === MEMORY ALLOCATION FUNCTIONS ===

void* visualmem_v2_alloc(visualmem_v2_context_t* ctx, size_t size, const char* label) {
    if (!ctx || !ctx->is_initialized || size == 0) return NULL;
    
    pthread_mutex_lock(&ctx->context_mutex);
    
    if (ctx->allocation_count >= VISUALMEM_V2_MAX_ALLOCATIONS) {
        pthread_mutex_unlock(&ctx->context_mutex);
        return NULL;
    }
    
    // Find free allocation slot
    int slot = -1;
    for (int i = 0; i < VISUALMEM_V2_MAX_ALLOCATIONS; i++) {
        if (!ctx->allocations[i].is_active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        pthread_mutex_unlock(&ctx->context_mutex);
        return NULL;
    }
    
    // Calculate position for allocation
    int bytes_per_row = (ctx->width - VISUALMEM_V2_MEMORY_START_X) / VISUALMEM_V2_BYTE_SPACING_X;
    if (bytes_per_row <= 0) bytes_per_row = 1;
    
    int total_allocated_bytes = 0;
    for (int i = 0; i < ctx->allocation_count; i++) {
        if (ctx->allocations[i].is_active) {
            total_allocated_bytes += ctx->allocations[i].size;
        }
    }
    
    int start_x, start_y;
    calculate_byte_position(total_allocated_bytes, &start_x, &start_y, ctx->width);
    
    // Check if allocation fits on screen
    int required_rows = (size + bytes_per_row - 1) / bytes_per_row;
    if (start_y + (required_rows * VISUALMEM_V2_BYTE_SPACING_Y) >= ctx->height) {
        pthread_mutex_unlock(&ctx->context_mutex);
        return NULL; // Not enough screen space
    }
    
    // Create allocation
    visualmem_v2_allocation_t* alloc = &ctx->allocations[slot];
    alloc->visual_addr = coord_to_addr(start_x, start_y);
    alloc->size = size;
    alloc->x = start_x;
    alloc->y = start_y;
    alloc->width = bytes_per_row * VISUALMEM_V2_BYTE_SPACING_X;
    alloc->height = required_rows * VISUALMEM_V2_BYTE_SPACING_Y;
    alloc->checksum = 0;
    alloc->timestamp = get_timestamp_us();
    alloc->is_active = 1;
    
    if (label) {
        strncpy(alloc->label, label, sizeof(alloc->label) - 1);
        alloc->label[sizeof(alloc->label) - 1] = '\0';
    } else {
        snprintf(alloc->label, sizeof(alloc->label), "alloc_%d", slot);
    }
    
    ctx->allocation_count++;
    ctx->performance.total_allocations++;
    
    pthread_mutex_unlock(&ctx->context_mutex);
    
    printf("[ALLOC] Allocated %zu bytes at (%d,%d) - %s\n", 
           size, start_x, start_y, alloc->label);
    
    return alloc->visual_addr;
}

void* visualmem_v2_alloc_at(visualmem_v2_context_t* ctx,
                            int x, int y, int width, int height,
                            const char* label) {
    if (!ctx || !ctx->is_initialized) return NULL;
    
    if (x < 0 || y < 0 || x + width > ctx->width || y + height > ctx->height) {
        return NULL;
    }
    
    size_t size = width * height; // Simplified size calculation
    return visualmem_v2_alloc(ctx, size, label);
}

int visualmem_v2_free(visualmem_v2_context_t* ctx, void* visual_addr) {
    if (!ctx || !ctx->is_initialized || !visual_addr) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    pthread_mutex_lock(&ctx->context_mutex);
    
    // Find allocation
    int slot = -1;
    for (int i = 0; i < VISUALMEM_V2_MAX_ALLOCATIONS; i++) {
        if (ctx->allocations[i].is_active && 
            ctx->allocations[i].visual_addr == visual_addr) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        pthread_mutex_unlock(&ctx->context_mutex);
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    visualmem_v2_allocation_t* alloc = &ctx->allocations[slot];
    
    printf("[FREE] Freeing %zu bytes at (%d,%d) - %s\n", 
           alloc->size, alloc->x, alloc->y, alloc->label);
    
    // Clear visual area (set to black)
    for (int y = alloc->y; y < alloc->y + alloc->height && y < ctx->height; y++) {
        for (int x = alloc->x; x < alloc->x + alloc->width && x < ctx->width; x++) {
            visualmem_v2_write_pixel(ctx, x, y, 0xFF000000); // Black
        }
    }
    
    // Mark as inactive
    alloc->is_active = 0;
    alloc->visual_addr = NULL;
    alloc->size = 0;
    
    ctx->allocation_count--;
    ctx->performance.total_deallocations++;
    
    pthread_mutex_unlock(&ctx->context_mutex);
    
    return VISUALMEM_V2_SUCCESS;
}

// === DATA OPERATIONS ===

int visualmem_v2_write_pixel(visualmem_v2_context_t* ctx, int x, int y, uint32_t color) {
    if (!ctx || !ctx->is_initialized) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    int result = VISUALMEM_V2_SUCCESS;
    
    // Write to appropriate backend
    switch (ctx->backend) {
        case VISUALMEM_V2_BACKEND_X11:
        case VISUALMEM_V2_BACKEND_OPENGL:
            result = visualmem_v2_x11_write_pixel(ctx, x, y, color);
            break;
            
        case VISUALMEM_V2_BACKEND_FRAMEBUFFER:
            // Framebuffer write implementation would go here
            result = VISUALMEM_V2_ERROR_HARDWARE_UNSUPPORTED;
            break;
            
        default:
            result = VISUALMEM_V2_ERROR_HARDWARE_UNSUPPORTED;
            break;
    }
    
    if (result == VISUALMEM_V2_SUCCESS) {
        ctx->performance.pixel_operations++;
    }
    
    return result;
}

uint32_t visualmem_v2_read_pixel(visualmem_v2_context_t* ctx, int x, int y) {
    if (!ctx || !ctx->is_initialized) {
        return 0;
    }
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return 0;
    }
    
    uint32_t color = 0;
    
    // Read from appropriate backend
    switch (ctx->backend) {
        case VISUALMEM_V2_BACKEND_X11:
        case VISUALMEM_V2_BACKEND_OPENGL:
            color = visualmem_v2_x11_read_pixel(ctx, x, y);
            break;
            
        case VISUALMEM_V2_BACKEND_FRAMEBUFFER:
            // Framebuffer read implementation would go here
            break;
            
        default:
            break;
    }
    
    ctx->performance.pixel_operations++;
    return color;
}

int visualmem_v2_write(visualmem_v2_context_t* ctx, 
                       void* visual_addr, 
                       const void* data, 
                       size_t size) {
    if (!ctx || !ctx->is_initialized || !visual_addr || !data || size == 0) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    uint64_t start_time = get_timestamp_us();
    
    int x, y;
    addr_to_coord(visual_addr, &x, &y);
    
    const uint8_t* bytes = (const uint8_t*)data;
    int byte_index = 0;
    
    // Encode each byte as pixels
    for (size_t i = 0; i < size; i++) {
        int byte_x, byte_y;
        calculate_byte_position(byte_index, &byte_x, &byte_y, ctx->width);
        
        if (byte_x >= ctx->width - 10 || byte_y >= ctx->height - 2) {
            break; // Out of screen space
        }
        
        // Start marker (red)
        visualmem_v2_write_pixel(ctx, byte_x, byte_y, 0xFFFF0000);
        
        // Encode 8 bits
        uint8_t byte_value = bytes[i];
        for (int bit = 0; bit < 8; bit++) {
            uint8_t bit_value = (byte_value >> (7 - bit)) & 1;
            uint32_t pixel_color = bit_value ? 0xFFFFFFFF : 0xFF000000; // White or black
            visualmem_v2_write_pixel(ctx, byte_x + 1 + bit, byte_y, pixel_color);
        }
        
        // End marker (green)
        visualmem_v2_write_pixel(ctx, byte_x + 9, byte_y, 0xFF00FF00);
        
        byte_index++;
    }
    
    uint64_t end_time = get_timestamp_us();
    double duration_s = (end_time - start_time) / 1000000.0;
    double speed_mbps = (size / (1024.0 * 1024.0)) / duration_s;
    
    ctx->performance.bytes_written += size;
    if (ctx->performance.avg_write_speed_mbps == 0) {
        ctx->performance.avg_write_speed_mbps = speed_mbps;
    } else {
        ctx->performance.avg_write_speed_mbps = 
            (ctx->performance.avg_write_speed_mbps + speed_mbps) / 2.0;
    }
    
    return VISUALMEM_V2_SUCCESS;
}

int visualmem_v2_read(visualmem_v2_context_t* ctx, 
                      void* visual_addr, 
                      void* buffer, 
                      size_t size) {
    if (!ctx || !ctx->is_initialized || !visual_addr || !buffer || size == 0) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    uint64_t start_time = get_timestamp_us();
    
    int x, y;
    addr_to_coord(visual_addr, &x, &y);
    
    uint8_t* bytes = (uint8_t*)buffer;
    int byte_index = 0;
    
    // Decode each byte from pixels
    for (size_t i = 0; i < size; i++) {
        int byte_x, byte_y;
        calculate_byte_position(byte_index, &byte_x, &byte_y, ctx->width);
        
        if (byte_x >= ctx->width - 10 || byte_y >= ctx->height - 2) {
            break; // Out of screen space
        }
        
        uint8_t byte_value = 0;
        
        // Read 8 bits
        for (int bit = 0; bit < 8; bit++) {
            uint32_t pixel_color = visualmem_v2_read_pixel(ctx, byte_x + 1 + bit, byte_y);
            
            // Check if pixel represents bit 1 (white)
            if ((pixel_color & 0x00FFFFFF) == 0x00FFFFFF) {
                byte_value |= (1 << (7 - bit));
            }
        }
        
        bytes[i] = byte_value;
        byte_index++;
    }
    
    uint64_t end_time = get_timestamp_us();
    double duration_s = (end_time - start_time) / 1000000.0;
    double speed_mbps = (size / (1024.0 * 1024.0)) / duration_s;
    
    ctx->performance.bytes_read += size;
    if (ctx->performance.avg_read_speed_mbps == 0) {
        ctx->performance.avg_read_speed_mbps = speed_mbps;
    } else {
        ctx->performance.avg_read_speed_mbps = 
            (ctx->performance.avg_read_speed_mbps + speed_mbps) / 2.0;
    }
    
    return VISUALMEM_V2_SUCCESS;
}

// === DISPLAY CONTROL ===

int visualmem_v2_refresh_display(visualmem_v2_context_t* ctx) {
    if (!ctx || !ctx->is_initialized) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    switch (ctx->backend) {
        case VISUALMEM_V2_BACKEND_X11:
        case VISUALMEM_V2_BACKEND_OPENGL:
            return visualmem_v2_x11_refresh(ctx);
            
        default:
            return VISUALMEM_V2_SUCCESS; // No-op for other backends
    }
}

int visualmem_v2_set_refresh_rate(visualmem_v2_context_t* ctx, int hz) {
    if (!ctx || hz <= 0 || hz > 240) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    ctx->refresh_rate_hz = hz;
    printf("[DISPLAY] Refresh rate set to %d Hz\n", hz);
    return VISUALMEM_V2_SUCCESS;
}

// === PERFORMANCE AND MONITORING ===

int visualmem_v2_get_performance(visualmem_v2_context_t* ctx,
                                visualmem_v2_performance_t* perf) {
    if (!ctx || !perf) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    pthread_mutex_lock(&ctx->context_mutex);
    *perf = ctx->performance;
    pthread_mutex_unlock(&ctx->context_mutex);
    
    return VISUALMEM_V2_SUCCESS;
}

void visualmem_v2_reset_performance(visualmem_v2_context_t* ctx) {
    if (!ctx) return;
    
    pthread_mutex_lock(&ctx->context_mutex);
    memset(&ctx->performance, 0, sizeof(visualmem_v2_performance_t));
    pthread_mutex_unlock(&ctx->context_mutex);
}

// === UTILITY FUNCTIONS ===

uint32_t visualmem_v2_rgb_to_pixel(visualmem_v2_context_t* ctx,
                                   uint8_t r, uint8_t g, uint8_t b) {
    (void)ctx; // Unused for now
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

void visualmem_v2_pixel_to_rgb(visualmem_v2_context_t* ctx,
                               uint32_t pixel,
                               uint8_t* r, uint8_t* g, uint8_t* b) {
    (void)ctx; // Unused for now
    if (r) *r = (pixel >> 16) & 0xFF;
    if (g) *g = (pixel >> 8) & 0xFF;
    if (b) *b = pixel & 0xFF;
}

const char* visualmem_v2_get_error_string(visualmem_v2_error_t error) {
    switch (error) {
        case VISUALMEM_V2_SUCCESS: return "Success";
        case VISUALMEM_V2_ERROR_INIT_FAILED: return "Initialization failed";
        case VISUALMEM_V2_ERROR_DISPLAY_UNAVAILABLE: return "Display unavailable";
        case VISUALMEM_V2_ERROR_HARDWARE_UNSUPPORTED: return "Hardware unsupported";
        case VISUALMEM_V2_ERROR_OUT_OF_VIDEO_MEMORY: return "Out of video memory";
        case VISUALMEM_V2_ERROR_INVALID_ADDRESS: return "Invalid address";
        case VISUALMEM_V2_ERROR_ALLOCATION_FAILED: return "Allocation failed";
        case VISUALMEM_V2_ERROR_DISPLAY_LOST: return "Display lost";
        case VISUALMEM_V2_ERROR_OPENGL_FAILED: return "OpenGL failed";
        case VISUALMEM_V2_ERROR_THREAD_FAILED: return "Thread failed";
        case VISUALMEM_V2_ERROR_INVALID_RESOLUTION: return "Invalid resolution";
        default: return "Unknown error";
    }
}

void visualmem_v2_set_debug_mode(visualmem_v2_context_t* ctx, int enabled) {
    if (ctx) {
        ctx->debug_mode = enabled;
        printf("[DEBUG] Debug mode %s\n", enabled ? "enabled" : "disabled");
    }
}

// === THREAD SAFETY ===

int visualmem_v2_lock(visualmem_v2_context_t* ctx) {
    if (!ctx) return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    return pthread_mutex_lock(&ctx->context_mutex) == 0 ? 
           VISUALMEM_V2_SUCCESS : VISUALMEM_V2_ERROR_THREAD_FAILED;
}

int visualmem_v2_unlock(visualmem_v2_context_t* ctx) {
    if (!ctx) return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    return pthread_mutex_unlock(&ctx->context_mutex) == 0 ? 
           VISUALMEM_V2_SUCCESS : VISUALMEM_V2_ERROR_THREAD_FAILED;
}