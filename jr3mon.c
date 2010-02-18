#include <ncurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "jr3pci-ioctl.h"

void drawline(int col, int value) {
	int start;
	if (value>=0) {
		start=40;
	} else {
		start=40+value;
	}
	if (fabs(value)<10) color_set(2,NULL);
	else if (fabs(value)>10) color_set(3,NULL);
	mvhline(col,start,'=',abs(value));
}

void drawtext(int sensors) {
	color_set(1,NULL);
	if (sensors>0) {
		mvaddstr(0,36,"Fx (N)");
		mvaddstr(2,36,"Fy (N)");
		mvaddstr(4,36,"Fz (N)");
		mvaddstr(6,34,"Mx (N*m*10)");
		mvaddstr(8,34,"My (N*m*10)");
		mvaddstr(10,34,"Mz (N*m*10");
	}
	
	if (sensors>1) {
		mvaddstr(14,36,"Fx (N)");
		mvaddstr(16,36,"Fy (N)");
		mvaddstr(18,36,"Fz (N)");
		mvaddstr(20,34,"Mx (N*m*10)");
		mvaddstr(22,34,"My (N*m*10)");
		mvaddstr(24,34,"Mz (N*m*10");
	}
}

int main(void)
{
    short f, b;
    six_axis_array fm,ac;
    force_array fs, as;
    int ret,i,fd, sensors=0;

    initscr();
    cbreak();
    noecho();

    if ((fd=open("/dev/jr3",O_RDWR)) < 0) {
	perror("Can't open device. No way to read force!");
    }
    ret=ioctl(fd,IOCTL0_JR3_GET_FULL_SCALES,&fs);
    if (ret==0) sensors++;
    ret=ioctl(fd,IOCTL1_JR3_GET_FULL_SCALES,&as);
    if (ret==0) sensors++;
    
    if (has_colors()) {
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	curs_set(0);
	while (1) {
		if (sensors>0)
			ioctl(fd,IOCTL0_JR3_FILTER0,&fm);
		if (sensors>1)
			ioctl(fd,IOCTL1_JR3_FILTER0,&ac);
		
		clear();
		drawtext(sensors);
		if (sensors>0) {
			drawline(1,fm.f[0]*fs.f[0]/16384);
			drawline(3,fm.f[1]*fs.f[1]/16384);
			drawline(5,fm.f[2]*fs.f[2]/16384);
			drawline(7,fm.m[0]*fs.m[0]/16384);
			drawline(9,fm.m[1]*fs.m[1]/16384);
			drawline(11,fm.m[2]*fs.m[2]/16384);
		}
		
		if (sensors>1) {
			drawline(17,ac.f[0]*as.f[0]/16384);
			drawline(19,ac.f[1]*as.f[1]/16384);
			drawline(21,ac.f[2]*as.f[2]/16384);
			drawline(23,ac.m[0]*as.m[0]/16384);
			drawline(25,ac.m[1]*as.m[1]/16384);
			drawline(27,ac.m[2]*as.m[2]/16384);
		}
		refresh();
		usleep(100000);
	}
    } else {
	printw("This program requires a color terminal");
	getch();
    }
    endwin();

    exit(0);
}
