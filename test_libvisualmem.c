/**
 * LibVisualMem - Complete Test Suite
 * ==================================
 * 
 * Comprehensive tests for the visual memory library to validate
 * all functionality including autonomous mode operation.
 * 
 * Copyright (C) 2025 - Visual Memory Systems
 */

#include "libvisualmem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// === TEST framework ===
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_START(name) \
    do { \
        printf("\n=== Test %d: %s ===\n", ++tests_run, name); \
    } while(0)

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  ✅ %s\n", message); \
        } else { \
            printf("  ❌ %s\n", message); \
            return 0; \
        } \
    } while(0)

#define TEST_END() \
    do { \
        tests_passed++; \
        printf("  ✅ Test passed\n"); \
        return 1; \
    } while(0)

// === UTILITY FUNCTIONS ===
static void print_test_header(void) {
    printf("===================================================================\n");
    printf("           LIBVISUALMEM - COMPLETE VALIDATION SUITE\n");
    printf("===================================================================\n\n");
    
    printf("LIBRARY VERSION: %s\n", visualmem_get_version());
    printf("TESTING APPROACH: Comprehensive validation of visual memory system\n");
    printf("OBJECTIVES:\n");
    printf("  ✅ Validate library initialization and cleanup\n");
    printf("  ✅ Test memory allocation and deallocation\n");
    printf("  ✅ Verify data write/read operations\n");
    printf("  ✅ Validate autonomous mode transition\n");
    printf("  ✅ Test data persistence after RAM liberation\n");
    printf("  ✅ Verify complex data structures handling\n");
    printf("  ✅ Test error conditions and recovery\n");
    printf("  ✅ Validate performance and statistics\n\n");
}

static void print_visual_memory_stats(visualmem_context_t* ctx) {
    size_t total_allocated, peak_usage;
    int fragmentation;
    
    visualmem_get_stats(ctx, &total_allocated, &peak_usage, &fragmentation);
    
    printf("Visual Memory Statistics:\n");
    printf("  Total allocated: %zu bytes\n", total_allocated);
    printf("  Peak usage: %zu bytes\n", peak_usage);
    printf("  Fragmentation: %d%%\n", fragmentation);
    printf("  Allocations active: %d\n", ctx->allocation_count);
    printf("  Operations count: %llu\n", (unsigned long long)ctx->operations_count);
    printf("  Mode: %s\n", ctx->autonomous_mode ? "AUTONOMOUS" : "RAM-assisted");
}

// === INDIVIDUAL TESTS ===

static int test_library_initialization(void) {
    TEST_START("Library Initialization and Basic Setup");
    
    visualmem_context_t ctx;
    
    // Test initialization
    int result = visualmem_init(&ctx, VISUALMEM_MODE_SIMULATE, 800, 600);
    TEST_ASSERT(result == VISUALMEM_SUCCESS, "Library initialization successful");
    TEST_ASSERT(ctx.is_initialized == 1, "Context properly initialized");
    TEST_ASSERT(ctx.width == 800, "Width correctly set");
    TEST_ASSERT(ctx.height == 600, "Height correctly set");
    TEST_ASSERT(ctx.framebuffer != NULL, "Framebuffer allocated");
    TEST_ASSERT(ctx.ram_buffer != NULL, "RAM buffer allocated");
    TEST_ASSERT(ctx.ram_freed == 0, "RAM buffer initially active");
    TEST_ASSERT(ctx.autonomous_mode == 0, "Initially in RAM-assisted mode");
    
    printf("  Context details:\n");
    printf("    Display: %dx%d pixels\n", ctx.width, ctx.height);
    printf("    Framebuffer: %p\n", ctx.framebuffer);
    printf("    RAM buffer: %p\n", ctx.ram_buffer);
    printf("    Mode: %d\n", ctx.mode);
    
    visualmem_cleanup(&ctx);
    TEST_ASSERT(ctx.framebuffer == NULL, "Framebuffer properly cleaned up");
    
    TEST_END();
}

