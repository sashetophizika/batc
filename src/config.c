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
#define toggle_flag(val) eq(val, "true\n") ? false : true;

void color_to_ansi(char *color, const char **elem) {
  if (eq(color, "none\n")) {
    *elem = "\033[0m\0";
  } else if (eq(color, "black\n")) {
    *elem = "\033[0;30m\0";
  } else if (eq(color, "red\n")) {
    *elem = "\033[0;31m\0";
  } else if (eq(color, "green\n")) {
    *elem = "\033[0;32m\0";
  } else if (eq(color, "yellow\n")) {
    *elem = "\033[0;33m\0";
  } else if (eq(color, "blue\n")) {
    *elem = "\033[0;34m\0";
  } else if (eq(color, "magenta\n")) {
    *elem = "\033[0;35m\0";
  } else if (eq(color, "cyan\n")) {
    *elem = "\033[0;36m\0";
  } else if (eq(color, "white\n")) {
    *elem = "\033[0;37m\0";
  } else if (color[0] == '#') {
    char r[3], g[3], b[3];

    strncpy(r, color + 1, 2);
    r[2] = '\0';
    strncpy(g, color + 3, 2);
    g[2] = '\0';
    strncpy(b, color + 5, 2);
    b[2] = '\0';

    char *ansi_color = malloc(100);
    snprintf(ansi_color, 100, "\033[38;2;%ld;%ld;%ldm", hex(r), hex(g), hex(b));
    *elem = ansi_color;
  } else if (eq(color, "NULL\n")) {
    *elem = NULL;
  } else {
    *elem = "\033[0m";
  }
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
      if (optarg[0] == 'c') {
        flags.mode = capacity;
      } else if (optarg[0] == 't') {
        flags.mode = temperature;
      } else if (optarg[0] == 'p') {
        flags.mode = power;
      } else if (optarg[0] == 'h') {
        flags.mode = health;
      }
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

  FILE *config_file = fopen(file_name, "r");
  char *line = NULL;
  size_t len = 0;

  if (config_file == NULL) {
    return;
  }

  char *key, *val;

  while (getline(&line, &len, config_file) != -1) {
    key = strtok(line, " =");
    val = strtok(NULL, " =");

    if (key == NULL || val == NULL) {
      continue;
    }

    if (key[0] == '#') {
      continue;
    }

    if (eq(key, "color_high")) {
      color_to_ansi(val, &colors.high);
    } else if (eq(key, "color_mid")) {
      color_to_ansi(val, &colors.mid);
    } else if (eq(key, "color_low")) {
      color_to_ansi(val, &colors.low);
    } else if (eq(key, "color_charge")) {
      color_to_ansi(val, &colors.charge);
    } else if (eq(key, "color_tech")) {
      color_to_ansi(val, &colors.tech);
    } else if (eq(key, "color_shell")) {
      color_to_ansi(val, &colors.shell);
    } else if (eq(key, "color_temp")) {
      color_to_ansi(val, &colors.temp);
    } else if (eq(key, "color_health")) {
      color_to_ansi(val, &colors.health);
    } else if (eq(key, "color_full")) {
      color_to_ansi(val, &colors.full);
    } else if (eq(key, "color_left")) {
      color_to_ansi(val, &colors.left);
    } else if (eq(key, "color_number")) {
      color_to_ansi(val, &colors.number);
    } else if (eq(key, "colors")) {
      flags.colors = toggle_flag(val);
    } else if (eq(key, "live")) {
      flags.live = toggle_flag(val);
    } else if (eq(key, "digits")) {
      flags.digits = toggle_flag(val);
    } else if (eq(key, "fat")) {
      flags.fat = toggle_flag(val);
    } else if (eq(key, "fetch")) {
      flags.fetch = toggle_flag(val);
    } else if (eq(key, "small")) {
      flags.small = toggle_flag(val);
    } else if (eq(key, "inline")) {
      flags.inlin = toggle_flag(val);
    } else if (eq(key, "minimal")) {
      flags.minimal = toggle_flag(val);
    } else if (eq(key, "alt_charge")) {
      flags.alt_charge = toggle_flag(val);
    } else if (eq(key, "extra_colors")) {
      flags.extra_colors = toggle_flag(val);
    } else if (eq(key, "mode")) {
      if (val[0] == 'c') {
        flags.mode = capacity;
      } else if (val[0] == 't') {
        flags.mode = temperature;
      } else if (val[0] == 'p') {
        flags.mode = power;
      } else if (val[0] == 'h') {
        flags.mode = health;
      }
    } else if (eq(key, "bat_number")) {
      char buffer[50];
      sprintf(buffer, "/sys/class/power_supply/BAT%c", val[0]);
      DIR *power_supply_dir = opendir(buffer);
      if (power_supply_dir == NULL) {
        printf("%s does not exist.", buffer);
        exit(0);
      }
      strcpy(flags.bat_number, buffer);
      closedir(power_supply_dir);
    }
  }

  fclose(config_file);
  if (line) {
    free(line);
  }
}
