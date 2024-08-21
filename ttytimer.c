/*
 *      TTY-CLOCK Main file.
 *      Copyright © 2009-2018 tty-timer contributors
 *      Copyright © 2008 Martin Duquesnoy <xorg62@gmail.com>
 *      All rights reserved.
 *
 *      Redistribution and use in source and binary forms, with or without
 *      modification, are permitted provided that the following conditions are
 *      met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
 *      * Neither the name of the  nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 *      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *      "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *      A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *      OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *      LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ttytimer.h"

void
init(void)
{
     struct sigaction sig;
     setlocale(LC_TIME,"");

     ttytimer.bg = COLOR_BLACK;

     /* Init ncurses */
     if (ttytimer.tty) {
          FILE *ftty = fopen(ttytimer.tty, "r+");
          if (!ftty) {
               fprintf(stderr, "tty-timer: error: '%s' couldn't be opened: %s.\n",
                       ttytimer.tty, strerror(errno));
               exit(EXIT_FAILURE);
          }
          ttytimer.ttyscr = newterm(NULL, ftty, ftty);
          assert(ttytimer.ttyscr != NULL);
          set_term(ttytimer.ttyscr);
     } else
          initscr();

     cbreak();
     noecho();
     keypad(stdscr, true);
     start_color();
     curs_set(false);
     clear();

     /* Init default terminal color */
     if(use_default_colors() == OK)
          ttytimer.bg = -1;

     /* Init color pair */
     init_pair(0, ttytimer.bg, ttytimer.bg);
     init_pair(1, ttytimer.bg, ttytimer.option.color);
     init_pair(2, ttytimer.option.color, ttytimer.bg);
     init_pair(3, ttytimer.bg, ttytimer.option.timeup_color);
//     init_pair(0, ttytimer.bg, ttytimer.bg);
//     init_pair(1, ttytimer.bg, ttytimer.option.color);
//     init_pair(2, ttytimer.option.color, ttytimer.bg);
     refresh();

     /* Init signal handler */
     sig.sa_handler = signal_handler;
     sig.sa_flags   = 0;
     sigaction(SIGTERM,  &sig, NULL);
     sigaction(SIGINT,   &sig, NULL);
     sigaction(SIGSEGV,  &sig, NULL);

     /* Init global struct */
     ttytimer.running = true;
     if(!ttytimer.geo.x)
          ttytimer.geo.x = 0;
     if(!ttytimer.geo.y)
          ttytimer.geo.y = 0;
     if(!ttytimer.geo.a)
          ttytimer.geo.a = 1;
     if(!ttytimer.geo.b)
          ttytimer.geo.b = 1;
     ttytimer.geo.w = (ttytimer.option.second) ? SECFRAMEW : NORMFRAMEW;
     ttytimer.geo.h = 7;
     ttytimer.time.start_time=time(NULL);
     update_hour();

     /* Create timer win */
     ttytimer.framewin = newwin(ttytimer.geo.h,
                                ttytimer.geo.w,
                                ttytimer.geo.x,
                                ttytimer.geo.y);
     if(ttytimer.option.box) {
          box(ttytimer.framewin, 0, 0);
     }

     if (ttytimer.option.bold)
     {
          wattron(ttytimer.framewin, A_BLINK);
     }

     set_center(ttytimer.option.center);

     nodelay(stdscr, true);

     wrefresh(ttytimer.framewin);

     return;
}

void
signal_handler(int signal)
{
     switch(signal)
     {
     case SIGINT:
     case SIGTERM:
          ttytimer.running = false;
          break;
          /* Segmentation fault signal */
     case SIGSEGV:
          endwin();
          fprintf(stderr, "Segmentation fault.\n");
          exit(EXIT_FAILURE);
          break;
     }

     return;
}

void
cleanup(void)
{
     if (ttytimer.ttyscr)
          delscreen(ttytimer.ttyscr);

     free(ttytimer.tty);
}

void
update_hour(void)
{

     /* Set remaining time */
     ttytimer.time.seconds = ttytimer.time.initial_seconds - (time(NULL) - ttytimer.time.start_time);
     if (ttytimer.time.seconds < 0) ttytimer.time.seconds = 0;

     /* Calculate time */
     int hours = ttytimer.time.seconds / 3600;
     int minutes = ttytimer.time.seconds % 3600 / 60;
     int seconds = ttytimer.time.seconds % 60;

     /* Set hour */
     ttytimer.date.hour[0] = hours / 10 % 10;
     ttytimer.date.hour[1] = hours % 10;

     /* Set minutes */
     ttytimer.date.minute[0] = minutes / 10;
     ttytimer.date.minute[1] = minutes % 10;

     /* Set seconds */
     ttytimer.date.second[0] = seconds / 10;
     ttytimer.date.second[1] = seconds % 10;

     return;
}

