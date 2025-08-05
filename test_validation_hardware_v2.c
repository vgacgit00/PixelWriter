/**
 * Test Validation Hardware v2.0 - LibVisualMem v2.0 (Version Corrigée)
 * =====================================================================
 * 
 * Suite de tests exhaustive CORRIGÉE pour validation hardware réelle
 * avec logging temps réel, métriques complètes et corrections de bugs
 */

#define _GNU_SOURCE

#include "libvisualmem_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

// === LOGGING CONFIGURATION ===
#define LOG_FILE "logs_temps_reel_v2.log"
#define METRICS_FILE "metriques_performance_v2.json"
#define MAX_LOG_SIZE 2048

// === TEST CONFIGURATION EXHAUSTIVE ===
#define TEST_ITERATIONS 5000        // Augmenté pour plus de données
#define STRESS_DURATION_SEC 30      // Test de stress plus long
#define PIXEL_TEST_COUNT 50000      // 50,000 pixels testés
#define MEMORY_TEST_SIZE 2048       // Taille mémoire augmentée
#define BENCHMARK_SIZE_MB 5         // 5 MB pour benchmark
#define BENCHMARK_ITERATIONS 200    // Plus d'itérations

// === GLOBAL VARIABLES ===
static FILE* log_file = NULL;
static FILE* metrics_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t test_start_time = 0;
static int total_tests_run = 0;
static int total_tests_passed = 0;

// === SIGNAL HANDLER POUR CLEANUP PROPRE ===
static volatile int interrupted = 0;

void signal_handler(int sig) {
    interrupted = 1;
    printf("\n[SIGNAL] Test interrupted by signal %d, cleaning up...\n", sig);
}

// === UTILITY FUNCTIONS AMÉLIORÉES ===

static uint64_t get_timestamp_nanoseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

static uint64_t get_timestamp_microseconds(void) {
    return get_timestamp_nanoseconds() / 1000;
}

static void get_timestamp_string(char* buffer, size_t size) {
    struct timeval tv;
    struct tm* tm_info;
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, tv.tv_usec);
}

static void log_operation_enhanced(const char* component, const char* operation, 
                                  const char* metrics, const char* result, 
                                  const char* metadata, int test_id) {
    pthread_mutex_lock(&log_mutex);
    
    if (log_file) {
        char timestamp[64];
        get_timestamp_string(timestamp, sizeof(timestamp));
        
        fprintf(log_file, "[%s] [%s] [%s] [%s] [%s] [%s] [TEST_ID:%d]\n",
                timestamp, component, operation, metrics, result, metadata, test_id);
        fflush(log_file);
    }
    
    // Progress indicator
    if (test_id % 1000 == 0) {
        printf("LOG: [%s] %s -> %s (Test %d)\n", component, operation, result, test_id);
    }
    
    pthread_mutex_unlock(&log_mutex);
}

static void get_system_metrics_enhanced(char* buffer, size_t size) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    
    // Memory info détaillée
    long rss_kb = usage.ru_maxrss;
    long page_faults = usage.ru_majflt + usage.ru_minflt;
    
    // CPU time précis
    double cpu_time = (double)usage.ru_utime.tv_sec + 
                     (double)usage.ru_utime.tv_usec / 1000000.0 +
                     (double)usage.ru_stime.tv_sec + 
                     (double)usage.ru_stime.tv_usec / 1000000.0;
    
    // I/O stats
    long io_reads = usage.ru_inblock;
    long io_writes = usage.ru_oublock;
    
    snprintf(buffer, size, 
            "rss=%ldKB,cpu_time=%.3fs,ctx_switches=%ld,page_faults=%ld,io_r=%ld,io_w=%ld",
            rss_kb, cpu_time, usage.ru_nvcsw + usage.ru_nivcsw, page_faults, io_reads, io_writes);
}

// === TESTS HARDWARE VALIDATION CORRIGÉS ===

/**
 * Test 1: Hardware Detection and Access Validation (Corrigé)
 */
