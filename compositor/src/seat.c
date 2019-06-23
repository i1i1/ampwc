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

#include "macro.h"
#include "wl-server.h"

#define SEAT_NAME "seat0"

struct seat {
	struct libinput *input;
	struct udev *udev;

	pvector devices;		// TODO: struct ?
	int ifd;			// libinput file descriptor
};

struct seat *g_seat;

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

static struct seat *
initialize_seat()
{
	struct seat *res;

	res = xmalloc(sizeof(*res));
	memset(res, 0, sizeof(*res));
	if ((res->udev = udev_new()) == NULL)
		error(1, "Can not create udev object.");
	res->input = libinput_udev_create_context(&input_iface, res, res->udev);
	assert(res->input);

	libinput_udev_assign_seat(res->input, SEAT_NAME);
	res->ifd = libinput_get_fd(res->input);
	pvector_init(&res->devices, xrealloc);

	return res;
}

static void
finalize_seat(struct seat **ps)
{
	struct seat *tmp = *ps;

	if (tmp == NULL)
		return;
	if (tmp->udev)
		udev_unref(tmp->udev);
	if (tmp->input)
		libinput_unref(tmp->input);
	pvector_free(&tmp->devices);
	*ps = NULL;
}

int
notify_seat(int fd, uint32_t mask, void *data)
{
	struct amcs_ctx *ctx;
	struct seat *seat = g_seat;
	struct libinput_event *ev = NULL;

	ctx = (struct amcs_ctx*) data;
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
			break;
		case LIBINPUT_EVENT_DEVICE_REMOVED:
			debug("device removed");
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
	debug("");
}

static void
seat_get_keyboard(struct wl_client *client, struct wl_resource *resource,
	uint32_t id)
{
	debug("");
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

}

static const struct wl_seat_interface seat_interface = {
	.get_pointer = seat_get_pointer,
	.get_keyboard = seat_get_keyboard,
	.get_touch = seat_get_touch,
	.release = seat_release
};

static void
bind_seat(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	debug("");

	resource = wl_resource_create(client, &wl_seat_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &seat_interface,
				       data, NULL);

	wl_seat_send_capabilities(resource, WL_SEAT_CAPABILITY_POINTER |
				  WL_SEAT_CAPABILITY_KEYBOARD);
}

int
seat_init(struct amcs_compositor *ctx)
{
	ctx->seat = wl_global_create(ctx->display, &wl_seat_interface, 6, ctx, &bind_seat);
	if (!ctx->seat) {
		warning("can't create seat inteface");
		return 1;
	}
	g_seat = initialize_seat();
	assert(g_seat);

	wl_event_loop_add_fd(ctx->evloop, g_seat->ifd, WL_EVENT_READABLE, notify_seat, ctx);

	return 0;
}

int
seat_finalize(struct amcs_compositor *ctx)
{
	finalize_seat(&g_seat);
	if (ctx->seat)
		wl_global_destroy(ctx->seat);
	return 0;

}