void
draw_number(int n, int x, int y, bool time_up_blink)
{
     int i, sy = y;

     for(i = 0; i < 30; ++i, ++sy)
     {
          if(sy == y + 6)
          {
               sy = y;
               ++x;
          }

          if (ttytimer.option.bold)
               wattron(ttytimer.framewin, A_BLINK);
          else
               wattroff(ttytimer.framewin, A_BLINK);

          int color_idx = number[n][i/2];
          if(color_idx && time_up_blink)
               color_idx=3;
          wbkgdset(ttytimer.framewin, COLOR_PAIR(color_idx));
          mvwaddch(ttytimer.framewin, x, sy, ' ');
     }
     wrefresh(ttytimer.framewin);

     return;
}

void
draw_timer(void)
{
     bool time_up_blink = (ttytimer.time.seconds == 0 && time(NULL) % 2 == 0);

     /* Draw hour numbers */
     draw_number(ttytimer.date.hour[0], 1, 1, time_up_blink);
     draw_number(ttytimer.date.hour[1], 1, 8, time_up_blink);
     chtype dotcolor = COLOR_PAIR(1);
     if (ttytimer.option.blink && time(NULL) % 2 == 0)
          dotcolor = COLOR_PAIR(2);
     if (ttytimer.time.seconds==0 && time(NULL) % 2 == 0)
          dotcolor = COLOR_PAIR(3);

     /* 2 dot for number separation */
     wbkgdset(ttytimer.framewin, dotcolor);
     mvwaddstr(ttytimer.framewin, 2, 16, "  ");
     mvwaddstr(ttytimer.framewin, 4, 16, "  ");

     /* Draw minute numbers */
     draw_number(ttytimer.date.minute[0], 1, 20, time_up_blink);
     draw_number(ttytimer.date.minute[1], 1, 27, time_up_blink);

     /* Draw second if the option is enabled */
     if(ttytimer.option.second)
     {
          /* Again 2 dot for number separation */
          wbkgdset(ttytimer.framewin, dotcolor);
          mvwaddstr(ttytimer.framewin, 2, NORMFRAMEW, "  ");
          mvwaddstr(ttytimer.framewin, 4, NORMFRAMEW, "  ");

          /* Draw second numbers */
          draw_number(ttytimer.date.second[0], 1, 39, time_up_blink);
          draw_number(ttytimer.date.second[1], 1, 46, time_up_blink);
     }

     return;
}

