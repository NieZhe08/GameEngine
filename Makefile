CXX = clang++
CXXFLAGS = -O2 -std=c++17 -Wall -Wextra

INCLUDES = -I./glm

SRCS = main.cpp game_engine.cpp
OBJS = $(SRCS:.cpp=.o)
EXEC = game_engine_linux

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@


clean:
	rm -f $(OBJS) $(EXEC)

rebuild: clean all

run: $(EXEC)
	./$(EXEC)

.PHONY: all clean rebuild run