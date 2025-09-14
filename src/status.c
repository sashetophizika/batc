#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "state.h"
#include "status.h"

inline static int min(int a, int b) { return a < b ? a : b; }

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

  FILE *const fp = fopen(fn, "r");
  if (fp == NULL) {
    line = calloc(1, sizeof(char));
    return line;
  }

  getline(&line, &len, fp);
  fclose(fp);
  return line;
}

int get_capacity(void) {
  char *const cap_str = get_param("capacity");
  const int cap = min(100, atoi(cap_str));

  free(cap_str);
  return cap;
}

bool get_is_charging(void) {
  bool is_charging = true;
  char *const status = get_param("status");

  if (!strcmp(status, "Unknown\n") || !strcmp(status, "Discharging\n") ||
      !strcmp(status, "Not charging\n")) {
    is_charging = false;
  }

  free(status);
  return is_charging;
}

float get_temp(void) {
  char *const temp_str = get_param("temp");
  const int temp = atoi(temp_str) / 10.;
  free(temp_str);
  return temp;
}

int get_times(void) {
  char *const cn = get_param("charge_now");
  char *const cf = get_param("charge_full");
  char *const cu = get_param("current_now");

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
  char *const cu = get_param("current_now");
  char *const vu = get_param("voltage_now");

  const int current_now = atoi(cu);
  const int voltage_now = atoi(vu);

  free(cu);
  free(vu);

  return (current_now * 1.0 * voltage_now) / 1000000000000;
}

float get_health(void) {
  char *const cf = get_param("charge_full");
  char *const cd = get_param("charge_full_design");

  const int charge_full = atoi(cf);
  const int charge_full_design = atoi(cd);

  free(cf);
  free(cd);

  return 100.0 * charge_full / (charge_full_design + 1);
}

float get_charge(void) {
  char *const cf = get_param("charge_full");
  char *const vm = get_param("voltage_min_design");
  const long charge_full = atoi(cf);
  const long voltage_min = atoi(vm);

  free(cf);
  free(vm);

  const float charge = charge_full * voltage_min / 100000000000.0;
  return charge / 10.0;
}

char *get_tech(void) {
  char *const tech = get_param("technology");
  tech[strcspn(tech, "\n")] = 0;
  return tech;
}

#define get_bat_data(m) bat.m = get_##m()

#define get_mode_data(m)                                                       \
  if (flags.mode == m || full) {                                               \
    get_bat_data(m);                                                           \
  }

#define get_const_data(m)                                                      \
  if (!bat.m) {                                                                \
    get_bat_data(m);                                                           \
  }

void bat_status(bool full) {
  get_bat_data(capacity);
  get_bat_data(is_charging);

  get_mode_data(temp);
  get_mode_data(power);
  get_mode_data(times);

  get_const_data(health);
  get_const_data(charge);
  get_const_data(tech);
}
