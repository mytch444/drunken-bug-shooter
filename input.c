#include <unistd.h>
#include <linux/kd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>

static struct termios tty_attr_old;
static int old_keyboard_mode;

/*
  I am trying to get raw input so I can shoot while moving
  but it doesnt seem to be a happening thing.
  Any help would be great.
 */

int setupkeyboard() {
  struct termios tty_attr;
  int flags;

  flags = fcntl(0, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(0, F_SETFL, flags);

  if (ioctl(0, KDGKBMODE, &old_keyboard_mode) < 0) {
    return 0;
  }

  tcgetattr(0, &tty_attr_old);
  
  tty_attr = tty_attr_old;
  tty_attr.c_lflag &= ~(ICANON | ECHO | ISIG);
  tty_attr.c_iflag &= ~(ISTRIP | INLCR | ICRNL | IGNCR | IXON | IXOFF);
  tcsetattr(0, TCSANOW, &tty_attr);

  ioctl(0, KDSKBMODE, K_RAW);
  return 1;
}

void restorekeyboard() {
  tcsetattr(0, TCSAFLUSH, &tty_attr_old);
  ioctl(0, KDSKBMODE, old_keyboard_mode);
}

void readkeyboard() {
  char buf[2];
  int res;
  
  running = 1;
  while (running) {
    res = read(0, &buf[0], 1);

    switch (buf[0]) {
    case 0x01:
      running = 0;
      break;
    }
  }
}

void curses_input() {
  int d;

  running = 1;
  while (running) {
    d = getch();
    if (d == 'q') running = false;
    if (d == KEY_DOWN) down();
    if (d == KEY_UP) up();
    if (d == ' ') shoot();    
  }
}

void *input() {
  running = 1;
  
  /*  if (setupkeyboard()) {
    readkeyboard();
    restorekeyboard(); 
  } else {
  */
  curses_input();
    //  }
}
