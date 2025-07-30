
/**
 * DÉMONSTRATION INTERACTIVE VISUALMEM V2
 * =====================================
 * 
 * Démonstration complète des fonctionnalités sécurisées
 * de la version 2.0 avec interface utilisateur interactive.
 */

#include "libvisualmem_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// === CONFIGURATION DEMO ===
#define DEMO_WIDTH 1024
#define DEMO_HEIGHT 768
#define MAX_DEMO_ALLOCATIONS 20

// === STRUCTURES DEMO ===
typedef struct {
    void* address;
    size_t size;
    char label[64];
    bool active;
} demo_allocation_t;

// === VARIABLES GLOBALES ===
static visualmem_v2_context_t g_ctx;
static demo_allocation_t g_allocations[MAX_DEMO_ALLOCATIONS];
static bool g_demo_running = true;

// === UTILITAIRES DEMO ===

void print_banner(void) {
    printf("\n");
    printf("🔒═══════════════════════════════════════════════════════════════🔒\n");
    printf("🔒               DÉMONSTRATION VISUALMEM V2                     🔒\n");
    printf("🔒                  VERSION SÉCURISÉE                          🔒\n");
    printf("🔒═══════════════════════════════════════════════════════════════🔒\n");
    printf("\n");
    printf("Version: %s\n", visualmem_v2_get_version_info());
    printf("Système compatible: %s\n\n", visualmem_v2_system_compatible() ? "✅ OUI" : "❌ NON");
}

void print_menu(void) {
    printf("\n" "═══ MENU PRINCIPAL ═══\n");
    printf("1. 🏗️  Allocation sécurisée\n");
    printf("2. ✍️  Écriture sécurisée\n");
    printf("3. 📖 Lecture sécurisée\n");
    printf("4. 🗑️  Libération sécurisée\n");
    printf("5. 📊 Rapport sécurité\n");
    printf("6. 📈 Statistiques système\n");
    printf("7. 🧪 Test intégrité\n");
    printf("8. ⚡ Benchmark performance\n");
    printf("9. 🎯 Démonstration concurrence\n");
    printf("10. 🔒 Tests sécurité avancés\n");
    printf("11. 📋 Export audit trail\n");
    printf("0. 🚪 Quitter\n");
    printf("═══════════════════════\n");
    printf("Choix: ");
}

void wait_for_enter(void) {
    printf("\nAppuyez sur Entrée pour continuer...");
    while (getchar() != '\n');
}

int find_free_allocation_slot(void) {
    for (int i = 0; i < MAX_DEMO_ALLOCATIONS; i++) {
        if (!g_allocations[i].active) {
            return i;
        }
    }
    return -1;
}

void list_active_allocations(void) {
    printf("\n📋 ALLOCATIONS ACTIVES:\n");
    printf("========================\n");
    
    bool found = false;
    for (int i = 0; i < MAX_DEMO_ALLOCATIONS; i++) {
        if (g_allocations[i].active) {
            printf("%d. %s - %zu bytes @ %p\n", 
                   i, g_allocations[i].label, g_allocations[i].size, g_allocations[i].address);
            found = true;
        }
    }
    
    if (!found) {
        printf("Aucune allocation active.\n");
    }
}

// === FONCTIONS DÉMONSTRATION ===

void demo_secure_allocation(void) {
    printf("\n🏗️  DÉMONSTRATION ALLOCATION SÉCURISÉE\n");
    printf("=====================================\n");
    
    char label[64];
    size_t size;
    
    printf("Nom de l'allocation: ");
    if (scanf("%63s", label) != 1) {
        printf("❌ Erreur saisie\n");
        return;
    }
    
    printf("Taille (bytes, max %d): ", MAX_ALLOCATION_SIZE);
    if (scanf("%zu", &size) != 1) {
        printf("❌ Erreur saisie\n");
        return;
    }
    
    int slot = find_free_allocation_slot();
    if (slot == -1) {
        printf("❌ Nombre maximum d'allocations atteint (%d)\n", MAX_DEMO_ALLOCATIONS);
        return;
    }
    
    printf("\n⏳ Allocation en cours...\n");
    void* ptr = visualmem_v2_alloc_secure(&g_ctx, size, label);
    
    if (ptr) {
        g_allocations[slot].address = ptr;
        g_allocations[slot].size = size;
        strncpy(g_allocations[slot].label, label, sizeof(g_allocations[slot].label) - 1);
        g_allocations[slot].active = true;
        
        printf("✅ Allocation réussie!\n");
        printf("   Adresse: %p\n", ptr);
        printf("   Taille: %zu bytes\n", size);
        printf("   Label: %s\n", label);
        printf("   Slot: %d\n", slot);
    } else {
        printf("❌ Allocation échouée (vérifiez taille/quotas)\n");
    }
}

