#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct battery {
  int capacity;
  int charging;
  int temp;
  int power;
};

struct colors {
  char *high;
  char *mid;
  char *low;
  char *temp;
  char *full;
  char *left;
  char *charge;
  char *shell;
};

struct flags {
  int colors;
  int live;
  int minimal;
  int small;
  int digits;
  int fat;
  int alt_charge;
  int extra_colors;
  int inlin;
  char mode;
  char bat_number[50];
};

struct battery bat = {0, 0, 0, 0};
struct battery previous_bat = {0, 0, 0, 0};

struct colors color = {
    "\033[0;32m\0", "\033[0;33m\0", "\033[0;31m\0", "\033[0;35m\0",
    "\033[0;36m\0", "\033[0;34m\0", "\033[0;36m\0", "\033[0m\0",
};

struct flags flags = {1, 0, 0, 0, 0, 0, 0, 1, 0, 'c', ""};

int newl = 0;
int indent = 0;
int rows = 0;
int cols = 0;

int redraw = 0;
int blocks = 0;

char polarity = '-';
char *battery_color = "";
char *previous_battery_color = "";

int previous_num_length = 0;
int previous_blocks = 0;

void toggle(int *x) { *x = *x ? 0 : 1; }

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
    char r[3];
    char g[3];
    char b[3];

    strncpy(r, color + 1, 2);
    r[2] = '\0';
    strncpy(g, color + 3, 2);
    g[2] = '\0';
    strcpy(b, color + 5);
    b[2] = '\0';

    char *a = malloc(100);
    sprintf(a, "\e[38;2;%ld;%ld;%ldm", strtol(r, NULL, 16), strtol(g, NULL, 16),
            strtol(b, NULL, 16));
    return a;
  }

  return color;
}

void *get_param(char *param) {
  char *line = NULL;
  size_t len = 0;
  char fn[100];
  snprintf(fn, 100, "%s/%s", flags.bat_number, param);

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
  previous_bat.capacity = bat.capacity;
  previous_bat.temp = bat.temp;
  previous_bat.power = bat.power;
  previous_bat.charging = bat.charging;

  char *cap = get_param("capacity");
  bat.capacity = atoi(cap);

  if (flags.mode == 't' || full) {
    char *temp = get_param("temp");
    bat.temp = atoi(temp) / 10;
  }

  char *status = get_param("status");
  if (!strcmp(status, "Discharging\n") || !strcmp(status, "Not charging\n")) {
    bat.charging = 0;
    polarity = '-';
  } else {
    bat.charging = 1;
    polarity = '+';
  }

  if (flags.mode == 'm' || full) {
    char *cn = get_param("charge_now");
    char *cf = get_param("charge_full");
    char *cu = get_param("current_now");

    int charge_now = atoi(cn);
    int charge_full = atoi(cf);
    int current_now = atoi(cu);

    if (bat.charging == 1)
      bat.power = (charge_full - charge_now) * 60 / (current_now + 1);
    else
      bat.power = charge_now * 60 / (current_now + 1);
  }

  blocks = flags.small ? bat.capacity / 7 : bat.capacity / 3;
}

void update_state() {
  if (bat.capacity < 20)
    battery_color = color.low;
  else if (bat.capacity < 60)
    battery_color = color.mid;
  else
    battery_color = color.high;

  int data = 0;
  switch (flags.mode) {
  case 'c':
    data = bat.capacity;
    break;
  case 'm':
    data = bat.power;

    if (flags.extra_colors == 1) {
      if (bat.charging == 1)
        battery_color = color.full;
      else
        battery_color = color.left;
    }
    break;
  case 't':
    data = bat.temp;

    if (flags.extra_colors == 1)
      battery_color = color.temp;
  }

  if (flags.colors == 0)
    battery_color = "\e[0m";

  if (previous_num_length != digit_count(data))
    redraw = 0;
  previous_num_length = digit_count(data);

  if (battery_color != previous_battery_color)
    redraw = 0;
  previous_battery_color = battery_color;
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
             "3g██████",
             col);
    chars = "000000000000000000000000000000";
  }

  printf("%s\033[%d;%dH", battery_color, row, col);

  if (negate == 0) {
    printf("%s", full);
  } else if (negate >= 6)
    printf("%s", empty);
  else {
    char temp[100] = {};
    for (int i = 0; i < 30; i++) {
      if ((i % 6 < negate) != (chars[i] == '1'))
        printf("█");
      else
        printf(" ");

      if (i % 6 == 5) {
        printf("\033[%d;%dH", row + (i + 1) / 6, col);
      }
    }
  }
}

