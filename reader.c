#include "visual_ram.h"

// Standalone reader utility for reading from visual memory
void print_memory_dump(VisualRAM* vram, int start_byte, int num_bytes) {
    printf("=== Visual Memory Dump ===\n");
    printf("Starting at byte %d, reading %d bytes:\n\n", start_byte, num_bytes);
    
    for (int i = 0; i < num_bytes; i++) {
        uint8_t byte_value = decode_byte_from_pixels(vram, start_byte + i);
        
        printf("Byte %3d: 0x%02X (%3d) ", start_byte + i, byte_value, byte_value);
        
        // Show binary representation
        printf("[");
        for (int bit = 7; bit >= 0; bit--) {
            printf("%d", (byte_value >> bit) & 1);
        }
        printf("] ");
        
        // Show ASCII if printable
        if (byte_value >= 32 && byte_value <= 126) {
            printf("'%c'", byte_value);
        } else if (byte_value == 0) {
            printf("'\\0'");
        } else {
            printf("'?'");
        }
        
        printf("\n");
        
        // Stop at null terminator for strings
        if (byte_value == 0) {
            printf("(Null terminator found - string end)\n");
            break;
        }
    }
    printf("\n");
}

void scan_visual_memory(VisualRAM* vram) {
    printf("=== Scanning Visual Memory for Data ===\n");
    
    // Scan for potential strings in visual memory
    char buffer[256];
    int max_scan = 50; // Scan first 50 bytes
    
    for (int start_pos = 0; start_pos < max_scan; start_pos++) {
        uint8_t first_byte = decode_byte_from_pixels(vram, start_pos);
        
        // Look for printable ASCII characters that might start a string
        if (first_byte >= 32 && first_byte <= 126) {
            read_string_from_visual_memory(vram, buffer, start_pos, sizeof(buffer));
            
            // Check if we found a meaningful string (at least 2 chars)
            if (strlen(buffer) >= 2) {
                printf("Found string at byte %d: '%s'\n", start_pos, buffer);
            }
        }
    }
    printf("\n");
}

int reader_main() {
    VisualRAM vram = {0};
    
    printf("=== Visual RAM Reader Utility ===\n");
    printf("This utility reads data from visual memory (screen pixels)\n\n");
    
    // Initialize (but we assume visual memory already exists)
    if (visual_ram_init(&vram) != 0) {
        fprintf(stderr, "Failed to initialize visual RAM reader\n");
        return 1;
    }
    
    // Immediately free RAM to simulate autonomous reading
    if (vram.pixel_buffer) {
        free(vram.pixel_buffer);
        vram.pixel_buffer = NULL;
        printf("Operating in autonomous mode - reading from screen only\n\n");
    }
    
    int choice;
    char buffer[256];
    
    while (1) {
        printf("=== Visual Memory Reader Menu ===\n");
        printf("1. Read string from specific position\n");
        printf("2. Memory dump (hex view)\n");
        printf("3. Scan for strings\n");
        printf("4. Read raw byte\n");
        printf("5. Exit\n");
        printf("Choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        
        switch (choice) {
            case 1: {
                int start_pos;
                printf("Enter starting byte position: ");
                scanf("%d", &start_pos);
                
                read_string_from_visual_memory(&vram, buffer, start_pos, sizeof(buffer));
                printf("String at position %d: '%s'\n\n", start_pos, buffer);
                break;
            }
            
            case 2: {
                int start_pos, num_bytes;
                printf("Enter starting byte position: ");
                scanf("%d", &start_pos);
                printf("Enter number of bytes to dump: ");
                scanf("%d", &num_bytes);
                
                print_memory_dump(&vram, start_pos, num_bytes);
                break;
            }
            
            case 3:
                scan_visual_memory(&vram);
                break;
                
            case 4: {
                int byte_pos;
                printf("Enter byte position: ");
                scanf("%d", &byte_pos);
                
                uint8_t byte_value = decode_byte_from_pixels(&vram, byte_pos);
                printf("Byte at position %d: 0x%02X (%d)\n", byte_pos, byte_value, byte_value);
                
                if (byte_value >= 32 && byte_value <= 126) {
                    printf("ASCII: '%c'\n", byte_value);
                }
                printf("\n");
                break;
            }
            
            case 5:
                printf("Exiting reader...\n");
                visual_ram_cleanup(&vram);
                return 0;
                
            default:
                printf("Invalid choice\n");
        }
    }
    
    return 0;
}

// Main function for standalone reader
#ifdef READER_STANDALONE
int main() {
    return reader_main();
}
#endif
