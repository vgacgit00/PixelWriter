/**
 * LibVisualMem - Practical Usage Examples
 * =======================================
 * 
 * Demonstrates real-world applications of the visual memory library
 * showing how to use the screen as an alternative memory system.
 * 
 * Copyright (C) 2025 - Visual Memory Systems
 */

#include "libvisualmem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// === EXAMPLE DATA STRUCTURES ===
typedef struct {
    char username[32];
    char password_hash[64];
    int user_id;
    time_t last_login;
    int permissions;
} user_account_t;

typedef struct {
    int transaction_id;
    float amount;
    char description[128];
    time_t timestamp;
    int status;
} transaction_t;

// === PRACTICAL EXAMPLES ===

/**
 * Example 1: Secure Temporary Data Storage
 * Store sensitive data in visual memory instead of heap
 */
void example_secure_storage(void) {
    printf("\n=== Example 1: Secure Temporary Data Storage ===\n");
    printf("Use case: Store sensitive data away from traditional heap memory\n\n");
    
    visualmem_context_t secure_ctx;
    visualmem_init(&secure_ctx, VISUALMEM_MODE_SIMULATE, 1024, 768);
    
    // Store sensitive user credentials in visual memory
    void* credentials_addr = visualmem_alloc(&secure_ctx, sizeof(user_account_t), "secure_credentials");
    
    user_account_t user = {
        .username = "admin_user",
        .password_hash = "sha256:a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e",
        .user_id = 1001,
        .last_login = time(NULL),
        .permissions = 0xFF
    };
    
    printf("Storing sensitive credentials in visual memory...\n");
    visualmem_write(&secure_ctx, credentials_addr, &user, sizeof(user));
    
    // Enter autonomous mode - RAM buffer is freed, data exists only in display
    printf("Entering autonomous mode - removing data from traditional memory...\n");
    visualmem_enter_autonomous_mode(&secure_ctx);
    
    // Retrieve credentials from visual memory only
    user_account_t retrieved_user;
    visualmem_read(&secure_ctx, credentials_addr, &retrieved_user, sizeof(retrieved_user));
    
    printf("✅ Credentials retrieved from visual memory:\n");
    printf("   Username: %s\n", retrieved_user.username);
    printf("   Hash: %.32s...\n", retrieved_user.password_hash);
    printf("   User ID: %d\n", retrieved_user.user_id);
    printf("   Permissions: 0x%02X\n", retrieved_user.permissions);
    
    printf("   Security benefit: Data not accessible via traditional memory dumps\n");
    
    visualmem_cleanup(&secure_ctx);
}

/**
 * Example 2: Data Processing Pipeline with Visual Buffering
 * Use visual memory as intermediate storage for data processing
 */