void print_number(int row) {
  int data = 0;
  switch (flags.mode) {
  case 'c':
    data = bat.capacity;
    break;
  case 'm':
    data = bat.power;
    break;
  case 't':
    data = bat.temp;
    break;
  }

  switch (digit_count(data)) {
  case 1:
    print_digit(data, row, indent + 16, blocks - 13);
    break;
  case 2:
    print_digit(data / 10, row, indent + 12, blocks - 9);
    print_digit(data % 10, row, indent + 20, blocks - 17);
    break;
  case 3:
  default:
    print_digit(data % 1000 / 100, row, indent + 8, blocks - 5);
    print_digit(data % 100 / 10, row, indent + 16, blocks - 13);
    print_digit(data % 10, row, indent + 24, blocks - 21);
    break;
  }
}

void print_charge() {
  if (flags.fat) {
    if (!bat.charging) {
      printf("\033[%d;%dH                \033[%d;%dH         "
             " "
             "\033[%d;%dH           ",
             newl + 3, indent + 47, newl + 4, indent + 47, newl + 5,
             indent + 47);
    } else {
      if (flags.alt_charge) {
        printf("\033[%d;%dH   %s███████  \033[%d;%dH    "
               "██ "
               " "
               "\033[%d;%dH███████  ",
               newl + 3, indent + 47, color.charge, newl + 4, indent + 47,
               newl + 5, indent + 47);
      } else {
        printf("\033[%d;%dH    %s███████  \033[%d;%dH    "
               "███"
               " "
               "\033[%d;%dH███████  ",
               newl + 3, indent + 47, color.charge, newl + 4, indent + 47,
               newl + 5, indent + 47);
      }
    }
  } else {
    if (!bat.charging) {
      printf("\033[%d;%dH  %s          "
             "\033[%d;%dH        "
             " ",
             newl + 3, indent + 47, color.charge, newl + 4, indent + 47);
    } else {
      if (flags.alt_charge)
        printf("\033[%d;%dH   %s██████  "
               "\033[%d;%dH██████ "
               " ",
               newl + 3, indent + 47, color.charge, newl + 4, indent + 47);
      else
        printf("\033[%d;%dH   %s████████  "
               "\033[%d;%dH████████ "
               " ",
               newl + 3, indent + 47, color.charge, newl + 4, indent + 47);
    }
  }
}

void print_col(int rows) {
  int diff = blocks - previous_blocks;
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

  for (int i = start; step * i < step * end; i += step) {
    int index = indent + previous_blocks + 2 + i;
    char col[100];
    printf("%s\e[%d;%dH", battery_color, newl + 1, index);

    for (int j = 0; j < rows; j++) {
      printf("%s\e[1B\e[1D", new_sym);
    }
    printf("\r\n");
  }
}

void print_bat() {
  int core_rows;
  int cap[8];

  if (flags.fat) {
    core_rows = 7;
    memcpy(cap, (int[]){0, 0, 1, 1, 1, 1, 1, 0}, sizeof cap);
  } else {
    core_rows = 6;
    memcpy(cap, (int[]){0, 0, 1, 1, 1, 1, 0, 0}, sizeof cap);
  }

  if (!redraw) {
    char *block_string = "█████████████████████████████████████\0";
    char *empty_string = "                                     \0";
    char fill[150];
    char empty[50];

    printf("\033[%d;%dH%s████████████████████████████████████████", newl,
           indent, color.shell);

    strncpy(fill, block_string, (blocks + 1) * 3);
    fill[(blocks + 1) * 3] = '\0';
    strncpy(empty, empty_string, 33 - blocks);
    empty[33 - blocks] = '\0';

    for (int i = 1; i <= core_rows; i++) {

      printf("\033[%d;%dH%s██%s%s%s%s████", newl + i, indent, color.shell,
             battery_color, fill, empty, color.shell);

      if (cap[i])
        printf("████");
    }

    printf("\033[%d;%dH████████████████████████████████████████",
           newl + core_rows + 1, indent);

    redraw = 1;
  } else {
    if (blocks != previous_blocks)
      print_col(core_rows);
  }
  previous_blocks = blocks;
}

