OS      := $(shell uname -s)
CC      := clang
LD      := ld

# Use 'lld' if available.
ifneq (,$(shell which lld))
  LD := lld
endif

WARNS   :=                      \
  -Wall                         \
  -Wextra                       \
  -Wmissing-prototypes          \
  -Wstrict-prototypes           \
  -Wimplicit                    \
  -Wshadow                      \
  -Wint-conversion              \
  -Wundef                       \
  -Wconversion                  \
  -Wpedantic                    \
  -Wno-missing-braces           \
  -Wno-unused-function          \
  -Wno-unused-parameter         \
  -Wno-sign-conversion          \
  -Wvla                         \
  -Wimplicit-fallthrough        \

PREFIX      ?= /usr/local
BINDIR      ?= $(PREFIX)/bin
DATADIR     ?= $(PREFIX)/share
MANDIR      ?= $(DATADIR)/man
CFLAGS      := $(CFLAGS) -O0 -g -msse4.1 -fno-omit-frame-pointer -fstrict-aliasing -pedantic -std=c11 $(WARNS)
CFLAGS      += $(shell pkg-config --cflags glfw3 glew gl)
CPPFLAGS    := $(CPPFLAGS) -DDEBUG -DGLEW_STATIC
LDFLAGS     := $(LDFLAGS) -fuse-ld=$(LD) -lm $(shell pkg-config --libs glfw3 glew)

ifeq ($(OS),Darwin)
  LDFLAGS += -framework OpenGL
else
  LDFLAGS += $(shell pkg-config --libs gl)
endif

INCS        :=
CSRC        ?= $(wildcard *.c)
OBJ         := $(CSRC:.c=.o) $(SSRC:.s=.o)
TARGET      := px
FSAN        := -fsanitize=undefined -fsanitize=address                \
               -fsanitize=leak -fsanitize=null -fsanitize=return

default: $(TARGET)

debug: CFLAGS   += $(FSAN)
debug: CFLAGS   += -O0 -g
debug: CPPFLAGS += -DDEBUG
debug: LDFLAGS  := -lasan -lubsan $(LDFLAGS)
debug: default

analyze:
	$(CC) --analyze -Xanalyzer -analyzer-output=text $(INCS) $(CSRC)

scan-build:
	scan-build -V $(CC) $(INCS) $(CSRC) $(LDFLAGS)

%.o: %.c Makefile
	$(CC) -MMD -MP -MT $@ -MF $*.d -c $(CFLAGS) $(INCS) $(CPPFLAGS) $<

$(TARGET): $(OBJ)
	$(CC) $^ $(INCS) $(LDFLAGS) -o $@

install: $(TARGET)
	cp -p $(TARGET) $(BINDIR)/$(TARGET)

%.d: ;
-include $(OBJ:.o=.d)

clean:
	rm -f $(OBJ) *.d *.plist
	rm -f $(TARGET)

watch:
	scripts/watch-files
