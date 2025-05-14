#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "state.h"
#include "status.h"

char *get_param(const char *param) {
  char *line = NULL;
  size_t len = 0;
  char fn[100];
  snprintf(fn, 100, "%s/%s", flags.bat_number, param);

#ifdef DEBUG
  if (!strcmp(param, "capacity"))
    strcpy(fn, "./debug/capacity");
#endif

  FILE *fp = fopen(fn, "r");
  if (fp == NULL) {
    if (!flags.live && !flags.minimal) {
      printf("Error: file %s not found.", fn);
    }

    line = malloc(3);
    memset(line, '0', 2);
    line[2] = '\0';
    return line;
  }

  getline(&line, &len, fp);

  fclose(fp);
  return line;
}

void bat_status(bool full) {
  char *cap = get_param("capacity");
  bat.capacity = atoi(cap);
  free(cap);

  if (state.redraw) {
    bat.tech = get_param("technology");
  }

  if (bat.capacity > 100 || bat.capacity < 0) {
    printf("Erorr: Battery reporting invalid capacity levels");
    exit(0);
  }

  char *status = get_param("status");
  if (!strcmp(status, "Unknown\n") || !strcmp(status, "Discharging\n") ||
      !strcmp(status, "Not charging\n")) {
    bat.is_charging = false;
  } else {
    bat.is_charging = true;
  }
  free(status);

  if (flags.mode == temperature || full) {
    char *temp = get_param("temp");
    bat.temp = atoi(temp) / 10;
    free(temp);
  }

  if (flags.mode == time_m || full) {
    char *cn = get_param("charge_now");
    char *cf = get_param("charge_full");
    char *cu = get_param("current_now");

    const int charge_now = atoi(cn);
    const int charge_full = atoi(cf);
    const int current_now = atoi(cu);

    free(cn);
    free(cf);
    free(cu);

    if (bat.is_charging == true) {
      bat.time = (charge_full - charge_now) * 60 / (current_now + 1);
    } else {
      bat.time = charge_now * 60 / (current_now + 1);
    }
  }

  if (flags.mode == power || full) {
    char *cu = get_param("current_now");
    char *vu = get_param("voltage_now");

    const int current_now = atoi(cu);
    const int voltage_now = atoi(vu);

    free(cu);
    free(vu);

    bat.power = (current_now * 1.0 * voltage_now) / 1000000000000;
  }

  if (flags.mode == health || full) {
    char *cf = get_param("charge_full");
    char *cd = get_param("charge_full_design");

    const int charge_full = atoi(cf);
    const int charge_full_design = atoi(cd);

    free(cf);
    free(cd);

    bat.health = 100 * charge_full / charge_full_design;
  }
}
