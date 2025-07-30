
/**
 * LIBVISUALMEM V2.0 - VERSION SÉCURISÉE ET OPTIMISÉE
 * ==================================================
 * 
 * Version complètement refactorisée intégrant toutes les corrections
 * de sécurité et optimisations identifiées par les audits Red Team.
 * 
 * Nouveautés V2:
 * - Thread safety complet
 * - Codes correcteurs d'erreurs (ECC)
 * - Protection anti-overflow
 * - Quotas et limites sécurisées
 * - Monitoring sécurité temps réel
 * - Anti-screenshot protection
 * - Effacement sécurisé
 * - Audit trail complet
 * 
 * Copyright (C) 2025 - Visual Memory Security Edition
 */

#ifndef LIBVISUALMEM_V2_H
#define LIBVISUALMEM_V2_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

// === CONSTANTES DE SÉCURITÉ ===
#define VISUALMEM_V2_VERSION "2.0.0-SECURE"
#define MAX_ALLOCATION_SIZE (1024 * 1024)  // 1MB max par allocation
#define MAX_ALLOCATIONS_PER_CONTEXT 50     // Limite quotas
#define MAX_TOTAL_MEMORY_PER_CONTEXT (10 * 1024 * 1024) // 10MB max
#define MAX_CONCURRENT_OPERATIONS 10       // Limite concurrence
#define CONTEXT_TIMEOUT_SECONDS 3600       // 1h timeout
#define ECC_BLOCK_SIZE 64                   // Blocs avec correction erreurs
#define ECC_CODE_SIZE 8                     // Taille code correcteur
#define SECURE_ERASE_PASSES 4               // Passes effacement sécurisé
#define AUDIT_TRAIL_MAX_ENTRIES 10000       // Historique audit

// === ÉNUMÉRATIONS ===
typedef enum {
    VISUALMEM_V2_MODE_SIMULATE = 0,
    VISUALMEM_V2_MODE_PIXEL = 1,
    VISUALMEM_V2_MODE_SECURE = 2         // Nouveau mode sécurisé
} visualmem_v2_mode_t;

typedef enum {
    VISUALMEM_V2_SUCCESS = 0,
    VISUALMEM_V2_ERROR_INVALID_PARAMS = -1,
    VISUALMEM_V2_ERROR_OUT_OF_MEMORY = -2,
    VISUALMEM_V2_ERROR_QUOTA_EXCEEDED = -3,
    VISUALMEM_V2_ERROR_SIZE_TOO_LARGE = -4,
    VISUALMEM_V2_ERROR_CORRUPTION_DETECTED = -5,
    VISUALMEM_V2_ERROR_THREAD_SAFETY = -6,
    VISUALMEM_V2_ERROR_SECURITY_VIOLATION = -7
} visualmem_v2_result_t;

typedef enum {
    SECURITY_EVENT_ALLOCATION = 0,
    SECURITY_EVENT_FREE = 1,
    SECURITY_EVENT_READ = 2,
    SECURITY_EVENT_WRITE = 3,
    SECURITY_EVENT_CORRUPTION = 4,
    SECURITY_EVENT_ATTACK_DETECTED = 5,
    SECURITY_EVENT_QUOTA_EXCEEDED = 6,
    SECURITY_EVENT_SUSPICIOUS_PATTERN = 7
} security_event_type_t;

typedef enum {
    SEVERITY_LOW = 1,
    SEVERITY_MEDIUM = 2,
    SEVERITY_HIGH = 3,
    SEVERITY_CRITICAL = 4
} security_severity_t;

// === STRUCTURES SÉCURISÉES ===

// Bloc de données avec correction d'erreurs
typedef struct {
    uint8_t data[ECC_BLOCK_SIZE];        // Données utiles
    uint8_t ecc_code[ECC_CODE_SIZE];     // Code correcteur Reed-Solomon
    uint32_t checksum;                   // Checksum intégrité
    uint64_t timestamp;                  // Timestamp création/modification
    uint32_t access_count;               // Compteur accès (détection anomalies)
} __attribute__((packed)) secure_data_block_t;

// Métadonnées d'allocation sécurisées
typedef struct allocation_metadata {
    void* address;                       // Adresse allocation
    size_t size;                        // Taille allocation
    char label[64];                     // Label sécurisé
    pthread_rwlock_t lock;              // Lock lecture/écriture
    volatile int ref_count;             // Compteur références atomique
    volatile bool is_free;              // Flag libération
    time_t creation_time;               // Timestamp création
    time_t last_access_time;            // Dernier accès
    uint32_t security_hash;             // Hash sécurité intégrité
    struct allocation_metadata* next;    // Liste chaînée
} allocation_metadata_t;

// Quotas et limites par contexte
typedef struct {
    int current_allocations;            // Allocations actuelles
    size_t current_total_memory;        // Mémoire totale utilisée
    int concurrent_operations;          // Opérations concurrentes
    time_t creation_time;               // Création contexte
    time_t last_activity;               // Dernière activité
    int security_violations;            // Violations sécurité
    double baseline_timing[8];          // Timing baseline (détection anomalies)
} context_limits_t;

