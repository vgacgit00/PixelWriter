#include "visual_ram.h"
#include <time.h>

void demo_autonomous_operations(VisualRAM* vram) {
    printf("\n=== Autonomous Visual Memory Operations Demo ===\n");
    
    // First, store some initial data
    const char* messages[] = {
        "HELLO",
        "WORLD", 
        "VISUAL",
        "MEMORY"
    };
    
    printf("Storing multiple strings in visual memory...\n");
    for (int i = 0; i < 4; i++) {
        write_string_to_visual_memory(vram, messages[i], i * 10);
        printf("Stored '%s' at position %d\n", messages[i], i * 10);
    }
    
    display_visual_memory(vram);
    printf("\nPress Enter to continue to autonomous mode...");
    getchar();
    
    // Free RAM - enter autonomous mode
    free_ram_keep_display(vram);
    
    printf("=== Now in Autonomous Mode ===\n");
    printf("Performing read/write operations using only screen pixels...\n\n");
    
    // Read back all the stored strings
    char buffer[100];
    for (int i = 0; i < 4; i++) {
        read_string_from_visual_memory(vram, buffer, i * 10, sizeof(buffer));
        printf("Position %d: '%s'\n", i * 10, buffer);
    }
    
    printf("\nModifying data in autonomous mode...\n");
    
    // Modify some data
    write_string_to_visual_memory(vram, "CODE", 40);
    write_string_to_visual_memory(vram, "TEST", 50);
    
    display_visual_memory(vram);
    
    printf("\nReading modified data:\n");
    read_string_from_visual_memory(vram, buffer, 40, sizeof(buffer));
    printf("Position 40: '%s'\n", buffer);
    read_string_from_visual_memory(vram, buffer, 50, sizeof(buffer));
    printf("Position 50: '%s'\n", buffer);
    
    printf("\n=== Autonomous Visual Memory Demo Complete ===\n");
}

void demo_byte_level_operations(VisualRAM* vram) {
    printf("\n=== Byte-Level Visual Memory Demo ===\n");
    
    // Enter autonomous mode
    free_ram_keep_display(vram);
    
    printf("Writing individual bytes to visual memory...\n");
    
    // Write some test bytes
    uint8_t test_bytes[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x00}; // "Hello"
    
    for (int i = 0; i < 6; i++) {
        encode_byte_to_pixels(vram, i, test_bytes[i]);
        printf("Wrote byte 0x%02X to position %d\n", test_bytes[i], i);
    }
    
    display_visual_memory(vram);
    
    printf("\nReading bytes back from visual memory:\n");
    for (int i = 0; i < 6; i++) {
        uint8_t read_byte = decode_byte_from_pixels(vram, i);
        printf("Position %d: 0x%02X", i, read_byte);
        if (read_byte >= 32 && read_byte <= 126) {
            printf(" ('%c')", read_byte);
        } else if (read_byte == 0) {
            printf(" (NULL)");
        }
        printf("\n");
    }
    
    // Test bit manipulation
    printf("\nTesting bit-level operations...\n");
    uint8_t test_byte = 0xAA; // 10101010 in binary
    encode_byte_to_pixels(vram, 10, test_byte);
    
    display_visual_memory(vram);
    
    uint8_t read_back = decode_byte_from_pixels(vram, 10);
    printf("Original: 0x%02X, Read back: 0x%02X\n", test_byte, read_back);
    
    printf("Binary representation:\n");
    printf("Original:  ");
    for (int i = 7; i >= 0; i--) printf("%d", (test_byte >> i) & 1);
    printf("\nRead back: ");
    for (int i = 7; i >= 0; i--) printf("%d", (read_back >> i) & 1);
    printf("\n");
    
    if (test_byte == read_back) {
        printf("✅ Bit-level integrity maintained!\n");
    } else {
        printf("❌ Bit-level corruption detected!\n");
    }
}

void demo_memory_persistence(VisualRAM* vram) {
    printf("\n=== Memory Persistence Demo ===\n");
    
    // Store data with timestamp
    time_t current_time = time(NULL);
    char timestamp_str[50];
    sprintf(timestamp_str, "T%ld", current_time);
    
    write_string_to_visual_memory(vram, timestamp_str, 0);
    printf("Stored timestamp: %s\n", timestamp_str);
    
    display_visual_memory(vram);
    free_ram_keep_display(vram);
    
    printf("RAM freed. Data exists only in screen pixels.\n");
    printf("Simulating system operations...\n");
    
    // Simulate some delay
    SDL_Delay(2000);
    
    printf("Reading timestamp from visual memory...\n");
    char read_timestamp[50];
    read_string_from_visual_memory(vram, read_timestamp, 0, sizeof(read_timestamp));
    
    printf("Original: %s\n", timestamp_str);
    printf("Read back: %s\n", read_timestamp);
    
    if (strcmp(timestamp_str, read_timestamp) == 0) {
        printf("✅ Data persisted successfully in visual memory!\n");
    } else {
        printf("❌ Data persistence failed!\n");
    }
}

int main() {
    VisualRAM vram = {0};
    
    printf("=== Visual RAM Autonomous Memory System - Full Demo ===\n");
    printf("This demonstration shows a complete autonomous visual memory system.\n\n");
    
    if (visual_ram_init(&vram) != 0) {
        fprintf(stderr, "Failed to initialize visual RAM\n");
        return 1;
    }
    
    int choice;
    
    while (1) {
        printf("\n=== Demo Menu ===\n");
        printf("1. Basic autonomous operations demo\n");
        printf("2. Byte-level operations demo\n");
        printf("3. Memory persistence demo\n");
        printf("4. Interactive mode\n");
        printf("5. Exit\n");
        printf("Choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input\n");
            while (getchar() != '\n');
            continue;
        }
        
        // Clear input buffer
        while (getchar() != '\n');
        
        switch (choice) {
            case 1:
                demo_autonomous_operations(&vram);
                
                // Reinitialize for next demo
                visual_ram_cleanup(&vram);
                if (visual_ram_init(&vram) != 0) {
                    fprintf(stderr, "Failed to reinitialize\n");
                    return 1;
                }
                break;
                
            case 2:
                demo_byte_level_operations(&vram);
                
                visual_ram_cleanup(&vram);
                if (visual_ram_init(&vram) != 0) {
                    fprintf(stderr, "Failed to reinitialize\n");
                    return 1;
                }
                break;
                
            case 3:
                demo_memory_persistence(&vram);
                
                visual_ram_cleanup(&vram);
                if (visual_ram_init(&vram) != 0) {
                    fprintf(stderr, "Failed to reinitialize\n");
                    return 1;
                }
                break;
                
            case 4:
                printf("Entering interactive mode...\n");
                printf("Use reader and writer utilities for interactive operations.\n");
                printf("Compile with: make reader && make writer\n");
                printf("Then run: ./reader or ./writer\n");
                break;
                
            case 5:
                printf("Exiting demo...\n");
                visual_ram_cleanup(&vram);
                return 0;
                
            default:
                printf("Invalid choice\n");
        }
    }
    
    return 0;
}
