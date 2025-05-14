#pragma once
#include <stdbool.h>

#define toggle(x)                                                              \
  {                                                                            \
    x = x ? false : true;                                                      \
  }

typedef enum Mode { capacity, power, temperature, health, time_m } Mode;

typedef struct Battery {
  int capacity;
  int temp;
  int power;
  int health;
  int time;
  bool is_charging;
  const char *tech;
} Battery;

typedef struct DrawState {
  int start_row;
  int start_col;
  int term_rows;
  int term_cols;
  bool redraw;
  const char *inner_color;
} DrawState;

typedef struct Colors {
  const char *high;
  const char *mid;
  const char *low;
  const char *temp;
  const char *health;
  const char *full;
  const char *left;
  const char *charge;
  const char *tech;
  const char *shell;
  const char *number;
} Colors;

typedef struct Flags {
  bool colors;
  bool live;
  bool minimal;
  bool small;
  bool digits;
  bool fat;
  bool alt_charge;
  bool extra_colors;
  bool inlin;
  bool tech;
  Mode mode;
  char bat_number[50];
} Flags;

extern Battery bat;
extern Colors colors;
extern Flags flags;
extern DrawState state;
