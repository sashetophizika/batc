#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "print.h"
#include "state.h"
#include "status.h"

#define MAX_BLOCKS_BIG 33
#define MAX_BLOCKS_SMALL 14
#define LEFTPAD_BIG 63
#define LEFTPAD_SMALL 31
#define CHARGE_SIZE_BIG 13
#define CHARGE_SIZE_SMALL 6

char *bat_name(char *batfile) {
  char *tempstr = calloc(strlen(batfile) + 1, sizeof(char));
  strcpy(tempstr, batfile);

  char *name = strtok(tempstr, "/");
  char *temp = name;

  while (temp != NULL) {
    name = temp;
    temp = strtok(NULL, "/");
  }
  return name;
}

void print_digit(int digit, int row, int col) {
  int bitstring;
  switch (digit) {
  case 0:                   // 111111
    bitstring = 1070546175; // 110011
                            // 110011
                            // 110011
                            // 111111
    break;
  case 1:                   // 111111
    bitstring = 1060160271; // 001100
                            // 001100
                            // 001100
                            // 001111
    break;
  case 2:                   // 111111
    bitstring = 1058012223; // 000011
                            // 111111
                            // 110000
                            // 111111
    break;
  case 3:                   // 111111
    bitstring = 1069808703; // 110000
                            // 111111
                            // 110000
                            // 111111
    break;
  case 4:                  /// 110000
    bitstring = 818150643; /// 110000
                           /// 111111
                           /// 110011
                           /// 110011
    break;
  case 5:                   // 111111
    bitstring = 1069805823; // 110000
                            // 111111
                            // 000011
                            // 111111
    break;
  case 6:                   // 111111
    bitstring = 1070592255; // 110011
                            // 111111
                            // 000011
                            // 111111
    break;
  case 7:                  /// 110000
    bitstring = 818089023; /// 110000
                           /// 110000
                           /// 110000
                           /// 111111
    break;
  case 8:                   // 111111
    bitstring = 1070595327; // 110011
                            // 111111
                            // 110011
                            // 111111
    break;
  case 9:                   // 111111
    bitstring = 1069808895; // 110000
                            // 111111
                            // 110011
                            // 111111
    break;
  default:
    bitstring = 0;
  }

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

int count_digits(int num) {
  int count = 0;
  do {
    num /= 10;
    count++;
  } while (num != 0);

  return count;
}

void print_number(int row) {
  int data = 0;
  if (flags.mode == capacity) {
    data = bat.capacity;
  } else if (flags.mode == power) {
    data = bat.power;
  } else if (flags.mode == temperature) {
    data = bat.temp;
  } else if (flags.mode == health) {
    data = bat.health;
  } else if (flags.mode == time_m) {
    data = bat.time;
  }

  int digits = count_digits(data);
  digits = digits > 4 ? 4 : digits;

  int col = state.start_col + 4 * digits + 12;
  for (int i = 0; i < digits; i++) {
    print_digit(flags.digits ? data % 10 : 42, row, col - 8 * i);
    data /= 10;
  }
}

void print_tech(void) {
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
    printf("             "
           "\033[1B\033[13D             "
           "\033[1B\033[13D             ");
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

void print_col(int blocks, int core_rows) {
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

void print_bat(void) {
  const int core_rows = 6 + flags.fat;
  const int blocks = bat.capacity / 3;

  if (state.redraw) {
    const char *block_string = "█████████████████████████████████████\0";
    const char *empty_string = "                                     \0";
    char fill_blocks[150], empty_blocks[50];

    strncpy(fill_blocks, block_string, (blocks + 1) * 3);
    fill_blocks[(blocks + 1) * 3] = '\0';
    strncpy(empty_blocks, empty_string, MAX_BLOCKS_BIG - blocks);
    empty_blocks[MAX_BLOCKS_BIG - blocks] = '\0';

    printf("\033[%d;%dH%s████████████████████████████████████████",
           state.start_row, state.start_col, colors.shell);

    for (int i = 1; i <= core_rows; i++) {
      printf("\033[%d;%dH", state.start_row + i, state.start_col);
      printf("%s██", colors.shell);
      printf("%s%s%s", state.inner_color, fill_blocks, empty_blocks);
      printf("%s████", colors.shell);

      if (i > 1 && i < core_rows) {
        printf("████");
      }
    }

    printf("\033[%d;%dH████████████████████████████████████████",
           state.start_row + core_rows + 1, state.start_col);

    if (!flags.fat && flags.live) {
      printf("\033[%d;%dH    \033[%d;%dH                                       "
             "      \r\n",
             state.start_row + core_rows, state.start_col + 40,
             state.start_row + core_rows + 2, state.start_col);
    }
    state.redraw = false;
  } else {
    print_col(blocks, core_rows);
  }
}

void define_position(void) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  state.term_rows = w.ws_row;
  state.term_cols = w.ws_col;

  if (flags.inlin || flags.fetch) {
    char buf[2];
    int i = 0;
    char c = '\0';

    struct termios term, restore;

    tcgetattr(0, &term);
    tcgetattr(0, &restore);
    tcsetattr(0, TCSANOW, &term);
    term.c_lflag &= ~(ICANON | ECHO);

    write(1, "\033[6n", 4);
    for (c = 0; c != 'R'; read(0, &c, 1)) {
      if (c >= '0' && c <= '9' && i < 2) {
        buf[i] = c;
        i++;
      } else if (c == ';') {
        i = 99;
      }
    }

    tcsetattr(0, TCSANOW, &restore);

    state.start_row = atoi(buf) + 1;
    const int min_rows = 10 + flags.fat;
    if (state.term_rows - state.start_row < min_rows) {
      for (int j = 0; j < min_rows; j++) {
        printf("\r\n");
      }
      state.start_row = state.term_rows - min_rows + 1;
    }

    state.start_col = 3;
  } else {
    printf("\033[2J");
    state.start_row = state.term_rows / 2 - 3;
    state.start_col = state.term_cols / 2 - 21;
  }
}

void update_state(void) {
  static int prev_digits = 0;
  static const char *prev_inner_color = "";

  if (bat.capacity < 20) {
    state.inner_color = colors.low;
  } else if (bat.capacity < 60) {
    state.inner_color = colors.mid;
  } else {
    state.inner_color = colors.high;
  }

  int data = 0;
  if (flags.mode == capacity) {
    data = bat.capacity;
  } else if (flags.mode == power) {
    data = bat.power;

    if (flags.extra_colors == true) {
      if (bat.is_charging == true)
        state.inner_color = colors.full;
      else
        state.inner_color = colors.left;
    }
  } else if (flags.mode == temperature) {
    data = bat.temp;

    if (flags.extra_colors == true) {
      state.inner_color = colors.temp;
    }
  } else if (flags.mode == health) {
    data = bat.health;

    if (flags.extra_colors == true) {
      state.inner_color = colors.health;
    }
  } else if (flags.mode == time_m) {
    data = bat.time;

    if (flags.extra_colors == true) {
      if (bat.is_charging == true)
        state.inner_color = colors.full;
      else
        state.inner_color = colors.left;
    }
  }

  if (flags.colors == false) {
    state.inner_color = "\033[0m";
  }

  if (prev_digits != count_digits(data)) {
    state.redraw = true;
  }
  prev_digits = count_digits(data);

  if (state.inner_color != prev_inner_color) {
    state.redraw = true;
  }
  prev_inner_color = state.inner_color;
}

void print_big(bool redefine) {
  if (redefine == true) {
    define_position();
  }

  update_state();
  print_bat();
  print_charge();
  print_tech();
  print_number(state.start_row + 2);

  if (flags.inlin || flags.fetch) {
    printf("\033[%d;0H", state.start_row - 1);
  }

  if (flags.fetch && !flags.live) {
    printf("\033[1B");
    print_minimal(LEFTPAD_BIG - (bat.is_charging ? 0 : CHARGE_SIZE_BIG));
  }
  printf("\r\n");
}

void print_small_bat_row(bool top) {
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
    printf("               \r\n");
  } else if (top) {
    printf("   %s▄▄▄\r\n", colors.charge);
  } else {
    printf("  %s▀▀▀\r\n", colors.charge);
  }
}

void print_small_bat(void) {
  printf("\n  %s██████████████████\r\n", colors.shell);
  print_small_bat_row(true);
  print_small_bat_row(false);
  printf("  %s██████████████████\n", colors.shell);
}

void print_small(void) {
  update_state();
  print_small_bat();

  printf("\r\033[5F");
  if (flags.fetch && !flags.live) {
    print_minimal(LEFTPAD_SMALL - (bat.is_charging ? 0 : CHARGE_SIZE_SMALL));
  }
}

void print_minimal(int padding) {
  bat_status(1);
  char color_pad[50];
  const char *key_color = NULL;

  if (padding == 0) {
    key_color = "\033[0m";
  } else {
    key_color = state.inner_color;
  }

  snprintf(color_pad, 50, "\033[%dC%s", padding, key_color);
  printf("%s%s\033[1m%s\033[22m\033[0m\r\n", color_pad, colors.charge,
         bat_name(flags.bat_number));

  printf("%sBattery\033[0m:       %d%%\r\n", color_pad, bat.capacity);
  printf("%sHealth\033[0m:        %d%%\r\n", color_pad, bat.health);
  printf("%sTemperature\033[0m:   %d°C\r\n", color_pad, bat.temp);
  printf("%sTechnology\033[0m:    %s\r", color_pad, bat.tech);

  if (bat.is_charging) {
    printf("%sCharging\033[0m:      True\r\n", color_pad);
    printf("%sPower In\033[0m:      %dW\r\n", color_pad, bat.power);
    printf("%sTime to Full\033[0m:  %dH %dM\r\n", color_pad, bat.time / 60,
           bat.time % 60);
  } else {
    printf("%sCharging\033[0m:      False\r\n", color_pad);
    printf("%sPower Draw\033[0m:    %dW\r\n", color_pad, bat.power);
    printf("%sTime Left\033[0m:     %dH %dM\r\n", color_pad, bat.time / 60,
           bat.time % 60);
  }
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
       "  -F, --fetch                      Adds extra information beside the "
       "battery\r\n"
       "  -d, --digits                     Draw the current capacity as a "
       "number in the battery\r\n"
       "  -m, --mode MODE                  Specify what to be "
       "printed with -d (capacity, power, temperature or health)\r\n"
       "  -e, --extra-colors               Disable extra core color patterns "
       "for different modes\r\n"
       "  -m, --minimal                    Minimal print of the battery "
       "status\r\n"
       "  -c, --alt-charge                 Use alternate charging symbol "
       "(requires nerd fonts)\r\n"
       "  -n, --no-color                   Disable colors\r\n"
       "  -b, --bat-number BAT_NUMBER      Specify battery number");
}
