#include <stdlib.h>    
#include <stdio.h> 
#include <unistd.h>      
#include <ncurses.h>   
#include <sys/time.h> 
#include <time.h>      
#include <string.h> 
#include <signal.h>
#include <termios.h>
#include "sudoku.h"

/* DEFINES */
#define TRUE 1
#define FALSE 0

#define GRID_LINES 19
#define GRID_COLS 37
#define GRID_Y 3
#define GRID_X 3
#define INFO_LINES 12
#define INFO_COLS 30
#define INFO_Y 3
#define INFO_X GRID_X + GRID_COLS + 5

#define LEVEL_LINES 1
#define LEVEL_COLS 12
#define LEVEL_Y 25
#define LEVEL_X 1

#define GRID_NUMBER_START_Y 1
#define GRID_NUMBER_START_X 2
#define GRID_LINE_DELTA 4
#define GRID_COL_DELTA 2
#define STATUS_LINES 1
#define STATUS_COLS GRID_COLS + INFO_COLS
#define STATUS_Y 2
#define STATUS_X GRID_X

#define MAX_HINT_TRY 5
#define SUDOKU_LENGTH STREAM_LENGTH - 1
#define INTRO ".WELCOME.............TO..............SUDOKU.............WORLD"



/* ���� */
static int g_playing = FALSE;
static char* g_stream; /* ��Ʈ������ */
static char plain_board[STREAM_LENGTH];
static char user_board[STREAM_LENGTH];
static DIFFICULTY g_level;
static WINDOW *grid, *infobox,*levelbox, *status;

struct timespec delay;
struct timespec prev;
int paused=0;
int hint_try = 0;
/* �Լ� */

static int is_valid_stream(char *s)
{
   char *p = s;
   short n = 0;
   while ((*p) != '\0')
   {
      if (n++ > SUDOKU_LENGTH)
         break;

      //���ڰ� 1���� 9 ���� �Ǵ� . ���� �˻� 
      if(!((*p >= 49 && *p <= 57) || *p == '.' ))
      {
         printf("Character %c at position %d is not allowed.\n", *p, n);
         return FALSE;
      }
      p++; // ���� ����
   }

   // stream�� ���̰� �������� ����(81)�� ��ġ �ϴ��� 
   if (n != SUDOKU_LENGTH )
   {
      printf("Stream has to be %d characters long.\n", SUDOKU_LENGTH);
      return FALSE;
   }

   // ������ ���� �˻�
   if (!is_valid_puzzle(s))
   {
      printf("Stream does not represent a valid sudoku puzzle.\n");
      return FALSE;
   }
   // ���� ������ ��� �ش����� ������, �� stream�� valid ��
   return TRUE;
}

// Ŀ�� �ʱ�ȭ
static void init_curses(void)
{
    initscr();
    clear();
    endwin();
    cbreak();
    noecho();

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
        
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
}

// ������ ȭ�鿡 �׸���
static void _draw_grid()
{
   // �� �Լ����� grid�� ���������� ����� WINDOW *�� ����

   int i, j;

   for(i = 0;i < 10;i++)
   {
         for(j = 0;j < 10;j++)
         {        
            if(i % 3 == 0)
               wattron(grid, A_BOLD|COLOR_PAIR(2));
            if(j % 3 == 0)
               wattron(grid, A_BOLD|COLOR_PAIR(2));

            wprintw(grid, "+");
            if((j % 3 == 0) && (i % 3 != 0))
            {
                 wattron(grid, A_BOLD|COLOR_PAIR(1));
            }
            if(j < 9)
               wprintw(grid, "---");
            if(i % 3 == 0)
            {
               wattron(grid, A_BOLD|COLOR_PAIR(1));
            }
        } 
      
        for(j = 0;j < 10 && i < 9;j++)
         {
           if(j % 3 == 0)
               wattron(grid, A_BOLD|COLOR_PAIR(2));
            if(j > 0)
               wprintw(grid, "   |");
            else
               wprintw(grid, "|");
            if(j % 3 == 0)
            {
               wattron(grid, A_BOLD|COLOR_PAIR(1));
            }
        }
   }
}

void showWindow()
{
    infobox = newwin(INFO_LINES, INFO_COLS, INFO_Y, INFO_X);
    wbkgd(infobox, COLOR_PAIR(2));

   
    wattron(infobox, A_BOLD|COLOR_PAIR(4));

   wprintw(infobox, "how to play?\n");
 
    wattroff(infobox, A_BOLD|COLOR_PAIR(2));
    wattron(infobox, COLOR_PAIR(3));
   

   
   wprintw(infobox, " S - Start play\n");
    wprintw(infobox, " N - New puzzle\n");
    wprintw(infobox, " x - Delete number\n");
   if(MAX_HINT_TRY>hint_try)
      wprintw(infobox, " H - Give a hint(max x%d)\n",MAX_HINT_TRY-hint_try);
   else if(MAX_HINT_TRY<=hint_try)
      wprintw(infobox, " H - No hint!!!!!\n");
    wprintw(infobox, " A - Giva a answer\n");
    wprintw(infobox, " C - Check ur answer\n");
    wprintw(infobox, " Q - Quit\n\n\n");
   
    mvwprintw(infobox,11,0, "level: %s\n", difficulty_to_str(g_level));
    wattroff(infobox, COLOR_PAIR(1));
}

