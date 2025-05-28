#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "print.h"
#include "state.h"

#define MAX_BLOCKS_BIG 33
#define MAX_BLOCKS_SMALL 14
#define FETCHCOL_BIG 63
#define FETCHCOL_SMALL 31
#define CHARGE_SIZE_BIG 13
#define CHARGE_SIZE_SMALL 6

static char *bat_name(char *batfile) {
  char *tempstr = calloc(strlen(batfile) + 1, sizeof(char));
  strncpy(tempstr, batfile, strlen(batfile) + 1);

  char *name = strtok(tempstr, "/");
  char *temp = name;

  while (temp != NULL) {
    name = temp;
    temp = strtok(NULL, "/");
  }

  char *n = calloc(5, sizeof(char));
  memcpy(n, name, 4);
  free(tempstr);
  return n;
}

static int get_bitstring(int digit) {
  switch (digit) {
  case 0:              // 111111
    return 1070546175; // 110011
                       // 110011
                       // 110011
                       // 111111

  case 1:              // 111111
    return 1060160271; // 001100
                       // 001100
                       // 001100
                       // 001111

  case 2:              // 111111
    return 1058012223; // 000011
                       // 111111
                       // 110000
                       // 111111

  case 3:              // 111111
    return 1069808703; // 110000
                       // 111111
                       // 110000
                       // 111111

  case 4:             /// 110000
    return 818150643; /// 110000
                      /// 111111
                      /// 110011
                      /// 110011

  case 5:              // 111111
    return 1069805823; // 110000
                       // 111111
                       // 000011
                       // 111111

  case 6:              // 111111
    return 1070592255; // 110011
                       // 111111
                       // 000011
                       // 111111

  case 7:             /// 110000
    return 818089023; /// 110000
                      /// 110000
                      /// 110000
                      /// 111111

  case 8:              // 111111
    return 1070595327; // 110011
                       // 111111
                       // 110011
                       // 111111

  case 9:              // 111111
    return 1069808895; // 110000
                       // 111111
                       // 110011
                       // 111111
  default:
    return 0;
  }
}

static void print_digit(int digit, int row, int col) {
  int bitstring = get_bitstring(digit);
  int negate = 0;

  const char *digit_color = colors.number;
  if (colors.number == NULL || !flags.digits) {
    negate = bat.capacity / 3 + state.start_col + 3 - col;
    digit_color = state.inner_color;
  }

  printf("\033[%d;%dH%s", row, col, digit_color);
  for (int i = 0; i < 30; i++) {
    if ((i % 6 < negate) != (bitstring >> i & 1)) {
      printf("█");
    } else if (colors.number == NULL) {
      printf(" ");
    } else {
      printf("\033[1C");
    }

    if (i % 6 == 5) {
      printf("\033[1B\033[6D");
    }
  }

  printf("\033[%d;0H", row + flags.fat + 3);
}

static int count_digits(int num) {
  int count = 0;
  do {
    num /= 10;
    count++;
  } while (num != 0);

  return count;
}

static int get_data(Mode mode) {
  switch (mode) {
  case capacity:
    return bat.capacity;
  case temperature:
    return bat.temp;
  case power:
    return bat.power;
  case time_m:
    return bat.time;
  case health:
    return bat.health;
  case charge:
    return bat.charge;
  }
}

static void print_number(int row) {
  int data = get_data(flags.mode);
  int digits = count_digits(data);
  digits = digits > 4 ? 4 : digits;

  int col = state.start_col + 4 * digits + 12;
  for (int i = 0; i < digits; i++) {
    print_digit(flags.digits ? data % 10 : 42, row, col - 8 * i);
    data /= 10;
  }
}

