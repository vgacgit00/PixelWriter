/**
 * LibVisualMem v2.0 - Hardware-Native Visual Memory Library
 * ========================================================
 * 
 * Real hardware implementation using X11, OpenGL, and direct framebuffer access
 * for authentic visual memory operations on physical/virtual displays.
 * 
 * Features:
 * - Real X11/Xvfb display integration
 * - Direct pixel manipulation on screen
 * - Hardware-accelerated operations (OpenGL)
 * - Multi-resolution support (640x480 to 1920x1080)
 * - Authentic visual persistence
 * - Performance optimized for real hardware
 * 
 * Copyright (C) 2025 - Visual Memory Systems v2.0
 * License: MIT
 */

#ifndef LIBVISUALMEM_V2_H
#define LIBVISUALMEM_V2_H

/**
 * LibVisualMem v2.0 - Hardware-Native Visual Memory Library
 * ========================================================
 * 
 * Real hardware implementation using X11, OpenGL, and direct framebuffer access
 * for authentic visual memory operations on physical/virtual displays.
 * 
 * Features:
 * - Real X11/Xvfb display integration
 * - Direct pixel manipulation on screen
 * - Hardware-accelerated operations (OpenGL)
 * - Multi-resolution support (640x480 to 1920x1080)
 * - Authentic visual persistence
 * - Performance optimized for real hardware
 * 
 * Copyright (C) 2025 - Visual Memory Systems v2.0
 * License: MIT
 */

#include <stdint.h>
#include <stddef.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <pthread.h>

// === LIBRARY VERSION ===
#define LIBVISUALMEM_V2_VERSION_MAJOR 2
#define LIBVISUALMEM_V2_VERSION_MINOR 0
#define LIBVISUALMEM_V2_VERSION_PATCH 0
#define LIBVISUALMEM_V2_VERSION "2.0.0"

// === HARDWARE CONFIGURATION ===
#define VISUALMEM_V2_MAX_WIDTH 1920
#define VISUALMEM_V2_MAX_HEIGHT 1080
#define VISUALMEM_V2_MIN_WIDTH 640
#define VISUALMEM_V2_MIN_HEIGHT 480
#define VISUALMEM_V2_MAX_ALLOCATIONS 2048
#define VISUALMEM_V2_BYTES_PER_PIXEL 4  // RGBA32

// === DISPLAY MODES ===
typedef enum {
    VISUALMEM_V2_MODE_X11_WINDOW,    // X11 windowed display
    VISUALMEM_V2_MODE_X11_FULLSCREEN, // X11 fullscreen
    VISUALMEM_V2_MODE_FRAMEBUFFER,   // Direct framebuffer access
    VISUALMEM_V2_MODE_XVFB,          // Virtual framebuffer (Xvfb)
    VISUALMEM_V2_MODE_OPENGL         // OpenGL accelerated
} visualmem_v2_mode_t;

// === HARDWARE BACKENDS ===
typedef enum {
    VISUALMEM_V2_BACKEND_AUTO,       // Auto-detect best backend
    VISUALMEM_V2_BACKEND_X11,        // X11/XLib backend
    VISUALMEM_V2_BACKEND_FRAMEBUFFER, // Linux framebuffer
    VISUALMEM_V2_BACKEND_OPENGL,     // OpenGL backend
    VISUALMEM_V2_BACKEND_DRM         // Direct Rendering Manager
} visualmem_v2_backend_t;

// === ERROR CODES ===
typedef enum {
    VISUALMEM_V2_SUCCESS = 0,
    VISUALMEM_V2_ERROR_INIT_FAILED = -1,
    VISUALMEM_V2_ERROR_DISPLAY_UNAVAILABLE = -2,
    VISUALMEM_V2_ERROR_HARDWARE_UNSUPPORTED = -3,
    VISUALMEM_V2_ERROR_OUT_OF_VIDEO_MEMORY = -4,
    VISUALMEM_V2_ERROR_INVALID_ADDRESS = -5,
    VISUALMEM_V2_ERROR_ALLOCATION_FAILED = -6,
    VISUALMEM_V2_ERROR_DISPLAY_LOST = -7,
    VISUALMEM_V2_ERROR_OPENGL_FAILED = -8,
    VISUALMEM_V2_ERROR_THREAD_FAILED = -9,
    VISUALMEM_V2_ERROR_INVALID_RESOLUTION = -10
} visualmem_v2_error_t;

// === PIXEL FORMATS ===
typedef enum {
    VISUALMEM_V2_PIXEL_RGBA32,       // 32-bit RGBA
    VISUALMEM_V2_PIXEL_RGB24,        // 24-bit RGB
    VISUALMEM_V2_PIXEL_RGB16,        // 16-bit RGB565
    VISUALMEM_V2_PIXEL_MONOCHROME    // 1-bit monochrome
} visualmem_v2_pixel_format_t;