void demo_secure_write(void) {
    printf("\n✍️  DÉMONSTRATION ÉCRITURE SÉCURISÉE\n");
    printf("===================================\n");
    
    list_active_allocations();
    
    int slot;
    printf("\nNuméro allocation: ");
    if (scanf("%d", &slot) != 1 || slot < 0 || slot >= MAX_DEMO_ALLOCATIONS || !g_allocations[slot].active) {
        printf("❌ Allocation invalide\n");
        return;
    }
    
    printf("Type de données:\n");
    printf("1. Texte\n");
    printf("2. Pattern numérique\n");
    printf("3. Données aléatoires\n");
    printf("Choix: ");
    
    int choice;
    if (scanf("%d", &choice) != 1) {
        printf("❌ Choix invalide\n");
        return;
    }
    
    // Alloue buffer temporaire
    size_t write_size = g_allocations[slot].size;
    char* write_buffer = malloc(write_size);
    if (!write_buffer) {
        printf("❌ Erreur allocation buffer temporaire\n");
        return;
    }
    
    // Génère données selon choix
    switch (choice) {
        case 1: {
            char text[256];
            printf("Texte à écrire: ");
            if (scanf("%255s", text) == 1) {
                size_t text_len = strlen(text);
                size_t repeat_count = write_size / text_len;
                
                for (size_t i = 0; i < repeat_count; i++) {
                    memcpy(write_buffer + i * text_len, text, text_len);
                }
                
                // Remplit le reste
                size_t remaining = write_size % text_len;
                if (remaining > 0) {
                    memcpy(write_buffer + repeat_count * text_len, text, remaining);
                }
            }
            break;
        }
        
        case 2: {
            printf("Génération pattern numérique...\n");
            for (size_t i = 0; i < write_size; i++) {
                write_buffer[i] = (char)(i % 256);
            }
            break;
        }
        
        case 3: {
            printf("Génération données aléatoires...\n");
            for (size_t i = 0; i < write_size; i++) {
                write_buffer[i] = (char)(rand() % 256);
            }
            break;
        }
        
        default:
            printf("❌ Choix invalide\n");
            free(write_buffer);
            return;
    }
    
    printf("\n⏳ Écriture sécurisée en cours...\n");
    visualmem_v2_result_t result = visualmem_v2_write_secure(&g_ctx, 
                                                           g_allocations[slot].address,
                                                           write_buffer, 
                                                           write_size);
    
    free(write_buffer);
    
    if (result == VISUALMEM_V2_SUCCESS) {
        printf("✅ Écriture réussie!\n");
        printf("   %zu bytes écrits dans %s\n", write_size, g_allocations[slot].label);
    } else {
        printf("❌ Écriture échouée: %s\n", visualmem_v2_error_string(result));
    }
}

