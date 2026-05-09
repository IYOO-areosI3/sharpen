/*
 * SharpenOS 3.7  –  The Cool, Lightweight Terminal with Cat Style :3
 *                    Zero warnings, theme manager, changelog,
 *                    SHpm with custom GitHub downloader (cute & descriptive!)
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

/* ---------- Fix MAX_INPUT redefinition ---------- */
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

/* ---------- Theme system ---------- */
typedef struct {
    const char *name;
    const char *prompt_color;
    const char *accent;
} SharpenTheme;

static const SharpenTheme themes[];
static const size_t theme_count;
static const SharpenTheme *current_theme;

static const SharpenTheme theme_default = { "default", CYAN, GREEN };
static const SharpenTheme theme_hacker  = { "hacker",  GREEN, GREEN };
static const SharpenTheme theme_pastel  = { "pastel",  MAGENTA, YELLOW };
static const SharpenTheme theme_dark    = { "dark",    BOLD BLUE, RESET };

static const SharpenTheme themes[] = {
    theme_default, theme_hacker, theme_pastel, theme_dark
};
static const size_t theme_count = sizeof(themes)/sizeof(themes[0]);
static const SharpenTheme *current_theme = &theme_default;

/* ---------- Human‑readable size ---------- */
static void format_size(unsigned long long bytes, char *buf, size_t bufsz) {
    const char *units[] = {"B","K","M","G","T","P"};
    int i = 0;
    double d = (double)bytes;
    while (d >= 1024.0 && i < 5) { d /= 1024.0; i++; }
    snprintf(buf, bufsz, "%.1f%s", d, units[i]);
}

/* ---------- Expand ~ / $VAR ---------- */
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
    for (i=0; raw[i] && i<max-1; i++) out[i] = expand_special(raw[i]);
    out[max-1] = NULL;
}

static void free_args(char **args) {
    for (int i=0; args[i]; i++) free(args[i]);
}

/* ---------- Pretty prompt ---------- */
static void print_prompt(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) strcpy(cwd, "?");
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char date[32];
    snprintf(date, sizeof(date), "%02d:%02d:%04d",
             tm->tm_mon+1, tm->tm_mday, tm->tm_year+1900);
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

/* ---------- Forward declarations ---------- */
static void builtin_ed(char **args);
static void builtin_lp(char **args);
static void builtin_file(char **args);
static void builtin_directory(char **args);
static void builtin_ram(char **args);
static void builtin_clear(char **args);
static void builtin_info(char **args);
static void builtin_help(char **args);
static void builtin_exit(char **args);
static void builtin_theme(char **args);
static void builtin_changelog(char **args);
static void builtin_shpm(char **args);

/* ---------- Command struct ---------- */
typedef struct {
    const char *name;
    const char *short_help;
    const char *long_help;
    void (*func)(char **args);
} SharpenCommand;

/* ---------- Built‑in implementations ---------- */

static void builtin_ed(char **args) {
    if (args[1] == NULL) { cat_error("ed: missing argument PATH:<directory>"); return; }
    if (strncmp(args[1], "PATH:", 5) != 0) {
        cat_error("ed: argument must be PATH:<directory>"); return;
    }
    const char *raw_path = args[1] + 5;
    if (*raw_path == '\0') return;
    char resolved[PATH_MAX];
    if (strcmp(raw_path, "~") == 0) {
        const char *home = getenv("HOME");
        if (!home) { cat_error("ed: HOME not set"); return; }
        strncpy(resolved, home, PATH_MAX); resolved[PATH_MAX-1] = '\0';
    } else if (raw_path[0] == '~' && raw_path[1] == '/') {
        const char *home = getenv("HOME");
        if (!home) { cat_error("ed: HOME not set"); return; }
        snprintf(resolved, PATH_MAX, "%s%s", home, raw_path+1);
    } else if (strcmp(raw_path, "^^") == 0) {
        strncpy(resolved, "..", PATH_MAX); resolved[PATH_MAX-1] = '\0';
    } else {
        strncpy(resolved, raw_path, PATH_MAX); resolved[PATH_MAX-1] = '\0';
    }
    if (chdir(resolved) != 0) cat_perror("ed");
}

static int is_executable(const struct stat *st) {
    return st->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH);
}
static void list_file(const char *name, const struct stat *st) {
    if (S_ISDIR(st->st_mode)) printf(BOLD BLUE "%s" RESET "\n", name);
    else if (S_ISLNK(st->st_mode)) printf(CYAN "%s" RESET "\n", name);
    else if (is_executable(st)) printf(GREEN "%s" RESET "\n", name);
    else if (S_ISSOCK(st->st_mode) || S_ISFIFO(st->st_mode)) printf(MAGENTA "%s" RESET "\n", name);
    else printf("%s\n", name);
}

