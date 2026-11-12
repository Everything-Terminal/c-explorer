#include <ncurses.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>   // NEW: stat, S_ISDIR
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

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        endwin();
        fprintf(stderr, "getcwd failed\n");
        return 1;
    }
    num_files = read_directory(cwd, files);

    while(1) {
        erase();

        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);

        attron(COLOR_PAIR(1) | A_BOLD);
        mvhline(0, 0, ' ', max_x);
        mvprintw(0, 0, " Stage 5 | %.*s", max_x > 12 ? max_x - 12 : 0, cwd);
        attroff(COLOR_PAIR(1) | A_BOLD);

        for (int i = 0; i < num_files && i < max_y - 1; i++) {
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
            mvhline(i + 1, 0, ' ', max_x);  // paint the full row (highlight spans the width)
            mvprintw(i + 1, 1, "%c %s%s",
                     i == highlight ? '>' : ' ',
                     files[i],
                     (is_dir && strcmp(files[i], "..") != 0) ? "/" : "");
            attroff(attr);
        }
        refresh();

        int ch = getch();
        if (ch == 'q') {
            break;
        } else if (ch == KEY_UP) {
            if (highlight > 0) highlight--;
        } else if (ch == KEY_DOWN) {
            if (highlight < num_files -1) highlight++;
        } else if (ch == '\n' || ch == KEY_ENTER) {
            char selected[MAX_NAME_LEN];
            snprintf(selected, sizeof selected, "%s", files[highlight]);

            if (chdir(selected) == 0) {
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    num_files = read_directory(cwd, files);
                    highlight = 0;
                }
            }
        }
    }

    endwin();
    return 0;
}
