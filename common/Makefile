CC = gcc
CFLAGS = -Wall -ggdb -Iinclude -std=c99

XDG_PROTO = ../xdg-shell.xml
SRC = $(wildcard src/*.c) src/xdg-shell.c
HDR = $(wildcard include/*.h)

OBJ = $(subst src, build, $(SRC:.c=.o))
DEP = $(subst src, build, $(SRC:.c=.d))

all: src/xdg-shell.c $(DEP) $(OBJ)

src/xdg-shell.c: $(XDG_PROTO)
	wayland-scanner code $< $@

build/%.d: src/%.c
	$(CC) $(CFLAGS) -MM $< | sed -e "s/^[^ \t]\+\.o:/build\/&/" > $@

build/%.o: src/%.c build/%.d
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ)

fullclean: clean
	rm -rf $(dep)

.PHONY: clean fullclean all deps
.SECONDARY: $(DEP)

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEP)
endif

