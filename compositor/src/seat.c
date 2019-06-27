#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <libinput.h>
#include <libudev.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <wayland-server.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include "common.h"
#include "macro.h"
#include "wl-server.h"

#define SEAT_NAME "seat0"

struct seat_connection {
	struct wl_resource *keyboard;
	struct wl_resource *pointer;
	struct wl_resource *seat;

	struct wl_list link;
};

struct seat {
	struct libinput *input;
	struct udev *udev;
	int ifd;			// libinput file descriptor
	int capabilities;

	struct wl_list clients;		// struct seat_connection *;
	struct seat_connection *focus;
};

static struct seat g_seat;

struct seat_connection *
seat_connection_new(struct wl_resource *seat)
{
	struct seat_connection * res;
	res = xmalloc(sizeof(*res));
	memset(res, 0, sizeof(*res));
	res->seat = seat;

	return res;
}

void
seat_connection_delete(struct seat_connection *con)
{
	if (con->link.next != NULL) {
		wl_list_remove(&con->link);
	}
	free(con);
}

static void
pointer_set_cursor(struct wl_client *client, struct wl_resource *resource,
	uint32_t serial, struct wl_resource *surface,
	int32_t hotspot_x, int32_t hotspot_y)
{
	error(1, "writeme");
}

static void
pointer_release(struct wl_client *client, struct wl_resource *resource)
{
	debug("unimplemented");
}

struct wl_pointer_interface pointer_interface = {
	.set_cursor = pointer_set_cursor,
	.release = pointer_release,
};

static int open_restricted(const char *path, int flags, void *user_data)
{
	return open(path, flags);
}

static void close_restricted(int fd, void *user_data)
{
	close(fd);
}

//actually not so restricted :-)
struct libinput_interface input_iface = {
	.open_restricted = open_restricted,
	.close_restricted = close_restricted,
};

static void
initialize_seat(struct seat *res)
{
	memset(res, 0, sizeof(*res));
	if ((res->udev = udev_new()) == NULL)
		error(1, "Can not create udev object.");
	res->input = libinput_udev_create_context(&input_iface, res, res->udev);
	assert(res->input);

	libinput_udev_assign_seat(res->input, SEAT_NAME);
	res->ifd = libinput_get_fd(res->input);
	wl_list_init(&res->clients);
}

static void
finalize_seat(struct seat *ps)
{
	struct seat_connection *con;
	if (ps == NULL)
		return;
	if (ps->udev)
		udev_unref(ps->udev);
	if (ps->input)
		libinput_unref(ps->input);
	wl_list_for_each(con, &g_seat.clients, link) {
		seat_connection_delete(con);
	}
}

static int
process_keyboard_event(struct amcs_compositor *ctx, struct libinput_event *ev)
{
	struct libinput_event_keyboard *k;
	uint32_t serial;
	uint32_t time, key;
	enum libinput_key_state state;
	enum wl_keyboard_key_state wlstate;

	debug("focus = %p", g_seat.focus);

	if (g_seat.focus == NULL || g_seat.focus->keyboard == NULL) {
		warning("can't send keyboard event, no client keyboard connection");
		return 1;
	}

	k = libinput_event_get_keyboard_event(ev);

	time = libinput_event_keyboard_get_time(k);
	key = libinput_event_keyboard_get_key(k);
	state = libinput_event_keyboard_get_key_state(k);
	if (state == LIBINPUT_KEY_STATE_RELEASED) {
		wlstate = WL_KEYBOARD_KEY_STATE_RELEASED;
	} else if (state == LIBINPUT_KEY_STATE_PRESSED) {
		wlstate = WL_KEYBOARD_KEY_STATE_PRESSED;
	} else {
		warning("unknown key state %d", state);
		return 1;
	}

	serial = wl_display_next_serial(ctx->display);
	debug("send (time, key, wlstate) (%d, %d, %d)", time, key, wlstate);
	wl_keyboard_send_key(g_seat.focus->keyboard, serial, time, key, wlstate);

	//amcs_compositor_send_key(key);
	/*
	wl_keyboard_send_key(wl_resource_from_link(&g_seat.resources)
	    WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP,;
	*/
	return 0;
}

static int
get_dev_caps(struct libinput_device *dev)
{
	int res = 0;
	if (libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_KEYBOARD))
		res |= WL_SEAT_CAPABILITY_KEYBOARD;
	if (libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_POINTER))
		res |= WL_SEAT_CAPABILITY_POINTER;;
	return res;
}

