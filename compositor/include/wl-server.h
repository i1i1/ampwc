#ifndef WL_SERVER_H_
#define WL_SERVER_H_

#include "windows.h"
#include "vector.h"

struct amcs_surface {
	struct wl_resource *res;
	struct wl_resource *xdgres;
	struct wl_resource *xdgtopres;

	int w;
	int h;

	struct amcs_win *aw;
	struct {
		struct wl_shm_buffer *buf;
		int x;
		int y;
		int upd_source;
		int xdg_serial;
	} pending;
	struct wl_array surf_states;

	struct wl_list link;
};

struct amcs_compositor {
	struct wl_global *comp;
	struct wl_global *shell;

	struct wl_display *display;
	struct wl_event_loop *evloop;

	struct renderer *renderer;
	//struct drmdev dev;

	struct wl_list clients;
	struct wl_list surfaces;

	//
	pvector screens;	// struct amcs_screen *
	pvector screen_roots;	// struct amcs_wtree *
	pvector cur_wins;	//struct amcs_win *

	struct wl_listener redraw_listener;
	struct wl_signal redraw_sig;
};

extern struct amcs_compositor compositor_ctx;

int   amcs_compositor_init    (struct amcs_compositor *ctx);
void  amcs_compositor_deinit  (struct amcs_compositor *ctx);

#endif