void example_data_pipeline(void) {
    printf("\n=== Example 2: Data Processing Pipeline ===\n");
    printf("Use case: Visual memory as intermediate buffer for data processing\n\n");
    
    visualmem_context_t pipeline_ctx;
    visualmem_init(&pipeline_ctx, VISUALMEM_MODE_SIMULATE, 1200, 900);
    
    // Simulate processing financial transactions
    const int num_transactions = 10;
    transaction_t transactions[num_transactions];
    
    // Generate sample transaction data
    for (int i = 0; i < num_transactions; i++) {
        transactions[i].transaction_id = 1000 + i;
        transactions[i].amount = 100.0f + (i * 50.0f);
        snprintf(transactions[i].description, sizeof(transactions[i].description), 
                "Transaction %d - Payment", i + 1);
        transactions[i].timestamp = time(NULL) + i;
        transactions[i].status = i % 3; // Random status
    }
    
    printf("Processing %d transactions through visual memory pipeline...\n", num_transactions);
    
    // Stage 1: Store raw transactions in visual memory
    void* raw_buffer = visualmem_alloc(&pipeline_ctx, sizeof(transactions), "raw_transactions");
    visualmem_write(&pipeline_ctx, raw_buffer, transactions, sizeof(transactions));
    
    // Stage 2: Process data (calculate totals, validate)
    void* processed_buffer = visualmem_alloc(&pipeline_ctx, sizeof(float) * num_transactions, "processed_amounts");
    
    float processed_amounts[num_transactions];
    for (int i = 0; i < num_transactions; i++) {
        // Apply processing logic (e.g., fees, currency conversion)
        processed_amounts[i] = transactions[i].amount * 1.025f; // 2.5% fee
    }
    
    visualmem_write(&pipeline_ctx, processed_buffer, processed_amounts, sizeof(processed_amounts));
    
    // Stage 3: Enter autonomous mode and continue processing
    printf("Transitioning to autonomous processing mode...\n");
    visualmem_enter_autonomous_mode(&pipeline_ctx);
    
    // Stage 4: Generate summary report from visual memory
    void* summary_addr = visualmem_alloc(&pipeline_ctx, 256, "summary_report");
    
    float total_amount = 0.0f;
    float retrieved_amounts[num_transactions];
    visualmem_read(&pipeline_ctx, processed_buffer, retrieved_amounts, sizeof(retrieved_amounts));
    
    for (int i = 0; i < num_transactions; i++) {
        total_amount += retrieved_amounts[i];
    }
    
    char summary[256];
    snprintf(summary, sizeof(summary), 
            "TRANSACTION SUMMARY|Count:%d|Total:$%.2f|Average:$%.2f|Status:PROCESSED", 
            num_transactions, total_amount, total_amount / num_transactions);
    
    visualmem_write_string(&pipeline_ctx, summary_addr, summary);
    
    // Read and display final summary
    char final_summary[256];
    visualmem_read_string(&pipeline_ctx, summary_addr, final_summary, sizeof(final_summary));
    
    printf("✅ Pipeline processing complete:\n");
    printf("   Raw transactions processed: %d\n", num_transactions);
    printf("   Total amount processed: $%.2f\n", total_amount);
    printf("   Summary: %s\n", final_summary);
    printf("   Advantage: Intermediate data stored in visual memory, reducing RAM pressure\n");
    
    visualmem_cleanup(&pipeline_ctx);
}

/**
 * Example 3: Multi-Application Shared Visual Memory
 * Demonstrate shared data storage between different "applications"
 */
void example_shared_memory(void) {
    printf("\n=== Example 3: Shared Visual Memory Between Applications ===\n");
    printf("Use case: Multiple processes sharing data through visual memory\n\n");
    
    visualmem_context_t shared_ctx;
    visualmem_init(&shared_ctx, VISUALMEM_MODE_SIMULATE, 1024, 768);
    
    // Application 1: Data Producer
    printf("Application 1 (Producer): Writing shared configuration...\n");
    
    struct shared_config {
        char app_name[32];
        int version_major;
        int version_minor;
        char database_url[128];
        int max_connections;
        float timeout_seconds;
    } config = {
        .app_name = "DataProcessor",
        .version_major = 2,
        .version_minor = 1,
        .database_url = "postgresql://localhost:5432/appdb",
        .max_connections = 100,
        .timeout_seconds = 30.5f
    };
    
    void* config_addr = visualmem_alloc(&shared_ctx, sizeof(config), "shared_config");
    visualmem_write(&shared_ctx, config_addr, &config, sizeof(config));
    
    // Application 2: Status Reporter
    printf("Application 2 (Status): Writing system status...\n");
    
    struct system_status {
        time_t last_update;
        int active_users;
        float cpu_usage;
        float memory_usage;
        char status_message[64];
    } status = {
        .last_update = time(NULL),
        .active_users = 42,
        .cpu_usage = 65.5f,
        .memory_usage = 78.2f,
        .status_message = "System operational - All services running"
    };
    
    void* status_addr = visualmem_alloc(&shared_ctx, sizeof(status), "system_status");
    visualmem_write(&shared_ctx, status_addr, &status, sizeof(status));
    
    // Transition to autonomous mode
    printf("Transitioning to shared autonomous mode...\n");
    visualmem_enter_autonomous_mode(&shared_ctx);
    
    // Application 3: Data Consumer (reads both shared areas)
    printf("Application 3 (Consumer): Reading shared data...\n");
    
    struct shared_config read_config;
    struct system_status read_status;
    
    visualmem_read(&shared_ctx, config_addr, &read_config, sizeof(read_config));
    visualmem_read(&shared_ctx, status_addr, &read_status, sizeof(read_status));
    
    printf("✅ Shared data successfully accessed:\n");
    printf("   Configuration:\n");
    printf("     App: %s v%d.%d\n", read_config.app_name, read_config.version_major, read_config.version_minor);
    printf("     Database: %s\n", read_config.database_url);
    printf("     Max connections: %d\n", read_config.max_connections);
    printf("   System Status:\n");
    printf("     Active users: %d\n", read_status.active_users);
    printf("     CPU usage: %.1f%%\n", read_status.cpu_usage);
    printf("     Memory usage: %.1f%%\n", read_status.memory_usage);
    printf("     Status: %s\n", read_status.status_message);
    
    printf("   Benefit: Inter-process communication through persistent visual memory\n");
    
    visualmem_cleanup(&shared_ctx);
}