// window �ʱ�ȭ
static void init_windows(void)
{
    keypad(stdscr, true);
    status = newwin(STATUS_LINES, STATUS_COLS, STATUS_Y, STATUS_X);

    // ������ �׸��� â
    grid = newwin(GRID_LINES, GRID_COLS, GRID_Y, GRID_X);
    _draw_grid(); // grid �׸���

    //timer â
    levelbox = newwin(LEVEL_LINES, LEVEL_COLS, LEVEL_X, LEVEL_Y);
   showWindow();
   
}


// ���ڷ� 81�ڸ� ���ڿ� �޾Ƽ� �׸��� ���� ä��� 
static void fill_grid(char *board)
{
   int row, col, x, y;
   int n;
   int c;

   wstandend(grid);
   y = GRID_NUMBER_START_Y;
   for(row=0; row < 9; row++)
   {
      x = GRID_NUMBER_START_X;
      for(col=0; col < 9; col++)
      {
         n = board[row*9+col];
         if(n == '.') // .�� ��ĭ���� ó�� 
            c = ' ';
         else
            c = n; 
         mvwprintw(grid, y, x, "%c", c);
         x += GRID_LINE_DELTA;
      }
      y += GRID_COL_DELTA;
   }
}

// �� ���� ���� 
static void new_puzzle(void)
{
   int holes = get_holes(g_level); // ���̵��� ���� ��ĭ �� ���� 
   char* stream;

   if (g_stream) // ���� ���� ������ ��Ʈ���� ������, �ش� ��Ʈ�� ���
      stream = g_stream;
   else // �׷��� ������ �������� �� ���� ����
      stream = generate_puzzle(holes);

   //todo
   strcpy(plain_board, stream);
   strcpy(user_board, stream);

   if (!g_stream)
      free(stream);

   // ������ ��Ʈ������ �׸��� ä���
   fill_grid(plain_board);

   g_playing = TRUE;
}

// ��Ʈ �Լ� 


static int hint()
{
   char tmp_board[STREAM_LENGTH];
   int i, j, solved;

   strcpy(tmp_board, user_board);
   solved = solve(tmp_board); // ���� Ǯ�
   
   if (solved != 0)
   {
      while(hint_try < MAX_HINT_TRY)
      {
         // ���� ��ǥ�� . �̸� �� �ڸ� �������� ä��� ��
         i = rand() % 8 + 1;
         j = rand() % 8 + 1;
         
         if ( user_board[i*9+j] == '.' && hint_try < MAX_HINT_TRY)
         {
            user_board[i*9+j] = tmp_board[i*9+j];
            hint_try++;
            return TRUE;
         }
      } 
      if(hint_try >= MAX_HINT_TRY)
      {
         werase(status);
        mvwprintw(status, 0, 0, "You can not get a hint anymore\n");
      }
   }
   return FALSE;
}

int set_ticker( int n_msecs )
{
        struct itimerval new_timeset;
        long    n_sec, n_usecs;

        n_sec = n_msecs / 1000 ;      /* int part   */
        n_usecs = ( n_msecs % 1000 ) * 1000L ;   /* remainder   */

        new_timeset.it_interval.tv_sec  = n_sec;        /* set reload       */
        new_timeset.it_interval.tv_usec = n_usecs;      /* new ticker value */
        new_timeset.it_value.tv_sec     = n_sec  ;      /* store this       */
        new_timeset.it_value.tv_usec    = n_usecs ;     /* and this         */

   return setitimer(ITIMER_REAL, &new_timeset, NULL);
}


