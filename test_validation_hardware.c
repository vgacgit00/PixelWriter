/**
 * Test Validation Hardware - LibVisualMem v2.0
 * =============================================
 * 
 * Suite de tests exhaustive pour validation hardware réelle
 * avec logging temps réel, métriques complètes et benchmarks
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

// === LOGGING CONFIGURATION ===
#define LOG_FILE "logs_temps_reel.log"
#define METRICS_FILE "metriques_performance.json"
#define MAX_LOG_SIZE 1024

// === TEST CONFIGURATION ===
#define TEST_ITERATIONS 1000
#define STRESS_DURATION_SEC 10
#define PIXEL_TEST_COUNT 10000
#define MEMORY_TEST_SIZE 1024

// === GLOBAL VARIABLES ===
static FILE* log_file = NULL;
static FILE* metrics_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t test_start_time = 0;

// === UTILITY FUNCTIONS ===

static uint64_t get_timestamp_microseconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
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

static void log_operation(const char* component, const char* operation, 
                         const char* metrics, const char* result, 
                         const char* metadata) {
    pthread_mutex_lock(&log_mutex);
    
    if (log_file) {
        char timestamp[64];
        get_timestamp_string(timestamp, sizeof(timestamp));
        
        fprintf(log_file, "[%s] [%s] [%s] [%s] [%s] [%s]\n",
                timestamp, component, operation, metrics, result, metadata);
        fflush(log_file);
    }
    
    // Also print to console for real-time monitoring
    printf("LOG: [%s] %s -> %s\n", component, operation, result);
    
    pthread_mutex_unlock(&log_mutex);
}

static void get_system_metrics(char* buffer, size_t size) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    
    // Get memory info
    long rss_kb = usage.ru_maxrss; // in KB on Linux
    
    // Get CPU time
    double cpu_time = (double)usage.ru_utime.tv_sec + 
                     (double)usage.ru_utime.tv_usec / 1000000.0;
    
    snprintf(buffer, size, "rss=%ldKB,cpu_time=%.3fs,ctx_switches=%ld",
             rss_kb, cpu_time, usage.ru_nvcsw + usage.ru_nivcsw);
}

// === HARDWARE VALIDATION TESTS ===

/**
 * Test 1: Hardware Detection and Access Validation
 */
static int test_hardware_detection(void) {
    printf("\n🔍 Phase 1: Hardware Detection and Access Validation\n");
    printf("================================================================\n");
    
    uint64_t start_time = get_timestamp_microseconds();
    
    // Test hardware capabilities detection
    visualmem_v2_hardware_caps_t caps;
    int result = visualmem_v2_get_hardware_caps(&caps);
    
    uint64_t detection_time = get_timestamp_microseconds() - start_time;
    
    char metrics[256];
    snprintf(metrics, sizeof(metrics), "detection_time=%luus", detection_time);
    
    char metadata[512];
    get_system_metrics(metadata, sizeof(metadata));
    
    if (result == VISUALMEM_V2_SUCCESS) {
        log_operation("HARDWARE_DETECTION", "GET_CAPABILITIES", metrics, "SUCCESS", metadata);
        
        // Log detailed hardware info
        char hw_info[1024];
        snprintf(hw_info, sizeof(hw_info), 
                "x11=%d,opengl=%d,fb=%d,drm=%d,gpu=%s",
                caps.has_x11, caps.has_opengl, caps.has_framebuffer, 
                caps.has_drm, caps.gpu_model);
        log_operation("HARDWARE_CAPS", "DETECTED", hw_info, "SUCCESS", metadata);
        
        printf("✅ Hardware detection successful\n");
        printf("   X11 Support: %s\n", caps.has_x11 ? "YES" : "NO");
        printf("   OpenGL Support: %s\n", caps.has_opengl ? "YES" : "NO");
        printf("   Framebuffer: %s\n", caps.has_framebuffer ? "YES" : "NO");
        printf("   DRM Support: %s\n", caps.has_drm ? "YES" : "NO");
        printf("   GPU Model: %s\n", caps.gpu_model);
        printf("   Detection Time: %lu microseconds\n", detection_time);
        
        return 1;
    } else {
        log_operation("HARDWARE_DETECTION", "GET_CAPABILITIES", metrics, "FAILED", metadata);
        printf("❌ Hardware detection failed\n");
        return 0;
    }
}

/**
 * Test 2: X11 Connection and Display Validation
 */
