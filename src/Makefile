CFLAGS  = -Wall -ggdb
LDFLAGS =
SERV_FLAGS = -lm -pthread `pkg-config --libs wayland-server libdrm libudev`
CLIENT_FLAGS = `pkg-config --libs wayland-client`

XDG_PROTO = protocol/xdg-shell.xml

WL_SRC = $(wildcard ./compositor/*.c)
WLC_SRC = $(wildcard ./client/*.c)

WL_OBJ = $(WL_SRC:.c=.o) protocol/xdg-shell.o
WLC_OBJ = $(WLC_SRC:.c=.o) protocol/xdg-shell.o

TARGETS = wlserv wlclient

wl_proto = protocol/xdg-shell-server.h
wlc_proto = protocol/xdg-shell-client.h


all: build

build: $(TARGETS)

wlserv: $(WL_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SERV_FLAGS) -o $@ $^

wlclient: $(WLC_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(CLIENT_FLAGS) -o $@ $^


compositor/%.o: compositor/%.c $(wl_proto)
	$(CC) $(CFLAGS) $(LDFLAGS) -I protocol -c -o $@ $< `pkg-config --cflags libdrm`

client/%.o: client/%.c $(wlc_proto)
	$(CC) $(CFLAGS) $(LDFLAGS) -I protocol -I compositor -c -o $@ $<

protocol/%.o: protocol/%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $<


protocol/xdg-shell-server.h: $(XDG_PROTO)
	wayland-scanner server-header $< $@

protocol/xdg-shell-client.h: $(XDG_PROTO)
	wayland-scanner client-header $< $@

protocol/xdg-shell.c: $(XDG_PROTO)
	wayland-scanner code $< $@


clean:
	rm -f $(WL_OBJ) $(WLC_OBJ) $(TARGETS) $(wl_proto) $(wlc_proto) protocol/xdg-shell.c

