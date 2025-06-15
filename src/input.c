#include <stdio.h>
#include <stdlib.h>

#include "input.h"
#include "print.h"
#include "state.h"
#include "status.h"

#define Esc 27
#define Ctrl_c 3

static void instant_update(void) {
  state.redraw = true;
  bat_status(false);
  print_battery(false);
}

static int handle_input(char c) {
  switch (c) {
  case 'd':
    toggle(flags.digits);
    print_battery(false);
    break;
  case 't':
    toggle(flags.tech);
    print_battery(false);
    break;
  case 'f':
    toggle(flags.fat);
    state.redraw = true;
    print_battery(false);
    break;
  case 'c':
    toggle(flags.alt_charge);
    if (!flags.small) {
      print_charge();
    }
    break;
  case 'e':
    toggle(flags.extra_colors);
    print_battery(false);
    break;
  case 'p':
    if (flags.mode == power) {
      flags.mode = times;
    } else if (flags.mode == times) {
      flags.mode = power;
    } else if (flags.mode == health) {
      flags.mode = charge;
    } else if (flags.mode == charge) {
      flags.mode = health;
    }

    bat_status(false);
    print_battery(false);
    break;
  case 'm':
    if (flags.mode == capacity) {
      flags.mode = temp;
    } else if (flags.mode == temp) {
      flags.mode = power;
    } else if (flags.mode == power || flags.mode == times) {
      flags.mode = health;
    } else if (flags.mode == health || flags.mode == charge) {
      flags.mode = capacity;
    }
    instant_update();
    break;
  case 'M':
    if (flags.mode == capacity) {
      flags.mode = health;
    } else if (flags.mode == temp) {
      flags.mode = capacity;
    } else if (flags.mode == power || flags.mode == times) {
      flags.mode = temp;
    } else if (flags.mode == health || flags.mode == charge) {
      flags.mode = power;
    }
    instant_update();
    break;
  case '1':
    flags.mode = capacity;
    instant_update();
    break;
  case '2':
    flags.mode = temp;
    instant_update();
    break;
  case '3':
    flags.mode = power;
    instant_update();
    break;
  case '4':
    flags.mode = health;
    instant_update();
    break;
  case '5':
    flags.mode = times;
    instant_update();
    break;
  case '6':
    flags.mode = charge;
    instant_update();
    break;

#ifdef DEBUG
  case 'a': {
    char b[100];
    snprintf(b, 100, "echo %d > ./debug/capacity", bat.capacity - 1);
    system(b);
    break;
  }
  case 's': {
    char b[100];
    snprintf(b, 100, "echo %d > ./debug/capacity", bat.capacity + 1);
    system(b);
    break;
  }
#endif
  case 'q':
  case Esc:
  case Ctrl_c:
    exit(0);
  default:
    break;
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