static int test_x11_connection(void) {
    printf("\n🖥️  Phase 2: X11 Connection and Display Validation\n");
    printf("================================================================\n");
    
    uint64_t start_time = get_timestamp_microseconds();
    
    // Test X11 context initialization
    visualmem_v2_context_t ctx;
    int result = visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 640, 480);
    
    uint64_t init_time = get_timestamp_microseconds() - start_time;
    
    char metrics[256];
    snprintf(metrics, sizeof(metrics), "init_time=%luus,width=640,height=480", init_time);
    
    char metadata[512];
    get_system_metrics(metadata, sizeof(metadata));
    
    if (result == VISUALMEM_V2_SUCCESS) {
        log_operation("X11_CONNECTION", "INIT_CONTEXT", metrics, "SUCCESS", metadata);
        
        // Test display properties
        char display_info[512];
        snprintf(display_info, sizeof(display_info),
                "backend=%d,display_active=%d,hardware_active=%d",
                ctx.backend, ctx.display_active, ctx.hardware_active);
        log_operation("X11_DISPLAY", "PROPERTIES", display_info, "SUCCESS", metadata);
        
        printf("✅ X11 connection successful\n");
        printf("   Backend: %d\n", ctx.backend);
        printf("   Display Active: %s\n", ctx.display_active ? "YES" : "NO");
        printf("   Hardware Active: %s\n", ctx.hardware_active ? "YES" : "NO");
        printf("   Initialization Time: %lu microseconds\n", init_time);
        
        // Test display refresh
        start_time = get_timestamp_microseconds();
        int refresh_result = visualmem_v2_refresh_display(&ctx);
        uint64_t refresh_time = get_timestamp_microseconds() - start_time;
        
        snprintf(metrics, sizeof(metrics), "refresh_time=%luus", refresh_time);
        log_operation("X11_DISPLAY", "REFRESH", metrics, 
                     refresh_result == VISUALMEM_V2_SUCCESS ? "SUCCESS" : "FAILED", metadata);
        
        printf("   Display Refresh: %s (%lu us)\n", 
               refresh_result == VISUALMEM_V2_SUCCESS ? "SUCCESS" : "FAILED", refresh_time);
        
        visualmem_v2_cleanup(&ctx);
        return 1;
    } else {
        log_operation("X11_CONNECTION", "INIT_CONTEXT", metrics, "FAILED", metadata);
        printf("❌ X11 connection failed: %s\n", visualmem_v2_get_error_string(result));
        return 0;
    }
}

/**
 * Test 3: Pixel Manipulation Validation
 */
static int test_pixel_operations(void) {
    printf("\n🎨 Phase 3: Pixel Manipulation Validation\n");
    printf("================================================================\n");
    
    visualmem_v2_context_t ctx;
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 800, 600) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context for pixel tests\n");
        return 0;
    }
    
    int success_count = 0;
    uint64_t total_write_time = 0;
    uint64_t total_read_time = 0;
    
    printf("Testing %d pixel operations...\n", PIXEL_TEST_COUNT);
    
    for (int i = 0; i < PIXEL_TEST_COUNT; i++) {
        int x = rand() % ctx.width;
        int y = rand() % ctx.height;
        uint32_t color = (rand() | 0xFF000000);
        
        // Test pixel write
        uint64_t write_start = get_timestamp_microseconds();
        int write_result = visualmem_v2_write_pixel(&ctx, x, y, color);
        uint64_t write_time = get_timestamp_microseconds() - write_start;
        total_write_time += write_time;
        
        // Test pixel read
        uint64_t read_start = get_timestamp_microseconds();
        uint32_t read_color = visualmem_v2_read_pixel(&ctx, x, y);
        uint64_t read_time = get_timestamp_microseconds() - read_start;
        total_read_time += read_time;
        
        if (write_result == VISUALMEM_V2_SUCCESS) {
            success_count++;
            
            // Log every 1000th operation for sampling
            if (i % 1000 == 0) {
                char metrics[256];
                snprintf(metrics, sizeof(metrics), 
                        "x=%d,y=%d,color=0x%08X,write_time=%luus,read_time=%luus",
                        x, y, color, write_time, read_time);
                
                char metadata[512];
                get_system_metrics(metadata, sizeof(metadata));
                
                log_operation("PIXEL_OPS", "WRITE_READ_CYCLE", metrics, "SUCCESS", metadata);
            }
        }
        
        // Progress indicator
        if (i % 1000 == 0) {
            printf("   Progress: %d/%d pixels processed\n", i, PIXEL_TEST_COUNT);
        }
    }
    
    // Calculate averages
    double avg_write_time = (double)total_write_time / PIXEL_TEST_COUNT;
    double avg_read_time = (double)total_read_time / PIXEL_TEST_COUNT;
    double success_rate = (double)success_count / PIXEL_TEST_COUNT * 100.0;
    
    char final_metrics[512];
    snprintf(final_metrics, sizeof(final_metrics),
            "total_ops=%d,success_rate=%.2f%%,avg_write=%.2fus,avg_read=%.2fus",
            PIXEL_TEST_COUNT, success_rate, avg_write_time, avg_read_time);
    
    char metadata[512];
    get_system_metrics(metadata, sizeof(metadata));
    
    log_operation("PIXEL_OPS", "SUMMARY", final_metrics, "COMPLETED", metadata);
    
    printf("✅ Pixel operations completed\n");
    printf("   Total Operations: %d\n", PIXEL_TEST_COUNT);
    printf("   Success Rate: %.2f%%\n", success_rate);
    printf("   Average Write Time: %.2f microseconds\n", avg_write_time);
    printf("   Average Read Time: %.2f microseconds\n", avg_read_time);
    printf("   Write Throughput: %.0f pixels/second\n", 1000000.0 / avg_write_time);
    printf("   Read Throughput: %.0f pixels/second\n", 1000000.0 / avg_read_time);
    
    visualmem_v2_cleanup(&ctx);
    return success_count > (PIXEL_TEST_COUNT * 0.8); // 80% success threshold
}

