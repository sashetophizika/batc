#include <dirent.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "print.h"
#include "state.h"
#include "utils.h"

void main_loop(bool redefine) {
  flags.small ? print_small() : print_big(redefine);
}

void print_help(void) {
  puts("Usage:\r\n"
       "  battery [-lsmbdfne]\r\n"
       "\r\n"
       "Options\r\n"
       "  -l, --live                       Monitor the battery live\r\n"
       "  -s, --small                      Draw a small version of the "
       "battery\r\n"
       "  -i, --inline                     Draw the battery inline instead "
       "  -t, --tech                       Draw the technology of the battery "
       "of the center of the screen\r\n"
       "  -f, --fat                        Draws a slightly thicker "
       "battery\r\n"
       "  -d, --digits                     Draw the current capacity as a "
       "number in the "
       "battery\r\n"
       "  -m, --mode MODE                  Specify the mode to be printed "
       "with "
       "-d (c for "
       "capacity, p for power, t for temperature, h for health)\r\n"
       "  -e, --extra-colors               Disable extra core color patterns "
       "for different "
       "modes\r\n"
       "  -m, --minimal                    Minimal print of the battery "
       "status\r\n"
       "  -c, --alt-charge                 Use alternate charging symbol "
       "(requires nerd "
       "fonts)\r\n"
       "  -n, --no-color                   Disable colors\r\n"
       "  -b, --bat-number BAT_NUMBER      Specify battery number");
}
