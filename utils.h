#pragma once

#define Esc 27
#define Ctrl_c 3
#define MAX_BLOCKS_BIG 33
#define MAX_BLOCKS_SMALL 14
#define toggle(x)                                                              \
  {                                                                            \
    x = x ? false : true;                                                      \
  }

void main_loop(bool redefine);
void print_help(void);
