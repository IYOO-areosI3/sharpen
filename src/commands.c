#include "sharpen.h"
#include "commands.h"
#include <ctype.h>
#include <sys/wait.h>

/* ---------- Command table (global, used by help) ---------- */
const SharpenCommand commands[] = {
    {"ed",       "enter directory (~, ^^)",
     "Usage: ed PATH:<directory>", builtin_ed},
    {"lp",       "list files/folders",
     "Usage: lp [options]", builtin_lp},
    {"file",     "create empty file",
     "Usage: file <name>", builtin_file},
    {"directory","create folder",
     "Usage: directory <name>", builtin_directory},
    {"ram",      "storage usage",
     "Usage: ram [path]", builtin_ram},
    {"clear",    "clear screen (also cls)",
     "Usage: clear or cls", builtin_clear},
    {"info",     "system info",
     "Usage: info", builtin_info},
    {"help",     "this help",
     "Usage: help [cmd]", builtin_help},
    {"exit",     "leave terminal",
     "Usage: exit", builtin_exit},
    {"theme",    "list/set theme",
     "Usage: theme [name]", builtin_theme},
    {"changelog","version history",
     "Usage: changelog", builtin_changelog},
    {"shpm",     "package manager",
     "Usage: shpm install <pkg> | shpm list", builtin_shpm},
    {"rd",       "search keyword in files",
     "Usage: rd <keyword> [start_dir]", builtin_rd}
};
const size_t cmd_count = sizeof(commands) / sizeof(commands[0]);

/* ========== Built‑in implementations ========== */

void builtin_ed(char **args) {
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
        snprintf(resolved, PATH_MAX, "%s%s", home, raw_path + 1);
    } else if (strcmp(raw_path, "^^") == 0) {
        strncpy(resolved, "..", PATH_MAX); resolved[PATH_MAX-1] = '\0';
    } else {
        strncpy(resolved, raw_path, PATH_MAX); resolved[PATH_MAX-1] = '\0';
    }
    if (chdir(resolved) != 0) cat_perror("ed");
}

void builtin_lp(char **args) {
    int show_files = 1, show_dirs = 1;
    const char *ext = NULL;
    for (int i = 1; args[i]; i++) {
        if (strcmp(args[i], "-nofile") == 0)
            show_files = 0;
        else if (strcmp(args[i], "-nofolder") == 0)
            show_dirs = 0;
        else if (strcmp(args[i], "-ext") == 0) {
            if (args[i+1]) ext = args[++i];
            else { cat_error("lp: -ext requires an extension"); return; }
        } else {
            fprintf(stderr, RED "lp: unknown option %s" RESET "\n", args[i]);
            return;
        }
    }
    DIR *dir = opendir(".");
    if (!dir) { cat_perror("lp"); return; }
    struct dirent *d; struct stat st;
    while ((d = readdir(dir))) {
        if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")) continue;
        if (stat(d->d_name, &st) == -1) continue;
        if (ext) {
            if (S_ISREG(st.st_mode)) {
                size_t nl = strlen(d->d_name), el = strlen(ext);
                if (nl >= el && strcmp(d->d_name + nl - el, ext) == 0)
                    list_file_colored(d->d_name, &st);
            }
        } else {
            if ((S_ISDIR(st.st_mode) && show_dirs) ||
                (S_ISREG(st.st_mode) && show_files))
                list_file_colored(d->d_name, &st);
        }
    }
    closedir(dir);
}

