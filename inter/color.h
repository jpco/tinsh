#ifndef JPSH_COLOR_H
#define JPSH_COLOR_H

/**
 * Colors a given set of arguments, according
 * to whether the arguments are commands or files;
 * prints the colored line (including "\n")
 */
void color_line (int argc, const char **argv);

/**
 * Splits a string into a set of commands, then
 * colors it with color_line.
 */
void color_line_s (const char *line);

#endif
