
/**
 * TESTS COMPLETS VISUALMEM V2 - VERSION SÉCURISÉE
 * ===============================================
 * 
 * Suite de tests exhaustive pour valider toutes les fonctionnalités
 * de sécurité et optimisations de la version 2.0.
 */

#include "libvisualmem_v2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// === CONFIGURATION TESTS ===
#define TEST_WIDTH 800
#define TEST_HEIGHT 600
#define TEST_THREADS 10
#define TEST_ITERATIONS 100

// === STRUCTURES TEST ===
typedef struct {
    int test_id;
    int passed;
    int failed;
    char description[256];
} test_result_t;

typedef struct {
    visualmem_v2_context_t* ctx;
    int thread_id;
    int operations;
    int errors;
} thread_test_data_t;

// === VARIABLES GLOBALES ===
static int g_total_tests = 0;
static int g_passed_tests = 0;
static int g_failed_tests = 0;

// === UTILITAIRES TEST ===

void print_test_header(const char* test_name) {
    printf("\n" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
    printf("TEST: %s\n", test_name);
    printf("=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
}

void record_test_result(const char* test_name, bool passed, const char* details) {
    g_total_tests++;
    if (passed) {
        g_passed_tests++;
        printf("✅ PASSÉ: %s\n", test_name);
    } else {
        g_failed_tests++;
        printf("❌ ÉCHOUÉ: %s - %s\n", test_name, details ? details : "");
    }
    if (details && passed) {
        printf("   %s\n", details);
    }
}

// === TESTS FONCTIONNELS DE BASE ===

bool test_context_initialization(void) {
    visualmem_v2_context_t ctx;
    
    // Test initialisation normale
    visualmem_v2_result_t result = visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_SECURE, 
                                                     TEST_WIDTH, TEST_HEIGHT, "test_ctx");
    
    if (result != VISUALMEM_V2_SUCCESS) {
        return false;
    }
    
    // Vérifications état initial
    bool valid = (ctx.initialized == true &&
                  ctx.width == TEST_WIDTH &&
                  ctx.height == TEST_HEIGHT &&
                  ctx.framebuffer != NULL &&
                  ctx.allocations == NULL &&
                  ctx.audit_trail != NULL);
    
    // Nettoyage
    visualmem_v2_cleanup_secure(&ctx);
    
    return valid;
}

bool test_parameter_validation(void) {
    visualmem_v2_context_t ctx;
    
    // Test paramètres invalides
    visualmem_v2_result_t result;
    
    // Contexte NULL
    result = visualmem_v2_init(NULL, VISUALMEM_V2_MODE_SECURE, 100, 100, "test");
    if (result == VISUALMEM_V2_SUCCESS) return false;
    
    // Dimensions invalides
    result = visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_SECURE, 0, 100, "test");
    if (result == VISUALMEM_V2_SUCCESS) return false;
    
    result = visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_SECURE, 100, -1, "test");
    if (result == VISUALMEM_V2_SUCCESS) return false;
    
    return true;
}

bool test_secure_allocation(void) {
    visualmem_v2_context_t ctx;
    
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_SECURE, TEST_WIDTH, TEST_HEIGHT, "alloc_test") 
        != VISUALMEM_V2_SUCCESS) {
        return false;
    }
    
    // Allocation normale
    void* ptr1 = visualmem_v2_alloc_secure(&ctx, 1024, "test_block");
    if (!ptr1) {
        visualmem_v2_cleanup_secure(&ctx);
        return false;
    }
    
    // Test taille trop grande
    void* ptr2 = visualmem_v2_alloc_secure(&ctx, MAX_ALLOCATION_SIZE + 1, "too_big");
    if (ptr2) { // Ne devrait pas réussir
        visualmem_v2_cleanup_secure(&ctx);
        return false;
    }
    
    // Test taille zéro
    void* ptr3 = visualmem_v2_alloc_secure(&ctx, 0, "zero_size");
    if (ptr3) { // Ne devrait pas réussir
        visualmem_v2_cleanup_secure(&ctx);
        return false;
    }
    
    // Libération
    visualmem_v2_result_t result = visualmem_v2_free_secure(&ctx, ptr1);
    if (result != VISUALMEM_V2_SUCCESS) {
        visualmem_v2_cleanup_secure(&ctx);
        return false;
    }
    
    visualmem_v2_cleanup_secure(&ctx);
    return true;
}

