
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
 
#define DRIVER_NAME 		"/dev/axidma"
 
#define AXIDMA_IOC_MAGIC 			'A'
#define AXIDMA_IOCGETCHN			_IO(AXIDMA_IOC_MAGIC, 0)
#define AXIDMA_IOCCFGANDSTART 		_IO(AXIDMA_IOC_MAGIC, 1)
#define AXIDMA_IOCGETSTATUS 		_IO(AXIDMA_IOC_MAGIC, 2)
#define AXIDMA_IOCRELEASECHN 		_IO(AXIDMA_IOC_MAGIC, 3)
 
int main(void)
{
	int fd = -1;
	int ret;
	
	/* open dev */
	fd = open(DRIVER_NAME, O_RDWR);
	if(fd < 0){
		printf("open %s failed\n", DRIVER_NAME);
		return -1;
	}
	
	/* get channel */
	ret = ioctl(fd, AXIDMA_IOCGETCHN, NULL);
	if(ret){
		printf("ioctl: get channel failed\n");
		goto error;
	}
 
	ret = ioctl(fd, AXIDMA_IOCCFGANDSTART, NULL);
	if(ret){
		printf("ioctl: config and start dma failed\n");
		goto error;
	}
 
	/* wait finish */
	while(1){
		ret = ioctl(fd, AXIDMA_IOCGETSTATUS, NULL);
		if(ret){
			printf("ioctl: AXIDMA_IOCGETSTATUS dma failed\n");
			goto error;
		}
		sleep(60);
	}
 
	/* release channel */
	ret = ioctl(fd, AXIDMA_IOCRELEASECHN, NULL);
	if(ret){
		printf("ioctl: release channel failed\n");
		goto error;
	}
 
	close(fd);
 
	return 0;
error:
	close(fd);
	return -1;
}

