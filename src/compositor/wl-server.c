#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include "macro.h"
#include "seat.h"
#include "wl-server.h"
#include "xdg-shell.h"

#include "windows.h"
#include "tty.h"

struct amcs_compositor compositor_ctx;
static amcs_screen *screens;

struct amcs_surface *
amcs_surface_new()
{
	struct amcs_surface *res;

	res = malloc(sizeof(*res));

	memset(res, 0, sizeof(*res));
	wl_array_init(&res->surf_states);

	return res;
}

void
amcs_surface_free(struct amcs_surface *surf)
{
	free(surf);
}

static void
sig_surfaces_redraw(struct wl_listener *listener, void *data)
{
	struct amcs_compositor *ctx = (struct amcs_compositor *) data;
	struct amcs_surface *surf;

	debug("");

	wl_list_for_each(surf, &ctx->surfaces, link) {
		debug("next surface %p", surf);
	}
}

static void
delete_surface(struct wl_resource *resource)
{
	struct amcs_surface *mysurf;

	mysurf = wl_resource_get_user_data(resource);
	debug("");

	assert(mysurf);

	//TODO: refcount for multiple used surfaces?
	if (!mysurf->res) {
		return;
	}
	wl_list_remove(&mysurf->link);

	amcs_surface_free(mysurf);
}

void
surf_delete(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

void
surf_attach(struct wl_client *client, struct wl_resource *resource,
	struct wl_resource *buffer, int32_t x, int32_t y)
{
	struct amcs_surface *mysurf;

	mysurf = wl_resource_get_user_data(resource);

	mysurf->pending.x = x;
	mysurf->pending.y = y;

	debug("resource %p, buffer %p, (x; y) (%d; %d)", resource, buffer, x, y);
	if (!buffer) {
		warning("buffer == NULL!!!");
		return;
	}
	mysurf->pending.buf = wl_shm_buffer_get(buffer);
}

void
surf_damage(struct wl_client *client, struct wl_resource *resource,
	int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct amcs_surface *mysurf;

	mysurf = wl_resource_get_user_data(resource);
	debug("resource %p, need to redraw buf (x; y) (w; h) (%d; %d) (%d %d)",
		resource, x, y, width, height);
	(void)mysurf;
}

void
surf_frame(struct wl_client *client, struct wl_resource *resource,
	uint32_t callback)
{
	debug("%p", resource);
}

void
surf_commit(struct wl_client *client, struct wl_resource *resource)
{
	struct amcs_surface *mysurf;
	struct wl_shm_buffer *buf;
	void *data;
	int x, y;

	mysurf = wl_resource_get_user_data(resource);
	buf = mysurf->pending.buf;
	debug("recieved commit, need to redraw stuff");

	x = mysurf->pending.x;
	y = mysurf->pending.y;
	if (mysurf->w + x < 0 || mysurf->h + y < 0) {
		error(1, "wrong resize, negative coordinates");
		return;
	}
	wl_shm_buffer_begin_access(buf);

	data = wl_shm_buffer_get_data(buf);
	debug("data = %p", data);
	debug("data item = %x", ((uint8_t*)data)[0]);
	wl_shm_buffer_end_access(buf);
	debug("end!");

}

struct wl_surface_interface surface_interface = {
	.destroy = surf_delete,
	.attach = surf_attach,
	.damage = surf_damage,
	.frame = surf_frame,
	NULL, //set_opaque_region
	NULL, //set_input_region
	.commit = surf_commit,
};


static void
compositor_create_surface(struct wl_client *client,
	struct wl_resource *resource, uint32_t id)
{
	struct amcs_compositor *comp;
	struct amcs_surface *mysurf;

	mysurf = amcs_surface_new();
	comp = wl_resource_get_user_data(resource);
	wl_list_insert(&comp->surfaces, &mysurf->link);

	debug("create surface wl_client = %p, id = %d, surf = %p", client, id, mysurf);
	mysurf->res = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(resource), id);
	wl_resource_set_implementation(mysurf->res, &surface_interface, mysurf, delete_surface);
}

