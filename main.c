#include <complex.h>
#include <getopt.h>
#include <locale.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

char *COLOR_100P = "\033[0;32m\0";
char *COLOR_60P = "\033[0;33m\0";
char *COLOR_20P = "\033[0;31m\0";
char *COLOR_TEMP = "\033[0;35m\0";
char *COLOR_TIME_FULL = "\033[0;36m\0";
char *COLOR_TIME_LEFT = "\033[0;34m\0";
char *COLOR_SHELL = "\033[0m\0";
char *COLOR_CHARGE = "\033[0;36m\0";

int COLORS = 1;
int LIVE = 0;
int MINIMAL = 0;
int SMALL = 0;
int DIGITS = 0;
int FAT = 0;
int ALT_CHARGE = 0;
int EXTRA_COLORS = 1;
char MODE = 'c';
char BAT_NUMBER[50];

int NEWL = 0;
int INDENT = 0;
int ROWS = 0;
int COLS = 0;

int PREVIOUS_BAT = 0;
int PREVIOUS_TEMP = 0;
int PREVIOUS_POWER = 0;
int PREVIOUS_CHARGING = 1;

int BAT = 0;
int TEMP = 0;
int POWER = 0;
int CHARGING = 0;

int REDRAW = 0;
int BLOCKS = 0;

char POLARITY = '-';
char *BATTERY_COLOR = "";
char *PREVIOUS_BATTERY_COLOR = "";

int PREVIOUS_NUM_LENGTH = 0;
int PREVIOUS_BLOCKS = 0;

void toggle(int *x) {
  if (*x)
    *x = 0;
  else
    *x = 1;
}

int digit_count(int num) {
  int count = 0;
  do {
    num /= 10;
    count++;
  } while (num != 0);

  return count;
}

char *color_to_ansi(char *color) {
  if (strcmp(color, "none\n") == 0)
    return "\e[0m\0";
  else if (strcmp(color, "black\n") == 0)
    return "\e[0;30m\0";
  else if (strcmp(color, "red\n") == 0)
    return "\e[0;31m\0";
  else if (strcmp(color, "green\n") == 0)
    return "\e[0;32m\0";
  else if (strcmp(color, "yellow\n") == 0)
    return "\e[0;33m\0";
  else if (strcmp(color, "blue\n") == 0)
    return "\e[0;34m\0";
  else if (strcmp(color, "magenta\n") == 0)
    return "\e[0;35m\0";
  else if (strcmp(color, "cyan\n") == 0)
    return "\e[0;36m\0";
  else if (strcmp(color, "white\n") == 0)
    return "\e[0;37m\0";
  else if (color[0] == '#') {
    char r[10];
    char g[10];
    char b[10];
    strncpy(r, color + 1, 2);
    strncpy(g, color + 3, 2);
    strcpy(b, color + 5);

    char *a = malloc(100);
    sprintf(a, "\e[38;2;%ld;%ld;%ldm", strtol(r, NULL, 16), strtol(g, NULL, 16),
            strtol(b, NULL, 16));
    return a;
  }

  return color;
}

char *get_param(char *param) {
  char *line = NULL;
  size_t len = 0;
  char fn[100];
  snprintf(fn, 100, "%s/%s", BAT_NUMBER, param);

  // if (!strcmp(param, "capacity"))
  //   strcpy(fn, "/home/sasho/testbat/capacity");

  FILE *fp = fopen(fn, "r");
  if (fp == NULL) {
    return "0";
  }

  getline(&line, &len, fp);

  fclose(fp);
  return line;
}

