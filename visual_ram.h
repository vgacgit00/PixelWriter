#ifndef VISUAL_RAM_H
#define VISUAL_RAM_H

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Configuration constants
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define BITS_PER_BYTE 8
#define MEMORY_START_X 10
#define MEMORY_START_Y 50
#define BYTE_SPACING 10

// Color definitions for memory encoding
#define COLOR_BIT_0     0x000000FF  // Black for bit 0
#define COLOR_BIT_1     0xFFFFFFFF  // White for bit 1
#define COLOR_MARKER_START 0xFF0000FF  // Red for start marker
#define COLOR_MARKER_END   0x00FF00FF  // Green for end marker
#define COLOR_MARKER_ADDR  0x0000FFFF  // Blue for address marker
#define COLOR_BACKGROUND   0x808080FF  // Gray background

// Structure to hold visual memory context
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint32_t* pixel_buffer;
    int is_initialized;
} VisualRAM;

// Function declarations
int visual_ram_init(VisualRAM* vram);
void visual_ram_cleanup(VisualRAM* vram);
void encode_byte_to_pixels(VisualRAM* vram, int byte_index, uint8_t byte_value);
uint8_t decode_byte_from_pixels(VisualRAM* vram, int byte_index);
void write_string_to_visual_memory(VisualRAM* vram, const char* str, int start_byte);
void read_string_from_visual_memory(VisualRAM* vram, char* buffer, int start_byte, int max_length);
void display_visual_memory(VisualRAM* vram);
void free_ram_keep_display(VisualRAM* vram);
void set_pixel_color(VisualRAM* vram, int x, int y, uint32_t color);
uint32_t get_pixel_color(VisualRAM* vram, int x, int y);

#endif // VISUAL_RAM_H
