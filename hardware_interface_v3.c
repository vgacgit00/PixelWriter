/**
 * Hardware Interface v3.0 - Implémentation des fonctions manquantes
 * ===============================================================
 * 
 * Implémentation des fonctions externes nécessaires pour LibVisualMem v3.0
 */

#include "libvisualmem_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// === HARDWARE DETECTION ===
int visualmem_v2_detect_hardware(visualmem_v2_hardware_caps_t* caps) {
    if (!caps) return VISUALMEM_V2_ERROR_INIT_FAILED;
    
    // Initialize caps
    memset(caps, 0, sizeof(visualmem_v2_hardware_caps_t));
    
    // Test X11 availability
    Display* test_display = XOpenDisplay(NULL);
    if (test_display) {
        caps->has_x11 = 1;
        XCloseDisplay(test_display);
    } else {
        caps->has_x11 = 0;
    }
    
    // Test OpenGL availability (simplified)
    caps->has_opengl = 0; // Simplified for now
    
    // Set default values
    caps->max_texture_size = 2048;
    caps->video_memory_mb = 64;
    strcpy(caps->gpu_vendor, "Generic");
    strcpy(caps->gpu_model, "Software Renderer");
    
    return VISUALMEM_V2_SUCCESS;
}

// === BACKEND SELECTION ===
visualmem_v2_backend_t visualmem_v2_select_best_backend(const visualmem_v2_hardware_caps_t* caps) {
    if (!caps) return VISUALMEM_V2_BACKEND_AUTO;
    
    if (caps->has_x11) {
        return VISUALMEM_V2_BACKEND_X11;
    }
    
    return VISUALMEM_V2_BACKEND_AUTO;
}

// === HARDWARE BACKEND INITIALIZATION ===
int visualmem_v2_init_hardware_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) return VISUALMEM_V2_ERROR_INIT_FAILED;
    
    // Initialize X11 backend
    if (ctx->backend == VISUALMEM_V2_BACKEND_X11) {
        ctx->x11.display = XOpenDisplay(NULL);
        if (!ctx->x11.display) {
            return VISUALMEM_V2_ERROR_DISPLAY_UNAVAILABLE;
        }
        
        ctx->x11.screen = DefaultScreen(ctx->x11.display);
        ctx->x11.visual = DefaultVisual(ctx->x11.display, ctx->x11.screen);
        ctx->x11.depth = DefaultDepth(ctx->x11.display, ctx->x11.screen);
        
        // Create window
        ctx->x11.window = XCreateSimpleWindow(ctx->x11.display,
                                            DefaultRootWindow(ctx->x11.display),
                                            0, 0, ctx->width, ctx->height,
                                            1, BlackPixel(ctx->x11.display, ctx->x11.screen),
                                            WhitePixel(ctx->x11.display, ctx->x11.screen));
        
        if (!ctx->x11.window) {
            XCloseDisplay(ctx->x11.display);
            return VISUALMEM_V2_ERROR_DISPLAY_UNAVAILABLE;
        }
        
        // Create graphics context
        ctx->x11.gc = XCreateGC(ctx->x11.display, ctx->x11.window, 0, NULL);
        if (!ctx->x11.gc) {
            XDestroyWindow(ctx->x11.display, ctx->x11.window);
            XCloseDisplay(ctx->x11.display);
            return VISUALMEM_V2_ERROR_DISPLAY_UNAVAILABLE;
        }
        
        // Map window
        XMapWindow(ctx->x11.display, ctx->x11.window);
        XFlush(ctx->x11.display);
        
        // Create image buffer
        ctx->x11.ximage = XCreateImage(ctx->x11.display, ctx->x11.visual,
                                      ctx->x11.depth, ZPixmap, 0, NULL,
                                      ctx->width, ctx->height, 32, 0);
        
        if (!ctx->x11.ximage) {
            XFreeGC(ctx->x11.display, ctx->x11.gc);
            XDestroyWindow(ctx->x11.display, ctx->x11.window);
            XCloseDisplay(ctx->x11.display);
            return VISUALMEM_V2_ERROR_DISPLAY_UNAVAILABLE;
        }
        
        // Allocate image data
        ctx->x11.ximage->data = malloc(ctx->x11.ximage->bytes_per_line * ctx->x11.ximage->height);
        if (!ctx->x11.ximage->data) {
            XDestroyImage(ctx->x11.ximage);
            XFreeGC(ctx->x11.display, ctx->x11.gc);
            XDestroyWindow(ctx->x11.display, ctx->x11.window);
            XCloseDisplay(ctx->x11.display);
            return VISUALMEM_V2_ERROR_OUT_OF_VIDEO_MEMORY;
        }
        
        ctx->display_active = 1;
        return VISUALMEM_V2_SUCCESS;
    }
    
    return VISUALMEM_V2_ERROR_HARDWARE_UNSUPPORTED;
}

