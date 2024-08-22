#include <dirent.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define toggle(x)                                                              \
  { x = x ? false : true; }

typedef struct Battery {
  int capacity;
  int temp;
  int time;
  bool charging;
} Battery;

typedef struct Colors {
  const char *high;
  const char *mid;
  const char *low;
  const char *temp;
  const char *full;
  const char *left;
  const char *charge;
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
  char mode;
  char bat_number[50];
} Flags;

Battery bat = {0, 0, 0, false};
Battery previous_bat = {0, 0, 0, false};

Colors colors = {.high = "\033[0;32m\0",
                 .mid = "\033[0;33m\0",
                 .low = "\033[0;31m\0",
                 .temp = "\033[0;35m\0",
                 .full = "\033[0;36m\0",
                 .left = "\033[0;34m\0",
                 .charge = "\033[0;36m\0",
                 .shell = "\033[0m\0",
                 .number = NULL};

Flags flags = {.colors = true,
               .live = false,
               .minimal = false,
               .small = false,
               .digits = false,
               .fat = false,
               .alt_charge = false,
               .extra_colors = true,
               .inlin = false,
               .mode = 'c',
               .bat_number = ""};

int newl = 0;
int indent = 0;
int rows = 0;
int cols = 0;

bool redraw = true;
int blocks = 0;

const char *battery_color = "";
const char *previous_battery_color = "";

int previous_num_length = 0;
int previous_blocks = 0;

int digit_count(int num) {
  int count = 0;
  do {
    num /= 10;
    count++;
  } while (num != 0);

  return count;
}

const char *color_to_ansi(char *color) {
  if (strcmp(color, "none\n") == 0) {
    return "\033[0m\0";
  } else if (strcmp(color, "black\n") == 0) {
    return "\033[0;30m\0";
  } else if (strcmp(color, "red\n") == 0) {
    return "\033[0;31m\0";
  } else if (strcmp(color, "green\n") == 0) {
    return "\033[0;32m\0";
  } else if (strcmp(color, "yellow\n") == 0) {
    return "\033[0;33m\0";
  } else if (strcmp(color, "blue\n") == 0) {
    return "\033[0;34m\0";
  } else if (strcmp(color, "magenta\n") == 0) {
    return "\033[0;35m\0";
  } else if (strcmp(color, "cyan\n") == 0) {
    return "\033[0;36m\0";
  } else if (strcmp(color, "white\n") == 0) {
    return "\033[0;37m\0";
  } else if (color[0] == '#') {
    char r[3], g[3], b[3];

    strncpy(r, color + 1, 2);
    r[2] = '\0';
    strncpy(g, color + 3, 2);
    g[2] = '\0';
    strncpy(b, color + 5, 2);
    b[2] = '\0';

    char *a = malloc(100);
    sprintf(a, "\033[38;2;%ld;%ld;%ldm", strtol(r, NULL, 16),
            strtol(g, NULL, 16), strtol(b, NULL, 16));
    return a;
  }

  return "\033[0m";
}

char *get_param(const char *param) {
  char *line = NULL;
  size_t len = 0;
  char fn[100];
  snprintf(fn, 100, "%s/%s", flags.bat_number, param);

  /*if (!strcmp(param, "capacity"))*/
  /*  strcpy(fn, "/home/sasho/testbat/capacity");*/

  FILE *fp = fopen(fn, "r");
  if (fp == NULL) {
    printf("Error: file %s not found.", fn);
    exit(0);
  }

  getline(&line, &len, fp);

  fclose(fp);
  return line;
}

