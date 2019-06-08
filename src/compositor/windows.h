#ifndef _AWC_WINDOWS_H
#define _AWC_WINDOWS_H

#include <stdint.h>

#define DRIPATH "/dev/dri/"

enum stypes {
	HSPLIT = 1,
	VSPLIT,
};

typedef struct amcs_wind amcs_wind;
typedef struct amcs_screen amcs_screen;
typedef struct amcs_screen amcs_screen_list;


amcs_screen_list *amcs_wind_get_screens(const char *path);
amcs_screen_list *amcs_wind_merge_screen_lists(amcs_screen_list *dst,
					       amcs_screen_list *src);
amcs_screen_list *amcs_wind_get_next(amcs_screen_list *screens);

amcs_wind *amcs_wind_get_root(amcs_screen *screen);
amcs_wind *amcs_wind_get_parent(amcs_wind *wind);
amcs_wind *amcs_wind_get_brother(amcs_wind *wind);

int amcs_wind_get_width(amcs_wind *wind);
int amcs_wind_get_height(amcs_wind *wind);

void amcs_wind_setbuf(amcs_wind *wind, uint32_t *buf);
uint32_t *amcs_wind_getbuf(amcs_wind *wind);
void amcs_wind_commit_buf(amcs_screen *screen);

amcs_wind *amcs_wind_split(amcs_wind *wind, int splittype);

void amcs_wind_free_screens(amcs_screen_list *screens);

#endif // _AWC_WINDOWS_H
