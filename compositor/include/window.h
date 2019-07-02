#ifndef _AWC_WINDOWS_H
#define _AWC_WINDOWS_H

#include <stdint.h>
#include "amcs_drm.h"

#include "vector.h"

#define DRIPATH "/dev/dri/"

/*
 * amcs_container -- container for child windows / splits.
 * root container has no parent
 */
struct amcs_win;
struct amcs_container;
struct amcs_screen;

enum win_objtype {
	WT_TREE = 0,
	WT_WIN = 1,
};

enum container_type {
	CONTAINER_HSPLIT = 0,
	CONTAINER_VSPLIT,
};

#define AMCS_CONTAINER(v) ((struct amcs_container *)v)
struct amcs_container {
	enum win_objtype type;
	struct amcs_container *parent;
	int w, h;
	int x, y;

	enum container_type wt;
	pvector subwins; // struct amcs_win *

	struct amcs_screen *screen; // NOTE: Valid only for root wtree
};

struct amcs_buf {
	uint32_t *dt;
	int h, w;
	int sz;
};

/* Notify callback for window resize */
typedef int (*win_update_cb)(struct amcs_win *w, void *opaq);
#define AMCS_WIN(v) ((struct amcs_win *)v)
struct amcs_win {
	enum win_objtype type;
	struct amcs_container *parent;

	int w, h;
	int x, y;

	struct amcs_buf buf;

	void *opaq;	//TODO: getter/setter ???
	win_update_cb upd_cb;
	void *upd_opaq;
};

struct amcs_screen {
	int w, h;
	int pitch;
	uint8_t *buf;
	amcs_drm_card *card;
};

typedef int (*container_pass_cb)(struct amcs_win *w, void *opaq);

/* This functions may change amcs_win size. */

struct amcs_container *amcs_container_new(struct amcs_container *par, enum container_type t);
void amcs_container_free(struct amcs_container *wt);

void amcs_container_set_screen(struct amcs_container *wt, struct amcs_screen *screen);
int amcs_container_pass(struct amcs_container *wt, container_pass_cb cb, void *data);

// create new window, associate with window another object (*opaq*)
struct amcs_win *amcs_win_new(struct amcs_container *par, void *opaq,
		win_update_cb upd);
void amcs_win_free(struct amcs_win *w);

/*
 * Insert window into specified position
 * Thrid argument may be -1, for appending at the end of *wt*
 */
int amcs_container_insert(struct amcs_container *wt, struct amcs_win *w, int pos);
int amcs_container_remove(struct amcs_container *wt, struct amcs_win *w);
void amcs_container_remove_all(struct amcs_container *wt);
int amcs_container_remove_idx(struct amcs_container *wt, int pos);
int amcs_container_pos(struct amcs_container *wt, struct amcs_win *w);

//resize childs
//int amcs_container_update(

/* Common */

int amcs_container_resize_subwins(struct amcs_container *wt);
int amcs_win_commit(struct amcs_win *w);
int amcs_win_orphain(struct amcs_win *w);
//int amcs_win_resize(struct amcs_win *w,);
///
//
int amcs_screens_add(pvector *amcs_screens, const char *path);
void amcs_screens_free(pvector *amcs_screens);

void amcs_container_debug(struct amcs_container *wt);
//end
#endif // _AWC_WINDOWS_H
