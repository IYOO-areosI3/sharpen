/*
 * SharpenOS 3.4  –  The Cool, Lightweight Terminal with Cat Style :3
 *                    UPDATED: help <command> gives detailed usage
 *                    NEW: ed PATH now supports ~ and ^^ symbols
 *
 * Compile: gcc -o SharpenOS SharpenOS.c
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

#define MAX_INPUT 1024
#define MAX_ARGS  64

/* ---------- ANSI sequences ---------- */
#define BOLD   "\x1b[1m"
#define RESET  "\x1b[0m"
#define RED    "\x1b[31m"
#define GREEN  "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE   "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN   "\x1b[36m"

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

/* ---------- Stylish two‑line prompt (with a cat) ---------- */
static void print_prompt(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        strcpy(cwd, "?");

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char date[20];
    snprintf(date, sizeof(date), "%02d:%02d:%04d",
             tm->tm_mon+1, tm->tm_mday, tm->tm_year+1900);

    printf(BOLD CYAN "╭── :3 " RESET BOLD GREEN "SharpenOS" RESET
           BOLD CYAN " · " RESET YELLOW "%s" RESET
           BOLD CYAN " · " RESET GREEN "%s" RESET
           BOLD CYAN " ───╮\n" RESET, date, cwd);
    printf(BOLD CYAN "╰" RESET BOLD GREEN "─> " RESET);
    fflush(stdout);
}

/* ---------- Built‑in: ed PATH:<dir> with ~ and ^^ ---------- */
static void execute_ed(char **args) {
    if (args[1] == NULL) {
        cat_error("ed: missing argument PATH:<directory>");
        return;
    }
    if (strncmp(args[1], "PATH:", 5) != 0) {
        cat_error("ed: argument must be PATH:<directory>");
        return;
    }
    const char *raw_path = args[1] + 5;
    if (*raw_path == '\0') return;   // PATH: alone = stay

    char resolved[PATH_MAX];
    /* Handle special symbols */
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

    if (chdir(resolved) != 0) {
        cat_perror("ed");
    }
}

/* ---------- Colorised file listing (executable, dir, link…) ---------- */
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

