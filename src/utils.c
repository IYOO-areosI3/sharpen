#include "sharpen.h"

/* ---------- Theme definitions (global) ---------- */
static const SharpenTheme theme_default = { "default", CYAN, GREEN };
static const SharpenTheme theme_hacker  = { "hacker",  GREEN, GREEN };
static const SharpenTheme theme_pastel  = { "pastel",  MAGENTA, YELLOW };
static const SharpenTheme theme_dark    = { "dark",    BOLD BLUE, RESET };

const SharpenTheme *themes[] = { &theme_default, &theme_hacker, &theme_pastel, &theme_dark };
const size_t theme_count = sizeof(themes) / sizeof(themes[0]);
const SharpenTheme *current_theme = &theme_default;

void theme_set(const char *name) {
    for (size_t i = 0; i < theme_count; i++) {
        if (strcmp(name, themes[i]->name) == 0) {
            current_theme = themes[i];
            return;
        }
    }
}

/* ---------- Cat error helpers ---------- */
void cat_error(const char *msg) {
    fprintf(stderr, RED ":3 " BOLD "Nyā~ " RESET RED "%s" RESET "\n", msg);
}
void cat_perror(const char *prefix) {
    fprintf(stderr, RED ":3 " BOLD "Nyā~ " RESET RED "%s: %s" RESET "\n",
            prefix, strerror(errno));
}

/* ---------- Human-readable size ---------- */
void format_size(unsigned long long bytes, char *buf, size_t bufsz) {
    const char *units[] = {"B","K","M","G","T","P"};
    int i = 0;
    double d = (double)bytes;
    while (d >= 1024.0 && i < 5) {
        d /= 1024.0;
        i++;
    }
    snprintf(buf, bufsz, "%.1f%s", d, units[i]);
}

/* ---------- Expand ~ / $VAR ---------- */
char *expand_special(const char *token) {
    if (!token || *token == '\0') return strdup("");
    if (strcmp(token, "~") == 0) {
        const char *home = getenv("HOME");
        return home ? strdup(home) : strdup("~");
    }
    if (token[0] == '~' && token[1] == '/') {
        const char *home = getenv("HOME");
        if (home) {
            size_t len = strlen(home) + strlen(token);
            char *res = malloc(len);
            snprintf(res, len, "%s%s", home, token + 1);
            return res;
        }
    }
    if (token[0] == '$') {
        const char *val = getenv(token + 1);
        return val ? strdup(val) : strdup("");
    }
    return strdup(token);
}

void expand_args(char **raw, char **out, int max) {
    int i;
    for (i = 0; raw[i] && i < max - 1; i++)
        out[i] = expand_special(raw[i]);
    out[max - 1] = NULL;
}

void free_args(char **args) {
    for (int i = 0; args[i]; i++) free(args[i]);
}

/* ---------- File helpers ---------- */
int is_executable_file(const struct stat *st) {
    return st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH);
}

void list_file_colored(const char *name, const struct stat *st) {
    if (S_ISDIR(st->st_mode))
        printf(BOLD BLUE "%s" RESET "\n", name);
    else if (S_ISLNK(st->st_mode))
        printf(CYAN "%s" RESET "\n", name);
    else if (is_executable_file(st))
        printf(GREEN "%s" RESET "\n", name);
    else if (S_ISSOCK(st->st_mode) || S_ISFIFO(st->st_mode))
        printf(MAGENTA "%s" RESET "\n", name);
    else
        printf("%s\n", name);
}

/* ---------- Splash ---------- */
void show_splash(void) {
    printf(BOLD GREEN
        "    ███████╗██╗  ██╗ █████╗ ██████╗ ██████╗ ███████╗███╗   ██╗ ██████╗ ███████╗\n"
        "    ██╔════╝██║  ██║██╔══██╗██╔══██╗██╔══██╗██╔════╝████╗  ██║██╔═══██╗██╔════╝\n"
        "    ███████╗███████║███████║██████╔╝██████╔╝█████╗  ██╔██╗ ██║██║   ██║███████╗\n"
        "    ╚════██║██╔══██║██╔══██║██╔══██╗██╔══██╗██╔══╝  ██║╚██╗██║██║   ██║╚════██║\n"
        "    ███████║██║  ██║██║  ██║██║  ██║██████╔╝███████╗██║ ╚████║╚██████╔╝███████║\n"
        "    ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝\n"
    RESET);
    printf(BOLD CYAN "               Cool, Lightweight Terminal  :3\n\n" RESET);
}

/* ---------- Prompt ---------- */
void print_prompt(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        strcpy(cwd, "?");
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char date[32];  /* avoid snprintf truncation warning */
    snprintf(date, sizeof(date), "%02d:%02d:%04d",
             tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900);

    const char *main_col = current_theme->prompt_color;
    const char *acc_col = current_theme->accent;

    printf(BOLD "%s╭── :3 " RESET BOLD GREEN "SharpenOS" RESET
           BOLD "%s" " · " RESET YELLOW "%s" RESET
           BOLD "%s" " · " RESET "%s" "%s" RESET
           BOLD "%s" " ───╮\n" RESET,
           main_col, main_col, date, main_col, acc_col, cwd, main_col);
    printf(BOLD "%s╰" RESET BOLD GREEN "─> " RESET, main_col);
    fflush(stdout);
}