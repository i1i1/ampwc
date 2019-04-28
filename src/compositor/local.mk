dir=compositor

XDG_PROTO=$(dir)/protocol/xdg-shell.xml

WL_OBJ = $(patsubst %,$(dir)/%, wl-server.o protocol/xdg-shell.o \
	xdg-shell.o seat.o)
WLC_OBJ =$(patsubst %,$(dir)/%, wl-client.o utils.o protocol/xdg-shell.o)

wl_proto = $(dir)/protocol/xdg-shell-server.h \
	  $(dir)/protocol/xdg-shell-client.h

$(dir)/protocol/xdg-shell-server.h:
	wayland-scanner server-header $(XDG_PROTO) $@

$(dir)/protocol/xdg-shell-client.h:
	wayland-scanner client-header $(XDG_PROTO) $@

$(dir)/protocol/xdg-shell.c:
	wayland-scanner code $(XDG_PROTO) $@


wlserv: $(wl_proto) $(WL_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) `pkg-config --libs wayland-server`

wlclient: $(wl_proto) $(WLC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) `pkg-config --libs wayland-client`


TARGETS += wlserv wlclient
CLEANUP += $(WL_OBJ) $(WLC_OBJ)