void big_loop(int opt) {
  if (opt == 1) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    rows = w.ws_row;
    cols = w.ws_col;

    if (flags.inlin && !flags.live) {
      char buf[2] = {};
      int i = 0;
      char ch;

      struct termios term, restore;

      tcgetattr(0, &term);
      tcgetattr(0, &restore);
      tcsetattr(0, TCSANOW, &term);
      term.c_lflag &= ~(ICANON | ECHO);

      write(1, "\033[6n", 4);
      for (ch = 0; ch != 'R'; read(0, &ch, 1)) {
        if (ch >= '0' && ch <= '9') {
          if (i < 2)
            buf[i] = ch;
          i++;
        }
      }

      tcsetattr(0, TCSANOW, &restore);

      newl = atoi(buf) + 1;
      int min_rows = 9 + flags.fat;
      if (rows - newl < min_rows) {
        for (int i = 0; i < min_rows; i++) {
          printf("\r\n");
        }
        newl = rows - min_rows + 1;
      }

      indent = 0;
    } else {
      printf("\e[2J");
      newl = rows / 2 - 3;
      indent = cols / 2 - 24;
    }
  }

  update_state();
  print_bat();
  print_charge();

  if (flags.digits)
    print_number(newl + 2);
  else {
    printf("\r\n");
  }

  if (!flags.inlin)
    printf("\r\n");
}

void print_small_bat_row() {
  int i = 0;

  printf("%s██%s", color.shell, battery_color);
  while (i < 14) {
    if (i < blocks)
      printf("█");
    else
      printf(" ");
    i++;
  }
  printf("%s████▊", color.shell);
}

void print_small_bat() {
  printf("\n%s███████████████████\r\n", color.shell);

  print_small_bat_row();

  if (bat.charging)
    printf("   %s▄▄▄\r\n", color.charge);
  else
    printf("               \r\n");

  print_small_bat_row();

  if (bat.charging)
    printf("  %s▀▀▀\r\n", color.charge);
  else
    printf("            \r\n");

  printf("%s███████████████████\n", color.shell);
}

void small_loop() {
  update_state();
  print_small_bat();
  printf("\e[5F");
}

void main_loop(int opt) { flags.small ? small_loop() : big_loop(opt); }

int handle_input(char c) {
  switch (c) {
  case 'd':
    toggle(&flags.digits);
    redraw = 0;
    main_loop(0);
    break;
  case 'f':
    toggle(&flags.fat);
    redraw = 0;
    main_loop(1);
    break;
  case 'c':
    toggle(&flags.alt_charge);
    print_charge();
    break;
  case 'e':
    toggle(&flags.extra_colors);
    main_loop(0);
    break;
  case 'm':
    if (flags.mode == 'c')
      flags.mode = 't';
    else if (flags.mode == 't')
      flags.mode = 'm';
    else if (flags.mode == 'm')
      flags.mode = 'c';

    redraw = 0;
    bat_status(0);
    main_loop(0);
    break;
  // case 'a': {
  //   char b[100];
  //   snprintf(b, 100, "echo %d > ~/testbat/capacity", bat.capacity- 1);
  //   system(b);
  //   break;
  // }
  // case 's': {
  //   char b[100];
  //   snprintf(b, 100, "echo %d > ~/testbat/capacity", bat.capacity+ 1);
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
       "  battery [-lsmbdfne]\r\n"
       "\r\n"
       "Options\r\n"
       "  -l, --live                       Monitor the battery live\r\n"
       "  -s, --small                      Print a small version of the "
       "battery\r\n"
       "  -i, --inline                     Prints the battery inline instead "
       "of the center of the screen\r\n"
       "  -f, --fat                        Draws a slightly thicker "
       "battery\r\n"
       "  -d, --digits                     Prints the current capacity as a "
       "number in the "
       "battery\r\n"
       "  -m, --mode MODE                  Specify the mode to be printed "
       "with "
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
      {"mode", required_argument, NULL, 'm'},
      {"bat-number", required_argument, NULL, 'b'},
      {"fat", no_argument, NULL, 'f'},
      {"no-color", no_argument, NULL, 'n'},
      {"digits", no_argument, NULL, 'd'},
      {"alt-charge", no_argument, NULL, 'c'},
      {"live", no_argument, NULL, 'l'},
      {"minimal", no_argument, NULL, 'm'},
      {"inline", no_argument, NULL, 'i'},
      {"small", no_argument, NULL, 's'},
      {"help", no_argument, NULL, 'h'},
  };
  char opt;
  while ((opt = getopt_long(argc, argv, ":hnlmtM:csidfb:", long_options,
                            NULL)) != -1) {
    switch (opt) {
    case 'n':
      toggle(&flags.colors);
      break;
    case 'l':
      toggle(&flags.live);
      break;
    case 'm':
      toggle(&flags.minimal);
      break;
    case 'i':
      toggle(&flags.inlin);
      break;
    case 's':
      toggle(&flags.small);
      break;
    case 'd':
      toggle(&flags.digits);
      break;
    case 'f':
      toggle(&flags.fat);
      break;
    case 'c':
      toggle(&flags.alt_charge);
      break;
    case 'e':
      toggle(&flags.extra_colors);
      break;
    case 'M':
      flags.mode = optarg[0];
      break;
    case 'b': {
      char buffer[50];
      sprintf(buffer, "/sys/class/power_supply/bat%s", optarg);
      strcpy(flags.bat_number, buffer);
    } break;
    case 'h':
      print_help();
      exit(0);
    }
  }
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

      if (!strcmp(key, "color_high"))
        color.high = color_to_ansi(val);
      else if (!strcmp(key, "color_mid"))
        color.mid = color_to_ansi(val);
      else if (!strcmp(key, "color_low"))
        color.low = color_to_ansi(val);
      else if (!strcmp(key, "color_charge"))
        color.charge = color_to_ansi(val);
      else if (!strcmp(key, "color_shell"))
        color.shell = color_to_ansi(val);
      else if (!strcmp(key, "color_temp"))
        color.temp = color_to_ansi(val);
      else if (!strcmp(key, "color_full"))
        color.full = color_to_ansi(val);
      else if (!strcmp(key, "color_left"))
        color.left = color_to_ansi(val);
      else if (!strcmp(key, "colors"))
        flags.colors = strcmp(val, "true\n") ? 0 : 1;
      else if (!strcmp(key, "live"))
        flags.live = strcmp(val, "true\n") ? 0 : 1;
      else if (!strcmp(key, "digits"))
        flags.digits = strcmp(val, "true\n") ? 0 : 1;
      else if (!strcmp(key, "fat"))
        flags.fat = strcmp(val, "true\n") ? 0 : 1;
      else if (!strcmp(key, "small"))
        flags.small = strcmp(val, "true\n") ? 0 : 1;
      else if (!strcmp(key, "minimal"))
        flags.minimal = strcmp(val, "true\n") ? 0 : 1;
      else if (!strcmp(key, "alt_charge"))
        flags.alt_charge = strcmp(val, "true\n") ? 0 : 1;
      else if (!strcmp(key, "extra_colors"))
        flags.extra_colors = strcmp(val, "true\n") ? 0 : 1;
      else if (!strcmp(key, "mode"))
        flags.mode = val[0];
    }

    fclose(fp);
    if (line)
      free(line);
  }
}