static void print_tech(void) {
  if (flags.inlin || flags.fetch) {
    return;
  }

  printf("%s\033[%d;%dH", colors.tech, state.start_row + 2,
         state.start_col - 16);
  if (!flags.tech) {
    printf("             "
           "\033[1B\033[13D             "
           "\033[1B\033[13D             "
           "\033[1B\033[13D             "
           "\033[1B\033[13D             ");
    return;
  }

  if (strstr(bat.tech, "Ni") != NULL) {
    if (flags.fat) {
      printf("███   ██  ██"
             "\033[1B\033[13D███  ██    "
             "\033[1B\033[13D██ █ ██  ██"
             "\033[1B\033[13D██  ███  ██"
             "\033[1B\033[13D██   ███  ██");
    } else {
      printf(" ███  ██  ██"
             "\033[1B\033[13D ███ ██    "
             "\033[1B\033[13D ██ ███  ██"
             "\033[1B\033[13D ██  ███  ██"
             "\033[1B\033[13D             ");
    }
  } else if (strstr(bat.tech, "Li") != NULL) {
    if (flags.fat) {
      printf("  ██       ██"
             "\033[1B\033[13D  ██         "
             "\033[1B\033[13D  ██       ██"
             "\033[1B\033[13D  ██       ██"
             "\033[1B\033[13D  ███████  ██");
    } else {
      printf("   ██      ██"
             "\033[1B\033[13D   ██        "
             "\033[1B\033[13D   ██      ██"
             "\033[1B\033[13D   ██████  ██"
             "\033[1B\033[13D             ");
    }
  }
}

void print_charge(void) {
  printf("%s\033[%d;%dH", colors.charge, state.start_row + 3,
         state.start_col + 47);

  if (!bat.is_charging) {
    const char *clear = flags.fetch ? " " : "             ";
    const int back = flags.fetch ? 1 : 13;
    printf("%1$s"
           "\033[1B\033[%2$dD%1$s"
           "\033[1B\033[%2$dD%1$s",
           clear, back);
  } else if (flags.fat && flags.alt_charge) {
    printf("   ███████ "
           "\033[1B\033[13D    ██     "
           "\033[1B\033[13D███████    ");
  } else if (flags.fat) {
    printf("    ███████  "
           "\033[1B\033[13D    ███      "
           "\033[1B\033[13D███████      ");
  } else if (flags.alt_charge) {
    printf("   ██████ "
           "\033[1B\033[13D ██████    "
           "\033[1B\033[13D             ");
  } else {
    printf("   ████████  "
           "\033[1B\033[13D████████     "
           "\033[1B\033[13D             ");
  }
}

static void print_col(int blocks, int core_rows) {
  static int prev_blocks = 0;

  if (blocks == prev_blocks) {
    return;
  }

  const int diff = blocks - prev_blocks;
  const char *new_sym;
  int step, start, end;

  if (diff > 0) {
    step = 1;
    start = 1;
    end = diff + 1;
    new_sym = "█";
  } else {
    step = -1;
    start = 0;
    end = diff;
    new_sym = " ";
  }

  int col = 0;
  for (int i = start; step * i < step * end; i += step) {
    col = state.start_col + prev_blocks + 2 + i;
    printf("%s\033[%d;%dH", state.inner_color, state.start_row + 1, col);

    for (int j = 0; j < core_rows; j++) {
      printf("%s\033[1B\033[1D", new_sym);
    }
  }
  prev_blocks = blocks;
}

static void print_shell(int core_rows) {
  printf("%s\033[%d;%dH████████████████████████████████████████", colors.shell,
         state.start_row, state.start_col);
  printf("\033[%d;%dH████████████████████████████████████████",
         state.start_row + core_rows + 1, state.start_col);

  for (int i = 1; i <= core_rows; i++) {
    const char *nub = i > 1 && i < core_rows ? "████" : "";
    printf("\033[%d;%dH██\033[%dC████%s", state.start_row + i, state.start_col,
           MAX_BLOCKS_BIG + 1, nub);
  }

  if (!flags.fat && flags.live) {
    printf("\033[%d;%dH    \033[%d;%dH                                       "
           "      \r\n",
           state.start_row + core_rows, state.start_col + MAX_BLOCKS_BIG + 7,
           state.start_row + core_rows + 2, state.start_col);
  }
}