// === HARDWARE CAPABILITIES ===
typedef struct {
    int has_x11;                     // X11 support available
    int has_opengl;                  // OpenGL support available
    int has_framebuffer;             // Direct framebuffer access
    int has_drm;                     // DRM/KMS support
    int max_texture_size;            // Maximum OpenGL texture size
    int video_memory_mb;             // Available video memory (MB)
    char gpu_vendor[64];             // GPU vendor string
    char gpu_model[128];             // GPU model string
} visualmem_v2_hardware_caps_t;

// === X11 DISPLAY CONTEXT ===
typedef struct {
    Display* display;                // X11 display connection
    Window window;                   // X11 window handle
    GC gc;                          // Graphics context
    XImage* ximage;                 // X11 image buffer
    Colormap colormap;              // Color map
    int screen;                     // Screen number
    Visual* visual;                 // Visual information
    int depth;                      // Color depth
} visualmem_v2_x11_context_t;

// === OPENGL CONTEXT ===
typedef struct {
    void* gl_context;               // OpenGL context (GLXContext)
    unsigned int texture_id;        // OpenGL texture ID
    unsigned int framebuffer_id;    // OpenGL framebuffer ID
    unsigned int shader_program;    // Shader program ID
    int gl_version_major;           // OpenGL major version
    int gl_version_minor;           // OpenGL minor version
} visualmem_v2_opengl_context_t;

// === FRAMEBUFFER CONTEXT ===
typedef struct {
    int fb_fd;                      // Framebuffer file descriptor
    void* fb_memory;                // Mapped framebuffer memory
    size_t fb_size;                 // Framebuffer size in bytes
    int fb_width;                   // Framebuffer width
    int fb_height;                  // Framebuffer height
    int fb_bpp;                     // Bits per pixel
    int fb_stride;                  // Bytes per line
} visualmem_v2_framebuffer_context_t;

// === MEMORY ALLOCATION INFO ===
typedef struct {
    void* visual_addr;              // Visual coordinate address
    size_t size;                    // Allocated size in bytes
    int x, y;                       // Screen coordinates
    int width, height;              // Allocation dimensions
    uint32_t checksum;              // Data integrity checksum
    uint64_t timestamp;             // Allocation timestamp
    int is_active;                  // Allocation status
    char label[64];                 // Allocation label
    pthread_mutex_t mutex;          // Thread safety
} visualmem_v2_allocation_t;

// === PERFORMANCE METRICS ===
typedef struct {
    uint64_t total_allocations;     // Total allocations made
    uint64_t total_deallocations;   // Total deallocations
    uint64_t bytes_written;         // Total bytes written
    uint64_t bytes_read;            // Total bytes read
    double avg_write_speed_mbps;    // Average write speed (MB/s)
    double avg_read_speed_mbps;     // Average read speed (MB/s)
    uint64_t display_refreshes;     // Display refresh count
    double frame_rate;              // Current frame rate
    uint64_t pixel_operations;      // Total pixel operations
} visualmem_v2_performance_t;

// === MAIN CONTEXT STRUCTURE ===
typedef struct {
    // Display properties
    int width;
    int height;
    visualmem_v2_mode_t mode;
    visualmem_v2_backend_t backend;
    visualmem_v2_pixel_format_t pixel_format;
    
    // Hardware contexts
    visualmem_v2_x11_context_t x11;
    visualmem_v2_opengl_context_t opengl;
    visualmem_v2_framebuffer_context_t framebuffer;
    visualmem_v2_hardware_caps_t hardware;
    
    // Memory management
    void* video_memory;             // Real video memory buffer
    visualmem_v2_allocation_t allocations[VISUALMEM_V2_MAX_ALLOCATIONS];
    int allocation_count;
    
    // Threading and synchronization
    pthread_t display_thread;       // Display refresh thread
    pthread_mutex_t context_mutex;  // Context protection
    pthread_cond_t display_cond;    // Display synchronization
    int display_thread_running;     // Thread control flag
    
    // Status and performance
    int is_initialized;
    int hardware_active;            // Hardware acceleration active
    int display_active;             // Display is active
    visualmem_v2_performance_t performance;
    
    // Configuration
    int vsync_enabled;              // Vertical sync
    int double_buffering;           // Double buffering
    int refresh_rate_hz;            // Target refresh rate
    int debug_mode;                 // Debug logging
} visualmem_v2_context_t;

// === CORE API FUNCTIONS ===

/**
 * Initialize visual memory system with hardware detection
 */
