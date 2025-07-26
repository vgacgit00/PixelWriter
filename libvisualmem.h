#ifndef LIBVISUALMEM_H
#define LIBVISUALMEM_H

/**
 * LibVisualMem - Autonomous Visual Memory Library
 * ===============================================
 * 
 * A C library that transforms display screens into persistent memory storage,
 * enabling autonomous memory operations after traditional RAM liberation.
 * 
 * Features:
 * - Visual memory allocation (valloc/vfree)
 * - Screen-based data storage using pixel patterns
 * - Autonomous operation mode (no CPU/RAM dependency after init)
 * - Multi-mode support (text/pixel/hybrid)
 * - Memory addressing through visual coordinates
 * - Data integrity with error correction
 * 
 * Copyright (C) 2025 - Visual Memory Systems
 * License: MIT
 */

#include <stdint.h>
#include <stddef.h>

// === LIBRARY VERSION ===
#define LIBVISUALMEM_VERSION_MAJOR 1
#define LIBVISUALMEM_VERSION_MINOR 0
#define LIBVISUALMEM_VERSION_PATCH 0
#define LIBVISUALMEM_VERSION "1.0.0"

// === CONFIGURATION CONSTANTS ===
#define VISUALMEM_MAX_WIDTH 1920
#define VISUALMEM_MAX_HEIGHT 1080
#define VISUALMEM_BITS_PER_BYTE 8
#define VISUALMEM_MAX_ALLOCATIONS 1024
#define VISUALMEM_HEADER_SIZE 64  // Reserved screen area for metadata

// === MEMORY MODES ===
typedef enum {
    VISUALMEM_MODE_TEXT,      // Character-based visual memory (proof of concept)
    VISUALMEM_MODE_PIXEL,     // Pixel-based visual memory (full graphical)
    VISUALMEM_MODE_HYBRID,    // Combined text + pixel mode
    VISUALMEM_MODE_SIMULATE   // Simulation mode (no actual display)
} visualmem_mode_t;

// === COLOR ENCODING FOR PIXELS ===
typedef enum {
    VISUALMEM_COLOR_BIT_0     = 0xFF000000,  // Black for bit 0
    VISUALMEM_COLOR_BIT_1     = 0xFFFFFFFF,  // White for bit 1
    VISUALMEM_COLOR_START     = 0xFFFF0000,  // Red for start marker
    VISUALMEM_COLOR_END       = 0xFF00FF00,  // Green for end marker
    VISUALMEM_COLOR_ADDR      = 0xFF0000FF,  // Blue for address marker
    VISUALMEM_COLOR_FREE      = 0xFF808080,  // Gray for free space
    VISUALMEM_COLOR_RESERVED  = 0xFF800080   // Purple for reserved areas
} visualmem_color_t;

// === ERROR CODES ===
typedef enum {
    VISUALMEM_SUCCESS = 0,
    VISUALMEM_ERROR_INIT_FAILED = -1,
    VISUALMEM_ERROR_INVALID_MODE = -2,
    VISUALMEM_ERROR_OUT_OF_MEMORY = -3,
    VISUALMEM_ERROR_INVALID_ADDRESS = -4,
    VISUALMEM_ERROR_ALLOCATION_FAILED = -5,
    VISUALMEM_ERROR_FRAGMENTATION = -6,
    VISUALMEM_ERROR_CORRUPTION = -7,
    VISUALMEM_ERROR_NOT_INITIALIZED = -8,
    VISUALMEM_ERROR_DISPLAY_UNAVAILABLE = -9,
    VISUALMEM_ERROR_INVALID_SIZE = -10
} visualmem_error_t;

// === MEMORY ALLOCATION INFO ===
typedef struct {
    void* visual_addr;        // Visual coordinate address (x,y encoded)
    size_t size;             // Allocated size in bytes
    uint32_t checksum;       // Data integrity checksum
    uint64_t timestamp;      // Allocation timestamp
    int is_active;           // Allocation status
    char label[32];          // Optional allocation label
} visualmem_allocation_t;

