CC = gcc
CFLAGS = -Wall -ggdb -Iinclude -I../common/include
LDFLAGS = -lm `pkg-config --libs wayland-client`

HDR = include/xdg-shell-client.h
OBJ = $(wildcard ../common/build/*.o)
PRE = include/xdg-shell-client.h
OUT = ../wlclient

XDG_PROTO = ../xdg-shell.xml

include ../common/gener.mk

$(PRE): $(XDG_PROTO)
	wayland-scanner client-header $< $@

test: all
	xterm -e $(OUT)

userclean:
	rm -f $(PRE)

.PHONY: test

