CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

# Nama executable dan foldernya
BIN_DIR = bin
TARGET = $(BIN_DIR)/arion.exe

# File objek
OBJS = main.o lexer.o utils.o token.o \
       parser_core.o parser_toplevel.o \
       parser_declarations.o parser_statements.o parser_expressions.o

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

# Compile parser
parser_core.o: src/parser/parser_core.cpp src/parser/parser.hpp src/parser/parse_tree.hpp
	$(CXX) $(CXXFLAGS) -c src/parser/parser_core.cpp

parser_toplevel.o: src/parser/parser_toplevel.cpp src/parser/parser.hpp src/parser/parse_tree.hpp
	$(CXX) $(CXXFLAGS) -c src/parser/parser_toplevel.cpp

parser_declarations.o: src/parser/parser_declarations.cpp src/parser/parser.hpp src/parser/parse_tree.hpp
	$(CXX) $(CXXFLAGS) -c src/parser/parser_declarations.cpp

parser_statements.o: src/parser/parser_statements.cpp src/parser/parser.hpp src/parser/parse_tree.hpp
	$(CXX) $(CXXFLAGS) -c src/parser/parser_statements.cpp

parser_expressions.o: src/parser/parser_expressions.cpp src/parser/parser.hpp src/parser/parse_tree.hpp
	$(CXX) $(CXXFLAGS) -c src/parser/parser_expressions.cpp

# Clean
clean:
	rm -f $(OBJS)
	rm -rf $(BIN_DIR)

# Run
run: $(TARGET)
	./$(TARGET)