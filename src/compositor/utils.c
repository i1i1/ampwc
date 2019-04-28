#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int
tempfile()
{
	int fd;
	char filename[] = "XXXXXX";

	fd = mkstemp(filename);
	if (fd < 0)
		return -1;
	unlink(filename);
	fcntl(fd, F_SETFD,  FD_CLOEXEC);

	return fd;
}