static void
update_capabilities(struct libinput_device *dev, int isAdd)
{
	struct seat_connection *con;
	int caps;

	assert(isAdd == 1 && "unimplemented yet");

	caps = get_dev_caps(dev);
	if ((g_seat.capabilities | caps) !=  caps) {
		g_seat.capabilities |= caps;
		wl_list_for_each(con, &g_seat.clients, link) {
			wl_seat_send_capabilities(con->seat, g_seat.capabilities);
		}
	}
}

int
notify_seat(int fd, uint32_t mask, void *data)
{
	struct amcs_compositor *ctx;
	struct seat *seat = &g_seat;
	struct libinput_event *ev = NULL;

	ctx = (struct amcs_compositor *) data;
	debug("notify_seat triggered");
	libinput_dispatch(seat->input);
	while ((ev = libinput_get_event(seat->input)) != NULL) {
		struct libinput_device *dev;
		int evtype;
		evtype = libinput_event_get_type(ev);
		switch (evtype) {
		case LIBINPUT_EVENT_DEVICE_ADDED:
			dev = libinput_event_get_device(ev);
			debug("device added sysname = %s, name %s", libinput_device_get_sysname(dev), libinput_device_get_name(dev));
			update_capabilities(dev, 1);
			break;
		case LIBINPUT_EVENT_DEVICE_REMOVED:
			debug("device removed");
			update_capabilities(dev, 0);
			break;
		case LIBINPUT_EVENT_KEYBOARD_KEY:
			process_keyboard_event(ctx, ev);
			break;
		case LIBINPUT_EVENT_POINTER_MOTION:
			debug("mouse moved");
			break;
		case LIBINPUT_EVENT_POINTER_BUTTON:
			debug("mouse key pressed");
			break;
		default:
			debug("next input event %p, type = %d", ev, evtype);
			break;
		}

		libinput_event_destroy(ev);
	}
	return 0;
}

static void
seat_get_pointer(struct wl_client *client, struct wl_resource *resource,
	uint32_t id)
{
	struct wl_resource *res;

	RESOURCE_CREATE(res, client, &wl_pointer_interface,
			wl_resource_get_version(resource), id);
	wl_resource_set_implementation(res, &pointer_interface, &g_seat, NULL);
}

static void
seat_get_keyboard(struct wl_client *client, struct wl_resource *resource,
	uint32_t id)
{
	warning("");
}

static void
seat_get_touch(struct wl_client *client, struct wl_resource *resource,
	uint32_t id)
{
	error(2, "not implemented");

}

static void
seat_release(struct wl_client *client, struct wl_resource *resource)
{
	warning("");
}

static const struct wl_seat_interface seat_interface = {
	.get_pointer = seat_get_pointer,
	.get_keyboard = seat_get_keyboard,
	.get_touch = seat_get_touch,
	.release = seat_release
};

static void
unbind_seat(struct wl_resource *resource)
{
	struct seat_connection *con;
	con = wl_resource_get_user_data(resource);
	assert(con);
	seat_connection_delete(con);
}

static void
bind_seat(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;
	struct seat_connection *con;

	debug("client %p", client);

	RESOURCE_CREATE(resource, client, &wl_seat_interface, version, id);

	con = seat_connection_new(resource);

	wl_resource_set_implementation(resource, &seat_interface,
				       con, unbind_seat);

	wl_list_insert(&g_seat.clients, &con->link);

	if (g_seat.capabilities)
		wl_seat_send_capabilities(resource, g_seat.capabilities);
}

int
seat_init(struct amcs_compositor *ctx)
{
	ctx->seat = wl_global_create(ctx->display, &wl_seat_interface, 6, ctx, &bind_seat);
	if (!ctx->seat) {
		warning("can't create seat inteface");
		return 1;
	}
	initialize_seat(&g_seat);

	wl_event_loop_add_fd(ctx->evloop, g_seat.ifd, WL_EVENT_READABLE, notify_seat, ctx);

	return 0;
}

int
seat_focus(struct wl_resource *res)
{
	struct wl_client *owner;
	struct seat_connection *con;

	debug("");
	owner = wl_resource_get_client(res);
	assert(owner != NULL);
	wl_list_for_each(con, &g_seat.clients, link) {
		struct wl_client *seat_owner;
		seat_owner = wl_resource_get_client(res);
		assert(seat_owner != NULL);
		if (seat_owner == owner) {
			debug("change owner");
			g_seat.focus = con;
			return 0;
		}
	}
	return 1;

}

int
seat_finalize(struct amcs_compositor *ctx)
{
	finalize_seat(&g_seat);
	if (ctx->seat)
		wl_global_destroy(ctx->seat);
	return 0;

}