bool test_secure_read_write(void) {
    visualmem_v2_context_t ctx;
    
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_SECURE, TEST_WIDTH, TEST_HEIGHT, "rw_test") 
        != VISUALMEM_V2_SUCCESS) {
        return false;
    }
    
    // Allocation
    void* ptr = visualmem_v2_alloc_secure(&ctx, 1024, "rw_block");
    if (!ptr) {
        visualmem_v2_cleanup_secure(&ctx);
        return false;
    }
    
    // Données test
    char write_data[1024];
    char read_data[1024];
    
    for (int i = 0; i < 1024; i++) {
        write_data[i] = (char)(i % 256);
    }
    
    // Écriture
    visualmem_v2_result_t result = visualmem_v2_write_secure(&ctx, ptr, write_data, 1024);
    if (result != VISUALMEM_V2_SUCCESS) {
        visualmem_v2_cleanup_secure(&ctx);
        return false;
    }
    
    // Lecture
    result = visualmem_v2_read_secure(&ctx, ptr, read_data, 1024);
    if (result != VISUALMEM_V2_SUCCESS) {
        visualmem_v2_cleanup_secure(&ctx);
        return false;
    }
    
    // Vérification intégrité
    bool data_ok = (memcmp(write_data, read_data, 1024) == 0);
    
    visualmem_v2_cleanup_secure(&ctx);
    return data_ok;
}

// === TESTS SÉCURITÉ ===

bool test_buffer_overflow_protection(void) {
    visualmem_v2_context_t ctx;
    
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_SECURE, TEST_WIDTH, TEST_HEIGHT, "overflow_test") 
        != VISUALMEM_V2_SUCCESS) {
        return false;
    }
    
    // Allocation petite
    void* ptr = visualmem_v2_alloc_secure(&ctx, 100, "small_block");
    if (!ptr) {
        visualmem_v2_cleanup_secure(&ctx);
        return false;
    }
    
    // Tentative écriture dépassant taille
    char big_data[200];
    memset(big_data, 0xAA, sizeof(big_data));
    
    visualmem_v2_result_t result = visualmem_v2_write_secure(&ctx, ptr, big_data, 200);
    
    // Doit échouer avec violation sécurité
    bool protection_ok = (result == VISUALMEM_V2_ERROR_SECURITY_VIOLATION);
    
    visualmem_v2_cleanup_secure(&ctx);
    return protection_ok;
}

bool test_quota_limits(void) {
    visualmem_v2_context_t ctx;
    
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_SECURE, TEST_WIDTH, TEST_HEIGHT, "quota_test") 
        != VISUALMEM_V2_SUCCESS) {
        return false;
    }
    
    // Allocations jusqu'à limite
    void* ptrs[MAX_ALLOCATIONS_PER_CONTEXT + 5];
    int successful_allocs = 0;
    
    for (int i = 0; i < MAX_ALLOCATIONS_PER_CONTEXT + 5; i++) {
        ptrs[i] = visualmem_v2_alloc_secure(&ctx, 1024, "quota_block");
        if (ptrs[i]) {
            successful_allocs++;
        }
    }
    
    // Devrait s'arrêter à la limite
    bool quota_respected = (successful_allocs <= MAX_ALLOCATIONS_PER_CONTEXT);
    
    visualmem_v2_cleanup_secure(&ctx);
    return quota_respected;
}

bool test_use_after_free_protection(void) {
    visualmem_v2_context_t ctx;
    
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_SECURE, TEST_WIDTH, TEST_HEIGHT, "uaf_test") 
        != VISUALMEM_V2_SUCCESS) {
        return false;
    }
    
    // Allocation et libération
    void* ptr = visualmem_v2_alloc_secure(&ctx, 1024, "uaf_block");
    if (!ptr) {
        visualmem_v2_cleanup_secure(&ctx);
        return false;
    }
    
    visualmem_v2_result_t result = visualmem_v2_free_secure(&ctx, ptr);
    if (result != VISUALMEM_V2_SUCCESS) {
        visualmem_v2_cleanup_secure(&ctx);
        return false;
    }
    
    // Tentative utilisation après libération
    char data[100];
    result = visualmem_v2_read_secure(&ctx, ptr, data, 100);
    
    // Doit échouer
    bool protection_ok = (result != VISUALMEM_V2_SUCCESS);
    
    visualmem_v2_cleanup_secure(&ctx);
    return protection_ok;
}

// === TESTS CONCURRENCE ===

