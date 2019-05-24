#ifndef __DEBUG_PRINT_H_
#define __DEBUG_PRINT_H_

#define LIGHT_BLUE    "\033[1;34m"
#define DARY_GRAY     "\033[1;30m"
#define LIGHT_RED     "\033[1;31m"
#define LIGHT_PURPLE "\033[1;35m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define NONE          "\033[m"

#define ANSI_COLOR_RESET   "\x1b[0m"

#define red_pr(fmt, args...)		printf(ANSI_COLOR_RED fmt NONE,##args)
#define purple_pr(fmt, args...)		printf(LIGHT_PURPLE fmt NONE,##args)
#define blue_pr(fmt, args...)		printf(LIGHT_BLUE fmt NONE,##args)

#endif
