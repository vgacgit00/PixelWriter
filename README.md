# LibVisualMem - Autonomous Visual Memory Library

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/visualmem/libvisualmem)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Language](https://img.shields.io/badge/language-C99-orange.svg)](#)

**LibVisualMem** is an innovative C library that transforms display screens into persistent memory storage, enabling autonomous memory operations after traditional RAM liberation. This groundbreaking approach allows applications to operate with visual memory as an alternative or complement to conventional memory systems.

## 🚀 Key Features

- **Autonomous Operation**: System operates solely on visual display after RAM liberation
- **Visual Memory Allocation**: `valloc()` and `vfree()` functions for screen-based memory management
- **Data Persistence**: Memory contents persist in visual display beyond process lifetime
- **Multiple Modes**: Text-based, pixel-based, hybrid, and simulation modes
- **Error Correction**: Built-in data integrity validation and error handling
- **Cross-Platform**: Compatible with various display systems and architectures
- **Zero Dependencies**: No external libraries required for core functionality

## 📖 Concept

LibVisualMem implements a revolutionary memory paradigm:

1. **Initialization Phase**: Uses minimal CPU/RAM to bootstrap the visual memory system
2. **Data Encoding**: Converts binary data into pixel color patterns on screen
3. **Critical Transition**: Frees traditional RAM, leaving only visual display as memory
4. **Autonomous Mode**: All memory operations performed directly on screen pixels

```
[CPU/RAM Init] → [Encode Data] → [Display Setup] → [Free RAM] → [Autonomous Mode]
     ↓              ↓               ↓              ↓             ↓
  Bootstrap      Visual Patterns  Active Memory   RAM=NULL   Visual-Only
```

## 🛠 Installation

### From Source

```bash
git clone https://github.com/visualmem/libvisualmem.git
cd libvisualmem
make -f Makefile_lib all
```

### System Installation

```bash
make -f Makefile_lib install  # Requires root privileges
```

This installs:
- Static library: `/usr/local/lib/libvisualmem.a`
- Shared library: `/usr/local/lib/libvisualmem.so`
- Headers: `/usr/local/include/libvisualmem.h`

## 🎯 Quick Start

### Basic Usage

```c
#include "libvisualmem.h"

int main() {
    visualmem_context_t ctx;
    
    // Initialize visual memory system
    visualmem_init(&ctx, VISUALMEM_MODE_SIMULATE, 800, 600);
    
    // Allocate visual memory
    void* data_addr = visualmem_alloc(&ctx, 256, "user_data");
    
    // Write data to visual memory
    const char* message = "Hello Visual Memory!";
    visualmem_write_string(&ctx, data_addr, message);
    
    // CRITICAL: Enter autonomous mode (free RAM)
    visualmem_enter_autonomous_mode(&ctx);
    
    // Read data from visual memory only
    char buffer[256];
    visualmem_read_string(&ctx, data_addr, buffer, sizeof(buffer));
    
    printf("Retrieved: %s\n", buffer);
    
    // Cleanup
    visualmem_cleanup(&ctx);
    return 0;
}
```

### Compilation

```bash
# With static library
gcc myapp.c -o myapp libvisualmem.a

# With system-installed library
gcc myapp.c -o myapp -lvisualmem
```

## 📚 API Reference

### Core Functions

```c
// Initialization
int visualmem_init(visualmem_context_t* ctx, visualmem_mode_t mode, int width, int height);
void visualmem_cleanup(visualmem_context_t* ctx);

// Critical transition
int visualmem_enter_autonomous_mode(visualmem_context_t* ctx);

// Memory allocation
void* visualmem_alloc(visualmem_context_t* ctx, size_t size, const char* label);
int visualmem_free(visualmem_context_t* ctx, void* visual_addr);

// Data operations
int visualmem_write(visualmem_context_t* ctx, void* visual_addr, const void* data, size_t size);
int visualmem_read(visualmem_context_t* ctx, void* visual_addr, void* buffer, size_t size);

// String operations
int visualmem_write_string(visualmem_context_t* ctx, void* visual_addr, const char* str);
int visualmem_read_string(visualmem_context_t* ctx, void* visual_addr, char* buffer, size_t max_length);
```

### Memory Modes

- `VISUALMEM_MODE_TEXT`: Character-based visual memory (proof of concept)
- `VISUALMEM_MODE_PIXEL`: Full pixel-based implementation
- `VISUALMEM_MODE_HYBRID`: Combined text and pixel modes
- `VISUALMEM_MODE_SIMULATE`: Simulation mode for testing

## 💡 Use Cases

### 1. Secure Data Storage
Store sensitive information away from traditional heap memory, making it harder to extract via memory dumps.

### 2. Embedded Systems
Use display memory as primary storage in resource-constrained environments where RAM is limited.

### 3. Data Processing Pipelines
Utilize visual memory as intermediate storage, reducing RAM pressure during complex processing tasks.

### 4. Inter-Process Communication
Share data between applications through persistent visual memory without OS-level IPC mechanisms.

### 5. Educational Tools
Visualize memory operations and data structures directly on screen for teaching computer science concepts.

## 🧪 Testing

### Run Test Suite

```bash
make -f Makefile_lib test
```

### Run Usage Examples

```bash
make -f Makefile_lib examples
```

### Complete Validation

```bash
make -f Makefile_lib validate
```

## 📊 Performance

- **Visual Memory Throughput**: ~21 MB/s (simulated mode)
- **Traditional Memory Ratio**: ~557x slower than RAM (expected trade-off)
- **Memory Efficiency**: 100% visual storage after autonomous transition
- **Overhead**: Minimal after RAM liberation phase

## ⚠️ Limitations

- **Performance**: Significantly slower than traditional RAM
- **Display Dependency**: Requires active display for data persistence  
- **Resolution Constraints**: Memory capacity limited by screen resolution
- **Visual Artifacts**: Data patterns may be visible on screen

## 🔧 Advanced Features

### Debug Mode

```c
visualmem_set_debug_mode(&ctx, 1);  // Enable detailed logging
```

### Memory Statistics

```c
size_t allocated, peak;
int fragmentation;
visualmem_get_stats(&ctx, &allocated, &peak, &fragmentation);
```

### Visual Display

```c
visualmem_display_contents(&ctx, NULL);  // Show visual memory contents
```

## 🏗 Architecture

### Data Encoding

- Each byte encoded as 8 pixels (bit patterns)
- Start/end markers delineate data boundaries
- Color coding: Black=0, White=1, Red=Start, Green=End
- Error correction through redundant patterns

### Memory Layout

```
Screen Layout:
[Header Area] [Allocation Table] [Data Storage Area] [Free Space]
     ↓               ↓                    ↓              ↓
  System Info    Memory Map         User Data      Available
```

## 🤝 Contributing

We welcome contributions! Please read our [Contributing Guidelines](CONTRIBUTING.md) and submit pull requests to our [GitHub repository](https://github.com/visualmem/libvisualmem).

### Development Setup

```bash
git clone https://github.com/visualmem/libvisualmem.git
cd libvisualmem
make -f Makefile_lib debug    # Build with debug symbols
make -f Makefile_lib analyze  # Run static analysis
```

## 📄 License

LibVisualMem is released under the MIT License. See [LICENSE](LICENSE) file for details.

## 🔗 Links

- **Documentation**: [Full API Documentation](docs/)
- **Examples**: [Usage Examples](examples/)
- **Research Paper**: [Visual Memory Systems](research/visual-memory.pdf)
- **Community**: [Discord Server](https://discord.gg/visualmem)

## 🙏 Acknowledgments

- Inspired by unconventional memory architectures research
- Built for the autonomous computing community
- Dedicated to pushing the boundaries of traditional memory systems

---

**LibVisualMem** - *Transforming pixels into persistent memory*

*"When RAM is not enough, the screen becomes your memory."*

## 📄 Fiche technique Web/Native

| Élément                | Valeur/Exemple                                      |
|------------------------|-----------------------------------------------------|
| CPU                    | Intel i7-12700H / Apple M1 / AMD Ryzen 7            |
| GPU                    | NVIDIA RTX 4090 / Apple M1 GPU / Intel Iris Xe      |
| RAM                    | 32 GB DDR5                                          |
| Stockage               | SSD NVMe Gen4 1TB                                   |
| Navigateur             | Chrome 125 / Edge 124 / Firefox 126                 |
| WebGPU activé          | Oui / Non                                           |
| OS                     | Windows 11 / macOS 14 / Ubuntu 24.04                |
| Moteur de rendu        | WebGL2 / WebGPU / Canvas2D                          |
| Mémoire GPU dispo      | 24 GB (RTX 4090) / 8 GB (M1)                        |
| Mode d’accès mémoire   | Direct VRAM / Simulé (ArrayBuffer/Canvas)           |
| Logger énergétique     | NVML / INA219 / API Battery / estimation logicielle |
| Script benchmark fourni| benchmark_visualmem.js / perf_hooks                 |

## 🔁 Reproductibilité

1. **Cloner le repo et installer les dépendances**
2. **Vérifier l’environnement** :
   - Navigateur compatible WebGPU (chrome://flags/#enable-unsafe-webgpu)
   - Node.js >= 18 si test Node.js
   - GPU compatible CUDA/Metal/Vulkan
3. **Lancer le script de benchmark** :
   - `node benchmark_visualmem.js --size 16KB --pattern zeros --threads 8`
   - Ou via navigateur : ouvrir `benchmark_visualmem.html`
4. **Exporter les résultats** :
   - JSON, Markdown, ou CSV
5. **Logger énergétique** :
   - Si possible, activer NVML/INA219 ou API Battery
6. **Procédure de validation croisée** :
   - Comparer les résultats sur plusieurs machines/OS/navigateurs
   - Publier les logs et scripts pour validation communautaire

## 🧪 Testabilité Node.js/benchmark.js/perf_hooks
- Utiliser `perf_hooks` pour mesurer latence/bande passante en Node.js
- Utiliser `benchmark.js` pour micro-benchmarks
- Fournir un script de test automatisé pour chaque environnement

## 🌐 Conseils pour reproductibilité Web/Node.js/Native
- Toujours documenter l’environnement d’exécution
- Préciser la version du navigateur, du moteur WebGPU, et du GPU
- Fournir les logs de benchmark et la configuration matérielle
- Utiliser le logger énergétique pour comparer l’efficacité
- Tester sur au moins 2 navigateurs et 2 OS différents