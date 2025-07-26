#include "visual_ram_simple.h"
#include <assert.h>

// Test automatisé complet du système de mémoire visuelle
int run_automated_tests() {
    TextVisualRAM tvram = {0};
    int tests_passed = 0;
    int total_tests = 0;
    
    printf("=== TESTS AUTOMATISÉS DU SYSTÈME DE MÉMOIRE VISUELLE ===\n\n");
    
    // Test 1: Initialisation
    total_tests++;
    printf("Test 1: Initialisation du système...\n");
    if (text_visual_ram_init(&tvram) == 0) {
        printf("✅ Initialisation réussie\n");
        printf("   - RAM buffer allouée: %p\n", (void*)tvram.ram_buffer);
        printf("   - Status RAM libre: %s\n", tvram.ram_freed ? "OUI" : "NON");
        tests_passed++;
    } else {
        printf("❌ Échec de l'initialisation\n");
        return 1;
    }
    
    // Test 2: Encodage/Décodage d'un seul octet
    total_tests++;
    printf("\nTest 2: Encodage/Décodage d'un octet...\n");
    uint8_t test_byte = 0xA5; // 10100101 en binaire
    encode_byte_to_text(&tvram, 0, test_byte);
    uint8_t decoded_byte = decode_byte_from_text(&tvram, 0);
    
    printf("   - Octet original: 0x%02X (", test_byte);
    for (int i = 7; i >= 0; i--) printf("%d", (test_byte >> i) & 1);
    printf(")\n");
    printf("   - Octet décodé:   0x%02X (", decoded_byte);
    for (int i = 7; i >= 0; i--) printf("%d", (decoded_byte >> i) & 1);
    printf(")\n");
    
    if (test_byte == decoded_byte) {
        printf("✅ Intégrité des bits préservée\n");
        tests_passed++;
    } else {
        printf("❌ Corruption des bits détectée\n");
    }
    
    // Test 3: Encodage/Décodage de chaîne
    total_tests++;
    printf("\nTest 3: Encodage/Décodage de chaîne...\n");
    const char* test_string = "TEST123";
    write_string_to_text_memory(&tvram, test_string, 5);
    
    char decoded_string[100];
    read_string_from_text_memory(&tvram, decoded_string, 5, sizeof(decoded_string));
    
    printf("   - Chaîne originale: '%s'\n", test_string);
    printf("   - Chaîne décodée:   '%s'\n", decoded_string);
    
    if (strcmp(test_string, decoded_string) == 0) {
        printf("✅ Intégrité de la chaîne préservée\n");
        tests_passed++;
    } else {
        printf("❌ Corruption de la chaîne détectée\n");
    }
    
    // Test 4: Transition vers mode autonome
    total_tests++;
    printf("\nTest 4: Transition vers mode autonome (libération RAM)...\n");
    printf("   - Avant: RAM libre = %s\n", tvram.ram_freed ? "OUI" : "NON");
    free_ram_keep_text_display(&tvram);
    printf("   - Après: RAM libre = %s\n", tvram.ram_freed ? "OUI" : "NON");
    
    if (tvram.ram_freed && tvram.ram_buffer == NULL) {
        printf("✅ Transition vers mode autonome réussie\n");
        tests_passed++;
    } else {
        printf("❌ Échec de la transition autonome\n");
    }
    
    // Test 5: Opérations en mode autonome
    total_tests++;
    printf("\nTest 5: Opérations en mode autonome (sans RAM)...\n");
    
    // Écriture en mode autonome
    const char* autonomous_string = "AUTONOMOUS";
    write_string_to_text_memory(&tvram, autonomous_string, 10);
    
    // Lecture en mode autonome
    char autonomous_decoded[100];
    read_string_from_text_memory(&tvram, autonomous_decoded, 10, sizeof(autonomous_decoded));
    
    printf("   - Écrit en mode autonome: '%s'\n", autonomous_string);
    printf("   - Lu en mode autonome:    '%s'\n", autonomous_decoded);
    
    if (strcmp(autonomous_string, autonomous_decoded) == 0) {
        printf("✅ Opérations autonomes réussies\n");
        tests_passed++;
    } else {
        printf("❌ Échec des opérations autonomes\n");
    }
    
    // Test 6: Persistance des données après libération RAM
    total_tests++;
    printf("\nTest 6: Persistance des données après libération RAM...\n");
    
    // Vérifier que les données précédentes sont toujours là
    char persistent_data[100];
    read_string_from_text_memory(&tvram, persistent_data, 5, sizeof(persistent_data));
    
    printf("   - Données persistantes: '%s'\n", persistent_data);
    printf("   - Données attendues:    '%s'\n", test_string);
    
    if (strcmp(test_string, persistent_data) == 0) {
        printf("✅ Persistance des données confirmée\n");
        tests_passed++;
    } else {
        printf("❌ Perte de données détectée\n");
    }
    
    // Test 7: Multiple écritures/lectures
    total_tests++;
    printf("\nTest 7: Écritures/Lectures multiples...\n");
    
    const char* messages[] = {"A", "BB", "CCC", "DDDD"};
    int positions[] = {20, 25, 30, 35};
    int success_count = 0;
    
    // Écrire plusieurs messages
    for (int i = 0; i < 4; i++) {
        write_string_to_text_memory(&tvram, messages[i], positions[i]);
    }
    
    // Lire et vérifier
    for (int i = 0; i < 4; i++) {
        char buffer[100];
        read_string_from_text_memory(&tvram, buffer, positions[i], sizeof(buffer));
        printf("   - Position %d: '%s' -> '%s' %s\n", 
               positions[i], messages[i], buffer,
               strcmp(messages[i], buffer) == 0 ? "✅" : "❌");
        if (strcmp(messages[i], buffer) == 0) success_count++;
    }
    
    if (success_count == 4) {
        printf("✅ Toutes les écritures/lectures multiples réussies\n");
        tests_passed++;
    } else {
        printf("❌ %d/4 écritures/lectures ont échoué\n", 4 - success_count);
    }
    
    // Test 8: Limites du système
    total_tests++;
    printf("\nTest 8: Test des limites du système...\n");
    
    // Tester l'écriture à la limite de l'affichage
    uint8_t limit_byte = 0xFF;
    int limit_position = 50; // Position proche de la limite
    
    encode_byte_to_text(&tvram, limit_position, limit_byte);
    uint8_t limit_decoded = decode_byte_from_text(&tvram, limit_position);
    
    printf("   - Octet à la limite: 0x%02X -> 0x%02X\n", limit_byte, limit_decoded);
    
    if (limit_byte == limit_decoded) {
        printf("✅ Gestion des limites correcte\n");
        tests_passed++;
    } else {
        printf("❌ Problème de gestion des limites\n");
    }
    
    // Affichage final de l'état de la mémoire
    printf("\n=== ÉTAT FINAL DE LA MÉMOIRE VISUELLE ===\n");
    display_text_memory(&tvram);
    
    // Nettoyage
    text_visual_ram_cleanup(&tvram);
    
    // Résumé des tests
    printf("\n=== RÉSUMÉ DES TESTS ===\n");
    printf("Tests réussis: %d/%d\n", tests_passed, total_tests);
    printf("Taux de réussite: %.1f%%\n", (float)tests_passed / total_tests * 100);
    
    if (tests_passed == total_tests) {
        printf("\n🎉 TOUS LES TESTS SONT PASSÉS! 🎉\n");
        printf("Le système de mémoire visuelle autonome fonctionne parfaitement.\n");
        return 0;
    } else {
        printf("\n⚠️  CERTAINS TESTS ONT ÉCHOUÉ ⚠️\n");
        printf("Le système nécessite des corrections.\n");
        return 1;
    }
}