// === VISUAL MEMORY CONTEXT ===
typedef struct {
    // Display properties
    int width;
    int height;
    visualmem_mode_t mode;
    
    // Memory management
    void* framebuffer;          // Visual display buffer
    void* ram_buffer;           // Temporary RAM buffer (freed after init)
    visualmem_allocation_t allocations[VISUALMEM_MAX_ALLOCATIONS];
    int allocation_count;
    
    // Status flags
    int is_initialized;
    int ram_freed;              // Critical: RAM liberation status
    int autonomous_mode;        // Operating without CPU/RAM dependency
    
    // Statistics
    size_t total_allocated;
    size_t peak_usage;
    uint64_t operations_count;
    
    // Configuration
    int enable_error_correction;
    int enable_compression;
    int debug_mode;
    
} visualmem_context_t;

// === COORDINATE SYSTEM ===
typedef struct {
    int x;
    int y;
    int width;
    int height;
} visualmem_rect_t;

// === CORE LIBRARY FUNCTIONS ===

/**
 * Initialize visual memory system
 * @param ctx Context to initialize
 * @param mode Operating mode (text/pixel/hybrid/simulate)
 * @param width Display width
 * @param height Display height
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_init(visualmem_context_t* ctx, visualmem_mode_t mode, int width, int height);

/**
 * Cleanup and free all visual memory resources
 * @param ctx Context to cleanup
 */
void visualmem_cleanup(visualmem_context_t* ctx);

/**
 * CRITICAL: Free RAM buffer and enter autonomous mode
 * After this call, system operates solely on visual display
 * @param ctx Context
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_enter_autonomous_mode(visualmem_context_t* ctx);

// === MEMORY ALLOCATION FUNCTIONS ===

/**
 * Allocate visual memory (equivalent to malloc)
 * @param ctx Context
 * @param size Bytes to allocate
 * @param label Optional allocation label
 * @return Visual address or NULL on failure
 */
void* visualmem_alloc(visualmem_context_t* ctx, size_t size, const char* label);

/**
 * Free visual memory (equivalent to free)
 * @param ctx Context
 * @param visual_addr Address returned by visualmem_alloc
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_free(visualmem_context_t* ctx, void* visual_addr);

/**
 * Reallocate visual memory (equivalent to realloc)
 * @param ctx Context
 * @param visual_addr Existing address
 * @param new_size New size in bytes
 * @return New visual address or NULL on failure
 */
void* visualmem_realloc(visualmem_context_t* ctx, void* visual_addr, size_t new_size);

// === DATA ACCESS FUNCTIONS ===

/**
 * Write data to visual memory
 * @param ctx Context
 * @param visual_addr Target visual address
 * @param data Source data buffer
 * @param size Bytes to write
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_write(visualmem_context_t* ctx, void* visual_addr, const void* data, size_t size);

/**
 * Read data from visual memory
 * @param ctx Context
 * @param visual_addr Source visual address
 * @param buffer Destination buffer
 * @param size Bytes to read
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_read(visualmem_context_t* ctx, void* visual_addr, void* buffer, size_t size);

/**
 * Copy data between visual memory locations
 * @param ctx Context
 * @param dest_addr Destination visual address
 * @param src_addr Source visual address
 * @param size Bytes to copy
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_copy(visualmem_context_t* ctx, void* dest_addr, void* src_addr, size_t size);

/**
 * Set visual memory to specific value
 * @param ctx Context
 * @param visual_addr Target address
 * @param value Value to set (0-255)
 * @param size Bytes to set
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_set(visualmem_context_t* ctx, void* visual_addr, int value, size_t size);

// === STRING FUNCTIONS ===

/**
 * Write string to visual memory
 * @param ctx Context
 * @param visual_addr Target address
 * @param str Null-terminated string
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_write_string(visualmem_context_t* ctx, void* visual_addr, const char* str);

/**
 * Read string from visual memory
 * @param ctx Context
 * @param visual_addr Source address
 * @param buffer Destination buffer
 * @param max_length Maximum string length including null terminator
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_read_string(visualmem_context_t* ctx, void* visual_addr, char* buffer, size_t max_length);

// === UTILITY FUNCTIONS ===

/**
 * Get allocation information
 * @param ctx Context
 * @param visual_addr Visual address to query
 * @return Allocation info or NULL if not found
 */
