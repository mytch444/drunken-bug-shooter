#include <curses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

char *highscoresname = ".drunken-bug-shooter";
char *tmphighscorespath = "/tmp/drunken-bug-shooter";
char highscorespath[512];

typedef struct _object {
  short r, c, w, h, speed;
  char *str;
} object;

short LEN(const char *str);
short randr(short min, short max);
void draw(char dc, short r, short c);
void drawstring(char *string, short r, short c);
void drawmessage(char *message, short offset);
void drawarrow(short r);
void drawobject(object o);
short collides(object a, object b);
void *repeat();
void newbug(struct _object *b);
void newbullet(struct _object *b, int r, int c);
void copyobject(struct _object *n, struct _object *o);

int rmax, cmax, nbullets, nbugs;
object *bullets;
object *bugs;
object arrow;
short lives, running, score, level, levels;

short LEN(const char *str) {
  const char *s;
  for (s = str; *s; ++s);
  return (s - str);
}

short randr(short min, short max) {
  return (rand() % (max - min) + min);
}

void draw(char dc, short r, short c) {
  if (r > rmax || c > cmax) return; 
  move(r, c);
  delch();
  insch(dc);
}

void drawstring(char *string, short r, short c) {
  mvaddnstr(r, c, string, cmax - c);
}

void drawmessage(char *message, short offset) {
  short r = rmax / 2 + offset;
  short c = cmax / 2 - LEN(message) / 2;
  drawstring(message, r, c);
}

void clearrow(int r) {
  char space[cmax];
  int i;
  for (i = 0; i < cmax - 1; i++) space[i] = ' ';
  space[i] = '\0';
  
  drawstring(space, r, 0);
}

void drawarrow(short r) {
  short ri, ci;
  short c = cmax - 4; 
  for (ci = c; ci < c + 4; ci++) {
    for (ri = r - (ci - c); ri <= r + (ci - c); ri++) {
      if (ri == r) continue; 
      draw('*', ri, ci);
    }
  }
}

void drawobject(object o) {
  char s[16] = {'\0'};
  int i, j, l, pr;
  pr = 0;
  for (i = 0, j = 0; o.str[j] != '\0'; j++) {
    if (o.str[j] == '\n') {
      for (l = 0; l < j - i; l++) {
	s[l] = o.str[l + i];
      }
      drawstring(s, o.r + pr, o.c);
      pr++;
      i = j + 1;
    }
  }
}

short collides(object a, object b) {
  if (
      ( (b.c >= a.c && b.c <= a.c + a.w) || (a.c >= b.c && a.c <= b.c + b.w) ) &&
      ( (b.r >= a.r && b.r <= a.r + a.h) || (a.r >= b.r && a.r <= b.r + b.h) )
      ) return 1;
  
  return 0;
}

void newbug(struct _object *b) {
  (*b).w = 4;
  (*b).h = 4;
  (*b).speed = 1;
  (*b).r = randr(10, rmax);
  (*b).c = -2;
  
  (*b).str = "b  b\n bb \n bb \nb  b\n";
}

void newbullet(struct _object *b, int r, int c) {
  (*b).r = r;
  (*b).c = c;
  (*b).w = 4;
  (*b).h = 1;
  (*b).speed = 3;

  (*b).str = "----\n";
}

void copyobject(struct _object *n, struct _object *o) {
  (*n).w = (*o).w;
  (*n).h = (*o).h;
  (*n).speed = (*o).speed;
  (*n).str = (*o).str;
  (*n).r = (*o).r;
  (*n).c = (*o).c;
}

void game() {
  short i;
  while (running) {
    if (lives < 1) break;
    
    clear();
    
    //    drawarrow(ar);

    // draw and update bullets
    for (i = 0; i < nbullets; i++) {
      if (!bullets[i].speed) continue;
      drawobject(bullets[i]);
      bullets[i].c -= bullets[i].speed;
      if (bullets[i].c < -bullets[i].w) bullets[i].speed = 0;
    }

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
	  newbug(&bugs[i]);
	  bullets[j].speed = bullets[j].c = 0;
	  score++;
	}
    }

    drawobject(arrow);
    
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
    usleep(50000);
  }
}

void *input() {
  int d;
  int i;
  
  while (running) {
    d = getch();
    if (d == 'q') running = false;
    if (d == KEY_DOWN) arrow.r++;
    if (d == KEY_UP) arrow.r--;
    if (d == ' ') {
      for (i = 0; i < nbullets; i++) {
	if (!bullets[i].speed) {
	  newbullet(&bullets[i], arrow.r + arrow.h / 2, cmax);
	  break;
	}
      }
    }
    
    if (arrow.r > rmax - 1) arrow.r = rmax - 1;
    if (arrow.r < 0) arrow.r = 0;
  }
}

int getnamescore(char *line, char *name) {
  int i, s;
  s = 0;
  for (i = 0; i < 80; i++) {
    if (line[i] == ':') {
      s = i + 1;
      break;
    }
    name[i] = line[i]; 
  }
  
  char sscore[80] = {'\0'};
  for (i = s; i < 80 && line[i] != '\n' && line[i]; i++) sscore[i - s] = line[i];
  
  int score = atoi(sscore);
  return score;
}

