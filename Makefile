# Compiler settings
CXX = clang++
CC = clang
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra -Wno-deprecated-declarations
CFLAGS = -O3 -std=c11 -Wall -Wextra -Wno-unused-parameter
CPPFLAGS = -I./glm -I./SDL2 -I./SDL_image -I./SDL_mixer -I./SDL2_ttf -I./lua -I./box2d -I.
LDLIBS = -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -ldl -lm

# Source files
SOURCE_FILES = main.cpp game_engine.cpp game_utils.cpp

# Build bundled Lua runtime (exclude standalone interpreters to avoid duplicate main)
LUA_SOURCE_FILES = $(filter-out lua/lua.c lua/luac.c, $(wildcard lua/*.c))
LUA_OBJECT_FILES = $(LUA_SOURCE_FILES:.c=.o)

# Build bundled Box2D runtime (sources live in nested folders)
BOX2D_SOURCE_FILES = $(wildcard box2d/common/*.cpp box2d/collision/*.cpp box2d/dynamics/*.cpp box2d/rope/*.cpp)
BOX2D_OBJECT_FILES = $(patsubst %.cpp,%.o,$(BOX2D_SOURCE_FILES))

# Object files (automatically generated from source files)
OBJECT_FILES = main.o game_engine.o game_utils.o $(LUA_OBJECT_FILES) $(BOX2D_OBJECT_FILES)

# Executable name
EXECUTABLE = game_engine_linux

# Default target - builds the executable
all: $(EXECUTABLE)

# Link object files to create executable
$(EXECUTABLE): $(OBJECT_FILES)
	$(CXX) $(CXXFLAGS) $(OBJECT_FILES) -o $(EXECUTABLE) $(LDLIBS)

# Compile main.cpp to main.o
main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c main.cpp -o main.o

# Compile game_engine.cpp to game_engine.o
game_engine.o: game_engine.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c game_engine.cpp -o game_engine.o

# Compile game_utils.cpp to game_utils.o
game_utils.o: game_utils.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c game_utils.cpp -o game_utils.o

# Compile bundled Lua C sources
lua/%.o: lua/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compile bundled Box2D C++ sources
box2d/%.o: box2d/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Compile json_parser.cpp to json_parser.o
json_parser.o: json_paraser.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c json_paraser.cpp -o json_parser.o

# Compile scene_db.cpp to scene_db.o
scene_db.o: scene_db.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c scene_db.cpp -o scene_db.o

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