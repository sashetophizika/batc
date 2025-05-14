#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "config.h"
#include "input.h"
#include "print.h"
#include "state.h"
#include "status.h"

void cleanup(void) {
  system("/bin/stty cooked echo");

  printf("\033[0m\033[?25h");
  if (flags.small) {
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
    DIR *power_supply_dir = opendir("/sys/class/power_supply");
    while ((bat_dirs = readdir(power_supply_dir))) {
      if (bat_dirs->d_name[0] == 'B' && bat_dirs->d_name[3] < bat_index) {
        bat_index = bat_dirs->d_name[3];
      }
    }
    sprintf(flags.bat_number, "/sys/class/power_supply/BAT%c", bat_index);
    closedir(power_supply_dir);
  }
}

void loop_sleep(void) {
#ifdef DEBUG
  usleep(2500);
#else
  usleep(250000);
#endif
}

int main(int argc, char **argv) {
  parse_config();
  parse_flags(argc, argv);

  atexit(cleanup);
  setup();

  if (flags.minimal) {
    print_minimal();
    exit(0);
  }

  bat_status(false);
  print_battery(true);

  struct winsize w;
  Battery prev_bat = bat;

  if (flags.live) {
    pthread_t thread;
    pthread_create(&thread, NULL, event_loop, NULL);

    while (true) {
      if (!flags.small || flags.digits) {
        if (flags.mode == temperature && prev_bat.temp != bat.temp) {
          print_battery(false);
        } else if (flags.mode == power && prev_bat.power != bat.power) {
          print_battery(false);
        } else if (flags.mode == health && prev_bat.health != bat.health) {
          print_battery(false);
        }
      }

      if (prev_bat.capacity != bat.capacity ||
          prev_bat.is_charging != bat.is_charging) {
        print_battery(false);
      }

      ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
      if (w.ws_row != state.term_rows || w.ws_col != state.term_cols) {
        state.redraw = true;
        print_battery(true);
      }

      loop_sleep();
      prev_bat = bat;
      bat_status(false);
    }
  }
}