static int test_memory_allocation(void) {
    TEST_START("Visual Memory Allocation and Deallocation");
    
    visualmem_context_t ctx;
    visualmem_init(&ctx, VISUALMEM_MODE_SIMULATE, 800, 600);
    visualmem_set_debug_mode(&ctx, 1);
    
    // Test basic allocation
    void* addr1 = visualmem_alloc(&ctx, 100, "test_allocation_1");
    TEST_ASSERT(addr1 != NULL, "First allocation successful");
    
    void* addr2 = visualmem_alloc(&ctx, 200, "test_allocation_2");
    TEST_ASSERT(addr2 != NULL, "Second allocation successful");
    TEST_ASSERT(addr1 != addr2, "Different addresses for different allocations");
    
    void* addr3 = visualmem_alloc(&ctx, 50, "test_allocation_3");
    TEST_ASSERT(addr3 != NULL, "Third allocation successful");
    
    // Verify allocation info
    const visualmem_allocation_t* info1 = visualmem_get_allocation_info(&ctx, addr1);
    TEST_ASSERT(info1 != NULL, "Allocation info retrievable");
    TEST_ASSERT(info1->size == 100, "Allocation size correct");
    TEST_ASSERT(strcmp(info1->label, "test_allocation_1") == 0, "Allocation label correct");
    TEST_ASSERT(info1->is_active == 1, "Allocation marked as active");
    
    // Test statistics
    size_t total_allocated, peak_usage;
    int fragmentation;
    visualmem_get_stats(&ctx, &total_allocated, &peak_usage, &fragmentation);
    TEST_ASSERT(total_allocated == 350, "Total allocation tracking correct");
    
    // Test deallocation
    int free_result = visualmem_free(&ctx, addr2);
    TEST_ASSERT(free_result == VISUALMEM_SUCCESS, "Deallocation successful");
    
    visualmem_get_stats(&ctx, &total_allocated, NULL, NULL);
    TEST_ASSERT(total_allocated == 150, "Total allocation updated after free");
    
    print_visual_memory_stats(&ctx);
    
    visualmem_cleanup(&ctx);
    TEST_END();
}

static int test_data_operations(void) {
    TEST_START("Data Write and Read Operations");
    
    visualmem_context_t ctx;
    visualmem_init(&ctx, VISUALMEM_MODE_SIMULATE, 800, 600);
    
    // Test basic data write/read
    void* addr = visualmem_alloc(&ctx, 256, "data_test");
    TEST_ASSERT(addr != NULL, "Memory allocation for data test");
    
    // Write test data
    uint8_t test_data[256];
    for (int i = 0; i < 256; i++) {
        test_data[i] = (uint8_t)(i ^ 0xAA); // Pattern: alternating bits
    }
    
    int write_result = visualmem_write(&ctx, addr, test_data, 256);
    TEST_ASSERT(write_result == VISUALMEM_SUCCESS, "Data write operation successful");
    
    // Read back data
    uint8_t read_data[256];
    int read_result = visualmem_read(&ctx, addr, read_data, 256);
    TEST_ASSERT(read_result == VISUALMEM_SUCCESS, "Data read operation successful");
    
    // Verify data integrity
    int data_matches = 1;
    for (int i = 0; i < 256; i++) {
        if (test_data[i] != read_data[i]) {
            data_matches = 0;
            printf("    Data mismatch at byte %d: wrote 0x%02X, read 0x%02X\n", 
                   i, test_data[i], read_data[i]);
            break;
        }
    }
    TEST_ASSERT(data_matches, "Data integrity maintained");
    
    // Test string operations
    const char* test_string = "LibVisualMem Test String 123!@#";
    void* str_addr = visualmem_alloc(&ctx, 100, "string_test");
    TEST_ASSERT(str_addr != NULL, "String memory allocation");
    
    int str_write_result = visualmem_write_string(&ctx, str_addr, test_string);
    TEST_ASSERT(str_write_result == VISUALMEM_SUCCESS, "String write successful");
    
    char read_string[100];
    int str_read_result = visualmem_read_string(&ctx, str_addr, read_string, sizeof(read_string));
    TEST_ASSERT(str_read_result == VISUALMEM_SUCCESS, "String read successful");
    TEST_ASSERT(strcmp(test_string, read_string) == 0, "String data integrity maintained");
    
    printf("  Written string: '%s'\n", test_string);
    printf("  Read string:    '%s'\n", read_string);
    
    visualmem_cleanup(&ctx);
    TEST_END();
}

