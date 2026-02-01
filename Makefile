# Compiler settings
COMPILER = clang++
COMPILER_FLAGS = -O3 -std=c++17 -Wall -Wextra -Wno-deprecated-declarations
INCLUDE_PATHS = -I./glm

# Source files
SOURCE_FILES = main.cpp game_engine.cpp 

# Object files (automatically generated from source files)
OBJECT_FILES = main.o game_engine.o 

# Executable name
EXECUTABLE = game_engine_linux

# Default target - builds the executable
all: $(EXECUTABLE)

# Link object files to create executable
$(EXECUTABLE): $(OBJECT_FILES)
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE_PATHS) $(OBJECT_FILES) -o $(EXECUTABLE)

# Compile main.cpp to main.o
main.o: main.cpp
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE_PATHS) -c main.cpp -o main.o

# Compile game_engine.cpp to game_engine.o
game_engine.o: game_engine.cpp
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE_PATHS) -c game_engine.cpp -o game_engine.o

# Compile json_parser.cpp to json_parser.o
json_parser.o: json_paraser.cpp
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE_PATHS) -c json_paraser.cpp -o json_parser.o

# Compile scene_db.cpp to scene_db.o
scene_db.o: scene_db.cpp
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE_PATHS) -c scene_db.cpp -o scene_db.o

# Clean build artifacts
clean:
	rm -f $(OBJECT_FILES) $(EXECUTABLE)

# Rebuild from scratch
rebuild: clean all

# Run the executable
run: $(EXECUTABLE)
	./$(EXECUTABLE)

# Phony targets (not actual files)
.PHONY: all clean rebuild run