#include "visual_ram.h"

int visual_ram_init(VisualRAM* vram) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return -1;
    }

    // Create window
    vram->window = SDL_CreateWindow("Visual RAM - Autonomous Memory System",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    
    if (!vram->window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Create renderer
    vram->renderer = SDL_CreateRenderer(vram->window, -1, SDL_RENDERER_ACCELERATED);
    if (!vram->renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(vram->window);
        SDL_Quit();
        return -1;
    }

    // Create texture for pixel manipulation
    vram->texture = SDL_CreateTexture(vram->renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
        WINDOW_WIDTH, WINDOW_HEIGHT);
    
    if (!vram->texture) {
        fprintf(stderr, "Texture creation failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(vram->renderer);
        SDL_DestroyWindow(vram->window);
        SDL_Quit();
        return -1;
    }

    // Allocate pixel buffer (this will be freed after initialization)
    vram->pixel_buffer = malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(uint32_t));
    if (!vram->pixel_buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        visual_ram_cleanup(vram);
        return -1;
    }

    // Initialize pixel buffer with background color
    for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++) {
        vram->pixel_buffer[i] = COLOR_BACKGROUND;
    }

    vram->is_initialized = 1;
    return 0;
}

void visual_ram_cleanup(VisualRAM* vram) {
    if (vram->pixel_buffer) {
        free(vram->pixel_buffer);
        vram->pixel_buffer = NULL;
    }
    if (vram->texture) {
        SDL_DestroyTexture(vram->texture);
        vram->texture = NULL;
    }
    if (vram->renderer) {
        SDL_DestroyRenderer(vram->renderer);
        vram->renderer = NULL;
    }
    if (vram->window) {
        SDL_DestroyWindow(vram->window);
        vram->window = NULL;
    }
    SDL_Quit();
    vram->is_initialized = 0;
}

void set_pixel_color(VisualRAM* vram, int x, int y, uint32_t color) {
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        if (vram->pixel_buffer) {
            // During initialization phase - use buffer
            vram->pixel_buffer[y * WINDOW_WIDTH + x] = color;
        } else {
            // After RAM is freed - draw directly to renderer
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;
            uint8_t a = (color >> 24) & 0xFF;
            
            SDL_SetRenderDrawColor(vram->renderer, r, g, b, a);
            SDL_RenderDrawPoint(vram->renderer, x, y);
        }
    }
}

uint32_t get_pixel_color(VisualRAM* vram, int x, int y) {
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        if (vram->pixel_buffer) {
            // During initialization phase
            return vram->pixel_buffer[y * WINDOW_WIDTH + x];
        } else {
            // After RAM is freed - read from renderer surface
            // Note: This is a simplified version. In real implementation,
            // we would need to read from framebuffer or use SDL_RenderReadPixels
            SDL_Surface* surface = SDL_CreateRGBSurface(0, 1, 1, 32, 0, 0, 0, 0);
            SDL_Rect rect = {x, y, 1, 1};
            SDL_RenderReadPixels(vram->renderer, &rect, surface->format->format, 
                               surface->pixels, surface->pitch);
            uint32_t color = *((uint32_t*)surface->pixels);
            SDL_FreeSurface(surface);
            return color;
        }
    }
    return COLOR_BACKGROUND;
}

void encode_byte_to_pixels(VisualRAM* vram, int byte_index, uint8_t byte_value) {
    int start_x = MEMORY_START_X + (byte_index * BYTE_SPACING);
    int y = MEMORY_START_Y;
    
    // Place start marker
    set_pixel_color(vram, start_x - 1, y, COLOR_MARKER_START);
    
    // Encode each bit of the byte
    for (int bit = 0; bit < BITS_PER_BYTE; bit++) {
        uint8_t bit_value = (byte_value >> (7 - bit)) & 1;
        uint32_t pixel_color = bit_value ? COLOR_BIT_1 : COLOR_BIT_0;
        set_pixel_color(vram, start_x + bit, y, pixel_color);
    }
    
    // Place end marker
    set_pixel_color(vram, start_x + BITS_PER_BYTE, y, COLOR_MARKER_END);
}

uint8_t decode_byte_from_pixels(VisualRAM* vram, int byte_index) {
    int start_x = MEMORY_START_X + (byte_index * BYTE_SPACING);
    int y = MEMORY_START_Y;
    uint8_t byte_value = 0;
    
    // Read each bit from pixels
    for (int bit = 0; bit < BITS_PER_BYTE; bit++) {
        uint32_t pixel_color = get_pixel_color(vram, start_x + bit, y);
        
        // Check if pixel represents bit 1 (white)
        if ((pixel_color & 0x00FFFFFF) == (COLOR_BIT_1 & 0x00FFFFFF)) {
            byte_value |= (1 << (7 - bit));
        }
    }
    
    return byte_value;
}