static void builtin_lp(char **args) {
    int show_files = 1, show_dirs = 1;
    const char *ext = NULL;
    for (int i=1; args[i]; i++) {
        if (strcmp(args[i],"-nofile")==0) show_files = 0;
        else if (strcmp(args[i],"-nofolder")==0) show_dirs = 0;
        else if (strcmp(args[i],"-ext")==0) {
            if (args[i+1]) ext = args[++i];
            else { cat_error("lp: -ext requires an extension (e.g. -ext .lua)"); return; }
        } else {
            fprintf(stderr, RED "lp: unknown option %s" RESET "\n", args[i]); return;
        }
    }
    DIR *dir = opendir(".");
    if (!dir) { cat_perror("lp"); return; }
    struct dirent *d; struct stat st;
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
    if (args[1] == NULL) { cat_error("file: Nyā~ give me a filename!  e.g. file meow.txt"); return; }
    int fd = open(args[1], O_CREAT|O_WRONLY|O_TRUNC, 0644);
    if (fd < 0) { cat_perror("file"); return; }
    close(fd);
    printf(BOLD GREEN "Nyā~ created file: %s" RESET "\n", args[1]);
}

static void builtin_directory(char **args) {
    if (args[1] == NULL) { cat_error("directory: Nyā~ I need a name!  e.g. directory purr"); return; }
    if (mkdir(args[1], 0755) != 0) { cat_perror("directory"); }
    else printf(BOLD GREEN "Nyā~ created directory: %s" RESET "\n", args[1]);
}

static void builtin_ram(char **args) {
    const char *path = (args[1] != NULL) ? args[1] : ".";
    struct statvfs vfs;
    if (statvfs(path, &vfs) != 0) { cat_perror("ram"); return; }
    unsigned long long total = (unsigned long long)vfs.f_frsize * vfs.f_blocks;
    unsigned long long avail = (unsigned long long)vfs.f_frsize * vfs.f_bavail;
    unsigned long long used = total - avail;
    char total_str[32], used_str[32], avail_str[32];
    format_size(total, total_str, sizeof(total_str));
    format_size(used, used_str, sizeof(used_str));
    format_size(avail, avail_str, sizeof(avail_str));
    double percent_used = (total > 0) ? (double)used / total * 100.0 : 0.0;
    int bar_len = 20;
    int filled = (int)(percent_used / 100.0 * bar_len + 0.5);
    if (filled > bar_len) filled = bar_len;
    printf(BOLD CYAN "╭── 📦 Storage on %s ──╮\n" RESET, path);
    printf("  ");
    for (int i = 0; i < bar_len; i++) printf(i < filled ? RED "█" : GREEN "█");
    printf(RESET "  %5.1f%% used\n", percent_used);
    printf("  " BOLD "Free:" RESET " %s  |  " BOLD "Used:" RESET " %s  |  " BOLD "Total:" RESET " %s\n",
           avail_str, used_str, total_str);
    printf(BOLD CYAN "╰──────────────────────────╯\n" RESET);
}

static void builtin_info(char **args) {
    (void)args;
    struct utsname uts; uname(&uts);
    char host[256]; gethostname(host, sizeof(host));
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    printf(BOLD CYAN "╭────────── SharpenOS System Info ──────────╮\n" RESET);
    printf("  " BOLD "User:" RESET " %s\n", pw ? pw->pw_name : "?");
    printf("  " BOLD "Host:" RESET " %s\n", host);
    printf("  " BOLD "OS:" RESET " %s %s (%s)\n", uts.sysname, uts.release, uts.machine);
    time_t now = time(NULL);
    printf("  " BOLD "Date:" RESET " %s", ctime(&now));
    printf(BOLD CYAN "╰───────────────────────────────────────────╯\n" RESET);
}

static void builtin_clear(char **args) {
    (void)args;
    printf("\x1b[2J\x1b[H");
}

static void builtin_exit(char **args) { (void)args; /* handled in main */ }

