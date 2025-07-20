CC = gcc
CFLAGS = -Wall -g -pthread
SRC_DIR = src
BUILD_DIR = build

# 获取所有 .c 文件
SRCS := $(shell find $(SRC_DIR) -name '*.c')

# 将 .c -> .o 并替换路径
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

all: clean server

server: $(OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^

# 编译每个 .c 文件为对应的 .o 文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)/*
