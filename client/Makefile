CC = gcc
CFLAGS = -Wall -ggdb -Iinclude -I../common/include
LDFLAGS = -lm `pkg-config --libs wayland-client`

XDG_PROTO = ../xdg-shell.xml
SRC = $(wildcard src/*.c)
HDR = $(wildcard include/*.h) include/xdg-shell-server.h

OBJ = $(subst src, build, $(SRC:.c=.o))
DEP = $(subst src, build, $(SRC:.c=.d))
COMMON_OBJ = $(wildcard ../common/build/*.o)

OUT = ../wlclient


all: include/xdg-shell-client.h $(DEP) $(OUT)

include/xdg-shell-client.h: $(XDG_PROTO)
	wayland-scanner client-header $< $@

$(OUT): $(OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

build/%.d: src/%.c
	$(CC) $(CFLAGS) -MM $< | sed -e "s/^[^ \t]\+\.o:/build\/&/" > $@

build/%.o: src/%.c build/%.d
	$(CC) $(CFLAGS) -c $< -o $@

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
