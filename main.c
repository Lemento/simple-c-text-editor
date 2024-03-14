#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

void enableRawMode(void);
int main(void)
{
	enableRawMode();

	//  Read 1 byte of user input
	//  terminal will not output user input and will exit on 'q' keypress
	printf("Press 'q' to exit\r\n");
	while (1)
	{
	  char c = '\0';
	  read(STDIN_FILENO, &c, 1);
	  // print every input characters ascii value
	  if (iscntrl(c))
	  { printf("%d\r\n", c); }
	  else{ printf("%d ('%c')\r\n", c, c); }

	  if (c == 'q') break;
	}

	return 0;
}

struct termios orig_termios;

void disableRawMode(void)
{ tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

void enableRawMode(void)
{
	//  Make copy of terminals original status
	tcgetattr(STDIN_FILENO, &orig_termios);
	//  Restore terminal status at program exit
	atexit(disableRawMode);

	struct termios raw = orig_termios;
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

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