void* thread_concurrent_operations(void* arg) {
    thread_test_data_t* data = (thread_test_data_t*)arg;
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        // Allocation
        void* ptr = visualmem_v2_alloc_secure(data->ctx, 512, "thread_block");
        if (!ptr) {
            data->errors++;
            continue;
        }
        
        // Écriture
        char test_data[512];
        memset(test_data, data->thread_id + i, sizeof(test_data));
        
        visualmem_v2_result_t result = visualmem_v2_write_secure(data->ctx, ptr, test_data, 512);
        if (result != VISUALMEM_V2_SUCCESS) {
            data->errors++;
        }
        
        // Lecture
        char read_buffer[512];
        result = visualmem_v2_read_secure(data->ctx, ptr, read_buffer, 512);
        if (result != VISUALMEM_V2_SUCCESS) {
            data->errors++;
        }
        
        // Vérification
        if (memcmp(test_data, read_buffer, 512) != 0) {
            data->errors++;
        }
        
        // Libération
        result = visualmem_v2_free_secure(data->ctx, ptr);
        if (result != VISUALMEM_V2_SUCCESS) {
            data->errors++;
        }
        
        data->operations++;
        
        // Petit délai pour simulation charge
        if (i % 10 == 0) {
            usleep(1000); // 1ms
        }
    }
    
    return NULL;
}

bool test_thread_safety(void) {
    visualmem_v2_context_t ctx;
    
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_SECURE, TEST_WIDTH, TEST_HEIGHT, "thread_test") 
        != VISUALMEM_V2_SUCCESS) {
        return false;
    }
    
    // Création threads
    pthread_t threads[TEST_THREADS];
    thread_test_data_t thread_data[TEST_THREADS];
    
    for (int i = 0; i < TEST_THREADS; i++) {
        thread_data[i].ctx = &ctx;
        thread_data[i].thread_id = i;
        thread_data[i].operations = 0;
        thread_data[i].errors = 0;
        
        if (pthread_create(&threads[i], NULL, thread_concurrent_operations, &thread_data[i]) != 0) {
            visualmem_v2_cleanup_secure(&ctx);
            return false;
        }
    }
    
    // Attendre threads
    for (int i = 0; i < TEST_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Vérification résultats
    int total_operations = 0;
    int total_errors = 0;
    
    for (int i = 0; i < TEST_THREADS; i++) {
        total_operations += thread_data[i].operations;
        total_errors += thread_data[i].errors;
    }
    
    double error_rate = (double)total_errors / total_operations * 100.0;
    
    printf("   Operations: %d, Errors: %d, Error rate: %.2f%%\n", 
           total_operations, total_errors, error_rate);
    
    visualmem_v2_cleanup_secure(&ctx);
    
    // Taux d'erreur acceptable < 1%
    return (error_rate < 1.0);
}

// === TESTS PERFORMANCE ===

bool test_performance_benchmarks(void) {
    visualmem_v2_context_t ctx;
    
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_SECURE, TEST_WIDTH, TEST_HEIGHT, "perf_test") 
        != VISUALMEM_V2_SUCCESS) {
        return false;
    }
    
    clock_t start, end;
    const int iterations = 1000;
    
    // Benchmark allocations
    start = clock();
    void* ptrs[iterations];
    for (int i = 0; i < iterations; i++) {
        ptrs[i] = visualmem_v2_alloc_secure(&ctx, 1024, "perf_block");
    }
    end = clock();
    
    double alloc_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    double alloc_per_ms = iterations / alloc_time;
    
    // Benchmark écritures
    char data[1024];
    memset(data, 0xAB, sizeof(data));
    
    start = clock();
    for (int i = 0; i < iterations; i++) {
        if (ptrs[i]) {
            visualmem_v2_write_secure(&ctx, ptrs[i], data, 1024);
        }
    }
    end = clock();
    
    double write_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    double write_per_ms = iterations / write_time;
    
    // Benchmark lectures
    char buffer[1024];
    
    start = clock();
    for (int i = 0; i < iterations; i++) {
        if (ptrs[i]) {
            visualmem_v2_read_secure(&ctx, ptrs[i], buffer, 1024);
        }
    }
    end = clock();
    
    double read_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    double read_per_ms = iterations / read_time;
    
    // Nettoyage
    for (int i = 0; i < iterations; i++) {
        if (ptrs[i]) {
            visualmem_v2_free_secure(&ctx, ptrs[i]);
        }
    }
    
    printf("   Alloc: %.1f ops/ms (%.2fms total)\n", alloc_per_ms, alloc_time);
    printf("   Write: %.1f ops/ms (%.2fms total)\n", write_per_ms, write_time);
    printf("   Read:  %.1f ops/ms (%.2fms total)\n", read_per_ms, read_time);
    
    visualmem_v2_cleanup_secure(&ctx);
    
    // Performance acceptable: >10 ops/ms pour chaque opération
    return (alloc_per_ms > 10.0 && write_per_ms > 10.0 && read_per_ms > 10.0);
}

// === TESTS AUDIT ET MONITORING ===