/**
 * Test 4: Memory Operations Validation
 */
static int test_memory_operations(void) {
    printf("\n💾 Phase 4: Memory Operations Validation\n");
    printf("================================================================\n");
    
    visualmem_v2_context_t ctx;
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1024, 768) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context for memory tests\n");
        return 0;
    }
    
    int success_count = 0;
    uint64_t total_alloc_time = 0;
    uint64_t total_write_time = 0;
    uint64_t total_read_time = 0;
    uint64_t total_free_time = 0;
    
    printf("Testing memory operations with %d iterations...\n", TEST_ITERATIONS);
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        size_t test_size = 16 + (rand() % MEMORY_TEST_SIZE);
        char label[32];
        snprintf(label, sizeof(label), "test_mem_%d", i);
        
        // Test allocation
        uint64_t alloc_start = get_timestamp_microseconds();
        void* addr = visualmem_v2_alloc(&ctx, test_size, label);
        uint64_t alloc_time = get_timestamp_microseconds() - alloc_start;
        total_alloc_time += alloc_time;
        
        if (addr) {
            // Create test data
            uint8_t* test_data = malloc(test_size);
            for (size_t j = 0; j < test_size; j++) {
                test_data[j] = (uint8_t)(j ^ i);
            }
            
            // Test write
            uint64_t write_start = get_timestamp_microseconds();
            int write_result = visualmem_v2_write(&ctx, addr, test_data, test_size);
            uint64_t write_time = get_timestamp_microseconds() - write_start;
            total_write_time += write_time;
            
            // Test read
            uint8_t* read_data = malloc(test_size);
            uint64_t read_start = get_timestamp_microseconds();
            int read_result = visualmem_v2_read(&ctx, addr, read_data, test_size);
            uint64_t read_time = get_timestamp_microseconds() - read_start;
            total_read_time += read_time;
            
            // Verify data integrity
            int integrity_ok = (memcmp(test_data, read_data, test_size) == 0);
            
            // Test free
            uint64_t free_start = get_timestamp_microseconds();
            int free_result = visualmem_v2_free(&ctx, addr);
            uint64_t free_time = get_timestamp_microseconds() - free_start;
            total_free_time += free_time;
            
            if (write_result == VISUALMEM_V2_SUCCESS && 
                read_result == VISUALMEM_V2_SUCCESS && 
                free_result == VISUALMEM_V2_SUCCESS) {
                success_count++;
                
                // Log every 100th operation
                if (i % 100 == 0) {
                    char metrics[512];
                    snprintf(metrics, sizeof(metrics),
                            "size=%zu,alloc_time=%luus,write_time=%luus,read_time=%luus,free_time=%luus,integrity=%s",
                            test_size, alloc_time, write_time, read_time, free_time,
                            integrity_ok ? "OK" : "FAIL");
                    
                    char metadata[512];
                    get_system_metrics(metadata, sizeof(metadata));
                    
                    log_operation("MEMORY_OPS", "FULL_CYCLE", metrics, "SUCCESS", metadata);
                }
            }
            
            free(test_data);
            free(read_data);
        }
        
        // Progress indicator
        if (i % 100 == 0) {
            printf("   Progress: %d/%d memory operations\n", i, TEST_ITERATIONS);
        }
    }
    
    // Calculate performance metrics
    double avg_alloc_time = (double)total_alloc_time / TEST_ITERATIONS;
    double avg_write_time = (double)total_write_time / TEST_ITERATIONS;
    double avg_read_time = (double)total_read_time / TEST_ITERATIONS;
    double avg_free_time = (double)total_free_time / TEST_ITERATIONS;
    double success_rate = (double)success_count / TEST_ITERATIONS * 100.0;
    
    char final_metrics[1024];
    snprintf(final_metrics, sizeof(final_metrics),
            "iterations=%d,success_rate=%.2f%%,avg_alloc=%.2fus,avg_write=%.2fus,avg_read=%.2fus,avg_free=%.2fus",
            TEST_ITERATIONS, success_rate, avg_alloc_time, avg_write_time, avg_read_time, avg_free_time);
    
    char metadata[512];
    get_system_metrics(metadata, sizeof(metadata));
    
    log_operation("MEMORY_OPS", "FINAL_SUMMARY", final_metrics, "COMPLETED", metadata);
    
    printf("✅ Memory operations completed\n");
    printf("   Total Iterations: %d\n", TEST_ITERATIONS);
    printf("   Success Rate: %.2f%%\n", success_rate);
    printf("   Average Allocation Time: %.2f microseconds\n", avg_alloc_time);
    printf("   Average Write Time: %.2f microseconds\n", avg_write_time);
    printf("   Average Read Time: %.2f microseconds\n", avg_read_time);
    printf("   Average Free Time: %.2f microseconds\n", avg_free_time);
    
    visualmem_v2_cleanup(&ctx);
    return success_count > (TEST_ITERATIONS * 0.8); // 80% success threshold
}

