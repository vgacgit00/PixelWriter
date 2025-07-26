/**
 * LibVisualMem - Autonomous Visual Memory Library Implementation
 * =============================================================
 * 
 * Core implementation of the visual memory system that transforms
 * display screens into persistent memory storage.
 * 
 * Copyright (C) 2025 - Visual Memory Systems
 * License: MIT
 */

#include "libvisualmem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

// === INTERNAL CONSTANTS ===
#define VISUALMEM_MAGIC_HEADER 0x56495355  // "VISU" in hex
#define VISUALMEM_BYTE_SPACING_X 10  // 8 bits + 2 markers
#define VISUALMEM_BYTE_SPACING_Y 2   // Vertical spacing between bytes
#define VISUALMEM_MEMORY_START_X 20  // Skip header area
#define VISUALMEM_MEMORY_START_Y 20

// === INTERNAL STRUCTURES ===
typedef struct {
    uint32_t magic;
    int version;
    visualmem_mode_t mode;
    int width;
    int height;
    int allocation_count;
    uint64_t timestamp;
} visualmem_header_t;

// === COORDINATE CONVERSION ===
static inline void* coord_to_addr(int x, int y) {
    return (void*)((uintptr_t)((y << 16) | x));
}

static inline void addr_to_coord(void* addr, int* x, int* y) {
    uintptr_t val = (uintptr_t)addr;
    *x = val & 0xFFFF;
    *y = (val >> 16) & 0xFFFF;
}

static void calculate_byte_position(int byte_index, int* x, int* y, int width) {
    int bytes_per_row = (width - VISUALMEM_MEMORY_START_X) / VISUALMEM_BYTE_SPACING_X;
    int row = byte_index / bytes_per_row;
    int col = byte_index % bytes_per_row;
    
    *x = VISUALMEM_MEMORY_START_X + (col * VISUALMEM_BYTE_SPACING_X);
    *y = VISUALMEM_MEMORY_START_Y + (row * VISUALMEM_BYTE_SPACING_Y);
}

// === FRAMEBUFFER OPERATIONS ===
static void set_pixel_color(visualmem_context_t* ctx, int x, int y, uint32_t color) {
    if (x >= 0 && x < ctx->width && y >= 0 && y < ctx->height) {
        int index = y * ctx->width + x;
        uint32_t* framebuffer = (uint32_t*)ctx->framebuffer;
        
        if (ctx->ram_buffer && !ctx->ram_freed) {
            // During initialization - use RAM buffer
            uint32_t* ram_buf = (uint32_t*)ctx->ram_buffer;
            ram_buf[index] = color;
        }
        
        // Always update framebuffer (this is the persistent visual memory)
        framebuffer[index] = color;
    }
}

static uint32_t get_pixel_color(visualmem_context_t* ctx, int x, int y) {
    if (x >= 0 && x < ctx->width && y >= 0 && y < ctx->height) {
        int index = y * ctx->width + x;
        
        if (ctx->ram_buffer && !ctx->ram_freed) {
            // During initialization - read from RAM buffer
            uint32_t* ram_buf = (uint32_t*)ctx->ram_buffer;
            return ram_buf[index];
        } else {
            // Autonomous mode - read directly from framebuffer (visual memory)
            uint32_t* framebuffer = (uint32_t*)ctx->framebuffer;
            return framebuffer[index];
        }
    }
    return VISUALMEM_COLOR_FREE;
}

// === BYTE ENCODING/DECODING ===
static void encode_byte_to_pixels(visualmem_context_t* ctx, int byte_index, uint8_t byte_value) {
    int base_x, base_y;
    calculate_byte_position(byte_index, &base_x, &base_y, ctx->width);
    
    if (base_x >= ctx->width - 10 || base_y >= ctx->height - 2) {
        return; // Out of bounds
    }
    
    // Place start marker
    set_pixel_color(ctx, base_x, base_y, VISUALMEM_COLOR_START);
    
    // Encode each bit as a pixel
    for (int bit = 0; bit < VISUALMEM_BITS_PER_BYTE; bit++) {
        uint8_t bit_value = (byte_value >> (7 - bit)) & 1;
        uint32_t pixel_color = bit_value ? VISUALMEM_COLOR_BIT_1 : VISUALMEM_COLOR_BIT_0;
        set_pixel_color(ctx, base_x + 1 + bit, base_y, pixel_color);
    }
    
    // Place end marker
    set_pixel_color(ctx, base_x + 9, base_y, VISUALMEM_COLOR_END);
}

