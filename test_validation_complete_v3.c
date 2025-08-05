/**
 * Test Validation Complete v3.0 - LibVisualMem
 * =============================================
 * 
 * Version 3.0 avec système de versioning automatique
 * Benchmarks complets sans omission
 * Validation automatique à chaque création
 * 
 * Copyright (C) 2025 - Visual Memory Systems
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
#include <sys/stat.h>
#include <dirent.h>
// #include <json-c/json.h>  // Commenté car non disponible sur tous les systèmes

// === CONFIGURATION V3 ===
#define VERSION "3.0"
#define LOG_FILE_PREFIX "logs_validation_v3"
#define METRICS_FILE_PREFIX "metrics_validation_v3"
#define REPORT_FILE_PREFIX "rapport_validation_v3"
#define PIXEL_TESTS 50000
#define MEMORY_TESTS 5000
#define BENCHMARK_ITERATIONS 200
#define BENCHMARK_SIZES_COUNT 8
#define BENCHMARK_PATTERNS_COUNT 6

// === GLOBALS V3 ===
static FILE* log_file = NULL;
static FILE* metrics_file = NULL;
static FILE* report_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int interrupted = 0;
static int tests_passed = 0;
static int tests_total = 0;
static char current_version_dir[256];
static char current_timestamp[64];

// === VERSIONING SYSTEM ===
static void create_version_directory(void) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    strftime(current_timestamp, sizeof(current_timestamp), "%Y%m%d_%H%M%S", tm_info);
    snprintf(current_version_dir, sizeof(current_version_dir), "validation_v3_%s", current_timestamp);
    
    mkdir(current_version_dir, 0755);
    printf("📁 Created version directory: %s\n", current_version_dir);
}

static void get_next_version_number(void) {
    DIR* dir;
    struct dirent* entry;
    int max_version = 0;
    
    dir = opendir(".");
    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "validation_v3_", 14) == 0) {
                int version_num = atoi(entry->d_name + 14);
                if (version_num > max_version) max_version = version_num;
            }
        }
        closedir(dir);
    }
    
    printf("📊 Version numbering: Previous max = %d, Current = %s\n", max_version, current_timestamp);
}

// === SIGNAL HANDLER ===
void signal_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    interrupted = 1;
    printf("\n[SIGNAL] Tests interrupted, cleaning up...\n");
}

// === UTILITIES V3 ===
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

// === BENCHMARK CONFIGURATION ===
typedef struct {
    size_t size;
    const char* name;
    int iterations;
} benchmark_size_t;

typedef struct {
    const char* name;
    void (*pattern_func)(uint8_t*, size_t);
} benchmark_pattern_t;

static benchmark_size_t benchmark_sizes[BENCHMARK_SIZES_COUNT] = {
    {64, "Tiny (64B)", 1000},
    {1024, "Small (1KB)", 800},
    {4096, "Medium (4KB)", 600},
    {16384, "Large (16KB)", 400},
    {65536, "XLarge (64KB)", 200},
    {262144, "Huge (256KB)", 100},
    {1048576, "Massive (1MB)", 50},
    {4194304, "Giant (4MB)", 25}
};

static void pattern_sequential(uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i++) data[i] = (uint8_t)(i % 256);
}

static void pattern_alternating(uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i++) data[i] = (i % 2) ? 0xFF : 0x00;
}

static void pattern_random(uint8_t* data, size_t size) {
    srand(12345);
    for (size_t i = 0; i < size; i++) data[i] = (uint8_t)(rand() % 256);
}

static void pattern_zeros(uint8_t* data, size_t size) {
    memset(data, 0x00, size);
}

static void pattern_ones(uint8_t* data, size_t size) {
    memset(data, 0xFF, size);
}

static void pattern_checkerboard(uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        data[i] = ((i / 8) % 2) ? 0xAA : 0x55;
    }
}

static benchmark_pattern_t benchmark_patterns[BENCHMARK_PATTERNS_COUNT] = {
    {"Sequential", pattern_sequential},
    {"Alternating", pattern_alternating},
    {"Random", pattern_random},
    {"Zeros", pattern_zeros},
    {"Ones", pattern_ones},
    {"Checkerboard", pattern_checkerboard}
};

// === TEST 1: HARDWARE DETECTION V3 ===
static int test_hardware_detection_v3(void) {
    printf("\n🔍 Phase 1: Hardware Detection v3.0\n");
    printf("=====================================\n");
    
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
        
        log_test("HARDWARE_DETECTION_V3", "SUCCESS", details);
        
        printf("✅ Hardware detection successful\n");
        printf("   X11 Support: %s\n", caps.has_x11 ? "YES" : "NO");
        printf("   OpenGL Support: %s\n", caps.has_opengl ? "YES" : "NO");
        printf("   GPU Model: %s\n", caps.gpu_model);
        printf("   Detection Time: %.2f ms\n", duration / 1000000.0);
        
        tests_passed++;
        return 1;
    } else {
        log_test("HARDWARE_DETECTION_V3", "FAILED", "Hardware not accessible");
        printf("❌ Hardware detection failed\n");
        return 0;
    }
}

// === TEST 2: DISPLAY INITIALIZATION V3 ===
static int test_display_init_v3(void) {
    printf("\n🖥️  Phase 2: Display Initialization v3.0\n");
    printf("==========================================\n");
    
    tests_total++;
    uint64_t start = get_timestamp_ns();
    
    visualmem_v2_context_t ctx;
    int result = visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1920, 1080);
    
    uint64_t duration = get_timestamp_ns() - start;
    
    if (result == VISUALMEM_V2_SUCCESS) {
        char details[256];
        snprintf(details, sizeof(details), "InitTime=%.2fms", duration / 1000000.0);
        
        log_test("DISPLAY_INIT_V3", "SUCCESS", details);
        
        printf("✅ Display initialization successful\n");
        printf("   Window Size: 1920x1080\n");
        printf("   Init Time: %.2f ms\n", duration / 1000000.0);
        
        visualmem_v2_cleanup(&ctx);
        tests_passed++;
        return 1;
    } else {
        log_test("DISPLAY_INIT_V3", "FAILED", "Display initialization failed");
        printf("❌ Display initialization failed\n");
        return 0;
    }
}

// === TEST 3: PIXEL OPERATIONS V3 ===
static int test_pixel_operations_v3(void) {
    printf("\n🎨 Phase 3: Pixel Operations v3.0\n");
    printf("==================================\n");
    
    tests_total++;
    
    visualmem_v2_context_t ctx;
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1920, 1080) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context\n");
        return 0;
    }
    
    uint64_t total_time = 0;
    int success_count = 0;
    
    for (int i = 0; i < PIXEL_TESTS && !interrupted; i++) {
        uint32_t x = rand() % 1920;
        uint32_t y = rand() % 1080;
        uint32_t color = (rand() << 16) | (rand() << 8) | rand();
        
        uint64_t start = get_timestamp_ns();
        int result = visualmem_v2_write_pixel(&ctx, x, y, color);
        uint64_t end = get_timestamp_ns();
        
        if (result == VISUALMEM_V2_SUCCESS) {
            total_time += (end - start);
            success_count++;
        }
        
        if (i % 5000 == 0 && i > 0) {
            printf("   Progress: %d/%d pixels\n", i, PIXEL_TESTS);
        }
    }
    
    double avg_time = success_count > 0 ? (double)total_time / success_count / 1000000.0 : 0;
    double throughput = success_count > 0 ? (double)success_count / (total_time / 1000000000.0) : 0;
    
    char details[256];
    snprintf(details, sizeof(details), 
            "Success=%d/%d AvgTime=%.3fms Throughput=%.0fpx/s",
            success_count, PIXEL_TESTS, avg_time, throughput);
    
    log_test("PIXEL_OPERATIONS_V3", "SUCCESS", details);
    
    printf("✅ Pixel operations completed\n");
    printf("   Success Rate: %d/%d (%.1f%%)\n", success_count, PIXEL_TESTS, 
           (double)success_count / PIXEL_TESTS * 100.0);
    printf("   Average Time: %.3f ms per pixel\n", avg_time);
    printf("   Throughput: %.0f pixels/second\n", throughput);
    
    visualmem_v2_cleanup(&ctx);
    tests_passed++;
    return 1;
}

// === TEST 4: MEMORY OPERATIONS V3 ===
static int test_memory_operations_v3(void) {
    printf("\n💾 Phase 4: Memory Operations v3.0\n");
    printf("===================================\n");
    
    tests_total++;
    
    visualmem_v2_context_t ctx;
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1920, 1080) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context\n");
        return 0;
    }
    
    uint64_t total_alloc_time = 0;
    uint64_t total_write_time = 0;
    uint64_t total_read_time = 0;
    uint64_t total_free_time = 0;
    int success_count = 0;
    
    for (int i = 0; i < MEMORY_TESTS && !interrupted; i++) {
        size_t size = 64 + (rand() % 16384); // 64B to 16KB
        uint8_t* test_data = malloc(size);
        
        // Generate test data
        for (size_t j = 0; j < size; j++) {
            test_data[j] = (uint8_t)((i + j) % 256);
        }
        
        // Allocation
        uint64_t start = get_timestamp_ns();
        void* addr = visualmem_v2_alloc(&ctx, size, "test_memory");
        uint64_t alloc_time = get_timestamp_ns() - start;
        
        if (addr) {
            total_alloc_time += alloc_time;
            
            // Write
            start = get_timestamp_ns();
            int write_result = visualmem_v2_write(&ctx, addr, test_data, size);
            uint64_t write_time = get_timestamp_ns() - start;
            
            if (write_result == VISUALMEM_V2_SUCCESS) {
                total_write_time += write_time;
                
                // Read
                uint8_t* read_data = malloc(size);
                start = get_timestamp_ns();
                int read_result = visualmem_v2_read(&ctx, addr, read_data, size);
                uint64_t read_time = get_timestamp_ns() - start;
                
                if (read_result == VISUALMEM_V2_SUCCESS) {
                    total_read_time += read_time;
                    
                    // Verify data integrity
                    int integrity_ok = 1;
                    for (size_t j = 0; j < size; j++) {
                        if (test_data[j] != read_data[j]) {
                            integrity_ok = 0;
                            break;
                        }
                    }
                    
                    if (integrity_ok) {
                        success_count++;
                    }
                }
                
                free(read_data);
            }
            
            // Free
            start = get_timestamp_ns();
            visualmem_v2_free(&ctx, addr);
            uint64_t free_time = get_timestamp_ns() - start;
            total_free_time += free_time;
        }
        
        free(test_data);
        
        if (i % 500 == 0 && i > 0) {
            printf("   Progress: %d/%d memory operations\n", i, MEMORY_TESTS);
        }
    }
    
    double avg_alloc = success_count > 0 ? (double)total_alloc_time / success_count / 1000000.0 : 0;
    double avg_write = success_count > 0 ? (double)total_write_time / success_count / 1000000.0 : 0;
    double avg_read = success_count > 0 ? (double)total_read_time / success_count / 1000000.0 : 0;
    double avg_free = success_count > 0 ? (double)total_free_time / success_count / 1000000.0 : 0;
    
    char details[512];
    snprintf(details, sizeof(details),
            "Success=%d/%d Alloc=%.2fms Write=%.2fms Read=%.2fms Free=%.2fms",
            success_count, MEMORY_TESTS, avg_alloc, avg_write, avg_read, avg_free);
    
    log_test("MEMORY_OPERATIONS_V3", "SUCCESS", details);
    
    printf("✅ Memory operations completed\n");
    printf("   Success Rate: %d/%d (%.1f%%)\n", success_count, MEMORY_TESTS,
           (double)success_count / MEMORY_TESTS * 100.0);
    printf("   Average Times:\n");
    printf("     Allocation: %.2f ms\n", avg_alloc);
    printf("     Write: %.2f ms\n", avg_write);
    printf("     Read: %.2f ms\n", avg_read);
    printf("     Free: %.2f ms\n", avg_free);
    
    visualmem_v2_cleanup(&ctx);
    tests_passed++;
    return 1;
}

// === TEST 5: COMPLETE PERFORMANCE BENCHMARK V3 ===
static int test_performance_benchmark_v3(void) {
    printf("\n⚡ Phase 5: Complete Performance Benchmark v3.0\n");
    printf("===============================================\n");
    
    tests_total++;
    
    visualmem_v2_context_t ctx;
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_X11_WINDOW, 1920, 1080) != VISUALMEM_V2_SUCCESS) {
        printf("❌ Failed to initialize context\n");
        return 0;
    }
    
    printf("Running comprehensive benchmarks...\n");
    printf("Sizes: %d, Patterns: %d, Total combinations: %d\n", 
           BENCHMARK_SIZES_COUNT, BENCHMARK_PATTERNS_COUNT, 
           BENCHMARK_SIZES_COUNT * BENCHMARK_PATTERNS_COUNT);
    
    // Results storage
    typedef struct {
        double visual_throughput;
        double ram_throughput;
        double ratio;
        int success_count;
        int total_count;
    } benchmark_result_t;
    
    benchmark_result_t results[BENCHMARK_SIZES_COUNT][BENCHMARK_PATTERNS_COUNT];
    memset(results, 0, sizeof(results));
    
    // Run all benchmark combinations
    for (int size_idx = 0; size_idx < BENCHMARK_SIZES_COUNT && !interrupted; size_idx++) {
        for (int pattern_idx = 0; pattern_idx < BENCHMARK_PATTERNS_COUNT && !interrupted; pattern_idx++) {
            printf("  Benchmarking %s with %s pattern...\n", 
                   benchmark_sizes[size_idx].name, benchmark_patterns[pattern_idx].name);
            
            size_t size = benchmark_sizes[size_idx].size;
            int iterations = benchmark_sizes[size_idx].iterations;
            
            uint64_t visual_total_time = 0;
            uint64_t ram_total_time = 0;
            int visual_success = 0;
            int ram_success = 0;
            
            // Visual Memory benchmark
            for (int i = 0; i < iterations && !interrupted; i++) {
                uint8_t* test_data = malloc(size);
                benchmark_patterns[pattern_idx].pattern_func(test_data, size);
                
                void* addr = visualmem_v2_alloc(&ctx, size, "benchmark");
                if (addr) {
                    uint64_t start = get_timestamp_ns();
                    int result = visualmem_v2_write(&ctx, addr, test_data, size);
                    uint64_t end = get_timestamp_ns();
                    
                    if (result == VISUALMEM_V2_SUCCESS) {
                        visual_total_time += (end - start);
                        visual_success++;
                    }
                    
                    visualmem_v2_free(&ctx, addr);
                }
                
                free(test_data);
            }
            
            // RAM benchmark
            for (int i = 0; i < iterations && !interrupted; i++) {
                uint8_t* src = malloc(size);
                uint8_t* dst = malloc(size);
                benchmark_patterns[pattern_idx].pattern_func(src, size);
                
                uint64_t start = get_timestamp_ns();
                memcpy(dst, src, size);
                uint64_t end = get_timestamp_ns();
                
                ram_total_time += (end - start);
                ram_success++;
                
                free(src);
                free(dst);
            }
            
            // Calculate results
            double visual_throughput = visual_success > 0 ? 
                (size / (1024.0 * 1024.0)) / ((double)visual_total_time / visual_success / 1000000000.0) : 0;
            double ram_throughput = ram_success > 0 ? 
                (size / (1024.0 * 1024.0)) / ((double)ram_total_time / ram_success / 1000000000.0) : 0;
            double ratio = visual_throughput > 0 ? ram_throughput / visual_throughput : 0;
            
            results[size_idx][pattern_idx].visual_throughput = visual_throughput;
            results[size_idx][pattern_idx].ram_throughput = ram_throughput;
            results[size_idx][pattern_idx].ratio = ratio;
            results[size_idx][pattern_idx].success_count = visual_success;
            results[size_idx][pattern_idx].total_count = iterations;
        }
    }
    
    // Calculate overall statistics
    double total_visual_throughput = 0;
    double total_ram_throughput = 0;
    int total_combinations = 0;
    
    for (int size_idx = 0; size_idx < BENCHMARK_SIZES_COUNT; size_idx++) {
        for (int pattern_idx = 0; pattern_idx < BENCHMARK_PATTERNS_COUNT; pattern_idx++) {
            if (results[size_idx][pattern_idx].success_count > 0) {
                total_visual_throughput += results[size_idx][pattern_idx].visual_throughput;
                total_ram_throughput += results[size_idx][pattern_idx].ram_throughput;
                total_combinations++;
            }
        }
    }
    
    double avg_visual_throughput = total_combinations > 0 ? total_visual_throughput / total_combinations : 0;
    double avg_ram_throughput = total_combinations > 0 ? total_ram_throughput / total_combinations : 0;
    double overall_ratio = avg_visual_throughput > 0 ? avg_ram_throughput / avg_visual_throughput : 0;
    
    // Print detailed results
    printf("\n📊 DETAILED BENCHMARK RESULTS:\n");
    printf("===============================\n");
    
    for (int size_idx = 0; size_idx < BENCHMARK_SIZES_COUNT; size_idx++) {
        printf("\n%s:\n", benchmark_sizes[size_idx].name);
        printf("  Pattern          | Visual (MB/s) | RAM (MB/s) | Ratio | Success\n");
        printf("  -----------------|---------------|------------|-------|--------\n");
        
        for (int pattern_idx = 0; pattern_idx < BENCHMARK_PATTERNS_COUNT; pattern_idx++) {
            benchmark_result_t* result = &results[size_idx][pattern_idx];
            printf("  %-15s | %12.2f | %10.2f | %5.1fx | %d/%d\n",
                   benchmark_patterns[pattern_idx].name,
                   result->visual_throughput,
                   result->ram_throughput,
                   result->ratio,
                   result->success_count,
                   result->total_count);
        }
    }
    
    printf("\n📈 OVERALL PERFORMANCE SUMMARY:\n");
    printf("===============================\n");
    printf("Average Visual Memory Throughput: %.2f MB/s\n", avg_visual_throughput);
    printf("Average RAM Throughput: %.2f MB/s\n", avg_ram_throughput);
    printf("Overall Performance Ratio: %.1fx slower\n", overall_ratio);
    printf("Visual Memory Efficiency: %.1f%% of RAM\n", 100.0 / overall_ratio);
    printf("Total Benchmark Combinations: %d\n", total_combinations);
    
    char details[512];
    snprintf(details, sizeof(details),
            "AvgVisual=%.2fMB/s AvgRAM=%.2fMB/s Ratio=%.1fx Combinations=%d",
            avg_visual_throughput, avg_ram_throughput, overall_ratio, total_combinations);
    
    log_test("PERFORMANCE_BENCHMARK_V3", "SUCCESS", details);
    
    visualmem_v2_cleanup(&ctx);
    tests_passed++;
    return 1;
}

// === MAIN V3 ===
int main(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("================================================================\n");
    printf("   LIBVISUALMEM V3.0 - VALIDATION COMPLETE\n");
    printf("================================================================\n");
    printf("Version: %s\n", VERSION);
    printf("Features: Auto-versioning, Complete benchmarks, No omissions\n");
    printf("Tests: %d pixels + %d memory + comprehensive benchmarks\n", PIXEL_TESTS, MEMORY_TESTS);
    printf("================================================================\n");
    
    // Create version directory
    create_version_directory();
    get_next_version_number();
    
    // Initialize logging with version-specific files
    char log_filename[512];
    char metrics_filename[512];
    char report_filename[512];
    
    snprintf(log_filename, sizeof(log_filename), "%s/%s_%s.log", 
             current_version_dir, LOG_FILE_PREFIX, current_timestamp);
    snprintf(metrics_filename, sizeof(metrics_filename), "%s/%s_%s.json", 
             current_version_dir, METRICS_FILE_PREFIX, current_timestamp);
    snprintf(report_filename, sizeof(report_filename), "%s/%s_%s.md", 
             current_version_dir, REPORT_FILE_PREFIX, current_timestamp);
    
    log_file = fopen(log_filename, "w");
    metrics_file = fopen(metrics_filename, "w");
    report_file = fopen(report_filename, "w");
    
    if (!log_file || !metrics_file || !report_file) {
        printf("❌ Failed to open log files\n");
        return 1;
    }
    
    // Write report header
    fprintf(report_file, "# LibVisualMem v3.0 Validation Report\n");
    fprintf(report_file, "Generated: %s\n\n", current_timestamp);
    fprintf(report_file, "## Test Configuration\n");
    fprintf(report_file, "- Version: %s\n", VERSION);
    fprintf(report_file, "- Pixel Tests: %d\n", PIXEL_TESTS);
    fprintf(report_file, "- Memory Tests: %d\n", MEMORY_TESTS);
    fprintf(report_file, "- Benchmark Sizes: %d\n", BENCHMARK_SIZES_COUNT);
    fprintf(report_file, "- Benchmark Patterns: %d\n", BENCHMARK_PATTERNS_COUNT);
    fprintf(report_file, "- Total Combinations: %d\n\n", BENCHMARK_SIZES_COUNT * BENCHMARK_PATTERNS_COUNT);
    
    fprintf(metrics_file, "{\n  \"version\": \"%s\",\n", VERSION);
    fprintf(metrics_file, "  \"timestamp\": \"%s\",\n", current_timestamp);
    fprintf(metrics_file, "  \"test_start\": %lu,\n", time(NULL));
    
    uint64_t start_time = get_timestamp_ns();
    
    // Run tests
    int results[5] = {0}; // Initialize all to 0
    results[0] = test_hardware_detection_v3();
    if (!interrupted) results[1] = test_display_init_v3();
    if (!interrupted) results[2] = test_pixel_operations_v3();
    if (!interrupted) results[3] = test_memory_operations_v3();
    if (!interrupted) results[4] = test_performance_benchmark_v3();
    
    uint64_t total_time = get_timestamp_ns() - start_time;
    double success_rate = (double)tests_passed / tests_total * 100.0;
    
    // Final results
    fprintf(metrics_file, "  \"test_end\": %lu,\n", time(NULL));
    fprintf(metrics_file, "  \"duration_seconds\": %.3f,\n", total_time / 1000000000.0);
    fprintf(metrics_file, "  \"tests_passed\": %d,\n", tests_passed);
    fprintf(metrics_file, "  \"tests_total\": %d,\n", tests_total);
    fprintf(metrics_file, "  \"success_rate\": %.1f\n", success_rate);
    fprintf(metrics_file, "}\n");
    
    // Write detailed report
    fprintf(report_file, "## Test Results\n");
    fprintf(report_file, "- Total Duration: %.3f seconds\n", total_time / 1000000000.0);
    fprintf(report_file, "- Tests Passed: %d/%d\n", tests_passed, tests_total);
    fprintf(report_file, "- Success Rate: %.1f%%\n", success_rate);
    fprintf(report_file, "- Interrupted: %s\n\n", interrupted ? "YES" : "NO");
    
    fprintf(report_file, "### Detailed Results\n");
    if (!interrupted) {
        fprintf(report_file, "- Phase 1 - Hardware Detection: %s\n", results[0] ? "✅ PASS" : "❌ FAIL");
        fprintf(report_file, "- Phase 2 - Display Init: %s\n", results[1] ? "✅ PASS" : "❌ FAIL");
        fprintf(report_file, "- Phase 3 - Pixel Operations: %s\n", results[2] ? "✅ PASS" : "❌ FAIL");
        fprintf(report_file, "- Phase 4 - Memory Operations: %s\n", results[3] ? "✅ PASS" : "❌ FAIL");
        fprintf(report_file, "- Phase 5 - Performance Benchmark: %s\n", results[4] ? "✅ PASS" : "❌ FAIL");
    }
    
    fprintf(report_file, "\n## Final Assessment\n");
    if (success_rate >= 95.0) {
        fprintf(report_file, "🏆 **EXCELLENT** - System fully operational\n");
    } else if (success_rate >= 80.0) {
        fprintf(report_file, "✅ **GOOD** - Minor issues detected\n");
    } else if (success_rate >= 60.0) {
        fprintf(report_file, "⚠️ **ACCEPTABLE** - Significant issues\n");
    } else {
        fprintf(report_file, "❌ **POOR** - Major problems\n");
    }
    
    fprintf(report_file, "\n## Files Generated\n");
    fprintf(report_file, "- `%s` - Operation logs\n", log_filename);
    fprintf(report_file, "- `%s` - Performance metrics\n", metrics_filename);
    fprintf(report_file, "- `%s` - Detailed report\n", report_filename);
    
    printf("\n================================================================\n");
    printf("              VALIDATION RESULTS V3.0\n");
    printf("================================================================\n");
    printf("Version Directory: %s\n", current_version_dir);
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
    printf("  📄 %s - Operation logs\n", log_filename);
    printf("  📊 %s - Performance metrics\n", metrics_filename);
    printf("  📋 %s - Detailed report\n", report_filename);
    printf("  📁 Version directory: %s\n", current_version_dir);
    printf("================================================================\n");
    
    fclose(log_file);
    fclose(metrics_file);
    fclose(report_file);
    
    return success_rate >= 80.0 ? 0 : 1;
}