int visualmem_v2_init(visualmem_v2_context_t* ctx, 
                      visualmem_v2_mode_t mode,
                      int width, int height);

/**
 * Initialize with specific backend
 */
int visualmem_v2_init_with_backend(visualmem_v2_context_t* ctx,
                                   visualmem_v2_backend_t backend,
                                   visualmem_v2_mode_t mode,
                                   int width, int height);

/**
 * Cleanup and release all resources
 */
void visualmem_v2_cleanup(visualmem_v2_context_t* ctx);

/**
 * Get hardware capabilities
 */
int visualmem_v2_get_hardware_caps(visualmem_v2_hardware_caps_t* caps);

// === MEMORY ALLOCATION FUNCTIONS ===

/**
 * Allocate visual memory with real screen coordinates
 */
void* visualmem_v2_alloc(visualmem_v2_context_t* ctx, 
                         size_t size, 
                         const char* label);

/**
 * Allocate visual memory at specific coordinates
 */
void* visualmem_v2_alloc_at(visualmem_v2_context_t* ctx,
                            int x, int y, int width, int height,
                            const char* label);

/**
 * Free visual memory allocation
 */
int visualmem_v2_free(visualmem_v2_context_t* ctx, void* visual_addr);

/**
 * Reallocate visual memory (resize)
 */
void* visualmem_v2_realloc(visualmem_v2_context_t* ctx, 
                           void* visual_addr, 
                           size_t new_size);

// === DATA OPERATIONS ===

/**
 * Write data to visual memory (real pixels)
 */
int visualmem_v2_write(visualmem_v2_context_t* ctx, 
                       void* visual_addr, 
                       const void* data, 
                       size_t size);

/**
 * Read data from visual memory (real pixels)
 */
int visualmem_v2_read(visualmem_v2_context_t* ctx, 
                      void* visual_addr, 
                      void* buffer, 
                      size_t size);

/**
 * Write pixel directly to screen coordinates
 */
int visualmem_v2_write_pixel(visualmem_v2_context_t* ctx, 
                             int x, int y, 
                             uint32_t color);

/**
 * Read pixel directly from screen coordinates
 */
uint32_t visualmem_v2_read_pixel(visualmem_v2_context_t* ctx, 
                                 int x, int y);

/**
 * Write image data to visual memory
 */
int visualmem_v2_write_image(visualmem_v2_context_t* ctx,
                             void* visual_addr,
                             const void* image_data,
                             int width, int height,
                             visualmem_v2_pixel_format_t format);

// === DISPLAY CONTROL ===

/**
 * Force display refresh/update
 */
int visualmem_v2_refresh_display(visualmem_v2_context_t* ctx);

/**
 * Set display refresh rate
 */
int visualmem_v2_set_refresh_rate(visualmem_v2_context_t* ctx, int hz);

/**
 * Enable/disable vsync
 */
int visualmem_v2_set_vsync(visualmem_v2_context_t* ctx, int enabled);

/**
 * Take screenshot of current visual memory
 */
int visualmem_v2_screenshot(visualmem_v2_context_t* ctx, 
                            const char* filename);

// === PERFORMANCE AND MONITORING ===

/**
 * Get performance statistics
 */
int visualmem_v2_get_performance(visualmem_v2_context_t* ctx,
                                visualmem_v2_performance_t* perf);

/**
 * Reset performance counters
 */
void visualmem_v2_reset_performance(visualmem_v2_context_t* ctx);

/**
 * Get memory usage statistics
 */
int visualmem_v2_get_memory_stats(visualmem_v2_context_t* ctx,
                                  size_t* allocated,
                                  size_t* peak,
                                  int* fragmentation);

// === UTILITY FUNCTIONS ===

/**
 * Convert RGB to system pixel format
 */
uint32_t visualmem_v2_rgb_to_pixel(visualmem_v2_context_t* ctx,
                                   uint8_t r, uint8_t g, uint8_t b);

/**
 * Convert pixel to RGB
 */
void visualmem_v2_pixel_to_rgb(visualmem_v2_context_t* ctx,
                               uint32_t pixel,
                               uint8_t* r, uint8_t* g, uint8_t* b);

/**
 * Get error string
 */
const char* visualmem_v2_get_error_string(visualmem_v2_error_t error);

/**
 * Enable debug mode
 */
void visualmem_v2_set_debug_mode(visualmem_v2_context_t* ctx, int enabled);

// === THREAD SAFETY ===

/**
 * Lock context for thread-safe operations
 */
int visualmem_v2_lock(visualmem_v2_context_t* ctx);

/**
 * Unlock context
 */
int visualmem_v2_unlock(visualmem_v2_context_t* ctx);

#endif // LIBVISUALMEM_V2_H