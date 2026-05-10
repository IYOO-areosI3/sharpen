#ifndef SHARPEN_COMMANDS_H
#define SHARPEN_COMMANDS_H

#include "sharpen.h"

void builtin_ed(char **args);
void builtin_lp(char **args);
void builtin_file(char **args);
void builtin_directory(char **args);
void builtin_ram(char **args);
void builtin_clear(char **args);
void builtin_info(char **args);
void builtin_help(char **args);
void builtin_exit(char **args);
void builtin_theme(char **args);
void builtin_changelog(char **args);
void builtin_shpm(char **args);
void builtin_rd(char **args);

#endif