static void builtin_theme(char **args) {
    if (args[1] == NULL) {
        printf(BOLD CYAN "Available themes:\n" RESET);
        for (size_t i = 0; i < theme_count; i++)
            printf("  " GREEN "%s" RESET "\n", themes[i].name);
        printf("Current theme: " BOLD GREEN "%s" RESET "\n", current_theme->name);
        return;
    }
    for (size_t i = 0; i < theme_count; i++) {
        if (strcmp(args[1], themes[i].name) == 0) {
            current_theme = &themes[i];
            printf(BOLD GREEN "Theme changed to %s :3\n" RESET, themes[i].name);
            return;
        }
    }
    cat_error("Unknown theme. Use 'theme' to list available themes.");
}

static const char changelog[] =
    BOLD GREEN "SharpenOS Changelog\n" RESET
    "-------------------------\n"
    "3.7 - SHpm with custom GitHub downloader, super cute :3\n"
    "3.6 - Theme manager, changelog, SHpm stub\n"
    "3.5 - Zero warnings, command struct, ^^ and ~ in ed\n"
    "3.4 - ed supports ~ and ^^\n"
    "3.3 - help <command> detailed\n"
    "3.2 - Error messages with :3 Nyā~\n"
    "3.1 - First public release\n";

static void builtin_changelog(char **args) {
    (void)args;
    printf("%s", changelog);
}

/* ========== SHpm – Sharp Package Manager ========== */
typedef struct {
    const char *name;
    const char *desc;
    const char *repo;        /* "owner/repo" */
    const char *version;
    const char *asset;       /* file to download from GitHub releases */
    unsigned long long size; /* approximate size in bytes */
    const char *post_install; /* optional path to executable after extraction */
} SHpmPackage;

static const SHpmPackage packages[] = {
    {"nyacat",
     "The cutest cat fetcher :3",
     "sharpen-os/nyacat",
     "1.0.0",
     "nyacat.linux.x86_64.tar.gz",
     1ULL * 1024 * 1024 * 1024, /* 1 GB */
     "nyacat/nyacat"}  /* after tar xf, binary inside nyacat/ */
};
static const size_t package_count = sizeof(packages)/sizeof(packages[0]);

/* Helper: check if a tool is executable */
static int tool_exists(const char *name) {
    char path[PATH_MAX];
    /* search in PATH? just check /usr/bin */
    snprintf(path, sizeof(path), "/usr/bin/%s", name);
    return access(path, X_OK) == 0;
}

/* Cute download spinner */
static void print_spin(const char *msg, pid_t child) {
    static const char spin[] = "|/-\\";
    int i = 0;
    printf(BOLD CYAN "%s... " RESET, msg);
    fflush(stdout);
    while (1) {
        int status;
        pid_t res = waitpid(child, &status, WNOHANG);
        if (res == child) break;  /* done */
        printf("\r" BOLD CYAN "%s... %c" RESET, msg, spin[i % 4]);
        fflush(stdout);
        i++;
        usleep(200000);  /* 0.2 sec */
    }
    printf("\r" BOLD CYAN "%-60s" RESET "\r", " "); /* clear line */
    fflush(stdout);
}

