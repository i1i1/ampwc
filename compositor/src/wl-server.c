#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <wayland-server.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include "common.h"
#include "macro.h"
#include "seat.h"
#include "wl-server.h"
#include "xdg-shell.h"

#include "udev.h"
#include "window.h"
#include "tty.h"

struct amcs_compositor compositor_ctx = {0};

struct amcs_client *
amcs_client_new(struct wl_client *client)
{
	struct amcs_client *res;

	res = xmalloc(sizeof(*res));
	memset(res, 0, sizeof(*res));
	res->client = client;
	return res;
}

void
amcs_client_free(struct amcs_client *c)
{
	free(c);
}

#define DEFAULT_TITLE "application"
#define DEFAULT_APPID "app"
struct amcs_surface *
amcs_surface_new()
{
	struct amcs_surface *res;

	res = xmalloc(sizeof(*res));
	memset(res, 0, sizeof(*res));
	wl_array_init(&res->surf_states);

	res->app_id = DEFAULT_APPID;
	res->title = DEFAULT_TITLE;

	return res;
}

void
amcs_surface_free(struct amcs_surface *surf)
{
	struct amcs_compositor *ctx = &compositor_ctx;
	if (surf->aw) {
		struct amcs_screen *s;

		//TODO: choose screen, choose another window
		int nroot;
		nroot = 0;
		s = pvector_get(&ctx->screens, nroot);
		if (surf->aw == s->curwin) {
			s->curwin = NULL;
		}

		amcs_win_orphain(surf->aw);

	}
	wl_array_release(&surf->surf_states);
	free(surf);
}