int num = 30;
void _countdown(int signum)
{
   int y, x;
    getyx(grid, y, x);
   mvprintw(25,1, "%d", num--);
   //wmove(grid, y, x);
   move(y+3,x+3);
   refresh();
   if ( num < 0 ){
      mvprintw(25,1,"DONE!\n");
   }
}
int main(int argc, char *argv[])
{
   int run = TRUE;
   int key, x, y, posx, posy;
   void _countdown(int);

   g_stream = NULL;

   signal(SIGALRM, _countdown);
   
   if ( set_ticker(1000) == -1 )
      perror("set_ticker");

   // curse�� window �ʱ�ȭ 
   init_curses();
   init_windows();

   srand(time(NULL)); // �����Լ� �õ� ���� 

   strcpy(plain_board, INTRO);
   strcpy(user_board, INTRO);
   fill_grid(plain_board);
   
   refresh();
   wrefresh(grid);
   wrefresh(infobox);
   wrefresh(levelbox);

   y = GRID_NUMBER_START_Y;
   x = GRID_NUMBER_START_X;
   wmove(grid, y, x);

   while(run)
   {

      mvprintw(0, 0, "welcome to sudoku world");
      refresh();
      wrefresh(grid);
      key = getch(); // �Է��� Ű�� ���� ���� ���� 
      werase(status);

      switch(key)
      {
         case KEY_LEFT:
            if(x>5)
               x -= GRID_LINE_DELTA;
            break;
         case KEY_RIGHT:
            if(x<34)
               x += GRID_LINE_DELTA;
            break;  
         case KEY_UP:
            if(y>2)
               y -= GRID_COL_DELTA;
            break;
         case KEY_DOWN:
            if(y<17)
               y += GRID_COL_DELTA;
            break;
          //���� 
         case 'Q':
         case 27: 
            run = FALSE;
            break;

         case 'A': // ���� Ǯ�� 
            if(g_playing)
            {
               werase(status);
               mvwprintw(status, 0, 0, "Solving puzzle...");
               refresh();
               wrefresh(status);
               solve(plain_board);
               fill_grid(plain_board);
               werase(status);
               mvwprintw(status, 0, 0, "You gave up...sorry about that :(");
               g_playing = FALSE;
            }
            break;


       case 'P': // ġƮŰ 
            if(g_playing)
            {
               werase(status);
               mvwprintw(status, 0, 0, "Solving puzzle...");
               refresh();
               wrefresh(status);
               solve(plain_board);
               fill_grid(plain_board);
               werase(status);
               strcpy(user_board,plain_board);
            }
            break;
         case 'S':
         case 'N': // �� ���� ����
         /* showWindow();*/
          hint_try= 0;
          
           if (!g_stream)
              mvwprintw(infobox,11,0, "level: %s\n", difficulty_to_str(g_level));
         showWindow();
            werase(status);
            mvwprintw(status, 0, 0, "Generating puzzle...");
         
            refresh();
            wrefresh(status);
            new_puzzle();
         
            num = 30;
            werase(status);
            g_playing = TRUE;

            if (g_stream)
            {
               free(g_stream);
               g_stream = NULL;
            }
            
         
            break;

         case 'C':   
         case 'c':
            if(g_playing)
            {
               int solvable;
               char tmp_board[STREAM_LENGTH];

               werase(status);

               strcpy(tmp_board, user_board);
               solvable= solve(tmp_board);

               if(solvable == 0)
               {
                  mvwprintw(status, 0, 0, "Not correct");
               }
               else
               {
                  if (strchr(user_board, '.') == NULL)
                  {
                     mvwprintw(status, 0, 0, "Solved!");
                     if(g_level == D_EASY)
                     {
                        mvwprintw(status, 0, 0, "NEXT LEVEL");
                        g_level = D_NORMAL;
                     }
                     else if(g_level == D_NORMAL)
                     {
                        mvwprintw(status, 0, 0, "NEXT LEVEL");
                        g_level = D_HARD;
                     }
                     else if(g_level == D_HARD)
                        mvwprintw(status, 0, 0, "You cleared final stage!");
                     

                     g_playing = FALSE;
                  }
                  else
                  {
                     mvwprintw(status, 0, 0, "You didn't even fill it up yet");
                  }
               }
            }
            break;
         // delete
         case KEY_DC:
         case KEY_BACKSPACE:
         case 127:
         case 'x':
            if(g_playing)
            {
               posy = (y-GRID_NUMBER_START_Y)/GRID_COL_DELTA;
               posx = (x-GRID_NUMBER_START_X)/GRID_LINE_DELTA;
               // if on empty position
               if(plain_board[posy*9+posx] == '.')
               {
                  user_board[posy*9+posx] = '.';
                  wprintw(grid, " ");
               }
               break;
            }

         case 'H':
            if (g_playing && hint())
            {
               fill_grid(user_board);
               werase(status);
               mvwprintw(status, 0, 0, "One blank was filled!");
            }
         showWindow();
            break;
         default:
            break;
      }
      /*if user inputs a number*/
      // ���� ȭ�鿡 ä���
     
      if(key >= 49 && key <= 57 && g_playing)
      {
         posy = (y-GRID_NUMBER_START_Y)/GRID_COL_DELTA;
         posx = (x-GRID_NUMBER_START_X)/GRID_LINE_DELTA;
         // if on empty position
         if(plain_board[posy*9+posx] == '.')
         {
            // add inputted number to grid
            wattron(grid, COLOR_PAIR(4));
            wprintw(grid, "%c", key);
            wattroff(grid, COLOR_PAIR(4));
            user_board[posy*9+posx] = key;
         }
      }

     
     wmove(grid, y,x);
      refresh();
      
      wrefresh(status);
      wrefresh(grid);
      wrefresh(infobox);
   }
   if (g_stream)
      free(g_stream);

   endwin();
   return EXIT_SUCCESS;
}