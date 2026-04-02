CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

# Nama executable dan foldernya
BIN_DIR = bin
TARGET = $(BIN_DIR)/lexer_test.exe

# File objek
OBJS = main.o lexer.o utils.o token.o

# Default target
all: $(TARGET)

# Membuat folder bin dan Link executable
$(TARGET): $(OBJS)
	mkdir -p $(BIN_DIR)
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

# Compile token
token.o: src/common/token.cpp src/common/token.hpp
	$(CXX) $(CXXFLAGS) -c src/common/token.cpp

# Clean (Menggunakan perintah Linux standar)
clean:
	rm -f $(OBJS)
	rm -rf $(BIN_DIR)

# Run
run: $(TARGET)
	./$(TARGET)