void builtin_file(char **args) {
    if (args[1] == NULL) { cat_error("file: give me a filename!"); return; }
    int fd = open(args[1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) { cat_perror("file"); return; }
    close(fd);
    printf(BOLD GREEN "Nyā~ created file: %s" RESET "\n", args[1]);
}

void builtin_directory(char **args) {
    if (args[1] == NULL) { cat_error("directory: give me a name!"); return; }
    if (mkdir(args[1], 0755) != 0) { cat_perror("directory"); }
    else printf(BOLD GREEN "Nyā~ created directory: %s" RESET "\n", args[1]);
}

void builtin_ram(char **args) {
    const char *path = (args[1] != NULL) ? args[1] : ".";
    struct statvfs vfs;
    if (statvfs(path, &vfs) != 0) { cat_perror("ram"); return; }
    unsigned long long total = (unsigned long long)vfs.f_frsize * vfs.f_blocks;
    unsigned long long avail = (unsigned long long)vfs.f_frsize * vfs.f_bavail;
    unsigned long long used = total - avail;
    char total_str[32], used_str[32], avail_str[32];
    format_size(total, total_str, sizeof(total_str));
    format_size(used,  used_str,  sizeof(used_str));
    format_size(avail, avail_str, sizeof(avail_str));
    /* Fixed: cast both operands to double to avoid -Wconversion */
    double percent_used = (total > 0) ? ((double)used / (double)total) * 100.0 : 0.0;
    int bar_len = 20;
    int filled = (int)(percent_used / 100.0 * bar_len + 0.5);
    if (filled > bar_len) filled = bar_len;
    printf(BOLD CYAN "╭── 📦 Storage on %s ──╮\n" RESET, path);
    printf("  ");
    for (int i = 0; i < bar_len; i++)
        printf(i < filled ? RED "█" : GREEN "█");
    printf(RESET "  %5.1f%% used\n", percent_used);
    printf("  " BOLD "Free:" RESET " %s  |  " BOLD "Used:" RESET " %s  |  " BOLD "Total:" RESET " %s\n",
           avail_str, used_str, total_str);
    printf(BOLD CYAN "╰──────────────────────────╯\n" RESET);
}

void builtin_clear(char **args) {
    (void)args;
    printf("\x1b[2J\x1b[H");
}

void builtin_info(char **args) {
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

void builtin_exit(char **args) {
    (void)args;  /* handled in main */
}

void builtin_theme(char **args) {
    if (args[1] == NULL) {
        printf(BOLD CYAN "Available themes:\n" RESET);
        for (size_t i = 0; i < theme_count; i++)
            printf("  " GREEN "%s" RESET "\n", themes[i]->name);
        printf("Current theme: " BOLD GREEN "%s" RESET "\n", current_theme->name);
        return;
    }
    for (size_t i = 0; i < theme_count; i++) {
        if (strcmp(args[1], themes[i]->name) == 0) {
            theme_set(args[1]);
            printf(BOLD GREEN "Theme changed to %s :3\n" RESET, themes[i]->name);
            return;
        }
    }
    cat_error("Unknown theme. Use 'theme' to list.");
}

void builtin_changelog(char **args) {
    (void)args;
    printf(
        BOLD GREEN "SharpenOS Changelog\n" RESET
        "-------------------------\n"
        "4.1 – Global headers, rd search, zero warnings\n"
        "4.0 – Modular project structure\n"
        "3.7 – SHpm with GitHub downloader\n"
        "3.6 – Theme manager, changelog\n"
        "3.5 – Zero warnings, command struct\n"
        "3.4 – ed supports ~ and ^^\n"
        "3.3 – help <cmd> detailed\n"
        "3.2 – Error messages with :3 Nyā~\n"
        "3.1 – First public release\n"
    );
}

/* ---------- rd (recursive search) ---------- */
static void search_in_file(const char *path, const char *keyword) {
    FILE *fp = fopen(path, "r");
    if (!fp) return;
    char line[1024];
    int lineno = 0;
    while (fgets(line, sizeof(line), fp)) {
        lineno++;
        char lower_line[1024];
        char lower_key[256];
        /* Fixed: explicit cast to char */
        for (int i = 0; line[i]; i++) lower_line[i] = (char)tolower((unsigned char)line[i]);
        lower_line[strlen(line)] = '\0';
        for (int i = 0; keyword[i]; i++) lower_key[i] = (char)tolower((unsigned char)keyword[i]);
        lower_key[strlen(keyword)] = '\0';

        if (strstr(lower_line, lower_key)) {
            printf(BOLD GREEN "%s:%d" RESET ": ", path, lineno);
            char *p = line;
            while (*p) {
                if (strncasecmp(p, keyword, strlen(keyword)) == 0) {
                    printf(RED "%s" RESET, keyword);
                    p += strlen(keyword);
                } else {
                    putchar(*p);
                    p++;
                }
            }
            printf("\n");
        }
    }
    fclose(fp);
}

static void search_recursive(const char *dir_path, const char *keyword) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        struct stat st;
        if (stat(full_path, &st) == -1) continue;
        if (S_ISDIR(st.st_mode))
            search_recursive(full_path, keyword);
        else if (S_ISREG(st.st_mode))
            search_in_file(full_path, keyword);
    }
    closedir(dir);
}

void builtin_rd(char **args) {
    if (args[1] == NULL) {
        cat_error("rd: give me a keyword to search for!");
        return;
    }
    const char *keyword = args[1];
    const char *start_dir = args[2] ? args[2] : ".";
    printf(BOLD CYAN "Searching for '%s' in %s..." RESET "\n", keyword, start_dir);
    search_recursive(start_dir, keyword);
}

/* ---------- SHpm (cute GitHub package manager) ---------- */
static int tool_exists(const char *name) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/usr/bin/%s", name);
    return access(path, X_OK) == 0;
}

static void print_spin(const char *msg, pid_t child) {
    static const char spin[] = "|/-\\";
    int i = 0;
    printf(BOLD CYAN "%s... " RESET, msg);
    fflush(stdout);
    while (1) {
        int status;
        pid_t res = waitpid(child, &status, WNOHANG);
        if (res == child) break;
        printf("\r" BOLD CYAN "%s... %c" RESET, msg, spin[i % 4]);
        fflush(stdout);
        i++;
        usleep(200000);  /* now defined because of _DEFAULT_SOURCE */
    }
    printf("\r" BOLD CYAN "%-60s" RESET "\r", " ");
    fflush(stdout);
}

typedef struct {
    const char *name;
    const char *desc;
    const char *repo;
    const char *version;
    const char *asset;
    unsigned long long size;
    const char *post_install;
} SHpmPackage;

