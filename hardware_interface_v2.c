/**
 * Hardware Interface v2.0 - LibVisualMem v2.0 (Version Corrigée)
 * ================================================================
 * 
 * Couche d'abstraction hardware CORRIGÉE avec gestion sécurisée
 * des ressources et prévention des segmentation faults
 */

#define _GNU_SOURCE

#include "libvisualmem_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>

// === DÉTECTION HARDWARE SÉCURISÉE ===

int visualmem_v2_detect_hardware(visualmem_v2_hardware_caps_t* caps) {
    if (!caps) {
        printf("[ERROR] Null caps pointer in detect_hardware\n");
        return VISUALMEM_V2_ERROR_INVALID_PARAMETER;
    }
    
    // Initialisation sécurisée
    memset(caps, 0, sizeof(visualmem_v2_hardware_caps_t));
    strcpy(caps->gpu_model, "Unknown");
    
    printf("[HARDWARE] Starting hardware detection...\n");
    
    // Test X11 avec gestion d'erreur
    Display* test_display = XOpenDisplay(NULL);
    if (test_display) {
        caps->has_x11 = 1;
        printf("[HARDWARE] X11 support detected\n");
        XCloseDisplay(test_display);
        test_display = NULL;
    } else {
        caps->has_x11 = 0;
        printf("[HARDWARE] X11 not available\n");
    }
    
    // Test OpenGL avec vérification
    if (caps->has_x11) {
        Display* gl_display = XOpenDisplay(NULL);
        if (gl_display) {
            int screen = DefaultScreen(gl_display);
            if (glXQueryExtension(gl_display, NULL, NULL)) {
                caps->has_opengl = 1;
                strcpy(caps->gpu_model, "Software Renderer");
                printf("[HARDWARE] OpenGL support detected\n");
            }
            XCloseDisplay(gl_display);
            gl_display = NULL;
        }
    }
    
    // Test Framebuffer
    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd >= 0) {
        caps->has_framebuffer = 1;
        printf("[HARDWARE] Framebuffer support detected\n");
        close(fb_fd);
    } else {
        caps->has_framebuffer = 0;
        printf("[HARDWARE] Framebuffer not available\n");
    }
    
    // Test DRM (Direct Rendering Manager)
    int drm_fd = open("/dev/dri/card0", O_RDWR);
    if (drm_fd >= 0) {
        caps->has_drm = 1;
        printf("[HARDWARE] DRM support detected\n");
        close(drm_fd);
    } else {
        caps->has_drm = 0;
        printf("[HARDWARE] DRM not available\n");
    }
    
    printf("[HARDWARE] Detection complete\n");
    return VISUALMEM_V2_SUCCESS;
}

// === SÉLECTION BACKEND OPTIMISÉE ===

visualmem_v2_backend_t visualmem_v2_select_best_backend(const visualmem_v2_hardware_caps_t* caps) {
    if (!caps) {
        printf("[ERROR] Null caps pointer in select_best_backend\n");
        return VISUALMEM_V2_BACKEND_AUTO;
    }
    
    // Priorité : OpenGL > X11 > Framebuffer > DRM
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
    
    printf("[HARDWARE] No suitable backend found, using AUTO\n");
    return VISUALMEM_V2_BACKEND_AUTO;
}

// === BACKEND X11 CORRIGÉ ===