void highscores() {
  FILE *hi;

  hi = fopen(highscorespath, "r");
  if (hi == NULL) {
    hi = fopen(highscorespath, "w");
    if (hi == NULL) {
      endwin();
      printf("Cant create new highscores file and there is none to read!\n");
      return;
    }
  }
    
  int n = 0;
  int max = 0;
  while (1) {
    char line[80] = {'\0'};
    char hname[80] = {'\0'};;
    int hscore;
    if (!fgets(line, sizeof line, hi)) break;
    hscore = getnamescore(line, hname);
    n++;
    if (LEN(hname) > max) max = LEN(hname);
  }
    
  fclose(hi);
  hi = fopen(highscorespath, "r");
  if (hi == NULL) {
    endwin();
    printf("Cant read highscores file!\n");
    return;
  }
      
  int r = rmax / 2 - n / 2;
  int nc = cmax / 2 - max / 2 - 2;
  int sc = cmax / 2 + max / 2 + 2;
  drawstring("Highscores", r - 2, cmax / 2 - 4);
  while (1) {
    char line[80] = {'\0'};
    char hname[80] = {'\0'};;
    int hscore;
    if (!fgets(line, sizeof line, hi)) break;
    hscore = getnamescore(line, hname);
      
    drawstring(hname, r, nc);
    char sscore[10] = {'\0'};
    sprintf(sscore, "%i", hscore);
    drawstring(sscore, r, sc);
    r++;
  }

  getch();
  endwin();
}

int checkhighscore(FILE *hi) {
  int place = 0;
  
  while (1) {
    char line[300] = {'\0'};;
    char hname[256] = {'\0'};
    int hscore;
    if (!fgets(line, sizeof line, hi)) break;
    hscore = getnamescore(line, hname);
    if (score > hscore) {
      break;
    }

    place++;
  }
  return place;
}


char* getstring(char *buf, int r, int c) {
  int i = 0;
  int j, d, offset;
  curs_set(1);
  while (1) {
    d = getch();
    
    if (d == '\n' && buf[0]) {
      break;
    } else if (d == KEY_BACKSPACE || d == KEY_DC || d == 127) {
      if (i > 0) {
	i--;
	buf[i] = '\0';
      }
    } else if (d == KEY_LEFT) {
      if (i > 0) i--;
    } else if (d == KEY_RIGHT) {
      if (i < LEN(buf)) i++;
    } else if (d == KEY_UP) {

    } else if (d == KEY_DOWN) {
      
    } else {
      for (j = LEN(buf); j > i; j--) buf[j] = buf[j - 1];
      buf[i] = d;
      i++;
    }

    offset = LEN(buf) / 2;
    
    clearrow(r);
    drawstring(buf, r, c - offset);
    move(r, c + i - offset);
    refresh();
  }

  curs_set(0);
  return buf;
}

void addhighscore(int place, char *name, int score) {
  FILE *hi, *ho;
  int i;

  ho = fopen(tmphighscorespath, "w");
  hi = fopen(highscorespath, "r");
  if (!hi || !ho) {
    drawmessage("Cant write or read highscores!", 0);
    return;
  }

  for (i = 0; i < place; i++) {
    char line[512] = {'\0'}; 
    fgets(line, sizeof line, hi);
    fprintf(ho, "%s", line);
  }

  fprintf(ho, "%s:%i\n", name, score);

  for (i++; i < 10; i++) {
    char line[80] = {'\0'}; 
    if (!fgets(line, sizeof line, hi)) break;
    fprintf(ho, "%s", line);
  }

  fclose(ho);

  hi = fopen(tmphighscorespath, "r");
  ho = fopen(highscorespath, "w");
    
  if (!hi || !ho) {
    drawmessage("Cant write or read highscores!", 0);
    return;
  }

  while (1) {
    char line[80] = {'\0'}; 
    if (!fgets(line, sizeof line, hi)) break;
    fprintf(ho, "%s", line);
  }
    
  fclose(hi);
  fclose(ho);
    
  remove(tmphighscorespath);
}

void highscore() {
  FILE *hi;
  int ishighscore = 0;
  int place = -1;
  
  hi = fopen(highscorespath, "r");
  if (hi == NULL) {
    ishighscore = 1;
    drawstring("Creating highscores file...", rmax - 1, cmax / 2 - 9);
    
    hi = fopen(highscorespath, "w");
    if (hi == NULL) {
      drawmessage("Cant create new highscores file and there is none to read!", 0);
      return;
    }
    fclose(hi);

    hi = fopen(highscorespath, "r");
    if (hi == NULL) {
      drawmessage("Cant create new highscores file and there is none to read!", 0);
      return;
    }
  }
  
  if (!ishighscore) {
    place = checkhighscore(hi);
    if (place < 10)
      ishighscore = 1;
  }
  
  fclose(hi);
  
  hi = fopen(highscorespath, "r");
  if (hi == NULL) {
    drawmessage("Can't read highscores file...", 0);
    return;
  }
  
  if (ishighscore) {
    drawmessage("Highscore!", 1);
    drawmessage("Enter a name below", 2);
    
    int r, c;
    r = rmax / 2 + 3;
    c = cmax / 2;
    move(r, c);
    
    char name[128] = {'\0'};
    *name = *getstring(name, r, c);

    addhighscore(place, name, score);
  }
}

void newgame() {
  char d; 
  short i;

  running = 1;
  
  //  ar = rmax / 2;
  
  nbullets = 10;
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
    "  *\n **\n***\n   \n***\n **\n  *\n";
    
  
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
    endwin();
    return;
  }
  
  pthread_t pth;
  pthread_create(&pth, NULL, input, "get keys");
  
  game();
  
  pthread_cancel(pth);
  
  if (lives <= 0) {
    clear();
    drawmessage("Game Over", -1);
    char string[10] = {'\0'};
    sprintf(string, "%i", score);
    drawmessage(string, 0);
  } else {
    endwin();
    return;
  }

  getch();
  clear();
  
  highscore();
  clear();
  highscores();
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
  //raw();
  noecho();
  keypad(wnd, TRUE);
  curs_set(0);
  getmaxyx(wnd, rmax, cmax);
  clear();
  
  if (argc > 1 && strcmp(argv[1], "-s") == 0) {
    highscores();
    return;
  }

  newgame();
  
  endwin();
}
