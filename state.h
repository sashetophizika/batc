#pragma once
#include <stdbool.h>

typedef enum Mode { capacity, power, temperature, health, time_m } Mode;

typedef struct Battery {
  int capacity;
  int temp;
  int power;
  int health;
  int time;
  bool is_charging;
  char *tech;
} Battery;

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
extern Battery prev_bat;

extern Colors colors;

extern Flags flags;

extern int start_row;
extern int start_col;
extern int rows;
extern int cols;

extern bool redraw;
extern int prev_digits;

extern int blocks;
extern int prev_blocks;

extern const char *inner_color;
extern const char *prev_inner_color;
