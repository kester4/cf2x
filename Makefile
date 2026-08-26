CC = gcc
CFLAGS = -O3 -march=native -mtune=native -std=c11 -Wall -Wextra -Wpedantic -I$(INCL_DIR)
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt

SRC_DIR   = src
INCL_DIR  = include
BUILD_DIR = build
BIN_DIR   = bin

ifeq ($(XDG_SESSION_TYPE), wayland)
	LDFLAGS += -lwayland-client -lwayland-cursor -lwayland-egl -lxkbcommon
endif

ifeq ($(XDG_SESSION_TYPE), x11)
	LDFLAGS += -lX11
endif

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

EXECUTABLE = bin/cf2x

.PHONY: all clean rebuild run

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@
	
run: $(EXECUTABLE)
	./$(EXECUTABLE)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

rebuild: clean all