void bat_status(int full) {
  PREVIOUS_BAT = BAT;
  PREVIOUS_TEMP = TEMP;
  PREVIOUS_POWER = POWER;
  PREVIOUS_CHARGING = CHARGING;

  BAT = atoi(get_param("capacity"));
  char *status = get_param("status");

  if (MODE == 't' || full) {
    TEMP = atoi(get_param("temp")) / 10;
  }

  if (!strcmp(status, "Discharging\n") || !strcmp(status, "Not charging\n")) {
    CHARGING = 0;
    POLARITY = '-';
  } else {
    CHARGING = 1;
    POLARITY = '+';
  }

  if (MODE == 'm' || full) {
    int charge_now = atoi(get_param("charge_now"));
    int charge_full = atoi(get_param("charge_full"));
    int current_now = atoi(get_param("current_now"));

    if (CHARGING == 1) {
      POWER = (charge_full - charge_now) * 60 / current_now;
    } else {
      POWER = charge_now * 60 / current_now;
    }
  }

  BLOCKS = BAT / 3;
}

void update_state() {
  if (BAT < 20)
    BATTERY_COLOR = COLOR_20P;
  else if (BAT < 60)
    BATTERY_COLOR = COLOR_60P;
  else
    BATTERY_COLOR = COLOR_100P;

  int data;
  switch (MODE) {
  case 'c':
    data = BAT;
    break;
  case 'm':
    data = POWER;

    if (EXTRA_COLORS == 1) {
      if (CHARGING == 1)
        BATTERY_COLOR = COLOR_TIME_FULL;
      else
        BATTERY_COLOR = COLOR_TIME_LEFT;
    }
    break;
  case 't':
    data = TEMP;

    if (EXTRA_COLORS == 1)
      BATTERY_COLOR = COLOR_TEMP;
  }

  if (COLORS == 0)
    BATTERY_COLOR = "\e[0m";

  if (PREVIOUS_NUM_LENGTH != digit_count(data))
    REDRAW = 0;
  PREVIOUS_NUM_LENGTH = digit_count(data);

  if (BATTERY_COLOR != PREVIOUS_BATTERY_COLOR)
    REDRAW = 0;
  PREVIOUS_BATTERY_COLOR = BATTERY_COLOR;
}