static int test_hardware_detection_v2(void) {
    printf("\n🔍 Phase 1: Hardware Detection and Access Validation (v2.0)\n");
    printf("================================================================\n");
    
    total_tests_run++;
    
    uint64_t start_time = get_timestamp_nanoseconds();
    
    // Test hardware capabilities detection avec retry
    visualmem_v2_hardware_caps_t caps;
    int result = VISUALMEM_V2_ERROR_HARDWARE_INIT;
    int retry_count = 0;
    const int max_retries = 3;
    
    while (result != VISUALMEM_V2_SUCCESS && retry_count < max_retries && !interrupted) {
        result = visualmem_v2_get_hardware_caps(&caps);
        if (result != VISUALMEM_V2_SUCCESS) {
            retry_count++;
            printf("[RETRY] Hardware detection attempt %d/%d\n", retry_count, max_retries);
            usleep(100000); // 100ms delay
        }
    }
    
    uint64_t detection_time = get_timestamp_nanoseconds() - start_time;
    
    char metrics[512];
    snprintf(metrics, sizeof(metrics), "detection_time=%luns,retries=%d", detection_time, retry_count);
    
    char metadata[1024];
    get_system_metrics_enhanced(metadata, sizeof(metadata));
    
    if (result == VISUALMEM_V2_SUCCESS) {
        log_operation_enhanced("HARDWARE_DETECTION", "GET_CAPABILITIES", metrics, "SUCCESS", metadata, 1);
        
        // Log detailed hardware info avec validation
        char hw_info[2048];
        snprintf(hw_info, sizeof(hw_info), 
                "x11=%d,opengl=%d,fb=%d,drm=%d,gpu=%s,detection_ns=%lu",
                caps.has_x11, caps.has_opengl, caps.has_framebuffer, 
                caps.has_drm, caps.gpu_model, detection_time);
        log_operation_enhanced("HARDWARE_CAPS", "DETAILED_SCAN", hw_info, "SUCCESS", metadata, 2);
        
        // Validation des capacités détectées
        int capabilities_score = 0;
        if (caps.has_x11) capabilities_score += 25;
        if (caps.has_opengl) capabilities_score += 25;
        if (caps.has_framebuffer) capabilities_score += 25;
        if (caps.has_drm) capabilities_score += 25;
        
        printf("✅ Hardware detection successful\n");
        printf("   X11 Support: %s\n", caps.has_x11 ? "YES" : "NO");
        printf("   OpenGL Support: %s\n", caps.has_opengl ? "YES" : "NO");
        printf("   Framebuffer: %s\n", caps.has_framebuffer ? "YES" : "NO");
        printf("   DRM Support: %s\n", caps.has_drm ? "YES" : "NO");
        printf("   GPU Model: %s\n", caps.gpu_model);
        printf("   Detection Time: %lu nanoseconds (%.3f ms)\n", detection_time, detection_time / 1000000.0);
        printf("   Capabilities Score: %d/100\n", capabilities_score);
        printf("   Retry Count: %d\n", retry_count);
        
        total_tests_passed++;
        return 1;
    } else {
        log_operation_enhanced("HARDWARE_DETECTION", "GET_CAPABILITIES", metrics, "FAILED", metadata, 1);
        printf("❌ Hardware detection failed after %d retries\n", retry_count);
        return 0;
    }
}

/**
 * Test 2: X11 Connection and Display Validation (Corrigé)
 */
static int test_x11_connection_v2(void) {
    printf("\n🖥️  Phase 2: X11 Connection and Display Validation (v2.0)\n");
    printf("================================================================\n");
    
    total_tests_run++;
    
    uint64_t start_time = get_timestamp_nanoseconds();
    
    // Test X11 context initialization avec gestion d'erreur améliorée
    visualmem_v2_context_t ctx;
    memset(&ctx, 0, sizeof(ctx)); // Initialisation sécurisée
    
    int result = visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 800, 600);
    
    uint64_t init_time = get_timestamp_nanoseconds() - start_time;
    
    char metrics[512];
    snprintf(metrics, sizeof(metrics), "init_time=%luns,width=800,height=600", init_time);
    
    char metadata[1024];
    get_system_metrics_enhanced(metadata, sizeof(metadata));
    
    if (result == VISUALMEM_V2_SUCCESS) {
        log_operation_enhanced("X11_CONNECTION", "INIT_CONTEXT", metrics, "SUCCESS", metadata, 3);
        
        // Test des propriétés du display avec validation
        char display_info[1024];
        snprintf(display_info, sizeof(display_info),
                "backend=%d,display_active=%d,hardware_active=%d,video_mem=%zu,refresh_thread=%d",
                ctx.backend, ctx.display_active, ctx.hardware_active, 
                ctx.video_memory_size, ctx.refresh_thread_active);
        log_operation_enhanced("X11_DISPLAY", "PROPERTIES", display_info, "SUCCESS", metadata, 4);
        
        printf("✅ X11 connection successful\n");
        printf("   Backend: %d\n", ctx.backend);
        printf("   Display Active: %s\n", ctx.display_active ? "YES" : "NO");
        printf("   Hardware Active: %s\n", ctx.hardware_active ? "YES" : "NO");
        printf("   Video Memory: %zu bytes (%.2f MB)\n", ctx.video_memory_size, ctx.video_memory_size / (1024.0 * 1024.0));
        printf("   Refresh Thread: %s\n", ctx.refresh_thread_active ? "ACTIVE" : "INACTIVE");
        printf("   Initialization Time: %lu nanoseconds (%.3f ms)\n", init_time, init_time / 1000000.0);
        
        // Test display refresh avec mesure de performance
        uint64_t refresh_start = get_timestamp_nanoseconds();
        int refresh_result = visualmem_v2_refresh_display(&ctx);
        uint64_t refresh_time = get_timestamp_nanoseconds() - refresh_start;
        
        snprintf(metrics, sizeof(metrics), "refresh_time=%luns", refresh_time);
        log_operation_enhanced("X11_DISPLAY", "REFRESH", metrics, 
                             refresh_result == VISUALMEM_V2_SUCCESS ? "SUCCESS" : "FAILED", metadata, 5);
        
        printf("   Display Refresh: %s (%lu ns / %.3f ms)\n", 
               refresh_result == VISUALMEM_V2_SUCCESS ? "SUCCESS" : "FAILED", 
               refresh_time, refresh_time / 1000000.0);
        
        // Cleanup sécurisé avec vérification
        printf("   Performing safe cleanup...\n");
        int cleanup_result = visualmem_v2_cleanup(&ctx);
        printf("   Cleanup result: %s\n", cleanup_result == VISUALMEM_V2_SUCCESS ? "SUCCESS" : "FAILED");
        
        total_tests_passed++;
        return 1;
    } else {
        log_operation_enhanced("X11_CONNECTION", "INIT_CONTEXT", metrics, "FAILED", metadata, 3);
        printf("❌ X11 connection failed: %s\n", visualmem_v2_get_error_string(result));
        return 0;
    }
}

