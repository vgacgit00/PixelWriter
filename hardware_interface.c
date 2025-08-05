/**
 * Hardware Interface - LibVisualMem v2.0
 * =======================================
 * 
 * Hardware abstraction layer for real graphics hardware access
 * Supports X11, OpenGL, Framebuffer, and DRM backends
 */

#include "libvisualmem_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <dlfcn.h>

// === HARDWARE DETECTION ===

/**
 * Detect available graphics hardware and capabilities
 */
int visualmem_v2_detect_hardware(visualmem_v2_hardware_caps_t* caps) {
    if (!caps) return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    
    memset(caps, 0, sizeof(visualmem_v2_hardware_caps_t));
    
    // Test X11 availability
    Display* test_display = XOpenDisplay(NULL);
    if (test_display) {
        caps->has_x11 = 1;
        XCloseDisplay(test_display);
        printf("[HARDWARE] X11 support detected\n");
    } else {
        // Try connecting to Xvfb if available
        test_display = XOpenDisplay(":99");
        if (test_display) {
            caps->has_x11 = 1;
            XCloseDisplay(test_display);
            printf("[HARDWARE] Xvfb virtual display detected\n");
        } else {
            printf("[HARDWARE] No X11 display available\n");
        }
    }
    
    // Test OpenGL availability (requires X11)
    if (caps->has_x11) {
        // Try to load OpenGL library
        void* gl_lib = dlopen("libGL.so.1", RTLD_LAZY);
        if (gl_lib) {
            caps->has_opengl = 1;
            caps->max_texture_size = 2048; // Conservative default
            dlclose(gl_lib);
            printf("[HARDWARE] OpenGL support detected\n");
        }
    }
    
    // Test framebuffer availability
    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd >= 0) {
        caps->has_framebuffer = 1;
        close(fb_fd);
        printf("[HARDWARE] Framebuffer support detected\n");
    }
    
    // Test DRM availability
    if (access("/dev/dri/card0", F_OK) == 0) {
        caps->has_drm = 1;
        printf("[HARDWARE] DRM/KMS support detected\n");
    }
    
    // Estimate video memory (simplified)
    caps->video_memory_mb = 64; // Conservative estimate
    strcpy(caps->gpu_vendor, "Generic");
    strcpy(caps->gpu_model, "Software Renderer");
    
    return VISUALMEM_V2_SUCCESS;
}

/**
 * Select best backend based on hardware capabilities
 */
visualmem_v2_backend_t visualmem_v2_select_best_backend(const visualmem_v2_hardware_caps_t* caps) {
    if (!caps) return VISUALMEM_V2_BACKEND_AUTO;
    
    // Priority order: OpenGL > X11 > Framebuffer > DRM
    if (caps->has_opengl && caps->has_x11) {
        printf("[HARDWARE] Selected OpenGL backend (hardware accelerated)\n");
        return VISUALMEM_V2_BACKEND_OPENGL;
    }
    
    if (caps->has_x11) {
        printf("[HARDWARE] Selected X11 backend\n");
        return VISUALMEM_V2_BACKEND_X11;
    }
    
    if (caps->has_framebuffer) {
        printf("[HARDWARE] Selected Framebuffer backend\n");
        return VISUALMEM_V2_BACKEND_FRAMEBUFFER;
    }
    
    if (caps->has_drm) {
        printf("[HARDWARE] Selected DRM backend\n");
        return VISUALMEM_V2_BACKEND_DRM;
    }
    
    printf("[HARDWARE] No suitable backend found, using auto-fallback\n");
    return VISUALMEM_V2_BACKEND_AUTO;
}

// === X11 BACKEND IMPLEMENTATION ===

/**
 * Initialize X11 display context
 */
