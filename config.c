#include "state.h"
#include "utils.h"
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

void color_to_ansi(char *color, const char **elem) {
  if (strcmp(color, "none\n") == 0) {
    *elem = "\033[0m\0";
  } else if (strcmp(color, "black\n") == 0) {
    *elem = "\033[0;30m\0";
  } else if (strcmp(color, "red\n") == 0) {
    *elem = "\033[0;31m\0";
  } else if (strcmp(color, "green\n") == 0) {
    *elem = "\033[0;32m\0";
  } else if (strcmp(color, "yellow\n") == 0) {
    *elem = "\033[0;33m\0";
  } else if (strcmp(color, "blue\n") == 0) {
    *elem = "\033[0;34m\0";
  } else if (strcmp(color, "magenta\n") == 0) {
    *elem = "\033[0;35m\0";
  } else if (strcmp(color, "cyan\n") == 0) {
    *elem = "\033[0;36m\0";
  } else if (strcmp(color, "white\n") == 0) {
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
    snprintf(ansi_color, 100, "\033[38;2;%ld;%ld;%ldm", strtol(r, NULL, 16),
             strtol(g, NULL, 16), strtol(b, NULL, 16));
    *elem = ansi_color;
  } else {
    *elem = "\033[0m";
  }
}

void parse_flags(int argc, char **argv) {
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
      {"tech", no_argument, NULL, 't'},
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

    if (!strcmp(key, "color_high")) {
      color_to_ansi(val, &colors.high);
    } else if (!strcmp(key, "color_mid")) {
      color_to_ansi(val, &colors.mid);
    } else if (!strcmp(key, "color_low")) {
      color_to_ansi(val, &colors.low);
    } else if (!strcmp(key, "color_charge")) {
      color_to_ansi(val, &colors.charge);
    } else if (!strcmp(key, "color_tech")) {
      color_to_ansi(val, &colors.tech);
    } else if (!strcmp(key, "color_shell")) {
      color_to_ansi(val, &colors.shell);
    } else if (!strcmp(key, "color_temp")) {
      color_to_ansi(val, &colors.temp);
    } else if (!strcmp(key, "color_health")) {
      color_to_ansi(val, &colors.health);
    } else if (!strcmp(key, "color_full")) {
      color_to_ansi(val, &colors.full);
    } else if (!strcmp(key, "color_left")) {
      color_to_ansi(val, &colors.left);
    } else if (!strcmp(key, "color_number")) {
      color_to_ansi(val, &colors.number);
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
      if (val[0] == 'c') {
        flags.mode = capacity;
      } else if (val[0] == 't') {
        flags.mode = temperature;
      } else if (val[0] == 'p') {
        flags.mode = power;
      } else if (val[0] == 'h') {
        flags.mode = health;
      }
    } else if (!strcmp(key, "bat_number")) {
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