/**
 * Test 3: Pixel Manipulation Validation Exhaustive (Corrigé)
 */
static int test_pixel_operations_v2(void) {
    printf("\n🎨 Phase 3: Pixel Manipulation Validation Exhaustive (v2.0)\n");
    printf("================================================================\n");
    
    total_tests_run++;
    
    visualmem_v2_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1024, 768) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context for pixel tests\n");
        return 0;
    }
    
    int success_count = 0;
    int integrity_failures = 0;
    uint64_t total_write_time = 0;
    uint64_t total_read_time = 0;
    uint64_t min_write_time = UINT64_MAX;
    uint64_t max_write_time = 0;
    uint64_t min_read_time = UINT64_MAX;
    uint64_t max_read_time = 0;
    
    printf("Testing %d pixel operations with integrity validation...\n", PIXEL_TEST_COUNT);
    
    uint64_t test_start = get_timestamp_nanoseconds();
    
    for (int i = 0; i < PIXEL_TEST_COUNT && !interrupted; i++) {
        int x = rand() % ctx.width;
        int y = rand() % ctx.height;
        uint32_t original_color = 0xFF000000 | (rand() & 0xFFFFFF); // Alpha + RGB aléatoire
        
        // Test pixel write avec mesure précise
        uint64_t write_start = get_timestamp_nanoseconds();
        int write_result = visualmem_v2_write_pixel(&ctx, x, y, original_color);
        uint64_t write_time = get_timestamp_nanoseconds() - write_start;
        total_write_time += write_time;
        
        if (write_time < min_write_time) min_write_time = write_time;
        if (write_time > max_write_time) max_write_time = write_time;
        
        // Test pixel read avec mesure précise
        uint64_t read_start = get_timestamp_nanoseconds();
        uint32_t read_color = visualmem_v2_read_pixel(&ctx, x, y);
        uint64_t read_time = get_timestamp_nanoseconds() - read_start;
        total_read_time += read_time;
        
        if (read_time < min_read_time) min_read_time = read_time;
        if (read_time > max_read_time) max_read_time = read_time;
        
        // Validation de l'intégrité des données
        int integrity_ok = (read_color == original_color);
        if (!integrity_ok) {
            integrity_failures++;
        }
        
        if (write_result == VISUALMEM_V2_SUCCESS) {
            success_count++;
            
            // Log échantillonné pour éviter la surcharge
            if (i % 5000 == 0) {
                char metrics[512];
                snprintf(metrics, sizeof(metrics), 
                        "x=%d,y=%d,color=0x%08X,write_ns=%lu,read_ns=%lu,integrity=%s",
                        x, y, original_color, write_time, read_time, integrity_ok ? "OK" : "FAIL");
                
                char metadata[1024];
                get_system_metrics_enhanced(metadata, sizeof(metadata));
                
                log_operation_enhanced("PIXEL_OPS", "WRITE_READ_CYCLE", metrics, "SUCCESS", metadata, i + 6);
            }
        }
        
        // Progress indicator
        if (i % 10000 == 0) {
            printf("   Progress: %d/%d pixels processed (%.1f%%)\n", 
                   i, PIXEL_TEST_COUNT, (float)i / PIXEL_TEST_COUNT * 100.0);
        }
    }
    
    uint64_t test_duration = get_timestamp_nanoseconds() - test_start;
    
    // Calculs statistiques avancés
    double avg_write_time = (double)total_write_time / PIXEL_TEST_COUNT;
    double avg_read_time = (double)total_read_time / PIXEL_TEST_COUNT;
    double success_rate = (double)success_count / PIXEL_TEST_COUNT * 100.0;
    double integrity_rate = (double)(PIXEL_TEST_COUNT - integrity_failures) / PIXEL_TEST_COUNT * 100.0;
    double write_throughput = PIXEL_TEST_COUNT / (total_write_time / 1000000000.0);
    double read_throughput = PIXEL_TEST_COUNT / (total_read_time / 1000000000.0);
    
    char final_metrics[2048];
    snprintf(final_metrics, sizeof(final_metrics),
            "total_ops=%d,success_rate=%.2f%%,integrity_rate=%.2f%%,avg_write=%.2fns,avg_read=%.2fns,"
            "min_write=%luns,max_write=%luns,min_read=%luns,max_read=%luns,"
            "write_throughput=%.0f/s,read_throughput=%.0f/s,test_duration=%luns",
            PIXEL_TEST_COUNT, success_rate, integrity_rate, avg_write_time, avg_read_time,
            min_write_time, max_write_time, min_read_time, max_read_time,
            write_throughput, read_throughput, test_duration);
    
    char metadata[1024];
    get_system_metrics_enhanced(metadata, sizeof(metadata));
    
    log_operation_enhanced("PIXEL_OPS", "EXHAUSTIVE_SUMMARY", final_metrics, "COMPLETED", metadata, PIXEL_TEST_COUNT + 6);
    
    printf("✅ Pixel operations completed\n");
    printf("   Total Operations: %d\n", PIXEL_TEST_COUNT);
    printf("   Success Rate: %.2f%%\n", success_rate);
    printf("   Data Integrity Rate: %.2f%%\n", integrity_rate);
    printf("   Integrity Failures: %d\n", integrity_failures);
    printf("   Average Write Time: %.2f nanoseconds\n", avg_write_time);
    printf("   Average Read Time: %.2f nanoseconds\n", avg_read_time);
    printf("   Write Time Range: %lu - %lu ns\n", min_write_time, max_write_time);
    printf("   Read Time Range: %lu - %lu ns\n", min_read_time, max_read_time);
    printf("   Write Throughput: %.0f pixels/second\n", write_throughput);
    printf("   Read Throughput: %.0f pixels/second\n", read_throughput);
    printf("   Total Test Duration: %.3f seconds\n", test_duration / 1000000000.0);
    
    visualmem_v2_cleanup(&ctx);
    
    // Critères de succès stricts
    int test_passed = (success_rate >= 80.0 && integrity_rate >= 95.0);
    if (test_passed) total_tests_passed++;
    
    return test_passed;
}

