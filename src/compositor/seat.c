#include <stdio.h>

#include <wayland-server.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include "macro.h"
#include "wl-server.h"

static const struct wl_seat_interface seat_interface = {
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

	wl_resource_set_implementation(resource, &wl_seat_interface,
				       data, NULL);
}

int
seat_init(struct amcs_compositor *ctx)
{
	ctx->shell = wl_global_create(ctx->display, &wl_seat_interface, 1, ctx, &bind_seat);
	if (!ctx->shell) {
		warning("can't create shell inteface");
		return 1;
	}

	return 0;
}