void print_digit(int digit, int row, int col, int negate) {
  char full[1000];
  char empty[1000];
  char *chars;

  switch (digit) {
  case 0:
    snprintf(full, 1000,
             "██████\e[1B\e[%1$dG██  ██\e[1B\e[%1$dG██  ██\e[1B\e[%1$dG██  "
             "██\e[1B\e[%1$dG██████",
             col);
    snprintf(empty, 1000,
             "      \e[1B\033[%1$dG  ██  \e[1B\e[%1$dG  ██  \e[1B\e[%1$dG  ██  "
             "\e[1B\e[%1$dG      ",
             col);
    chars = "111111110011110011110011111111";
    break;
  case 1:
    snprintf(full, 1000,
             "████  \e[1B\e[%1$dG  ██  \e[1B\e[%1$dG  ██  \e[1B\e[%1$dG  ██  "
             "\e[1B\e[%1$dG██████",
             col);
    snprintf(empty, 1000,
             "    ██\e[1B\e[%1$dG██  ██\e[1B\e[%1$dG██  ██\e[1B\e[%1$dG██  "
             "██\e[1B\e[%1$dG      ",
             col);
    chars = "111100001100001100001100111111";
    break;
  case 2:
    snprintf(full, 1000,
             "██████\e[1B\e[%1$dG    ██\e[1B\e[%1$dG██████\e[1B\e[%1$dG██    "
             "\e[1B\e[%1$dG██████",
             col);
    snprintf(empty, 1000,
             "      \e[1B\e[%1$dG████  \e[1B\e[%1$dG      \e[1B\e[%1$dG  "
             "████\e[1B\e[%1$dG      ",
             col);
    chars = "111111000011111111110000111111";
    break;
  case 3:
    snprintf(full, 1000,
             "██████\e[1B\e[%1$dG    ██\e[1B\e[%1$dG██████\e[1B\e[%1$dG    "
             "██\e[1B\e[%1$dG██████",
             col);
    snprintf(empty, 1000,
             "      \e[1B\e[%1$dG████  \e[1B\e[%1$dG      \e[1B\e[%1$dG████  "
             "\e[1B\e[%1$dG      ",
             col);
    chars = "111111000011111111000011111111";
    break;
  case 4:
    snprintf(full, 1000,
             "██  ██\e[1B\e[%1$dG██  ██\e[1B\e[%1$dG██████\e[1B\e[%1$dG    "
             "██\e[1B\e[%1$dG    ██",
             col);
    snprintf(empty, 1000,
             "  ██  \e[1B\e[%1$dG  ██  \e[1B\e[%1$dG      \e[1B\e[%1$dG████  "
             "\e[1B\e[%1$dG████  ",
             col);
    chars = "110011110011111111000011000011";
    break;
  case 5:
    snprintf(full, 1000,
             "██████\e[1B\e[%1$dG██    \e[1B\e[%1$dG██████\e[1B\e[%1$dG    "
             "██\e[1B\e[%1$dG██████",
             col);
    snprintf(empty, 1000,
             "      \e[1B\e[%1$dG  ████\e[1B\e[%1$dG      \e[1B\e[%1$dG████  "
             "\e[1B\e[%1$dG      ",
             col);
    chars = "111111110000111111000011111111";
    break;
  case 6:
    snprintf(full, 1000,
             "██████\e[1B\e[%1$dG██    \e[1B\e[%1$dG██████\e[1B\e[%1$dG██  "
             "██\e[1B\e[%1$dG██████",
             col);
    snprintf(empty, 1000,
             "      \e[1B\e[%1$dG  ████\e[1B\e[%1$dG      \e[1B\e[%1$dG  ██  "
             "\e[1B\e[%1$dG      ",
             col);
    chars = "111111110000111111110011111111";
    break;
  case 7:
    snprintf(full, 1000,
             "██████\e[1B\e[%1$dG    ██\e[1B\e[%1$dG    ██\e[1B\e[%1$dG    "
             "██\e[1B\e[%1$dG    ██",
             col);
    snprintf(empty, 1000,
             "      \e[1B\e[%1$dG████  \e[1B\e[%1$dG████  \e[1B\e[%1$dG████  "
             "\e[1B\e[%1$dG████  ",
             col);
    chars = "111111000011000011000011000011";
    break;
  case 8:
    snprintf(full, 1000,
             "██████\e[1B\e[%1$dG██  ██\e[1B\e[%1$dG██████\e[1B\e[%1$dG██  "
             "██\e[1B\e[%1$dG██████",
             col);
    snprintf(empty, 1000,
             "      \e[1B\e[%1$dG  ██  \e[1B\e[%1$dG      \e[1B\e[%1$dG  ██  "
             "\e[1B\e[%1$dG      ",
             col);
    chars = "111111110011111111110011111111";
    break;
  case 9:
    snprintf(full, 1000,
             "██████\e[1B\e[%1$dG██  ██\e[1B\e[%1$dG██████\e[1B\e[%1$dG    "
             "██\e[1B\e[%1$dG██████",
             col);
    snprintf(empty, 1000,
             "      \e[1B\e[%1$dG  ██  \e[1B\e[%1$dG      \e[1B\e[%1$dG████  "
             "\e[1B\e[%1$dG      ",
             col);
    chars = "111111110011111111000011111111";
    break;
  default:
    snprintf(full, 1000,
             "      \e[1B\e[%1$dG      \e[1B\e[%1$dG      \e[1B\e[%1$dG      "
             "\e[1B\e[%1$dG      ",
             col);
    snprintf(empty, 1000,
             "██████\e[1B\e[%1$dG██████\e[1B\e[%1$dG██████\e[1B\e[%1$"
             "dG██████\e[1B\e[$"
             "3G██████",
             col);
    chars = "000000000000000000000000000000";
  }

  char num[1000];
  snprintf(num, 200, "%s\033[%d;%dH", BATTERY_COLOR, row, col);

  if (negate == 0) {
    printf("%s%s", num, full);
  } else if (negate >= 6)
    printf("%s%s", num, empty);
  else {
    char temp[100] = {};
    for (int i = 0; i < 30; i++) {
      if (i % 6 < negate) {
        if (chars[i] == '1')
          strcat(num, " ");
        else
          strcat(num, "█");
      } else {
        if (chars[i] == '1')
          strcat(num, "█");
        else
          strcat(num, " ");
      }

      if (i % 6 == 5) {
        memset(temp, 0, sizeof(temp));
        sprintf(temp, "\033[%d;%dH", row + (i + 1) / 6, col);
        strcat(num, temp);
      }
    }
    printf("%s", num);
  }
}