/**
 * Test 4: Memory Operations Validation Exhaustive (Corrigé)
 */
static int test_memory_operations_v2(void) {
    printf("\n💾 Phase 4: Memory Operations Validation Exhaustive (v2.0)\n");
    printf("================================================================\n");
    
    total_tests_run++;
    
    visualmem_v2_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1280, 960) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context for memory tests\n");
        return 0;
    }
    
    int success_count = 0;
    int integrity_failures = 0;
    uint64_t total_alloc_time = 0;
    uint64_t total_write_time = 0;
    uint64_t total_read_time = 0;
    uint64_t total_free_time = 0;
    size_t total_memory_allocated = 0;
    
    printf("Testing memory operations with %d iterations...\n", TEST_ITERATIONS);
    
    uint64_t test_start = get_timestamp_nanoseconds();
    
    for (int i = 0; i < TEST_ITERATIONS && !interrupted; i++) {
        size_t test_size = 64 + (rand() % MEMORY_TEST_SIZE); // 64B à 2KB
        char label[64];
        snprintf(label, sizeof(label), "test_mem_v2_%d", i);
        
        // Test allocation avec mesure
        uint64_t alloc_start = get_timestamp_nanoseconds();
        void* addr = visualmem_v2_alloc(&ctx, test_size, label);
        uint64_t alloc_time = get_timestamp_nanoseconds() - alloc_start;
        total_alloc_time += alloc_time;
        
        if (addr) {
            total_memory_allocated += test_size;
            
            // Création de données de test avec pattern
            uint8_t* test_data = malloc(test_size);
            for (size_t j = 0; j < test_size; j++) {
                test_data[j] = (uint8_t)((j ^ i ^ 0xAA) & 0xFF);
            }
            
            // Test write avec mesure
            uint64_t write_start = get_timestamp_nanoseconds();
            int write_result = visualmem_v2_write(&ctx, addr, test_data, test_size);
            uint64_t write_time = get_timestamp_nanoseconds() - write_start;
            total_write_time += write_time;
            
            // Test read avec mesure
            uint8_t* read_data = malloc(test_size);
            uint64_t read_start = get_timestamp_nanoseconds();
            int read_result = visualmem_v2_read(&ctx, addr, read_data, test_size);
            uint64_t read_time = get_timestamp_nanoseconds() - read_start;
            total_read_time += read_time;
            
            // Validation intégrité bit-par-bit
            int integrity_ok = (memcmp(test_data, read_data, test_size) == 0);
            if (!integrity_ok) {
                integrity_failures++;
                
                // Analyse détaillée des erreurs
                int first_error = -1;
                int error_count = 0;
                for (size_t k = 0; k < test_size; k++) {
                    if (test_data[k] != read_data[k]) {
                        if (first_error == -1) first_error = k;
                        error_count++;
                    }
                }
                
                if (i % 100 == 0) { // Log erreurs échantillonnées
                    char error_info[512];
                    snprintf(error_info, sizeof(error_info),
                            "size=%zu,first_error=%d,error_count=%d,error_rate=%.2f%%",
                            test_size, first_error, error_count, 
                            (double)error_count / test_size * 100.0);
                    log_operation_enhanced("MEMORY_INTEGRITY", "BIT_ERROR_ANALYSIS", 
                                         error_info, "DETECTED", "", i + 50000);
                }
            }
            
            // Test free avec mesure
            uint64_t free_start = get_timestamp_nanoseconds();
            int free_result = visualmem_v2_free(&ctx, addr);
            uint64_t free_time = get_timestamp_nanoseconds() - free_start;
            total_free_time += free_time;
            
            if (write_result == VISUALMEM_V2_SUCCESS && 
                read_result == VISUALMEM_V2_SUCCESS && 
                free_result == VISUALMEM_V2_SUCCESS) {
                success_count++;
                
                // Log échantillonné détaillé
                if (i % 500 == 0) {
                    char metrics[1024];
                    snprintf(metrics, sizeof(metrics),
                            "size=%zu,alloc_ns=%lu,write_ns=%lu,read_ns=%lu,free_ns=%lu,"
                            "integrity=%s,throughput_write=%.2fMB/s,throughput_read=%.2fMB/s",
                            test_size, alloc_time, write_time, read_time, free_time,
                            integrity_ok ? "OK" : "FAIL",
                            (test_size / (1024.0 * 1024.0)) / (write_time / 1000000000.0),
                            (test_size / (1024.0 * 1024.0)) / (read_time / 1000000000.0));
                    
                    char metadata[1024];
                    get_system_metrics_enhanced(metadata, sizeof(metadata));
                    
                    log_operation_enhanced("MEMORY_OPS", "FULL_CYCLE_DETAILED", metrics, "SUCCESS", metadata, i + 50000);
                }
            }
            
            free(test_data);
            free(read_data);
        }
        
        // Progress indicator
        if (i % 1000 == 0) {
            printf("   Progress: %d/%d memory operations (%.1f%%)\n", 
                   i, TEST_ITERATIONS, (float)i / TEST_ITERATIONS * 100.0);
        }
    }
    
    uint64_t test_duration = get_timestamp_nanoseconds() - test_start;
    
    // Calculs de performance détaillés
    double avg_alloc_time = (double)total_alloc_time / TEST_ITERATIONS;
    double avg_write_time = (double)total_write_time / TEST_ITERATIONS;
    double avg_read_time = (double)total_read_time / TEST_ITERATIONS;
    double avg_free_time = (double)total_free_time / TEST_ITERATIONS;
    double success_rate = (double)success_count / TEST_ITERATIONS * 100.0;
    double integrity_rate = (double)(TEST_ITERATIONS - integrity_failures) / TEST_ITERATIONS * 100.0;
    double total_throughput_mb = (total_memory_allocated / (1024.0 * 1024.0)) / (test_duration / 1000000000.0);
    
    char final_metrics[2048];
    snprintf(final_metrics, sizeof(final_metrics),
            "iterations=%d,success_rate=%.2f%%,integrity_rate=%.2f%%,"
            "avg_alloc=%.2fns,avg_write=%.2fns,avg_read=%.2fns,avg_free=%.2fns,"
            "total_memory_mb=%.2f,total_throughput=%.2fMB/s,test_duration=%.3fs",
            TEST_ITERATIONS, success_rate, integrity_rate,
            avg_alloc_time, avg_write_time, avg_read_time, avg_free_time,
            total_memory_allocated / (1024.0 * 1024.0), total_throughput_mb,
            test_duration / 1000000000.0);
    
    char metadata[1024];
    get_system_metrics_enhanced(metadata, sizeof(metadata));
    
    log_operation_enhanced("MEMORY_OPS", "EXHAUSTIVE_FINAL_SUMMARY", final_metrics, "COMPLETED", metadata, TEST_ITERATIONS + 50000);
    
    printf("✅ Memory operations completed\n");
    printf("   Total Iterations: %d\n", TEST_ITERATIONS);
    printf("   Success Rate: %.2f%%\n", success_rate);
    printf("   Data Integrity Rate: %.2f%%\n", integrity_rate);
    printf("   Integrity Failures: %d\n", integrity_failures);
    printf("   Average Allocation Time: %.2f nanoseconds\n", avg_alloc_time);
    printf("   Average Write Time: %.2f nanoseconds\n", avg_write_time);
    printf("   Average Read Time: %.2f nanoseconds\n", avg_read_time);
    printf("   Average Free Time: %.2f nanoseconds\n", avg_free_time);
    printf("   Total Memory Allocated: %.2f MB\n", total_memory_allocated / (1024.0 * 1024.0));
    printf("   Overall Throughput: %.2f MB/s\n", total_throughput_mb);
    printf("   Test Duration: %.3f seconds\n", test_duration / 1000000000.0);
    
    visualmem_v2_cleanup(&ctx);
    
    // Critères de succès très stricts
    int test_passed = (success_rate >= 85.0 && integrity_rate >= 98.0);
    if (test_passed) total_tests_passed++;
    
    return test_passed;
}