static uint8_t decode_byte_from_pixels(visualmem_context_t* ctx, int byte_index) {
    int base_x, base_y;
    calculate_byte_position(byte_index, &base_x, &base_y, ctx->width);
    
    if (base_x >= ctx->width - 10 || base_y >= ctx->height - 2) {
        return 0; // Out of bounds
    }
    
    uint8_t byte_value = 0;
    
    // Read each bit from pixels
    for (int bit = 0; bit < VISUALMEM_BITS_PER_BYTE; bit++) {
        uint32_t pixel_color = get_pixel_color(ctx, base_x + 1 + bit, base_y);
        
        // Check if pixel represents bit 1
        if ((pixel_color & 0x00FFFFFF) == (VISUALMEM_COLOR_BIT_1 & 0x00FFFFFF)) {
            byte_value |= (1 << (7 - bit));
        }
    }
    
    return byte_value;
}

// === CORE LIBRARY FUNCTIONS ===

int visualmem_init(visualmem_context_t* ctx, visualmem_mode_t mode, int width, int height) {
    if (!ctx) return VISUALMEM_ERROR_INVALID_ADDRESS;
    if (width <= 0 || height <= 0 || width > VISUALMEM_MAX_WIDTH || height > VISUALMEM_MAX_HEIGHT) {
        return VISUALMEM_ERROR_INVALID_SIZE;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(visualmem_context_t));
    ctx->width = width;
    ctx->height = height;
    ctx->mode = mode;
    
    // Allocate framebuffer (this represents the actual screen/display)
    size_t framebuffer_size = width * height * sizeof(uint32_t);
    ctx->framebuffer = malloc(framebuffer_size);
    if (!ctx->framebuffer) {
        return VISUALMEM_ERROR_ALLOCATION_FAILED;
    }
    
    // Allocate temporary RAM buffer (will be freed in autonomous mode)
    ctx->ram_buffer = malloc(framebuffer_size);
    if (!ctx->ram_buffer) {
        free(ctx->framebuffer);
        return VISUALMEM_ERROR_ALLOCATION_FAILED;
    }
    
    // Initialize both buffers
    uint32_t* framebuffer = (uint32_t*)ctx->framebuffer;
    uint32_t* ram_buffer = (uint32_t*)ctx->ram_buffer;
    
    for (int i = 0; i < width * height; i++) {
        framebuffer[i] = VISUALMEM_COLOR_FREE;
        ram_buffer[i] = VISUALMEM_COLOR_FREE;
    }
    
    // Initialize allocation tracking
    for (int i = 0; i < VISUALMEM_MAX_ALLOCATIONS; i++) {
        ctx->allocations[i].is_active = 0;
    }
    
    // Set status flags
    ctx->is_initialized = 1;
    ctx->ram_freed = 0;
    ctx->autonomous_mode = 0;
    ctx->enable_error_correction = 1;
    ctx->enable_compression = 0;
    ctx->debug_mode = 0;
    
    // Write header to visual memory
    visualmem_header_t header = {
        .magic = VISUALMEM_MAGIC_HEADER,
        .version = (LIBVISUALMEM_VERSION_MAJOR << 16) | (LIBVISUALMEM_VERSION_MINOR << 8) | LIBVISUALMEM_VERSION_PATCH,
        .mode = mode,
        .width = width,
        .height = height,
        .allocation_count = 0,
        .timestamp = time(NULL)
    };
    
    // Encode header into first pixels
    uint8_t* header_bytes = (uint8_t*)&header;
    for (size_t i = 0; i < sizeof(header); i++) {
        encode_byte_to_pixels(ctx, i, header_bytes[i]);
    }
    
    if (ctx->debug_mode) {
        printf("Visual memory initialized: %dx%d, mode=%d, framebuffer=%p, ram_buffer=%p\n",
               width, height, mode, ctx->framebuffer, ctx->ram_buffer);
    }
    
    return VISUALMEM_SUCCESS;
}

void visualmem_cleanup(visualmem_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->ram_buffer) {
        free(ctx->ram_buffer);
        ctx->ram_buffer = NULL;
    }
    
    if (ctx->framebuffer) {
        free(ctx->framebuffer);
        ctx->framebuffer = NULL;
    }
    
    ctx->is_initialized = 0;
    ctx->ram_freed = 1;
}