int visualmem_v2_init_x11_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) {
        printf("[ERROR] Null context in init_x11_backend\n");
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    printf("[X11] Initializing X11 backend...\n");
    
    // Initialisation sécurisée de la structure X11
    memset(&ctx->x11, 0, sizeof(visualmem_v2_x11_context_t));
    
    // Connexion au serveur X11
    ctx->x11.display = XOpenDisplay(NULL);
    if (!ctx->x11.display) {
        printf("[ERROR] Cannot connect to X11 display\n");
        return VISUALMEM_V2_ERROR_HARDWARE_INIT;
    }
    
    printf("[X11] Connected to X11 display\n");
    
    // Configuration de l'écran
    ctx->x11.screen = DefaultScreen(ctx->x11.display);
    ctx->x11.depth = DefaultDepth(ctx->x11.display, ctx->x11.screen);
    ctx->x11.root_window = RootWindow(ctx->x11.display, ctx->x11.screen);
    
    printf("[X11] Screen: %d, Depth: %d bits\n", ctx->x11.screen, ctx->x11.depth);
    
    // Création de la fenêtre avec gestion d'erreur
    XSetWindowAttributes window_attrs;
    memset(&window_attrs, 0, sizeof(window_attrs));
    window_attrs.background_pixel = BlackPixel(ctx->x11.display, ctx->x11.screen);
    
    ctx->x11.window = XCreateWindow(
        ctx->x11.display, ctx->x11.root_window,
        0, 0, ctx->width, ctx->height,
        0, ctx->x11.depth, InputOutput,
        DefaultVisual(ctx->x11.display, ctx->x11.screen),
        CWBackPixel, &window_attrs
    );
    
    if (!ctx->x11.window) {
        printf("[ERROR] Failed to create X11 window\n");
        XCloseDisplay(ctx->x11.display);
        ctx->x11.display = NULL;
        return VISUALMEM_V2_ERROR_HARDWARE_INIT;
    }
    
    // Configuration et affichage de la fenêtre
    XStoreName(ctx->x11.display, ctx->x11.window, "LibVisualMem v2.0");
    XMapWindow(ctx->x11.display, ctx->x11.window);
    XFlush(ctx->x11.display);
    
    printf("[X11] Window mapped and visible\n");
    
    // Création de l'image avec vérification
    ctx->x11.image = XCreateImage(
        ctx->x11.display,
        DefaultVisual(ctx->x11.display, ctx->x11.screen),
        ctx->x11.depth,
        ZPixmap, 0, NULL,
        ctx->width, ctx->height,
        32, 0
    );
    
    if (!ctx->x11.image) {
        printf("[ERROR] Failed to create X11 image\n");
        XDestroyWindow(ctx->x11.display, ctx->x11.window);
        XCloseDisplay(ctx->x11.display);
        ctx->x11.display = NULL;
        ctx->x11.window = 0;
        return VISUALMEM_V2_ERROR_HARDWARE_INIT;
    }
    
    // Allocation du buffer image avec vérification
    size_t image_size = ctx->width * ctx->height * 4; // 32 bits par pixel
    ctx->x11.image->data = (char*)calloc(1, image_size);
    if (!ctx->x11.image->data) {
        printf("[ERROR] Failed to allocate image buffer\n");
        XDestroyImage(ctx->x11.image);
        XDestroyWindow(ctx->x11.display, ctx->x11.window);
        XCloseDisplay(ctx->x11.display);
        ctx->x11.display = NULL;
        ctx->x11.window = 0;
        ctx->x11.image = NULL;
        return VISUALMEM_V2_ERROR_MEMORY;
    }
    
    // Configuration du contexte graphique
    ctx->x11.gc = XCreateGC(ctx->x11.display, ctx->x11.window, 0, NULL);
    if (!ctx->x11.gc) {
        printf("[ERROR] Failed to create graphics context\n");
        free(ctx->x11.image->data);
        XDestroyImage(ctx->x11.image);
        XDestroyWindow(ctx->x11.display, ctx->x11.window);
        XCloseDisplay(ctx->x11.display);
        ctx->x11.display = NULL;
        ctx->x11.window = 0;
        ctx->x11.image = NULL;
        return VISUALMEM_V2_ERROR_HARDWARE_INIT;
    }
    
    printf("[X11] Backend initialized successfully (%dx%d, %d-bit)\n", 
           ctx->width, ctx->height, ctx->x11.depth);
    
    return VISUALMEM_V2_SUCCESS;
}

// === OPÉRATIONS PIXEL SÉCURISÉES ===

int visualmem_v2_x11_write_pixel(visualmem_v2_context_t* ctx, int x, int y, uint32_t color) {
    // Vérifications de sécurité
    if (!ctx || !ctx->x11.display || !ctx->x11.image || !ctx->x11.image->data) {
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return VISUALMEM_V2_ERROR_INVALID_PARAMETER;
    }
    
    // Écriture sécurisée du pixel
    int offset = (y * ctx->width + x) * 4;
    if (offset >= 0 && offset < (ctx->width * ctx->height * 4 - 3)) {
        uint32_t* pixel_data = (uint32_t*)(ctx->x11.image->data + offset);
        *pixel_data = color;
        return VISUALMEM_V2_SUCCESS;
    }
    
    return VISUALMEM_V2_ERROR_INVALID_PARAMETER;
}

