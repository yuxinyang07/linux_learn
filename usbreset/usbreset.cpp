#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <unistd.h>
using namespace std;
int  main(int argc, char **argv)
{
	const char *filename;
	int fd;
	filename = argv[1];
	fd = open(filename, O_WRONLY);
	ioctl(fd, USBDEVFS_RESET, 0);
	close(fd);
	return  0;
}