void cleanup() {
  system("/bin/stty cooked");
  system("/bin/stty echo");

  printf("\e[0m\e[?25h");
  if (flags.small) {
    printf("\e[5B");
  } else if (flags.live) {
    printf("\e[?47l\e[u");
  } else if (flags.inlin)
    printf("\e[3B");
  else {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    char buffer[20];
    printf("\e[%d;0H", w.ws_row);
  }
}

void setup() {
  system("/bin/stty raw");
  system("/bin/stty -echo");
  printf("\e[?25l");

  if (flags.live && !flags.small)
    printf("\e[?47h\e[s");

  strcpy(flags.bat_number, "/sys/class/power_supply/BAT0");
  parse_config();

  if (!flags.colors) {
    color.shell = "\e[0m";
    color.charge = "\e[0m";
  }
}

void print_minimal() {
  bat_status(1);
  printf("Battery: %d%%\r\nTemperature: %d°\r\n", bat.capacity, bat.temp);
  if (bat.charging)
    printf("Time to Full: %dH %dm\r\nCharging: True\r\n", bat.power / 60,
           bat.power % 60);
  else
    printf("Time Left: %dH %dm\r\nCharging: False\r\n", bat.power / 60,
           bat.power % 60);
  exit(0);
}

int main(int argc, char **argv) {
  handle_flags(argc, argv);

  setup();
  atexit(cleanup);

  if (flags.minimal) {
    print_minimal();
  }

  bat_status(0);
  main_loop(1);

  struct winsize w;
  if (flags.live) {
    pthread_t thread;
    pthread_create(&thread, NULL, event_loop, NULL);

    while (1) {
      usleep(200000);
      bat_status(0);

      if (!flags.small || flags.digits) {
        switch (flags.mode) {
        case 't':
          if (previous_bat.temp != bat.temp)
            main_loop(0);
          break;
        case 'm':
          if (previous_bat.power != bat.power)
            main_loop(0);
          break;
        }
      }

      if (previous_bat.capacity != bat.capacity ||
          previous_bat.charging != bat.charging) {
        main_loop(0);
      }

      ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
      if (w.ws_row != rows || w.ws_col != cols) {
        redraw = 0;
        main_loop(1);
      }
    }
  }
}
