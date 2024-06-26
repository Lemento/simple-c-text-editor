#define _DEFAUL_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

void enableRawMode(void);
void ted_init(void);
void ted_open(char const filename[]);
void ted_refreshScreen(void);
void ted_processKeypress(void);

#define KILO_VERSION "0.0.1"

#define CTRL_KEY(K) ((K) & 0x1F)

enum ted_Key
{
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

int main(int argc, char* argv[])
{
	enableRawMode();
	ted_init();
	if (argc >= 2)
	{ ted_open(argv[1]); }

	//  Read 1 byte of user input
	//  terminal will not output user input and will exit on 'q' keypress
	while (1)
	{
	  ted_refreshScreen();
	  ted_processKeypress();
	}

	return 0;
}

//  Returns size of current terminal in columns and rows
int getWindowSize (int* rows, int* cols)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{ return -1; }
	else
	{
	  *cols = ws.ws_col;
	  *rows = ws.ws_row;
	  return 0;
	}
}

void deathToMachine (char const* err_message)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(err_message);
	exit(1);
}

//  terminal screen buffer
struct abuf
{
	char* b;
	int len;
};
#define ABUF_INIT {NULL, 0}

void ab_append (struct abuf* ab, char const* s, int len)
{
	char*const new = realloc(ab->b, ab->len+len);

	if (new == NULL){ return; }
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void ab_free (struct abuf* ab)
{ free(ab->b); }

typedef struct
{
	int size;
	char* charas;
}  erow;

struct ted_Config
{
	int cx, cy;
	int screenRows;
	int screenCols;
	int numRows;
	erow *row;
	struct termios orig_termios;
};
struct ted_Config E;

void ted_init(void)
{
	E.cx = 0;
	E.cy = 0;
	E.numRows = 0;
	E.row = NULL;

	if (getWindowSize (&E.screenRows, &E.screenCols) == -1)
	{ deathToMachine("getWindowSize"); }
}

void ted_appendRow (char const* s, size_t len);
void ted_open(char const filename[])
{
	FILE*const fp = fopen(filename, "r");
	if (fp == NULL){ deathToMachine("fopen"); }

	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	linelen = getline(&line, &linecap, fp);
	while ((linelen = getline(&line, &linecap, fp)) != -1)
	{
	  while (linelen > 0 && (line[linelen-1] == '\n' ||
	                         line[linelen-1] == '\r'))
	  { linelen--; }
	  ted_appendRow (line, linelen);
	}

	free(line);
	fclose(fp);
}

void ted_appendRow (char const* s, size_t len)
{
	E.row = realloc (E.row, sizeof(erow)*(E.numRows+1));

	int at = E.numRows;
	E.row[at].size = len;
	E.row[at].charas = malloc(len+1);
	memcpy (E.row[at].charas, s, len);
	E.row[at].charas[len] = '\0';
	E.numRows++;
}

int ted_readKey(void)
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
	{
	  if (nread == -1 && errno != EAGAIN)
	  { deathToMachine("read"); }
	}

	if (c != '\x1b'){ return c; }
	
	char seq[3];

	if (read(STDIN_FILENO, &seq[0], 1) != 1){ return '\x1b'; }
	if (read(STDIN_FILENO, &seq[1], 1) != 1){ return '\x1b'; }

	if (seq[0] == '[')
	{
	  if (seq[1] >= '0' && seq[1] <= '9')
	  {
	    if (read(STDIN_FILENO, &seq[2], 1) != 1){ return '\x1b'; }
	    if (seq[2] == '~')
	    {
	      switch (seq[1])
	      {
	        case '1': return HOME_KEY;
		case '3': return DEL_KEY;
	        case '4': return END_KEY;
	        case '5': return PAGE_UP;
	        case '6': return PAGE_DOWN;
	        case '7': return HOME_KEY;
	        case '8': return END_KEY;
	      }
	    }
	  } else {
	    switch (seq[1])
	    {
	      case 'A': return ARROW_UP;
	      case 'B': return ARROW_DOWN;
	      case 'C': return ARROW_RIGHT;
	      case 'D': return ARROW_LEFT;
	      case 'H': return HOME_KEY;
	      case 'F': return END_KEY;
	    }
	  }

	} else if (seq[0] == 'O') {
	  switch (seq[1])
	  {
	    case 'H': return HOME_KEY;
	    case 'F': return END_KEY;
	  }
	}

	return '\x1b';
}