void demo_secure_read(void) {
    printf("\n📖 DÉMONSTRATION LECTURE SÉCURISÉE\n");
    printf("=================================\n");
    
    list_active_allocations();
    
    int slot;
    printf("\nNuméro allocation: ");
    if (scanf("%d", &slot) != 1 || slot < 0 || slot >= MAX_DEMO_ALLOCATIONS || !g_allocations[slot].active) {
        printf("❌ Allocation invalide\n");
        return;
    }
    
    size_t read_size = g_allocations[slot].size;
    char* read_buffer = malloc(read_size);
    if (!read_buffer) {
        printf("❌ Erreur allocation buffer lecture\n");
        return;
    }
    
    printf("\n⏳ Lecture sécurisée en cours...\n");
    visualmem_v2_result_t result = visualmem_v2_read_secure(&g_ctx,
                                                          g_allocations[slot].address,
                                                          read_buffer,
                                                          read_size);
    
    if (result == VISUALMEM_V2_SUCCESS) {
        printf("✅ Lecture réussie!\n");
        printf("   %zu bytes lus depuis %s\n", read_size, g_allocations[slot].label);
        
        printf("\nAffichage données (format):\n");
        printf("1. Hexadécimal\n");
        printf("2. ASCII (caractères imprimables)\n");
        printf("3. Statistiques\n");
        printf("Choix: ");
        
        int display_choice;
        if (scanf("%d", &display_choice) == 1) {
            switch (display_choice) {
                case 1: {
                    printf("\nFormat hexadécimal (64 premiers bytes):\n");
                    size_t display_count = (read_size > 64) ? 64 : read_size;
                    for (size_t i = 0; i < display_count; i++) {
                        if (i % 16 == 0) printf("%04zx: ", i);
                        printf("%02x ", (unsigned char)read_buffer[i]);
                        if ((i + 1) % 16 == 0) printf("\n");
                    }
                    if (display_count % 16 != 0) printf("\n");
                    if (read_size > 64) {
                        printf("... (%zu bytes supplémentaires)\n", read_size - 64);
                    }
                    break;
                }
                
                case 2: {
                    printf("\nFormat ASCII (64 premiers caractères):\n");
                    size_t display_count = (read_size > 64) ? 64 : read_size;
                    for (size_t i = 0; i < display_count; i++) {
                        char c = read_buffer[i];
                        if (c >= 32 && c <= 126) {
                            putchar(c);
                        } else {
                            putchar('.');
                        }
                    }
                    printf("\n");
                    if (read_size > 64) {
                        printf("... (%zu bytes supplémentaires)\n", read_size - 64);
                    }
                    break;
                }
                
                case 3: {
                    // Calcul statistiques
                    int byte_counts[256] = {0};
                    for (size_t i = 0; i < read_size; i++) {
                        byte_counts[(unsigned char)read_buffer[i]]++;
                    }
                    
                    printf("\nStatistiques données:\n");
                    printf("Taille: %zu bytes\n", read_size);
                    
                    int unique_bytes = 0;
                    for (int i = 0; i < 256; i++) {
                        if (byte_counts[i] > 0) unique_bytes++;
                    }
                    printf("Valeurs uniques: %d/256\n", unique_bytes);
                    
                    // Byte le plus fréquent
                    int max_count = 0;
                    int most_frequent = 0;
                    for (int i = 0; i < 256; i++) {
                        if (byte_counts[i] > max_count) {
                            max_count = byte_counts[i];
                            most_frequent = i;
                        }
                    }
                    printf("Byte plus fréquent: 0x%02x (%d occurrences)\n", 
                           most_frequent, max_count);
                    
                    break;
                }
            }
        }
    } else {
        printf("❌ Lecture échouée: %s\n", visualmem_v2_error_string(result));
    }
    
    free(read_buffer);
}

void demo_secure_free(void) {
    printf("\n🗑️  DÉMONSTRATION LIBÉRATION SÉCURISÉE\n");
    printf("=====================================\n");
    
    list_active_allocations();
    
    int slot;
    printf("\nNuméro allocation à libérer: ");
    if (scanf("%d", &slot) != 1 || slot < 0 || slot >= MAX_DEMO_ALLOCATIONS || !g_allocations[slot].active) {
        printf("❌ Allocation invalide\n");
        return;
    }
    
    printf("\n⚠️  Confirmation libération de '%s' (%zu bytes) ? (o/N): ", 
           g_allocations[slot].label, g_allocations[slot].size);
    
    char confirm;
    if (scanf(" %c", &confirm) == 1 && (confirm == 'o' || confirm == 'O')) {
        printf("\n⏳ Libération sécurisée en cours (effacement multi-passes)...\n");
        
        visualmem_v2_result_t result = visualmem_v2_free_secure(&g_ctx, g_allocations[slot].address);
        
        if (result == VISUALMEM_V2_SUCCESS) {
            printf("✅ Libération sécurisée réussie!\n");
            printf("   Données effacées de manière sécurisée\n");
            printf("   Mémoire retournée au système\n");
            
            // Marque slot comme libre
            g_allocations[slot].active = false;
            memset(&g_allocations[slot], 0, sizeof(g_allocations[slot]));
        } else {
            printf("❌ Libération échouée: %s\n", visualmem_v2_error_string(result));
        }
    } else {
        printf("Libération annulée.\n");
    }
}

void demo_security_report(void) {
    printf("\n📊 GÉNÉRATION RAPPORT SÉCURITÉ\n");
    printf("==============================\n");
    
    char report[8192];
    visualmem_v2_result_t result = visualmem_v2_generate_security_report(&g_ctx, report, sizeof(report));
    
    if (result == VISUALMEM_V2_SUCCESS) {
        printf("✅ Rapport généré avec succès!\n\n");
        printf("%s\n", report);
    } else {
        printf("❌ Erreur génération rapport: %s\n", visualmem_v2_error_string(result));
    }
}