void print_number(int row) {
  int data;
  switch (MODE) {
  case 'c':
    data = BAT;
    break;
  case 'm':
    data = POWER;
    break;
  case 't':
    data = TEMP;
    break;
  }

  int full_cols = BLOCKS + 4;
  int blocks1 = 0;
  int blocks2 = 0;
  int blocks3 = 0;

  switch (digit_count(data)) {
  case 1:
    if (full_cols > 16)
      blocks1 = full_cols - 17;
    print_digit(data, row, INDENT + 16, blocks1);
    break;
  case 2:
    if (full_cols > 12) {
      blocks1 = full_cols - 13;
      if (full_cols > 20)
        blocks2 = full_cols - 21;
    }

    print_digit(data / 10, row, INDENT + 12, blocks1);
    print_digit(data % 10, row, INDENT + 20, blocks2);
    break;
  case 3:
  default:
    if (full_cols > 8) {
      blocks1 = full_cols - 9;
      if (full_cols > 16) {
        blocks2 = full_cols - 17;
        if ((full_cols > 24))
          blocks3 = full_cols - 25;
      }
    }

    print_digit(data % 1000 / 100, row, INDENT + 8, blocks1);
    print_digit(data % 100 / 10, row, INDENT + 16, blocks2);
    print_digit(data % 10, row, INDENT + 24, blocks3);
    break;
  }
}

void print_charge() {
  char symbol[2000] = "";
  if (FAT) {
    if (CHARGING) {
      if (ALT_CHARGE) {
        snprintf(symbol, 2000,
                 "\033[%d;%dH   %s███████  \033[%d;%dH    "
                 "██ "
                 " "
                 "\033[%d;%dH███████  ",
                 NEWL + 3, INDENT + 47, COLOR_CHARGE, NEWL + 4, INDENT + 47,
                 NEWL + 5, INDENT + 47);
      } else {
        snprintf(symbol, 2000,
                 "\033[%d;%dH    %s███████  \033[%d;%dH    "
                 "███"
                 " "
                 "\033[%d;%dH███████  ",
                 NEWL + 3, INDENT + 47, COLOR_CHARGE, NEWL + 4, INDENT + 47,
                 NEWL + 5, INDENT + 47);
      }
    } else {
      snprintf(symbol, 2000,
               "\033[%d;%dH                \033[%d;%dH         "
               " "
               "\033[%d;%dH           ",
               NEWL + 3, INDENT + 47, NEWL + 4, INDENT + 47, NEWL + 5,
               INDENT + 47);
    }
  } else {
    if (CHARGING) {
      if (ALT_CHARGE)
        snprintf(symbol, 2000,
                 "\033[%d;%dH   %s██████  "
                 "\033[%d;%dH██████ "
                 " ",
                 NEWL + 3, INDENT + 47, COLOR_CHARGE, NEWL + 4, INDENT + 47);
      else
        snprintf(symbol, 2000,
                 "\033[%d;%dH   %s████████  "
                 "\033[%d;%dH████████ "
                 " ",
                 NEWL + 3, INDENT + 47, COLOR_CHARGE, NEWL + 4, INDENT + 47);
    } else {
      snprintf(symbol, 2000,
               "\033[%d;%dH  %s          "
               "\033[%d;%dH        "
               " ",
               NEWL + 3, INDENT + 47, COLOR_CHARGE, NEWL + 4, INDENT + 47);
    }
  }
  printf("%s", symbol);
}

