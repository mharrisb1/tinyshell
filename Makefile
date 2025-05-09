CC       := cc
CFLAGS   := -Wall -Wextra -std=c2x -Iinclude -Ivendor

SRC_DIR    := src
VENDOR_DIR := vendor
BUILD_DIR  := build

SRCS := $(shell find $(SRC_DIR) $(VENDOR_DIR) -type f -name '*.c' -not -name 'example.c')
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

TARGET := $(BUILD_DIR)/tiny

PREFIX ?= /usr/local
BINDIR   := $(PREFIX)/bin
INSTALL  := install

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "Built $@"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled: $< -> $@"

install: all
	@mkdir -p $(DESTDIR)$(BINDIR)
	sudo $(INSTALL) -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/tiny
	@echo "Installed to $(DESTDIR)$(BINDIR)/tiny"

uninstall:
	@echo "Uninstalling $(DESTDIR)$(BINDIR)/tiny (may require sudo)"
	- sudo rm -f "$(DESTDIR)$(BINDIR)/tiny"
	@echo "Removed $(DESTDIR)$(BINDIR)/tiny"

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned build artifacts."
