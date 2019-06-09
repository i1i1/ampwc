CC = gcc
CFLAGS = -Wall -ggdb -Iinclude -I../common/include -pthread `pkg-config --cflags libdrm`
LDFLAGS = -lm `pkg-config --libs wayland-server libdrm libudev`
TERM = xterm

XDG_PROTO = ../xdg-shell.xml
HDR = include/xdg-shell-server.h
PRE = include/xdg-shell-server.h
OBJ = $(wildcard ../common/build/*.o)

OUT = ../wlserv

include ../common/gener.mk

include/xdg-shell-server.h: $(XDG_PROTO)
	wayland-scanner server-header $< $@

test: all
	$(TERM) -e "$(OUT); cat >/dev/null"

userclean:
	rm -f $(PRE)

.PHONY: test