void bat_status(bool full) {
  previous_bat.capacity = bat.capacity;
  previous_bat.temp = bat.temp;
  previous_bat.time = bat.time;
  previous_bat.charging = bat.charging;

  char *cap = get_param("capacity");
  bat.capacity = atoi(cap);
  free(cap);

  if (bat.capacity > 100 || bat.capacity < 0) {
    printf("Erorr: Battery reporting invalid capacity levels");
    exit(0);
  }

  if (flags.mode == 't' || full) {
    char *temp = get_param("temp");
    bat.temp = atoi(temp) / 10;
    free(temp);
  }

  char *status = get_param("status");
  if (!strcmp(status, "Unknown\n") || !strcmp(status, "Discharging\n") ||
      !strcmp(status, "Not charging\n")) {
    bat.charging = false;
  } else {
    bat.charging = true;
  }
  free(status);

  if (flags.mode == 'm' || full) {
    char *cn = get_param("charge_now");
    char *cf = get_param("charge_full");
    char *cu = get_param("current_now");

    const int charge_now = atoi(cn);
    const int charge_full = atoi(cf);
    const int current_now = atoi(cu);

    free(cn);
    free(cf);
    free(cu);

    if (bat.charging == true) {
      bat.time = (charge_full - charge_now) * 60 / (current_now + 1);
    } else {
      bat.time = charge_now * 60 / (current_now + 1);
    }
  }

  blocks = flags.small ? bat.capacity / 7 : bat.capacity / 3;
}

void update_state(void) {
  if (bat.capacity < 20) {
    battery_color = colors.low;
  } else if (bat.capacity < 60) {
    battery_color = colors.mid;
  } else {
    battery_color = colors.high;
  }

  int data = 0;
  if (flags.mode == 'c') {
    data = bat.capacity;
  } else if (flags.mode == 'm') {
    data = bat.time;

    if (flags.extra_colors == true) {
      if (bat.charging == true)
        battery_color = colors.full;
      else
        battery_color = colors.left;
    }
  } else if (flags.mode == 't') {
    data = bat.temp;

    if (flags.extra_colors == true) {
      battery_color = colors.temp;
    }
  }

  if (flags.colors == false) {
    battery_color = "\033[0m";
  }

  if (previous_num_length != digit_count(data)) {
    redraw = true;
  }
  previous_num_length = digit_count(data);

  if (battery_color != previous_battery_color) {
    redraw = true;
  }
  previous_battery_color = battery_color;
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
    negate = blocks + indent + 3 - col;
    digit_color = battery_color;
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

void print_number(int row) {
  int data = 0;
  if (flags.mode == 'c') {
    data = bat.capacity;
  } else if (flags.mode == 'm') {
    data = bat.time;
  } else if (flags.mode == 't') {
    data = bat.temp;
  }

  int digits = digit_count(data);
  digits = digits > 4 ? 4 : digits;

  int col = indent + 4 * digits + 12;
  for (int i = 0; i < digits; i++) {
    print_digit(!flags.digits ? 42 : data % 10, row, col - 8 * i);
    data /= 10;
  }
}

void print_charge(void) {
  if (!bat.charging) {
    printf("%s\033[%d;%dH             "
           "\033[1B\033[13D             "
           "\033[1B\033[13D             ",
           colors.charge, newl + 3, indent + 47);
  } else if (flags.fat && flags.alt_charge) {
    printf("%s\033[%d;%dH   ███████ "
           "\033[1B\033[13D    ██     "
           "\033[1B\033[13D███████    ",
           colors.charge, newl + 3, indent + 47);
  } else if (flags.fat) {
    printf("%s\033[%d;%dH    ███████  "
           "\033[1B\033[13D    ███      "
           "\033[1B\033[13D███████      ",
           colors.charge, newl + 3, indent + 47);
  } else if (flags.alt_charge) {
    printf("%s\033[%d;%dH   ██████ "
           "\033[1B\033[13D ██████    "
           "\033[1B\033[13D             ",
           colors.charge, newl + 3, indent + 47);
  } else {
    printf("%s\033[%d;%dH   ████████  "
           "\033[1B\033[13D████████     "
           "\033[1B\033[13D             ",
           colors.charge, newl + 3, indent + 47);
  }
}

void print_col(int core_rows) {
  const int diff = blocks - previous_blocks;
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

  int index = 0;
  for (int i = start; step * i < step * end; i += step) {
    index = indent + previous_blocks + 2 + i;
    printf("%s\033[%d;%dH", battery_color, newl + 1, index);

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
    char fill[150], empty[50];

    printf("\033[%d;%dH%s████████████████████████████████████████", newl,
           indent, colors.shell);

    strncpy(fill, block_string, (blocks + 1) * 3);
    fill[(blocks + 1) * 3] = '\0';
    strncpy(empty, empty_string, 33 - blocks);
    empty[33 - blocks] = '\0';

    for (int i = 1; i <= core_rows; i++) {
      printf("\033[%d;%dH%s██%s%s%s%s████", newl + i, indent, colors.shell,
             battery_color, fill, empty, colors.shell);

      if (i > 1 && i < core_rows) {
        printf("████");
      }
    }

    printf("\033[%d;%dH████████████████████████████████████████",
           newl + core_rows + 1, indent);

    if (!flags.fat && flags.live) {
      printf("\033[%d;%dH    \033[%d;%dH                                       "
             "      \r\n",
             newl + core_rows, indent + 40 + flags.inlin, newl + core_rows + 2,
             indent);
    }
    redraw = false;
  } else {
    if (blocks != previous_blocks) {
      print_col(core_rows);
    }
  }
  previous_blocks = blocks;
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

    newl = atoi(buf) + 1;
    const int min_rows = 10 + flags.fat;
    if (rows - newl < min_rows) {
      for (int j = 0; j < min_rows; j++) {
        printf("\r\n");
      }
      newl = rows - min_rows + 1;
    }

    indent = 0;
  } else {
    printf("\033[2J");
    newl = rows / 2 - 3;
    indent = cols / 2 - 21;
  }
}

