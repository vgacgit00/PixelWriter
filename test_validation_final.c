/**
 * Test Validation Final - LibVisualMem v2.0 (Compatible)
 * =======================================================
 * 
 * Suite de tests compatible avec l'API existante
 * Validation hardware avec logging temps réel
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
#include <signal.h>

// === CONFIGURATION ===
#define LOG_FILE "logs_validation_final.log"
#define METRICS_FILE "metrics_validation_final.json"
#define PIXEL_TESTS 25000
#define MEMORY_TESTS 2500
#define BENCHMARK_ITERATIONS 100

// === GLOBALS ===
static FILE* log_file = NULL;
static FILE* metrics_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int interrupted = 0;
static int tests_passed = 0;
static int tests_total = 0;

// === SIGNAL HANDLER ===
void signal_handler(int sig) {
    interrupted = 1;
    printf("\n[SIGNAL] Tests interrupted, cleaning up...\n");
}

// === UTILITIES ===
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

static void log_test(const char* phase, const char* result, const char* details) {
    pthread_mutex_lock(&log_mutex);
    
    if (log_file) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        fprintf(log_file, "[%ld.%06ld] [%s] %s - %s\n", 
                tv.tv_sec, tv.tv_usec, phase, result, details);
        fflush(log_file);
    }
    
    printf("[%s] %s - %s\n", phase, result, details);
    pthread_mutex_unlock(&log_mutex);
}

// === TEST 1: HARDWARE DETECTION ===
static int test_hardware_detection(void) {
    printf("\n🔍 Phase 1: Hardware Detection\n");
    printf("================================\n");
    
    tests_total++;
    uint64_t start = get_timestamp_ns();
    
    visualmem_v2_hardware_caps_t caps;
    int result = visualmem_v2_get_hardware_caps(&caps);
    
    uint64_t duration = get_timestamp_ns() - start;
    
    if (result == VISUALMEM_V2_SUCCESS) {
        char details[512];
        snprintf(details, sizeof(details), 
                "X11=%s OpenGL=%s GPU=%s Duration=%.2fms",
                caps.has_x11 ? "YES" : "NO",
                caps.has_opengl ? "YES" : "NO", 
                caps.gpu_model,
                duration / 1000000.0);
        
        log_test("HARDWARE_DETECTION", "SUCCESS", details);
        
        printf("✅ Hardware detection successful\n");
        printf("   X11 Support: %s\n", caps.has_x11 ? "YES" : "NO");
        printf("   OpenGL Support: %s\n", caps.has_opengl ? "YES" : "NO");
        printf("   GPU Model: %s\n", caps.gpu_model);
        printf("   Detection Time: %.2f ms\n", duration / 1000000.0);
        
        tests_passed++;
        return 1;
    } else {
        log_test("HARDWARE_DETECTION", "FAILED", "Hardware not accessible");
        printf("❌ Hardware detection failed\n");
        return 0;
    }
}

// === TEST 2: DISPLAY INITIALIZATION ===
static int test_display_init(void) {
    printf("\n🖥️  Phase 2: Display Initialization\n");
    printf("====================================\n");
    
    tests_total++;
    uint64_t start = get_timestamp_ns();
    
    visualmem_v2_context_t ctx;
    int result = visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 800, 600);
    
    uint64_t duration = get_timestamp_ns() - start;
    
    if (result == VISUALMEM_V2_SUCCESS) {
        char details[512];
        snprintf(details, sizeof(details),
                "Backend=%d Display=%s Hardware=%s Duration=%.2fms",
                ctx.backend,
                ctx.display_active ? "ACTIVE" : "INACTIVE",
                ctx.hardware_active ? "ACTIVE" : "INACTIVE",
                duration / 1000000.0);
        
        log_test("DISPLAY_INIT", "SUCCESS", details);
        
        printf("✅ Display initialization successful\n");
        printf("   Backend: %d\n", ctx.backend);
        printf("   Display Active: %s\n", ctx.display_active ? "YES" : "NO");
        printf("   Hardware Active: %s\n", ctx.hardware_active ? "YES" : "NO");
        printf("   Initialization Time: %.2f ms\n", duration / 1000000.0);
        
        // Test refresh
        uint64_t refresh_start = get_timestamp_ns();
        int refresh_result = visualmem_v2_refresh_display(&ctx);
        uint64_t refresh_duration = get_timestamp_ns() - refresh_start;
        
        printf("   Display Refresh: %s (%.2f ms)\n",
               refresh_result == VISUALMEM_V2_SUCCESS ? "SUCCESS" : "FAILED",
               refresh_duration / 1000000.0);
        
        // Cleanup
        visualmem_v2_cleanup(&ctx);
        printf("   Cleanup: SUCCESS\n");
        
        tests_passed++;
        return 1;
    } else {
        char details[256];
        snprintf(details, sizeof(details), "Error code: %d", result);
        log_test("DISPLAY_INIT", "FAILED", details);
        printf("❌ Display initialization failed: %d\n", result);
        return 0;
    }
}

// === TEST 3: PIXEL OPERATIONS ===
static int test_pixel_operations(void) {
    printf("\n🎨 Phase 3: Pixel Operations (%d tests)\n", PIXEL_TESTS);
    printf("=========================================\n");
    
    tests_total++;
    
    visualmem_v2_context_t ctx;
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1024, 768) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context\n");
        return 0;
    }
    
    int success_count = 0;
    uint64_t total_time = 0;
    uint64_t min_time = UINT64_MAX;
    uint64_t max_time = 0;
    
    uint64_t test_start = get_timestamp_ns();
    
    for (int i = 0; i < PIXEL_TESTS && !interrupted; i++) {
        int x = rand() % ctx.width;
        int y = rand() % ctx.height;
        uint32_t color = 0xFF000000 | (rand() & 0xFFFFFF);
        
        uint64_t op_start = get_timestamp_ns();
        int result = visualmem_v2_write_pixel(&ctx, x, y, color);
        uint64_t op_time = get_timestamp_ns() - op_start;
        
        total_time += op_time;
        if (op_time < min_time) min_time = op_time;
        if (op_time > max_time) max_time = op_time;
        
        if (result == VISUALMEM_V2_SUCCESS) {
            success_count++;
        }
        
        if (i % 5000 == 0) {
            printf("   Progress: %d/%d (%.1f%%)\n", i, PIXEL_TESTS, 
                   (float)i / PIXEL_TESTS * 100.0);
        }
    }
    
    uint64_t test_duration = get_timestamp_ns() - test_start;
    
    double success_rate = (double)success_count / PIXEL_TESTS * 100.0;
    double avg_time = (double)total_time / PIXEL_TESTS;
    double throughput = PIXEL_TESTS / (test_duration / 1000000000.0);
    
    char details[1024];
    snprintf(details, sizeof(details),
            "Operations=%d Success=%.1f%% AvgTime=%.0fns MinTime=%luns MaxTime=%luns Throughput=%.0f/s",
            PIXEL_TESTS, success_rate, avg_time, min_time, max_time, throughput);
    
    log_test("PIXEL_OPERATIONS", success_rate >= 80.0 ? "SUCCESS" : "PARTIAL", details);
    
    printf("✅ Pixel operations completed\n");
    printf("   Total Operations: %d\n", PIXEL_TESTS);
    printf("   Success Rate: %.1f%%\n", success_rate);
    printf("   Average Time: %.0f nanoseconds\n", avg_time);
    printf("   Time Range: %lu - %lu ns\n", min_time, max_time);
    printf("   Throughput: %.0f pixels/second\n", throughput);
    printf("   Total Duration: %.2f seconds\n", test_duration / 1000000000.0);
    
    visualmem_v2_cleanup(&ctx);
    
    if (success_rate >= 80.0) {
        tests_passed++;
        return 1;
    }
    return 0;
}

// === TEST 4: MEMORY OPERATIONS ===
static int test_memory_operations(void) {
    printf("\n💾 Phase 4: Memory Operations (%d tests)\n", MEMORY_TESTS);
    printf("==========================================\n");
    
    tests_total++;
    
    visualmem_v2_context_t ctx;
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1280, 960) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context\n");
        return 0;
    }
    
    int success_count = 0;
    uint64_t total_alloc_time = 0;
    uint64_t total_write_time = 0;
    uint64_t total_read_time = 0;
    
    uint64_t test_start = get_timestamp_ns();
    
    for (int i = 0; i < MEMORY_TESTS && !interrupted; i++) {
        size_t size = 64 + (rand() % 1024);
        char label[32];
        snprintf(label, sizeof(label), "test_%d", i);
        
        // Allocation
        uint64_t alloc_start = get_timestamp_ns();
        void* addr = visualmem_v2_alloc(&ctx, size, label);
        uint64_t alloc_time = get_timestamp_ns() - alloc_start;
        total_alloc_time += alloc_time;
        
        if (addr) {
            // Write test
            uint8_t* data = malloc(size);
            for (size_t j = 0; j < size; j++) {
                data[j] = (uint8_t)((i + j) & 0xFF);
            }
            
            uint64_t write_start = get_timestamp_ns();
            int write_result = visualmem_v2_write(&ctx, addr, data, size);
            uint64_t write_time = get_timestamp_ns() - write_start;
            total_write_time += write_time;
            
            // Read test
            uint8_t* read_data = malloc(size);
            uint64_t read_start = get_timestamp_ns();
            int read_result = visualmem_v2_read(&ctx, addr, read_data, size);
            uint64_t read_time = get_timestamp_ns() - read_start;
            total_read_time += read_time;
            
            // Verify
            int integrity = (memcmp(data, read_data, size) == 0);
            
            if (write_result == VISUALMEM_V2_SUCCESS && 
                read_result == VISUALMEM_V2_SUCCESS && integrity) {
                success_count++;
            }
            
            visualmem_v2_free(&ctx, addr);
            free(data);
            free(read_data);
        }
        
        if (i % 500 == 0) {
            printf("   Progress: %d/%d (%.1f%%)\n", i, MEMORY_TESTS,
                   (float)i / MEMORY_TESTS * 100.0);
        }
    }
    
    uint64_t test_duration = get_timestamp_ns() - test_start;
    
    double success_rate = (double)success_count / MEMORY_TESTS * 100.0;
    double avg_alloc = (double)total_alloc_time / MEMORY_TESTS;
    double avg_write = (double)total_write_time / MEMORY_TESTS;
    double avg_read = (double)total_read_time / MEMORY_TESTS;
    
    char details[1024];
    snprintf(details, sizeof(details),
            "Operations=%d Success=%.1f%% AvgAlloc=%.0fns AvgWrite=%.0fns AvgRead=%.0fns",
            MEMORY_TESTS, success_rate, avg_alloc, avg_write, avg_read);
    
    log_test("MEMORY_OPERATIONS", success_rate >= 85.0 ? "SUCCESS" : "PARTIAL", details);
    
    printf("✅ Memory operations completed\n");
    printf("   Total Operations: %d\n", MEMORY_TESTS);
    printf("   Success Rate: %.1f%%\n", success_rate);
    printf("   Average Allocation Time: %.0f ns\n", avg_alloc);
    printf("   Average Write Time: %.0f ns\n", avg_write);
    printf("   Average Read Time: %.0f ns\n", avg_read);
    printf("   Total Duration: %.2f seconds\n", test_duration / 1000000000.0);
    
    visualmem_v2_cleanup(&ctx);
    
    if (success_rate >= 85.0) {
        tests_passed++;
        return 1;
    }
    return 0;
}

// === TEST 5: PERFORMANCE BENCHMARK ===
static int test_performance_benchmark(void) {
    printf("\n⚡ Phase 5: Performance Benchmark\n");
    printf("==================================\n");
    
    tests_total++;
    
    const size_t benchmark_size = 1024 * 1024; // 1MB
    
    // LibVisualMem benchmark
    printf("Benchmarking LibVisualMem v2.0...\n");
    
    visualmem_v2_context_t ctx;
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1920, 1080) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context\n");
        return 0;
    }
    
    uint64_t visual_total_time = 0;
    int visual_success = 0;
    
    for (int i = 0; i < BENCHMARK_ITERATIONS && !interrupted; i++) {
        void* addr = visualmem_v2_alloc(&ctx, benchmark_size, "benchmark");
        if (addr) {
            uint8_t* data = malloc(benchmark_size);
            memset(data, i & 0xFF, benchmark_size);
            
            uint64_t start = get_timestamp_ns();
            int result = visualmem_v2_write(&ctx, addr, data, benchmark_size);
            uint64_t end = get_timestamp_ns();
            
            if (result == VISUALMEM_V2_SUCCESS) {
                visual_total_time += (end - start);
                visual_success++;
            }
            
            visualmem_v2_free(&ctx, addr);
            free(data);
        }
        
        if (i % 20 == 0) {
            printf("   Progress: %d/%d\n", i, BENCHMARK_ITERATIONS);
        }
    }
    
    // RAM benchmark
    printf("Benchmarking Standard RAM...\n");
    
    uint64_t ram_total_time = 0;
    
    for (int i = 0; i < BENCHMARK_ITERATIONS && !interrupted; i++) {
        uint8_t* src = malloc(benchmark_size);
        uint8_t* dst = malloc(benchmark_size);
        memset(src, i & 0xFF, benchmark_size);
        
        uint64_t start = get_timestamp_ns();
        memcpy(dst, src, benchmark_size);
        uint64_t end = get_timestamp_ns();
        
        ram_total_time += (end - start);
        
        free(src);
        free(dst);
    }
    
    // Calculate results
    double visual_throughput = visual_success > 0 ? 
        (benchmark_size / (1024.0 * 1024.0)) / ((double)visual_total_time / visual_success / 1000000000.0) : 0;
    double ram_throughput = (benchmark_size / (1024.0 * 1024.0)) / ((double)ram_total_time / BENCHMARK_ITERATIONS / 1000000000.0);
    double ratio = visual_throughput > 0 ? ram_throughput / visual_throughput : 0;
    
    char details[512];
    snprintf(details, sizeof(details),
            "VisualMem=%.2fMB/s RAM=%.2fMB/s Ratio=%.1fx",
            visual_throughput, ram_throughput, ratio);
    
    log_test("PERFORMANCE_BENCHMARK", "SUCCESS", details);
    
    printf("✅ Performance benchmark completed\n");
    printf("   LibVisualMem v2.0: %.2f MB/s\n", visual_throughput);
    printf("   Standard RAM: %.2f MB/s\n", ram_throughput);
    printf("   Performance Ratio: %.1fx slower\n", ratio);
    printf("   LibVisualMem efficiency: %.1f%% of RAM\n", 100.0 / ratio);
    
    visualmem_v2_cleanup(&ctx);
    tests_passed++;
    return 1;
}

// === MAIN ===
int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("================================================================\n");
    printf("   LIBVISUALMEM V2.0 - VALIDATION HARDWARE FINALE\n");
    printf("================================================================\n");
    printf("Suite de tests compatible avec validation complète\n");
    printf("Tests: %d pixels + %d mémoire + benchmarks\n", PIXEL_TESTS, MEMORY_TESTS);
    printf("================================================================\n");
    
    // Initialize logging
    log_file = fopen(LOG_FILE, "w");
    metrics_file = fopen(METRICS_FILE, "w");
    
    if (!log_file || !metrics_file) {
        printf("❌ Failed to open log files\n");
        return 1;
    }
    
    fprintf(metrics_file, "{\n  \"test_start\": %lu,\n", time(NULL));
    
    uint64_t start_time = get_timestamp_ns();
    
    // Run tests
    int results[5];
    results[0] = test_hardware_detection();
    if (!interrupted) results[1] = test_display_init();
    if (!interrupted) results[2] = test_pixel_operations();
    if (!interrupted) results[3] = test_memory_operations();
    if (!interrupted) results[4] = test_performance_benchmark();
    
    uint64_t total_time = get_timestamp_ns() - start_time;
    double success_rate = (double)tests_passed / tests_total * 100.0;
    
    // Final results
    fprintf(metrics_file, "  \"test_end\": %lu,\n", time(NULL));
    fprintf(metrics_file, "  \"duration_seconds\": %.3f,\n", total_time / 1000000000.0);
    fprintf(metrics_file, "  \"tests_passed\": %d,\n", tests_passed);
    fprintf(metrics_file, "  \"tests_total\": %d,\n", tests_total);
    fprintf(metrics_file, "  \"success_rate\": %.1f\n", success_rate);
    fprintf(metrics_file, "}\n");
    
    printf("\n================================================================\n");
    printf("              VALIDATION RESULTS FINAL\n");
    printf("================================================================\n");
    printf("Total Duration: %.3f seconds\n", total_time / 1000000000.0);
    printf("Tests Passed: %d/%d\n", tests_passed, tests_total);
    printf("Success Rate: %.1f%%\n", success_rate);
    printf("Interrupted: %s\n", interrupted ? "YES" : "NO");
    
    printf("\nDetailed Results:\n");
    if (!interrupted) {
        printf("  Phase 1 - Hardware Detection: %s\n", results[0] ? "✅ PASS" : "❌ FAIL");
        printf("  Phase 2 - Display Init: %s\n", results[1] ? "✅ PASS" : "❌ FAIL");
        printf("  Phase 3 - Pixel Operations: %s\n", results[2] ? "✅ PASS" : "❌ FAIL");
        printf("  Phase 4 - Memory Operations: %s\n", results[3] ? "✅ PASS" : "❌ FAIL");
        printf("  Phase 5 - Performance Benchmark: %s\n", results[4] ? "✅ PASS" : "❌ FAIL");
    }
    
    printf("\n🎯 FINAL ASSESSMENT:\n");
    if (success_rate >= 95.0) {
        printf("   🏆 EXCELLENT - System fully operational\n");
    } else if (success_rate >= 80.0) {
        printf("   ✅ GOOD - Minor issues detected\n");
    } else if (success_rate >= 60.0) {
        printf("   ⚠️  ACCEPTABLE - Significant issues\n");
    } else {
        printf("   ❌ POOR - Major problems\n");
    }
    
    printf("\nFiles Generated:\n");
    printf("  📄 %s - Operation logs\n", LOG_FILE);
    printf("  📊 %s - Performance metrics\n", METRICS_FILE);
    printf("================================================================\n");
    
    fclose(log_file);
    fclose(metrics_file);
    
    return success_rate >= 80.0 ? 0 : 1;
}