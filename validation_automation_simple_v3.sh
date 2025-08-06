#!/bin/bash

# Script d'automatisation de validation LibVisualMem v3.0 Simple
# =============================================================
# Automatise la création, compilation et validation des nouvelles versions
# avec vérification complète et historique (version simple sans affichage)

set -e  # Arrêt en cas d'erreur

# Configuration
VERSION="3.0-SIMPLE"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(pwd)"
LOG_FILE="validation_automation_simple.log"
HISTORY_FILE="validation_history_simple.json"

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Fonctions utilitaires
log() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1" | tee -a "$LOG_FILE"
}

success() {
    echo -e "${GREEN}✅ $1${NC}" | tee -a "$LOG_FILE"
}

warning() {
    echo -e "${YELLOW}⚠️  $1${NC}" | tee -a "$LOG_FILE"
}

error() {
    echo -e "${RED}❌ $1${NC}" | tee -a "$LOG_FILE"
}

info() {
    echo -e "${CYAN}ℹ️  $1${NC}" | tee -a "$LOG_FILE"
}

# Vérification des prérequis
check_prerequisites() {
    log "Vérification des prérequis..."
    
    # Vérifier les outils de compilation
    if ! command -v gcc &> /dev/null; then
        error "GCC n'est pas installé"
        return 1
    fi
    
    # Vérifier les bibliothèques
    if ! pkg-config --exists x11; then
        warning "libX11-dev n'est pas installé"
        info "Installation automatique des dépendances..."
        sudo apt-get update && sudo apt-get install -y libx11-dev
    fi
    
    # Vérifier les fichiers sources
    if [[ ! -f "test_validation_simple_v3.c" ]]; then
        error "Fichier test_validation_simple_v3.c manquant"
        return 1
    fi
    
    if [[ ! -f "libvisualmem_v2.h" ]]; then
        error "Fichier libvisualmem_v2.h manquant"
        return 1
    fi
    
    success "Prérequis vérifiés"
}

# Compilation de la version
compile_version() {
    log "Compilation de la version v$VERSION..."
    
    if make -f Makefile_v3_simple clean; then
        success "Nettoyage effectué"
    else
        warning "Nettoyage échoué (peut être normal)"
    fi
    
    if make -f Makefile_v3_simple all; then
        success "Compilation réussie"
        return 0
    else
        error "Compilation échouée"
        return 1
    fi
}

# Validation complète
run_validation() {
    log "Lancement de la validation complète..."
    
    # Créer un timestamp unique
    TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
    VALIDATION_DIR="validation_simple_v3_${TIMESTAMP}"
    
    # Lancer le test
    if ./test_validation_simple_v3; then
        success "Validation terminée avec succès"
        
        # Vérifier que le répertoire de version a été créé
        if [[ -d "$VALIDATION_DIR" ]]; then
            success "Répertoire de version créé: $VALIDATION_DIR"
            
            # Lister les fichiers générés
            log "Fichiers générés:"
            ls -la "$VALIDATION_DIR/"
            
            return 0
        else
            error "Répertoire de version non créé"
            return 1
        fi
    else
        error "Validation échouée"
        return 1
    fi
}

# Analyse des résultats
analyze_results() {
    log "Analyse des résultats..."
    
    # Trouver le répertoire de validation le plus récent
    LATEST_DIR=$(ls -td validation_simple_v3_* 2>/dev/null | head -1)
    
    if [[ -z "$LATEST_DIR" ]]; then
        error "Aucun répertoire de validation trouvé"
        return 1
    fi
    
    info "Analyse du répertoire: $LATEST_DIR"
    
    # Vérifier les fichiers de résultats
    if [[ -f "$LATEST_DIR/metrics_validation_simple_v3_"*.json ]]; then
        METRICS_FILE=$(ls "$LATEST_DIR/metrics_validation_simple_v3_"*.json | head -1)
        success "Fichier de métriques trouvé: $METRICS_FILE"
        
        # Extraire les métriques
        if command -v jq &> /dev/null; then
            SUCCESS_RATE=$(jq -r '.success_rate' "$METRICS_FILE" 2>/dev/null || echo "N/A")
            DURATION=$(jq -r '.duration_seconds' "$METRICS_FILE" 2>/dev/null || echo "N/A")
            TESTS_PASSED=$(jq -r '.tests_passed' "$METRICS_FILE" 2>/dev/null || echo "N/A")
            TESTS_TOTAL=$(jq -r '.tests_total' "$METRICS_FILE" 2>/dev/null || echo "N/A")
            
            log "Métriques extraites:"
            log "  Taux de succès: $SUCCESS_RATE%"
            log "  Durée: $DURATION secondes"
            log "  Tests réussis: $TESTS_PASSED/$TESTS_TOTAL"
        else
            warning "jq non installé, impossible d'extraire les métriques JSON"
        fi
    else
        warning "Fichier de métriques non trouvé"
    fi
    
    # Vérifier le rapport
    if [[ -f "$LATEST_DIR/rapport_validation_simple_v3_"*.md ]]; then
        REPORT_FILE=$(ls "$LATEST_DIR/rapport_validation_simple_v3_"*.md | head -1)
        success "Rapport trouvé: $REPORT_FILE"
        
        # Afficher un extrait du rapport
        log "Extrait du rapport:"
        head -20 "$REPORT_FILE" | while read line; do
            echo "  $line"
        done
    else
        warning "Rapport non trouvé"
    fi
    
    # Vérifier les logs
    if [[ -f "$LATEST_DIR/logs_validation_simple_v3_"*.log ]]; then
        LOG_FILE_VALIDATION=$(ls "$LATEST_DIR/logs_validation_simple_v3_"*.log | head -1)
        success "Logs trouvés: $LOG_FILE_VALIDATION"
        
        # Compter les entrées de log
        LOG_ENTRIES=$(wc -l < "$LOG_FILE_VALIDATION" 2>/dev/null || echo "0")
        log "Nombre d'entrées de log: $LOG_ENTRIES"
    else
        warning "Logs non trouvés"
    fi
}

