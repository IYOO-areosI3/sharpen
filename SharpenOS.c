/*
 * SharpenOS 3.5  –  The Cool, Lightweight Terminal with Cat Style :3
 *                    Updated: zero compiler warnings (Wall Wextra)
 *                    Commands now organised with a cute struct!
 *
 * Compile: gcc -o SharpenOS SharpenOS.c -Wall -Wextra -std=c99
 * Run:     ./SharpenOS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <pwd.h>
#include <fcntl.h>

/* ---------- ANSI sequences ---------- */
#define BOLD   "\x1b[1m"
#define RESET  "\x1b[0m"
#define RED    "\x1b[31m"
#define GREEN  "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE   "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN   "\x1b[36m"

/* ---------- Fix the MAX_INPUT redefinition warning ---------- */
#undef  MAX_INPUT
#define MAX_INPUT 1024
#define MAX_ARGS  64

/* ---------- Cat‑style error macros ---------- */
static void cat_error(const char *msg) {
    fprintf(stderr, RED ":3 " BOLD "Nyā~ " RESET RED "%s" RESET "\n", msg);
}
static void cat_perror(const char *prefix) {
    fprintf(stderr, RED ":3 " BOLD "Nyā~ " RESET RED "%s: %s" RESET "\n",
            prefix, strerror(errno));
}

/* ---------- Human‑readable size helper ---------- */
static void format_size(unsigned long long bytes, char *buf, size_t bufsz) {
    const char *units[] = {"B","K","M","G","T","P"};
    int i = 0;
    double d = (double)bytes;
    while (d >= 1024.0 && i < 5) {
        d /= 1024.0;
        i++;
    }
    snprintf(buf, bufsz, "%.1f%s", d, units[i]);
}

/* ---------- Expand "~" and "$VAR" ---------- */
static char* expand_special(const char *token) {
    if (!token || *token=='\0') return strdup("");
    if (strcmp(token,"~")==0) {
        const char *home = getenv("HOME");
        return home ? strdup(home) : strdup("~");
    }
    if (token[0]=='~' && token[1]=='/') {
        const char *home = getenv("HOME");
        if (home) {
            size_t len = strlen(home)+strlen(token);
            char *res = malloc(len);
            snprintf(res, len, "%s%s", home, token+1);
            return res;
        }
    }
    if (token[0]=='$') {
        const char *val = getenv(token+1);
        return val ? strdup(val) : strdup("");
    }
    return strdup(token);
}

static void expand_args(char **raw, char **out, int max) {
    int i;
    for (i=0; raw[i] && i<max-1; i++)
        out[i] = expand_special(raw[i]);
    out[max-1] = NULL;
}

static void free_args(char **args) {
    for (int i=0; args[i]; i++) free(args[i]);
}

/* ---------- Stylish two‑line prompt ---------- */
static void print_prompt(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        strcpy(cwd, "?");

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    /* Fix snprintf truncation warning – bigger buffer */
    char date[32];
    snprintf(date, sizeof(date), "%02d:%02d:%04d",
             tm->tm_mon+1, tm->tm_mday, tm->tm_year+1900);

    printf(BOLD CYAN "╭── :3 " RESET BOLD GREEN "SharpenOS" RESET
           BOLD CYAN " · " RESET YELLOW "%s" RESET
           BOLD CYAN " · " RESET GREEN "%s" RESET
           BOLD CYAN " ───╮\n" RESET, date, cwd);
    printf(BOLD CYAN "╰" RESET BOLD GREEN "─> " RESET);
    fflush(stdout);
}

/* ---------- Forward declarations for built‑ins ---------- */
static void builtin_ed(char **args);
static void builtin_lp(char **args);
static void builtin_file(char **args);
static void builtin_directory(char **args);
static void builtin_ram(char **args);
static void builtin_clear(char **args);
static void builtin_info(char **args);
static void builtin_help(char **args);
static void builtin_exit(char **args);

/* ========== Command struct ========== */
typedef struct {
    const char *name;          /* command name */
    const char *short_help;    /* one‑line for 'help' overview */
    const char *long_help;     /* detailed usage for 'help <cmd>' */
    void (*func)(char **args); /* command handler */
} SharpenCommand;

/* ---------- Command implementations ---------- */

