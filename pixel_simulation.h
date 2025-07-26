#ifndef PIXEL_SIMULATION_H
#define PIXEL_SIMULATION_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Configuration constants for simulated pixel-based visual memory
#define SIM_WINDOW_WIDTH 800
#define SIM_WINDOW_HEIGHT 600
#define BITS_PER_BYTE 8
#define MEMORY_START_X 10
#define MEMORY_START_Y 50
#define BYTE_SPACING_X 10  // 8 pixels + 2 for markers
#define BYTE_SPACING_Y 2   // Space between rows of bytes

// Simulated pixel color definitions (ARGB format)
#define SIM_COLOR_BIT_0     0xFF000000  // Black for bit 0
#define SIM_COLOR_BIT_1     0xFFFFFFFF  // White for bit 1
#define SIM_COLOR_MARKER_START 0xFFFF0000  // Red for start marker
#define SIM_COLOR_MARKER_END   0xFF00FF00  // Green for end marker
#define SIM_COLOR_MARKER_ADDR  0xFF0000FF  // Blue for address marker
#define SIM_COLOR_BACKGROUND   0xFF808080  // Gray background

// Maximum bytes we can store (based on simulated screen dimensions)
#define MAX_BYTES_PER_ROW ((SIM_WINDOW_WIDTH - MEMORY_START_X) / BYTE_SPACING_X)
#define MAX_ROWS ((SIM_WINDOW_HEIGHT - MEMORY_START_Y) / BYTE_SPACING_Y)
#define MAX_BYTES_TOTAL (MAX_BYTES_PER_ROW * MAX_ROWS)

// Structure to hold simulated pixel-based visual memory context
typedef struct {
    uint32_t* pixel_framebuffer;  // Simulated screen framebuffer
    uint32_t* ram_buffer;         // Will be freed after initialization
    int buffer_width;
    int buffer_height;
    int is_initialized;
    int ram_freed;
    int display_update_count;     // Track display updates
} SimulatedPixelRAM;

// Function declarations
int sim_pixel_ram_init(SimulatedPixelRAM* spram);
void sim_pixel_ram_cleanup(SimulatedPixelRAM* spram);
void sim_encode_byte_to_pixels(SimulatedPixelRAM* spram, int byte_index, uint8_t byte_value);
uint8_t sim_decode_byte_from_pixels(SimulatedPixelRAM* spram, int byte_index);
void sim_write_string_to_pixel_memory(SimulatedPixelRAM* spram, const char* str, int start_byte);
void sim_read_string_from_pixel_memory(SimulatedPixelRAM* spram, char* buffer, int start_byte, int max_length);
void sim_display_pixel_memory(SimulatedPixelRAM* spram);
void sim_free_ram_keep_pixel_display(SimulatedPixelRAM* spram);
void sim_set_pixel_color(SimulatedPixelRAM* spram, int x, int y, uint32_t color);
uint32_t sim_get_pixel_color(SimulatedPixelRAM* spram, int x, int y);
void sim_calculate_byte_position(int byte_index, int* x, int* y);
void sim_render_visual_representation(SimulatedPixelRAM* spram);
const char* sim_color_to_char(uint32_t color);
void sim_export_framebuffer_state(SimulatedPixelRAM* spram, const char* description);

#endif // PIXEL_SIMULATION_H