#include <linux/input.h>
#include <curses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int nbugparts = 5;
char *bugparts[5] = {"bb\nbb", "bbbbb\nb     \nb     \nb     \nbbbbb\n", "bbbb\nb  b\nb  b\nbbbb\n", "bbb\nbbb\nbbb", "bbbbb\n"};

typedef struct _object {
  short r, c, w, h, speed;
  char *str;
  int *map;
  int health;
} object;

short LEN(const char *str);
short randr(short min, short max);
void draw(char dc, short r, short c);
void drawstring(char *string, short r, short c);
void drawmessage(char *message, short offset);
void map(struct _object *o);
void clearrow(int r);
void drawobject(object o);
short collides(object a, object b);
void newbug(struct _object *b);
void newbullet(struct _object *b, int r, int c);
void copyobject(struct _object *n, struct _object *o);
void up();
void down();
void addbullet();

void game();
void newgame();

int rmax, cmax, nbullets, nbugs;
object *bullets;
object *bugs;
object arrow;
short lives, running, score, level, levels;
int shootd, upd, downd;

#include "highscores.c"
#include "input.c"

short LEN(const char *str) {
  const char *s;
  for (s = str; *s; ++s);
  return (s - str);
}

short randr(short min, short max) {
  return (rand() % (max - min) + min);
}

void draw(char dc, short r, short c) {
  if (r >= rmax || c >= cmax) return; 
  mvaddch(r, c, dc);
}

void drawstring(char *string, short r, short c) {
  mvaddnstr(r, c, string, cmax - c);
}

void drawmessage(char *message, short offset) {
  short r = rmax / 2 + offset;
  short c = cmax / 2 - LEN(message) / 2;
  drawstring(message, r, c);
}

void map(struct _object *o) {
  int size = LEN((*o).str) * 2;
  int *map = malloc(sizeof(int) * size);
  int i, j;
  int r, c;
  int health = 0;
  i = r = c = 0;
  for (j = 0; (*o).str[j] != '\0'; j++) {
    if ((*o).str[j] == '\n') {
      r++;
      c = 0;
      continue;
    } else if ((*o).str[j] != ' ') {
      map[i] = r;
      map[i + 1] = c;      
      i += 2;
      health++;
    }
    c++;
  }
  map[i] = -1;

  (*o).map = map;
  (*o).health = health / 7 + 1;
}

void clearrow(int r) {
  char space[cmax];
  int i;
  for (i = 0; i < cmax - 1; i++) space[i] = ' ';
  space[i] = '\0';
  
  drawstring(space, r, 0);
}

void drawobject(object o) {
  int i, j, l, pr;
  i = pr = 0;
  for (j = 0; o.str[j] != '\0'; j++) {
    if (o.str[j] == '\n') {
      for (l = 0; l < j - i; l++) {
	if (o.str[l + i] != ' ')
	  draw(o.str[l + i], o.r + pr, o.c + l);
      }
      
      pr++;
      i = j + 1;
    }
  }
}

short collides(object a, object b) {
  int i, j;
  for (i = 0; a.map[i] != -1; i += 2) {
    for (j = 0; b.map[j] != -1; j += 2) {
      if (a.map[i] + a.r == b.map[j] + b.r) {
	if (a.map[i + 1] + a.c == b.map[j + 1] + b.c) {
	  return 1;
	}
      }
    }
  }
  
  return 0;
}

void newbug(struct _object *b) {
  (*b).speed = 1;
  (*b).r = randr(10, rmax);
  (*b).c = -15 - randr(0, 20);

  (*b).str = bugparts[randr(0, nbugparts)];

  map(&(*b));
}

void newbullet(struct _object *b, int r, int c) {
  (*b).r = r;
  (*b).c = c;
  (*b).w = 4;
  (*b).speed = 2;

  (*b).str = "----\n";
  map(&(*b));
}

void copyobject(struct _object *n, struct _object *o) {
  (*n).w = (*o).w;
  (*n).h = (*o).h;
  (*n).speed = (*o).speed;
  (*n).str = (*o).str;
  (*n).r = (*o).r;
  (*n).c = (*o).c;
  (*n).map = (*o).map;
}

