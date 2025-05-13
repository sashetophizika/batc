#include "state.h"
#include <unistd.h>

Battery bat = {0, 0, 0, 0, 0, false, NULL};
Battery prev_bat = {0, 0, 0, 0, 0, false, NULL};

Colors colors = {.high = "\033[0;32m\0",
                 .mid = "\033[0;33m\0",
                 .low = "\033[0;31m\0",
                 .temp = "\033[0;35m\0",
                 .health = "\033[0;31m\0",
                 .full = "\033[0;36m\0",
                 .left = "\033[0;34m\0",
                 .charge = "\033[0;36m\0",
                 .tech = "\033[0;36m\0",
                 .shell = "\033[0m\0",
                 .number = NULL};

Flags flags = {.colors = true,
               .live = false,
               .minimal = false,
               .small = false,
               .digits = false,
               .fat = false,
               .alt_charge = false,
               .extra_colors = true,
               .inlin = false,
               .tech = false,
               .mode = capacity,
               .bat_number = ""};

int start_row = 0;
int start_col = 0;
int rows = 0;
int cols = 0;

bool redraw = true;
int prev_digits = 0;

int blocks = 0;
int prev_blocks = 0;

const char *inner_color = "";
const char *prev_inner_color = "";
