#ifndef PIXEL_VISUAL_RAM_H
#define PIXEL_VISUAL_RAM_H

#include <SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Configuration constants for pixel-based visual memory
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define BITS_PER_BYTE 8
#define MEMORY_START_X 10
#define MEMORY_START_Y 50
#define BYTE_SPACING_X 10  // 8 pixels + 2 for markers
#define BYTE_SPACING_Y 2   // Space between rows of bytes

// Pixel color definitions for memory encoding (ARGB format)
#define COLOR_BIT_0     0xFF000000  // Black for bit 0
#define COLOR_BIT_1     0xFFFFFFFF  // White for bit 1
#define COLOR_MARKER_START 0xFFFF0000  // Red for start marker
#define COLOR_MARKER_END   0xFF00FF00  // Green for end marker
#define COLOR_MARKER_ADDR  0xFF0000FF  // Blue for address marker
#define COLOR_BACKGROUND   0xFF808080  // Gray background

// Maximum bytes we can store (based on screen dimensions)
#define MAX_BYTES_PER_ROW ((WINDOW_WIDTH - MEMORY_START_X) / BYTE_SPACING_X)
#define MAX_ROWS ((WINDOW_HEIGHT - MEMORY_START_Y) / BYTE_SPACING_Y)
#define MAX_BYTES_TOTAL (MAX_BYTES_PER_ROW * MAX_ROWS)

// Structure to hold pixel-based visual memory context
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* memory_texture;
    uint32_t* pixel_buffer;  // Will be freed after initialization
    int buffer_width;
    int buffer_height;
    int is_initialized;
    int ram_freed;
} PixelVisualRAM;

// Function declarations
int pixel_visual_ram_init(PixelVisualRAM* pvram);
void pixel_visual_ram_cleanup(PixelVisualRAM* pvram);
void encode_byte_to_pixels(PixelVisualRAM* pvram, int byte_index, uint8_t byte_value);
uint8_t decode_byte_from_pixels(PixelVisualRAM* pvram, int byte_index);
void write_string_to_pixel_memory(PixelVisualRAM* pvram, const char* str, int start_byte);
void read_string_from_pixel_memory(PixelVisualRAM* pvram, char* buffer, int start_byte, int max_length);
void display_pixel_memory(PixelVisualRAM* pvram);
void free_ram_keep_pixel_display(PixelVisualRAM* pvram);
void set_pixel_color(PixelVisualRAM* pvram, int x, int y, uint32_t color);
uint32_t get_pixel_color(PixelVisualRAM* pvram, int x, int y);
void calculate_byte_position(int byte_index, int* x, int* y);
void draw_memory_grid(PixelVisualRAM* pvram);
void draw_info_text(PixelVisualRAM* pvram, const char* text, int x, int y);

#endif // PIXEL_VISUAL_RAM_H