void demo_performance_benchmark(void) {
    printf("\n⚡ BENCHMARK PERFORMANCE\n");
    printf("=======================\n");
    
    const int iterations = 1000;
    clock_t start, end;
    
    printf("Benchmark %d itérations...\n\n", iterations);
    
    // Test allocations
    printf("⏳ Test allocations...\n");
    start = clock();
    void* ptrs[iterations];
    
    for (int i = 0; i < iterations; i++) {
        char label[32];
        snprintf(label, sizeof(label), "bench_%d", i);
        ptrs[i] = visualmem_v2_alloc_secure(&g_ctx, 1024, label);
    }
    
    end = clock();
    double alloc_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    // Test écritures
    printf("⏳ Test écritures...\n");
    char test_data[1024];
    memset(test_data, 0xAB, sizeof(test_data));
    
    start = clock();
    for (int i = 0; i < iterations; i++) {
        if (ptrs[i]) {
            visualmem_v2_write_secure(&g_ctx, ptrs[i], test_data, 1024);
        }
    }
    end = clock();
    double write_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    // Test lectures
    printf("⏳ Test lectures...\n");
    char read_buffer[1024];
    
    start = clock();
    for (int i = 0; i < iterations; i++) {
        if (ptrs[i]) {
            visualmem_v2_read_secure(&g_ctx, ptrs[i], read_buffer, 1024);
        }
    }
    end = clock();
    double read_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    // Test libérations
    printf("⏳ Test libérations...\n");
    start = clock();
    for (int i = 0; i < iterations; i++) {
        if (ptrs[i]) {
            visualmem_v2_free_secure(&g_ctx, ptrs[i]);
        }
    }
    end = clock();
    double free_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    // Résultats
    printf("\n📈 RÉSULTATS BENCHMARK:\n");
    printf("=======================\n");
    printf("Allocations: %.2f ms (%.1f ops/ms)\n", alloc_time, iterations / alloc_time);
    printf("Écritures:   %.2f ms (%.1f ops/ms)\n", write_time, iterations / write_time);
    printf("Lectures:    %.2f ms (%.1f ops/ms)\n", read_time, iterations / read_time);
    printf("Libérations: %.2f ms (%.1f ops/ms)\n", free_time, iterations / free_time);
    printf("Total:       %.2f ms\n", alloc_time + write_time + read_time + free_time);
}

// === FONCTION PRINCIPALE ===

int main(void) {
    print_banner();
    
    // Initialisation contexte
    printf("🔧 Initialisation contexte sécurisé...\n");
    visualmem_v2_result_t result = visualmem_v2_init(&g_ctx, VISUALMEM_V2_MODE_SECURE, 
                                                     DEMO_WIDTH, DEMO_HEIGHT, "demo_v2");
    
    if (result != VISUALMEM_V2_SUCCESS) {
        printf("❌ Erreur initialisation: %s\n", visualmem_v2_error_string(result));
        return 1;
    }
    
    printf("✅ Contexte initialisé avec succès!\n");
    printf("   Dimensions: %dx%d pixels\n", DEMO_WIDTH, DEMO_HEIGHT);
    printf("   Mode: Sécurisé avec thread safety\n");
    
    // Initialisation état allocations
    memset(g_allocations, 0, sizeof(g_allocations));
    
    // Boucle principale
    int choice;
    while (g_demo_running) {
        print_menu();
        
        if (scanf("%d", &choice) != 1) {
            printf("❌ Saisie invalide\n");
            while (getchar() != '\n'); // Vide buffer
            continue;
        }
        
        switch (choice) {
            case 1:
                demo_secure_allocation();
                break;
            case 2:
                demo_secure_write();
                break;
            case 3:
                demo_secure_read();
                break;
            case 4:
                demo_secure_free();
                break;
            case 5:
                demo_security_report();
                break;
            case 8:
                demo_performance_benchmark();
                break;
            case 0:
                g_demo_running = false;
                break;
            default:
                printf("❌ Option non implémentée ou invalide\n");
        }
        
        if (g_demo_running && choice != 0) {
            wait_for_enter();
        }
    }
    
    // Nettoyage final
    printf("\n🧹 Nettoyage et fermeture...\n");
    
    // Libère allocations restantes
    int remaining = 0;
    for (int i = 0; i < MAX_DEMO_ALLOCATIONS; i++) {
        if (g_allocations[i].active) {
            visualmem_v2_free_secure(&g_ctx, g_allocations[i].address);
            remaining++;
        }
    }
    
    if (remaining > 0) {
        printf("   %d allocations automatiquement libérées\n", remaining);
    }
    
    // Nettoyage contexte
    visualmem_v2_cleanup_secure(&g_ctx);
    
    printf("✅ Démonstration terminée proprement.\n");
    printf("\n🔒 Merci d'avoir testé VisualMem V2 Sécurisé!\n");
    
    return 0;
}