int main() {
    printf("RAPPORT COMPLET DE VALIDATION DU SYSTÈME DE MÉMOIRE VISUELLE AUTONOME\n");
    printf("=====================================================================\n\n");
    
    printf("Contexte: Implémentation d'un système de mémoire utilisant l'affichage\n");
    printf("         comme support de stockage persistant après libération de la RAM.\n\n");
    
    int result = run_automated_tests();
    
    if (result == 0) {
        printf("\n=== CONCLUSIONS FINALES ===\n");
        printf("✅ Le concept de mémoire visuelle autonome est VALIDÉ\n");
        printf("✅ L'initialisation avec RAM minimale fonctionne\n");
        printf("✅ La transition vers mode autonome (sans RAM) est réussie\n");
        printf("✅ Les opérations lecture/écriture en mode autonome fonctionnent\n");
        printf("✅ La persistance des données dans l'affichage est confirmée\n");
        printf("✅ L'intégrité des données (bits, octets, chaînes) est préservée\n");
        printf("✅ Les opérations multiples simultanées fonctionnent\n");
        printf("✅ La gestion des limites du système est correcte\n\n");
        
        printf("PRÊT POUR DÉPLOIEMENT: Le système peut être étendu à SDL2/pixels réels.\n");
        printf("Cette version textuelle prouve la viabilité du concept complet.\n");
    } else {
        printf("\n=== PROBLÈMES IDENTIFIÉS ===\n");
        printf("Le système nécessite des corrections avant déploiement.\n");
    }
    
    return result;
}