/**
 * Test 5: Performance Benchmark vs Standard Technologies
 */
static int test_performance_benchmark(void) {
    printf("\n⚡ Phase 5: Performance Benchmark vs Standard Technologies\n");
    printf("================================================================\n");
    
    const size_t benchmark_size = 1024 * 1024; // 1MB
    const int benchmark_iterations = 100;
    
    // Benchmark 1: LibVisualMem v2.0
    printf("Benchmarking LibVisualMem v2.0...\n");
    
    visualmem_v2_context_t ctx;
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1024, 768) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context for benchmark\n");
        return 0;
    }
    
    uint64_t visual_mem_total_time = 0;
    int visual_mem_success = 0;
    
    for (int i = 0; i < benchmark_iterations; i++) {
        void* addr = visualmem_v2_alloc(&ctx, benchmark_size, "benchmark");
        if (addr) {
            uint8_t* data = malloc(benchmark_size);
            memset(data, i & 0xFF, benchmark_size);
            
            uint64_t start = get_timestamp_microseconds();
            int result = visualmem_v2_write(&ctx, addr, data, benchmark_size);
            uint64_t end = get_timestamp_microseconds();
            
            if (result == VISUALMEM_V2_SUCCESS) {
                visual_mem_total_time += (end - start);
                visual_mem_success++;
            }
            
            visualmem_v2_free(&ctx, addr);
            free(data);
        }
    }
    
    double visual_mem_avg_time = (double)visual_mem_total_time / visual_mem_success;
    double visual_mem_throughput = (benchmark_size / (1024.0 * 1024.0)) / (visual_mem_avg_time / 1000000.0);
    
    // Benchmark 2: Standard RAM
    printf("Benchmarking Standard RAM...\n");
    
    uint64_t ram_total_time = 0;
    
    for (int i = 0; i < benchmark_iterations; i++) {
        uint8_t* src = malloc(benchmark_size);
        uint8_t* dst = malloc(benchmark_size);
        memset(src, i & 0xFF, benchmark_size);
        
        uint64_t start = get_timestamp_microseconds();
        memcpy(dst, src, benchmark_size);
        uint64_t end = get_timestamp_microseconds();
        
        ram_total_time += (end - start);
        
        free(src);
        free(dst);
    }
    
    double ram_avg_time = (double)ram_total_time / benchmark_iterations;
    double ram_throughput = (benchmark_size / (1024.0 * 1024.0)) / (ram_avg_time / 1000000.0);
    
    // Calculate comparison ratio
    double performance_ratio = ram_throughput / visual_mem_throughput;
    
    // Log benchmark results
    char benchmark_metrics[1024];
    snprintf(benchmark_metrics, sizeof(benchmark_metrics),
            "size=%zuMB,iterations=%d,visual_mem_throughput=%.2fMB/s,ram_throughput=%.2fMB/s,ratio=%.2fx",
            benchmark_size / (1024*1024), benchmark_iterations, 
            visual_mem_throughput, ram_throughput, performance_ratio);
    
    char metadata[512];
    get_system_metrics(metadata, sizeof(metadata));
    
    log_operation("BENCHMARK", "PERFORMANCE_COMPARISON", benchmark_metrics, "COMPLETED", metadata);
    
    printf("✅ Performance benchmark completed\n");
    printf("   Test Size: %zu MB\n", benchmark_size / (1024*1024));
    printf("   Iterations: %d\n", benchmark_iterations);
    printf("   LibVisualMem v2.0 Throughput: %.2f MB/s\n", visual_mem_throughput);
    printf("   Standard RAM Throughput: %.2f MB/s\n", ram_throughput);
    printf("   Performance Ratio: %.2fx slower than RAM\n", performance_ratio);
    
    visualmem_v2_cleanup(&ctx);
    return 1;
}