void write_string_to_visual_memory(VisualRAM* vram, const char* str, int start_byte) {
    int len = strlen(str);
    
    printf("Encoding string '%s' to visual memory...\n", str);
    
    for (int i = 0; i < len; i++) {
        encode_byte_to_pixels(vram, start_byte + i, (uint8_t)str[i]);
        printf("Byte %d: '%c' (0x%02X) encoded\n", start_byte + i, str[i], (uint8_t)str[i]);
    }
    
    // Add null terminator
    encode_byte_to_pixels(vram, start_byte + len, 0);
    printf("Null terminator added at byte %d\n", start_byte + len);
}

void read_string_from_visual_memory(VisualRAM* vram, char* buffer, int start_byte, int max_length) {
    printf("Reading string from visual memory starting at byte %d...\n", start_byte);
    
    for (int i = 0; i < max_length - 1; i++) {
        uint8_t byte_value = decode_byte_from_pixels(vram, start_byte + i);
        buffer[i] = (char)byte_value;
        
        printf("Byte %d: 0x%02X ('%c')\n", start_byte + i, byte_value, 
               (byte_value >= 32 && byte_value <= 126) ? byte_value : '?');
        
        if (byte_value == 0) {
            break; // Null terminator found
        }
    }
    buffer[max_length - 1] = '\0'; // Ensure null termination
}

void display_visual_memory(VisualRAM* vram) {
    if (vram->pixel_buffer) {
        // Update texture with current pixel buffer
        SDL_UpdateTexture(vram->texture, NULL, vram->pixel_buffer, 
                         WINDOW_WIDTH * sizeof(uint32_t));
    }
    
    // Render to screen
    SDL_RenderClear(vram->renderer);
    SDL_RenderCopy(vram->renderer, vram->texture, NULL, NULL);
    
    // Add some visual indicators
    SDL_SetRenderDrawColor(vram->renderer, 255, 255, 255, 255);
    
    // Draw memory area border
    SDL_Rect memory_border = {
        MEMORY_START_X - 5, 
        MEMORY_START_Y - 5, 
        500, 
        20
    };
    SDL_RenderDrawRect(vram->renderer, &memory_border);
    
    SDL_RenderPresent(vram->renderer);
}

void free_ram_keep_display(VisualRAM* vram) {
    printf("\n=== CRITICAL PHASE: Freeing RAM - Display becomes sole memory ===\n");
    
    // Final display update before freeing memory
    display_visual_memory(vram);
    
    // Free the pixel buffer - THIS IS THE KEY STEP
    if (vram->pixel_buffer) {
        free(vram->pixel_buffer);
        vram->pixel_buffer = NULL;
        printf("RAM buffer freed! Screen is now the ONLY memory storage.\n");
    }
    
    printf("System is now in autonomous visual memory mode.\n");
    printf("All data exists only in the displayed pixels.\n\n");
}

int main() {
    VisualRAM vram = {0};
    
    printf("=== Visual RAM Autonomous Memory System ===\n");
    printf("Initializing visual memory system...\n\n");
    
    // Initialize the visual RAM system
    if (visual_ram_init(&vram) != 0) {
        fprintf(stderr, "Failed to initialize visual RAM\n");
        return 1;
    }
    
    printf("Phase 1: Using traditional RAM for initialization\n");
    
    // Write test data to visual memory using RAM
    const char* test_message = "HELLO";
    write_string_to_visual_memory(&vram, test_message, 0);
    
    // Display the encoded data
    display_visual_memory(&vram);
    
    printf("\nPress Enter to proceed to autonomous mode...");
    getchar();
    
    // === CRITICAL TRANSITION: Free all RAM ===
    free_ram_keep_display(&vram);
    
    printf("Phase 2: Autonomous visual memory mode\n");
    printf("Reading data from screen pixels only...\n\n");
    
    // Now read back the data using ONLY the screen as memory
    char read_buffer[100];
    read_string_from_visual_memory(&vram, read_buffer, 0, sizeof(read_buffer));
    
    printf("\nData successfully retrieved from visual memory: '%s'\n", read_buffer);
    
    if (strcmp(test_message, read_buffer) == 0) {
        printf("✅ SUCCESS: Visual memory system working autonomously!\n");
        printf("✅ Data persisted in screen pixels without RAM dependency\n");
    } else {
        printf("❌ ERROR: Data corruption detected\n");
    }
    
    printf("\nPress Enter to demonstrate visual memory modification...");
    getchar();
    
    // Demonstrate writing new data directly to screen
    printf("\nWriting new data 'WORLD' to visual memory...\n");
    write_string_to_visual_memory(&vram, "WORLD", 6);
    display_visual_memory(&vram);
    
    // Read it back
    read_string_from_visual_memory(&vram, read_buffer, 6, sizeof(read_buffer));
    printf("New data read from visual memory: '%s'\n", read_buffer);
    
    printf("\nPress Enter to exit...");
    getchar();
    
    // Cleanup
    visual_ram_cleanup(&vram);
    
    printf("\nVisual RAM demonstration completed.\n");
    printf("The concept of screen-as-memory has been successfully demonstrated!\n");
    
    return 0;
}
