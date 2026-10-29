#include <ncurses.h>

int main(void) {
    initscr();

    mvprintw(2, 4, "Hello, ncurses!");
    mvprintw(4, 4, "Press any key to exit...");

    refresh();

    getch();

    endwin();
    return 0;
}