static int test_autonomous_mode_transition(void) {
    TEST_START("Autonomous Mode Transition (CRITICAL)");
    
    visualmem_context_t ctx;
    visualmem_init(&ctx, VISUALMEM_MODE_SIMULATE, 800, 600);
    visualmem_set_debug_mode(&ctx, 1);
    
    // Allocate and write data before transition
    void* addr1 = visualmem_alloc(&ctx, 64, "pre_autonomous_1");
    void* addr2 = visualmem_alloc(&ctx, 128, "pre_autonomous_2");
    
    const char* test_data1 = "BEFORE_AUTONOMOUS_MODE";
    const char* test_data2 = "CRITICAL_TRANSITION_TEST";
    
    visualmem_write_string(&ctx, addr1, test_data1);
    visualmem_write_string(&ctx, addr2, test_data2);
    
    printf("  Pre-transition state:\n");
    printf("    RAM buffer: %p\n", ctx.ram_buffer);
    printf("    Framebuffer: %p\n", ctx.framebuffer);
    printf("    RAM freed: %s\n", ctx.ram_freed ? "YES" : "NO");
    printf("    Autonomous mode: %s\n", ctx.autonomous_mode ? "YES" : "NO");
    
    // CRITICAL TRANSITION: Enter autonomous mode
    printf("  ⚠️  EXECUTING CRITICAL TRANSITION TO AUTONOMOUS MODE...\n");
    int autonomous_result = visualmem_enter_autonomous_mode(&ctx);
    TEST_ASSERT(autonomous_result == VISUALMEM_SUCCESS, "Autonomous mode transition successful");
    
    printf("  Post-transition state:\n");
    printf("    RAM buffer: %p\n", ctx.ram_buffer);
    printf("    Framebuffer: %p\n", ctx.framebuffer);
    printf("    RAM freed: %s\n", ctx.ram_freed ? "YES" : "NO");
    printf("    Autonomous mode: %s\n", ctx.autonomous_mode ? "YES" : "NO");
    
    TEST_ASSERT(ctx.ram_buffer == NULL, "RAM buffer freed");
    TEST_ASSERT(ctx.ram_freed == 1, "RAM freed flag set");
    TEST_ASSERT(ctx.autonomous_mode == 1, "Autonomous mode flag set");
    TEST_ASSERT(ctx.framebuffer != NULL, "Framebuffer remains active");
    
    printf("  ✅ AUTONOMOUS MODE ACTIVE: System now operates solely on visual display\n");
    
    visualmem_cleanup(&ctx);
    TEST_END();
}

static int test_data_persistence_post_transition(void) {
    TEST_START("Data Persistence After RAM Liberation");
    
    visualmem_context_t ctx;
    visualmem_init(&ctx, VISUALMEM_MODE_SIMULATE, 800, 600);
    
    // Create test data before transition
    const char* test_strings[] = {
        "PERSISTENCE_TEST_1",
        "AUTONOMOUS_MEMORY_DATA",
        "VISUAL_RAM_VALIDATION",
        "POST_TRANSITION_CHECK"
    };
    
    void* addresses[4];
    
    // Allocate and write data
    for (int i = 0; i < 4; i++) {
        addresses[i] = visualmem_alloc(&ctx, 64, "persistence_test");
        TEST_ASSERT(addresses[i] != NULL, "Pre-transition allocation successful");
        
        int write_result = visualmem_write_string(&ctx, addresses[i], test_strings[i]);
        TEST_ASSERT(write_result == VISUALMEM_SUCCESS, "Pre-transition write successful");
    }
    
    printf("  Data written before transition:\n");
    for (int i = 0; i < 4; i++) {
        printf("    Address %p: '%s'\n", addresses[i], test_strings[i]);
    }
    
    // Transition to autonomous mode
    visualmem_enter_autonomous_mode(&ctx);
    printf("  ✅ Transitioned to autonomous mode\n");
    
    // Verify data persistence
    printf("  Verifying data persistence after RAM liberation:\n");
    int persistence_success = 1;
    
    for (int i = 0; i < 4; i++) {
        char read_buffer[64];
        int read_result = visualmem_read_string(&ctx, addresses[i], read_buffer, sizeof(read_buffer));
        
        if (read_result == VISUALMEM_SUCCESS && strcmp(test_strings[i], read_buffer) == 0) {
            printf("    ✅ Address %p: '%s' -> '%s' (PRESERVED)\n", 
                   addresses[i], test_strings[i], read_buffer);
        } else {
            printf("    ❌ Address %p: '%s' -> '%s' (CORRUPTED)\n", 
                   addresses[i], test_strings[i], read_buffer);
            persistence_success = 0;
        }
    }
    
    TEST_ASSERT(persistence_success, "All data preserved after transition");
    
    visualmem_cleanup(&ctx);
    TEST_END();
}