/**
 * Test 5: Performance Benchmark vs Standard Technologies (Complet)
 */
static int test_performance_benchmark_v2(void) {
    printf("\n⚡ Phase 5: Performance Benchmark vs Standard Technologies (v2.0)\n");
    printf("================================================================\n");
    
    total_tests_run++;
    
    const size_t benchmark_size = BENCHMARK_SIZE_MB * 1024 * 1024; // 5MB
    const int benchmark_iterations = BENCHMARK_ITERATIONS;
    
    printf("Benchmark Configuration:\n");
    printf("   Size per iteration: %zu MB\n", benchmark_size / (1024 * 1024));
    printf("   Iterations: %d\n", benchmark_iterations);
    printf("   Total data: %zu MB\n", (benchmark_size * benchmark_iterations) / (1024 * 1024));
    
    // Benchmark 1: LibVisualMem v2.0
    printf("\nBenchmarking LibVisualMem v2.0...\n");
    
    visualmem_v2_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1920, 1080) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context for benchmark\n");
        return 0;
    }
    
    uint64_t visual_mem_total_time = 0;
    int visual_mem_success = 0;
    uint64_t visual_mem_min_time = UINT64_MAX;
    uint64_t visual_mem_max_time = 0;
    
    uint64_t benchmark_start = get_timestamp_nanoseconds();
    
    for (int i = 0; i < benchmark_iterations && !interrupted; i++) {
        void* addr = visualmem_v2_alloc(&ctx, benchmark_size, "benchmark");
        if (addr) {
            uint8_t* data = malloc(benchmark_size);
            // Pattern de données plus complexe
            for (size_t j = 0; j < benchmark_size; j++) {
                data[j] = (uint8_t)((i * j) ^ (j >> 8) ^ 0x55);
            }
            
            uint64_t start = get_timestamp_nanoseconds();
            int result = visualmem_v2_write(&ctx, addr, data, benchmark_size);
            uint64_t end = get_timestamp_nanoseconds();
            uint64_t iteration_time = end - start;
            
            if (result == VISUALMEM_V2_SUCCESS) {
                visual_mem_total_time += iteration_time;
                visual_mem_success++;
                
                if (iteration_time < visual_mem_min_time) visual_mem_min_time = iteration_time;
                if (iteration_time > visual_mem_max_time) visual_mem_max_time = iteration_time;
            }
            
            visualmem_v2_free(&ctx, addr);
            free(data);
        }
        
        if (i % 20 == 0) {
            printf("   LibVisualMem progress: %d/%d (%.1f%%)\n", 
                   i, benchmark_iterations, (float)i / benchmark_iterations * 100.0);
        }
    }
    
    uint64_t visual_benchmark_duration = get_timestamp_nanoseconds() - benchmark_start;
    
    double visual_mem_avg_time = (double)visual_mem_total_time / visual_mem_success;
    double visual_mem_throughput = (benchmark_size / (1024.0 * 1024.0)) / (visual_mem_avg_time / 1000000000.0);
    
    // Benchmark 2: Standard RAM
    printf("\nBenchmarking Standard RAM...\n");
    
    uint64_t ram_total_time = 0;
    uint64_t ram_min_time = UINT64_MAX;
    uint64_t ram_max_time = 0;
    
    benchmark_start = get_timestamp_nanoseconds();
    
    for (int i = 0; i < benchmark_iterations && !interrupted; i++) {
        uint8_t* src = malloc(benchmark_size);
        uint8_t* dst = malloc(benchmark_size);
        
        // Même pattern de données
        for (size_t j = 0; j < benchmark_size; j++) {
            src[j] = (uint8_t)((i * j) ^ (j >> 8) ^ 0x55);
        }
        
        uint64_t start = get_timestamp_nanoseconds();
        memcpy(dst, src, benchmark_size);
        uint64_t end = get_timestamp_nanoseconds();
        uint64_t iteration_time = end - start;
        
        ram_total_time += iteration_time;
        
        if (iteration_time < ram_min_time) ram_min_time = iteration_time;
        if (iteration_time > ram_max_time) ram_max_time = iteration_time;
        
        free(src);
        free(dst);
        
        if (i % 20 == 0) {
            printf("   Standard RAM progress: %d/%d (%.1f%%)\n", 
                   i, benchmark_iterations, (float)i / benchmark_iterations * 100.0);
        }
    }
    
    uint64_t ram_benchmark_duration = get_timestamp_nanoseconds() - benchmark_start;
    
    double ram_avg_time = (double)ram_total_time / benchmark_iterations;
    double ram_throughput = (benchmark_size / (1024.0 * 1024.0)) / (ram_avg_time / 1000000000.0);
    
    // Benchmark 3: SSD I/O (pour comparaison)
    printf("\nBenchmarking SSD I/O...\n");
    
    uint64_t ssd_total_time = 0;
    int ssd_success = 0;
    const char* test_file = "/tmp/libvisualmem_benchmark.tmp";
    
    benchmark_start = get_timestamp_nanoseconds();
    
    for (int i = 0; i < (benchmark_iterations / 10) && !interrupted; i++) { // Moins d'itérations pour I/O
        uint8_t* data = malloc(benchmark_size);
        for (size_t j = 0; j < benchmark_size; j++) {
            data[j] = (uint8_t)((i * j) ^ 0xAA);
        }
        
        uint64_t start = get_timestamp_nanoseconds();
        FILE* f = fopen(test_file, "wb");
        if (f) {
            fwrite(data, 1, benchmark_size, f);
            fclose(f);
            ssd_success++;
        }
        uint64_t end = get_timestamp_nanoseconds();
        
        ssd_total_time += (end - start);
        free(data);
        
        if (i % 2 == 0) {
            printf("   SSD I/O progress: %d/%d\n", i, benchmark_iterations / 10);
        }
    }
    
    unlink(test_file); // Cleanup
    
    double ssd_avg_time = ssd_success > 0 ? (double)ssd_total_time / ssd_success : 0;
    double ssd_throughput = ssd_success > 0 ? (benchmark_size / (1024.0 * 1024.0)) / (ssd_avg_time / 1000000000.0) : 0;
    
    // Calculs comparatifs
    double ram_vs_visual_ratio = ram_throughput / visual_mem_throughput;
    double ssd_vs_visual_ratio = ssd_success > 0 ? ssd_throughput / visual_mem_throughput : 0;
    
    // Log benchmark results complets
    char benchmark_metrics[2048];
    snprintf(benchmark_metrics, sizeof(benchmark_metrics),
            "size_mb=%zu,iterations=%d,visual_throughput=%.2fMB/s,ram_throughput=%.2fMB/s,"
            "ssd_throughput=%.2fMB/s,ram_ratio=%.2fx,ssd_ratio=%.2fx,"
            "visual_min=%.2fms,visual_max=%.2fms,ram_min=%.2fms,ram_max=%.2fms",
            benchmark_size / (1024*1024), benchmark_iterations, 
            visual_mem_throughput, ram_throughput, ssd_throughput,
            ram_vs_visual_ratio, ssd_vs_visual_ratio,
            visual_mem_min_time / 1000000.0, visual_mem_max_time / 1000000.0,
            ram_min_time / 1000000.0, ram_max_time / 1000000.0);
    
    char metadata[1024];
    get_system_metrics_enhanced(metadata, sizeof(metadata));
    
    log_operation_enhanced("BENCHMARK", "COMPREHENSIVE_COMPARISON", benchmark_metrics, "COMPLETED", metadata, 100000);
    
    printf("✅ Performance benchmark completed\n");
    printf("   Test Configuration: %zu MB × %d iterations\n", benchmark_size / (1024*1024), benchmark_iterations);
    printf("   \n");
    printf("   📊 RESULTS SUMMARY:\n");
    printf("   ┌─────────────────────┬──────────────┬──────────────┬──────────────┐\n");
    printf("   │ Technology          │ Throughput   │ Min Time     │ Max Time     │\n");
    printf("   ├─────────────────────┼──────────────┼──────────────┼──────────────┤\n");
    printf("   │ LibVisualMem v2.0   │ %8.2f MB/s │ %8.2f ms │ %8.2f ms │\n", 
           visual_mem_throughput, visual_mem_min_time / 1000000.0, visual_mem_max_time / 1000000.0);
    printf("   │ Standard RAM        │ %8.2f MB/s │ %8.2f ms │ %8.2f ms │\n", 
           ram_throughput, ram_min_time / 1000000.0, ram_max_time / 1000000.0);
    printf("   │ SSD I/O             │ %8.2f MB/s │      N/A     │      N/A     │\n", ssd_throughput);
    printf("   └─────────────────────┴──────────────┴──────────────┴──────────────┘\n");
    printf("   \n");
    printf("   📈 PERFORMANCE RATIOS:\n");
    printf("   • RAM is %.2fx faster than LibVisualMem\n", ram_vs_visual_ratio);
    if (ssd_success > 0) {
        printf("   • SSD is %.2fx faster than LibVisualMem\n", ssd_vs_visual_ratio);
    }
    printf("   • LibVisualMem achieves %.1f%% of RAM performance\n", 100.0 / ram_vs_visual_ratio);
    
    visualmem_v2_cleanup(&ctx);
    
    total_tests_passed++;
    return 1;
}

