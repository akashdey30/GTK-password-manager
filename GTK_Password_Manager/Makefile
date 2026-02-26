CC      := gcc
TARGET  := gtk-password-vault
BUILDDIR:= build

# ── pkg-config flags ────────────────────────────────────────
PKG_DEPS := gtk4 sqlite3 openssl
CFLAGS   := -std=c11 -Wall -Wextra -Wpedantic \
            -Iinclude \
            $(shell pkg-config --cflags $(PKG_DEPS)) \
            -DDEBUG
LDFLAGS  := $(shell pkg-config --libs $(PKG_DEPS))

# ── Source discovery ─────────────────────────────────────────
SRCS := $(shell find src -name '*.c')
OBJS := $(patsubst src/%.c,$(BUILDDIR)/%.o,$(SRCS))

# ── Default target ────────────────────────────────────────────
.PHONY: all clean run

all: $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "  LD  $@"

$(BUILDDIR)/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  CC  $<"

run: all
	./$(BUILDDIR)/$(TARGET)

clean:
	rm -rf $(BUILDDIR)