static void builtin_ed(char **args) {
    if (args[1] == NULL) {
        cat_error("ed: missing argument PATH:<directory>");
        return;
    }
    if (strncmp(args[1], "PATH:", 5) != 0) {
        cat_error("ed: argument must be PATH:<directory>");
        return;
    }
    const char *raw_path = args[1] + 5;
    if (*raw_path == '\0') return;

    char resolved[PATH_MAX];
    if (strcmp(raw_path, "~") == 0) {
        const char *home = getenv("HOME");
        if (!home) { cat_error("ed: HOME not set"); return; }
        strncpy(resolved, home, PATH_MAX);
        resolved[PATH_MAX-1] = '\0';
    } else if (raw_path[0] == '~' && raw_path[1] == '/') {
        const char *home = getenv("HOME");
        if (!home) { cat_error("ed: HOME not set"); return; }
        snprintf(resolved, PATH_MAX, "%s%s", home, raw_path+1);
    } else if (strcmp(raw_path, "^^") == 0) {
        strncpy(resolved, "..", PATH_MAX);
        resolved[PATH_MAX-1] = '\0';
    } else {
        strncpy(resolved, raw_path, PATH_MAX);
        resolved[PATH_MAX-1] = '\0';
    }

    if (chdir(resolved) != 0)
        cat_perror("ed");
}

/* ---------- Colorised file listing ---------- */
static int is_executable(const struct stat *st) {
    return st->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH);
}
static void list_file(const char *name, const struct stat *st) {
    if (S_ISDIR(st->st_mode))
        printf(BOLD BLUE "%s" RESET "\n", name);
    else if (S_ISLNK(st->st_mode))
        printf(CYAN "%s" RESET "\n", name);
    else if (is_executable(st))
        printf(GREEN "%s" RESET "\n", name);
    else if (S_ISSOCK(st->st_mode) || S_ISFIFO(st->st_mode))
        printf(MAGENTA "%s" RESET "\n", name);
    else
        printf("%s\n", name);
}

static void builtin_lp(char **args) {
    int show_files = 1, show_dirs = 1;
    const char *ext = NULL;

    for (int i=1; args[i]; i++) {
        if (strcmp(args[i],"-nofile")==0)
            show_files = 0;
        else if (strcmp(args[i],"-nofolder")==0)
            show_dirs = 0;
        else if (strcmp(args[i],"-ext")==0) {
            if (args[i+1]) ext = args[++i];
            else {
                cat_error("lp: -ext requires an extension (e.g. -ext .lua)");
                return;
            }
        } else {
            fprintf(stderr, RED "lp: unknown option %s" RESET "\n", args[i]);
            return;
        }
    }

    DIR *dir = opendir(".");
    if (!dir) { cat_perror("lp"); return; }

    struct dirent *d;
    struct stat st;
    while ((d = readdir(dir))) {
        if (!strcmp(d->d_name,".") || !strcmp(d->d_name,"..")) continue;
        if (stat(d->d_name, &st) == -1) continue;

        if (ext) {
            if (S_ISREG(st.st_mode)) {
                size_t nl = strlen(d->d_name), el = strlen(ext);
                if (nl>=el && strcmp(d->d_name+nl-el, ext)==0)
                    list_file(d->d_name, &st);
            }
        } else {
            if ((S_ISDIR(st.st_mode) && show_dirs) ||
                (S_ISREG(st.st_mode) && show_files))
                list_file(d->d_name, &st);
        }
    }
    closedir(dir);
}

static void builtin_file(char **args) {
    if (args[1] == NULL) {
        cat_error("file: Nyā~ give me a filename!  e.g. file meow.txt");
        return;
    }
    int fd = open(args[1], O_CREAT|O_WRONLY|O_TRUNC, 0644);
    if (fd < 0) {
        cat_perror("file");
        return;
    }
    close(fd);
    printf(BOLD GREEN "Nyā~ created file: %s" RESET "\n", args[1]);
}

static void builtin_directory(char **args) {
    if (args[1] == NULL) {
        cat_error("directory: Nyā~ I need a name!  e.g. directory purr");
        return;
    }
    if (mkdir(args[1], 0755) != 0) {
        cat_perror("directory");
    } else {
        printf(BOLD GREEN "Nyā~ created directory: %s" RESET "\n", args[1]);
    }
}

