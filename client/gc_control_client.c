#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/ioctl.h>
#include <termios.h>
#elif defined(OS_WINDOWS)
#include <conio.h>
#endif

#include "hid.h"
#include "../usb_rawhid_settings.h"


static char get_keystroke(void);
static void delay_ms(unsigned int msec);

void use_device(void) {
	int i, num;
	char c, buf[RAWHID_TX_SIZE+1];

  while (1) {
    // check if any Raw HID packet has arrived
    num = rawhid_recv(0, buf, RAWHID_TX_SIZE, 1);
    if (num < 0) {
      printf("\nerror reading, device went offline\n");
      return;
    }
    if (num > 0) {
      buf[RAWHID_TX_SIZE] = '\0';
      printf("recv: %s", buf);
    }

    // check if any input on stdin
    while ((c = get_keystroke()) >= 32) {
      printf("\ngot key '%c', sending...\n", c);
      buf[0] = c;
      num = rawhid_send(0, buf, RAWHID_RX_SIZE, 100);
      if (num < 0) {
        printf("\nerror writing, device went offline\n");
        return;
      }
    }
  }
}

int main() {
	int r;

	printf("Waiting for device:");
	fflush(stdout);
  while (1) {
    r = rawhid_open(1, VENDOR_ID, PRODUCT_ID, RAWHID_USAGE_PAGE, RAWHID_USAGE);
    if (r <= 0) {
      printf(".");
			fflush(stdout);
      delay_ms(1000);
      continue;
    }
    printf("found rawhid device\n");

    use_device();
  }
}

#if defined(OS_LINUX) || defined(OS_MACOSX)
// Linux (POSIX) implementation of _kbhit().
// Morgan McGuire, morgan@cs.brown.edu
static int _kbhit() {
	static const int STDIN = 0;
	static int initialized = 0;
	int bytesWaiting;

	if (!initialized) {
		// Use termios to turn off line buffering
		struct termios term;
		tcgetattr(STDIN, &term);
		term.c_lflag &= ~ICANON;
		tcsetattr(STDIN, TCSANOW, &term);
		setbuf(stdin, NULL);
		initialized = 1;
	}
	ioctl(STDIN, FIONREAD, &bytesWaiting);
	return bytesWaiting;
}
static char _getch(void) {
	char c;
	if (fread(&c, 1, 1, stdin) < 1) return 0;
	return c;
}
#endif


static char get_keystroke(void)
{
	if (_kbhit()) {
		char c = _getch();
		if (c >= 32) return c;
	}
	return 0;
}


#if (defined(WIN32) || defined(WINDOWS) || defined(__WINDOWS__))
#include <windows.h>
static void delay_ms(unsigned int msec)
{
	Sleep(msec);
}
#else
#include <unistd.h>
static void delay_ms(unsigned int msec)
{
	usleep(msec * 1000);
}
#endif