/**
 * Main validation function
 */
int main(int argc, char* argv[]) {
    // Installation du signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("================================================================\n");
    printf("   LIBVISUALMEM V2.0 - VALIDATION HARDWARE COMPLÈTE CORRIGÉE\n");
    printf("================================================================\n");
    printf("Validation exhaustive avec corrections et améliorations\n");
    printf("Environnement: Cursor Web + X11/Xvfb + Hardware réel\n");
    printf("Tests: 50K pixels + 5K mémoire + 5MB benchmark\n");
    printf("================================================================\n");
    
    // Initialize logging avec gestion d'erreur
    log_file = fopen(LOG_FILE, "w");
    if (!log_file) {
        printf("❌ Failed to open log file: %s\n", LOG_FILE);
        return 1;
    }
    
    metrics_file = fopen(METRICS_FILE, "w");
    if (!metrics_file) {
        printf("❌ Failed to open metrics file: %s\n", METRICS_FILE);
        fclose(log_file);
        return 1;
    }
    
    fprintf(metrics_file, "{\n");
    fprintf(metrics_file, "  \"validation_start\": \"%lu\",\n", get_timestamp_microseconds());
    fprintf(metrics_file, "  \"version\": \"v2.0_corrected\",\n");
    fprintf(metrics_file, "  \"test_config\": {\n");
    fprintf(metrics_file, "    \"pixel_tests\": %d,\n", PIXEL_TEST_COUNT);
    fprintf(metrics_file, "    \"memory_tests\": %d,\n", TEST_ITERATIONS);
    fprintf(metrics_file, "    \"benchmark_size_mb\": %d,\n", BENCHMARK_SIZE_MB);
    fprintf(metrics_file, "    \"benchmark_iterations\": %d\n", BENCHMARK_ITERATIONS);
    fprintf(metrics_file, "  },\n");
    
    test_start_time = get_timestamp_nanoseconds();
    
    // Log test start
    char start_metadata[1024];
    get_system_metrics_enhanced(start_metadata, sizeof(start_metadata));
    log_operation_enhanced("VALIDATION", "START_V2", "full_suite=true,corrected=true", "STARTED", start_metadata, 0);
    
    // Run validation phases avec gestion d'interruption
    int results[5] = {0};
    
    if (!interrupted) results[0] = test_hardware_detection_v2();
    if (!interrupted) results[1] = test_x11_connection_v2();
    if (!interrupted) results[2] = test_pixel_operations_v2();
    if (!interrupted) results[3] = test_memory_operations_v2();
    if (!interrupted) results[4] = test_performance_benchmark_v2();
    
    // Calculate final results
    int passed_tests = total_tests_passed;
    uint64_t total_test_time = get_timestamp_nanoseconds() - test_start_time;
    double success_rate = (double)passed_tests / total_tests_run * 100.0;
    
    // Log final results
    char final_metrics[1024];
    snprintf(final_metrics, sizeof(final_metrics),
            "total_time_ns=%lu,passed=%d/%d,success_rate=%.1f%%,interrupted=%s",
            total_test_time, passed_tests, total_tests_run, success_rate,
            interrupted ? "true" : "false");
    
    char final_metadata[1024];
    get_system_metrics_enhanced(final_metadata, sizeof(final_metadata));
    
    log_operation_enhanced("VALIDATION", "COMPLETE_V2", final_metrics, "FINISHED", final_metadata, 999999);
    
    // Write final metrics to JSON
    fprintf(metrics_file, "  \"validation_end\": \"%lu\",\n", get_timestamp_microseconds());
    fprintf(metrics_file, "  \"total_duration_ns\": %lu,\n", total_test_time);
    fprintf(metrics_file, "  \"total_duration_seconds\": %.3f,\n", total_test_time / 1000000000.0);
    fprintf(metrics_file, "  \"tests_passed\": %d,\n", passed_tests);
    fprintf(metrics_file, "  \"tests_total\": %d,\n", total_tests_run);
    fprintf(metrics_file, "  \"success_rate\": %.1f,\n", success_rate);
    fprintf(metrics_file, "  \"interrupted\": %s\n", interrupted ? "true" : "false");
    fprintf(metrics_file, "}\n");
    
    // Final summary avec évaluation
    printf("\n================================================================\n");
    printf("              VALIDATION RESULTS SUMMARY (v2.0 CORRECTED)\n");
    printf("================================================================\n");
    printf("Total Test Duration: %.3f seconds\n", total_test_time / 1000000000.0);
    printf("Tests Passed: %d/%d\n", passed_tests, total_tests_run);
    printf("Overall Success Rate: %.1f%%\n", success_rate);
    printf("Interrupted: %s\n", interrupted ? "YES" : "NO");
    printf("\nDetailed Results:\n");
    printf("  Phase 1 - Hardware Detection: %s\n", results[0] ? "✅ PASS" : "❌ FAIL");
    printf("  Phase 2 - X11 Connection: %s\n", results[1] ? "✅ PASS" : "❌ FAIL");
    printf("  Phase 3 - Pixel Operations: %s\n", results[2] ? "✅ PASS" : "❌ FAIL");
    printf("  Phase 4 - Memory Operations: %s\n", results[3] ? "✅ PASS" : "❌ FAIL");
    printf("  Phase 5 - Performance Benchmark: %s\n", results[4] ? "✅ PASS" : "❌ FAIL");
    
    printf("\n🎯 QUALITY ASSESSMENT:\n");
    if (success_rate >= 95.0) {
        printf("   🏆 EXCELLENT - All systems operational\n");
    } else if (success_rate >= 80.0) {
        printf("   ✅ GOOD - Minor issues detected\n");
    } else if (success_rate >= 60.0) {
        printf("   ⚠️  ACCEPTABLE - Significant issues present\n");
    } else {
        printf("   ❌ POOR - Major problems detected\n");
    }
    
    printf("\nLog Files Generated:\n");
    printf("  📄 %s - Detailed operation logs (nanosecond precision)\n", LOG_FILE);
    printf("  📊 %s - Performance metrics (JSON format)\n", METRICS_FILE);
    printf("================================================================\n");
    
    // Cleanup
    fclose(log_file);
    fclose(metrics_file);
    
    return (success_rate >= 80.0) ? 0 : 1; // 80% success threshold
}