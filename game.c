// 063A12AFA9527463553D6CBBBC8EB3EFE88744D3990638A61B0E45F2BFD56E47FFBBED876FE5 I think

#include <curses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

char *highscoresname = ".drunken-bug-shooter";
char *tmphighscoresname = ".drunken-bug-shooter.tmp";

typedef struct _object {
    short r, c, w, h, speed;
    char madeof;
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
void resetbug(struct _object *b);

short ar, rmax, cmax, nbullets, nbugs;
object *bullets;
object *bugs;
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
    refresh();
}

void drawstring(char *string, short r, short c) {
    short i;
    for (i = 0; i < LEN(string); i++) draw(string[i], r, c + i);
}

void drawmessage(char *message, short offset) {
    short r = rmax / 2 + offset;
    short c = cmax / 2 - LEN(message) / 2;
    drawstring(message, r, c);
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
    short r, c;
    for (r = o.r; r < o.r + o.h; r++)
        for (c = o.c; c < o.c + o.w; c++)
            draw(o.madeof, r, c);
}

short collides(object a, object b) {
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


void *game() {
    short i;
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

            short j;
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

            short r, c;
            for (r = 0; r < rmax; r++)
                for (c = 0; c < cmax; c++)
                    draw('#', r, c);

            drawmessage("Next Level", 0);
        }

        usleep(50000);
    }
}

void *input() {
    char d;
    int i;
    
    while (running) {
        d = getch();
        if (d == 'q') running = false;
        if (d == 'j') ar++;
        if (d == 'k') ar--;
        if (d == ' ') {
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

main(int argc, char *argv[]) {
    char d; 
    short i;
    WINDOW *wnd;

    char highscorespath[128] = {'\0'};
    char tmphighscorespath[128] = {'\0'};
    sprintf(highscorespath, "%s/%s", getenv("HOME"), highscoresname);
    sprintf(tmphighscorespath, "%s/%s", getenv("HOME"), tmphighscoresname);

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
    curs_set(0);
    getmaxyx(wnd, rmax, cmax);
    clear();
    refresh();

    if (argc > 1 && strcmp(argv[1], "-s") == 0) {
        FILE *hi;

        hi = fopen(highscorespath, "r");
        if (hi == NULL) {
            endwin();
            printf("Cant read highscores file!!!\n");
            return;
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
            printf("Cant read highscores file!!!\n");
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

        char d = getch();

        endwin();
        return;
    }

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
    drawmessage("Drunken Bug Shooter", -2);
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
    pthread_create(&pth, NULL, input, "drawing");

    game(pth);

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

    FILE *hi;
    int ishighscore = 0;
    int place = -1;

    hi = fopen(highscorespath, "r");
    if (hi == NULL) {
        ishighscore = 1;
        drawstring("Creating highscores file...", rmax - 1, cmax / 2 - 9);
        
        FILE *h = fopen(highscorespath, "w");
        if (h == NULL) {
            endwin();
            printf("Cant create new highscores file and there is none to read!\n");
            return;
        }
        fclose(h);

        hi = fopen(highscorespath, "r");
        if (hi == NULL) {
            endwin();
            printf("Cant create new highscores file and there is none to read!\n");
            return;
        }
    }
    
    i = 0;
    if (!ishighscore) {
        while (1) {
            char line[80] = {'\0'};;
            char hname[80] = {'\0'};
            int hscore;
            if (!fgets(line, sizeof line, hi)) break;
            hscore = getnamescore(line, hname);
            if (place == -1 && score > hscore) {
                ishighscore = 1;
                place = i;
            }
            i++;
        }
        if (i < 10) ishighscore = 1;
        if (place == -1) place = i;
    }

    fclose(hi);

    hi = fopen(highscorespath, "r");
    if (hi == NULL) {
        endwin();
        printf("Can't read highscores file... But I could before so I have no fucking clue whats going on...\n");
        return;
    }

    if (ishighscore) {
        drawmessage("Highscore!", 1);
        drawmessage("Enter a name below", 2);

        int r, c;
        r = rmax / 2 + 3;
        c = cmax / 2;
        move(r, c);
        curs_set(1);

        char name[80] = {'\0'};;
        int d;
        i = 0;
        while (1) {
            d = getch();
            if (d == '\n') {
                if (!name[0]) continue;
                break;
            }
            if (d == KEY_BACKSPACE || d == KEY_DC || d == 127 || d == "^G") {
                if (i > 0) {
                    i -= 2;
                    name[i + 1] = '\0';
                } else {
                    i = -1;
                    name[0] = '\0';
                }
            } else if (d == ':') continue;
            else {
                name[i] = d;
            }

            int j;
            for (j = 0; j < cmax; j++) draw(' ', r, j);
            for (j = 0; j < 80 && name[j]; j++) {
                draw(name[j], r, c - i / 2 + j);
            }
            move(r, c - i / 2 + j);

            i++;
        }

        FILE *ho;
        ho = fopen(tmphighscorespath, "w");
        if (ho == NULL) {
            drawstring("Cant write highscores!", rmax - 1, cmax / 2 - 11);
            char d = getch();
            endwin();
            return;
        }

        for (i = 0; i < place; i++) {
            char line[80] = {'\0'}; 
            fgets(line, sizeof line, hi);
            fprintf(ho, "%s", line);
        }

        fprintf(ho, "%s:%i\n", name, score);

        for (i++; i < 10; i++) {
            char line[80] = {'\0'}; 
            if (!fgets(line, sizeof line, hi)) break;
            fprintf(ho, "%s", line);
        }

        fclose(hi);
        fclose(ho);

        hi = fopen(tmphighscorespath, "r");
        ho = fopen(highscorespath, "w");

        if (!hi || !ho) {
            drawstring("Cant write or read highscores!", rmax - 1, cmax / 2 - 15);
            char d = getch();
            endwin();
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
    } else {
        while (d = getch()) if (d == 'q') break;
    }

    endwin();
}
