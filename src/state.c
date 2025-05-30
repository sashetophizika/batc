#include <unistd.h>

#include "state.h"

Battery bat = {.capacity = 0,
               .temp = 0,
               .power = 0,
               .times = 0,
               .health = 0,
               .charge = 0,
               .is_charging = false,
               .tech = NULL};

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
               .fetch = false,
               .alt_charge = false,
               .extra_colors = true,
               .inlin = false,
               .tech = false,
               .mode = capacity,
               .bat_number = ""};

DrawState state = {.start_row = 0,
                   .start_col = 0,
                   .term_rows = 0,
                   .term_cols = 0,
                   .redraw = true,
                   .inner_color = NULL};

bool bat_eq(Battery *bat1, Battery *bat2) {
  if (bat1->capacity == bat2->capacity && bat1->temp == bat2->temp &&
      bat1->power == bat2->power && bat1->times == bat2->times &&
      bat1->charge == bat2->charge && bat1->health == bat2->health &&
      bat1->is_charging == bat2->is_charging) {
    return true;
  }
  return false;
}
