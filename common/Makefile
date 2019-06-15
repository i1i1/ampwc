CC = gcc
CFLAGS = -Wall -ggdb -Iinclude -std=c99

XDG_PROTO = ../xdg-shell.xml
SRC = src/xdg-shell.c
PRE = src/xdg-shell.c

include ../common/gener.mk

$(PRE): $(XDG_PROTO)
	$(call prettify, GEN, $@, \
	    wayland-scanner code $< $@)

userclean:
	$(call prettify, RM, $(PRE), \
	    rm -f $(PRE))

