CC = gcc
CFLAGS = -Wall -ggdb -Iinclude -I../common/include
LDFLAGS = -lm `pkg-config --libs wayland-client`
TERM = xterm

HDR = include/xdg-shell-client.h
OBJ = $(wildcard ../common/build/*.o)
PRE = include/xdg-shell-client.h
OUT = ../wlclient

XDG_PROTO = ../xdg-shell.xml

include ../common/gener.mk

$(PRE): $(XDG_PROTO)
	$(call prettify, GEN, $@, \
	    wayland-scanner client-header $< $@)

test: all
	$(call prettify, RUN, $(OUT), \
	    $(TERM) -e "$(OUT); cat >/dev/null")

userclean:
	$(call prettify, RM, $(PRE), \
	    rm -f $(PRE))

.PHONY: test

