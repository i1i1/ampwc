#include <assert.h>
#include <stdio.h>

#include <wayland-server.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include "protocol/xdg-shell-server.h"

#include "macro.h"
#include "wl-server.h"

void
default_stuff()
{
	debug("CHECK");
}

static const struct xdg_toplevel_interface toplevel_interface = {
	.destroy = NULL,
	.set_parent = NULL,
	.set_title = NULL,
	.set_app_id = NULL,
	.show_window_menu = NULL,
	.move = NULL,
	.resize = NULL,
	.set_max_size = NULL,
	.set_min_size = NULL,
	.set_maximized = NULL,
	.unset_maximized = NULL,
	.set_fullscreen = NULL,
	.set_minimized = NULL,
};


void
window_init(struct amcs_surface *mysurf)
{
	uint32_t serial;
	uint32_t *newst;

	serial = wl_display_next_serial(compositor_ctx.display);

	mysurf->pending.xdg_serial = serial;
	debug("generated serial = %d", serial);
	newst = wl_array_add(&mysurf->surf_states, sizeof(*newst));
	*newst = XDG_TOPLEVEL_STATE_ACTIVATED;

	xdg_toplevel_send_configure(mysurf->xdgtopres, 700, 500, &mysurf->surf_states);

	xdg_surface_send_configure(mysurf->xdgres, serial);
}

static void
surf_get_toplevel(struct wl_client *client,
	struct wl_resource *resource, uint32_t id)
{
	struct amcs_surface *mysurf;
	mysurf = wl_resource_get_user_data(resource);

	debug("mysurf %p", mysurf);
	mysurf->xdgtopres = wl_resource_create(client, &xdg_toplevel_interface, wl_resource_get_version(resource), id);
	wl_resource_set_implementation(mysurf->xdgtopres, &toplevel_interface, mysurf, NULL);

	window_init(mysurf);
}

static void
surf_set_window_geometry(struct wl_client *client, struct wl_resource *resource,
	int32_t x, int32_t y, int32_t width, int32_t height)
{
	debug("x = %d y = %d w = %d h = %d", x, y, width, height);
}

static void
surf_ack_configure(struct wl_client *client,
	struct wl_resource *resource, uint32_t serial)
{
	struct amcs_surface *mysurf;
	mysurf = wl_resource_get_user_data(resource);

	debug("serial = %d", serial);
	assert(serial == mysurf->pending.xdg_serial);
}


static const struct xdg_surface_interface surface_interface = {
	.destroy = NULL,
	.get_toplevel = surf_get_toplevel,
	.get_popup = NULL,
	.set_window_geometry = surf_set_window_geometry,
	.ack_configure = surf_ack_configure
};

void
wmbase_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

void
wmbase_create_positioner(struct wl_client *client, struct wl_resource *resource,
	uint32_t id)
{
	debug("");
}

void
wmbase_get_xdg_surface(struct wl_client *client, struct wl_resource *resource,
	uint32_t id, struct wl_resource *surface)
{
	struct amcs_surface *mysurf;

	mysurf = wl_resource_get_user_data(surface);

	mysurf->xdgres = wl_resource_create(client, &xdg_surface_interface, wl_resource_get_version(resource), id);
	wl_resource_set_implementation(mysurf->xdgres, &surface_interface, mysurf, NULL);
}

void
wmbase_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	debug("serial = %x", serial);
}

static const struct xdg_wm_base_interface xdg_base_interface = {
	.destroy = wmbase_destroy,
	.create_positioner = wmbase_create_positioner,
	.get_xdg_surface = wmbase_get_xdg_surface,
	.pong = wmbase_pong
};

static void
bind_shell(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	debug("");

	resource = wl_resource_create(client, &xdg_wm_base_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &xdg_base_interface,
				       data, NULL);
}

int
xdg_shell_init(struct amcs_compositor *ctx)
{
	debug("shm iface version %d", xdg_wm_base_interface.version);
	ctx->shell = wl_global_create(ctx->display, &xdg_wm_base_interface, 1, ctx, &bind_shell);
	if (!ctx->shell) {
		warning("can't create shell inteface");
		return 1;
	}

	return 0;
}