void
timer_move(int x, int y, int w, int h)
{

     /* Erase border for a clean move */
     wbkgdset(ttytimer.framewin, COLOR_PAIR(0));
     wborder(ttytimer.framewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
     werase(ttytimer.framewin);
     wrefresh(ttytimer.framewin);

     /* Frame win move */
     mvwin(ttytimer.framewin, (ttytimer.geo.x = x), (ttytimer.geo.y = y));
     wresize(ttytimer.framewin, (ttytimer.geo.h = h), (ttytimer.geo.w = w));

     if (ttytimer.option.box)
     {
          box(ttytimer.framewin, 0, 0);
     }

     wrefresh(ttytimer.framewin);
     return;
}

/* Useless but fun :) */
void
timer_rebound(void)
{
     if(!ttytimer.option.rebound)
          return;

     if(ttytimer.geo.x < 1)
          ttytimer.geo.a = 1;
     if(ttytimer.geo.x > (LINES - ttytimer.geo.h - DATEWINH))
          ttytimer.geo.a = -1;
     if(ttytimer.geo.y < 1)
          ttytimer.geo.b = 1;
     if(ttytimer.geo.y > (COLS - ttytimer.geo.w - 1))
          ttytimer.geo.b = -1;

     timer_move(ttytimer.geo.x + ttytimer.geo.a,
                ttytimer.geo.y + ttytimer.geo.b,
                ttytimer.geo.w,
                ttytimer.geo.h);

     return;
}

void
set_second(void)
{
     int new_w = (((ttytimer.option.second = !ttytimer.option.second)) ? SECFRAMEW : NORMFRAMEW);
     int y_adj;

     for(y_adj = 0; (ttytimer.geo.y - y_adj) > (COLS - new_w - 1); ++y_adj);

     timer_move(ttytimer.geo.x, (ttytimer.geo.y - y_adj), new_w, ttytimer.geo.h);

     set_center(ttytimer.option.center);

     return;
}

void
set_center(bool b)
{
     if((ttytimer.option.center = b))
     {
          ttytimer.option.rebound = false;

          timer_move((LINES / 2 - (ttytimer.geo.h / 2)),
                     (COLS  / 2 - (ttytimer.geo.w / 2)),
                     ttytimer.geo.w,
                     ttytimer.geo.h);
     }

     return;
}

void
set_box(bool b)
{
     ttytimer.option.box = b;

     wbkgdset(ttytimer.framewin, COLOR_PAIR(0));

     if(ttytimer.option.box) {
          wbkgdset(ttytimer.framewin, COLOR_PAIR(0));
          box(ttytimer.framewin, 0, 0);
     }
     else {
          wborder(ttytimer.framewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
     }

     wrefresh(ttytimer.framewin);
}

void
key_event(void)
{
     int i, c;

     struct timespec length = { ttytimer.option.delay, ttytimer.option.nsdelay };
     
     fd_set rfds;
     FD_ZERO(&rfds);
     FD_SET(STDIN_FILENO, &rfds);

     if (ttytimer.option.screensaver)
     {
          c = wgetch(stdscr);
          if(c != ERR && ttytimer.option.noquit == false)
          {
               ttytimer.running = false;
          }
          else
          {
               nanosleep(&length, NULL);
               for(i = 0; i < 8; ++i)
                    if(c == (i + '0'))
                    {
                         ttytimer.option.color = i;
                         init_pair(1, ttytimer.bg, i);
                         init_pair(2, i, ttytimer.bg);
                    }
          }
          return;
     }


     switch(c = wgetch(stdscr))
     {
     case KEY_RESIZE:
          endwin();
          init();
          break;

     case KEY_UP:
     case 'k':
     case 'K':
          if(ttytimer.geo.x >= 1
             && !ttytimer.option.center)
               timer_move(ttytimer.geo.x - 1, ttytimer.geo.y, ttytimer.geo.w, ttytimer.geo.h);
          break;

     case KEY_DOWN:
     case 'j':
     case 'J':
          if(ttytimer.geo.x <= (LINES - ttytimer.geo.h - DATEWINH)
             && !ttytimer.option.center)
               timer_move(ttytimer.geo.x + 1, ttytimer.geo.y, ttytimer.geo.w, ttytimer.geo.h);
          break;

     case KEY_LEFT:
     case 'h':
     case 'H':
          if(ttytimer.geo.y >= 1
             && !ttytimer.option.center)
               timer_move(ttytimer.geo.x, ttytimer.geo.y - 1, ttytimer.geo.w, ttytimer.geo.h);
          break;

     case KEY_RIGHT:
     case 'l':
     case 'L':
          if(ttytimer.geo.y <= (COLS - ttytimer.geo.w - 1)
             && !ttytimer.option.center)
               timer_move(ttytimer.geo.x, ttytimer.geo.y + 1, ttytimer.geo.w, ttytimer.geo.h);
          break;

     case 'q':
     case 'Q':
          if (ttytimer.option.noquit == false)
               ttytimer.running = false;
          break;

     case 's':
     case 'S':
          set_second();
          break;

     case 'c':
     case 'C':
          set_center(!ttytimer.option.center);
          break;

     case 'b':
     case 'B':
          ttytimer.option.bold = !ttytimer.option.bold;
          break;

     case 'r':
     case 'R':
          ttytimer.option.rebound = !ttytimer.option.rebound;
          if(ttytimer.option.rebound && ttytimer.option.center)
               ttytimer.option.center = false;
          break;

     case 'x':
     case 'X':
          set_box(!ttytimer.option.box);
          break;

     case '0': case '1': case '2': case '3':
     case '4': case '5': case '6': case '7':
          i = c - '0';
          ttytimer.option.color = i;
          init_pair(1, ttytimer.bg, i);
          init_pair(2, i, ttytimer.bg);
          break;

     default:
          pselect(1, &rfds, NULL, NULL, &length, NULL);
     }

     return;
}

int
main(int argc, char **argv)
{
     int c;

     /* Alloc ttytimer */
     memset(&ttytimer, 0, sizeof(ttytimer_t));
     /* Default color */
     ttytimer.option.color = COLOR_GREEN; /* COLOR_GREEN = 2 */
     ttytimer.option.timeup_color = COLOR_RED; /* COLOR_GREEN = 2 */
     /* Default delay */
     ttytimer.option.delay = 1; /* 1FPS */
     ttytimer.option.nsdelay = 0; /* -0FPS */
     ttytimer.option.blink = false;
     /* Show seconds */
     ttytimer.option.second = 1;

     atexit(cleanup);

     while ((c = getopt(argc, argv, "ivScbrhBxnC:d:T:a:e:")) != -1)
     {
          switch(c)
          {
          case 'h':
          default:
               printf("usage : tty-timer [-ivScbrahBxn] [-C [0-7]] [-u [0-7]] [-d delay] [-a nsdelay] [-T tty] {time}\n"
                      "    -S            Screensaver mode                               \n"
                      "    -x            Show box                                       \n"
                      "    -c            Set the timer at the center of the terminal    \n"
                      "    -C [0-7]      Set the timer color                            \n"
                      "    -u [0-7]      Set the timer color when time is up            \n"
                      "    -b            Use bold colors                                \n"
                      "    -T tty        Display the timer on the specified terminal    \n"
                      "    -r            Do rebound the timer                           \n"
                      "    -n            Don't quit on keypress                         \n"
                      "    -v            Show tty-timer version                         \n"
                      "    -i            Show some info about tty-timer                 \n"
                      "    -h            Show this page                                 \n"
                      "    -B            Enable blinking colon                          \n"
                      "    -d delay      Set the delay between two redraws of the timer. Default 1s. \n"
                      "    -a nsdelay    Additional delay between two redraws in nanoseconds. Default 0ns.\n");
               exit(EXIT_SUCCESS);
               break;
          case 'i':
               puts("TTY-Clock 2 © by Martin Duquesnoy (xorg62@gmail.com), Grey (grey@greytheory.net)");
               exit(EXIT_SUCCESS);
               break;
          case 'v':
               puts("TTY-Clock 2 © devel version");
               exit(EXIT_SUCCESS);
               break;
          case 'S':
               ttytimer.option.screensaver = true;
               break;
          case 'c':
               ttytimer.option.center = true;
               break;
          case 'b':
               ttytimer.option.bold = true;
               break;
          case 'C':
               if(atoi(optarg) >= 0 && atoi(optarg) < 8)
                    ttytimer.option.color = atoi(optarg);
               break;
          case 'u':
               if(atoi(optarg) >= 0 && atoi(optarg) < 8)
                    ttytimer.option.timeup_color = atoi(optarg);
               break;
               
          case 'r':
               ttytimer.option.rebound = true;
               break;
          case 'd':
               if(atol(optarg) >= 0 && atol(optarg) < 100)
                    ttytimer.option.delay = atol(optarg);
               break;
          case 'B':
               ttytimer.option.blink = true;
               break;
          case 'a':
               if(atol(optarg) >= 0 && atol(optarg) < 1000000000)
                    ttytimer.option.nsdelay = atol(optarg);
               break;
          case 'x':
               ttytimer.option.box = true;
               break;
          case 'T': {
               struct stat sbuf;
               if (stat(optarg, &sbuf) == -1) {
                    fprintf(stderr, "tty-timer: error: couldn't stat '%s': %s.\n",
                              optarg, strerror(errno));
                    exit(EXIT_FAILURE);
               } else if (!S_ISCHR(sbuf.st_mode)) {
                    fprintf(stderr, "tty-timer: error: '%s' doesn't appear to be a character device.\n",
                              optarg);
                    exit(EXIT_FAILURE);
               } else {
                    free(ttytimer.tty);
                    ttytimer.tty = strdup(optarg);
               }}
               break;
          case 'n':
               ttytimer.option.noquit = true;
               break;
          }
     }

     if (optind < argc) {
          for ( int i = 0 ; i < strlen(argv[optind]) ; ++i) {
               if (!isdigit(argv[optind][i])){
                    fprintf(stderr, "tty-timer: error: time ('%s') isn't a number.\n",
                              optarg);
                    exit(EXIT_FAILURE);
               }
          }
          ttytimer.time.initial_seconds=atoi(argv[optind]);
     } else {
          fprintf(stderr, "tty-timer: error: time must be inputted.\n");
          exit(EXIT_FAILURE);
     }


     init();
     attron(A_BLINK);
     while(ttytimer.running)
     {
          timer_rebound();
          update_hour();
          draw_timer();
          key_event();
     }

     endwin();

     return 0;
}

// vim: expandtab tabstop=5 softtabstop=5 shiftwidth=5
