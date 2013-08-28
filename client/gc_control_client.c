#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/ioctl.h>
#include <termios.h>
#elif defined(OS_WINDOWS)
#include <conio.h>
#endif

#include "hid.h"
#include "../usb_rawhid_settings.h"

typedef uint16_t controller_status_t;

static char get_keystroke(void);
static void delay_ms(unsigned int msec);

struct keymap_mapping {
  char key;
  unsigned char low_byte;
  unsigned char high_byte;
} keymap[] = {
  {'h', 0x00, 0x81},
  {'j', 0x00, 0x84},
  {'k', 0x00, 0x88},
  {'l', 0x00, 0x82},
};

int read_from_device_and_print() {
  static unsigned char buf[RAWHID_TX_SIZE+1];

  // check if any Raw HID packet has arrived
  int num = rawhid_recv(0, buf, RAWHID_TX_SIZE, 5);
  if (num < 0) {
    printf("\nerror reading, device went offline\n");
    return num;
  }
  if (num > 0) {
    buf[RAWHID_TX_SIZE] = '\0';
    printf("recv: %s", buf);
  }
  return num;
}
int read_from_keyboard_and_send_cmd(void) {
  int i, num;
  char c;
  static unsigned char buf[RAWHID_RX_SIZE];
  // check if any input on stdin
  while ((c = get_keystroke()) >= 32) {
    buf[0] = 0x00; buf[1] = 0x80; // No buttons pressed
    for (i = 0; i < sizeof(keymap)/sizeof(struct keymap_mapping); i++) {
      struct keymap_mapping *current_mapping = &keymap[i];
      if (current_mapping->key == c) {
        buf[0] = current_mapping->low_byte;
        buf[1] = current_mapping->high_byte;
      }
    }

    printf("\ngot key '%c', sending command 0x%02x%02x\n", c, buf[0], buf[1]);
    num = rawhid_send(0, buf, RAWHID_RX_SIZE, 100);
    if (num < 0) {
      printf("\nerror writing, device went offline\n");
      return num;
    }
  }
  return 0;
}
void use_device_with_keyboard(void) {
  while (1) {
    if (read_from_device_and_print() < 0)
      return;
    if (read_from_keyboard_and_send_cmd() < 0)
      return;
  }
}

void die(const char *format, ... ) {
  va_list args;
  fprintf(stderr, "Error: ");
  va_start(args, format);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(1);
}

// Only use with a (pointer to) string literal as first argument!
int streq(const char *a, const char *b) {
  return strcmp(a, b) == 0;
}

#define STATUS_NO_BUTTONS 0x0080

void send_status(controller_status_t status) {
  int num;
  static unsigned char buf[RAWHID_RX_SIZE];

  buf[0] = (status >> 8) & 0xff;
  buf[1] = status & 0xff;
  //printf("Sending status 0x%04x\n", status);
  num = rawhid_send(0, buf, RAWHID_RX_SIZE, 200);
  if (num < 0)
    die("Sending status failed, device went offline\n");
  else if (num == 0)
    die("Sending status failed, timeout was reached\n");
}

static const struct button_entry {
  const char *name;
  int bit_idx;
} buttons[] = {
  {"Left",   0},
  {"Right",  1},
  {"Down",   2},
  {"Up",     3},
  {"Z",      4},
  {"A",      8},
  {"B",      9},
  {"Start", 12},
};
static int get_button_idx(const char *button_name) {
  int i;
  for (i=0; i<sizeof(buttons)/sizeof(struct button_entry); i++)
    if (streq(buttons[i].name, button_name))
      return buttons[i].bit_idx;
  die("Could not find btn_index for button '%s'", button_name);
  return -1;
}
void press_button(controller_status_t *status, const char *button_name) {
  int bit_idx = get_button_idx(button_name);
  *status = *status | (1 << bit_idx);
  send_status(*status);
}

void release_button(controller_status_t *status, const char *button_name) {
  int bit_idx = get_button_idx(button_name);
  *status = *status & ~(1 << bit_idx);
  send_status(*status);
}