# Mise à jour de l'historique
update_history() {
    log "Mise à jour de l'historique..."
    
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    LATEST_DIR=$(ls -td validation_simple_v3_* 2>/dev/null | head -1)
    
    if [[ -n "$LATEST_DIR" ]]; then
        # Créer l'entrée d'historique
        HISTORY_ENTRY=$(cat <<EOF
{
  "timestamp": "$TIMESTAMP",
  "version": "$VERSION",
  "validation_dir": "$LATEST_DIR",
  "status": "completed"
}
EOF
)
        
        # Ajouter à l'historique
        if [[ -f "$HISTORY_FILE" ]]; then
            # Ajouter à un tableau JSON existant
            sed -i 's/\]$/,/' "$HISTORY_FILE" 2>/dev/null || true
            echo "$HISTORY_ENTRY" >> "$HISTORY_FILE"
            sed -i 's/,$/]/' "$HISTORY_FILE" 2>/dev/null || true
        else
            # Créer un nouveau fichier d'historique
            echo "[$HISTORY_ENTRY]" > "$HISTORY_FILE"
        fi
        
        success "Historique mis à jour"
    else
        warning "Aucun répertoire de validation trouvé pour l'historique"
    fi
}

# Validation de l'intégrité
validate_integrity() {
    log "Validation de l'intégrité..."
    
    LATEST_DIR=$(ls -td validation_simple_v3_* 2>/dev/null | head -1)
    
    if [[ -z "$LATEST_DIR" ]]; then
        error "Aucun répertoire de validation à vérifier"
        return 1
    fi
    
    # Vérifier la structure du répertoire
    REQUIRED_FILES=("logs_validation_simple_v3_" "metrics_validation_simple_v3_" "rapport_validation_simple_v3_")
    
    for pattern in "${REQUIRED_FILES[@]}"; do
        if ls "$LATEST_DIR/${pattern}"* 1> /dev/null 2>&1; then
            success "Fichier $pattern trouvé"
        else
            error "Fichier $pattern manquant"
            return 1
        fi
    done
    
    # Vérifier la taille des fichiers
    for file in "$LATEST_DIR"/*; do
        if [[ -f "$file" ]]; then
            SIZE=$(stat -c%s "$file" 2>/dev/null || echo "0")
            if [[ "$SIZE" -gt 0 ]]; then
                success "Fichier $(basename "$file") valide (${SIZE} bytes)"
            else
                warning "Fichier $(basename "$file") vide"
            fi
        fi
    done
    
    success "Intégrité validée"
}

# Fonction principale
main() {
    echo -e "${PURPLE}"
    echo "=================================================================="
    echo "   AUTOMATISATION DE VALIDATION LIBVISUALMEM V$VERSION"
    echo "=================================================================="
    echo -e "${NC}"
    
    # Initialiser le log
    echo "=== Début de l'automatisation $(date) ===" > "$LOG_FILE"
    
    # Étapes de validation
    if check_prerequisites; then
        if compile_version; then
            if run_validation; then
                analyze_results
                update_history
                validate_integrity
                
                echo -e "${GREEN}"
                echo "=================================================================="
                echo "   VALIDATION V$VERSION TERMINÉE AVEC SUCCÈS"
                echo "=================================================================="
                echo -e "${NC}"
                
                # Afficher le répertoire créé
                LATEST_DIR=$(ls -td validation_simple_v3_* 2>/dev/null | head -1)
                if [[ -n "$LATEST_DIR" ]]; then
                    echo "📁 Répertoire de version: $LATEST_DIR"
                    echo "📄 Logs: $LATEST_DIR/logs_validation_simple_v3_*.log"
                    echo "📊 Métriques: $LATEST_DIR/metrics_validation_simple_v3_*.json"
                    echo "📋 Rapport: $LATEST_DIR/rapport_validation_simple_v3_*.md"
                fi
                
                return 0
            else
                error "Échec de la validation"
                return 1
            fi
        else
            error "Échec de la compilation"
            return 1
        fi
    else
        error "Échec de la vérification des prérequis"
        return 1
    fi
}

# Gestion des arguments
case "${1:-}" in
    "clean")
        log "Nettoyage des fichiers de validation..."
        rm -rf validation_simple_v3_*
        rm -f validation_automation_simple.log validation_history_simple.json
        success "Nettoyage terminé"
        ;;
    "history")
        if [[ -f "$HISTORY_FILE" ]]; then
            log "Historique des validations:"
            cat "$HISTORY_FILE" | jq '.' 2>/dev/null || cat "$HISTORY_FILE"
        else
            info "Aucun historique trouvé"
        fi
        ;;
    "help")
        echo "Script d'automatisation de validation LibVisualMem v$VERSION"
        echo ""
        echo "Usage: $0 [COMMANDE]"
        echo ""
        echo "Commandes:"
        echo "  (aucune)  - Lance la validation complète"
        echo "  clean     - Nettoie les fichiers de validation"
        echo "  history   - Affiche l'historique des validations"
        echo "  help      - Affiche cette aide"
        ;;
    *)
        main
        ;;
esac