// === HARDWARE BACKEND CLEANUP ===
void visualmem_v2_cleanup_hardware_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->backend == VISUALMEM_V2_BACKEND_X11) {
        if (ctx->x11.ximage) {
            if (ctx->x11.ximage->data) {
                free(ctx->x11.ximage->data);
            }
            XDestroyImage(ctx->x11.ximage);
            ctx->x11.ximage = NULL;
        }
        
        if (ctx->x11.gc) {
            XFreeGC(ctx->x11.display, ctx->x11.gc);
            ctx->x11.gc = NULL;
        }
        
        if (ctx->x11.window) {
            XDestroyWindow(ctx->x11.display, ctx->x11.window);
            ctx->x11.window = 0;
        }
        
        if (ctx->x11.display) {
            XCloseDisplay(ctx->x11.display);
            ctx->x11.display = NULL;
        }
        
        ctx->display_active = 0;
    }
}

// === X11 PIXEL OPERATIONS ===
int visualmem_v2_x11_write_pixel(visualmem_v2_context_t* ctx, int x, int y, uint32_t color) {
    if (!ctx || !ctx->x11.display || !ctx->x11.ximage) {
        return VISUALMEM_V2_ERROR_DISPLAY_LOST;
    }
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return VISUALMEM_V2_ERROR_INVALID_ADDRESS;
    }
    
    // Convert color to X11 format
    unsigned long pixel = color;
    
    // Set pixel in image buffer
    XPutPixel(ctx->x11.ximage, x, y, pixel);
    
    // Update display
    XPutImage(ctx->x11.display, ctx->x11.window, ctx->x11.gc, ctx->x11.ximage,
              x, y, x, y, 1, 1);
    
    return VISUALMEM_V2_SUCCESS;
}

uint32_t visualmem_v2_x11_read_pixel(visualmem_v2_context_t* ctx, int x, int y) {
    if (!ctx || !ctx->x11.display || !ctx->x11.ximage) {
        return 0;
    }
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return 0;
    }
    
    // Get pixel from image buffer
    unsigned long pixel = XGetPixel(ctx->x11.ximage, x, y);
    
    return (uint32_t)pixel;
}

// === X11 DISPLAY REFRESH ===
int visualmem_v2_x11_refresh(visualmem_v2_context_t* ctx) {
    if (!ctx || !ctx->x11.display) {
        return VISUALMEM_V2_ERROR_DISPLAY_LOST;
    }
    
    // Flush X11 events
    XFlush(ctx->x11.display);
    
    // Process pending events
    XEvent event;
    while (XPending(ctx->x11.display)) {
        XNextEvent(ctx->x11.display, &event);
        
        switch (event.type) {
            case Expose:
                // Redraw window if needed
                break;
            case ClientMessage:
                // Handle window close
                if (event.xclient.data.l[0] == 33) { // WM_DELETE_WINDOW
                    ctx->display_thread_running = 0;
                }
                break;
        }
    }
    
    return VISUALMEM_V2_SUCCESS;
}