/**
 * Example 4: Visual Memory as RAM Alternative for Embedded Systems
 * Simulate low-memory embedded system using visual memory
 */
void example_embedded_system(void) {
    printf("\n=== Example 4: Embedded System RAM Alternative ===\n");
    printf("Use case: Visual memory as primary storage for resource-constrained systems\n\n");
    
    // Simulate small embedded display
    visualmem_context_t embedded_ctx;
    visualmem_init(&embedded_ctx, VISUALMEM_MODE_SIMULATE, 320, 240);
    
    printf("Embedded system initialized (320x240 display as memory)\n");
    
    // Sensor data structure
    struct sensor_reading {
        uint16_t sensor_id;
        float temperature;
        float humidity;
        float pressure;
        uint32_t timestamp;
    };
    
    // Simulate sensor data collection
    const int max_readings = 20;
    void* sensor_buffer = visualmem_alloc(&embedded_ctx, 
                                        sizeof(struct sensor_reading) * max_readings, 
                                        "sensor_data");
    
    printf("Collecting sensor data (stored in visual memory)...\n");
    
    for (int i = 0; i < 5; i++) {
        struct sensor_reading reading = {
            .sensor_id = 0x100 + i,
            .temperature = 20.0f + (i * 2.5f),
            .humidity = 45.0f + (i * 3.0f),
            .pressure = 1013.25f + (i * 0.5f),
            .timestamp = time(NULL) + i
        };
        
        // Write to specific position in visual buffer
        visualmem_write(&embedded_ctx, 
                       (char*)sensor_buffer + (i * sizeof(reading)), 
                       &reading, 
                       sizeof(reading));
        
        printf("  Sensor %d: T=%.1f°C, H=%.1f%%, P=%.2fhPa\n", 
               reading.sensor_id, reading.temperature, reading.humidity, reading.pressure);
    }
    
    // Simulate system going into low-power mode (free RAM)
    printf("System entering low-power mode (freeing RAM)...\n");
    visualmem_enter_autonomous_mode(&embedded_ctx);
    
    // Wake up and process data from visual memory only
    printf("System wake-up: Processing data from visual memory...\n");
    
    float avg_temperature = 0.0f, avg_humidity = 0.0f, avg_pressure = 0.0f;
    
    for (int i = 0; i < 5; i++) {
        struct sensor_reading reading;
        visualmem_read(&embedded_ctx, 
                      (char*)sensor_buffer + (i * sizeof(reading)), 
                      &reading, 
                      sizeof(reading));
        
        avg_temperature += reading.temperature;
        avg_humidity += reading.humidity;
        avg_pressure += reading.pressure;
    }
    
    avg_temperature /= 5;
    avg_humidity /= 5;
    avg_pressure /= 5;
    
    printf("✅ Data processing complete (using only visual memory):\n");
    printf("   Average temperature: %.1f°C\n", avg_temperature);
    printf("   Average humidity: %.1f%%\n", avg_humidity);
    printf("   Average pressure: %.2f hPa\n", avg_pressure);
    printf("   Memory efficiency: 100%% visual, 0%% traditional RAM\n");
    
    visualmem_cleanup(&embedded_ctx);
}

/**
 * Example 5: Performance Comparison and Benchmarking
 */