void big_loop(bool redefine) {
  if (redefine == true) {
    define_position();
  }

  update_state();
  print_bat();
  print_charge();
  print_number(newl + 2);

  if (flags.inlin) {
    printf("\033[%d;0H", newl - 1);
  } else {
    printf("\r\n");
  }
}

void print_small_bat_row(bool top) {
  printf("%s██%s", colors.shell, battery_color);
  for (int i = 0; i < 14; i++) {
    if (i < blocks) {
      printf("█");
    } else {
      printf(" ");
    }
  }
  printf("%s████", colors.shell);

  if (!bat.charging) {
    printf("               \r\n");
  } else if (top) {
    printf("   %s▄▄▄\r\n", colors.charge);
  } else {
    printf("  %s▀▀▀\r\n", colors.charge);
  }
}

void print_small_bat(void) {
  printf("\n%s██████████████████\r\n", colors.shell);
  print_small_bat_row(true);
  print_small_bat_row(false);
  printf("%s██████████████████\n", colors.shell);
}

void small_loop(void) {
  update_state();
  print_small_bat();
  printf("\033[5F");
}

void main_loop(bool redefine) {
  flags.small ? small_loop() : big_loop(redefine);
}

int handle_input(char c) {
  switch (c) {
  case 'd':
    toggle(flags.digits);
    main_loop(false);
    break;
  case 'f':
    toggle(flags.fat);
    redraw = true;
    main_loop(false);
    break;
  case 'c':
    toggle(flags.alt_charge);
    print_charge();
    break;
  case 'e':
    toggle(flags.extra_colors);
    main_loop(false);
    break;
  case 'm':
    if (flags.mode == 'c') {
      flags.mode = 't';
    } else if (flags.mode == 't') {
      flags.mode = 'm';
    } else if (flags.mode == 'm') {
      flags.mode = 'c';
    }

    redraw = true;
    bat_status(false);
    main_loop(false);
    break;
  /*case 'a': {*/
  /*  char b[100];*/
  /*  snprintf(b, 100, "echo %d > ~/testbat/capacity", bat.capacity - 1);*/
  /*  system(b);*/
  /*  break;*/
  /*}*/
  /*case 's': {*/
  /*  char b[100];*/
  /*  snprintf(b, 100, "echo %d > ~/testbat/capacity", bat.capacity + 1);*/
  /*  system(b);*/
  /*  break;*/
  /*}*/
  case 'q':
  case 27: // Escape
  case 3:  // Ctrl-c
    exit(0);
  }
  return 1;
}