static void builtin_shpm(char **args) {
    if (args[1] == NULL) {
        printf(BOLD GREEN "SHpm" RESET " - Sharp Package Manager :3\n");
        printf("Usage: shpm install <package>\n");
        printf("       shpm list\n");
        return;
    }

    if (strcmp(args[1], "list") == 0) {
        printf(BOLD CYAN "Available packages:\n" RESET);
        for (size_t i = 0; i < package_count; i++) {
            char size[32];
            format_size(packages[i].size, size, sizeof(size));
            printf("  " GREEN "%-15s" RESET " - %s (" YELLOW "%s" RESET ")\n",
                   packages[i].name, packages[i].desc, size);
        }
        return;
    }

    if (strcmp(args[1], "install") != 0) {
        cat_error("shpm: unknown subcommand. Use 'install' or 'list'.");
        return;
    }
    if (args[2] == NULL) { cat_error("shpm install: missing package name."); return; }

    /* Find package */
    const SHpmPackage *pkg = NULL;
    for (size_t i = 0; i < package_count; i++) {
        if (strcmp(args[2], packages[i].name) == 0) {
            pkg = &packages[i];
            break;
        }
    }
    if (!pkg) { cat_error("shpm install: package not found in our tiny repo."); return; }

    /* Where to install */
    const char *home = getenv("HOME");
    if (!home) { cat_error("shpm: HOME not set"); return; }

    char install_base[PATH_MAX];
    snprintf(install_base, sizeof(install_base), "%s/.sharpen/packages/%s", home, pkg->name);
    char tmp_dir[PATH_MAX];
    snprintf(tmp_dir, sizeof(tmp_dir), "%s/.sharpen/tmp", home);

    /* Check free space on the target filesystem */
    mkdir(tmp_dir, 0755); /* ensure it exists for statvfs */
    struct statvfs vfs;
    if (statvfs(tmp_dir, &vfs) != 0) { cat_perror("shpm: statvfs"); return; }
    unsigned long long free_bytes = (unsigned long long)vfs.f_frsize * vfs.f_bavail;
    char free_str[32], pkg_str[32];
    format_size(free_bytes, free_str, sizeof(free_str));
    format_size(pkg->size, pkg_str, sizeof(pkg_str));

    printf(BOLD CYAN "Free space:" RESET " %s   " BOLD CYAN "Package:" RESET " %s\n", free_str, pkg_str);
    if ((free_bytes - pkg->size) < 50ULL * 1024 * 1024 * 1024) {
        printf(RED "Nyā! After installing %s, you'd have less than 50 GB free.\n" RESET, pkg->name);
        printf("Please free up space to at least 55 GB (50 GB + package size).\n");
        printf("Then try again :3\n");
        return;
    }

    /* Pick downloader */
    int use_curl = tool_exists("curl");
    int use_wget = tool_exists("wget");
    if (!use_curl && !use_wget) {
        cat_error("shpm: need curl or wget to download. Install one of them!");
        return;
    }

    /* Build download URL */
    char url[512];
    snprintf(url, sizeof(url),
             "https://github.com/%s/releases/download/%s/%s",
             pkg->repo, pkg->version, pkg->asset);
    char tarball[PATH_MAX];
    snprintf(tarball, sizeof(tarball), "%s/%s", tmp_dir, pkg->asset);

    printf(BOLD CYAN "╭── Installing " GREEN "%s" CYAN " from GitHub ──╮\n" RESET, pkg->name);
    printf("" BOLD "Repo:" RESET " %s\n", pkg->repo);
    printf("" BOLD "Version:" RESET " %s\n", pkg->version);

    /* Create temp dir */
    mkdir(tmp_dir, 0755);
    mkdir(install_base, 0755);

    /* Download with cuteness */
    printf(BOLD GREEN "Nyā~ Downloading %s...\n" RESET, pkg->name);
    pid_t pid = fork();
    if (pid == 0) {
        /* child */
        if (use_curl) {
            execlp("curl", "curl", "-L", "-o", tarball, url, (char*)NULL);
        } else {
            execlp("wget", "wget", "-O", tarball, url, (char*)NULL);
        }
        cat_perror("shpm: exec downloader");
        _exit(EXIT_FAILURE);
    } else if (pid < 0) {
        cat_perror("shpm: fork"); return;
    } else {
        print_spin("Downloading", pid);
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            cat_error("Download failed! Check your internet and the package name.");
            unlink(tarball);  /* clean up partial file */
            return;
        }
    }

    /* Extract if tarball */
    printf(BOLD GREEN "Extracting %s...\n" RESET, pkg->name);
    pid = fork();
    if (pid == 0) {
        execlp("tar", "tar", "xzf", tarball, "-C", install_base, (char*)NULL);
        cat_perror("shpm: exec tar");
        _exit(EXIT_FAILURE);
    } else if (pid < 0) {
        cat_perror("shpm: fork"); return;
    } else {
        print_spin("Extracting", pid);
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            cat_error("Extraction failed!");
            unlink(tarball);
            return;
        }
    }

    /* Remove tarball */
    unlink(tarball);

    /* Create marker file */
    char marker[PATH_MAX];
    snprintf(marker, sizeof(marker), "%s/.installed", install_base);
    FILE *f = fopen(marker, "w");
    if (f) { fprintf(f, "%s %s\n", pkg->name, pkg->version); fclose(f); }

    /* Show post-install message */
    printf(BOLD GREEN "╰── " GREEN "Package %s installed successfully! :3\n" RESET, pkg->name);
    if (pkg->post_install) {
        printf("You can run it with:  " BOLD "%s/%s\n" RESET, install_base, pkg->post_install);
    }
}

/* ---------- Help system ---------- */
static void builtin_help(char **args);