// Entrée audit trail
typedef struct {
    uint64_t timestamp;                 // Timestamp précis
    security_event_type_t event_type;   // Type événement
    security_severity_t severity;       // Sévérité
    pid_t process_id;                   // PID processus
    uid_t user_id;                      // UID utilisateur
    void* address;                      // Adresse concernée
    size_t size;                        // Taille opération
    char details[256];                  // Détails événement
    uint32_t hash;                      // Hash pour intégrité
} audit_entry_t;

// Monitoring sécurité temps réel
typedef struct {
    int buffer_overflow_attempts;       // Tentatives overflow
    int corruption_detections;          // Corruptions détectées
    int timing_anomalies;              // Anomalies temporelles
    int concurrent_violations;          // Violations concurrence
    int suspicious_patterns;            // Patterns suspects
    time_t last_security_scan;         // Dernier scan sécurité
    bool screenshot_protection_active;  // Protection screenshot
} security_monitor_t;

// Contexte principal V2 sécurisé
typedef struct {
    // Configuration de base
    visualmem_v2_mode_t mode;
    int width, height;
    uint32_t* framebuffer;
    
    // Sécurité et thread safety
    pthread_mutex_t context_mutex;      // Mutex principal contexte
    pthread_rwlock_t allocations_lock;  // Lock liste allocations
    allocation_metadata_t* allocations; // Liste allocations sécurisées
    
    // Quotas et limites
    context_limits_t limits;
    
    // Monitoring sécurité
    security_monitor_t security;
    
    // Audit trail
    audit_entry_t* audit_trail;
    int audit_count;
    int audit_capacity;
    pthread_mutex_t audit_mutex;
    
    // État système
    bool autonomous_mode;
    bool initialized;
    uint32_t context_id;                // ID unique contexte
    char context_name[64];              // Nom contexte
} visualmem_v2_context_t;

// === API PRINCIPALE V2 ===

/**
 * Initialise un contexte mémoire visuelle V2 sécurisé
 */
visualmem_v2_result_t visualmem_v2_init(
    visualmem_v2_context_t* ctx,
    visualmem_v2_mode_t mode,
    int width,
    int height,
    const char* context_name
);

/**
 * Allocation mémoire sécurisée avec validation complète
 */
void* visualmem_v2_alloc_secure(
    visualmem_v2_context_t* ctx,
    size_t size,
    const char* label
);

/**
 * Écriture sécurisée avec codes correcteurs
 */
visualmem_v2_result_t visualmem_v2_write_secure(
    visualmem_v2_context_t* ctx,
    void* addr,
    const void* data,
    size_t size
);

/**
 * Lecture sécurisée avec vérification intégrité
 */
visualmem_v2_result_t visualmem_v2_read_secure(
    visualmem_v2_context_t* ctx,
    void* addr,
    void* buffer,
    size_t size
);

/**
 * Libération sécurisée avec effacement multi-passes
 */
visualmem_v2_result_t visualmem_v2_free_secure(
    visualmem_v2_context_t* ctx,
    void* addr
);

/**
 * Mode autonome sécurisé avec audit complet
 */
visualmem_v2_result_t visualmem_v2_enter_autonomous_mode_secure(
    visualmem_v2_context_t* ctx
);

/**
 * Nettoyage complet et sécurisé du contexte
 */
visualmem_v2_result_t visualmem_v2_cleanup_secure(
    visualmem_v2_context_t* ctx
);

// === API SÉCURITÉ ===

/**
 * Scan sécurité complet du contexte
 */
visualmem_v2_result_t visualmem_v2_security_scan(
    visualmem_v2_context_t* ctx
);

/**
 * Génère rapport sécurité détaillé
 */
visualmem_v2_result_t visualmem_v2_generate_security_report(
    visualmem_v2_context_t* ctx,
    char* report_buffer,
    size_t buffer_size
);

/**
 * Active/désactive protection anti-screenshot
 */
visualmem_v2_result_t visualmem_v2_toggle_screenshot_protection(
    visualmem_v2_context_t* ctx,
    bool enable
);

/**
 * Effacement sécurisé complet (multi-passes)
 */
visualmem_v2_result_t visualmem_v2_secure_erase_all(
    visualmem_v2_context_t* ctx
);

/**
 * Export audit trail chiffré
 */
visualmem_v2_result_t visualmem_v2_export_audit_trail(
    visualmem_v2_context_t* ctx,
    const char* filename,
    const char* encryption_key
);

// === API DIAGNOSTIQUES ===

/**
 * Test intégrité complet du système
 */
visualmem_v2_result_t visualmem_v2_integrity_test(
    visualmem_v2_context_t* ctx
);

/**
 * Benchmark performance sécurisé
 */
visualmem_v2_result_t visualmem_v2_benchmark_secure(
    visualmem_v2_context_t* ctx,
    char* results_buffer,
    size_t buffer_size
);

/**
 * Statistiques détaillées utilisation
 */
visualmem_v2_result_t visualmem_v2_get_statistics(
    visualmem_v2_context_t* ctx,
    char* stats_buffer,
    size_t buffer_size
);

// === FONCTIONS UTILITAIRES ===

/**
 * Convertit code erreur en string lisible
 */
const char* visualmem_v2_error_string(visualmem_v2_result_t result);

/**
 * Version et informations build
 */
const char* visualmem_v2_get_version_info(void);

/**
 * Test compatibilité système
 */
bool visualmem_v2_system_compatible(void);

#endif // LIBVISUALMEM_V2_H