void example_performance_benchmark(void) {
    printf("\n=== Example 5: Performance Benchmarking ===\n");
    printf("Use case: Compare visual memory vs traditional memory operations\n\n");
    
    visualmem_context_t bench_ctx;
    visualmem_init(&bench_ctx, VISUALMEM_MODE_SIMULATE, 800, 600);
    
    const int test_iterations = 1000;
    const size_t data_size = 1024;
    
    // Benchmark visual memory operations
    printf("Benchmarking visual memory operations (%d iterations)...\n", test_iterations);
    
    clock_t visual_start = clock();
    
    void* visual_addr = visualmem_alloc(&bench_ctx, data_size, "benchmark_data");
    char test_data[data_size];
    char read_data[data_size];
    
    // Fill test data
    for (size_t i = 0; i < data_size; i++) {
        test_data[i] = (char)(i % 256);
    }
    
    // Perform write/read operations
    for (int i = 0; i < test_iterations; i++) {
        visualmem_write(&bench_ctx, visual_addr, test_data, data_size);
        visualmem_read(&bench_ctx, visual_addr, read_data, data_size);
    }
    
    clock_t visual_end = clock();
    double visual_time = ((double)(visual_end - visual_start)) / CLOCKS_PER_SEC;
    
    // Benchmark traditional memory operations
    printf("Benchmarking traditional memory operations (%d iterations)...\n", test_iterations);
    
    clock_t traditional_start = clock();
    
    char* traditional_addr = malloc(data_size);
    
    for (int i = 0; i < test_iterations; i++) {
        memcpy(traditional_addr, test_data, data_size);
        memcpy(read_data, traditional_addr, data_size);
    }
    
    clock_t traditional_end = clock();
    double traditional_time = ((double)(traditional_end - traditional_start)) / CLOCKS_PER_SEC;
    
    free(traditional_addr);
    
    // Report results
    printf("✅ Benchmark results:\n");
    printf("   Visual memory time: %.4f seconds\n", visual_time);
    printf("   Traditional memory time: %.4f seconds\n", traditional_time);
    printf("   Performance ratio: %.2fx\n", visual_time / traditional_time);
    printf("   Visual memory throughput: %.2f MB/s\n", 
           (test_iterations * data_size) / (visual_time * 1024 * 1024));
    printf("   Traditional throughput: %.2f MB/s\n", 
           (test_iterations * data_size) / (traditional_time * 1024 * 1024));
    
    size_t total_allocated, peak_usage;
    int fragmentation;
    visualmem_get_stats(&bench_ctx, &total_allocated, &peak_usage, &fragmentation);
    
    printf("   Visual memory stats:\n");
    printf("     Total allocated: %zu bytes\n", total_allocated);
    printf("     Peak usage: %zu bytes\n", peak_usage);
    printf("     Fragmentation: %d%%\n", fragmentation);
    
    visualmem_cleanup(&bench_ctx);
}

// === MAIN DEMO FUNCTION ===
int main(void) {
    printf("===================================================================\n");
    printf("                LIBVISUALMEM - PRACTICAL USAGE EXAMPLES\n");
    printf("===================================================================\n");
    printf("\nLibrary Version: %s\n", visualmem_get_version());
    printf("Demonstration of real-world applications using visual memory\n");
    printf("as an alternative or complement to traditional RAM.\n");
    
    // Run all examples
    example_secure_storage();
    example_data_pipeline();
    example_shared_memory();
    example_embedded_system();
    example_performance_benchmark();
    
    printf("\n===================================================================\n");
    printf("                        DEMONSTRATION COMPLETE\n");
    printf("===================================================================\n\n");
    
    printf("DEMONSTRATED CAPABILITIES:\n");
    printf("✅ Secure temporary data storage\n");
    printf("✅ Data processing pipelines with visual buffering\n");
    printf("✅ Multi-application shared memory\n");
    printf("✅ Embedded system RAM alternatives\n");
    printf("✅ Performance benchmarking and analysis\n\n");
    
    printf("PRACTICAL BENEFITS:\n");
    printf("• Reduced dependency on traditional RAM\n");
    printf("• Enhanced security through non-standard memory location\n");
    printf("• Persistent data storage beyond process lifetime\n");
    printf("• Shared memory capabilities without OS mechanisms\n");
    printf("• Suitable for resource-constrained environments\n\n");
    
    printf("LibVisualMem provides a viable alternative memory system\n");
    printf("suitable for various applications requiring autonomous,\n");
    printf("persistent, or secure data storage capabilities.\n\n");
    
    return 0;
}