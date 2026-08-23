CC = gcc
CFLAGS = -O3 -march=native -mtune=native -std=c99 -Wall -Wextra -Wpedantic
LDFLAGS = -lraylib -lwayland-client -lwayland-cursor -lwayland-egl -lxkbcommon -lGL -lm -lpthread -ldl -lrt

SRC_DIR = src
BUILD_DIR = build

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/tokenizer.c $(SRC_DIR)/evaluator.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

EXECUTABLE = bin/cf2x

.PHONY: all clean rebuild run

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@
	
run: $(EXECUTABLE)
	./$(EXECUTABLE)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

rebuild: clean all