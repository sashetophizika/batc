#include <complex.h>
#include <locale.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
int FAT = 1;
int ALT_CHARGE = 1;
int EXTRA_COLORS = 1;
char MODE = 'c';
char *BAT_NUMBER = "/sys/class/power_supply/BAT0";

int NEWL = 0;
int INDENT = 0;
int ROWS = 0;
int COLS = 0;

int BAT = 0;
int TEMP = 0;
int POWER = 0;
int CHARGING = 1;
int REDRAW = 0;
int BLOCKS = 0;
int FULL_COLS = 0;

char POLARITY = '-';
char *OLD_COLOR = "";
char *BATTERY_COLOR = "";

int OLD_NUM_LENGTH = 0;
int OLD_BLOCKS = 0;
int OLD_INDEX = 0;

void toggle(int *x) {
  if (*x)
    *x = 0;
  else
    *x = 1;
}

void hex_to_ansi(char **color) {
  if (strcmp(*color, "none") == 0)
    *color = "\e[0m\0";
  else if (strcmp(*color, "black") == 0)
    *color = "\e[0;30m\0";
  else if (strcmp(*color, "red") == 0)
    *color = "\e[0;31m\0";
  else if (strcmp(*color, "green") == 0)
    *color = "\e[0;32m\0";
  else if (strcmp(*color, "yellow") == 0)
    *color = "\e[0;33m\0";
  else if (strcmp(*color, "blue") == 0)
    *color = "\e[0;34m\0";
  else if (strcmp(*color, "magenta") == 0)
    *color = "\e[0;35m\0";
  else if (strcmp(*color, "cyan") == 0)
    *color = "\e[0;36m\0";
  else if (strcmp(*color, "white") == 0)
    *color = "\e[0;37m\0";
  else if (*color[0] == '#') {
    char r[10];
    char g[10];
    char b[10];
    strncpy(r, *color + 1, 2);
    strncpy(g, *color + 3, 2);
    strcpy(b, *color + 5);

    char a[100];
    sprintf(a, "\e[38;2;%ld;%ld;%ldm", strtol(r, NULL, 16), strtol(g, NULL, 16),
            strtol(b, NULL, 16));
    *color = a;
  }
}

char *get_param(char *param) {
  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  char fn[100];
  snprintf(fn, 100, "%s/%s", BAT_NUMBER, param);

  fp = fopen(fn, "r");
  if (fp == NULL) {
    return "0";
  }

  getline(&line, &len, fp);

  fclose(fp);
  return line;
}

void bat_status() {
  BAT = atoi(get_param("capacity"));
  char *status = get_param("status");

  if (MODE == 't') {
    TEMP = atoi(get_param("temp")) / 10;
  }

  if (!strcmp(status, "Discharging") || !strcmp(status, "Not charging")) {
    CHARGING = 0;
    POLARITY = '-';
  } else {
    CHARGING = 1;
    POLARITY = '+';
  }

  if (MODE == 'm') {
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
  FULL_COLS = BAT / 3 + 4;
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

int digit_count(int num) {
  int count = 0;
  while (num != 0) {
    num /= 10;
    count++;
  }
  return count;
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

  int blocks1 = 0;
  int blocks2 = 0;
  int blocks3 = 0;
  switch (digit_count(data)) {
  case 1:
    if (FULL_COLS > 16)
      blocks1 = FULL_COLS - 17;
    print_digit(data, row, INDENT + 16, blocks1);
    break;
  case 2:
    if (FULL_COLS > 12) {
      blocks1 = FULL_COLS - 13;
      if (FULL_COLS > 20)
        blocks2 = FULL_COLS - 21;
    }

    print_digit(data / 10, row, INDENT + 12, blocks1);
    print_digit(data % 10, row, INDENT + 20, blocks2);
    break;
  case 3:
  default:
    if (FULL_COLS > 8) {
      blocks1 = FULL_COLS - 9;
      if (FULL_COLS > 16) {
        blocks2 = FULL_COLS - 17;
        if ((FULL_COLS > 24))
          blocks3 = FULL_COLS - 25;
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
    char empty[35];
    char cap_size[200];

    printf("\033[%d;%dH%s████████████████████████████████████████\n", NEWL,
           INDENT, COLOR_SHELL);

    strncpy(fill, block_string, (BLOCKS + 1) * 3);
    fill[BLOCKS * 3 + 4] = '\0';
    strncpy(empty, empty_string, 33 - BLOCKS);
    empty[34 - BLOCKS] = '\0';

    for (int i = 1; i <= core_rows; i++) {
      memset(cap_size, 0, sizeof(cap_size));
      strncpy(cap_size, block_string, cap[i]);
      cap_size[cap[i]] = '\0';

      printf("\033[%d;%dH%s██%s%s%s%s%s", NEWL + i, INDENT, COLOR_SHELL,
             BATTERY_COLOR, fill, empty, COLOR_SHELL, cap_size);
    }

    printf("\033[%d;%dH%s████████████████████████████████████████\n",
           NEWL + core_rows + 1, INDENT, COLOR_SHELL);

    REDRAW = 1;
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
  print_number(NEWL + 2);
}

void main_loop(int opt) { big_loop(opt); }

int handle_input(char c) {
  switch (c) {
  case 'b':
    bat_status();
    break;
  case 'f':
    toggle(&FAT);
    big_loop(0);
    break;
  case 'a':
    toggle(&ALT_CHARGE);
    big_loop(0);
    break;
  case 'c':
    toggle(&CHARGING);
    big_loop(0);
    break;
  case 'p':
    printf("%d\n", BAT);
    printf("%d\n", TEMP);
    printf("%d\n", POWER);
    printf("%d\n", CHARGING);
    break;
  case 'q':
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

void cleanup() {
  system("/bin/stty cooked");
  system("/bin/stty echo");
}

int main() {
  system("/bin/stty raw");
  system("/bin/stty -echo");
  atexit(cleanup);

  char c = '0';
  bat_status();
  main_loop(1);
  pthread_t thread;
  pthread_create(&thread, NULL, event_loop, NULL);

  struct winsize w;

  while (1) {
    bat_status();
    if (!SMALL) {
      switch (MODE) {

      case 't':
        main_loop(0);
        break;
      case 'm':
        main_loop(0);
        break;
      }

      ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
      if (w.ws_row != ROWS || w.ws_col != COLS) {
        REDRAW = 0;
        main_loop(1);
      }
    }
  }
}
