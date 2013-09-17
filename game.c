#include <curses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

typedef struct _object {
    int r, c, w, h, speed;
    char madeof;
} object;

int LEN(const char *str);
int randr(int min, int max);
void draw(char dc, int r, int c);
void drawstring(char *string, int r, int c);
void drawmessage(char *message, int offset);
void drawarrow(int r);
void drawobject(object o);
int collides(object a, object b);
void *repeat();
void resetbug(struct _object *b);

int ar, rmax, cmax, nbullets, nbugs;
object *bullets;
object *bugs;
int lives, running, score, level, levels;

int LEN(const char *str) {
    const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

int randr(int min, int max) {
    return (rand() % (max - min) + min);
}

void draw(char dc, int r, int c) {
    if (r > rmax || c > cmax) return; 
    move(r, c);
    delch();
    insch(dc);
    refresh();
}

void drawstring(char *string, int r, int c) {
    int i;
    for (i = 0; i < LEN(string); i++) draw(string[i], r, c + i);
}

void drawmessage(char *message, int offset) {
    int r = rmax / 2 + offset;
    int c = cmax / 2 - LEN(message) / 2;
    drawstring(message, r, c);
}

void drawarrow(int r) {
    int ri, ci;
    int c = cmax - 4; 
    for (ci = c; ci < c + 4; ci++) {
        for (ri = r - (ci - c); ri <= r + (ci - c); ri++) {
            if (ri == r) continue; 
            draw('*', ri, ci);
        }
    }
}

void drawobject(object o) {
    int r, c;
    for (r = o.r; r < o.r + o.h; r++)
        for (c = o.c; c < o.c + o.w; c++)
            draw(o.madeof, r, c);
}

int collides(object a, object b) {
    if (
            ( (b.c >= a.c && b.c <= a.c + a.w) || (a.c >= b.c && a.c <= b.c + b.w) ) &&
            ( (b.r >= a.r && b.r <= a.r + a.h) || (a.r >= b.r && a.r <= b.r + b.h) )
            ) return 1;
    return 0;
}

void resetbug(struct _object *b) {
    (*b).w = 2;
    (*b).h = 2;
    (*b).speed = 1;
    (*b).madeof = 'b';
    (*b).r = randr(10, rmax);
    (*b).c = -2;
}


void *repeat() {
    int i;
    while (running) {
        if (lives <= 0) break;

        clear();

        drawarrow(ar);

        for (i = 0; i < nbullets; i++) {
            if (!bullets[i].speed) continue;
            drawobject(bullets[i]);
            bullets[i].c -= bullets[i].speed;
            if (bullets[i].c < -bullets[i].w) bullets[i].speed = 0;
        }

        for (i = 0; i < nbugs; i++) {
            drawobject(bugs[i]);
            bugs[i].c += bugs[i].speed;
            if (bugs[i].c > cmax) {
                resetbug(&bugs[i]);
                lives--;
            }

            int j;
            for (j = 0; j < nbullets; j++)
                if (bullets[j].speed && collides(bugs[i], bullets[j])) {
                    resetbug(&bugs[i]);
                    bullets[j].speed = bullets[j].c = 0;
                    score++;
                }
        }

        char string[10] = {'\0'};
        sprintf(string, "s: %i   l: %i", score, lives);
        drawstring(string, 0, 0);

        if (score > level) {
            level *= 2;
            levels++;

            nbugs += levels;;
            bugs = realloc(bugs, nbugs * sizeof(object));
            for (i = 0; i < nbugs; i++) {
                resetbug(&bugs[i]); 
            }

            int r, c;
            for (r = 0; r < rmax; r++)
                for (c = 0; c < cmax; c++)
                    draw('#', r, c);

            drawmessage("Next Level", 0);
        }

        usleep(50000);
    }

    if (lives <= 0) {
        clear();
        drawmessage("Game Over", -1);
        char string[10] = {'\0'};
        sprintf(string, "%i", score);
        drawmessage(string, 0);
    }
}

main() {
    char d;
    int i;
    WINDOW *wnd;

    srand(time(NULL));

    wnd = initscr();
    //cbreak();
    raw();
    noecho();
    curs_set(0);
    getmaxyx(wnd, rmax, cmax);
    clear();
    refresh();

    running = 1;

    ar = rmax / 2;

    nbullets = 10;
    nbugs = 5;

    lives = 3;
    score = 0;
    level = 25;
    levels = 1;

    bullets = malloc(nbullets * sizeof(object));
    for (i = 0; i < nbullets; i++) {
        bullets[i].r = 0;
        bullets[i].c = 0;
        bullets[i].w = 4;
        bullets[i].h = 1;
        bullets[i].speed = 0;
        bullets[i].madeof = '-';
    }

    bugs = malloc(nbugs * sizeof(object));
    for (i = 0; i < nbugs; i++) {
        resetbug(&bugs[i]); 
    }

    clear();
    drawmessage("Game", -2);
    drawmessage("Stop the bugs from getting past you", -1);
    drawmessage("j for down, k for up, space to shoot", 0);
    drawmessage("q to exit", 1);
    drawmessage("Anything else to start", 2);
    d = getch();
    if (d == 'q') {
        endwin();
        return;
    }

    pthread_t pth;
    pthread_create(&pth, NULL, repeat, "drawing");

    while (running) {
        d = getch();
        if (d == 'q') break;
        if (d == 'j') ar++;
        if (d == 'k') ar--;
        if (d == ' ') {
            int i;
            for (i = 0; i < nbullets; i++) {
                if (!bullets[i].speed) {
                    bullets[i].r = ar;
                    bullets[i].c = cmax;
                    bullets[i].w = 4;
                    bullets[i].h = 1;
                    bullets[i].speed = 3;
                    bullets[i].madeof = '-';
                    break;
                }
            }
        }

        if (ar > rmax - 1) ar = rmax - 1;
        if (ar < 0) ar = 0;
    }

    pthread_cancel(pth);
    endwin();
}
