#include <stdio.h>
#include <string.h>
#include <libudev.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "udev.h"
#include "macro.h"

static amcs_monitors *monitor_list;

amcs_monitors*
amcs_udev_get_monitors(void)
{
	const char *path, *status;
	struct udev *udev;
	struct udev_device *udev_dev;
	struct udev_enumerate *udev_enum;
	struct udev_list_entry *udev_list, *udev_list_entry;
	amcs_monitors *monitors, *monitor;
	const char *sysname;


	if ((udev = udev_new()) == NULL)
		error(1, "Can not create udev object.");

	if ((udev_enum = udev_enumerate_new(udev)) == NULL )
		error(1, "Can not create unumerate object");

	udev_enumerate_add_match_subsystem(udev_enum, "drm");
	udev_enumerate_scan_devices(udev_enum);

	if ((udev_list = udev_enumerate_get_list_entry(udev_enum)) == NULL)
		error(1, "Can not get dev list");

	monitors = NULL;

	udev_list_entry_foreach(udev_list_entry, udev_list) {
		path = udev_list_entry_get_name(udev_list_entry);
		udev_dev = udev_device_new_from_syspath(udev, path);
		status = udev_device_get_sysattr_value(udev_dev, "status");

		if (status != NULL) {
			monitor = xmalloc(sizeof (amcs_monitors));
			monitor->next = monitors;
			monitors = monitor;

			sysname = udev_device_get_sysname(udev_dev);
			monitor->name = xmalloc(strlen(sysname) + 1);
			strcpy(monitor->name, sysname);
			monitor->status = 0;

			if (strcmp(status, "connected") == 0)
				monitor->status = 1;
		}

		udev_device_unref(udev_dev);
	}

	if (monitor_list == NULL)
		monitor_list = monitors;

	udev_enumerate_unref(udev_enum);
	udev_unref(udev);

	return monitors;
}

static void
send_changes(void (*update_status)(const char* name, int status))
{
	amcs_monitors *new_monitor_list, *new_monitor, *monitor;


	new_monitor_list = new_monitor = amcs_udev_get_monitors();

	for (; new_monitor != NULL; new_monitor = new_monitor->next) {
		monitor = monitor_list;

		for (; monitor != NULL; monitor = monitor->next) {
			if (strcmp(monitor->name, new_monitor->name) == 0 &&
			    monitor->status != new_monitor->status) {
				monitor->status = new_monitor->status;
				update_status(monitor->name, monitor->status);
			}
		}
	}

	free(new_monitor_list->name);
	free(new_monitor_list);
}

static void*
monitor_tracking(void *update_status)
{
	int ret;
	int udev_fd;
	fd_set fds;
	struct udev *udev;
	struct udev_monitor *udev_mon;


	if ((udev = udev_new()) == NULL)
		error(1, "Can not create udev object.");

	udev_mon = udev_monitor_new_from_netlink(udev, "udev");

	udev_monitor_filter_add_match_subsystem_devtype(udev_mon, "drm", NULL);
	udev_monitor_enable_receiving(udev_mon);
	udev_fd = udev_monitor_get_fd(udev_mon);

	FD_ZERO(&fds);
	FD_SET(udev_fd, &fds);

	while (1) {
		ret = select(udev_fd + 1, &fds, NULL, NULL, NULL);

		if (ret <= 0) {
			perror("monitor_tracking()");
			warning("monitor tracking thread exited");
			break;
		}
		else if (!FD_ISSET(udev_fd, &fds))
			continue;

		send_changes((void(*)(const char*, int)) update_status);
	}

	return NULL;
}

int
amcs_udev_monitor_tracking(void (*update_status)(const char *name, int status))
{
	pthread_t thread;
	pthread_attr_t attr;


	if (pthread_attr_init(&attr))
		goto thread_error;

	if (pthread_create(&thread, &attr, monitor_tracking, update_status))
		goto thread_error;

	return 0;

thread_error:
	warning("monitoring thread create error");

	return 1;
}
