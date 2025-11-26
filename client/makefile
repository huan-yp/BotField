# Makefile for Battlefield Backend

# 编译器和标志
CXX = g++
CXXFLAGS = -O2 -std=c++17 -Wall
INCLUDE = -Isrc/third_party
LDFLAGS = -fopenmp

# 目录
SRC_DIR = src
BUILD_DIR = build
THIRD_PARTY_DIR = src/third_party

# 源文件
SOURCES = $(SRC_DIR)/battlefield.cpp \
          $(THIRD_PARTY_DIR)/jsoncpp/jsoncpp.cpp

# 目标文件
OBJECTS = $(BUILD_DIR)/battlefield.o \
          $(BUILD_DIR)/jsoncpp.o

# 可执行文件
TARGET = $(BUILD_DIR)/main

# 默认目标
all: $(TARGET)

# 链接
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# 编译 battlefield.cpp
$(BUILD_DIR)/battlefield.o: $(SRC_DIR)/battlefield.cpp
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

# 编译 jsoncpp.cpp
$(BUILD_DIR)/jsoncpp.o: $(THIRD_PARTY_DIR)/jsoncpp/jsoncpp.cpp
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

# 清理
clean:
	@powershell -Command "if (Test-Path '$(BUILD_DIR)\*.o') { Remove-Item '$(BUILD_DIR)\*.o' -Force }"
	@powershell -Command "if (Test-Path '$(TARGET).exe') { Remove-Item '$(TARGET).exe' -Force }"
	@powershell -Command "if (Test-Path '$(TARGET)') { Remove-Item '$(TARGET)' -Force }"

# 重新编译
rebuild: clean all

# 运行
run: $(TARGET)
	$(TARGET)

.PHONY: all clean rebuild run
