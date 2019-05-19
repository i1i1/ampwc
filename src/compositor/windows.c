#include <string.h>
#include <sys/types.h>
#include <assert.h>

#include "udev.h"
#include "windows.h"
#include "macro.h"

#define NOSPLIT 0


struct amcs_wind {
	enum stypes stype;
	int w, h;
	int x, y;
	uint32_t *buf;

	amcs_wind *parent;
	amcs_wind *subwind[2];
};

struct amcs_screen {
	int w, h;
	int pitch;
	amcs_wind *root;
	uint8_t *buf;
	amcs_drm_card **card;

	amcs_screen_list *next;
};


static void
update_screen_state(const char *name, int status)
{
	if (status)
		debug("EVENT: %s: connected", name);
	else
		debug("EVENT: %s: disconnected", name);
}

amcs_screen_list*
amcs_wind_get_screens(const char *path)
{
	amcs_drm_card **card;
	amcs_drm_dev_list *dev_list;
	amcs_screen *screen;
	amcs_screen_list *screens;
	amcs_monitor_list *monitors;


	assert(path);

	monitors = amcs_udev_get_monitors();
	amcs_udev_monitor_tracking(update_screen_state);

	card = xmalloc(sizeof (amcs_drm_card*));
	if ((*card = amcs_drm_init(path)) == NULL) {
		free(card);

		return NULL;
	}

	dev_list = amcs_drm_get_devlist(*card);
	screens = screen = xmalloc(sizeof (amcs_screen));

	while (1) {
		screen->w = amcs_drm_get_width(dev_list);
		screen->h = amcs_drm_get_height(dev_list);
		screen->pitch = amcs_drm_get_pitch(dev_list);
		screen->buf = amcs_drm_get_buf(dev_list);
		screen->card = card;
		screen->next = NULL;

		screen->root = xmalloc(sizeof (amcs_wind));
		screen->root->stype = NOSPLIT;
		screen->root->w = amcs_drm_get_width(dev_list);
		screen->root->h = amcs_drm_get_height(dev_list);
		screen->root->x = 0;
		screen->root->y = 0;
		screen->root->buf = NULL;
		screen->root->parent = NULL;
		screen->root->subwind[0] = NULL;
		screen->root->subwind[1] = NULL;

		if ((dev_list = amcs_drm_get_nextentry(dev_list)) == NULL)
			break;

		screen->next = xmalloc(sizeof (amcs_screen));
		screen = screen->next;
		screen->next = NULL;
	}

	return screens;
}

amcs_screen_list*
amcs_wind_merge_screen_lists(amcs_screen_list *dst, amcs_screen_list *src)
{
	if (src == NULL)
		return dst;
	else if (dst == NULL)
		return src;

	for (; dst->next != NULL; dst = dst->next)
		;

	dst->next = src;

	return dst;
}

amcs_wind*
amcs_wind_get_root(amcs_screen *screen)
{
	assert(screen);

	return screen->root;
}

amcs_wind*
amcs_wind_get_parent(amcs_wind *wind)
{
	assert(wind);

	return wind->parent;
}

amcs_wind*
amcs_wind_get_brother(amcs_wind *wind)
{
	assert(wind);

	if (wind->parent->subwind[0] == wind)
		return wind->parent->subwind[1];

	return wind->parent->subwind[0];
}

void
amcs_wind_setbuf(amcs_wind *wind, uint32_t *buf)
{
	assert(wind);
	wind->buf = buf;
}

uint32_t*
amcs_wind_getbuf(amcs_wind *wind)
{
	assert(wind);

	if (wind->buf == NULL)
		wind->buf = xmalloc(4 * wind->h * wind->w);

	return wind->buf;
}

void
commit_buf(amcs_screen *screen, amcs_wind *wind) {
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

void
amcs_wind_commit_buf(amcs_screen *screen)
{
	assert(screen);
	commit_buf(screen, screen->root);
}

int
amcs_wind_get_width(amcs_wind *wind)
{
	assert(wind);
	return wind->w;
}

int
amcs_wind_get_height(amcs_wind *wind)
{
	assert(wind);
	return wind->h;
}

amcs_wind*
amcs_wind_split(amcs_wind *wind, int stype)
{
	assert(stype == VSPLIT || stype == HSPLIT);
	assert(wind);

	wind->stype = stype;
	wind->subwind[0] = xmalloc(sizeof (amcs_wind));
	wind->subwind[1] = xmalloc(sizeof (amcs_wind));

	wind->subwind[0]->stype = NOSPLIT;
	wind->subwind[0]->h = (stype == HSPLIT) ? wind->h/2 : wind->h;
	wind->subwind[0]->w = (stype == VSPLIT) ? wind->w/2 : wind->w;
	wind->subwind[0]->y = wind->y;
	wind->subwind[0]->x = wind->x;
	wind->subwind[0]->buf = NULL;
	wind->subwind[0]->parent = wind;
	wind->subwind[0]->subwind[0] = NULL;
	wind->subwind[0]->subwind[1] = NULL;

	wind->subwind[1]->stype = NOSPLIT;
	wind->subwind[1]->h = wind->h - ((stype == HSPLIT) ? wind->subwind[0]->h : 0);
	wind->subwind[1]->w = wind->w - ((stype == VSPLIT) ? wind->subwind[0]->w : 0);
	wind->subwind[1]->y = wind->y + ((stype == HSPLIT) ? wind->h/2 : 0);
	wind->subwind[1]->x = wind->x + ((stype == VSPLIT) ? wind->w/2 : 0);
	wind->subwind[1]->buf = NULL;
	wind->subwind[1]->parent = wind;
	wind->subwind[1]->subwind[0] = NULL;
	wind->subwind[1]->subwind[1] = NULL;

	return wind->subwind[0];
}

static void
free_windows(amcs_wind *wind)
{
	if (wind == NULL)
		return;

	if (wind->buf != NULL)
		free(wind->buf);

	free_windows(wind->subwind[0]);
	free_windows(wind->subwind[1]);
	free(wind);
}

void
amcs_wind_free_screens(amcs_screen_list *screens)
{
	amcs_screen *screen;


	assert(screens);

	while (screens != NULL) {
		free_windows(screens->root);

		if (*screens->card != NULL) {
			amcs_drm_free(*screens->card);
			*screens->card = NULL;
		}

		screen = screens;
		screens = screens->next;
		free(screen);
	}
}
