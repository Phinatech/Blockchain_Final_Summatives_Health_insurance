# ─── ALU Health Insurance Blockchain — Makefile ──────────────────────────────
# Build: make          (default: aluhealth binary)
# Clean: make clean
# Run:   ./aluhealth
# ──────────────────────────────────────────────────────────────────────────────

CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -std=c11 -Iinclude
CFLAGS  += $(shell pkg-config --cflags openssl 2>/dev/null || echo "-I/usr/include/openssl")
LDFLAGS := -lm $(shell pkg-config --libs openssl 2>/dev/null || echo "-lssl -lcrypto")

TARGET  := aluhealth
SRCDIR  := src
OBJDIR  := obj

SRCS := $(wildcard $(SRCDIR)/*.c) main.c
OBJS := $(patsubst %.c, $(OBJDIR)/%.o, $(notdir $(SRCS)))

.PHONY: all clean

all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "[make] Built: $(TARGET)"

# src/*.c → obj/
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# main.c → obj/
$(OBJDIR)/main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJDIR) $(TARGET)

# ── Convenience targets ───────────────────────────────────────────────────────
run: all
	./$(TARGET)

check: all
	./$(TARGET) --selftest