static const SHpmPackage packages[] = {
    {"nyacat", "The cutest cat fetcher :3",
     "sharpen-os/nyacat", "1.0.0", "nyacat.linux.x86_64.tar.gz",
     1ULL * 1024 * 1024 * 1024,  /* 1 GB */
     "nyacat/nyacat"}
};
static const size_t pkg_count = sizeof(packages)/sizeof(packages[0]);

void builtin_shpm(char **args) {
    if (args[1] == NULL) {
        printf(BOLD GREEN "SHpm" RESET " - Sharp Package Manager :3\n");
        printf("Usage: shpm install <package>\n");
        printf("       shpm list\n");
        return;
    }
    if (strcmp(args[1], "list") == 0) {
        printf(BOLD CYAN "Available packages:\n" RESET);
        for (size_t i = 0; i < pkg_count; i++) {
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

    const SHpmPackage *pkg = NULL;
    for (size_t i = 0; i < pkg_count; i++) {
        if (strcmp(args[2], packages[i].name) == 0) { pkg = &packages[i]; break; }
    }
    if (!pkg) { cat_error("shpm install: package not found."); return; }

    const char *home = getenv("HOME");
    if (!home) { cat_error("shpm: HOME not set"); return; }

    char install_base[PATH_MAX];
    snprintf(install_base, sizeof(install_base), "%s/.sharpen/packages/%s", home, pkg->name);
    char tmp_dir[PATH_MAX];
    snprintf(tmp_dir, sizeof(tmp_dir), "%s/.sharpen/tmp", home);
    mkdir(tmp_dir, 0755);

    struct statvfs vfs;
    if (statvfs(tmp_dir, &vfs) != 0) { cat_perror("shpm: statvfs"); return; }
    unsigned long long free_bytes = (unsigned long long)vfs.f_frsize * vfs.f_bavail;
    char free_str[32], pkg_str[32];
    format_size(free_bytes, free_str, sizeof(free_str));
    format_size(pkg->size, pkg_str, sizeof(pkg_str));
    printf(BOLD CYAN "Free space:" RESET " %s   " BOLD CYAN "Package:" RESET " %s\n", free_str, pkg_str);
    if ((free_bytes - pkg->size) < 50ULL * 1024 * 1024 * 1024) {
        printf(RED "Nyā! After install you'd have less than 50 GB free.\n" RESET);
        printf("Please free up space and try again :3\n");
        return;
    }

    int use_curl = tool_exists("curl");
    int use_wget = tool_exists("wget");
    if (!use_curl && !use_wget) {
        cat_error("shpm: need curl or wget to download.");
        return;
    }
    char url[512];
    snprintf(url, sizeof(url), "https://github.com/%s/releases/download/%s/%s",
             pkg->repo, pkg->version, pkg->asset);
    char tarball[PATH_MAX];
    snprintf(tarball, sizeof(tarball), "%s/%s", tmp_dir, pkg->asset);

    printf(BOLD CYAN "╭── Installing " GREEN "%s" CYAN " from GitHub ──╮\n" RESET, pkg->name);
    printf("" BOLD "Repo:" RESET " %s\n", pkg->repo);
    printf("" BOLD "Version:" RESET " %s\n", pkg->version);
    printf(BOLD GREEN "Nyā~ Downloading...\n" RESET);

    pid_t pid = fork();
    if (pid == 0) {
        if (use_curl)
            execlp("curl", "curl", "-L", "-o", tarball, url, (char*)NULL);
        else
            execlp("wget", "wget", "-O", tarball, url, (char*)NULL);
        _exit(1);
    } else if (pid < 0) { cat_perror("fork"); return; }
    else {
        print_spin("Downloading", pid);
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            cat_error("Download failed!");
            unlink(tarball); return;
        }
    }

    mkdir(install_base, 0755);
    printf(BOLD GREEN "Extracting...\n" RESET);
    pid = fork();
    if (pid == 0) {
        execlp("tar", "tar", "xzf", tarball, "-C", install_base, (char*)NULL);
        _exit(1);
    } else if (pid < 0) { cat_perror("fork"); return; }
    else {
        print_spin("Extracting", pid);
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            cat_error("Extraction failed!");
            unlink(tarball); return;
        }
    }
    unlink(tarball);

    char marker[PATH_MAX];
    snprintf(marker, sizeof(marker), "%s/.installed", install_base);
    FILE *f = fopen(marker, "w");
    if (f) { fprintf(f, "%s %s\n", pkg->name, pkg->version); fclose(f); }

    printf(BOLD GREEN "╰── " GREEN "Package %s installed! :3\n" RESET, pkg->name);
    if (pkg->post_install)
        printf("Run with:  " BOLD "%s/%s\n" RESET, install_base, pkg->post_install);
}

/* ---------- help (uses command table) ---------- */
void builtin_help(char **args) {
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
        printf(BOLD GREEN "clear / cls" RESET " – Clear the screen\nUsage: clear or cls\n");
        return;
    }
    cat_error("No detailed help for that command. Try 'help' alone.");
}