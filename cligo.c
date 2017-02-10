// stole some code from http://www.cs.uleth.ca/~holzmann/C/system/ttyraw.c
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#define CSI "\033["
#define kaya CSI "48;5;221m"
#define cursor CSI "48;5;196m"

#define clear CSI "2J" CSI "H"

#define black CSI "38;5;232m"
#define white CSI "38;5;231m"
#define bold 
#define nocolor CSI "m"

// enable
#define mousemode CSI "?1000h"
#define altscreen CSI "?1049h"
// disable
#define cleanup CSI "?1000l" CSI "?1049l"

/*#define xaxis "ABCDEFGHJKLMOPQRST"*/
#define xaxis "     A B C D E F G H J K L M N O P Q R S T     "
#define empty "                                               "

#define A 1
#define B 2
#define C 3
#define D 4
#define E 5
#define F 6
#define G 7
#define H 8
#define J 9
#define K 10
#define L 11
#define M 12
#define N 13
#define O 14
#define P 15
#define Q 16
#define R 17
#define S 18
#define T 19

char board[20][20];
char turn = 'b';

struct termios orig_term;

void printboard(void);
void tty_reset(void) {
  /*printf(cleanup);*/ // is it ub to use stdio in atexit?
  write(1, cleanup, sizeof cleanup -1);
  tcsetattr(0, TCSAFLUSH, &orig_term);
}

struct { int x, y; } cur = { 1, 1 };

static void up(void)    { cur.y = cur.y == 19 ? cur.y : cur.y + 1; }
static void down(void)  { cur.y = cur.y ==  1 ? cur.y : cur.y - 1; }
static void left(void)  { cur.x = cur.x ==  1 ? cur.x : cur.x - 1; }
static void right(void) { cur.x = cur.x == 19 ? cur.x : cur.x + 1; }

void click(int x, int y) {
  if (!board[x][y]) {
    board[x][y] = turn;
    turn = turn == 'b' ? 'w' : 'b';
  }
  cur.x = x;
  cur.y = y;
}

int main() {

  if (!isatty(0)) return 255;
  if (tcgetattr(0, &orig_term)) return 255;
  struct termios term;

#if 1
  term = orig_term;
  term.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  term.c_oflag &= ~(OPOST);
  term.c_cflag |= (CS8);
  // keep ISIG for tests
  term.c_lflag &= ~(ECHO | ICANON | IEXTEN /* | ISIG */);
  term.c_cc[VMIN] = 1;
  term.c_cc[VTIME] = 0;
#else
  cfmakeraw(&term);
#endif

  if (tcsetattr(0, TCSAFLUSH, &term)) return 255;
  atexit(tty_reset);
  printf(mousemode altscreen);

  char buf[20];
  while (1) {
    printboard();
    if (read(0, buf, 1) < 0) continue;

    switch (buf[0]) {
      case 'a': case 'h': left();  break;
      case 'd': case 'l': right(); break;
      case 's': case 'j': down();  break;
      case 'w': case 'k': up();    break;
      case ' ': click(cur.x, cur.y); break;
      case '\033':
        if (read(0, buf, 1) < 0) continue;
        switch (buf[0]) {
          case '[':
            if (read(0, buf, 1) < 0) continue;
            switch (buf[0]) {
              // arrows
              case 'A': up();    break;
              case 'B': down();  break;
              case 'C': right(); break;
              case 'D': left();  break;

              // mouse
              // we're in xterm's normal tracking mode
              case 'M':
                if (read(0, buf, 3) < 0) continue;
                // check if it's mouse button 1
                if ((buf[0] & 3) == 0) {
                  // xterm reports position + 32 for y but not for x?!?!
                  buf[1] -= 32;
                  buf[2] -= 32;

                  // 5 columns on the left for the numbers
                  // divide by 2 because cells are 2 character wide
                  buf[1] = (buf[1] - 5) / 2;

                  // subtract 2 header lines
                  // add 1 because 1-based indexing
                  buf[2] = 19 - (buf[2] - 2) + 1;

                  if (buf[1] >= 1 && buf[1] <= 19 && buf[2] >= 1 && buf[2] <= 19)
                    click(buf[1], buf[2]);
                  dprintf(2, "%d %d\n", buf[1], buf[2]);
                }
            }
        }
    }
  }
}

void printboard() {
  printf(clear);
  printf(kaya black empty nocolor "\r\n");
  printf(kaya black xaxis nocolor "\r\n");
  for (int j = 19; j > 0; j--) {
    printf(kaya black "  %2d ", j);
    for (int i = 1; i <= 19; i++) {

      if (cur.x == i && cur.y == j) printf(cursor);

      switch (board[i][j]) {
        case 'b': printf(bold black "⏺" ); break;
        case 'w': printf(bold white "⏺" ); break;
        /*case 'b': printf(bold black "•" nocolor kaya); break;*/
        /*case 'w': printf(bold white "•" nocolor kaya); break;*/
        default:  
          if ((i ==  4 && (j == 4 || j == 10 || j == 16)) ||
              (i == 10 && (j == 4 || j == 10 || j == 16)) ||
              (i == 16 && (j == 4 || j == 10 || j == 16)))
            printf(black "+");
          else
            /*printf(black "･");*/
            printf(black ".");
      }
      printf(" " nocolor kaya);
    }
    printf(kaya black "%2d  " nocolor "\r\n", j);
  }
  printf(kaya black xaxis nocolor "\r\n");
  printf(kaya black empty nocolor "\r\n");
}
