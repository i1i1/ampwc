#ifndef _UDEV_H
#define _UDEV_H

typedef struct amcs_monitors {
	char *name;
	int status;

	struct amcs_monitors *next;
} amcs_monitors;

struct amcs_monitors *amcs_udev_get_monitors(void);
int amcs_udev_monitor_tracking(void (*update_status)(const char *name, int status));

#endif // _UDEV_H