static void print_core(int blocks, int core_rows) {
  const char *block_string = "█████████████████████████████████████\0";
  const char *empty_string = "                                     \0";
  char fill_blocks[150], empty_blocks[50];

  strncpy(fill_blocks, block_string, (blocks + 1) * 3);
  fill_blocks[(blocks + 1) * 3] = '\0';
  strncpy(empty_blocks, empty_string, MAX_BLOCKS_BIG - blocks);
  empty_blocks[MAX_BLOCKS_BIG - blocks] = '\0';

  for (int i = 1; i <= core_rows; i++) {
    printf("\033[%d;%dH%s%s%s", state.start_row + i, state.start_col + 2,
           state.inner_color, fill_blocks, empty_blocks);
  }
}

static void print_bat(void) {
  const int core_rows = 6 + flags.fat;
  const int blocks = bat.capacity / 3;

  if (state.redraw) {
    print_shell(core_rows);
    print_core(blocks, core_rows);
    state.redraw = false;
  } else {
    print_col(blocks, core_rows);
  }
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

static void define_position(void) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  state.term_rows = w.ws_row;
  state.term_cols = w.ws_col;

  if (flags.inlin || flags.fetch) {
    const int min_rows = 10 + flags.fat;
    char *pos = get_cursor_position();

    state.start_row = atoi(pos) + 1;
    state.start_col = 3;

    if (state.term_rows - state.start_row < min_rows) {
      state.start_row = state.term_rows - min_rows + 1;

      for (int j = 0; j < min_rows; j++) {
        printf("\r\n");
      }
    }

    free(pos);
  } else {
    printf("\033[2J");
    state.start_row = state.term_rows / 2 - 3;
    state.start_col = state.term_cols / 2 - 21;
  }
}

static const char *get_default_color(void) {
  if (bat.capacity < 20) {
    return colors.low;
  } else if (bat.capacity < 60) {
    return colors.mid;
  } else {
    return colors.high;
  }
}

static void update_color(Mode mode) {
  if (!flags.extra_colors) {
    state.inner_color = get_default_color();
    return;
  }

  switch (mode) {
  case capacity:
    state.inner_color = get_default_color();
    return;
  case time_m:
  case power:
    state.inner_color = bat.is_charging ? colors.full : colors.left;
    return;
  case health:
  case charge:
    state.inner_color = colors.health;
    return;
  case temperature:
    state.inner_color = colors.temp;
    return;
  }
}

static void update_state(void) {
  static int prev_digits = 0;
  static const char *prev_inner_color = "";

  update_color(flags.mode);
  int data = get_data(flags.mode);

  if (flags.colors == false) {
    state.inner_color = "\033[0m";
  }

  if (prev_digits != count_digits(data) ||
      state.inner_color != prev_inner_color) {
    state.redraw = true;
  }

  prev_digits = count_digits(data);
  prev_inner_color = state.inner_color;
}

static void print_fetch(void) {
  if (!flags.fetch) {
    return;
  }

  int col = 0;
  if (flags.small) {
    col = FETCHCOL_SMALL - (bat.is_charging ? 0 : CHARGE_SIZE_SMALL);
  } else {
    col = FETCHCOL_BIG - (bat.is_charging ? 0 + !flags.fat : CHARGE_SIZE_BIG);
    printf("\033[%d;0H", state.start_row + flags.fetch - 1);
  }

  print_minimal(col);
  printf("\r\033[%dF", 8 + flags.small);
}

static void print_big(bool redefine) {
  if (redefine == true) {
    define_position();
  }

  update_state();
  print_fetch();
  print_bat();
  print_charge();
  print_tech();
  print_number(state.start_row + 2);

  if (flags.inlin || flags.fetch) {
    printf("\033[%d;0H", state.start_row + 2 * flags.fetch + flags.fat - 1);
  }

  printf("\r\n");
}

static void print_small_bat_row(bool top) {
  const int blocks = bat.capacity / 7;
  printf("  %s██%s", colors.shell, state.inner_color);
  for (int i = 0; i < MAX_BLOCKS_SMALL; i++) {
    if (i < blocks) {
      printf("█");
    } else {
      printf(" ");
    }
  }
  printf("%s████", colors.shell);

  if (!bat.is_charging) {
    printf("%s\r\n", flags.fetch ? "   " : "               ");
  } else if (top) {
    printf("   %s▄▄▄\r\n", colors.charge);
  } else {
    printf("  %s▀▀▀\r\n", colors.charge);
  }
}

