#ifndef SHARPEN_H
#define SHARPEN_H

/* Enable POSIX functions (gethostname, usleep, strncasecmp, etc.) */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>        /* for strncasecmp */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <sys/types.h>       /* for uid_t */
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <limits.h>          /* for PATH_MAX */
#include <pwd.h>
#include <fcntl.h>

/* ---------- ANSI colour codes ---------- */
#define BOLD    "\x1b[1m"
#define RESET   "\x1b[0m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"

/* ---------- Fix system macro ---------- */
#undef  MAX_INPUT
#define MAX_INPUT 1024
#define MAX_ARGS  64

/* ---------- Theme struct ---------- */
typedef struct {
    const char *name;
    const char *prompt_color;
    const char *accent;
} SharpenTheme;

extern const SharpenTheme *current_theme;
extern const SharpenTheme *themes[];
extern const size_t theme_count;

void theme_set(const char *name);

/* ---------- Command struct ---------- */
typedef struct {
    const char *name;
    const char *short_help;
    const char *long_help;
    void (*func)(char **args);
} SharpenCommand;

extern const SharpenCommand commands[];
extern const size_t cmd_count;

/* Cat error helpers */
void cat_error(const char *msg);
void cat_perror(const char *prefix);

/* Utility prototypes */
void format_size(unsigned long long bytes, char *buf, size_t bufsz);
char *expand_special(const char *token);
void expand_args(char **raw, char **out, int max);
void free_args(char **args);
int  is_executable_file(const struct stat *st);
void list_file_colored(const char *name, const struct stat *st);
void print_prompt(void);
void show_splash(void);

#endif