#include <stdio.h>
#include <stdlib.h>
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

static char *get_cursor_position(void) {
  char *buf = calloc(3, sizeof(char));
  int i = 0;
  char c = '\0';

  write(STDOUT_FILENO, "\033[6n", 4);
  for (c = 0; c != 'R'; read(STDIN_FILENO, &c, 1)) {
    if (c >= '0' && c <= '9' && i < 3) {
      buf[i] = c;
      i++;
    } else if (c == ';') {
      i = 99;
    }
  }

  return buf;
}

void define_position(int w, int h) {
  state.term_rows = h;
  state.term_cols = w;

  if (flags.small) {
    state.start_row = (state.term_rows - BAT_HEIGHT) / 2;
    state.start_col = (state.term_cols - BAT_WIDTH) / 2;

  } else if (flags.inlin || flags.fetch) {
    char *pos = get_cursor_position();
    state.start_row = atoi(pos) + 1;
    free(pos);

    state.start_col = 3;
    const int min_rows = 10 + flags.fat;

    if (state.term_rows - state.start_row < min_rows) {
      state.start_row = state.term_rows - min_rows + 1;

      for (int j = 0; j < min_rows; j++) {
        printf("\r\n");
      }
    }

  } else {
    printf("\033[2J");
    state.start_row = (state.term_rows - BAT_HEIGHT) / 2;
    state.start_col = (state.term_cols - BAT_WIDTH) / 2;
  }
}
