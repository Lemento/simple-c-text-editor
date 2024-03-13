#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

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
	raw.c_lflag &= ~(ECHO); //  Turn off echoing
	raw.c_lflag &= ~(ICANON); // Turn off canonical mode
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(void)
{
	enableRawMode();

	//  Read 1 byte of user input
	//  terminal will not output user input and will exit on 'q' keypress
	printf("Press 'q' to exit\n");
	char c;
	while (read(STDIN_FILENO, &c, 1)==1 && c != 'q');

	return 0;
}
