#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static struct termios termios_initial;

#define FH_STDIN (0)


static void
resetunbuffered(void)
{
	tcsetattr(FH_STDIN, TCSANOW, &termios_initial);
}


void
setunbuffered(void)
{
	struct termios termio;
	tcgetattr(FH_STDIN, &termio);
	termios_initial = termio;
	termio.c_lflag &= (~ICANON & ~ECHO);
	tcsetattr(FH_STDIN, TCSANOW, &termio);
	atexit(resetunbuffered);
}


int
kbhit(void)
{
	int data_available = 0;
	ioctl(FH_STDIN, FIONREAD, &data_available);
	return data_available;
}


int
getch()
{
	char buf[1];
	int r = (int)read(FH_STDIN, buf, 1);
	return buf[0];
}