bool test_security_monitoring(void) {
    visualmem_v2_context_t ctx;
    
    if (visualmem_v2_init(&ctx, VISUALMEM_V2_MODE_SECURE, TEST_WIDTH, TEST_HEIGHT, "monitor_test") 
        != VISUALMEM_V2_SUCCESS) {
        return false;
    }
    
    // Operations normales pour générer audit trail
    void* ptr1 = visualmem_v2_alloc_secure(&ctx, 1024, "monitor_block1");
    void* ptr2 = visualmem_v2_alloc_secure(&ctx, 2048, "monitor_block2");
    
    if (ptr1 && ptr2) {
        char data[1024];
        memset(data, 0xCC, sizeof(data));
        
        visualmem_v2_write_secure(&ctx, ptr1, data, 1024);
        visualmem_v2_read_secure(&ctx, ptr1, data, 1024);
        
        visualmem_v2_free_secure(&ctx, ptr1);
        visualmem_v2_free_secure(&ctx, ptr2);
    }
    
    // Tentatives malveillantes pour tester monitoring
    visualmem_v2_alloc_secure(&ctx, MAX_ALLOCATION_SIZE + 1, "too_big"); // Devrait échouer
    
    // Génération rapport sécurité
    char report[4096];
    visualmem_v2_result_t result = visualmem_v2_generate_security_report(&ctx, report, sizeof(report));
    
    bool monitoring_ok = (result == VISUALMEM_V2_SUCCESS && strlen(report) > 0);
    
    if (monitoring_ok) {
        printf("   Security report generated (%zu bytes)\n", strlen(report));
    }
    
    visualmem_v2_cleanup_secure(&ctx);
    return monitoring_ok;
}

// === FONCTION PRINCIPALE TESTS ===

int main(void) {
    printf("🔒 TESTS COMPLETS VISUALMEM V2 - VERSION SÉCURISÉE\n");
    printf("==================================================\n");
    printf("Version: %s\n", visualmem_v2_get_version_info());
    printf("Compatibilité système: %s\n", visualmem_v2_system_compatible() ? "✅ OK" : "❌ NON");
    
    // === TESTS FONCTIONNELS ===
    print_test_header("TESTS FONCTIONNELS DE BASE");
    
    record_test_result("Initialisation contexte", 
                      test_context_initialization(),
                      "Initialisation et validation état initial");
    
    record_test_result("Validation paramètres", 
                      test_parameter_validation(),
                      "Rejection paramètres invalides");
    
    record_test_result("Allocation sécurisée", 
                      test_secure_allocation(),
                      "Allocation avec validation taille et quotas");
    
    record_test_result("Lecture/Écriture sécurisée", 
                      test_secure_read_write(),
                      "I/O avec vérification intégrité");
    
    // === TESTS SÉCURITÉ ===
    print_test_header("TESTS SÉCURITÉ");
    
    record_test_result("Protection buffer overflow", 
                      test_buffer_overflow_protection(),
                      "Détection tentatives dépassement buffer");
    
    record_test_result("Limites quotas", 
                      test_quota_limits(),
                      "Enforcement quotas allocations/mémoire");
    
    record_test_result("Protection use-after-free", 
                      test_use_after_free_protection(),
                      "Blocage accès après libération");
    
    // === TESTS CONCURRENCE ===
    print_test_header("TESTS CONCURRENCE");
    
    record_test_result("Thread safety", 
                      test_thread_safety(),
                      "Opérations concurrentes multiples threads");
    
    // === TESTS PERFORMANCE ===
    print_test_header("TESTS PERFORMANCE");
    
    record_test_result("Benchmarks performance", 
                      test_performance_benchmarks(),
                      "Mesures performances opérations");
    
    // === TESTS MONITORING ===
    print_test_header("TESTS MONITORING");
    
    record_test_result("Monitoring sécurité", 
                      test_security_monitoring(),
                      "Audit trail et rapports sécurité");
    
    // === RÉSULTATS FINAUX ===
    printf("\n" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
    printf("RÉSULTATS FINAUX TESTS V2\n");
    printf("=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
    
    printf("Total tests: %d\n", g_total_tests);
    printf("Tests réussis: %d (%.1f%%)\n", g_passed_tests, 
           (double)g_passed_tests / g_total_tests * 100.0);
    printf("Tests échoués: %d (%.1f%%)\n", g_failed_tests, 
           (double)g_failed_tests / g_total_tests * 100.0);
    
    if (g_failed_tests == 0) {
        printf("\n🎉 TOUS LES TESTS RÉUSSIS - VERSION V2 VALIDÉE!\n");
        printf("✅ La version sécurisée est prête pour déploiement\n");
    } else {
        printf("\n⚠️  CERTAINS TESTS ONT ÉCHOUÉ\n");
        printf("❌ Corrections nécessaires avant déploiement\n");
    }
    
    printf("\n🔒 Suite de tests sécurisée terminée.\n");
    
    return (g_failed_tests == 0) ? 0 : 1;
}
