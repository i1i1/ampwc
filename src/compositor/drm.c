#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/mman.h>

#include "macro.h"
#include "drm.h"


#define DEPTH 24
#define BPP 32


static int
dumb_is_supported(int fd)
{
	uint64_t dumb;
	int flags;


	if ((flags = fcntl(fd, F_GETFD)) < 0 ||
	    fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0)
		return 0;

	if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &dumb) < 0 || dumb == 0)
		return 0;

	debug("Dumb buffer is supported");

	return 1;
}

static void
drm_setFB(int fd, amcs_drm_dev *dev)
{
	struct drm_mode_create_dumb creq;
	struct drm_mode_map_dumb mreq;


	// get settings, and FB id
	memset(&creq, 0, sizeof (struct drm_mode_create_dumb));
	creq.width = dev->w;
	creq.height = dev->h;
	creq.bpp = BPP;

	if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0)
		error(1, "drmIoctl error");

	dev->pitch = creq.pitch;
	dev->size = creq.size;
	dev->handle = creq.handle;

	debug("\tpitch: %d, size: %d, handle: %d", creq.pitch, (int)creq.size, creq.handle);

	if (drmModeAddFB(fd, dev->w, dev->h, DEPTH, BPP, dev->pitch,
			 dev->handle, &dev->fb_id))
		error(1, "error adding frame buffer");


	// get settings, and map buffer
	memset(&mreq, 0, sizeof (struct drm_mode_map_dumb));
	mreq.handle = dev->handle;

	if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq))
		error(1, "error getting map dumb");

	dev->buf = mmap(NULL, dev->size, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, mreq.offset);
	if (dev->buf == MAP_FAILED)
		error(1, "error mapping buffer");

	if (drmModeSetCrtc(fd, dev->crtc_id, dev->fb_id, 0, 0,
			   &dev->conn_id, 1, &dev->mode))
		error(1, "error setting crtc");
}

amcs_drm_card*
amcs_drm_init(const char *path)
{
	int i;

	amcs_drm_card *card;
	amcs_drm_dev *dev_list;
	amcs_drm_dev *drmdev;

	drmModeRes *res;
	drmModeConnector *conn;
	drmModeEncoder *enc;


	debug("Init dev: %s", path);
	card = xmalloc(sizeof (amcs_drm_card));
	card->path = path;

	if ((card->fd = open(path, O_RDWR)) < 0) {
		free(card);
		error(1, "Can not open device: %s", path);
	}

	if (!dumb_is_supported(card->fd)) {
		free(card);
		error(1, "dumb buffer is not supported");
	}

	if ((res = drmModeGetResources(card->fd)) == NULL) {
		free(card);
		error(1, "error getting drm resources");
	}


	debug("Count connectors: %d", res->count_connectors);

	dev_list = NULL;
	for (i = 0; i < res->count_connectors; ++i) {
		conn = drmModeGetConnector(card->fd, res->connectors[i]);

		if (conn == NULL)
			continue;

		if (conn->connection != DRM_MODE_CONNECTED || conn->count_modes <= 0)
			goto free_connector;

		enc = drmModeGetEncoder(card->fd, conn->encoder_id);
		if (enc == NULL)
			goto free_connector;

		drmdev = xmalloc(sizeof (amcs_drm_dev));
		drmdev->next = dev_list;
		dev_list = card->list = drmdev;

		dev_list->conn_id = conn->connector_id;
		dev_list->enc_id = conn->encoder_id;
		dev_list->crtc_id = enc->crtc_id;

		dev_list->mode = conn->modes[0];
		dev_list->w = conn->modes[0].hdisplay;
		dev_list->h = conn->modes[0].vdisplay;

		debug("\tconn id: %d", dev_list->conn_id);
		debug("\tenc id: %d", dev_list->enc_id);
		debug("\tcrtc id: %d", dev_list->crtc_id);
		debug("\tmode: %dx%d", dev_list->w, dev_list->h);

		drm_setFB(card->fd, dev_list);

		drmModeFreeEncoder(enc);

free_connector:
		drmModeFreeConnector(conn);
		debug("\t____________________");
	}

	drmModeFreeResources(res);

	return card;
}

void
amcs_drm_free(amcs_drm_card *card)
{
	amcs_drm_dev *dev, *dev_list;
	struct drm_mode_destroy_dumb dreq;


	for (dev_list = card->list; dev_list != NULL;) {
		munmap(dev_list->buf, dev_list->size);
		drmModeRmFB(card->fd, dev_list->fb_id);
		dreq.handle = dev_list->handle;
		drmIoctl(card->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);

		dev = dev_list;
		dev_list = dev_list->next;
		free(dev);
	}

	close(card->fd);
	free(card);
}
