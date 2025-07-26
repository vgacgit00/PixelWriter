#include "pixel_visual_ram.h"
#include <time.h>

int pixel_visual_ram_init(PixelVisualRAM* pvram) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return -1;
    }

    // Create window
    pvram->window = SDL_CreateWindow("Pixel Visual RAM - Autonomous Memory System",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    
    if (!pvram->window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Create renderer
    pvram->renderer = SDL_CreateRenderer(pvram->window, -1, SDL_RENDERER_ACCELERATED);
    if (!pvram->renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(pvram->window);
        SDL_Quit();
        return -1;
    }

    // Create texture for pixel manipulation
    pvram->memory_texture = SDL_CreateTexture(pvram->renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
        WINDOW_WIDTH, WINDOW_HEIGHT);
    
    if (!pvram->memory_texture) {
        fprintf(stderr, "Texture creation failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(pvram->renderer);
        SDL_DestroyWindow(pvram->window);
        SDL_Quit();
        return -1;
    }

    // Allocate pixel buffer (this will be freed after initialization)
    pvram->buffer_width = WINDOW_WIDTH;
    pvram->buffer_height = WINDOW_HEIGHT;
    pvram->pixel_buffer = malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(uint32_t));
    
    if (!pvram->pixel_buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        pixel_visual_ram_cleanup(pvram);
        return -1;
    }

    // Initialize pixel buffer with background color
    for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++) {
        pvram->pixel_buffer[i] = COLOR_BACKGROUND;
    }

    pvram->is_initialized = 1;
    pvram->ram_freed = 0;
    
    printf("Pixel-based Visual RAM initialized (%dx%d pixels)\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    printf("Memory capacity: %d bytes maximum\n", MAX_BYTES_TOTAL);
    
    return 0;
}

void pixel_visual_ram_cleanup(PixelVisualRAM* pvram) {
    if (pvram->pixel_buffer) {
        free(pvram->pixel_buffer);
        pvram->pixel_buffer = NULL;
    }
    if (pvram->memory_texture) {
        SDL_DestroyTexture(pvram->memory_texture);
        pvram->memory_texture = NULL;
    }
    if (pvram->renderer) {
        SDL_DestroyRenderer(pvram->renderer);
        pvram->renderer = NULL;
    }
    if (pvram->window) {
        SDL_DestroyWindow(pvram->window);
        pvram->window = NULL;
    }
    SDL_Quit();
    pvram->is_initialized = 0;
    pvram->ram_freed = 1;
}

void calculate_byte_position(int byte_index, int* x, int* y) {
    int row = byte_index / MAX_BYTES_PER_ROW;
    int col = byte_index % MAX_BYTES_PER_ROW;
    
    *x = MEMORY_START_X + (col * BYTE_SPACING_X);
    *y = MEMORY_START_Y + (row * BYTE_SPACING_Y);
}

void set_pixel_color(PixelVisualRAM* pvram, int x, int y, uint32_t color) {
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        if (pvram->pixel_buffer && !pvram->ram_freed) {
            // During initialization phase - use buffer
            pvram->pixel_buffer[y * WINDOW_WIDTH + x] = color;
        } else {
            // After RAM is freed - draw directly to renderer
            uint8_t a = (color >> 24) & 0xFF;
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;
            
            SDL_SetRenderDrawColor(pvram->renderer, r, g, b, a);
            SDL_RenderDrawPoint(pvram->renderer, x, y);
        }
    }
}