uint32_t visualmem_v2_x11_read_pixel(visualmem_v2_context_t* ctx, int x, int y) {
    // Vérifications de sécurité
    if (!ctx || !ctx->x11.display || !ctx->x11.image || !ctx->x11.image->data) {
        return 0;
    }
    
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height) {
        return 0;
    }
    
    // Lecture sécurisée du pixel
    int offset = (y * ctx->width + x) * 4;
    if (offset >= 0 && offset < (ctx->width * ctx->height * 4 - 3)) {
        uint32_t* pixel_data = (uint32_t*)(ctx->x11.image->data + offset);
        return *pixel_data;
    }
    
    return 0;
}

// === REFRESH DISPLAY SÉCURISÉ ===

int visualmem_v2_x11_refresh(visualmem_v2_context_t* ctx) {
    // Vérifications de sécurité
    if (!ctx || !ctx->x11.display || !ctx->x11.window || !ctx->x11.image || !ctx->x11.gc) {
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    // Affichage de l'image avec gestion d'erreur
    int result = XPutImage(
        ctx->x11.display, ctx->x11.window, ctx->x11.gc,
        ctx->x11.image, 0, 0, 0, 0,
        ctx->width, ctx->height
    );
    
    if (result == BadMatch || result == BadDrawable || result == BadGC) {
        printf("[ERROR] XPutImage failed with error: %d\n", result);
        return VISUALMEM_V2_ERROR_HARDWARE_OPERATION;
    }
    
    XFlush(ctx->x11.display);
    return VISUALMEM_V2_SUCCESS;
}

// === CLEANUP SÉCURISÉ (CORRECTION PRINCIPALE) ===

int visualmem_v2_cleanup_x11_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) {
        printf("[ERROR] Null context in cleanup_x11_backend\n");
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    printf("[X11] Cleaning up X11 backend...\n");
    
    // Nettoyage dans l'ordre inverse de création avec vérifications
    
    // 1. Graphics Context
    if (ctx->x11.gc && ctx->x11.display) {
        XFreeGC(ctx->x11.display, ctx->x11.gc);
        ctx->x11.gc = NULL;
        printf("[X11] Graphics context freed\n");
    }
    
    // 2. Image data et XImage
    if (ctx->x11.image) {
        if (ctx->x11.image->data) {
            free(ctx->x11.image->data);
            ctx->x11.image->data = NULL;
            printf("[X11] Image data freed\n");
        }
        // Note: XDestroyImage libère aussi image->data, mais on l'a déjà fait
        ctx->x11.image->data = NULL; // Évite double free
        XDestroyImage(ctx->x11.image);
        ctx->x11.image = NULL;
        printf("[X11] XImage destroyed\n");
    }
    
    // 3. Window
    if (ctx->x11.window && ctx->x11.display) {
        XDestroyWindow(ctx->x11.display, ctx->x11.window);
        ctx->x11.window = 0;
        printf("[X11] Window destroyed\n");
    }
    
    // 4. Display (toujours en dernier)
    if (ctx->x11.display) {
        XCloseDisplay(ctx->x11.display);
        ctx->x11.display = NULL;
        printf("[X11] Display connection closed\n");
    }
    
    // Réinitialisation complète de la structure
    memset(&ctx->x11, 0, sizeof(visualmem_v2_x11_context_t));
    
    printf("[X11] Cleanup completed successfully\n");
    return VISUALMEM_V2_SUCCESS;
}

// === BACKENDS FRAMEBUFFER (INCHANGÉS MAIS SÉCURISÉS) ===