static void builtin_ram(char **args) {
    const char *path = (args[1] != NULL) ? args[1] : ".";

    struct statvfs vfs;
    if (statvfs(path, &vfs) != 0) {
        cat_perror("ram");
        return;
    }

    unsigned long long total = (unsigned long long)vfs.f_frsize * vfs.f_blocks;
    unsigned long long avail = (unsigned long long)vfs.f_frsize * vfs.f_bavail;
    unsigned long long used = total - avail;

    char total_str[32], used_str[32], avail_str[32];
    format_size(total, total_str, sizeof(total_str));
    format_size(used,  used_str,  sizeof(used_str));
    format_size(avail, avail_str, sizeof(avail_str));

    double percent_used = (total > 0) ? (double)used / total * 100.0 : 0.0;

    int bar_len = 20;
    int filled = (int)(percent_used / 100.0 * bar_len + 0.5);
    if (filled > bar_len) filled = bar_len;

    printf(BOLD CYAN "╭── 📦 Storage on %s ──╮\n" RESET, path);
    printf("  ");
    for (int i = 0; i < bar_len; i++) {
        if (i < filled)
            printf(RED "█");
        else
            printf(GREEN "█");
    }
    printf(RESET "  %5.1f%% used\n", percent_used);
    printf("  " BOLD "Free:" RESET " %s  |  " BOLD "Used:" RESET " %s  |  " BOLD "Total:" RESET " %s\n",
           avail_str, used_str, total_str);
    printf(BOLD CYAN "╰──────────────────────────╯\n" RESET);
}

static void builtin_info(char **args) {
    (void)args; /* unused */
    struct utsname uts;
    uname(&uts);
    char host[256];
    gethostname(host, sizeof(host));
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    printf(BOLD CYAN "╭────────── SharpenOS System Info ──────────╮\n" RESET);
    printf("  " BOLD "User:" RESET " %s\n", pw ? pw->pw_name : "?");
    printf("  " BOLD "Host:" RESET " %s\n", host);
    printf("  " BOLD "OS:" RESET " %s %s (%s)\n",
           uts.sysname, uts.release, uts.machine);
    time_t now = time(NULL);
    printf("  " BOLD "Date:" RESET " %s", ctime(&now));
    printf(BOLD CYAN "╰───────────────────────────────────────────╯\n" RESET);
}

static void builtin_clear(char **args) {
    (void)args;
    printf("\x1b[2J\x1b[H");
}

static void builtin_exit(char **args) {
    (void)args;
    /* handled in main – just a placeholder */
}

/* ---------- Help system ---------- */
/* We forward‑declare the commands array so builtin_help can use it */
static const SharpenCommand commands[];
static const size_t cmd_count;

static void builtin_help(char **args) {
    if (args[1] == NULL) {
        printf(BOLD CYAN "╭──────── SharpenOS Commands ─────────╮\n" RESET);
        for (size_t i = 0; i < cmd_count; i++) {
            printf("  " GREEN "%-15s" RESET " → %s\n",
                   commands[i].name, commands[i].short_help);
        }
        printf(BOLD CYAN "╰────────────────────────────────────╯\n" RESET);
        printf("Any other command runs as a normal program.\n");
        return;
    }

    /* Detailed help for a specific command */
    for (size_t i = 0; i < cmd_count; i++) {
        if (strcmp(args[1], commands[i].name) == 0) {
            printf(BOLD GREEN "%s" RESET " – %s\n", commands[i].name, commands[i].short_help);
            printf("%s\n", commands[i].long_help);
            return;
        }
    }
    /* Also check for cls as an alias for clear */
    if (strcmp(args[1], "cls") == 0) {
        printf(BOLD GREEN "clear / cls" RESET " – Clear the screen\n");
        printf("Usage: " BOLD "clear" RESET " or " BOLD "cls" RESET "\n");
        return;
    }
    cat_error("No detailed help for that command. Try 'help' alone.");
}