static void
compositor_create_region(struct wl_client *client,
			 struct wl_resource *resource, uint32_t id)
{
	debug("create region wl_client = %p, id = %d", client, id);
}

static const struct wl_compositor_interface compositor_interface = {
	.create_surface = compositor_create_surface,
	.create_region = compositor_create_region
};

static void
bind_compositor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	debug("");

	resource = wl_resource_create(client, &wl_compositor_interface,
				      version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &compositor_interface,
				       data, NULL);
}

static void
start_draw(void)
{
	screens = amcs_wind_get_screens("/dev/dri/card0");

/*
	if ((errno = pthread_create(&draw_thread, NULL, draw, NULL)) != 0)
		perror_and_ret(, "pthread_create()");

	return;
*/
}

static void
stop_draw(void)
{
/*
	if ((errno = pthread_cancel(draw_thread)) != 0)
		perror_and_ret(, "pthread_cancel()");

	if ((errno = pthread_join(draw_thread, NULL)) != 0)
		perror_and_ret(, "pthread_join()");
*/
	amcs_wind_free_screens(screens);
}

int
amcs_compositor_init(struct amcs_compositor *ctx)
{
	const char *sockpath = NULL;

	memset(ctx, 0, sizeof(*ctx));

	wl_list_init(&ctx->clients);
	wl_list_init(&ctx->surfaces);

	ctx->display = wl_display_create();
	if (!ctx->display) {
		warning("error wl_display_create");
		goto finalize;

	}
	debug("ctx ptr %p", ctx);
	debug("ctx->display created %p", ctx->display);
	sockpath = wl_display_add_socket_auto(ctx->display);

	if (!sockpath) {
		warning("error add_socket auto error");
		goto finalize;
	}
	debug("sockpath = %s", sockpath);
	setenv("WAYLAND_DISPLAY", sockpath, 1);

	ctx->evloop = wl_display_get_event_loop(ctx->display);
	if (!ctx->evloop) {
		warning("wl_display_get_event_loop err");
		goto finalize;
	}
	debug("event loop %p", ctx->evloop);

	wl_display_init_shm(ctx->display);

	debug("compositor iface version %d", wl_compositor_interface.version);
	// compositor stuff
	ctx->comp = wl_global_create(ctx->display, &wl_compositor_interface, 3, ctx, &bind_compositor);
	if (!ctx->comp) {
		warning("can't use compositor");
		goto finalize;
	}

	if (xdg_shell_init(ctx) != 0 ||
	    seat_init(ctx) != 0) {
		goto finalize;
	}

	debug("compositor created, adding redraw signal");
	wl_signal_init(&ctx->redraw_sig);
	ctx->redraw_listener.notify = sig_surfaces_redraw;
	wl_signal_add(&ctx->redraw_sig, &ctx->redraw_listener);

	return 0;

finalize:
	amcs_compositor_deinit(ctx);
	return 1;
}

void
amcs_compositor_deinit(struct amcs_compositor *ctx)
{
	if (ctx->display)
		wl_display_destroy(ctx->display);
	if (ctx->comp)
		wl_global_destroy(ctx->comp);
}

int
main(int argc, const char *argv[])
{
	int rc;

	debug("tty init");
	awc_tty_open(0);
	awc_tty_sethand(start_draw, stop_draw);
	awc_tty_activate();

	if (amcs_compositor_init(&compositor_ctx) != 0)
		return 1;

	debug("event loop dispatch");
	while (1) {
		rc = wl_event_loop_dispatch(compositor_ctx.evloop, 2000);

		if (rc < 0 && errno != EINTR) {
			warning("error at loop dispatch");
			break;
		}

		wl_signal_emit(&compositor_ctx.redraw_sig, &compositor_ctx);
		//debug("evloop rc = %d", rc);

		wl_display_flush_clients(compositor_ctx.display);
	}

	amcs_compositor_deinit(&compositor_ctx);

	return 0;
}

