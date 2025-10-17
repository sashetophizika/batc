#pragma once
#include <stdbool.h>

#define toggle(x)                                                              \
  {                                                                            \
    x = x ? false : true;                                                      \
  }

#define BAT_HEIGHT 6
#define BAT_WIDTH 42
#define MAX_BLOCKS_BIG 33
#define MAX_BLOCKS_SMALL 14
#define FETCHCOL_BIG 63
#define FETCHCOL_SMALL 31
#define CHARGE_SIZE_BIG 13
#define CHARGE_SIZE_SMALL 6

typedef enum Mode {
  capacity,
  power,
  temp,
  times,
  health,
  charge,
} Mode;

typedef struct Battery {
  int capacity;
  int temp;
  int times;
  float power;
  float health;
  float charge;
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
  bool fetch;
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

bool bat_eq(Battery *bat1, Battery *bat2);

void define_position(int w, int h);
