#include <ncurses.h>

int main(void) {
    // Setup
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // State
    const char *names[] = {"..", "Downloads", "notes.txt", "photo.png"};
    int count = 4;
    int highlight = 0;

    // The event loop
    while(1) {
        // 1. DRAW the current state.
        erase();
        mvprintw(0, 0, "Stage 2 - use up/down, q to quit");

        for (int i = 0; i < count; i++) {
            if (i == highlight) {
                attron(A_REVERSE);
                mvprintw(i + 2, 2, "> %s", names[i]);
                attroff(A_REVERSE); // Always turn it off again.
            } else {
                mvprintw(i + 2, 2, "  %s", names[i]);
            }
        }
        refresh();

        // 2. WAIT for a key.
        int ch = getch();

        // 3. UPDATE the state.
        if (ch == 'q') {
            break;
        } else if (ch == KEY_UP) {
            if (highlight > 0) highlight--;
        } else if (ch == KEY_DOWN) {
            if (highlight < count - 1) highlight++;
        }
    }

    // Cleanup
    endwin();
    return 0;
}