const visualmem_allocation_t* visualmem_get_allocation_info(visualmem_context_t* ctx, void* visual_addr);

/**
 * Get memory statistics
 * @param ctx Context
 * @param total_allocated Output: total allocated bytes
 * @param peak_usage Output: peak memory usage
 * @param fragmentation Output: fragmentation percentage (0-100)
 */
void visualmem_get_stats(visualmem_context_t* ctx, size_t* total_allocated, 
                        size_t* peak_usage, int* fragmentation);

/**
 * Verify data integrity
 * @param ctx Context
 * @param visual_addr Address to verify
 * @return VISUALMEM_SUCCESS if data is intact, error code otherwise
 */
int visualmem_verify_integrity(visualmem_context_t* ctx, void* visual_addr);

/**
 * Defragment visual memory
 * @param ctx Context
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_defragment(visualmem_context_t* ctx);

/**
 * Export visual memory state to file (debugging)
 * @param ctx Context
 * @param filename Output filename
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_export_state(visualmem_context_t* ctx, const char* filename);

/**
 * Display visual memory contents (debugging)
 * @param ctx Context
 * @param rect Screen region to display, NULL for full screen
 */
void visualmem_display_contents(visualmem_context_t* ctx, const visualmem_rect_t* rect);

// === ERROR HANDLING ===

/**
 * Get human-readable error message
 * @param error_code Error code from visualmem functions
 * @return Error message string
 */
const char* visualmem_get_error_string(visualmem_error_t error_code);

/**
 * Get library version string
 * @return Version string (e.g., "1.0.0")
 */
const char* visualmem_get_version(void);

// === ADVANCED FEATURES ===

/**
 * Create visual memory snapshot
 * @param ctx Context
 * @param snapshot_id Unique snapshot identifier
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_create_snapshot(visualmem_context_t* ctx, const char* snapshot_id);

/**
 * Restore from visual memory snapshot
 * @param ctx Context
 * @param snapshot_id Snapshot identifier
 * @return VISUALMEM_SUCCESS or error code
 */
int visualmem_restore_snapshot(visualmem_context_t* ctx, const char* snapshot_id);

/**
 * Enable/disable debug mode
 * @param ctx Context
 * @param enable Debug mode flag
 */
void visualmem_set_debug_mode(visualmem_context_t* ctx, int enable);

// === CONVENIENCE MACROS ===

// Memory allocation shortcuts
#define VALLOC(ctx, size) visualmem_alloc(ctx, size, #size)
#define VFREE(ctx, addr) visualmem_free(ctx, addr)
#define VWRITE(ctx, addr, data, size) visualmem_write(ctx, addr, data, size)
#define VREAD(ctx, addr, buffer, size) visualmem_read(ctx, addr, buffer, size)

// Error checking macro
#define VISUALMEM_CHECK(expr) do { \
    int _ret = (expr); \
    if (_ret != VISUALMEM_SUCCESS) { \
        fprintf(stderr, "Visual memory error at %s:%d: %s\n", \
                __FILE__, __LINE__, visualmem_get_error_string(_ret)); \
        return _ret; \
    } \
} while(0)

// === COMPATIBILITY LAYER ===

// Drop-in replacements for standard memory functions
#define visual_malloc(ctx, size) visualmem_alloc(ctx, size, "malloc")
#define visual_free(ctx, addr) visualmem_free(ctx, addr)
#define visual_memcpy(ctx, dest, src, size) visualmem_copy(ctx, dest, src, size)
#define visual_memset(ctx, addr, value, size) visualmem_set(ctx, addr, value, size)

#endif // LIBVISUALMEM_H