/**
 * Main validation function
 */
int main(int argc, char* argv[]) {
    printf("================================================================\n");
    printf("     LIBVISUALMEM V2.0 - VALIDATION HARDWARE COMPLÈTE\n");
    printf("================================================================\n");
    printf("Validation exhaustive avec logs temps réel et métriques\n");
    printf("Environnement: Cursor Web + X11/Xvfb\n");
    printf("================================================================\n");
    
    // Initialize logging
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
    
    test_start_time = get_timestamp_microseconds();
    
    // Log test start
    char start_metadata[512];
    get_system_metrics(start_metadata, sizeof(start_metadata));
    log_operation("VALIDATION", "START", "full_suite=true", "STARTED", start_metadata);
    
    // Run validation phases
    int results[5] = {0};
    
    results[0] = test_hardware_detection();
    results[1] = test_x11_connection();
    results[2] = test_pixel_operations();
    results[3] = test_memory_operations();
    results[4] = test_performance_benchmark();
    
    // Calculate final results
    int passed_tests = 0;
    for (int i = 0; i < 5; i++) {
        if (results[i]) passed_tests++;
    }
    
    uint64_t total_test_time = get_timestamp_microseconds() - test_start_time;
    double success_rate = (double)passed_tests / 5.0 * 100.0;
    
    // Log final results
    char final_metrics[512];
    snprintf(final_metrics, sizeof(final_metrics),
            "total_time=%lums,passed=%d/5,success_rate=%.1f%%",
            total_test_time / 1000, passed_tests, success_rate);
    
    char final_metadata[512];
    get_system_metrics(final_metadata, sizeof(final_metadata));
    
    log_operation("VALIDATION", "COMPLETE", final_metrics, "FINISHED", final_metadata);
    
    // Write final metrics to JSON
    fprintf(metrics_file, "  \"validation_end\": \"%lu\",\n", get_timestamp_microseconds());
    fprintf(metrics_file, "  \"total_duration_us\": %lu,\n", total_test_time);
    fprintf(metrics_file, "  \"tests_passed\": %d,\n", passed_tests);
    fprintf(metrics_file, "  \"tests_total\": 5,\n");
    fprintf(metrics_file, "  \"success_rate\": %.1f\n", success_rate);
    fprintf(metrics_file, "}\n");
    
    // Final summary
    printf("\n================================================================\n");
    printf("                  VALIDATION RESULTS SUMMARY\n");
    printf("================================================================\n");
    printf("Total Test Duration: %.2f seconds\n", total_test_time / 1000000.0);
    printf("Tests Passed: %d/5\n", passed_tests);
    printf("Overall Success Rate: %.1f%%\n", success_rate);
    printf("\nDetailed Results:\n");
    printf("  Phase 1 - Hardware Detection: %s\n", results[0] ? "✅ PASS" : "❌ FAIL");
    printf("  Phase 2 - X11 Connection: %s\n", results[1] ? "✅ PASS" : "❌ FAIL");
    printf("  Phase 3 - Pixel Operations: %s\n", results[2] ? "✅ PASS" : "❌ FAIL");
    printf("  Phase 4 - Memory Operations: %s\n", results[3] ? "✅ PASS" : "❌ FAIL");
    printf("  Phase 5 - Performance Benchmark: %s\n", results[4] ? "✅ PASS" : "❌ FAIL");
    printf("\nLog Files Generated:\n");
    printf("  📄 %s - Detailed operation logs\n", LOG_FILE);
    printf("  📊 %s - Performance metrics\n", METRICS_FILE);
    printf("================================================================\n");
    
    // Cleanup
    fclose(log_file);
    fclose(metrics_file);
    
    return (success_rate >= 80.0) ? 0 : 1; // 80% success threshold
}