CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

# Name of the executable
TARGET = lexer_test.exe

# Source files
SRCS = src/main.cpp src/lexer/lexer.cpp src/common/utils.cpp
OBJS = main.o lexer.o utils.o

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compile main
main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) -c src/main.cpp

# Compile lexer
lexer.o: src/lexer/lexer.cpp src/lexer/lexer.hpp
	$(CXX) $(CXXFLAGS) -c src/lexer/lexer.cpp

# Compile utils
utils.o: src/common/utils.cpp src/common/utils.hpp
	$(CXX) $(CXXFLAGS) -c src/common/utils.cpp

# Clean objects and executable
# Note: uses CMD 'del' syntax or fallback to 'rm' if using unix-like terminal on Windows
clean:
	del /Q /F $(OBJS) $(TARGET) 2>nul || rm -f $(OBJS) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)