static void print_small_bat(void) {
  printf("\n  %s██████████████████\r\n", colors.shell);
  print_small_bat_row(true);
  print_small_bat_row(false);
  printf("  %s██████████████████\n", colors.shell);
}

static void print_small(void) {
  update_state();
  print_fetch();
  print_small_bat();
  printf("\r\033[5F");
}

#define print_key(k)                                                           \
  printf("%s%.*s%.*s\r\n", color_pad, max_len, k,                              \
         max_len - (int)strlen(k) + 4, "\033[0m:");

static void print_keys(int col) {
  int max_len;
  if (state.term_cols) {
    max_len = state.term_cols < col ? 0 : state.term_cols - col;
  } else {
    max_len = 1000;
  }

  char color_pad[50];
  const char *key_color = col ? state.inner_color : "\033[0m";

  char *bat_num = bat_name(flags.bat_number);
  printf("%s%s\033[1m%.*s\033[22m\033[0m\r\n", color_pad, colors.charge,
         max_len, bat_num);
  free(bat_num);

  print_key("Battery");
  print_key("Health");
  print_key("Max Charge");
  print_key("Temperature");
  print_key("Technology");
  print_key("Charging");

  if (bat.is_charging) {
    print_key("Power In");
    print_key("Time to Full");
  } else {
    print_key("Power Draw");
    print_key("Time Left");
  }
}

static char *repeat(char c, int n) {
  char *s = calloc(n, sizeof(char));
  int max = n > 13 ? 13 : n;
  for (int i = 0; i < max; i++) {
    s[i] = c;
  }
  return s;
}

#define print_val(format, ...)                                                 \
  snprintf(val_str, 10, format, __VA_ARGS__);                                  \
  printf("\033[%dC%s%.*s\r\n", col, sweep, max_len, val_str);

static void print_vals(int col) {
  int max_len;
  if (state.term_cols) {
    max_len = state.term_cols < col ? 0 : state.term_cols - col;
  } else {
    max_len = 1000;
  }

  char sweep[30];
  memset(sweep, 0, 30);
  char *clean = repeat(' ', max_len);
  snprintf(sweep, 30, "%s\033[%dD", clean, max_len > 13 ? 13 : max_len);
  free(clean);

  printf("\033[0m\r\n");
  char val_str[10];

  print_val("%d%%", bat.capacity);
  print_val("%.1f%%", bat.health);
  print_val("%.1fWh", bat.charge);
  print_val("%d°C", bat.temp);
  print_val("%s", bat.tech);
  print_val("%s", bat.is_charging ? "True" : "False");
  print_val("%.1fW", bat.power);
  print_val("%dH %dM", bat.time / 60, bat.time % 60);
}

void print_minimal(int col) {
  static int prev_padding = 0;

  if (prev_padding != 0 && prev_padding != col) {
    for (int i = 0; i < 9; i++) {
      printf("\033[%dC                           \r\n", prev_padding);
    }
    printf("\033[9F");
    print_keys(col);
    printf("\033[9F");
  }

  if (prev_padding == 0 || state.redraw) {
    print_keys(col);
    printf("\033[9F");
  }

  prev_padding = col;
  print_vals(col + 16);
}

void print_battery(bool redefine) {
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
       "  -i, --inline                     Draw the battery inline instead"
       "of the center of the screen\r\n"
       "  -t, --tech                       Draw the technology of the "
       "battery\r\n"
       "  -f, --fat                        Draws a slightly thicker "
       "battery\r\n"
       "  -F, --fetch                      Adds extra information beside "
       "the "
       "battery\r\n"
       "  -d, --digits                     Draw the current capacity as a "
       "number in the battery\r\n"
       "  -m, --mode MODE                  Specify what to be "
       "printed with -d (capacity, power, temperature or health)\r\n"
       "  -e, --extra-colors               Disable extra core color "
       "patterns "
       "for different modes\r\n"
       "  -m, --minimal                    Minimal print of the battery "
       "status\r\n"
       "  -c, --alt-charge                 Use alternate charging symbol "
       "(requires nerd fonts)\r\n"
       "  -n, --no-color                   Disable colors\r\n"
       "  -b, --bat-number BAT_NUMBER      Specify battery number");
}
