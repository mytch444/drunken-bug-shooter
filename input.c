#include <unistd.h>
#include <linux/kd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>

char *keyboard = "/dev/input/event11";

/*
  I am trying to get raw input so I can shoot while moving
  but it doesnt seem to be a happening thing.
  Any help would be great.
 */

int rawinput() {
  int fd, rd, value;
  char name[256] = "Unknown";
  char *device = NULL;
  
  device = keyboard;

  if ((fd = open(device, O_RDONLY | O_NONBLOCK)) == -1)
    return 1;

  struct input_event ev[64];
  int size = sizeof(struct input_event);

  ioctl(fd, EVIOCGNAME (sizeof (name)), name);

  while (running) {
    //    usleep(1000);
    
    rd = read (fd, ev, size * 64);

    switch (ev[0].code) {
    case 16:
      running = 0;
      break;
    case 57:
      shootd = 1;
      break;
    case 103:
      upd = 1;
      break;
    case 108:
      downd = 1;
      break;
    }

    switch (ev[1].code) {
    case 16:
      running = 0;
      break;
    case 57:
      shootd = 0;
      break;
    case 103:
      upd = 0;
      break;
    case 108:
      downd = 0;
      break;
    }
  }

  return 0;
}

void curses_input() {
  int d;

  while (running) {
    d = getch();

    if (d == 'q') running = 0;
    if (d == KEY_DOWN) down();
    if (d == KEY_UP) up();
    if (d == ' ') addbullet();;
  }
}

void *input() {
  //  if (rawinput() == 1)
    curses_input();
}