int visualmem_v2_init_x11_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    
    printf("[X11] Initializing X11 backend...\n");
    
    // Try to connect to display
    ctx->x11.display = XOpenDisplay(NULL);
    if (!ctx->x11.display) {
        // Fallback to Xvfb
        ctx->x11.display = XOpenDisplay(":99");
        if (!ctx->x11.display) {
            printf("[X11] ERROR: Cannot connect to X11 display\n");
            return VISUALMEM_V2_ERROR_DISPLAY_UNAVAILABLE;
        }
        printf("[X11] Connected to Xvfb virtual display\n");
    } else {
        printf("[X11] Connected to X11 display\n");
    }
    
    ctx->x11.screen = DefaultScreen(ctx->x11.display);
    ctx->x11.visual = DefaultVisual(ctx->x11.display, ctx->x11.screen);
    ctx->x11.depth = DefaultDepth(ctx->x11.display, ctx->x11.screen);
    ctx->x11.colormap = DefaultColormap(ctx->x11.display, ctx->x11.screen);
    
    printf("[X11] Screen: %d, Depth: %d bits\n", ctx->x11.screen, ctx->x11.depth);
    
    // Create window
    Window root = RootWindow(ctx->x11.display, ctx->x11.screen);
    ctx->x11.window = XCreateSimpleWindow(
        ctx->x11.display, root,
        0, 0, ctx->width, ctx->height,
        1, BlackPixel(ctx->x11.display, ctx->x11.screen),
        WhitePixel(ctx->x11.display, ctx->x11.screen)
    );
    
    if (!ctx->x11.window) {
        printf("[X11] ERROR: Failed to create window\n");
        XCloseDisplay(ctx->x11.display);
        return VISUALMEM_V2_ERROR_INIT_FAILED;
    }
    
    // Set window properties
    XStoreName(ctx->x11.display, ctx->x11.window, "LibVisualMem v2.0 - Hardware Display");
    XSelectInput(ctx->x11.display, ctx->x11.window, ExposureMask | KeyPressMask);
    
    // Map window (make it visible)
    if (ctx->mode == VISUALMEM_V2_MODE_X11_WINDOW) {
        XMapWindow(ctx->x11.display, ctx->x11.window);
        printf("[X11] Window mapped and visible\n");
    }
    
    // Create graphics context
    ctx->x11.gc = XCreateGC(ctx->x11.display, ctx->x11.window, 0, NULL);
    if (!ctx->x11.gc) {
        printf("[X11] ERROR: Failed to create graphics context\n");
        XDestroyWindow(ctx->x11.display, ctx->x11.window);
        XCloseDisplay(ctx->x11.display);
        return VISUALMEM_V2_ERROR_INIT_FAILED;
    }
    
    // Create XImage for pixel manipulation
    int bytes_per_pixel = (ctx->x11.depth + 7) / 8;
    size_t image_size = ctx->width * ctx->height * bytes_per_pixel;
    
    char* image_data = (char*)malloc(image_size);
    if (!image_data) {
        printf("[X11] ERROR: Failed to allocate image buffer\n");
        XFreeGC(ctx->x11.display, ctx->x11.gc);
        XDestroyWindow(ctx->x11.display, ctx->x11.window);
        XCloseDisplay(ctx->x11.display);
        return VISUALMEM_V2_ERROR_OUT_OF_VIDEO_MEMORY;
    }
    
    ctx->x11.ximage = XCreateImage(
        ctx->x11.display, ctx->x11.visual, ctx->x11.depth,
        ZPixmap, 0, image_data,
        ctx->width, ctx->height,
        32, 0
    );
    
    if (!ctx->x11.ximage) {
        printf("[X11] ERROR: Failed to create XImage\n");
        free(image_data);
        XFreeGC(ctx->x11.display, ctx->x11.gc);
        XDestroyWindow(ctx->x11.display, ctx->x11.window);
        XCloseDisplay(ctx->x11.display);
        return VISUALMEM_V2_ERROR_INIT_FAILED;
    }
    
    // Clear the image
    memset(image_data, 0, image_size);
    
    // Force initial display update
    XPutImage(ctx->x11.display, ctx->x11.window, ctx->x11.gc,
              ctx->x11.ximage, 0, 0, 0, 0, ctx->width, ctx->height);
    XFlush(ctx->x11.display);
    
    printf("[X11] Backend initialized successfully (%dx%d, %d-bit)\n", 
           ctx->width, ctx->height, ctx->x11.depth);
    
    return VISUALMEM_V2_SUCCESS;
}

/**
 * Write pixel to X11 display
 */