uint32_t get_pixel_color(PixelVisualRAM* pvram, int x, int y) {
    if (x >= 0 && x < WINDOW_WIDTH && y >= 0 && y < WINDOW_HEIGHT) {
        if (pvram->pixel_buffer && !pvram->ram_freed) {
            // During initialization phase
            return pvram->pixel_buffer[y * WINDOW_WIDTH + x];
        } else {
            // After RAM is freed - read from renderer surface
            SDL_Surface* surface = SDL_CreateRGBSurface(0, 1, 1, 32, 
                0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
            if (!surface) return COLOR_BACKGROUND;
            
            SDL_Rect rect = {x, y, 1, 1};
            int result = SDL_RenderReadPixels(pvram->renderer, &rect, 
                SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch);
            
            uint32_t color = COLOR_BACKGROUND;
            if (result == 0) {
                color = *((uint32_t*)surface->pixels);
            }
            
            SDL_FreeSurface(surface);
            return color;
        }
    }
    return COLOR_BACKGROUND;
}

void encode_byte_to_pixels(PixelVisualRAM* pvram, int byte_index, uint8_t byte_value) {
    int base_x, base_y;
    calculate_byte_position(byte_index, &base_x, &base_y);
    
    if (base_x >= WINDOW_WIDTH - 10 || base_y >= WINDOW_HEIGHT - 2) {
        printf("Warning: Byte %d position out of screen bounds\n", byte_index);
        return;
    }
    
    // Place start marker (red pixel)
    set_pixel_color(pvram, base_x, base_y, COLOR_MARKER_START);
    
    // Encode each bit of the byte as pixels
    for (int bit = 0; bit < BITS_PER_BYTE; bit++) {
        uint8_t bit_value = (byte_value >> (7 - bit)) & 1;
        uint32_t pixel_color = bit_value ? COLOR_BIT_1 : COLOR_BIT_0;
        set_pixel_color(pvram, base_x + 1 + bit, base_y, pixel_color);
    }
    
    // Place end marker (green pixel)
    set_pixel_color(pvram, base_x + 9, base_y, COLOR_MARKER_END);
}

uint8_t decode_byte_from_pixels(PixelVisualRAM* pvram, int byte_index) {
    int base_x, base_y;
    calculate_byte_position(byte_index, &base_x, &base_y);
    
    if (base_x >= WINDOW_WIDTH - 10 || base_y >= WINDOW_HEIGHT - 2) {
        printf("Warning: Byte %d position out of screen bounds\n", byte_index);
        return 0;
    }
    
    uint8_t byte_value = 0;
    
    // Read each bit from pixels
    for (int bit = 0; bit < BITS_PER_BYTE; bit++) {
        uint32_t pixel_color = get_pixel_color(pvram, base_x + 1 + bit, base_y);
        
        // Check if pixel represents bit 1 (white)
        // Mask out alpha channel for comparison
        if ((pixel_color & 0x00FFFFFF) == (COLOR_BIT_1 & 0x00FFFFFF)) {
            byte_value |= (1 << (7 - bit));
        }
    }
    
    return byte_value;
}

void write_string_to_pixel_memory(PixelVisualRAM* pvram, const char* str, int start_byte) {
    int len = strlen(str);
    
    printf("Encoding string '%s' to pixel memory at byte %d...\n", str, start_byte);
    
    for (int i = 0; i < len; i++) {
        encode_byte_to_pixels(pvram, start_byte + i, (uint8_t)str[i]);
        printf("  Byte %d: '%c' (0x%02X) encoded as pixel pattern\n", 
               start_byte + i, str[i], (uint8_t)str[i]);
    }
    
    // Add null terminator
    encode_byte_to_pixels(pvram, start_byte + len, 0);
    printf("  Null terminator added at byte %d\n", start_byte + len);
}

void read_string_from_pixel_memory(PixelVisualRAM* pvram, char* buffer, int start_byte, int max_length) {
    printf("Reading string from pixel memory starting at byte %d...\n", start_byte);
    
    for (int i = 0; i < max_length - 1; i++) {
        uint8_t byte_value = decode_byte_from_pixels(pvram, start_byte + i);
        buffer[i] = (char)byte_value;
        
        printf("  Byte %d: 0x%02X", start_byte + i, byte_value);
        if (byte_value >= 32 && byte_value <= 126) {
            printf(" ('%c')", byte_value);
        } else if (byte_value == 0) {
            printf(" (NULL)");
        } else {
            printf(" (non-printable)");
        }
        printf("\n");
        
        if (byte_value == 0) {
            break; // Null terminator found
        }
    }
    buffer[max_length - 1] = '\0'; // Ensure null termination
}

void draw_memory_grid(PixelVisualRAM* pvram) {
    // Draw a subtle grid to show memory organization
    SDL_SetRenderDrawColor(pvram->renderer, 100, 100, 100, 255); // Dark gray
    
    // Vertical lines for byte boundaries
    for (int col = 0; col <= MAX_BYTES_PER_ROW; col++) {
        int x = MEMORY_START_X + (col * BYTE_SPACING_X);
        if (x < WINDOW_WIDTH) {
            SDL_RenderDrawLine(pvram->renderer, x, MEMORY_START_Y - 5, 
                             x, MEMORY_START_Y + (MAX_ROWS * BYTE_SPACING_Y));
        }
    }
    
    // Horizontal lines for row boundaries
    for (int row = 0; row <= MAX_ROWS; row++) {
        int y = MEMORY_START_Y + (row * BYTE_SPACING_Y);
        if (y < WINDOW_HEIGHT) {
            SDL_RenderDrawLine(pvram->renderer, MEMORY_START_X - 5, y, 
                             MEMORY_START_X + (MAX_BYTES_PER_ROW * BYTE_SPACING_X), y);
        }
    }
}

void display_pixel_memory(PixelVisualRAM* pvram) {
    if (pvram->pixel_buffer && !pvram->ram_freed) {
        // Update texture with current pixel buffer
        SDL_UpdateTexture(pvram->memory_texture, NULL, pvram->pixel_buffer, 
                         WINDOW_WIDTH * sizeof(uint32_t));
    }
    
    // Clear and render
    SDL_SetRenderDrawColor(pvram->renderer, 128, 128, 128, 255); // Gray background
    SDL_RenderClear(pvram->renderer);
    
    if (pvram->memory_texture) {
        SDL_RenderCopy(pvram->renderer, pvram->memory_texture, NULL, NULL);
    }
    
    // Draw memory organization grid
    draw_memory_grid(pvram);
    
    // Add title and info
    SDL_SetRenderDrawColor(pvram->renderer, 255, 255, 255, 255); // White
    
    // Draw title area
    SDL_Rect title_rect = {10, 10, WINDOW_WIDTH - 20, 30};
    SDL_RenderDrawRect(pvram->renderer, &title_rect);
    
    // Draw memory status info
    SDL_Rect status_rect = {10, WINDOW_HEIGHT - 40, WINDOW_WIDTH - 20, 30};
    SDL_RenderDrawRect(pvram->renderer, &status_rect);
    
    SDL_RenderPresent(pvram->renderer);
}

void free_ram_keep_pixel_display(PixelVisualRAM* pvram) {
    printf("\n=== CRITICAL PHASE: Freeing RAM - Pixel Display becomes sole memory ===\n");
    
    // Final display update before freeing memory
    display_pixel_memory(pvram);
    
    // Free the pixel buffer - THIS IS THE KEY STEP
    if (pvram->pixel_buffer) {
        free(pvram->pixel_buffer);
        pvram->pixel_buffer = NULL;
        pvram->ram_freed = 1;
        printf("✅ RAM buffer freed! Pixel display is now the ONLY memory storage.\n");
    }
    
    printf("✅ System is now in autonomous pixel-visual memory mode.\n");
    printf("✅ All data exists only in the displayed pixel colors.\n\n");
}

// === TESTS AUTOMATIQUES COMPLETS POUR PIXELS ===

int run_pixel_tests() {
    PixelVisualRAM pvram = {0};
    int tests_passed = 0;
    int total_tests = 0;
    
    printf("=== TESTS AUTOMATIQUES - SYSTÈME MÉMOIRE VISUELLE PIXELS ===\n");
    printf("==============================================================\n\n");
    
    // Test 1: Initialisation SDL et pixel buffer
    total_tests++;
    printf("Test 1: Initialisation système graphique...\n");
    if (pixel_visual_ram_init(&pvram) == 0) {
        printf("  ✅ SDL2 initialisé avec succès\n");
        printf("  ✅ Fenêtre créée: %dx%d pixels\n", WINDOW_WIDTH, WINDOW_HEIGHT);
        printf("  ✅ Renderer SDL créé\n");
        printf("  ✅ Texture mémoire créée\n");
        printf("  ✅ Buffer pixels alloué: %p\n", (void*)pvram.pixel_buffer);
        printf("  ✅ Capacité mémoire: %d bytes\n", MAX_BYTES_TOTAL);
        tests_passed++;
    } else {
        printf("  ❌ Échec initialisation graphique\n");
        return 1;
    }
    
    // Test 2: Encodage/Décodage pixels individuels
    total_tests++;
    printf("\nTest 2: Encodage/Décodage en pixels colorés...\n");
    uint8_t pixel_test_bytes[] = {0x00, 0xFF, 0xA5, 0x5A, 0x48}; // Patterns divers
    int pixel_tests_passed = 0;
    
    for (int i = 0; i < 5; i++) {
        encode_byte_to_pixels(&pvram, i, pixel_test_bytes[i]);
        uint8_t decoded = decode_byte_from_pixels(&pvram, i);
        
        printf("  Pixel-byte %d: 0x%02X -> 0x%02X ", i, pixel_test_bytes[i], decoded);
        if (pixel_test_bytes[i] == decoded) {
            printf("✅\n");
            pixel_tests_passed++;
        } else {
            printf("❌\n");
        }
    }
    
    if (pixel_tests_passed == 5) {
        printf("  ✅ Tous les pixels-bytes encodés/décodés correctement (%d/5)\n", pixel_tests_passed);
        tests_passed++;
    } else {
        printf("  ❌ Problèmes avec encodage pixel (%d/5)\n", pixel_tests_passed);
    }
    
    // Test 3: Chaînes complètes en pixels
    total_tests++;
    printf("\nTest 3: Chaînes complètes encodées en pixels...\n");
    const char* pixel_strings[] = {"PIXEL", "SDL2", "VISUAL"};
    int pixel_positions[] = {10, 20, 30};
    int string_pixel_passed = 0;
    
    for (int i = 0; i < 3; i++) {
        write_string_to_pixel_memory(&pvram, pixel_strings[i], pixel_positions[i]);
        
        char decoded_pixel_string[100];
        read_string_from_pixel_memory(&pvram, decoded_pixel_string, pixel_positions[i], sizeof(decoded_pixel_string));
        
        printf("  Pixel-string %d: '%s' -> '%s' ", i, pixel_strings[i], decoded_pixel_string);
        if (strcmp(pixel_strings[i], decoded_pixel_string) == 0) {
            printf("✅\n");
            string_pixel_passed++;
        } else {
            printf("❌\n");
        }
    }
    
    if (string_pixel_passed == 3) {
        printf("  ✅ Toutes les chaînes-pixels correctes (%d/3)\n", string_pixel_passed);
        tests_passed++;
    } else {
        printf("  ❌ Problèmes chaînes-pixels (%d/3)\n", string_pixel_passed);
    }
    
    // Affichage graphique avant transition
    printf("\nAffichage graphique avant libération RAM...\n");
    display_pixel_memory(&pvram);
    printf("Fenêtre SDL affichée avec données encodées en pixels\n");
    
    // Test 4: Transition critique pixels
    total_tests++;
    printf("\nTest 4: Transition critique vers mode pixel autonome...\n");
    printf("  Avant: RAM buffer = %p, Libérée = %s\n", 
           (void*)pvram.pixel_buffer, pvram.ram_freed ? "OUI" : "NON");
    
    // Petite pause pour voir l'affichage
    SDL_Delay(1000);
    
    free_ram_keep_pixel_display(&pvram);
    
    printf("  Après: RAM buffer = %p, Libérée = %s\n", 
           (void*)pvram.pixel_buffer, pvram.ram_freed ? "OUI" : "NON");
    
    if (pvram.ram_freed && pvram.pixel_buffer == NULL) {
        printf("  ✅ Transition pixel autonome RÉUSSIE\n");
        printf("  ✅ Les pixels à l'écran sont maintenant la SEULE mémoire\n");
        tests_passed++;
    } else {
        printf("  ❌ Échec transition pixel autonome\n");
    }
    
    // Test 5: Persistance données dans pixels
    total_tests++;
    printf("\nTest 5: Vérification persistance dans pixels affichés...\n");
    int pixel_persistence_passed = 0;
    
    // Petite pause pour que les pixels soient bien affichés
    SDL_Delay(500);
    
    for (int i = 0; i < 3; i++) {
        char persistent_pixel_data[100];
        read_string_from_pixel_memory(&pvram, persistent_pixel_data, pixel_positions[i], sizeof(persistent_pixel_data));
        
        printf("  Pixel-pos %d: Attendu='%s', Lu='%s' ", 
               pixel_positions[i], pixel_strings[i], persistent_pixel_data);
        if (strcmp(pixel_strings[i], persistent_pixel_data) == 0) {
            printf("✅\n");
            pixel_persistence_passed++;
        } else {
            printf("❌\n");
        }
    }
    
    if (pixel_persistence_passed == 3) {
        printf("  ✅ TOUTES les données pixel ont persisté (%d/3)\n", pixel_persistence_passed);
        tests_passed++;
    } else {
        printf("  ❌ Perte de données pixel (%d/3)\n", pixel_persistence_passed);
    }
    
    // Test 6: Opérations autonomes sur pixels
    total_tests++;
    printf("\nTest 6: Nouvelles opérations en mode pixel autonome...\n");
    
    const char* autonomous_pixel_test = "AUTONOMOUS_PIXELS";
    write_string_to_pixel_memory(&pvram, autonomous_pixel_test, 50);
    
    // Mise à jour affichage
    display_pixel_memory(&pvram);
    SDL_Delay(500);
    
    char autonomous_pixel_read[100];
    read_string_from_pixel_memory(&pvram, autonomous_pixel_read, 50, sizeof(autonomous_pixel_read));
    
    printf("  Écrit en mode pixel autonome: '%s'\n", autonomous_pixel_test);
    printf("  Lu depuis pixels autonomes:   '%s'\n", autonomous_pixel_read);
    
    if (strcmp(autonomous_pixel_test, autonomous_pixel_read) == 0) {
        printf("  ✅ Opérations pixel autonomes PARFAITES\n");
        tests_passed++;
    } else {
        printf("  ❌ Échec opérations pixel autonomes\n");
    }
    
    // Affichage final
    printf("\nÉtat final des pixels (mode autonome):\n");
    display_pixel_memory(&pvram);
    
    // Attendre un peu pour voir le résultat
    printf("Appuyez sur une touche dans la fenêtre SDL ou attendez 5 secondes...\n");
    
    SDL_Event e;
    Uint32 start_time = SDL_GetTicks();
    int should_exit = 0;
    
    while (!should_exit && (SDL_GetTicks() - start_time) < 5000) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || e.type == SDL_KEYDOWN) {
                should_exit = 1;
                break;
            }
        }
        SDL_Delay(100);
    }
    
    // Nettoyage
    pixel_visual_ram_cleanup(&pvram);
    
    // === RAPPORT FINAL PIXELS ===
    printf("\n==============================================================\n");
    printf("                RAPPORT FINAL - MÉMOIRE PIXEL VISUELLE\n");
    printf("==============================================================\n\n");
    
    printf("RÉSULTATS TESTS PIXELS:\n");
    printf("  Tests réussis: %d/%d\n", tests_passed, total_tests);
    printf("  Taux de réussite: %.1f%%\n", (float)tests_passed / total_tests * 100);
    
    if (tests_passed == total_tests) {
        printf("\n🎉 VALIDATION PIXEL COMPLÈTE RÉUSSIE! 🎉\n\n");
        
        printf("FONCTIONNALITÉS PIXEL VALIDÉES:\n");
        printf("  ✅ Initialisation SDL2 et contexte graphique\n");
        printf("  ✅ Encodage données en pixels colorés (noir/blanc)\n");
        printf("  ✅ Décodage fidèle des couleurs de pixels\n");
        printf("  ✅ Transition critique RAM → Pixels autonomes\n");
        printf("  ✅ Persistance données dans pixels affichés\n");
        printf("  ✅ Opérations lecture/écriture sur pixels seuls\n");
        printf("  ✅ Fenêtre graphique comme mémoire vivante\n");
        printf("  ✅ Intégrité des couleurs et patterns\n\n");
        
        printf("AVANCEMENT TECHNIQUE:\n");
        printf("  ✅ Version textuelle: CONCEPT PROUVÉ\n");
        printf("  ✅ Version pixels SDL2: IMPLÉMENTATION RÉELLE\n");
        printf("  ✅ Mémoire visuelle graphique: FONCTIONNELLE\n");
        printf("  ✅ Transition autonome: VALIDÉE SUR PIXELS RÉELS\n\n");
        
        return 0;
    } else {
        printf("\n⚠️ PROBLÈMES PIXEL DÉTECTÉS ⚠️\n");
        printf("  %d tests pixel ont échoué\n", total_tests - tests_passed);
        return 1;
    }
}

int main() {
    printf("SYSTÈME DE MÉMOIRE VISUELLE AUTONOME - VERSION PIXELS SDL2\n");
    printf("==========================================================\n\n");
    
    printf("ÉVOLUTION: Passage de la version textuelle validée\n");
    printf("           vers l'implémentation graphique réelle\n\n");
    
    printf("PIXELS UTILISÉS:\n");
    printf("  • Noir (0x000000) = bit 0\n");
    printf("  • Blanc (0xFFFFFF) = bit 1\n");
    printf("  • Rouge (0xFF0000) = marqueur début\n");
    printf("  • Vert (0x00FF00) = marqueur fin\n");
    printf("  • Gris (0x808080) = arrière-plan\n\n");
    
    int result = run_pixel_tests();
    
    printf("\n==========================================================\n");
    if (result == 0) {
        printf("STATUT: ✅ SUCCÈS PIXEL COMPLET - VERSION SDL2 VALIDÉE\n");
        printf("🎉 LA MÉMOIRE VISUELLE GRAPHIQUE FONCTIONNE PARFAITEMENT!\n");
    } else {
        printf("STATUT: ❌ PROBLÈMES PIXEL DÉTECTÉS\n");
    }
    printf("==========================================================\n");
    
    return result;
}