#ifndef COMMON_V8MPIW7G
#define COMMON_V8MPIW7G

#include <sys/time.h>

#define RESOURCE_CREATE(out, client, interface, version, id) do {	\
	out = wl_resource_create(client, interface, version, id);	\
	if (resource == NULL) {						\
		wl_client_post_no_memory(client);			\
		return;							\
	}								\
} while (0)

static inline uint32_t
get_time(void)
{
	struct timeval timeval;

	gettimeofday(&timeval, NULL);
	return timeval.tv_sec * 1000 + timeval.tv_usec / 1000;
}

#endif
