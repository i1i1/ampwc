#ifndef _AWC_WINDOWS_H
#define _AWC_WINDOWS_H

#include <stdint.h>
#include "amcs_drm.h"

#include "vector.h"

#define DRIPATH "/dev/dri/"

/*
 * amcs_wintree -- container for child windows / splits.
 * root wintree has no parent
 */
struct amcs_win;
struct amcs_wintree;
struct amcs_screen;

enum win_objtype {
	WT_TREE = 0,
	WT_WIN = 1,
};

enum wintree_type {
	WINTREE_HSPLIT = 0,
	WINTREE_VSPLIT,
};

#define AMCS_WINTREE(v) ((struct amcs_wintree *)v)
struct amcs_wintree {
	enum win_objtype type;
	struct amcs_wintree *parent;
	int w, h;
	int x, y;

	enum wintree_type wt;
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
	struct amcs_wintree *parent;

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

typedef int (*wintree_pass_cb)(struct amcs_win *w, void *opaq);

/* This functions may change amcs_win size. */

struct amcs_wintree *amcs_wintree_new(struct amcs_wintree *par, enum wintree_type t);
void amcs_wintree_free(struct amcs_wintree *wt);

void amcs_wintree_set_screen(struct amcs_wintree *wt, struct amcs_screen *screen);
int amcs_wintree_pass(struct amcs_wintree *wt, wintree_pass_cb cb, void *data);

// create new window, associate with window another object (*opaq*)
struct amcs_win *amcs_win_new(struct amcs_wintree *par, void *opaq,
		win_update_cb upd, void *upd_opaq);
void amcs_win_free(struct amcs_win *w);

/*
 * Insert window into specified position
 * Thrid argument may be -1, for appending at the end of *wt*
 */
int amcs_wintree_insert(struct amcs_wintree *wt, struct amcs_win *w, int pos);
int amcs_wintree_remove(struct amcs_wintree *wt, struct amcs_win *w);
void amcs_wintree_remove_all(struct amcs_wintree *wt);
int amcs_wintree_remove_idx(struct amcs_wintree *wt, int pos);
int amcs_wintree_pos(struct amcs_wintree *wt, struct amcs_win *w);

//resize childs
//int amcs_wintree_update(

/* Common */

int amcs_wintree_resize_subwins(struct amcs_wintree *wt);
int amcs_win_commit(struct amcs_win *w);
int amcs_win_orphain(struct amcs_win *w);
//int amcs_win_resize(struct amcs_win *w,);
///
//
int amcs_screens_add(pvector *amcs_screens, const char *path);
void amcs_screens_free(pvector *amcs_screens);

void amcs_wintree_debug(struct amcs_wintree *wt);
//end
#endif // _AWC_WINDOWS_H
