#include <ncurses.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>    // qsort
#include <string.h>
#include <strings.h>   // strcasecmp (case-insensitive compare)
#include <sys/stat.h>
#include <unistd.h>

#define MAX_FILES 1000
#define MAX_NAME_LEN 256
// The comparison function for qsort.
// qsort hands you two `const void *` pointers (it can't know the type).
// You cast them back to what they really are - here, pointers to our
// fixed-size strings, and returns:
// negative   -> a sorts BEFORE b
// zero       -> equal
// positive   -> a sorts AFTER b
// strcasecmp already follows that contract, ignoring uppler/lower case.
static int cmp_names(const void *a, const void *b) {
    return strcasecmp((const char *)a, (const char *)b);
}

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
// sort everything EXCEPT slot 0, so ".." stays on top.
// files + 1      -> address of row 1 (pointer arithmetic on rows)
// count - 1      -> how many rows to sort
// MAX_NAME_LEN   -> the size of each element (one 256-byte row)
    if (count > 1) {
        qsort(files + 1, (size_t)count - 1, MAX_NAME_LEN, cmp_names);
    }
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
        init_pair(1, COLOR_WHITE, COLOR_BLUE);
        init_pair(2, COLOR_CYAN, -1);
        init_pair(3, -1, -1);
    }

    char cwd[1024];
    char files[MAX_FILES][MAX_NAME_LEN];
    int num_files = 0;
    int highlight = 0;
    int top = 0;

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        endwin();
        fprintf(stderr, "getcwd failed\n");
        return 1;
    }
    num_files = read_directory(cwd, files);

    while(1) {
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);

        int visible = max_y -2;
        if (visible < 1) visible = 1;

        if (highlight < top) top = highlight;
        if (highlight >= top + visible) top = highlight - visible + 1;

        erase();

        attron(COLOR_PAIR(1) | A_BOLD);
        mvhline(0, 0, ' ', max_x);
        mvprintw(0, 0, " Stage 7 | %.*s", max_x > 12 ? max_x - 12 : 0, cwd);
        attroff(COLOR_PAIR(1) | A_BOLD);

        for (int row = 0; row < visible && top + row < num_files; row++) {
            int i = top + row;

            char full_path[1024 + MAX_NAME_LEN + 2];
            snprintf(full_path, sizeof full_path, "%s/%s", cwd, files[i]);

            struct stat st;
            int is_dir = 0;
            if (stat(full_path, &st) == 0) {
                is_dir = S_ISDIR(st.st_mode);
            }

            attr_t attr = COLOR_PAIR(is_dir ? 2 : 3);
            if (is_dir) attr |= A_BOLD;
            if (i == highlight) attr |= A_REVERSE;

            attron(attr);
            mvhline(row + 1, 0, ' ', max_x);
            mvprintw(row + 1, 1, "%c %s%s",
                     i == highlight ? '>' : ' ',
                     files[i],
                     (is_dir && strcmp(files[i], "..") != 0) ? "/" : "");
            attroff(attr);
        }

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
        } else if (ch == KEY_PPAGE) {
            highlight -= visible;
            if (highlight < 0) highlight = 0;
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
                    top = 0;
                }
            }
        }
    }

    endwin();
    return 0;
}
