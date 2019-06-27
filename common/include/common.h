#ifndef COMMON_V8MPIW7G
#define COMMON_V8MPIW7G

#define RESOURCE_CREATE(out, client, interface, version, id) do {	\
	out = wl_resource_create(client, interface, version, id);	\
	if (resource == NULL) {						\
		wl_client_post_no_memory(client);			\
		return;							\
	}								\
} while (0)

#endif