void *event_loop(void *legolas) {
  char c = '0';
  while (handle_input(c)) {
    c = getchar();
  }

  return NULL;
}

void print_help(void) {
  puts("Usage:\r\n"
       "  battery [-lsmbdfne]\r\n"
       "\r\n"
       "Options\r\n"
       "  -l, --live                       Monitor the battery live\r\n"
       "  -s, --small                      Print a small version of the "
       "battery\r\n"
       "  -i, --inline                     Print the battery inline instead "
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

  char opt = 0;
  while (opt != -1) {
    opt = getopt_long(argc, argv, ":hnlmtM:csidfb:", long_options, NULL);
    switch (opt) {
    case 'n':
      toggle(flags.colors);
      break;
    case 'l':
      toggle(flags.live);
      break;
    case 'm':
      toggle(flags.minimal);
      break;
    case 'i':
      toggle(flags.inlin);
      break;
    case 's':
      toggle(flags.small);
      break;
    case 'd':
      toggle(flags.digits);
      break;
    case 'f':
      toggle(flags.fat);
      break;
    case 'c':
      toggle(flags.alt_charge);
      break;
    case 'e':
      toggle(flags.extra_colors);
      break;
    case 'M':
      flags.mode = optarg[0];
      break;
    case 'b': {
      char buffer[50];
      sprintf(buffer, "/sys/class/power_supply/BAT%s", optarg);
      if (opendir(buffer) == NULL) {
        printf("%s does not exist.", buffer);
        exit(0);
      }
      strcpy(flags.bat_number, buffer);
    } break;
    case 'h':
      print_help();
      exit(0);
    }
  }
}

void parse_config(void) {
  char file_name[100];
  const char *home = getenv("HOME");
  if (home == NULL) {
    return;
  }

  snprintf(file_name, 100, "%s/.config/batc/batc.conf", home);
  if (access(file_name, F_OK)) {
    snprintf(file_name, 100, "%s/.config/batc/config", home);
    if (access(file_name, F_OK)) {
      return;
    }
  }

  FILE *file_pointer = fopen(file_name, "r");
  char *line = NULL;
  size_t len = 0;

  if (file_pointer == NULL) {
    return;
  }

  char *key, *val;

  while (getline(&line, &len, file_pointer) != -1) {
    key = strtok(line, " =");
    val = strtok(NULL, " =");

    if (key == NULL || val == NULL) {
      continue;
    }

    if (key[0] == '#') {
      continue;
    }

    if (!strcmp(key, "color_high")) {
      colors.high = color_to_ansi(val);
    } else if (!strcmp(key, "color_mid")) {
      colors.mid = color_to_ansi(val);
    } else if (!strcmp(key, "color_low")) {
      colors.low = color_to_ansi(val);
    } else if (!strcmp(key, "color_charge")) {
      colors.charge = color_to_ansi(val);
    } else if (!strcmp(key, "color_shell")) {
      colors.shell = color_to_ansi(val);
    } else if (!strcmp(key, "color_temp")) {
      colors.temp = color_to_ansi(val);
    } else if (!strcmp(key, "color_full")) {
      colors.full = color_to_ansi(val);
    } else if (!strcmp(key, "color_left")) {
      colors.left = color_to_ansi(val);
    } else if (!strcmp(key, "color_number")) {
      colors.number = color_to_ansi(val);
    } else if (!strcmp(key, "colors")) {
      flags.colors = strcmp(val, "true\n") ? false : true;
    } else if (!strcmp(key, "live")) {
      flags.live = strcmp(val, "true\n") ? false : true;
    } else if (!strcmp(key, "digits")) {
      flags.digits = strcmp(val, "true\n") ? false : true;
    } else if (!strcmp(key, "fat")) {
      flags.fat = strcmp(val, "true\n") ? false : true;
    } else if (!strcmp(key, "small")) {
      flags.small = strcmp(val, "true\n") ? false : true;
    } else if (!strcmp(key, "inline")) {
      flags.inlin = strcmp(val, "true\n") ? false : true;
    } else if (!strcmp(key, "minimal")) {
      flags.minimal = strcmp(val, "true\n") ? false : true;
    } else if (!strcmp(key, "alt_charge")) {
      flags.alt_charge = strcmp(val, "true\n") ? false : true;
    } else if (!strcmp(key, "extra_colors")) {
      flags.extra_colors = strcmp(val, "true\n") ? false : true;
    } else if (!strcmp(key, "mode")) {
      flags.mode = val[0];
    } else if (!strcmp(key, "bat_number")) {
      char buffer[50];
      sprintf(buffer, "/sys/class/power_supply/BAT%c", val[0]);
      if (opendir(buffer) == NULL) {
        printf("%s does not exist.", buffer);
        exit(0);
      }
      strcpy(flags.bat_number, buffer);
    }
  }

  fclose(file_pointer);
  if (line) {
    free(line);
  }
}

void cleanup(void) {
  system("/bin/stty cooked echo");

  printf("\033[0m\033[?25h");
  if (flags.small) {
    printf("\033[5B");
  } else if (flags.inlin) {
    printf("\033[3B\033[%d;0H", newl + 8 + flags.fat);
  } else if (flags.live) {
    printf("\033[?47l\033[u");
  } else if (!flags.minimal) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    printf("\033[%d;0H", w.ws_row);
  }
}

