#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "jr3pci-ioctl.h"

int main(void) {
	six_axis_array fm;
	force_array fs;
	int ret,i,fd;
	
	if ((fd=open("/dev/jr3",O_RDWR)) < 0) {
		perror("Can't open device. No way to read force!");
	}
	
	ret=ioctl(fd,IOCTL0_JR3_GET_FULL_SCALES,&fs);
	printf("Full scales are %d %d %d %d %d %d\n",fs.f[0],fs.f[1],fs.f[2],fs.m[0],fs.m[1],fs.m[2]);
	
	ret=ioctl(fd, IOCTL0_JR3_RESET);
	
	close(fd);
}