/* ---------- Built‑in: lp [options] ---------- */
static void execute_lp(char **args) {
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

/* ---------- Built‑in: file <filename> (touch) ---------- */
static void execute_file(char **args) {
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

/* ---------- Built‑in: directory <dirname> (mkdir) ---------- */
static void execute_directory(char **args) {
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

/* ---------- Built‑in: ram (storage with bar & percentage) ---------- */
static void execute_ram(char **args) {
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

/* ---------- Built‑in: info ---------- */
static void execute_info(void) {
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

/* ---------- Detailed help for a specific command ---------- */
static void print_command_help(const char *cmd) {
    if (strcmp(cmd, "ed") == 0) {
        printf(BOLD GREEN "ed" RESET " – Enter Directory\n");
        printf("Usage: " BOLD "ed PATH:<directory>" RESET "\n");
        printf("Special Path Symbols:\n");
        printf("  " BOLD "~" RESET "        your Linux home directory\n");
        printf("  " BOLD "~/" RESET "path   start from home (e.g. ~/Documents)\n");
        printf("  " BOLD "^^" RESET "       go up one level (like ..)\n");
        printf("Examples:\n");
        printf("  " BOLD "ed PATH:~" RESET "\n");
        printf("  " BOLD "ed PATH:~/Downloads" RESET "\n");
        printf("  " BOLD "ed PATH:^^" RESET "\n");
    } else if (strcmp(cmd, "lp") == 0) {
        printf(BOLD GREEN "lp" RESET " – List Path\n");
        printf("Usage: " BOLD "lp [options]" RESET "\n");
        printf("Options:\n");
        printf("  " BOLD "-nofile" RESET "    show only folders\n");
        printf("  " BOLD "-nofolder" RESET "  show only files\n");
        printf("  " BOLD "-ext .ext" RESET "   filter by extension (e.g. " BOLD "-ext .lua" RESET ")\n");
    } else if (strcmp(cmd, "file") == 0) {
        printf(BOLD GREEN "file" RESET " – Create a new file\n");
        printf("Usage: " BOLD "file <filename>" RESET "\n");
        printf("Example: " BOLD "file meow.txt" RESET "\n");
    } else if (strcmp(cmd, "directory") == 0) {
        printf(BOLD GREEN "directory" RESET " – Create a new folder\n");
        printf("Usage: " BOLD "directory <dirname>" RESET "\n");
        printf("Example: " BOLD "directory purr" RESET "\n");
    } else if (strcmp(cmd, "ram") == 0) {
        printf(BOLD GREEN "ram" RESET " – Show storage usage\n");
        printf("Usage: " BOLD "ram [path]" RESET "\n");
        printf("If no path given, uses the current directory.\n");
        printf("Example: " BOLD "ram /sdcard" RESET "\n");
    } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
        printf(BOLD GREEN "clear / cls" RESET " – Clear the screen\n");
        printf("Usage: " BOLD "clear" RESET " or " BOLD "cls" RESET "\n");
    } else if (strcmp(cmd, "info") == 0) {
        printf(BOLD GREEN "info" RESET " – System information\n");
        printf("Usage: " BOLD "info" RESET "\n");
    } else if (strcmp(cmd, "help") == 0) {
        printf(BOLD GREEN "help" RESET " – Show help\n");
        printf("Usage: " BOLD "help" RESET " or " BOLD "help <command>" RESET "\n");
        printf("Type just 'help' for a list of all commands.\n");
    } else if (strcmp(cmd, "exit") == 0) {
        printf(BOLD GREEN "exit" RESET " – Leave SharpenOS\n");
        printf("Usage: " BOLD "exit" RESET "\n");
    } else {
        cat_error("No detailed help for that command. Try 'help' alone.");
    }
}

/* ---------- Built‑in: help [command] ---------- */
static void execute_help(char **args) {
    if (args[1] == NULL) {
        // General help list
        printf(BOLD CYAN "╭──────── SharpenOS Commands ─────────╮\n" RESET);
        printf(GREEN "  ed PATH:<dir>" RESET "   → change directory (use ~, ^^)\n");
        printf(GREEN "  lp [opts]" RESET "       → list files & folders (no ls!)\n");
        printf("      -nofile    show only folders\n");
        printf("      -nofolder  show only files\n");
        printf("      -ext .ext  filter by extension\n");
        printf(GREEN "  file <name>" RESET "      → create empty file (no touch!)\n");
        printf(GREEN "  directory <name>" RESET "→ create folder (no mkdir!)\n");
        printf(GREEN "  ram [path]" RESET "       → show storage usage with bar :3\n");
        printf(GREEN "  clear / cls" RESET "     → clear screen\n");
        printf(GREEN "  info" RESET "            → system information\n");
        printf(GREEN "  help [cmd]" RESET "      → this meow~sage (use 'help <cmd>' for details)\n");
        printf(GREEN "  exit" RESET "           → leave the terminal\n");
        printf(BOLD CYAN "╰────────────────────────────────────╯\n" RESET);
        printf("Any other command runs as a normal program.\n");
    } else {
        print_command_help(args[1]);
    }
}

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
        // Prevent double‑free by zeroing the expanded args array
        memset(exp_args, 0, sizeof(exp_args));

        print_prompt();
        if (!fgets(raw_input, MAX_INPUT, stdin)) {
            printf("\n");
            break;
        }
        raw_input[strcspn(raw_input, "\n")] = '\0';

        // Tokenise
        int n = 0;
        char *tok = strtok(raw_input, " \t\n");
        while (tok && n < MAX_ARGS-1) {
            raw_args[n++] = tok;
            tok = strtok(NULL, " \t\n");
        }
        raw_args[n] = NULL;
        if (n == 0) continue;

        // Expand ~ and $ variables
        expand_args(raw_args, exp_args, MAX_ARGS);

        /* ----- Block legacy commands ----- */
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

        /* ----- Dispatch built‑ins or external ----- */
        if (strcmp(exp_args[0], "exit") == 0) {
            free_args(exp_args);
            break;
        } else if (strcmp(exp_args[0], "ed") == 0) {
            execute_ed(exp_args);
        } else if (strcmp(exp_args[0], "lp") == 0) {
            execute_lp(exp_args);
        } else if (strcmp(exp_args[0], "file") == 0) {
            execute_file(exp_args);
        } else if (strcmp(exp_args[0], "directory") == 0) {
            execute_directory(exp_args);
        } else if (strcmp(exp_args[0], "ram") == 0) {
            execute_ram(exp_args);
        } else if (strcmp(exp_args[0], "clear") == 0 ||
                   strcmp(exp_args[0], "cls") == 0) {
            printf("\x1b[2J\x1b[H");
        } else if (strcmp(exp_args[0], "info") == 0) {
            execute_info();
        } else if (strcmp(exp_args[0], "help") == 0) {
            execute_help(exp_args);
        } else {
            execute_external(exp_args);
        }

        free_args(exp_args);
    }

    printf(BOLD GREEN "\nGoodbye from SharpenOS! Stay purr-fect! :3\n" RESET);
    return 0;
}