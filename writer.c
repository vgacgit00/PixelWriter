#include "visual_ram.h"

// Standalone writer utility for modifying visual memory
void write_byte_to_visual_memory(VisualRAM* vram, int byte_pos, uint8_t value) {
    printf("Writing byte 0x%02X (%d) to position %d\n", value, value, byte_pos);
    
    encode_byte_to_pixels(vram, byte_pos, value);
    
    // Update display to show the change
    display_visual_memory(vram);
    
    printf("Visual memory updated at position %d\n", byte_pos);
}

void modify_string_in_visual_memory(VisualRAM* vram, int start_pos, const char* new_str) {
    printf("Modifying string at position %d to: '%s'\n", start_pos, new_str);
    
    write_string_to_visual_memory(vram, new_str, start_pos);
    display_visual_memory(vram);
    
    printf("String modification complete\n");
}

void clear_visual_memory_range(VisualRAM* vram, int start_pos, int num_bytes) {
    printf("Clearing %d bytes starting at position %d\n", num_bytes, start_pos);
    
    for (int i = 0; i < num_bytes; i++) {
        encode_byte_to_pixels(vram, start_pos + i, 0);
    }
    
    display_visual_memory(vram);
    printf("Memory range cleared\n");
}

int writer_main() {
    VisualRAM vram = {0};
    
    printf("=== Visual RAM Writer Utility ===\n");
    printf("This utility modifies data in visual memory (screen pixels)\n\n");
    
    // Initialize visual RAM
    if (visual_ram_init(&vram) != 0) {
        fprintf(stderr, "Failed to initialize visual RAM writer\n");
        return 1;
    }
    
    // Immediately free RAM to simulate autonomous writing
    if (vram.pixel_buffer) {
        free(vram.pixel_buffer);
        vram.pixel_buffer = NULL;
        printf("Operating in autonomous mode - writing to screen only\n\n");
    }
    
    int choice;
    
    while (1) {
        printf("=== Visual Memory Writer Menu ===\n");
        printf("1. Write string to specific position\n");
        printf("2. Write single byte\n");
        printf("3. Clear memory range\n");
        printf("4. Fill memory with pattern\n");
        printf("5. Display current visual memory\n");
        printf("6. Exit\n");
        printf("Choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        
        switch (choice) {
            case 1: {
                int start_pos;
                char str[256];
                
                printf("Enter starting position: ");
                scanf("%d", &start_pos);
                printf("Enter string to write: ");
                scanf(" %255s", str); // Space before % to skip whitespace
                
                modify_string_in_visual_memory(&vram, start_pos, str);
                printf("\n");
                break;
            }
            
            case 2: {
                int byte_pos, value;
                
                printf("Enter byte position: ");
                scanf("%d", &byte_pos);
                printf("Enter byte value (0-255): ");
                scanf("%d", &value);
                
                if (value >= 0 && value <= 255) {
                    write_byte_to_visual_memory(&vram, byte_pos, (uint8_t)value);
                } else {
                    printf("Invalid byte value (must be 0-255)\n");
                }
                printf("\n");
                break;
            }
            
            case 3: {
                int start_pos, num_bytes;
                
                printf("Enter starting position: ");
                scanf("%d", &start_pos);
                printf("Enter number of bytes to clear: ");
                scanf("%d", &num_bytes);
                
                clear_visual_memory_range(&vram, start_pos, num_bytes);
                printf("\n");
                break;
            }
            
            case 4: {
                int start_pos, num_bytes, pattern;
                
                printf("Enter starting position: ");
                scanf("%d", &start_pos);
                printf("Enter number of bytes: ");
                scanf("%d", &num_bytes);
                printf("Enter pattern byte (0-255): ");
                scanf("%d", &pattern);
                
                if (pattern >= 0 && pattern <= 255) {
                    printf("Filling %d bytes with pattern 0x%02X starting at position %d\n", 
                           num_bytes, pattern, start_pos);
                    
                    for (int i = 0; i < num_bytes; i++) {
                        encode_byte_to_pixels(&vram, start_pos + i, (uint8_t)pattern);
                    }
                    
                    display_visual_memory(&vram);
                    printf("Pattern fill complete\n");
                } else {
                    printf("Invalid pattern value (must be 0-255)\n");
                }
                printf("\n");
                break;
            }
            
            case 5:
                printf("Displaying current visual memory state:\n");
                display_visual_memory(&vram);
                printf("Display updated\n\n");
                break;
                
            case 6:
                printf("Exiting writer...\n");
                visual_ram_cleanup(&vram);
                return 0;
                
            default:
                printf("Invalid choice\n");
        }
    }
    
    return 0;
}

// Main function for standalone writer
#ifdef WRITER_STANDALONE
int main() {
    return writer_main();
}
#endif
