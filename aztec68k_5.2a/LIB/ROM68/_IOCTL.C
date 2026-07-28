#include <sgtty.h>
#include <fcntl.h>

int
_ioctl(int fd, int cmd, struct sgttyb *arg)
{
	return -1;
}
