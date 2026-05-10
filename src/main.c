#include "sharpen.h"

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
        while (tok && n < MAX_ARGS - 1) {
            raw_args[n++] = tok;
            tok = strtok(NULL, " \t\n");
        }
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
        if (!found) {
            pid_t pid = fork();
            if (pid == 0) {
                execvp(exp_args[0], exp_args);
                cat_perror("exec");
                _exit(127);
            } else if (pid > 0) {
                int status; waitpid(pid, &status, 0);
            } else {
                cat_perror("fork");
            }
        }
        free_args(exp_args);
    }

quit:
    printf(BOLD GREEN "\nGoodbye from SharpenOS! Stay purr-fect! :3\n" RESET);
    return 0;
}