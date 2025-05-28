#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "state.h"
#include "status.h"

static char *get_param(const char *param) {
  char *line = NULL;
  size_t len = 0;
  char fn[100];
  snprintf(fn, 100, "%s/%s", flags.bat_number, param);

#ifdef DEBUG
  if (!strcmp(param, "capacity"))
    strcpy(fn, "./debug/capacity");
#endif

  if (access(fn, F_OK)) {
    line = calloc(1, sizeof(char));
    return line;
  }

  FILE *fp = fopen(fn, "r");
  if (fp == NULL) {
    line = calloc(1, sizeof(char));
    return line;
  }

  getline(&line, &len, fp);
  fclose(fp);
  return line;
}

int get_capacity(void) {
  char *cap_str = get_param("capacity");
  int cap = atoi(cap_str);
  free(cap_str);
  return cap;
}

bool get_is_charging(void) {
  bool is_charging = true;
  char *status = get_param("status");

  if (!strcmp(status, "Unknown\n") || !strcmp(status, "Discharging\n") ||
      !strcmp(status, "Not charging\n")) {
    is_charging = false;
  }

  free(status);
  return is_charging;
}

float get_temp(void) {
  char *temp_str = get_param("temp");
  int temp = atoi(temp_str) / 10.;
  free(temp_str);
  return temp;
}

int get_time(void) {
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
    return (charge_full - charge_now) * 60 / (current_now + 1);
  } else {
    return charge_now * 60 / (current_now + 1);
  }
}

float get_power(void) {
  char *cu = get_param("current_now");
  char *vu = get_param("voltage_now");

  const int current_now = atoi(cu);
  const int voltage_now = atoi(vu);

  free(cu);
  free(vu);

  return (current_now * 1.0 * voltage_now) / 1000000000000;
}

float get_health(void) {
  char *cf = get_param("charge_full");
  char *cd = get_param("charge_full_design");

  const int charge_full = atoi(cf);
  const int charge_full_design = atoi(cd);

  free(cf);
  free(cd);

  return 100.0 * charge_full / (charge_full_design + 1);
}

float get_charge(void) {
  char *cf = get_param("charge_full");
  char *vm = get_param("voltage_min_design");
  const long charge_full = atoi(cf);
  const long voltage_min = atoi(vm);

  free(cf);
  free(vm);

  float charge = charge_full * voltage_min / 100000000000.0;
  return charge / 10.0;
}

void bat_status(bool full) {
  bat.capacity = get_capacity();
  if (bat.capacity > 100 || bat.capacity < 0) {
    printf("Erorr: Battery reporting invalid capacity level: %d", bat.capacity);
    exit(0);
  }

  bat.is_charging = get_is_charging();

  if (flags.mode == temperature || full) {
    bat.temp = get_temp();
  }

  if (flags.mode == time_m || full) {
    bat.time = get_time();
  }

  if (flags.mode == power || full) {
    bat.power = get_power();
  }

  if (flags.mode == health || full) {
    bat.health = get_health();
  }

  if (flags.mode == charge || full) {
    bat.charge = get_charge();
  }

  if (bat.tech == NULL) {
    char *tech = get_param("technology");
    tech[strcspn(tech, "\n")] = 0;
    bat.tech = tech;
  }
}