void print_col(int rows) {
  int diff = BLOCKS - PREVIOUS_BLOCKS;
  int step;
  int start;
  int end;
  char *new_sym;

  if (diff > 0) {
    step = 1;
    end = diff + 1;
    start = 1;
    new_sym = "█";
  } else {
    step = -1;
    start = 0;
    end = diff;
    new_sym = " ";
  }

  printf("\e[7;0H%d", BLOCKS);
  printf("\e[8;0H%d", PREVIOUS_BLOCKS);
  printf("\e[9;0H%d", diff);
  printf("\e[10;0H%d", start);
  printf("\e[11;0H%d", end);
  printf("\e[12;0H%d", step);

  for (int i = start; step * i < step * end; i += step) {
    int index = INDENT + PREVIOUS_BLOCKS + 2 + i;
    char col[100];
    snprintf(col, 100, "%s\e[%d;%dH", BATTERY_COLOR, NEWL + 1, index);

    for (int j = 0; j < rows; j++) {
      char buffer[20];
      snprintf(buffer, 20, "%s\e[1B\e[1D", new_sym);
      strcat(col, buffer);
    }
    printf("%s\r\n", col);
  }
}

void print_bat() {
  int core_rows;
  int cap[8];

  if (FAT) {
    core_rows = 7;
    memcpy(cap, (int[]){0, 12, 24, 24, 24, 24, 24, 12}, sizeof cap);
  } else {
    core_rows = 6;
    memcpy(cap, (int[]){0, 12, 24, 24, 24, 24, 12, 0}, sizeof cap);
  }

  if (!REDRAW) {
    char *block_string = "█████████████████████████████████████\0";
    char *empty_string = "                                     \0";
    char fill[150];
    char empty[50];
    char cap_size[200];

    printf("\033[%d;%dH%s████████████████████████████████████████\n", NEWL,
           INDENT, COLOR_SHELL);

    strncpy(fill, block_string, (BLOCKS + 1) * 3);
    fill[(BLOCKS + 1) * 3] = '\0';
    strncpy(empty, empty_string, 33 - BLOCKS);
    empty[33 - BLOCKS] = '\0';

    for (int i = 1; i <= core_rows; i++) {
      memset(cap_size, 0, sizeof(cap_size));
      strncpy(cap_size, block_string, cap[i]);
      cap_size[cap[i]] = '\0';

      printf("\033[%d;%dH%s██%s%s%s%s%s", NEWL + i, INDENT, COLOR_SHELL,
             BATTERY_COLOR, fill, empty, COLOR_SHELL, cap_size);
    }

    printf("\033[%d;%dH%s████████████████████████████████████████\n",
           NEWL + core_rows + 1, INDENT, COLOR_SHELL);

    PREVIOUS_BLOCKS = BLOCKS;
    REDRAW = 1;
  } else {
    if (BLOCKS != PREVIOUS_BLOCKS)
      print_col(core_rows);
    PREVIOUS_BLOCKS = BLOCKS;
  }
}

void big_loop(int opt) {
  if (opt == 1) {
    printf("\e[2J");
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    ROWS = w.ws_row;
    COLS = w.ws_col;
    NEWL = ROWS / 2 - 3;
    INDENT = COLS / 2 - 24;
  }

  update_state();
  print_bat();
  print_charge();

  if (DIGITS)
    print_number(NEWL + 2);
  printf("\r\n");
}

void print_small_bat_row() {
  BLOCKS = BAT / 7;
  int i = 0;

  printf("%s██%s", COLOR_SHELL, BATTERY_COLOR);
  while (i < 14) {
    if (i < BLOCKS)
      printf("█");
    else
      printf(" ");

    i++;
  }
  printf("%s████▊", COLOR_SHELL);
}

void print_small_bat() {
  printf("\n%s███████████████████\r\n", COLOR_SHELL);

  print_small_bat_row();

  if (CHARGING)
    printf("   %s▄▄▄\r\n", COLOR_CHARGE);
  else
    printf("               \r\n");

  print_small_bat_row();

  if (CHARGING)
    printf("  %s▀▀▀\r\n", COLOR_CHARGE);
  else
    printf("            \r\n");

  printf("%s███████████████████\n", COLOR_SHELL);
}

void small_loop() {
  update_state();
  print_small_bat();
  printf("\e[5F");
}

void main_loop(int opt) {
  if (SMALL)
    small_loop();
  else
    big_loop(opt);
}