/* Returns dirname of path in a malloc'd buffer */
static char *mallocing_dirname(const char *path) {
  char *tmp, *dirname_of_path;

  tmp = strdup(path);
  if (tmp == NULL)
    return NULL;
  dirname_of_path = strdup(dirname(tmp));
  if (dirname_of_path == NULL)
    return NULL;
  free(tmp);

  return dirname_of_path;
}

static char *gnu_getcwd() {
  size_t size = 100;

  while (1) {
    char *buffer = (char*)malloc(size);
    if (buffer == NULL)
      return 0;
    if (getcwd(buffer, size) == buffer)
      return buffer;
    free(buffer);
    if (errno != ERANGE)
      return 0;
    size *= 2;
  }
}

#define MAX_TOKEN_LENGTH 64
#define STRING_TOKEN_SCANF_FMT "%64s"

void play_macro(const char *filename) {
  char *old_cwd, *dirname_of_file;

  char cmd[MAX_TOKEN_LENGTH+1], str_arg[MAX_TOKEN_LENGTH+1];
  double decimal_arg;
  unsigned int uint_arg;
  static controller_status_t controller_status = STATUS_NO_BUTTONS;

  FILE *f = fopen(filename, "r");
  if (!f) {
    printf("Unable to open file %s\n", filename);
    exit(1);
  }

  if ((old_cwd = gnu_getcwd()) == NULL)
    die("Getting CWD failed: %s", strerror(errno));

  if ((dirname_of_file = mallocing_dirname(filename)) == NULL)
    die("mallocing_dirname failed");
  chdir(dirname_of_file);
  free(dirname_of_file);

  /*
   * Macro syntax:
   *    Until-end-of-line-comment character is '#'
   *    PressAndRelease <ButtonName(string)>
   *    Sleep <NumberOfSeconds(double)>
   *    Import <filename(string)> <numberOfRepeats(integer)>
   */
  while (1) {
    if (fscanf(f, STRING_TOKEN_SCANF_FMT, cmd) < 1)
      break;

    if (cmd[0] == '#') { // # starts a until-end-of-line comment
      char c;
      printf(cmd);
      while ((c = fgetc(f)) != '\n' && c != EOF)
        putchar(c);
      putchar('\n');
    } else if (streq("PressAndRelease", cmd)) {
      if (fscanf(f, STRING_TOKEN_SCANF_FMT, str_arg) < 1)
        die("Argument of PressAndRelease missing");
      printf("%s %s\n", cmd, str_arg);
      press_button(&controller_status, str_arg);
      delay_ms(100);
      release_button(&controller_status, str_arg);
    } else if (streq("Sleep", cmd)) {
      if (fscanf(f, "%lf", &decimal_arg) < 1)
        die("Argument of Sleep invalid or missing");
      printf("%s %f\n", cmd, decimal_arg);
      delay_ms(decimal_arg * 1000);
    } else if (streq("Import", cmd)) {
      int i;
      if (fscanf(f, STRING_TOKEN_SCANF_FMT " %u", str_arg, &uint_arg) < 2)
        die("Argument(s) of Import invalid or missing");
      printf("%s %s %u\n", cmd, str_arg, uint_arg);
      for (i=1;  i<= uint_arg; i++) {
        printf("Playing macro %s", str_arg);
        if (uint_arg > 1)
          printf(" (Repeat %d of %d)", i, uint_arg);
        putchar('\n');
        play_macro(str_arg);
      }
    }
  }

  if (chdir(old_cwd) != 0)
    die("chdir to %s failed", old_cwd);
  free(old_cwd);
  fclose(f);
}

void die_gracefully(int signal_number) {
  printf("Caught signal %d, dying gracefully...\n", signal_number);
  send_status(STATUS_NO_BUTTONS);
  exit(0);
}

int main(int argc, char **argv) {
	int r;

  signal(SIGINT, die_gracefully);
  signal(SIGQUIT, die_gracefully);
  signal(SIGTERM, die_gracefully);
  signal(SIGKILL, die_gracefully);

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

    if (argc == 2) {
      while (1) {
        printf("Playing macro %s\n", argv[1]);
        play_macro(argv[1]);
      }
    } else {
      use_device_with_keyboard();
    }
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