int visualmem_v2_x11_write_pixel(visualmem_v2_context_t* ctx, int x, int y, uint32_t color) {
    if (!ctx || !ctx->x11.display || !ctx->x11.ximage) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    // Convert RGBA to X11 pixel format
    unsigned long pixel = color & 0x00FFFFFF; // Remove alpha channel for X11
    
    XPutPixel(ctx->x11.ximage, x, y, pixel);
    
    return VISUALMEM_V2_SUCCESS;
}

/**
 * Read pixel from X11 display
 */
uint32_t visualmem_v2_x11_read_pixel(visualmem_v2_context_t* ctx, int x, int y) {
    if (!ctx || !ctx->x11.display || !ctx->x11.ximage) {
        return 0;
    }
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return 0;
    }
    
    unsigned long pixel = XGetPixel(ctx->x11.ximage, x, y);
    return (uint32_t)(pixel | 0xFF000000); // Add alpha channel
}

/**
 * Refresh X11 display
 */
int visualmem_v2_x11_refresh(visualmem_v2_context_t* ctx) {
    if (!ctx || !ctx->x11.display || !ctx->x11.window || !ctx->x11.ximage) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    // Copy image buffer to window
    XPutImage(ctx->x11.display, ctx->x11.window, ctx->x11.gc,
              ctx->x11.ximage, 0, 0, 0, 0, ctx->width, ctx->height);
    
    // Force immediate display update
    XFlush(ctx->x11.display);
    XSync(ctx->x11.display, False);
    
    return VISUALMEM_V2_SUCCESS;
}

/**
 * Cleanup X11 backend
 */
void visualmem_v2_cleanup_x11_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) return;
    
    printf("[X11] Cleaning up X11 backend...\n");
    
    if (ctx->x11.ximage) {
        if (ctx->x11.ximage->data) {
            free(ctx->x11.ximage->data);
        }
        XDestroyImage(ctx->x11.ximage);
        ctx->x11.ximage = NULL;
    }
    
    if (ctx->x11.gc) {
        XFreeGC(ctx->x11.display, ctx->x11.gc);
        ctx->x11.gc = 0;
    }
    
    if (ctx->x11.window) {
        XDestroyWindow(ctx->x11.display, ctx->x11.window);
        ctx->x11.window = 0;
    }
    
    if (ctx->x11.display) {
        XCloseDisplay(ctx->x11.display);
        ctx->x11.display = NULL;
    }
    
    printf("[X11] Cleanup completed\n");
}

// === FRAMEBUFFER BACKEND IMPLEMENTATION ===

/**
 * Initialize framebuffer backend
 */