void setup(void) {
  system("/bin/stty raw -echo");
  printf("\033[?25l");

  if (flags.live && !(flags.small || flags.inlin)) {
    printf("\033[?47h\033[s");
  }

  if (!flags.colors) {
    colors.shell = "\033[0m";
    colors.charge = "\033[0m";
    colors.number = NULL;
  }

  if (flags.bat_number[0] == '\0') {
    struct dirent *bat_dirs;
    char bat_index = '9';
    DIR *power_supply = opendir("/sys/class/power_supply");
    while ((bat_dirs = readdir(power_supply))) {
      if (bat_dirs->d_name[0] == 'B' && bat_dirs->d_name[3] < bat_index) {
        bat_index = bat_dirs->d_name[3];
      }
    }
    sprintf(flags.bat_number, "/sys/class/power_supply/BAT%c", bat_index);
  }
}

void print_minimal(void) {
  bat_status(1);
  printf("Battery: %d%%\r\nTemperature: %d°\r\n", bat.capacity, bat.temp);
  if (bat.charging) {
    printf("Time to Full: %dH %dm\r\nCharging: True\r\n", bat.time / 60,
           bat.time % 60);
  } else {
    printf("Time Left: %dH %dm\r\nCharging: False\r\n", bat.time / 60,
           bat.time % 60);
  }
}

int main(int argc, char **argv) {
  parse_config();
  handle_flags(argc, argv);

  atexit(cleanup);
  setup();

  if (flags.minimal) {
    print_minimal();
    exit(0);
  }

  bat_status(false);
  main_loop(true);

  struct winsize w;
  if (flags.live) {
    pthread_t thread;
    pthread_create(&thread, NULL, event_loop, NULL);

    while (true) {
      usleep(200000);
      bat_status(0);

      if (!flags.small || flags.digits) {
        if (flags.mode == 't' && previous_bat.temp != bat.temp) {
          main_loop(false);
        } else if (flags.mode == 'm' && previous_bat.time != bat.time) {
          main_loop(false);
        }
      }

      if (previous_bat.capacity != bat.capacity ||
          previous_bat.charging != bat.charging) {
        main_loop(false);
      }

      ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
      if (w.ws_row != rows || w.ws_col != cols) {
        redraw = true;
        main_loop(true);
      }
    }
  }
}
