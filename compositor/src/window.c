#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "drm.h"
#include "macro.h"
#include "udev.h"
#include "window.h"

static struct amcs_wintree *
win_get_root(struct amcs_win *w)
{
	struct amcs_wintree *wt = AMCS_WINTREE(w);

	assert(w);
	while (wt->parent)
		wt = wt->parent;
	return wt;
}

static void
update_screen_state(const char *name, int status)
{
	if (status)
		debug("EVENT: %s: connected", name);
	else
		debug("EVENT: %s: disconnected", name);
}

// Public functions

int
amcs_screens_add(pvector *amcs_screens, const char *path)
{
	amcs_drm_card *card;
	amcs_drm_dev_list *dev_list;
	struct amcs_screen *screen;

	assert(amcs_screens && path);

	amcs_udev_monitor_tracking(update_screen_state);

	if ((card = amcs_drm_init(path)) == NULL) {
		return 1;
	}

	dev_list = card->list;

	while (dev_list) {
		screen = xmalloc(sizeof (*screen));
		screen->w = dev_list->w;
		screen->h = dev_list->h;
		screen->pitch = dev_list->pitch;
		screen->buf = dev_list->buf;
		screen->card = card;
		pvector_push(amcs_screens, screen);

		card = NULL; // set card only for first screen

		dev_list = dev_list->next;
	}
	return 0;
}

void
amcs_screens_free(pvector *amcs_screens)
{
	int i;
	struct amcs_screen **sarr;

	assert(amcs_screens);
	sarr = pvector_data(amcs_screens);
	for (i = 0; i < pvector_len(amcs_screens); i++) {
		if (sarr[i]->card != NULL) {
			amcs_drm_free(sarr[i]->card);
		}
		free(sarr[i]);
	}
	for (i = 0; i < pvector_len(amcs_screens); i++)
		pvector_pop(amcs_screens);
}

struct amcs_wintree *
amcs_wintree_new(struct amcs_wintree *par, enum wintree_type t)
{
	struct amcs_wintree *res;

	res = xmalloc(sizeof(*res));
	memset(res, 0, sizeof(*res));
	res->type = WT_TREE;
	res->wt = t;
	res->parent = par;
	pvector_init(&res->subwins, xrealloc);

	amcs_wintree_resize_subwins(par);
	return res;
}

void
amcs_wintree_free(struct amcs_wintree *wt)
{
	assert(wt && wt->type == WT_TREE);
	amcs_wintree_remove_all(wt);
	free(wt);
}

void
amcs_wintree_set_screen(struct amcs_wintree *wt, struct amcs_screen *screen)
{
	assert(wt && wt->type == WT_TREE);

	wt->screen = screen;
	wt->x = wt->y = 0;
	wt->w = screen->w;
	wt->h = screen->h;
	amcs_wintree_resize_subwins(wt);
}

int
amcs_wintree_pass(struct amcs_wintree *wt, wintree_pass_cb cb, void *data)
{
	struct amcs_win **arr;
	int i, rc;

	if (wt == NULL)
		return 0;
	debug("pass");

	rc = cb(AMCS_WIN(wt), data);

	if (wt->type == WT_WIN)
		return rc;
	arr = pvector_data(&wt->subwins);
	for (i = 0; i < pvector_len(&wt->subwins); i++) {
		if (arr[i]->type == WT_WIN)
			rc = cb(arr[i], data);
		else if (arr[i]->type == WT_TREE)
			rc = amcs_wintree_pass(AMCS_WINTREE(arr[i]), cb, data);
		else
			error(2, "should not reach");

		if (rc != 0)
			return rc;
	}
	return 0;
}

int
amcs_wintree_insert(struct amcs_wintree *wt, struct amcs_win *w, int pos)
{
	assert(wt && wt->type == WT_TREE && w->type == WT_WIN);

	debug("insert %p, into %p nsubwinds %zd", w, wt, pvector_len(&wt->subwins));
	pvector_push(&wt->subwins, w);
	w->parent = wt;
	amcs_wintree_resize_subwins(wt);
	return 0;
}

int
amcs_wintree_pos(struct amcs_wintree *wt, struct amcs_win *w)
{
	struct amcs_win **arr;
	int i;

	assert(wt && wt->type == WT_TREE);

	arr = pvector_data(&wt->subwins);
	for (i = 0; i < pvector_len(&wt->subwins); i++) {
		if (arr[i] == w)
			return i;
	}
	return -1;
}

int
amcs_wintree_remove(struct amcs_wintree *wt, struct amcs_win *w)
{
	int pos;
	assert(wt && wt->type == WT_TREE);

	pos = amcs_wintree_pos(wt, w);
	if (pos >= 0) {
		struct amcs_win *w;
		w = pvector_get(&wt->subwins, pos);
		w->parent = NULL;

		pvector_del(&wt->subwins, pos);
		amcs_wintree_resize_subwins(wt);
		return 0;
	}
	error(2, "should not happen, programmer error");
	return 1;
}