/* ========== Table of commands ========== */
static const SharpenCommand commands[] = {
    {"ed",
     "change directory (use ~, ^^)",
     "Usage: " BOLD "ed PATH:<directory>" RESET "\n"
     "Special Path Symbols:\n"
     "  " BOLD "~" RESET "        your Linux home directory\n"
     "  " BOLD "~/" RESET "path   start from home (e.g. ~/Documents)\n"
     "  " BOLD "^^" RESET "       go up one level (like ..)\n"
     "Examples:\n"
     "  " BOLD "ed PATH:~" RESET "\n"
     "  " BOLD "ed PATH:~/Downloads" RESET "\n"
     "  " BOLD "ed PATH:^^" RESET "\n",
     builtin_ed},
    {"lp",
     "list files & folders (no ls!)",
     "Usage: " BOLD "lp [options]" RESET "\n"
     "Options:\n"
     "  " BOLD "-nofile" RESET "    show only folders\n"
     "  " BOLD "-nofolder" RESET "  show only files\n"
     "  " BOLD "-ext .ext" RESET "   filter by extension (e.g. " BOLD "-ext .lua" RESET ")\n",
     builtin_lp},
    {"file",
     "create empty file (no touch!)",
     "Usage: " BOLD "file <filename>" RESET "\n"
     "Example: " BOLD "file meow.txt" RESET "\n",
     builtin_file},
    {"directory",
     "create folder (no mkdir!)",
     "Usage: " BOLD "directory <dirname>" RESET "\n"
     "Example: " BOLD "directory purr" RESET "\n",
     builtin_directory},
    {"ram",
     "show storage usage with bar :3",
     "Usage: " BOLD "ram [path]" RESET "\n"
     "Example: " BOLD "ram /sdcard" RESET "\n",
     builtin_ram},
    {"clear",
     "clear screen (also cls)",
     "Usage: " BOLD "clear" RESET " or " BOLD "cls" RESET "\n",
     builtin_clear},
    {"info",
     "system information",
     "Usage: " BOLD "info" RESET "\n",
     builtin_info},
    {"help",
     "this meow~sage (use 'help <cmd>' for details)",
     "Usage: " BOLD "help" RESET " or " BOLD "help <command>" RESET "\n",
     builtin_help},
    {"exit",
     "leave the terminal",
     "Usage: " BOLD "exit" RESET "\n",
     builtin_exit}
};
static const size_t cmd_count = sizeof(commands)/sizeof(commands[0]);

/* ---------- External command runner ---------- */
static void execute_external(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        execvp(args[0], args);
        cat_perror("exec");
        _exit(EXIT_FAILURE);
    } else if (pid < 0) {
        cat_perror("fork");
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

/* ---------- Splash with a cat face ---------- */
static void show_splash(void) {
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

/* ---------- Main loop ---------- */
int main(void) {
    char raw_input[MAX_INPUT];
    char *raw_args[MAX_ARGS];
    char *exp_args[MAX_ARGS];
    signal(SIGINT, SIG_IGN);

    show_splash();

    while (1) {
        memset(exp_args, 0, sizeof(exp_args));

        print_prompt();
        if (!fgets(raw_input, MAX_INPUT, stdin)) {
            printf("\n");
            break;
        }
        raw_input[strcspn(raw_input, "\n")] = '\0';

        int n = 0;
        char *tok = strtok(raw_input, " \t\n");
        while (tok && n < MAX_ARGS-1) {
            raw_args[n++] = tok;
            tok = strtok(NULL, " \t\n");
        }
        raw_args[n] = NULL;
        if (n == 0) continue;

        expand_args(raw_args, exp_args, MAX_ARGS);

        /* Block legacy commands */
        if (strcmp(exp_args[0], "ls") == 0 ||
            strcmp(exp_args[0], "cd") == 0 ||
            strcmp(exp_args[0], "touch") == 0 ||
            strcmp(exp_args[0], "mkdir") == 0) {
            cat_error("Oops! Those commands aren't allowed here!\n"
                      "  × ls   → use lp\n"
                      "  × cd   → use ed PATH:<dir>\n"
                      "  × touch → use file <name>\n"
                      "  × mkdir → use directory <name>\n"
                      "  >^._.^<");
            free_args(exp_args);
            continue;
        }

        /* Dispatch using the struct array */
        int found = 0;
        for (size_t i = 0; i < cmd_count; i++) {
            if (strcmp(exp_args[0], commands[i].name) == 0 ||
                (strcmp(exp_args[0], "cls") == 0 && strcmp(commands[i].name, "clear") == 0)) {
                /* exit is handled specially to break the loop */
                if (strcmp(commands[i].name, "exit") == 0) {
                    free_args(exp_args);
                    goto quit;
                }
                commands[i].func(exp_args);
                found = 1;
                break;
            }
        }
        if (!found)
            execute_external(exp_args);

        free_args(exp_args);
    }

quit:
    printf(BOLD GREEN "\nGoodbye from SharpenOS! Stay purr-fect! :3\n" RESET);
    return 0;
}