void ted_moveCursor (int key)
{
	switch (key)
	{
	  case ARROW_LEFT:
	    if (E.cx != 0)
	    { E.cx--; }
	    break;
	  case ARROW_RIGHT:
	    if (E.cx != E.screenCols - 1)
	    { E.cx++; }
	    break;
	  case ARROW_UP:
	    if (E.cy != 0)
	    { E.cy--; }
	    break;
	  case ARROW_DOWN:
	    if (E.cy != E.screenRows - 1)
	    { E.cy++; }
	    break;
	}
}

void ted_processKeypress(void)
{
	int c = ted_readKey();

	switch (c)
	{
	  case CTRL_KEY('q'): exit(0);
	    write(STDOUT_FILENO, "\x1b[2J", 4);
	    write(STDOUT_FILENO, "\x1b[H", 3);
	    exit(0);
	    break;

	  case HOME_KEY:
	    E.cx = 0;
	    break;
	  case END_KEY:
	    E.cx = E.screenCols - 1;
	    break;

	  case PAGE_UP:
	  case PAGE_DOWN:
	    {
	      int times = E.screenRows;
	      while (times--)
	      { ted_moveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN); }
	    }
	    break;

	  case ARROW_UP:
	  case ARROW_DOWN:
	  case ARROW_LEFT:
	  case ARROW_RIGHT:
	    ted_moveCursor(c);
	    break;

	  default: break;
	}
}

void ted_drawRows(struct abuf* ab)
{
	int y = 0;
	for (; y < E.screenRows; y++)
	{
	  if (y >= E.numRows)
	  {
	    if (E.numRows == 0 && y == E.screenRows/3)
	    {
	      char welcome[80];
	      int welcomelen = snprintf(welcome, sizeof(welcome),
	        "Kilo editor -- version %s", KILO_VERSION);
	      if (welcomelen > E.screenCols)
	      { welcomelen = E.screenCols; }
	      int padding = (E.screenCols - welcomelen) / 2;
	      if (padding)
	      {
	        ab_append(ab, "~", 1);
	        padding--;
	      }
	      while(padding--){ ab_append(ab, " ", 1); }
	      ab_append(ab, welcome, welcomelen);
	    }
	    else{ ab_append(ab, "~", 1); }
	  }
	  else
	  {
	    int len = E.row[y].size;
	    if (len > E.screenCols){ len = E.screenCols; }
	    ab_append(ab, E.row[y].charas, len);
	  }

	  ab_append(ab, "\x1b[K", 3);
	  if (y < E.screenRows - 1)
	  { ab_append(ab, "\r\n", 2); }
	}
}

void ted_refreshScreen(void)
{
	struct abuf ab = ABUF_INIT;

	ab_append(&ab, "\x1b[?25l", 6); // Hide cursor
	//ab_append(&ab, "\x1b[2J", 4);
	ab_append(&ab, "\x1b[H", 3);

	ted_drawRows(&ab);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy+1, E.cx+1);
	ab_append(&ab, buf, strlen(buf));

	ab_append(&ab, "\x1b[?25h", 6); // Show cursor

	write(STDOUT_FILENO, ab.b, ab.len);
	ab_free(&ab);
}

void disableRawMode(void)
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
	{ deathToMachine("tcsettattr"); }
}

void enableRawMode(void)
{
	//  Make copy of terminals original status
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
	{ deathToMachine("tcsettattr"); }
	//  Restore terminal status at program exit
	atexit(disableRawMode);

	struct termios raw = E.orig_termios;
	raw.c_iflag &= ~BRKINT;
	raw.c_iflag &= ~ICRNL; // Disable carriage return and new line
	raw.c_iflag &= ~INPCK;
	raw.c_iflag &= ~ISTRIP;
	raw.c_iflag &= ~IXON; // Turn off software flow control (Ctrl-S and Ctrl-Q)
	raw.c_oflag &= ~OPOST;
	raw.c_cflag |= CS8;
	raw.c_lflag &= ~ECHO; //  Turn off echoing
	raw.c_lflag &= ~ICANON; // Turn off canonical mode
	raw.c_lflag &= ~IEXTEN; // Turn off Ctrl-V (and Ctrl-O in MacOS)
	raw.c_lflag &= ~ISIG; // Turn off Ctrl-Z and Ctrl-D termination and suspend signals
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
	{ deathToMachine("tcsettattr"); }
}
