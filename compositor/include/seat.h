#ifndef SEAT_V8UZNNQ
#define SEAT_V8UZNNQ

#include "wl-server.h"

int seat_init(struct amcs_compositor *ctx);

/* Focus on client, that owns this resource. */
//TODO
int seat_focus(struct wl_resource *res);

int seat_finalize(struct amcs_compositor *ctx);

#endif
