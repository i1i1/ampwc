#ifndef _AWC_WINDOWS_H
#define _AWC_WINDOWS_H

#include <stdint.h>

#include "drm.h"

enum splittypes {
	HSPLIT,
	VSPLIT,
	NOSPLIT,
};

typedef struct amcs_wind {
	int splittype;
	int w, h;
	int x, y;
	uint32_t *buf;

	struct amcs_wind *parent;
	struct amcs_wind *subwind;
} amcs_wind;

typedef struct amcs_screen {
	int w, h;
	int pitch;
	amcs_wind *root;
	uint8_t *buf;
	amcs_drm_card *card;

	struct amcs_screen *next;
} amcs_screen;

amcs_screen *amcs_wind_get_screens(const char *path);

amcs_wind *amcs_wind_get_root(amcs_screen *screen);
amcs_wind *amcs_wind_get_parent(amcs_wind *wind);
amcs_wind *amcs_wind_get_brother(amcs_wind *wind);

int amcs_wind_get_width(amcs_wind *wind);
int amcs_wind_get_height(amcs_wind *wind);

int amcs_wind_setbuf(amcs_wind *wind, uint32_t *buf);
uint32_t *amcs_wind_getbuf(amcs_wind *wind);
int amcs_wind_swapbuf(amcs_screen *screen, amcs_wind *wind);

amcs_wind *amcs_wind_split(amcs_wind *wind, int splittype);

void amcs_wind_free_screens(amcs_screen *screens);

#endif // _AWC_WINDOWS_H
