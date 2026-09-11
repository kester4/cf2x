CC = gcc
CFLAGS = -O3 -flto -std=c11 -Wall -Wextra -Wpedantic -I$(INCL_DIR)
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt

mode ?= default

SRC_DIR   = src
INCL_DIR  = include
BUILD_DIR = build
BIN_DIR   = bin

ifdef HIGHDPI
    CFLAGS += -DHIGH_DPI
endif

ifeq ($(mode), default)
	CFLAGS += -march=native -mtune=native

	ifeq ($(XDG_SESSION_TYPE), wayland)
		LDFLAGS += -lwayland-client -lwayland-cursor -lwayland-egl -lxkbcommon
	else
		LDFLAGS += -lX11
	endif

else ifeq ($(mode), release)
	CFLAGS += -march=x86-64 -mtune=generic -s -DNDEBUG -DRELEASE_BUILD
	LDFLAGS += -lX11

endif

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

EXECUTABLE = $(BIN_DIR)/cf2x

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
