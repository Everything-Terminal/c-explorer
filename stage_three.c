#include <ncurses.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_FILES 1000
#define MAX_NAME_LEN 256

int read_directory(const char *path, char files[][MAX_NAME_LEN]) {
    int count = 0;

    snprintf(files[count++], MAX_NAME_LEN, "..");

    DIR *dir = opendir(path);
    if (dir == NULL) {
        return count;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && count < MAX_FILES) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        snprintf(files[count], MAX_NAME_LEN, "%s", ent->d_name);
        count++;
    }

    closedir(dir);
    return count;
}

int main(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // State
    char cwd[1024];
    char files[MAX_FILES][MAX_NAME_LEN];
    int num_files = 0;
    int highlight = 0;

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        endwin();
        fprintf(stderr, "getcwd failed\n");
        return 1;
    }
    num_files = read_directory(cwd, files);

    // Event loop
    while (1) {
        erase();

        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        (void)max_x;

        mvprintw(0, 0, "Stage 3 | %s", cwd);

        for  (int i = 0; i < num_files && i < max_y - 1; i++) {
            if (i == highlight) {
                attron(A_REVERSE);
                mvprintw(i + 1, 0, " > %s", files[i]);
                attroff(A_REVERSE);
            } else {
                mvprintw(i + 1, 0, "   %s", files[i]);
            }
        }

        refresh();

        int ch = getch();
        if (ch == 'q') {
            break;
        } else if (ch == KEY_UP) {
            if (highlight > 0) highlight--;
        } else if (ch == KEY_DOWN) {
            if (highlight < num_files - 1) highlight++;
        }
    }

    endwin();
    return 0;
}
