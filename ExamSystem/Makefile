# Makefile for Course-Based Examination System (macOS)

# Compiler
CXX = g++

# Homebrew OpenSSL paths
OPENSSL_PREFIX := $(shell brew --prefix openssl)

# Compiler flags
CXXFLAGS = -std=c++11 -Wall -Iinclude -I$(OPENSSL_PREFIX)/include

# Linker flags
LDFLAGS = -lfltk -lsqlite3 -L$(OPENSSL_PREFIX)/lib -lcrypto

# Directories
SRC_DIR = src
BUILD_DIR = build
DB_DIR = database

# Source files
SOURCES = \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/DatabaseManager.cpp \
	$(SRC_DIR)/LoginWindow.cpp \
	$(SRC_DIR)/AdminDashboard.cpp \
	$(SRC_DIR)/CourseSelectionWindow.cpp \
	$(SRC_DIR)/InstructionsWindow.cpp \
	$(SRC_DIR)/ExamWindow.cpp \
	$(SRC_DIR)/ResultWindow.cpp \
	$(SRC_DIR)/User.cpp \
	$(SRC_DIR)/Course.cpp \
	$(SRC_DIR)/Question.cpp \
	$(SRC_DIR)/Result.cpp \
	$(SRC_DIR)/Utils.cpp

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Executable name
TARGET = exam_system

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(DB_DIR)

# Link
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete. Run with ./$(TARGET)"

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)

# Clean everything including database
cleanall: clean
	rm -rf $(DB_DIR)

# Rebuild from scratch
rebuild: clean all

# Run
run: all
	./$(TARGET)

.PHONY: all clean cleanall rebuild run directories