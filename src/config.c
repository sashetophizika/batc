#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "print.h"
#include "state.h"

#define eq(s1, s2) strcmp(s1, s2) == 0
#define hex(s) strtol(s, NULL, 16)
#define set_rgb(c, o)                                                          \
  strncpy(c, color + o, 2);                                                    \
  c[2] = 0

#define str(s) #s
#define set_color(word, ansi)                                                  \
  if (eq(color, str(word) "\n")) {                                             \
    *elem = "\033[0;" str(ansi) "m\0";                                         \
    return;                                                                    \
  }

static void color_to_ansi(char *color, const char **elem) {
  set_color("none", "0");
  set_color("black", "30");
  set_color("red", "31");
  set_color("green", "32");
  set_color("yellow", "33");
  set_color("blue", "34");
  set_color("magenta", "35");
  set_color("cyan", "36");
  set_color("white", "37");

  if (color[0] == '#') {
    char r[3], g[3], b[3];

    set_rgb(r, 1);
    set_rgb(g, 3);
    set_rgb(b, 5);

    char *ansi_color = malloc(100);
    snprintf(ansi_color, 100, "\033[38;2;%ld;%ld;%ldm", hex(r), hex(g), hex(b));
    *elem = ansi_color;
    return;
  }

  if (eq(color, "NULL\n")) {
    *elem = NULL;
    return;
  }
  *elem = "\033[0m";
}

#define set_to(m)                                                              \
  if (c == #m[0])                                                              \
    flags.mode = m;                                                            \
  return

void set_mode(char c) {
  set_to(capacity);
  set_to(temp);
  set_to(power);
  set_to(health);
}

void set_bat_number(char num) {
  char buffer[50];
  sprintf(buffer, "/sys/class/power_supply/BAT%c", num);
  DIR *power_supply_dir = opendir(buffer);
  if (power_supply_dir == NULL) {
    printf("%s does not exist.\n", buffer);
    exit(1);
  }
  strcpy(flags.bat_number, buffer);
  closedir(power_supply_dir);
}

void parse_flags(int argc, char **argv) {
  static struct option long_options[] = {
      {"mode", required_argument, NULL, 'M'},
      {"bat-number", required_argument, NULL, 'b'},
      {"fat", no_argument, NULL, 'f'},
      {"fetch", no_argument, NULL, 'F'},
      {"no-color", no_argument, NULL, 'n'},
      {"digits", no_argument, NULL, 'd'},
      {"alt-charge", no_argument, NULL, 'c'},
      {"live", no_argument, NULL, 'l'},
      {"minimal", no_argument, NULL, 'm'},
      {"inline", no_argument, NULL, 'i'},
      {"tech", no_argument, NULL, 't'},
      {"small", no_argument, NULL, 's'},
      {"help", no_argument, NULL, 'h'},
  };

  char opt = 0;
  while (opt != -1) {
    opt = getopt_long(argc, argv, ":hnlmtM:csidfFb:", long_options, NULL);
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
    case 't':
      toggle(flags.tech);
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
    case 'F':
      toggle(flags.fetch);
      break;
    case 'c':
      toggle(flags.alt_charge);
      break;
    case 'e':
      toggle(flags.extra_colors);
      break;
    case 'M':
      set_mode(optarg[0]);
      break;
    case 'b': {
      set_bat_number(optarg[0]);
    } break;
    case 'h':
      print_help();
      exit(0);
    }
  }
}

FILE *get_config(void) {
  const char *home = getenv("HOME");
  if (!home) {
    return NULL;
  }

  char config_file[100];
  snprintf(config_file, 100, "%s/.config/batc/batc.conf", home);
  if (access(config_file, F_OK)) {
    snprintf(config_file, 100, "%s/.config/batc/config", home);
    if (access(config_file, F_OK)) {
      return NULL;
    }
  }
  return fopen(config_file, "r");
}

#define parse_color(c)                                                         \
  if (eq(key, "color_" str(c))) {                                              \
    color_to_ansi(val, &colors.c);                                             \
    continue;                                                                  \
  }

#define parse_flag(f)                                                          \
  if (eq(key, #f)) {                                                           \
    flags.f = eq(val, "true\n") ? true : false;                                \
    continue;                                                                  \
  }

#define parse_flag_arg(f)                                                      \
  if (eq(key, #f)) {                                                           \
    set_##f(val[0]);                                                           \
    continue;                                                                  \
  }

void parse_config(void) {
  FILE *const config = get_config();
  if (!config) {
    return;
  }

  size_t len = 0;
  char *line, *key, *val = NULL;

  while (getline(&line, &len, config) != -1) {
    key = strtok(line, " =");
    val = strtok(NULL, " =");

    if (!key || !val) {
      continue;
    }

    if (key[0] == '#') {
      continue;
    }

    parse_color(high);
    parse_color(mid);
    parse_color(low);
    parse_color(temp);
    parse_color(health);
    parse_color(full);
    parse_color(left);
    parse_color(tech);
    parse_color(shell);
    parse_color(charge);
    parse_color(number);

    parse_flag(colors);
    parse_flag(live);
    parse_flag(digits);
    parse_flag(fat);
    parse_flag(fetch);
    parse_flag(small);
    parse_flag(inlin);
    parse_flag(minimal);
    parse_flag(alt_charge);
    parse_flag(extra_colors);

    parse_flag_arg(mode);
    parse_flag_arg(bat_number);
  }

  fclose(config);
  if (line) {
    free(line);
  }
}