/* ========== Command table ========== */
static const SharpenCommand commands[] = {
    {"ed",       "enter directory (~, ^^)",     "Usage: " BOLD "ed PATH:<directory>" RESET, builtin_ed},
    {"lp",       "list files/folders (no ls!)", "Usage: " BOLD "lp [options]" RESET "\nOptions: -nofile, -nofolder, -ext .ext", builtin_lp},
    {"file",     "create empty file",           "Usage: " BOLD "file <name>" RESET, builtin_file},
    {"directory","create folder",               "Usage: " BOLD "directory <name>" RESET, builtin_directory},
    {"ram",      "storage usage with bar",      "Usage: " BOLD "ram [path]" RESET, builtin_ram},
    {"clear",    "clear screen (also cls)",     "Usage: " BOLD "clear" RESET " or " BOLD "cls" RESET, builtin_clear},
    {"info",     "system information",          "Usage: " BOLD "info" RESET, builtin_info},
    {"help",     "show this help",              "Usage: " BOLD "help" RESET " or " BOLD "help <cmd>" RESET, builtin_help},
    {"exit",     "leave terminal",              "Usage: " BOLD "exit" RESET, builtin_exit},
    {"theme",    "list/set colour theme",       "Usage: " BOLD "theme [name]" RESET "\nAvailable: default, hacker, pastel, dark", builtin_theme},
    {"changelog","show version history",        "Usage: " BOLD "changelog" RESET, builtin_changelog},
    {"shpm",     "Sharp Package Manager",       "Usage: " BOLD "shpm install <pkg>" RESET " / " BOLD "shpm list" RESET, builtin_shpm}
};
static const size_t cmd_count = sizeof(commands)/sizeof(commands[0]);

static void builtin_help(char **args) {
    if (args[1] == NULL) {
        printf(BOLD CYAN "╭──────── SharpenOS Commands ─────────╮\n" RESET);
        for (size_t i = 0; i < cmd_count; i++)
            printf("  " GREEN "%-12s" RESET " → %s\n", commands[i].name, commands[i].short_help);
        printf(BOLD CYAN "╰────────────────────────────────────╯\n" RESET);
        printf("Any other command runs as a normal program.\n");
        return;
    }
    for (size_t i = 0; i < cmd_count; i++) {
        if (strcmp(args[1], commands[i].name) == 0) {
            printf(BOLD GREEN "%s" RESET " – %s\n", commands[i].name, commands[i].short_help);
            printf("%s\n", commands[i].long_help);
            return;
        }
    }
    if (strcmp(args[1], "cls") == 0) {
        printf(BOLD GREEN "clear / cls" RESET " – Clear the screen\nUsage: " BOLD "clear" RESET " or " BOLD "cls" RESET "\n");
        return;
    }
    cat_error("No detailed help for that command. Try 'help' alone.");
}

/* ---------- External runner ---------- */
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
        int status; waitpid(pid, &status, 0);
    }
}

/* ---------- Splash ---------- */
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
        if (!fgets(raw_input, MAX_INPUT, stdin)) { printf("\n"); break; }
        raw_input[strcspn(raw_input, "\n")] = '\0';

        int n = 0;
        char *tok = strtok(raw_input, " \t\n");
        while (tok && n < MAX_ARGS-1) { raw_args[n++] = tok; tok = strtok(NULL, " \t\n"); }
        raw_args[n] = NULL;
        if (n == 0) continue;

        expand_args(raw_args, exp_args, MAX_ARGS);

        /* Block legacy commands */
        if (strcmp(exp_args[0], "ls") == 0 || strcmp(exp_args[0], "cd") == 0 ||
            strcmp(exp_args[0], "touch") == 0 || strcmp(exp_args[0], "mkdir") == 0) {
            cat_error("Oops! Those commands aren't allowed here!\n"
                      "  × ls   → use lp\n"
                      "  × cd   → use ed PATH:<dir>\n"
                      "  × touch → use file <name>\n"
                      "  × mkdir → use directory <name>\n"
                      "  >^._.^<");
            free_args(exp_args);
            continue;
        }

        int found = 0;
        for (size_t i = 0; i < cmd_count; i++) {
            if (strcmp(exp_args[0], commands[i].name) == 0 ||
                (strcmp(exp_args[0], "cls") == 0 && strcmp(commands[i].name, "clear") == 0)) {
                if (strcmp(commands[i].name, "exit") == 0) {
                    free_args(exp_args);
                    goto quit;
                }
                commands[i].func(exp_args);
                found = 1;
                break;
            }
        }
        if (!found) execute_external(exp_args);
        free_args(exp_args);
    }

quit:
    printf(BOLD GREEN "\nGoodbye from SharpenOS! Stay purr-fect! :3\n" RESET);
    return 0;
}