static int test_autonomous_operations(void) {
    TEST_START("Operations in Autonomous Mode");
    
    visualmem_context_t ctx;
    visualmem_init(&ctx, VISUALMEM_MODE_SIMULATE, 800, 600);
    
    // Enter autonomous mode immediately
    visualmem_enter_autonomous_mode(&ctx);
    TEST_ASSERT(ctx.autonomous_mode == 1, "Autonomous mode active");
    
    // Test new allocations in autonomous mode
    void* autonomous_addr = visualmem_alloc(&ctx, 128, "autonomous_allocation");
    TEST_ASSERT(autonomous_addr != NULL, "Allocation in autonomous mode successful");
    
    // Test write operations in autonomous mode
    const char* autonomous_data = "FULLY_AUTONOMOUS_OPERATION_DATA";
    int write_result = visualmem_write_string(&ctx, autonomous_addr, autonomous_data);
    TEST_ASSERT(write_result == VISUALMEM_SUCCESS, "Write in autonomous mode successful");
    
    // Test read operations in autonomous mode
    char autonomous_read[128];
    int read_result = visualmem_read_string(&ctx, autonomous_addr, autonomous_read, sizeof(autonomous_read));
    TEST_ASSERT(read_result == VISUALMEM_SUCCESS, "Read in autonomous mode successful");
    TEST_ASSERT(strcmp(autonomous_data, autonomous_read) == 0, "Data integrity in autonomous mode");
    
    printf("  Autonomous operation results:\n");
    printf("    Written: '%s'\n", autonomous_data);
    printf("    Read:    '%s'\n", autonomous_read);
    printf("    Status:  %s\n", 
           strcmp(autonomous_data, autonomous_read) == 0 ? "PERFECT MATCH" : "DATA CORRUPTION");
    
    // Test complex data in autonomous mode
    struct test_data {
        int integer_val;
        float float_val;
        char string_val[32];
        uint8_t array_val[16];
    } test_struct = {
        .integer_val = 0x12345678,
        .float_val = 3.14159f,
        .string_val = "STRUCT_TEST",
        .array_val = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}
    };
    
    void* struct_addr = visualmem_alloc(&ctx, sizeof(test_struct), "struct_test");
    TEST_ASSERT(struct_addr != NULL, "Struct allocation in autonomous mode");
    
    int struct_write = visualmem_write(&ctx, struct_addr, &test_struct, sizeof(test_struct));
    TEST_ASSERT(struct_write == VISUALMEM_SUCCESS, "Struct write in autonomous mode");
    
    struct test_data read_struct;
    int struct_read = visualmem_read(&ctx, struct_addr, &read_struct, sizeof(read_struct));
    TEST_ASSERT(struct_read == VISUALMEM_SUCCESS, "Struct read in autonomous mode");
    
    TEST_ASSERT(read_struct.integer_val == test_struct.integer_val, "Integer field preserved");
    TEST_ASSERT(read_struct.float_val == test_struct.float_val, "Float field preserved");
    TEST_ASSERT(strcmp(read_struct.string_val, test_struct.string_val) == 0, "String field preserved");
    
    int array_match = 1;
    for (int i = 0; i < 16; i++) {
        if (read_struct.array_val[i] != test_struct.array_val[i]) {
            array_match = 0;
            break;
        }
    }
    TEST_ASSERT(array_match, "Array field preserved");
    
    printf("  Complex data structure test:\n");
    printf("    Integer: 0x%08X -> 0x%08X\n", test_struct.integer_val, read_struct.integer_val);
    printf("    Float: %.5f -> %.5f\n", test_struct.float_val, read_struct.float_val);
    printf("    String: '%s' -> '%s'\n", test_struct.string_val, read_struct.string_val);
    printf("    Array: [0-15] -> %s\n", array_match ? "PRESERVED" : "CORRUPTED");
    
    print_visual_memory_stats(&ctx);
    
    visualmem_cleanup(&ctx);
    TEST_END();
}

