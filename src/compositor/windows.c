#include <string.h>
#include <sys/types.h>

#include "windows.h"
#include "macro.h"

amcs_screen*
amcs_wind_get_screens(const char *path)
{
	amcs_drm_card *card;
	amcs_drm_dev *dev_list;
	amcs_screen *screen, *screens;


	screens = screen = xmalloc(sizeof (amcs_screen));
	card = amcs_drm_init(path);
	dev_list = card->list;

	while (1) {
		screen->w = dev_list->w;
		screen->h = dev_list->h;
		screen->pitch = dev_list->pitch;
		screen->buf = dev_list->buf;
		screen->card = card;
		screen->next = NULL;

		screen->root = xmalloc(sizeof (amcs_wind));
		screen->root->splittype = NOSPLIT;
		screen->root->w = dev_list->w;
		screen->root->h = dev_list->h;
		screen->root->x = 0;
		screen->root->y = 0;
		screen->root->buf = NULL;
		screen->root->parent = NULL;
		screen->root->subwind = NULL;

		if ((dev_list = dev_list->next) == NULL)
			break;

		screen->next = xmalloc(sizeof (amcs_screen));
		screen = screen->next;
		screen->next = NULL;
	}

	return screens;
}

amcs_wind*
amcs_wind_get_root(amcs_screen *screen)
{
	if (screen == NULL)
		return NULL;

	return screen->root;
}

amcs_wind*
amcs_wind_get_parent(amcs_wind *wind)
{
	if (wind == NULL || wind->parent == NULL)
		return wind;

	return wind->parent;
}

amcs_wind*
amcs_wind_get_brother(amcs_wind *wind)
{
	if (wind == NULL)
		return NULL;

	if (wind->parent->subwind == wind)
		return wind->parent->subwind + 1;

	return wind->parent->subwind;
}

int
amcs_wind_setbuf(amcs_wind *wind, uint32_t *buf)
{
	if (wind == NULL)
		return 1;

	wind->buf = buf;

	return 0;
}

uint32_t*
amcs_wind_getbuf(amcs_wind *wind)
{
	if (wind == NULL)
		return NULL;

	if (wind->buf == NULL)
		wind->buf = xmalloc(4 * wind->h * wind->w);

	return wind->buf;
}

int
amcs_wind_swapbuf(amcs_screen *screen, amcs_wind *wind)
{
	int i, j;
	size_t offset;


	if (screen == NULL || wind == NULL)
		return 1;

	if (wind->splittype != NOSPLIT) {
		amcs_wind_swapbuf(screen, wind->subwind);
		amcs_wind_swapbuf(screen, wind->subwind + 1);

		return 0;
	}

	for (i = 0; i < wind->h; ++i) {
		for (j = 0; j < wind->w; ++j) {
			offset = screen->pitch * (i + wind->y) + 4*(j + wind->x);
			*(uint32_t*)&screen->buf[offset] = wind->buf[wind->w * i + j];
		}
	}

	return 0;
}

int
amcs_wind_get_width(amcs_wind *wind)
{
	if (wind == NULL)
		return 0;

	return wind->w;
}

int
amcs_wind_get_height(amcs_wind *wind)
{
	if (wind == NULL)
		return 0;

	return wind->h;
}

amcs_wind*
amcs_wind_split(amcs_wind *wind, int splittype)
{
	if ((splittype != VSPLIT && splittype != HSPLIT) || wind == NULL)
		return NULL;

	wind->splittype = splittype;
	wind->subwind = xmalloc(2 * sizeof (amcs_wind));

	wind->subwind[0].splittype = NOSPLIT;
	wind->subwind[0].h = (splittype == HSPLIT) ? wind->h/2 : wind->h;
	wind->subwind[0].w = (splittype == VSPLIT) ? wind->w/2 : wind->w;
	wind->subwind[0].y = wind->y;
	wind->subwind[0].x = wind->x;
	wind->subwind[0].buf = xmalloc(4 * wind->subwind[0].h * wind->subwind[0].w);
	wind->subwind[0].parent = wind;
	wind->subwind[0].subwind = NULL;

	wind->subwind[1].splittype = NOSPLIT;
	wind->subwind[1].h = wind->h - ((splittype == HSPLIT) ? wind->subwind->h : 0);
	wind->subwind[1].w = wind->w - ((splittype == VSPLIT) ? wind->subwind->w : 0);
	wind->subwind[1].y = wind->y + ((splittype == HSPLIT) ? wind->h/2 : 0);
	wind->subwind[1].x = wind->x + ((splittype == VSPLIT) ? wind->w/2 : 0);
	wind->subwind[1].buf = xmalloc(4 * wind->subwind[1].h * wind->subwind[1].w);
	wind->subwind[1].parent = wind;
	wind->subwind[1].subwind = NULL;

	return wind->subwind;
}

static void
free_windows(amcs_wind *wind)
{
	if (wind != NULL) {
		free(wind->buf);
		free_windows(wind->subwind);
		free(wind);
	}
}

void
amcs_wind_free_screens(amcs_screen *screens)
{
	amcs_screen *screen;

	amcs_drm_free(screens->card);

	while (screens != NULL) {
		free_windows(screens->root);

		screen = screens;
		screens = screens->next;
		free(screen);
	}
}