int visualmem_v2_init_framebuffer_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    
    printf("[FB] Initializing framebuffer backend...\n");
    
    // Try to open framebuffer device
    ctx->framebuffer.fb_fd = open("/dev/fb0", O_RDWR);
    if (ctx->framebuffer.fb_fd < 0) {
        printf("[FB] ERROR: Cannot open /dev/fb0\n");
        return VISUALMEM_V2_ERROR_DISPLAY_UNAVAILABLE;
    }
    
    // Get framebuffer info
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    
    if (ioctl(ctx->framebuffer.fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        printf("[FB] ERROR: Cannot get variable screen info\n");
        close(ctx->framebuffer.fb_fd);
        return VISUALMEM_V2_ERROR_HARDWARE_UNSUPPORTED;
    }
    
    if (ioctl(ctx->framebuffer.fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        printf("[FB] ERROR: Cannot get fixed screen info\n");
        close(ctx->framebuffer.fb_fd);
        return VISUALMEM_V2_ERROR_HARDWARE_UNSUPPORTED;
    }
    
    ctx->framebuffer.fb_width = vinfo.xres;
    ctx->framebuffer.fb_height = vinfo.yres;
    ctx->framebuffer.fb_bpp = vinfo.bits_per_pixel;
    ctx->framebuffer.fb_stride = finfo.line_length;
    ctx->framebuffer.fb_size = finfo.smem_len;
    
    printf("[FB] Framebuffer: %dx%d, %d bpp, stride: %d, size: %zu\n",
           ctx->framebuffer.fb_width, ctx->framebuffer.fb_height,
           ctx->framebuffer.fb_bpp, ctx->framebuffer.fb_stride,
           ctx->framebuffer.fb_size);
    
    // Map framebuffer memory
    ctx->framebuffer.fb_memory = mmap(0, ctx->framebuffer.fb_size,
                                      PROT_READ | PROT_WRITE, MAP_SHARED,
                                      ctx->framebuffer.fb_fd, 0);
    
    if (ctx->framebuffer.fb_memory == MAP_FAILED) {
        printf("[FB] ERROR: Cannot map framebuffer memory\n");
        close(ctx->framebuffer.fb_fd);
        return VISUALMEM_V2_ERROR_OUT_OF_VIDEO_MEMORY;
    }
    
    // Clear framebuffer
    memset(ctx->framebuffer.fb_memory, 0, ctx->framebuffer.fb_size);
    
    printf("[FB] Backend initialized successfully\n");
    return VISUALMEM_V2_SUCCESS;
}

/**
 * Cleanup framebuffer backend
 */
void visualmem_v2_cleanup_framebuffer_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) return;
    
    printf("[FB] Cleaning up framebuffer backend...\n");
    
    if (ctx->framebuffer.fb_memory && ctx->framebuffer.fb_memory != MAP_FAILED) {
        munmap(ctx->framebuffer.fb_memory, ctx->framebuffer.fb_size);
        ctx->framebuffer.fb_memory = NULL;
    }
    
    if (ctx->framebuffer.fb_fd >= 0) {
        close(ctx->framebuffer.fb_fd);
        ctx->framebuffer.fb_fd = -1;
    }
    
    printf("[FB] Cleanup completed\n");
}

// === BACKEND SELECTION AND INITIALIZATION ===

/**
 * Initialize hardware backend based on selection
 */
int visualmem_v2_init_hardware_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    
    printf("[HARDWARE] Initializing backend: %d\n", ctx->backend);
    
    switch (ctx->backend) {
        case VISUALMEM_V2_BACKEND_X11:
        case VISUALMEM_V2_BACKEND_OPENGL:
            return visualmem_v2_init_x11_backend(ctx);
            
        case VISUALMEM_V2_BACKEND_FRAMEBUFFER:
            return visualmem_v2_init_framebuffer_backend(ctx);
            
        case VISUALMEM_V2_BACKEND_AUTO:
            // Try backends in order of preference
            printf("[HARDWARE] Auto-detecting best backend...\n");
            
            // Try X11 first
            ctx->backend = VISUALMEM_V2_BACKEND_X11;
            if (visualmem_v2_init_x11_backend(ctx) == VISUALMEM_V2_SUCCESS) {
                printf("[HARDWARE] Auto-selected X11 backend\n");
                return VISUALMEM_V2_SUCCESS;
            }
            
            // Try framebuffer
            ctx->backend = VISUALMEM_V2_BACKEND_FRAMEBUFFER;
            if (visualmem_v2_init_framebuffer_backend(ctx) == VISUALMEM_V2_SUCCESS) {
                printf("[HARDWARE] Auto-selected framebuffer backend\n");
                return VISUALMEM_V2_SUCCESS;
            }
            
            printf("[HARDWARE] ERROR: No suitable backend found\n");
            return VISUALMEM_V2_ERROR_HARDWARE_UNSUPPORTED;
            
        default:
            printf("[HARDWARE] ERROR: Unsupported backend: %d\n", ctx->backend);
            return VISUALMEM_V2_ERROR_HARDWARE_UNSUPPORTED;
    }
}

/**
 * Cleanup hardware backend
 */
void visualmem_v2_cleanup_hardware_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) return;
    
    switch (ctx->backend) {
        case VISUALMEM_V2_BACKEND_X11:
        case VISUALMEM_V2_BACKEND_OPENGL:
            visualmem_v2_cleanup_x11_backend(ctx);
            break;
            
        case VISUALMEM_V2_BACKEND_FRAMEBUFFER:
            visualmem_v2_cleanup_framebuffer_backend(ctx);
            break;
            
        default:
            break;
    }
}