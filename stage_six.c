#include <ncurses.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
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

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_WHITE, COLOR_BLUE);  // 1: title bar
        init_pair(2, COLOR_CYAN, -1);           // 2: directories
        init_pair(3, -1, -1);                   // 3: files
    }

    char cwd[1024];
    char files[MAX_FILES][MAX_NAME_LEN];
    int num_files = 0;
    int highlight = 0;
    int top = 0;  // index of the first entry shown on screen

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        endwin();
        fprintf(stderr, "getcwd failed\n");
        return 1;
    }
    num_files = read_directory(cwd, files);

    while(1) {
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);

        // New: the list area is the screen minus the title bar (top)
        // and the status bar (bottom).
        int visible = max_y -2;
        if (visible < 1) visible = 1;

        // SCROLLING. Think of the screen as a window sliding over
        // a tall list. We only move the window when the highlight
        // tries to leave it:
        // highlight above the window -> slide up so it is the first row
        // highlight below the window -> slide down so it is the last row
        if (highlight < top) top = highlight;
        if (highlight >= top + visible) top = highlight - visible + 1;

        erase();

        attron(COLOR_PAIR(1) | A_BOLD);
        mvhline(0, 0, ' ', max_x);
        mvprintw(0, 0, " Stage 6 | %.*s", max_x > 12 ? max_x - 12 : 0, cwd);
        attroff(COLOR_PAIR(1) | A_BOLD);

        // The loop now draws screen ROWS (0..visible-1) and maps
        // each to a list INDEX (top + row). Same list, shifted view.
        for (int row = 0; row < visible && top + row < num_files; row++) {
            int i = top + row;

            char full_path[1024 + MAX_NAME_LEN + 2];
            snprintf(full_path, sizeof full_path, "%s/%s", cwd, files[i]);

            struct stat st;
            int is_dir = 0;
            if (stat(full_path, &st) == 0) {    // 0 = success
                is_dir = S_ISDIR(st.st_mode);   // test the "type" bits
            }

            attr_t attr = COLOR_PAIR(is_dir ? 2 : 3);
            if (is_dir) attr |= A_BOLD;
            if (i == highlight) attr |= A_REVERSE;

            attron(attr);
            mvhline(row + 1, 0, ' ', max_x);  // paint the full row (highlight spans the width)
            mvprintw(row + 1, 1, "%c %s%s",
                     i == highlight ? '>' : ' ',
                     files[i],
                     (is_dir && strcmp(files[i], "..") != 0) ? "/" : "");
            attroff(attr);
        }

        // Status bar on the LAST row of the screen
        attron(COLOR_PAIR(1));
        mvhline(max_y - 1, 0, ' ', max_x);
        mvprintw(max_y - 1, 1, "%d/%d  | arrows move PgUp/PgDn jump  Home/End Enter open  q to quit",
                 num_files ? highlight + 1 : 0, num_files);
        attroff(COLOR_PAIR(1));

        refresh();

        int ch = getch();
        if (ch == 'q') {
            break;
        } else if (ch == KEY_UP) {
            if (highlight > 0) highlight--;
        } else if (ch == KEY_DOWN) {
            if (highlight < num_files -1) highlight++;
        } else if (ch == KEY_PPAGE) {         // Page Up
            highlight -= visible;
            if (highlight < 0) highlight = 0; // clamp to the first row
        } else if (ch == KEY_NPAGE) {
            highlight += visible;
            if (highlight > num_files - 1) highlight = num_files - 1;
        } else if (ch == KEY_HOME) {
            highlight = 0;
        } else if (ch == KEY_END) {
            highlight = num_files - 1;
        } else if (ch == '\n' || ch == KEY_ENTER) {
            char selected[MAX_NAME_LEN];
            snprintf(selected, sizeof selected, "%s", files[highlight]);

            if (chdir(selected) == 0) {
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    num_files = read_directory(cwd, files);
                    highlight = 0;
                    top = 0; // Also reset the scroll position
                }
            }
        }
    }

    endwin();
    return 0;
}
