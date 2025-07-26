#ifndef VISUAL_RAM_SIMPLE_H
#define VISUAL_RAM_SIMPLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

// Configuration constants for text-based visual memory
#define MEMORY_WIDTH 80
#define MEMORY_HEIGHT 24
#define MAX_MEMORY_SIZE (MEMORY_WIDTH * MEMORY_HEIGHT)

// Character representations for visual memory
#define CHAR_BIT_0 '.'     // Represents bit 0
#define CHAR_BIT_1 '#'     // Represents bit 1
#define CHAR_MARKER_START 'S'  // Start marker
#define CHAR_MARKER_END 'E'    // End marker
#define CHAR_BACKGROUND ' '    // Background

// Structure to hold text-based visual memory
typedef struct {
    char visual_display[MEMORY_HEIGHT][MEMORY_WIDTH + 1]; // +1 for null terminator
    char* ram_buffer; // This will be freed after initialization
    int is_initialized;
    int ram_freed; // Flag to track if RAM has been freed
} TextVisualRAM;

// Function declarations
int text_visual_ram_init(TextVisualRAM* tvram);
void text_visual_ram_cleanup(TextVisualRAM* tvram);
void encode_byte_to_text(TextVisualRAM* tvram, int byte_index, uint8_t byte_value);
uint8_t decode_byte_from_text(TextVisualRAM* tvram, int byte_index);
void write_string_to_text_memory(TextVisualRAM* tvram, const char* str, int start_byte);
void read_string_from_text_memory(TextVisualRAM* tvram, char* buffer, int start_byte, int max_length);
void display_text_memory(TextVisualRAM* tvram);
void free_ram_keep_text_display(TextVisualRAM* tvram);
void set_text_char(TextVisualRAM* tvram, int x, int y, char ch);
char get_text_char(TextVisualRAM* tvram, int x, int y);

#endif // VISUAL_RAM_SIMPLE_H