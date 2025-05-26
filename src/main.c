#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "config.h"
#include "input.h"
#include "print.h"
#include "state.h"
#include "status.h"

static void cleanup(void) {
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag |= ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &term);

  printf("\033[0m\033[?25h");

  if (flags.fetch) {
    printf("\033[%dB", 7 + 2 * flags.small);
  } else if (flags.small) {
    printf("\033[5B");
  } else if (flags.inlin) {
    printf("\033[3B\033[%d;0H", state.start_row + 8 + flags.fat);
  } else if (flags.live) {
    printf("\033[?47l\033[u");
  } else if (!flags.minimal) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    printf("\033[%d;0H", w.ws_row);
  }
}

static void setup(void) {
  struct termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &term);

  printf("\033[?25l");

  if (flags.live && !(flags.small || flags.inlin || flags.fetch)) {
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
    DIR *power_supply_dir = opendir("/sys/class/power_supply");
    while ((bat_dirs = readdir(power_supply_dir))) {
      if (bat_dirs->d_name[0] == 'B' && bat_dirs->d_name[3] < bat_index) {
        bat_index = bat_dirs->d_name[3];
      }
    }

    char buffer[50];
    snprintf(buffer, 50, "/sys/class/power_supply/BAT%c", bat_index);
    if (opendir(buffer) == NULL) {
      printf("No battery found.");
      exit(0);
    }

    strncpy(flags.bat_number, buffer, 50);
    closedir(power_supply_dir);
  }
}

static void loop_sleep(void) {
#ifdef DEBUG
  usleep(2500);
#else
  usleep(250000);
#endif
}

int main(int argc, char **argv) {
  parse_config();
  parse_flags(argc, argv);

  if (flags.minimal) {
    bat_status(true);
    print_minimal(0);
    exit(0);
  }

  atexit(cleanup);
  setup();

  bat_status(flags.fetch);
  print_battery(true);

  struct winsize w;
  Battery prev_bat = bat;

  if (flags.live) {
    pthread_t thread;
    pthread_create(&thread, NULL, event_loop, NULL);

    while (true) {
      bool should_print = false;
      bool should_redefine = false;
      if (flags.fetch && bat_eq(&bat, &prev_bat)) {
        should_print = true;
      }

      if (!flags.small || flags.digits) {
        if (flags.mode == temperature && prev_bat.temp != bat.temp) {
          should_print = true;
        } else if (flags.mode == power && prev_bat.power != bat.power) {
          should_print = true;
        } else if (flags.mode == health && prev_bat.health != bat.health) {
          should_print = true;
        }
      }

      if (prev_bat.capacity != bat.capacity ||
          prev_bat.is_charging != bat.is_charging) {
        should_print = true;
      }

      ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
      if (w.ws_row != state.term_rows || w.ws_col != state.term_cols) {
        state.redraw = true;
        should_print = true;
        should_redefine = true;
      }

      if (should_print) {
        print_battery(should_redefine);
      }

      loop_sleep();
      prev_bat = bat;
      bat_status(flags.fetch);
    }
  }
}