int visualmem_v2_init_framebuffer_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) {
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    printf("[FB] Initializing framebuffer backend...\n");
    
    // Tentative d'ouverture du framebuffer
    ctx->framebuffer.fd = open("/dev/fb0", O_RDWR);
    if (ctx->framebuffer.fd < 0) {
        printf("[FB] Cannot open framebuffer device\n");
        return VISUALMEM_V2_ERROR_HARDWARE_INIT;
    }
    
    // Configuration du framebuffer (implémentation basique)
    struct fb_var_screeninfo vinfo;
    if (ioctl(ctx->framebuffer.fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        printf("[FB] Cannot get framebuffer info\n");
        close(ctx->framebuffer.fd);
        ctx->framebuffer.fd = -1;
        return VISUALMEM_V2_ERROR_HARDWARE_INIT;
    }
    
    ctx->framebuffer.width = vinfo.xres;
    ctx->framebuffer.height = vinfo.yres;
    ctx->framebuffer.bpp = vinfo.bits_per_pixel;
    
    // Mapping mémoire
    size_t screen_size = ctx->framebuffer.width * ctx->framebuffer.height * (ctx->framebuffer.bpp / 8);
    ctx->framebuffer.buffer = (uint8_t*)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->framebuffer.fd, 0);
    
    if (ctx->framebuffer.buffer == MAP_FAILED) {
        printf("[FB] Cannot map framebuffer memory\n");
        close(ctx->framebuffer.fd);
        ctx->framebuffer.fd = -1;
        return VISUALMEM_V2_ERROR_MEMORY;
    }
    
    printf("[FB] Framebuffer initialized (%dx%d, %d bpp)\n", 
           ctx->framebuffer.width, ctx->framebuffer.height, ctx->framebuffer.bpp);
    
    return VISUALMEM_V2_SUCCESS;
}

int visualmem_v2_cleanup_framebuffer_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) {
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    printf("[FB] Cleaning up framebuffer backend...\n");
    
    if (ctx->framebuffer.buffer && ctx->framebuffer.buffer != MAP_FAILED) {
        size_t screen_size = ctx->framebuffer.width * ctx->framebuffer.height * (ctx->framebuffer.bpp / 8);
        munmap(ctx->framebuffer.buffer, screen_size);
        ctx->framebuffer.buffer = NULL;
    }
    
    if (ctx->framebuffer.fd >= 0) {
        close(ctx->framebuffer.fd);
        ctx->framebuffer.fd = -1;
    }
    
    memset(&ctx->framebuffer, 0, sizeof(visualmem_v2_framebuffer_context_t));
    printf("[FB] Cleanup completed\n");
    
    return VISUALMEM_V2_SUCCESS;
}

// === INITIALISATION/CLEANUP HARDWARE PRINCIPAL ===

int visualmem_v2_init_hardware_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) {
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    printf("[HARDWARE] Initializing backend: %d\n", ctx->backend);
    
    switch (ctx->backend) {
        case VISUALMEM_V2_BACKEND_X11:
        case VISUALMEM_V2_BACKEND_OPENGL:
            return visualmem_v2_init_x11_backend(ctx);
            
        case VISUALMEM_V2_BACKEND_FRAMEBUFFER:
            return visualmem_v2_init_framebuffer_backend(ctx);
            
        case VISUALMEM_V2_BACKEND_DRM:
            printf("[HARDWARE] DRM backend not implemented yet\n");
            return VISUALMEM_V2_ERROR_NOT_IMPLEMENTED;
            
        default:
            printf("[ERROR] Unknown backend: %d\n", ctx->backend);
            return VISUALMEM_V2_ERROR_INVALID_PARAMETER;
    }
}

int visualmem_v2_cleanup_hardware_backend(visualmem_v2_context_t* ctx) {
    if (!ctx) {
        return VISUALMEM_V2_ERROR_INVALID_CONTEXT;
    }
    
    printf("[HARDWARE] Cleaning up backend: %d\n", ctx->backend);
    
    switch (ctx->backend) {
        case VISUALMEM_V2_BACKEND_X11:
        case VISUALMEM_V2_BACKEND_OPENGL:
            return visualmem_v2_cleanup_x11_backend(ctx);
            
        case VISUALMEM_V2_BACKEND_FRAMEBUFFER:
            return visualmem_v2_cleanup_framebuffer_backend(ctx);
            
        case VISUALMEM_V2_BACKEND_DRM:
            printf("[HARDWARE] DRM backend cleanup not implemented\n");
            return VISUALMEM_V2_SUCCESS;
            
        default:
            printf("[WARNING] Unknown backend for cleanup: %d\n", ctx->backend);
            return VISUALMEM_V2_SUCCESS;
    }
}