void game() {
  short i;
  while (running) {
    if (lives < 1) running = 0;
    
    clear();

    if (shootd) addbullet();
    if (upd) up();
    if (downd) down();
    
    if (arrow.r + arrow.h / 2 > rmax - 1) arrow.r = rmax - 1 - arrow.h / 2;
    if (arrow.r + arrow.h / 2 < 0) arrow.r = -arrow.h / 2;

    drawobject(arrow);
    
    // draw and update bugs
    for (i = 0; i < nbugs; i++) {
      drawobject(bugs[i]);
      bugs[i].c += bugs[i].speed;
      if (bugs[i].c > cmax) {
	newbug(&bugs[i]);
	lives--;
      }

      // check for collisions with bullets
      short j;
      for (j = 0; j < nbullets; j++)
	if (bullets[j].speed && collides(bugs[i], bullets[j])) {
	  bullets[j].speed = bullets[j].c = 0;
	  score++;
	  bugs[i].health--;
	  if (bugs[i].health < 0) {
	    newbug(&bugs[i]);
	  }
	}
    }

    // draw and update bullets
    for (i = 0; i < nbullets; i++) {
      if (!bullets[i].speed) continue;
      drawobject(bullets[i]);
      bullets[i].c -= bullets[i].speed;
      if (bullets[i].c < -bullets[i].w) bullets[i].speed = 0;
    }
    
    char string[10] = {'\0'};
    sprintf(string, "s: %i   l: %i", score, lives);
    drawstring(string, 0, 0);

    // if time to add another bug... add another bug
    if (score > level) {
      level *= 2;
      levels++;
      
      object *old;
      old = malloc(nbugs * sizeof(object));
      for (i = 0; i < nbugs; i++) {
	copyobject(&old[i], &bugs[i]);
      }
      int oldn = nbugs;
      
      nbugs += levels;;
      bugs = realloc(bugs, nbugs * sizeof(object));
      for (i = 0; i < oldn; i++) {
	copyobject(&bugs[i], &old[i]);
      }
      for (; i < nbugs; i++) {
	newbug(&bugs[i]);
      }
    }
    
    refresh();
    usleep(35000);
  }
}

void up() {
  arrow.r--;
}

void down() {
  arrow.r++;
}

void addbullet() {
  int i;
  for (i = 0; i < nbullets; i++) {
    if (!bullets[i].speed) {
      newbullet(&bullets[i], arrow.r + arrow.h / 2, cmax);
      break;
    }
  }
}

void newgame() {
  char d; 
  short i;

  running = 1;
  
  nbullets = cmax / 2;
  nbugs = 5;
  
  lives = 3;
  score = 0;
  level = 25;
  levels = 1;

  arrow.r = rmax / 2;
  arrow.c = cmax - 3;
  arrow.w = 3;
  arrow.h = 7;
  arrow.str =
    "  *\n"
    " **\n"
    "***\n"
    "   \n"
    "***\n"
    " **\n"
    "  *\n";
    
  bullets = malloc(nbullets * sizeof(object));
  for (i = 0; i < nbullets; i++) {
    newbullet(&bullets[i], 0, 0);
  }
  
  bugs = malloc(nbugs * sizeof(object));
  for (i = 0; i < nbugs; i++) {
    newbug(&bugs[i]); 
  }
  
  drawmessage("Drunken Bug Shooter", -2);
  drawmessage("Stop the bugs from getting past you", -1);
  drawmessage("Arrow keys to move, space to shoot", 0);
  drawmessage("Q to exit", 1);
  drawmessage("Anything else to start", 2);
  d = getch();
  if (d == 'q') {
    return;
  }
  
  pthread_t pth;
  pthread_create(&pth, NULL, input, "get keys");

  game();
  
  pthread_cancel(pth);
  
  clear();

  drawmessage("Game Over", -1);
  char string[10] = {'\0'};
  sprintf(string, "%i", score);
  drawmessage(string, 0);
  drawmessage("Return to continue", 2);

  while ((d = getch()) && d != '\n')
    if (d == 'q') return;
  
  clear();
  highscore();

  clear();
  highscores();
  while ((d = getch()) && d != '\n')
    if (d == 'q') return;
}

main(int argc, char *argv[]) {
  WINDOW *wnd;
  
  sprintf(highscorespath, "%s/%s", getenv("HOME"), highscoresname);
  
  if (argc > 1 && strcmp(argv[1], "-c") == 0) {
    remove(highscorespath);
    printf("Removed highscores file.\n");
    return;
  }
  
  srand(time(NULL));
  
  wnd = initscr();
  keypad(stdscr, TRUE);
  cbreak();
  noecho();
  curs_set(0);
  getmaxyx(wnd, rmax, cmax);
  clear();
  
  if (argc > 1 && strcmp(argv[1], "-s") == 0) {
    highscores();
    endwin();
    return;
  }

  newgame();
  
  endwin();
}
