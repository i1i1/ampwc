#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>

#include <string.h>

#include <wayland-client.h>

#include "protocol/xdg-shell-client.h"
#include "macro.h"
#include "utils.h"

#define WIN_WIDTH 800
#define WIN_HEIGHT 600

struct client_ctx {
	struct wl_compositor *comp;
	struct wl_shm *shm;
	struct wl_shm_pool *pool;

	struct xdg_wm_base *shell;
	struct wl_seat *seat;

	struct wl_surface *surf;
	struct xdg_surface *xdgsurf;
	struct xdg_toplevel *toplevel;

	//surface size
	int h;
	int w;

	struct wl_buffer *buf;
	void *data;
	int datasz;

	struct wl_event_source *timer_ev;
} g_ctx;

void
toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
	int32_t width, int32_t height, struct wl_array *states)
{
	struct client_ctx *ctx;

	debug("width = %d, height = %d", width, height);

	ctx = data;
	ctx->h = height;
	ctx->w = width;
}

void
toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	debug("close request!!");
	exit(1);
}

void
xdgsurf_configure(void *data, struct xdg_surface *xdg_surface,
	uint32_t serial)
{
	struct client_ctx *ctx;

	debug("serial = %d", serial);
	//TODO: resize surface
	ctx = data;
	xdg_surface_ack_configure(ctx->xdgsurf, serial);
}

struct xdg_surface_listener xdgsurf_listener = {
	.configure = xdgsurf_configure
};

struct xdg_toplevel_listener toplevel_listener = {
	.configure = toplevel_configure,
	.close = toplevel_close,
};

void
global_add(void *our_data, struct wl_registry *registry, uint32_t name,
	const char *interface, uint32_t version)
{
	debug("global_add iface = %s nm %d, version %d", interface, name, version);
	if (strcmp(interface, "wl_compositor") == 0) {
		g_ctx.comp = wl_registry_bind(registry, name, &wl_compositor_interface, 3);
	} else if (strcmp(interface, "wl_shm") == 0) {
		g_ctx.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, "wl_seat") == 0) {
		g_ctx.seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
	} else if(strcmp(interface, "xdg_wm_base") == 0) {
		debug("@WRITEME xdg_shell initialize");
		g_ctx.shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	}
}

void
global_remove(void *our_data, struct wl_registry *registry, uint32_t name)
{
	debug("global_remove nm %d", name);
}

struct wl_registry_listener registry_listener = {
	.global = global_add,
	.global_remove = global_remove
};

void
paint_init(struct client_ctx *ctx)
{
	int fd;
	int stride, mapsz;

	fd = tempfile();

	ctx->h = WIN_HEIGHT;
	ctx->w = WIN_WIDTH;

	stride =  ctx->w * 4;
	mapsz = stride * ctx->h;

	debug("pain_init stride %d!", stride);
	posix_fallocate(fd, 0, mapsz);

	ctx->data = mmap(NULL, mapsz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ctx->data == NULL) {
		warning("mmap error :-(");
		exit(1);
	}
	ctx->datasz = mapsz;
	memset(ctx->data, 0x42, mapsz);

	ctx->pool = wl_shm_create_pool(ctx->shm, fd, mapsz);

	ctx->buf = wl_shm_pool_create_buffer(ctx->pool, 0, ctx->w, ctx->h, stride, WL_SHM_FORMAT_ARGB8888);
	debug("wl_buffer created = %p", ctx->buf);


	//xdg_surface_set_window_geometry(ctx->xdgsurf, 0, 0, 600, 800);
	wl_surface_attach(ctx->surf, ctx->buf, ctx->w, ctx->h);
	wl_surface_commit(ctx->surf);
}

void
paint_surface(struct wl_surface *surf)
{
	memset(g_ctx.data, random() & 0xff, g_ctx.datasz);
	wl_surface_commit(surf);
}

int
main(int argc, const char *argv[])
{
	struct wl_display *display = wl_display_connect(NULL);
	int rc, niter;

	if (display == NULL) {
		warning("can't connect to display");
		return 1;
	}

	struct wl_registry *registry = wl_display_get_registry(display);

	if (registry == NULL) {
		warning("can't get registry");
		return 2;
	}

	void *our_data = NULL;	/* arbitrary state you want to keep around */;
	rc = wl_registry_add_listener(registry, &registry_listener, our_data);
	if (rc != 0) {
		warning("wl_registry_add_listener rc = %d", rc);
		goto finalize;
	}

	debug("roundtrip");
	wl_display_roundtrip(display);

	if (!g_ctx.comp) {
		warning("can't get compositor interface");
		goto finalize;
	}

	if (g_ctx.comp == NULL)
		error(1, "can't get compositor interface");
	if (g_ctx.shell == NULL)
		error(2, "can't get xdg_shell interface");

	g_ctx.surf = wl_compositor_create_surface(g_ctx.comp);
	if (!g_ctx.surf) {
		warning("can't get surface");
		goto finalize;
	}
	g_ctx.xdgsurf = xdg_wm_base_get_xdg_surface(g_ctx.shell, g_ctx.surf);
	g_ctx.toplevel = xdg_surface_get_toplevel(g_ctx.xdgsurf);

	xdg_surface_add_listener(g_ctx.xdgsurf, &xdgsurf_listener, &g_ctx);
	xdg_toplevel_add_listener(g_ctx.toplevel, &toplevel_listener, &g_ctx);

	paint_init(&g_ctx);

	debug("start dispatching stuff");
	niter = 0;
	while (1) {
		niter++;
		debug("wainting for events");
		rc = wl_display_dispatch(display);
		if (rc < 0) {
			warning("dispatch error %d", rc);
			break;
		} else {
			debug("dispatched %d events", rc);
		}
		paint_surface(g_ctx.surf);
		//wl_surface_destroy(surf);
	}

	debug("wayland client finalize!");

finalize:
	debug("finalize");
	if (display) {
		wl_display_disconnect(display);
	}

	return 0;
}