int visualmem_enter_autonomous_mode(visualmem_context_t* ctx) {
    if (!ctx || !ctx->is_initialized) {
        return VISUALMEM_ERROR_NOT_INITIALIZED;
    }
    
    if (ctx->ram_freed) {
        return VISUALMEM_SUCCESS; // Already in autonomous mode
    }
    
    if (ctx->debug_mode) {
        printf("CRITICAL TRANSITION: Entering autonomous visual memory mode...\n");
        printf("Before: RAM buffer=%p, Framebuffer=%p, RAM freed=%s\n",
               ctx->ram_buffer, ctx->framebuffer, ctx->ram_freed ? "YES" : "NO");
    }
    
    // Final synchronization: copy RAM buffer to framebuffer
    if (ctx->ram_buffer && ctx->framebuffer) {
        memcpy(ctx->framebuffer, ctx->ram_buffer, 
               ctx->width * ctx->height * sizeof(uint32_t));
    }
    
    // Free RAM buffer - THIS IS THE CRITICAL STEP
    if (ctx->ram_buffer) {
        free(ctx->ram_buffer);
        ctx->ram_buffer = NULL;
        ctx->ram_freed = 1;
        ctx->autonomous_mode = 1;
    }
    
    if (ctx->debug_mode) {
        printf("After: RAM buffer=%p, Framebuffer=%p, RAM freed=%s\n",
               ctx->ram_buffer, ctx->framebuffer, ctx->ram_freed ? "YES" : "NO");
        printf("✅ AUTONOMOUS MODE ACTIVE: System now operates solely on visual display memory\n");
    }
    
    return VISUALMEM_SUCCESS;
}

