# Makefile for Visual RAM Autonomous Memory System

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LIBS = -lSDL2

# Object files
OBJS = main.o
READER_OBJS = reader.o
WRITER_OBJS = writer.o
DEMO_OBJS = demo.o

# Executables
TARGETS = main reader writer demo

# Default target
all: $(TARGETS)

# Main program (initialization and basic demo)
main: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Reader utility (standalone)
reader: $(READER_OBJS)
	$(CC) $(CFLAGS) -DREADER_STANDALONE -o $@ $^ $(LIBS)

# Writer utility (standalone)  
writer: $(WRITER_OBJS)
	$(CC) $(CFLAGS) -DWRITER_STANDALONE -o $@ $^ $(LIBS)

# Demo program (comprehensive demonstration)
demo: $(DEMO_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Object file compilation
main.o: main.c visual_ram.h
	$(CC) $(CFLAGS) -c main.c

reader.o: reader.c visual_ram.h
	$(CC) $(CFLAGS) -c reader.c

writer.o: writer.c visual_ram.h
	$(CC) $(CFLAGS) -c writer.c

demo.o: demo.c visual_ram.h
	$(CC) $(CFLAGS) -c demo.c

# Install SDL2 development libraries (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install libsdl2-dev

# Clean build files
clean:
	rm -f $(OBJS) $(READER_OBJS) $(WRITER_OBJS) $(DEMO_OBJS) $(TARGETS)

# Run the main demo
run-main: main
	./main

# Run the comprehensive demo
run-demo: demo
	./demo

# Run reader utility
run-reader: reader
	./reader

# Run writer utility
run-writer: writer
	./writer

# Build and run quick test
test: all
	@echo "=== Running Visual RAM Test ==="
	@echo "1. Building all components..."
	@echo "2. Running main program..."
	./main

# Help target
help:
	@echo "Visual RAM Autonomous Memory System - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build all programs"
	@echo "  main         - Build main initialization program"
	@echo "  reader       - Build standalone reader utility"
	@echo "  writer       - Build standalone writer utility"
	@echo "  demo         - Build comprehensive demo"
	@echo "  install-deps - Install SDL2 development libraries"
	@echo "  clean        - Remove build files"
	@echo "  test         - Build and run quick test"
	@echo "  run-main     - Build and run main program"
	@echo "  run-demo     - Build and run comprehensive demo"
	@echo "  run-reader   - Build and run reader utility"
	@echo "  run-writer   - Build and run writer utility"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Usage Examples:"
	@echo "  make install-deps  # First time setup"
	@echo "  make all           # Build everything"
	@echo "  make run-demo      # Run full demonstration"
	@echo "  make run-main      # Run basic program"

.PHONY: all clean install-deps run-main run-demo run-reader run-writer test help