int
amcs_wintree_remove_idx(struct amcs_wintree *wt, int pos)
{
	assert(wt && wt->type == WT_TREE);

	pvector_del(&wt->subwins, pos);
	amcs_wintree_resize_subwins(wt);
	return 0;
}

void
amcs_wintree_remove_all(struct amcs_wintree *wt)
{
	int i;
	assert(wt && wt->type == WT_TREE);

	for (i = 0; i < pvector_len(&wt->subwins); i++) {
		pvector_pop(&wt->subwins);
	}
	amcs_wintree_resize_subwins(wt);
}

struct amcs_win *
amcs_win_new(struct amcs_wintree *par, void *opaq, win_update_cb upd, void *upd_opaq)
{
	struct amcs_win *res;

	debug("win_new, %p", par);
	res = xmalloc(sizeof(*res));
	memset(res, 0, sizeof(*res));
	res->type = WT_WIN;
	res->opaq = opaq;
	res->upd_cb = upd;
	res->upd_opaq = upd_opaq;
	if (par)
		amcs_wintree_insert(par, res, -1);

	return res;
}

void
amcs_win_free(struct amcs_win *w)
{
	assert(w && w->type == WT_WIN);
	amcs_win_orphain(w);
	if (w->buf.dt)
		free(w->buf.dt);
	free(w);
}

static int
_commit_cb(struct amcs_win *w, void *opaq)
{
	int rc;
	debug("commit cb");
	if (w && w->type == WT_WIN && w->buf.dt) {
		amcs_win_commit(w);
		if (w->upd_cb) {
			rc = w->upd_cb(w, w->upd_opaq);
			assert(rc == 0 || "TODO: writeme");
		}
	}
	return 0;
}

int
amcs_wintree_resize_subwins(struct amcs_wintree *wt)
{
	int i, nwin, step;

	if (wt == NULL || wt->type == WT_WIN)
		return 0;

	nwin = pvector_len(&wt->subwins);
	if (nwin == 0)
		return 0;

	if (wt->wt == WINTREE_HSPLIT) {
		step = wt->h / nwin;
	} else {
		step = wt->w / nwin;
	}

	assert(step); // is that really happens?

	for (i = 0; i < nwin; i++) {
		struct amcs_win *tmp;
		tmp = pvector_get(&wt->subwins, i);

		if (wt->wt == WINTREE_HSPLIT) {
			tmp->x = wt->x;
			tmp->y = wt->y + i * step;
			tmp->w = wt->w;
			tmp->h = step;
		} else {
			tmp->x = wt->x + i * step;
			tmp->y = wt->y;
			tmp->w = step;
			tmp->h = wt->h;
		}
		if (tmp->type == WT_TREE) {
			amcs_wintree_resize_subwins(AMCS_WINTREE(tmp));
		}
	}
	amcs_wintree_pass(wt, _commit_cb, NULL);
	return 0;
}

int
amcs_win_commit(struct amcs_win *win)
{
	struct amcs_wintree *root;
	struct amcs_screen *screen;
	int i, j, h, w;
	size_t offset;

	root = win_get_root(win);
	debug("get root %p", root);
	if (root->screen == NULL)
		return -1;

	h = MIN(win->h, win->buf.h);
	w = MIN(win->w, win->buf.w);
	screen = root->screen;
	for (i = 0; i < h; ++i) {
		for (j = 0; j < w; ++j) {
			offset = screen->pitch * (i + win->y) + 4*(j + win->x);
			*(uint32_t*)&screen->buf[offset] = win->buf.dt[win->buf.w * i + j];
		}
	}

	return 0;
}

int
amcs_win_orphain(struct amcs_win *w)
{
	struct amcs_wintree *par;

	assert(w);
	if (w->parent == NULL)
		return 1;
	par = w->parent;
	w->parent = NULL;

	amcs_wintree_remove(par, w);
	return 0;
}

static int
_print_cb(struct amcs_win *w, void *opaq)
{
	debug("tp = %s, x = %d y = %d"
	      "         w = %d h = %d", w->type == WT_WIN ? "wt_win" : "wt_wtree",
			w->x, w->y, w->w, w->h);
	return 0;
}

void
amcs_wintree_debug(struct amcs_wintree *wt)
{
	if (wt == NULL)
		return;
	amcs_wintree_pass(wt, _print_cb, NULL);
}
////////

/*
void
static void
commit_buf(amcs_screen *screen, amcs_wind *wind)
{
	int i, j;
	size_t offset;


	if (wind->stype != NOSPLIT) {
		commit_buf(screen, wind->subwind[0]);
		commit_buf(screen, wind->subwind[1]);

		return;
	}

	for (i = 0; i < wind->h; ++i) {
		for (j = 0; j < wind->w; ++j) {
			offset = screen->pitch * (i + wind->y) + 4*(j + wind->x);
			*(uint32_t*)&screen->buf[offset] = wind->buf[wind->w * i + j];
		}
	}
}
*/

