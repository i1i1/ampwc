CC = gcc
CFLAGS = -Wall -ggdb -Iinclude -I../common/include -pthread `pkg-config --cflags libdrm`
LDFLAGS = -lm `pkg-config --libs wayland-server libdrm libudev`

XDG_PROTO = ../xdg-shell.xml
SRC = $(wildcard src/*.c)
HDR = $(wildcard include/*.h) include/xdg-shell-server.h

OBJ = $(subst src, build, $(SRC:.c=.o))
DEP = $(subst src, build, $(SRC:.c=.d))
COMMON_OBJ = $(wildcard ../common/build/*.o)

OUT = ../wlserv


all: include/xdg-shell-server.h $(DEP) $(OUT)

include/xdg-shell-server.h: $(XDG_PROTO)
	wayland-scanner server-header $< $@

$(OUT): $(OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

build/%.d: src/%.c
	$(CC) $(CFLAGS) -MM $< | sed -e "s/^[^ \t]\+\.o:/build\/&/" > $@

build/%.o: src/%.c build/%.d
	$(CC) $(CFLAGS) -c $< -o $@ `pkg-config --cflags libdrm`

test: all
	xterm -e $(OUT)

clean:
	rm -rf $(OBJ)
	rm -rf $(OUT)

fullclean: clean
	rm -rf $(dep)

.PHONY: clean fullclean test all deps
.SECONDARY: $(dep)

ifneq "$(MAKECMDGOALS)" "clean"
-include $(dep)
endif