void* visualmem_alloc(visualmem_context_t* ctx, size_t size, const char* label) {
    if (!ctx || !ctx->is_initialized || size == 0) {
        return NULL;
    }
    
    // Find free allocation slot
    int slot = -1;
    for (int i = 0; i < VISUALMEM_MAX_ALLOCATIONS; i++) {
        if (!ctx->allocations[i].is_active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return NULL; // No free slots
    }
    
    // Calculate position for this allocation
    // Simple linear allocation for now
    int start_byte = VISUALMEM_HEADER_SIZE + ctx->allocation_count * 200; // Space allocations apart
    int max_bytes = ((ctx->width - VISUALMEM_MEMORY_START_X) / VISUALMEM_BYTE_SPACING_X) *
                    ((ctx->height - VISUALMEM_MEMORY_START_Y) / VISUALMEM_BYTE_SPACING_Y);
    
    if (start_byte + (int)size > max_bytes) {
        return NULL; // Not enough visual memory
    }
    
    // Create allocation record
    ctx->allocations[slot].visual_addr = coord_to_addr(start_byte, 0);
    ctx->allocations[slot].size = size;
    ctx->allocations[slot].checksum = 0; // Will be calculated on write
    ctx->allocations[slot].timestamp = time(NULL);
    ctx->allocations[slot].is_active = 1;
    
    if (label) {
        strncpy(ctx->allocations[slot].label, label, sizeof(ctx->allocations[slot].label) - 1);
        ctx->allocations[slot].label[sizeof(ctx->allocations[slot].label) - 1] = '\0';
    } else {
        ctx->allocations[slot].label[0] = '\0';
    }
    
    // Update statistics
    ctx->total_allocated += size;
    if (ctx->total_allocated > ctx->peak_usage) {
        ctx->peak_usage = ctx->total_allocated;
    }
    ctx->allocation_count++;
    
    if (ctx->debug_mode) {
        printf("Visual memory allocated: %zu bytes at visual address %p, label='%s'\n",
               size, ctx->allocations[slot].visual_addr, 
               ctx->allocations[slot].label);
    }
    
    return ctx->allocations[slot].visual_addr;
}

int visualmem_free(visualmem_context_t* ctx, void* visual_addr) {
    if (!ctx || !visual_addr) {
        return VISUALMEM_ERROR_INVALID_ADDRESS;
    }
    
    // Find allocation
    int slot = -1;
    for (int i = 0; i < VISUALMEM_MAX_ALLOCATIONS; i++) {
        if (ctx->allocations[i].is_active && 
            ctx->allocations[i].visual_addr == visual_addr) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return VISUALMEM_ERROR_INVALID_ADDRESS;
    }
    
    // Clear visual memory area
    int x, y;
    addr_to_coord(visual_addr, &x, &y);
    
    // Clear the allocated pixels
    for (size_t i = 0; i < ctx->allocations[slot].size; i++) {
        encode_byte_to_pixels(ctx, x + i, 0);
    }
    
    // Update statistics
    ctx->total_allocated -= ctx->allocations[slot].size;
    ctx->allocation_count--;
    
    if (ctx->debug_mode) {
        printf("Visual memory freed: %zu bytes at visual address %p, label='%s'\n",
               ctx->allocations[slot].size, visual_addr, 
               ctx->allocations[slot].label);
    }
    
    // Mark allocation as inactive
    ctx->allocations[slot].is_active = 0;
    
    return VISUALMEM_SUCCESS;
}

int visualmem_write(visualmem_context_t* ctx, void* visual_addr, const void* data, size_t size) {
    if (!ctx || !visual_addr || !data || size == 0) {
        return VISUALMEM_ERROR_INVALID_ADDRESS;
    }
    
    // Find allocation
    const visualmem_allocation_t* alloc = visualmem_get_allocation_info(ctx, visual_addr);
    if (!alloc) {
        return VISUALMEM_ERROR_INVALID_ADDRESS;
    }
    
    if (size > alloc->size) {
        return VISUALMEM_ERROR_INVALID_SIZE;
    }
    
    // Get starting byte position from allocation table
    int byte_offset = -1;
    for (int i = 0; i < VISUALMEM_MAX_ALLOCATIONS; i++) {
        if (ctx->allocations[i].is_active && ctx->allocations[i].visual_addr == visual_addr) {
            int x, y;
            addr_to_coord(visual_addr, &x, &y);
            byte_offset = x;
            break;
        }
    }
    
    if (byte_offset == -1) {
        return VISUALMEM_ERROR_INVALID_ADDRESS;
    }
    
    // Write data byte by byte to visual memory
    const uint8_t* src_bytes = (const uint8_t*)data;
    for (size_t i = 0; i < size; i++) {
        encode_byte_to_pixels(ctx, byte_offset + i, src_bytes[i]);
    }
    
    ctx->operations_count++;
    
    if (ctx->debug_mode) {
        printf("Visual memory write: %zu bytes to visual address %p\n", size, visual_addr);
    }
    
    return VISUALMEM_SUCCESS;
}

int visualmem_read(visualmem_context_t* ctx, void* visual_addr, void* buffer, size_t size) {
    if (!ctx || !visual_addr || !buffer || size == 0) {
        return VISUALMEM_ERROR_INVALID_ADDRESS;
    }
    
    // Find allocation
    const visualmem_allocation_t* alloc = visualmem_get_allocation_info(ctx, visual_addr);
    if (!alloc) {
        return VISUALMEM_ERROR_INVALID_ADDRESS;
    }
    
    if (size > alloc->size) {
        return VISUALMEM_ERROR_INVALID_SIZE;
    }
    
    // Get starting byte position from allocation table
    int byte_offset = -1;
    for (int i = 0; i < VISUALMEM_MAX_ALLOCATIONS; i++) {
        if (ctx->allocations[i].is_active && ctx->allocations[i].visual_addr == visual_addr) {
            int x, y;
            addr_to_coord(visual_addr, &x, &y);
            byte_offset = x;
            break;
        }
    }
    
    if (byte_offset == -1) {
        return VISUALMEM_ERROR_INVALID_ADDRESS;
    }
    
    // Read data byte by byte from visual memory
    uint8_t* dest_bytes = (uint8_t*)buffer;
    for (size_t i = 0; i < size; i++) {
        dest_bytes[i] = decode_byte_from_pixels(ctx, byte_offset + i);
    }
    
    ctx->operations_count++;
    
    if (ctx->debug_mode) {
        printf("Visual memory read: %zu bytes from visual address %p\n", size, visual_addr);
    }
    
    return VISUALMEM_SUCCESS;
}

int visualmem_write_string(visualmem_context_t* ctx, void* visual_addr, const char* str) {
    if (!str) return VISUALMEM_ERROR_INVALID_ADDRESS;
    
    size_t len = strlen(str);
    int result = visualmem_write(ctx, visual_addr, str, len);
    if (result != VISUALMEM_SUCCESS) return result;
    
    // Write null terminator
    uint8_t null_byte = 0;
    int x, y;
    addr_to_coord(visual_addr, &x, &y);
    encode_byte_to_pixels(ctx, x + len, null_byte);
    
    return VISUALMEM_SUCCESS;
}

int visualmem_read_string(visualmem_context_t* ctx, void* visual_addr, char* buffer, size_t max_length) {
    if (!ctx || !visual_addr || !buffer || max_length == 0) {
        return VISUALMEM_ERROR_INVALID_ADDRESS;
    }
    
    int x, y;
    addr_to_coord(visual_addr, &x, &y);
    
    // Read characters until null terminator or max length
    for (size_t i = 0; i < max_length - 1; i++) {
        uint8_t byte_value = decode_byte_from_pixels(ctx, x + i);
        buffer[i] = (char)byte_value;
        
        if (byte_value == 0) {
            break; // Found null terminator
        }
    }
    
    buffer[max_length - 1] = '\0'; // Ensure null termination
    
    return VISUALMEM_SUCCESS;
}

const visualmem_allocation_t* visualmem_get_allocation_info(visualmem_context_t* ctx, void* visual_addr) {
    if (!ctx || !visual_addr) return NULL;
    
    for (int i = 0; i < VISUALMEM_MAX_ALLOCATIONS; i++) {
        if (ctx->allocations[i].is_active && 
            ctx->allocations[i].visual_addr == visual_addr) {
            return &ctx->allocations[i];
        }
    }
    
    return NULL;
}

void visualmem_get_stats(visualmem_context_t* ctx, size_t* total_allocated, 
                        size_t* peak_usage, int* fragmentation) {
    if (!ctx) return;
    
    if (total_allocated) *total_allocated = ctx->total_allocated;
    if (peak_usage) *peak_usage = ctx->peak_usage;
    if (fragmentation) {
        // Simple fragmentation calculation
        *fragmentation = ctx->allocation_count > 1 ? 
            (ctx->allocation_count * 10) : 0; // Rough estimate
    }
}

const char* visualmem_get_error_string(visualmem_error_t error_code) {
    switch (error_code) {
        case VISUALMEM_SUCCESS: return "Success";
        case VISUALMEM_ERROR_INIT_FAILED: return "Initialization failed";
        case VISUALMEM_ERROR_INVALID_MODE: return "Invalid mode";
        case VISUALMEM_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case VISUALMEM_ERROR_INVALID_ADDRESS: return "Invalid address";
        case VISUALMEM_ERROR_ALLOCATION_FAILED: return "Allocation failed";
        case VISUALMEM_ERROR_FRAGMENTATION: return "Memory fragmentation";
        case VISUALMEM_ERROR_CORRUPTION: return "Data corruption detected";
        case VISUALMEM_ERROR_NOT_INITIALIZED: return "System not initialized";
        case VISUALMEM_ERROR_DISPLAY_UNAVAILABLE: return "Display unavailable";
        case VISUALMEM_ERROR_INVALID_SIZE: return "Invalid size";
        default: return "Unknown error";
    }
}

const char* visualmem_get_version(void) {
    return LIBVISUALMEM_VERSION;
}

void visualmem_set_debug_mode(visualmem_context_t* ctx, int enable) {
    if (ctx) {
        ctx->debug_mode = enable;
    }
}

void visualmem_display_contents(visualmem_context_t* ctx, const visualmem_rect_t* rect) {
    if (!ctx || !ctx->framebuffer) return;
    
    int start_x = rect ? rect->x : 0;
    int start_y = rect ? rect->y : 0;
    int end_x = rect ? (rect->x + rect->width) : ctx->width;
    int end_y = rect ? (rect->y + rect->height) : ctx->height;
    
    if (end_x > ctx->width) end_x = ctx->width;
    if (end_y > ctx->height) end_y = ctx->height;
    
    printf("\n=== Visual Memory Contents ===\n");
    printf("Legend: '.'=bit0, '#'=bit1, 'S'=start, 'E'=end, ' '=free\n\n");
    
    uint32_t* framebuffer = (uint32_t*)ctx->framebuffer;
    
    // Show first 80 characters of each row for readability
    int display_width = (end_x - start_x > 80) ? 80 : (end_x - start_x);
    
    for (int y = start_y; y < end_y && y < start_y + 20; y++) {
        printf("%2d ", y);
        for (int x = start_x; x < start_x + display_width; x++) {
            uint32_t color = framebuffer[y * ctx->width + x];
            
            char c = ' ';
            switch (color & 0x00FFFFFF) {
                case (VISUALMEM_COLOR_BIT_0 & 0x00FFFFFF): c = '.'; break;
                case (VISUALMEM_COLOR_BIT_1 & 0x00FFFFFF): c = '#'; break;
                case (VISUALMEM_COLOR_START & 0x00FFFFFF): c = 'S'; break;
                case (VISUALMEM_COLOR_END & 0x00FFFFFF): c = 'E'; break;
                case (VISUALMEM_COLOR_ADDR & 0x00FFFFFF): c = 'A'; break;
                case (VISUALMEM_COLOR_RESERVED & 0x00FFFFFF): c = 'R'; break;
                default: c = ' '; break;
            }
            printf("%c", c);
        }
        printf("\n");
    }
    
    printf("\n=== End Visual Memory Contents ===\n");
}