int handle_input(char c) {
  switch (c) {
  case 'd':
    toggle(&DIGITS);
    REDRAW = 0;
    bat_status(0);
    main_loop(0);
    break;
  case 'f':
    toggle(&FAT);
    REDRAW = 0;
    bat_status(0);
    main_loop(1);
    break;
  case 'c':
    toggle(&ALT_CHARGE);
    bat_status(0);
    print_charge();
    break;
  case 't':
    if (MODE == 'c')
      MODE = 't';
    else if (MODE == 't')
      MODE = 'm';
    else if (MODE == 'm')
      MODE = 'c';
    REDRAW = 0;
    bat_status(0);
    main_loop(0);
    break;
  // case 'a': {
  //   char b[100];
  //   snprintf(b, 100, "echo %d > ~/testbat/capacity", BAT - 1);
  //   system(b);
  //   break;
  // }
  // case 's': {
  //   char b[100];
  //   snprintf(b, 100, "echo %d > ~/testbat/capacity", BAT + 1);
  //   system(b);
  //   break;
  // }
  case 'q':
  case 27:
    exit(0);
  }
  return 1;
}

void *event_loop() {
  char c = '0';
  while (handle_input(c)) {
    c = getchar();
  }

  return NULL;
}

void print_help() {
  puts("Usage:\r\n"
       "  battery [-lsmbdfn]\r\n"
       "\r\n"
       "OPTIONS\r\n"
       "  -l, --live                       Monitor the battery live\r\n"
       "  -s, --small                      Print a small version of the "
       "battery\r\n"
       "  -f, --fat                        Draws a slightly thicker battery\r\n"
       "  -d, --digits                     Prints the current capacity as a "
       "number in the "
       "battery\r\n"
       "  -M, --mode MODE                  Specify the mode to be printed with "
       "-d (c for "
       "capacity, m for time, t for temperature)\r\n"
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

void handle_flags(int argc, char **argv) {
  static struct option long_options[] = {
      {"mode", required_argument, NULL, 'M'},
      {"bat-number", required_argument, NULL, 'b'},
      {"fat", no_argument, NULL, 'f'},
      {"no-color", no_argument, NULL, 'n'},
      {"digits", no_argument, NULL, 'd'},
      {"alt-charge", no_argument, NULL, 'c'},
      {"live", no_argument, NULL, 'l'},
      {"minimal", no_argument, NULL, 'm'},
      {"small", no_argument, NULL, 's'},
      {"help", no_argument, NULL, 'h'},
  };
  char opt;
  while ((opt = getopt_long(argc, argv, ":hnlmtM:csdfb:", long_options,
                            NULL)) != -1) {
    switch (opt) {
    case 'n':
      toggle(&COLORS);
      break;
    case 'l':
      toggle(&LIVE);
      break;
    case 'm':
      toggle(&MINIMAL);
      break;
    case 's':
      toggle(&SMALL);
      break;
    case 'd':
      toggle(&DIGITS);
      break;
    case 'f':
      toggle(&FAT);
      break;
    case 'c':
      toggle(&ALT_CHARGE);
      break;
    case 'e':
      toggle(&EXTRA_COLORS);
      break;
    case 'M':
      MODE = optarg[0];
      break;
    case 'b': {
      char buffer[50];
      sprintf(buffer, "/sys/class/power_supply/BAT%s", optarg);
      strcpy(BAT_NUMBER, buffer);
    } break;
    case 'h':
      print_help();
      exit(0);
    }
  }
}

int bools_to_int(char *s) {
  if (!strcmp(s, "true\n")) {
    return 1;
  }
  return 0;
}

void parse_config() {
  char fn[100];
  char *home = getenv("HOME");
  snprintf(fn, 100, "%s/.config/batc/config", home);

  if (!access(fn, F_OK)) {
    FILE *fp = fopen(fn, "r");
    char *line = NULL;
    size_t len = 0;
    if (fp == NULL) {
      return;
    }

    char *key;
    char *val;
    while (getline(&line, &len, fp) != -1) {
      key = strtok(line, " =");
      val = strtok(NULL, " =");

      if (key == NULL || val == NULL)
        continue;

      if (!strcmp(key, "color_100p"))
        COLOR_100P = color_to_ansi(val);
      else if (!strcmp(key, "color_60p"))
        COLOR_60P = color_to_ansi(val);
      else if (!strcmp(key, "color_20p"))
        COLOR_20P = color_to_ansi(val);
      else if (!strcmp(key, "color_charge"))
        COLOR_CHARGE = color_to_ansi(val);
      else if (!strcmp(key, "color_shell"))
        COLOR_SHELL = color_to_ansi(val);
      else if (!strcmp(key, "color_temp"))
        COLOR_TEMP = color_to_ansi(val);
      else if (!strcmp(key, "color_time_full"))
        COLOR_TIME_FULL = color_to_ansi(val);
      else if (!strcmp(key, "color_time_left"))
        COLOR_TIME_LEFT = color_to_ansi(val);
      else if (!strcmp(key, "colors"))
        COLORS = bools_to_int(val);
      else if (!strcmp(key, "live"))
        LIVE = bools_to_int(val);
      else if (!strcmp(key, "digits"))
        DIGITS = bools_to_int(val);
      else if (!strcmp(key, "fat"))
        FAT = bools_to_int(val);
      else if (!strcmp(key, "small"))
        SMALL = bools_to_int(val);
      else if (!strcmp(key, "minimal"))
        MINIMAL = bools_to_int(val);
      else if (!strcmp(key, "alt_charge"))
        ALT_CHARGE = bools_to_int(val);
      else if (!strcmp(key, "extra_colors"))
        EXTRA_COLORS = bools_to_int(val);
      else if (!strcmp(key, "mode"))
        MODE = val[0];
    }
  }
}

void cleanup() {
  system("/bin/stty cooked");
  system("/bin/stty echo");
  if (SMALL) {
    printf("\e[?25h\e[5B");
  } else {

    if (LIVE) {
      printf("\e[?25h\e[?47l\e[u");
    } else {
      struct winsize w;
      ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
      char buffer[20];
      printf("\e[?25h\e[%d;0H", w.ws_row);
    }
  }
}

void setup() {
  system("/bin/stty raw");
  system("/bin/stty -echo");
  printf("\e[?25l");

  if (LIVE && !SMALL)
    printf("\e[?47h\e[s");

  strcpy(BAT_NUMBER, "/sys/class/power_supply/BAT0");
  parse_config();

  if (!COLORS) {
    COLOR_SHELL = "\e[0m";
    COLOR_CHARGE = "\e[0m";
  }
}

void minimal() {
  bat_status(1);
  printf("Battery: %d%%\r\n", BAT);
  printf("Temperature: %d°\r\n", TEMP);
  if (CHARGING)
    printf("Time to Full: %dh %dm\r\nCharging: True\r\n", POWER / 60,
           POWER % 60);
  else
    printf("Time Left: %dh %dm\r\nCharging: False\r\n", POWER / 60, POWER % 60);
  exit(0);
}

int main(int argc, char **argv) {
  handle_flags(argc, argv);

  setup();
  atexit(cleanup);

  if (MINIMAL) {
    minimal();
  }

  bat_status(0);
  main_loop(1);

  struct winsize w;
  if (LIVE) {
    pthread_t thread;
    pthread_create(&thread, NULL, event_loop, NULL);

    while (1) {
      usleep(200000);
      bat_status(0);

      if (!SMALL) {
        switch (MODE) {
        case 't':
          if (PREVIOUS_TEMP != TEMP)
            main_loop(0);
          break;
        case 'm':
          if (PREVIOUS_POWER != POWER)
            main_loop(0);
          break;
        }
      }
      if (PREVIOUS_BAT != BAT || PREVIOUS_CHARGING != CHARGING) {
        main_loop(0);
      }

      ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
      if (w.ws_row != ROWS || w.ws_col != COLS) {
        REDRAW = 0;
        main_loop(1);
      }
    }
  }
}
