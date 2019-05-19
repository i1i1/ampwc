#ifndef _AMCS_DRM_H
#define _AMCS_DRM_H

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <sys/types.h>


#define DRIPATH "/dev/dri/"


typedef struct amcs_drm_card amcs_drm_card;
typedef struct amcs_drm_dev amcs_drm_dev_list;
typedef struct amcs_drm_dev amcs_drm_dev;


amcs_drm_card *amcs_drm_init(const char *path);

amcs_drm_dev_list *amcs_drm_get_devlist(amcs_drm_card *card);
uint8_t *amcs_drm_get_buf(amcs_drm_dev *dev);
int32_t amcs_drm_get_pitch(amcs_drm_dev *dev);
int32_t amcs_drm_get_width(amcs_drm_dev *dev);
int32_t amcs_drm_get_height(amcs_drm_dev *dev);
amcs_drm_dev_list *amcs_drm_get_nextentry(amcs_drm_dev_list *dev);

void amcs_drm_free(amcs_drm_card *card);

#endif // _AMCS_DRM_H