static void
sig_surfaces_redraw(struct wl_listener *listener, void *data)
{
	struct amcs_compositor *ctx = (struct amcs_compositor *) data;
	int i;

	debug("");

	/*
	struct amcs_surface *surf;
	wl_list_for_each(surf, &ctx->surfaces, link) {
		debug("next surface %p", surf);
	}
	*/
	for (i = 0; i < pvector_len(&ctx->screens); i++) {
		struct amcs_screen *s = pvector_get(&ctx->screens, i);
		debug("next screen");
		amcs_container_debug(s->root);
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

static void
surf_delete(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
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

static void
surf_damage(struct wl_client *client, struct wl_resource *resource,
	int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct amcs_surface *mysurf;

	mysurf = wl_resource_get_user_data(resource);
	debug("resource %p, need to redraw buf (x; y) (w; h) (%d; %d) (%d %d)",
		resource, x, y, width, height);
	(void)mysurf;
}

static void
surf_frame(struct wl_client *client, struct wl_resource *resource,
	uint32_t id)
{
	struct wl_resource *res;
	struct amcs_surface *mysurf;
	mysurf = wl_resource_get_user_data(resource);

	RESOURCE_CREATE(res, client, &wl_callback_interface, 1, id);
	wl_resource_set_implementation(res, NULL, NULL, NULL);
	mysurf->redraw_cb = res;
	warning("%p", resource);
}

static void
surf_set_opaque_region(struct wl_client *client,
	struct wl_resource *resource, struct wl_resource *region)
{
	warning("");
}

static void
surf_set_input_region(struct wl_client *client,
	 struct wl_resource *resource,
	 struct wl_resource *region)
{
	warning("");
}

static void
surf_commit(struct wl_client *client, struct wl_resource *resource)
{
	struct amcs_surface *mysurf;
	struct wl_shm_buffer *buf;
	void *data;
	int x, y;

	mysurf = wl_resource_get_user_data(resource);
	buf = mysurf->pending.buf;
	debug("recieved commit, need to redraw stuff");
	if (buf == NULL) {
		warning("nothing to commit, ignore request");
		return;
	}

	x = mysurf->pending.x;
	y = mysurf->pending.y;
	if (mysurf->w + x < 0 || mysurf->h + y < 0) {
		error(1, "TODO, negative coordinates");
	}
	wl_shm_buffer_begin_access(buf);

	data = wl_shm_buffer_get_data(buf);
	debug("data = %p", data);
	if (mysurf->aw) {
		int bufsz, h, w;
		int format;

		h = wl_shm_buffer_get_height(buf);
		w = wl_shm_buffer_get_width(buf);
		debug("try to commit buf, (x, y) (%d, %d), (w, h) (%d, %d)",
		      x, y, w, h);

		format = wl_shm_buffer_get_format(buf);
		if (format != WL_SHM_FORMAT_ARGB8888 &&
		    format != WL_SHM_FORMAT_XRGB8888) {
			warning("unknown buffer format, ignore");
			goto finalize;
		}

		bufsz = h * w * 4;
		if (mysurf->aw->buf.sz < bufsz)
			mysurf->aw->buf.dt = xrealloc(mysurf->aw->buf.dt, bufsz);
		mysurf->aw->buf.sz = bufsz;
		mysurf->aw->buf.h = h;
		mysurf->aw->buf.w = w;

		memcpy(mysurf->aw->buf.dt, data, bufsz);
		amcs_win_commit(mysurf->aw);
	}
	debug("data[0] = %x", ((uint8_t*)data)[0]);
finalize:
	wl_shm_buffer_end_access(buf);
	debug("end!");
}

static void
surf_set_buffer_transform(struct wl_client *client,
	struct wl_resource *resource, int32_t transform)
{
	warning("");
}

static void
surf_set_buffer_scale(struct wl_client *client,
	struct wl_resource *resource, int32_t scale)
{
	warning("");
}

struct wl_surface_interface surface_interface = {
	.destroy = surf_delete,
	.attach = surf_attach,
	.damage = surf_damage,
	.frame = surf_frame,
	.set_opaque_region = surf_set_opaque_region, //set_opaque_region
	.set_input_region = surf_set_input_region, //set_input_region
	.commit = surf_commit,
	.set_buffer_transform = surf_set_buffer_transform,
	.set_buffer_scale = surf_set_buffer_scale,
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
	RESOURCE_CREATE(mysurf->res, client, &wl_surface_interface, wl_resource_get_version(resource), id);
	wl_resource_set_implementation(mysurf->res, &surface_interface, mysurf, delete_surface);
}

static void
region_destroy(struct wl_client *client, struct wl_resource *resource)
{
	warning("");
}

static void
region_add(struct wl_client *client, struct wl_resource *resource,
	int32_t x, int32_t y, int32_t width, int32_t height)
{
	warning("");
}

static void
region_subtract(struct wl_client *client, struct wl_resource *resource,
	int32_t x, int32_t y, int32_t width, int32_t height)
{
	warning("");
}

struct wl_region_interface region_interface = {
	.destroy = region_destroy,
	.add = region_add,
	.subtract = region_subtract,
};

static void
compositor_create_region(struct wl_client *client,
			 struct wl_resource *resource, uint32_t id)
{
	struct wl_resource *res;

	//TODO
	debug("create region wl_client = %p, id = %d", client, id);
	RESOURCE_CREATE(res, client, &wl_region_interface,
			wl_resource_get_version(resource), id);
	wl_resource_set_implementation(res, &region_interface, resource, NULL);
}

static const struct wl_compositor_interface compositor_interface = {
	.create_surface = compositor_create_surface,
	.create_region = compositor_create_region
};

static void
unbind_compositor(struct wl_resource *resource)
{
	struct amcs_client *c;

	debug("");
	c = amcs_get_client(resource);
	if (c == NULL) {
		warning("unbind compositor error, can't locate client");
		return;
	}
	wl_list_remove(&c->link);
	amcs_client_free(c);
}

static void
bind_compositor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;
	struct amcs_compositor *ctx;
	struct amcs_client *c;

	debug("");

	ctx = data;
	RESOURCE_CREATE(resource, client, &wl_compositor_interface, version, id);
	wl_resource_set_implementation(resource, &compositor_interface,
				       data, unbind_compositor);
	c = amcs_client_new(client);
	wl_list_insert(&ctx->clients, &c->link);
}

static void
start_draw(void)
{
	int i;
	char path[PATH_MAX];
	const char **cards;
	struct amcs_compositor *ctx = &compositor_ctx;
	int nscreens;

	debug("start draw");
	cards = amcs_udev_get_cardnames();

	assert(pvector_len(&ctx->screens) == 0);

	for (i = 0; cards[i] != NULL; ++i) {
		snprintf(path, sizeof(path), "%s%s", DRIPATH, cards[i]);
		amcs_screens_add(&ctx->screens, path);
	}

	nscreens = pvector_len(&ctx->screens);
	if (nscreens == 0)
		error(1, "screens not found");

	struct amcs_screen *s;

	for (int i = 0; i < nscreens; i++) {
		struct amcs_container *wt;
		s = pvector_get(&ctx->screens, i);

		if (s->root) {
			wt = amcs_container_new(NULL, CONTAINER_VSPLIT);
			amcs_container_set_screen(wt, s);
		}
		amcs_container_set_screen(s->root, s);
	}
	amcs_udev_free_cardnames(cards);
}

static void
stop_draw(void)
{
	struct amcs_compositor *ctx = &compositor_ctx;
//	struct amcs_surface *surf;
	int i;

	debug("stop_draw nscreens %zd", pvector_len(&ctx->screens));
	if (pvector_len(&ctx->screens) == 0)
		return;

	amcs_screens_free(&ctx->screens);
	for (i = 0; i < pvector_len(&ctx->screens); i++) {
		struct amcs_screen *s = pvector_get(&ctx->screens, i);
		s->root->screen = NULL;
	}
	pvector_clear(&ctx->screens);
}

static void
data_dev_start_drag(struct wl_client *client,
	struct wl_resource *resource, struct wl_resource *source,
	struct wl_resource *origin, struct wl_resource *icon,
	uint32_t serial)
{
	warning("");
}

static void
data_dev_set_selection(struct wl_client *client,
	struct wl_resource *resource, struct wl_resource *source,
	uint32_t serial)
{
	warning("");
}

static void
data_dev_release(struct wl_client *client,
	struct wl_resource *resource)
{
	warning("");
}

static const struct wl_data_device_interface data_device_interface = {
	.start_drag = data_dev_start_drag,
	.set_selection = data_dev_set_selection,
	.release = data_dev_release,
};

static void
devman_create_data_source(struct wl_client *client,
	struct wl_resource *resource, uint32_t id)
{
	warning("");
}

static void
devman_get_data_device(struct wl_client *client,
	struct wl_resource *resource, uint32_t id, struct wl_resource *seat)
{
	struct wl_resource *res;

	debug("");
	RESOURCE_CREATE(res, client, &wl_data_device_interface, wl_resource_get_version(resource), id);
	wl_resource_set_implementation(res, &data_device_interface, resource, NULL);
}

static const struct wl_data_device_manager_interface data_device_manager_interface = {
	.create_data_source = devman_create_data_source,
	.get_data_device = devman_get_data_device,
};

static void
bind_devman(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	debug("");
	RESOURCE_CREATE(resource, client, &wl_data_device_manager_interface, version, id);
	wl_resource_set_implementation(resource, &data_device_manager_interface, data, NULL);
}

static int
device_manager_init(struct amcs_compositor *ctx)
{
	ctx->devman = wl_global_create(ctx->display, &wl_data_device_manager_interface, 3, ctx, &bind_devman);
	if (!ctx->devman) {
		warning("can't create shell inteface");
		return 1;
	}
	return 0;
}

static void
output_release(struct wl_client *client,
	struct wl_resource *resource)
{
	warning("");
}

static const struct wl_output_interface output_interface = {
	.release = output_release,
};

static void
bind_output(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource *resource;
//	struct amcs_compositor *ctx;
	struct amcs_client *c;

//	ctx = data;

	debug("");
	RESOURCE_CREATE(resource, client, &wl_output_interface, version, id);
	wl_resource_set_implementation(resource, &output_interface, data, NULL);
	//FIXME: temporary shit code
	wl_output_send_geometry(resource, 0, 0, 1024, 1024, 0,
			"unknown", "unknown", WL_OUTPUT_TRANSFORM_NORMAL);
	wl_output_send_mode(resource, 0, 1024, 1024, 60);
	wl_output_send_scale(resource, 1);
	wl_output_send_done(resource);
	c = amcs_get_client(resource);
	assert(c && "can't locate client");
	c->output = resource;
}

static int
output_init(struct amcs_compositor *ctx)
{
	ctx->output = wl_global_create(ctx->display, &wl_output_interface, 3, ctx, &bind_output);
	if (!ctx->output) {
		warning("can't create output interface");
		return 1;
	}
	return 0;
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
	    seat_init(ctx) != 0 ||
	    device_manager_init(ctx) != 0 ||
	    output_init(ctx) != 0) {
		goto finalize;
	}

	debug("compositor created, adding redraw signal");
	wl_signal_init(&ctx->redraw_sig);
	ctx->redraw_listener.notify = sig_surfaces_redraw;
	wl_signal_add(&ctx->redraw_sig, &ctx->redraw_listener);

	pvector_init(&ctx->screens, xrealloc);

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
	xdg_shell_finalize(ctx);
	seat_finalize(ctx);
}

struct amcs_client *
amcs_get_client(struct wl_resource *res)
{
	struct wl_client *c;
	struct amcs_client *iter;

	if (res == NULL)
		return NULL;

	c = wl_resource_get_client(res);
	debug("==== resource %p client %p", res, c);
	
	if (c == NULL)
		return NULL;

	wl_list_for_each(iter, &compositor_ctx.clients, link) {
		debug("next iter = %p", iter);
		if (iter->client == c)
			return iter;
	}
	return NULL;
}

static void
term_handler(int signo)
{
	struct sigaction act;
	sigset_t set;

	memset(&act, 0, sizeof(act));
	sigemptyset(&set);
	act.sa_handler = SIG_DFL;
	sigaction(signo, &act, NULL);

	stop_draw();
	amcs_tty_restore_term();
	raise(signo);
}

int
main(int argc, const char *argv[])
{
	int rc;
	struct sigaction act;
	sigset_t set;

	memset(&act, 0, sizeof(act));
	if (amcs_compositor_init(&compositor_ctx) != 0)
		return 1;

	debug("tty init");
	amcs_tty_open(0);
	amcs_tty_sethand(start_draw, stop_draw);
	amcs_tty_activate();

	sigemptyset(&set);
	act.sa_handler = term_handler;

	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGSEGV, &act, NULL);

	atexit(stop_draw);

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