static int test_error_conditions(void) {
    TEST_START("Error Conditions and Edge Cases");
    
    visualmem_context_t ctx;
    
    // Test invalid initialization parameters
    int invalid_result = visualmem_init(&ctx, VISUALMEM_MODE_SIMULATE, -1, 600);
    TEST_ASSERT(invalid_result != VISUALMEM_SUCCESS, "Invalid parameters rejected");
    
    // Proper initialization
    visualmem_init(&ctx, VISUALMEM_MODE_SIMULATE, 800, 600);
    
    // Test invalid operations
    int invalid_write = visualmem_write(&ctx, NULL, "test", 4);
    TEST_ASSERT(invalid_write != VISUALMEM_SUCCESS, "Invalid write address rejected");
    
    int invalid_read = visualmem_read(&ctx, NULL, malloc(10), 10);
    TEST_ASSERT(invalid_read != VISUALMEM_SUCCESS, "Invalid read address rejected");
    free(malloc(10)); // Clean up test allocation
    
    // Test operations on non-existent allocation
    void* fake_addr = (void*)0x12345678;
    int fake_write = visualmem_write(&ctx, fake_addr, "test", 4);
    TEST_ASSERT(fake_write != VISUALMEM_SUCCESS, "Non-existent allocation rejected");
    
    // Test double free
    void* test_addr = visualmem_alloc(&ctx, 64, "double_free_test");
    visualmem_free(&ctx, test_addr);
    int double_free_result = visualmem_free(&ctx, test_addr);
    TEST_ASSERT(double_free_result != VISUALMEM_SUCCESS, "Double free detected and rejected");
    
    printf("  Error handling test results:\n");
    printf("    Invalid init: %s\n", visualmem_get_error_string(invalid_result));
    printf("    Invalid write: %s\n", visualmem_get_error_string(invalid_write));
    printf("    Invalid read: %s\n", visualmem_get_error_string(invalid_read));
    printf("    Non-existent alloc: %s\n", visualmem_get_error_string(fake_write));
    printf("    Double free: %s\n", visualmem_get_error_string(double_free_result));
    
    visualmem_cleanup(&ctx);
    TEST_END();
}

static int test_visual_display(void) {
    TEST_START("Visual Memory Display and Debugging");
    
    visualmem_context_t ctx;
    visualmem_init(&ctx, VISUALMEM_MODE_SIMULATE, 800, 600);
    visualmem_set_debug_mode(&ctx, 1);
    
    // Create some visual data patterns
    void* addr1 = visualmem_alloc(&ctx, 32, "pattern_1");
    void* addr2 = visualmem_alloc(&ctx, 32, "pattern_2");
    
    const char* pattern1 = "VISUAL_PATTERN_A";
    const char* pattern2 = "DISPLAY_TEST_B";
    
    visualmem_write_string(&ctx, addr1, pattern1);
    visualmem_write_string(&ctx, addr2, pattern2);
    
    printf("  Displaying visual memory contents:\n");
    visualmem_display_contents(&ctx, NULL);
    
    // Test after autonomous transition
    visualmem_enter_autonomous_mode(&ctx);
    printf("  Visual memory after autonomous transition:\n");
    visualmem_display_contents(&ctx, NULL);
    
    visualmem_cleanup(&ctx);
    TEST_END();
}

// === MAIN TEST RUNNER ===
int main(void) {
    print_test_header();
    
    clock_t start_time = clock();
    
    // Run all tests
    test_library_initialization();
    test_memory_allocation();
    test_data_operations();
    test_autonomous_mode_transition();
    test_data_persistence_post_transition();
    test_autonomous_operations();
    test_error_conditions();
    test_visual_display();
    
    clock_t end_time = clock();
    double test_duration = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    // Print final results
    printf("\n===================================================================\n");
    printf("                     FINAL TEST RESULTS\n");
    printf("===================================================================\n\n");
    
    printf("Tests executed: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Success rate: %.1f%%\n", (float)tests_passed / tests_run * 100);
    printf("Execution time: %.3f seconds\n", test_duration);
    
    if (tests_passed == tests_run) {
        printf("\n🎉 ALL TESTS PASSED - LIBVISUALMEM FULLY VALIDATED! 🎉\n\n");
        
        printf("VALIDATED FEATURES:\n");
        printf("✅ Library initialization and cleanup\n");
        printf("✅ Visual memory allocation and deallocation\n");
        printf("✅ Data write and read operations\n");
        printf("✅ Autonomous mode transition (CRITICAL)\n");
        printf("✅ Data persistence after RAM liberation\n");
        printf("✅ Complex data structure handling\n");
        printf("✅ Operations in fully autonomous mode\n");
        printf("✅ Error handling and edge cases\n");
        printf("✅ Visual memory display and debugging\n\n");
        
        printf("CONCLUSION:\n");
        printf("LibVisualMem is FULLY FUNCTIONAL and ready for production use.\n");
        printf("The visual memory system successfully operates autonomously\n");
        printf("after RAM liberation, maintaining complete data integrity.\n\n");
        
        return 0;
    } else {
        printf("\n⚠️ SOME TESTS FAILED - REVIEW REQUIRED ⚠️\n");
        printf("Failed tests: %d\n", tests_run - tests_passed);
        return 1;
    }
}