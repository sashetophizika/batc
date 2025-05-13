#include <dirent.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "print.h"
#include "state.h"
#include "status.h"
#include "utils.h"

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
    negate = blocks + start_col + 3 - col;
    digit_color = inner_color;
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

  int col = start_col + 4 * digits + 12;
  for (int i = 0; i < digits; i++) {
    print_digit(flags.digits ? data % 10 : 42, row, col - 8 * i);
    data /= 10;
  }
}

void print_tech(void) {
  printf("%s\033[%d;%dH", colors.tech, start_row + 2, start_col - 16);
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
  printf("%s\033[%d;%dH", colors.charge, start_row + 3, start_col + 47);
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

void print_col(int core_rows) {
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
    col = start_col + prev_blocks + 2 + i;
    printf("%s\033[%d;%dH", inner_color, start_row + 1, col);

    for (int j = 0; j < core_rows; j++) {
      printf("%s\033[1B\033[1D", new_sym);
    }
  }
}

void print_bat(void) {
  const int core_rows = 6 + flags.fat;

  if (redraw) {
    const char *block_string = "█████████████████████████████████████\0";
    const char *empty_string = "                                     \0";
    char fill_blocks[150], empty_blocks[50];

    strncpy(fill_blocks, block_string, (blocks + 1) * 3);
    fill_blocks[(blocks + 1) * 3] = '\0';
    strncpy(empty_blocks, empty_string, MAX_BLOCKS_BIG - blocks);
    empty_blocks[MAX_BLOCKS_BIG - blocks] = '\0';

    printf("\033[%d;%dH%s████████████████████████████████████████", start_row,
           start_col, colors.shell);

    for (int i = 1; i <= core_rows; i++) {
      printf("\033[%d;%dH%s██%s%s%s%s████", start_row + i, start_col,
             colors.shell, inner_color, fill_blocks, empty_blocks,
             colors.shell);

      if (i > 1 && i < core_rows) {
        printf("████");
      }
    }

    printf("\033[%d;%dH████████████████████████████████████████",
           start_row + core_rows + 1, start_col);

    if (!flags.fat && flags.live) {
      printf("\033[%d;%dH    \033[%d;%dH                                       "
             "      \r\n",
             start_row + core_rows, start_col + 40, start_row + core_rows + 2,
             start_col);
    }
    redraw = false;
  } else {
    if (blocks != prev_blocks) {
      print_col(core_rows);
    }
  }
  prev_blocks = blocks;
}

void define_position(void) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  rows = w.ws_row;
  cols = w.ws_col;

  if (flags.inlin) {
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

    start_row = atoi(buf) + 1;
    const int min_rows = 10 + flags.fat;
    if (rows - start_row < min_rows) {
      for (int j = 0; j < min_rows; j++) {
        printf("\r\n");
      }
      start_row = rows - min_rows + 1;
    }

    start_col = 3;
  } else {
    printf("\033[2J");
    start_row = rows / 2 - 3;
    start_col = cols / 2 - 21;
  }
}

void update_state(void) {
  if (bat.capacity < 20) {
    inner_color = colors.low;
  } else if (bat.capacity < 60) {
    inner_color = colors.mid;
  } else {
    inner_color = colors.high;
  }

  int data = 0;
  if (flags.mode == capacity) {
    data = bat.capacity;
  } else if (flags.mode == power) {
    data = bat.power;

    if (flags.extra_colors == true) {
      if (bat.is_charging == true)
        inner_color = colors.full;
      else
        inner_color = colors.left;
    }
  } else if (flags.mode == temperature) {
    data = bat.temp;

    if (flags.extra_colors == true) {
      inner_color = colors.temp;
    }
  } else if (flags.mode == health) {
    data = bat.health;

    if (flags.extra_colors == true) {
      inner_color = colors.health;
    }
  } else if (flags.mode == time_m) {
    data = bat.time;

    if (flags.extra_colors == true) {
      if (bat.is_charging == true)
        inner_color = colors.full;
      else
        inner_color = colors.left;
    }
  }

  if (flags.colors == false) {
    inner_color = "\033[0m";
  }

  if (prev_digits != count_digits(data)) {
    redraw = true;
  }
  prev_digits = count_digits(data);

  if (inner_color != prev_inner_color) {
    redraw = true;
  }
  prev_inner_color = inner_color;
}

void print_big(bool redefine) {
  if (redefine == true) {
    define_position();
  }

  update_state();
  print_bat();
  print_charge();
  print_tech();
  print_number(start_row + 2);

  if (flags.inlin) {
    printf("\033[%d;0H", start_row - 1);
  }
  printf("\r\n");
}

void print_small_bat_row(bool top) {
  printf("  %s██%s", colors.shell, inner_color);
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
  printf("\033[5F");
}

void print_minimal(void) {
  bat_status(1);
  printf("Battery:       %d%%\r\nHealth:        "
         "%d%%\r\nTemperature:   %d°C\r\nTechnology:    "
         "%s\r",
         bat.capacity, bat.health, bat.temp, bat.tech);
  if (bat.is_charging) {
    printf("Power In:      %dW\r\nTime to Full:  %dH %dM\r\nCharging:      "
           "True\r\n",
           bat.power, bat.time / 60, bat.time % 60);
  } else {
    printf("Power Draw:    %dW\r\nTime Left:     %dH %dM\r\nCharging:      "
           "False\r\n",
           bat.power, bat.time / 60, bat.time % 60);
  }
}
