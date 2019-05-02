#ifndef _AMCS_DRM_H
#define _AMCS_DRM_H

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <sys/types.h>


typedef struct amcs_drm_dev {
	uint8_t *buf;
	uint32_t conn_id, enc_id, crtc_id, fb_id;
	uint32_t w, h;
	uint32_t pitch, size, handle;
	drmModeModeInfo mode;

	struct amcs_drm_dev *next;
} amcs_drm_dev;

typedef struct amcs_drm_card {
	const char *path;
	int fd;

	amcs_drm_dev *list;
} amcs_drm_card;

amcs_drm_card *amcs_drm_init(const char *path);
void amcs_drm_free(amcs_drm_card *dev);

#endif // _AMCS_DRM_H
