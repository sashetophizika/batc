# Battery

### Simple utility to monitor the battery in a stylish manner.

![image](screenshot.png "screenshot")
### Small version:

![image](screenshot_small.png "screenshot_small")
### Version with some extra options:

![image](screenshot_full.png "screenshot_full")

## Installation

```bash
git clone https://github.com/sashetophizika/batc.git
cd batc && make
make install # user installation
sudo make install # system installation
```

## Usage

### Basic:
```
$ batc
```
### Options:

* **-l, --live**: monitor the battery live (close with `q` or `Esc`)
* **-s, --small**: print a small inline battery instead
* **-i, --inline**: print the battery inline instead of the center of the screen
* **-f, --fat**: print a slightly thicker battery
* **-d, --digits**: print the current capacity as a number inside of the battery (does not work with -s)
* **-M, --mode=(mode)**: specify mode to be printed with -d (c for capacity, m for time left or to full, t for temperature)
* **-e, --extra-colors**: disable extra core color pattern for different modes
* **-m, --minimal**: print minimal text output
* **-c, --alt-charge**: use an alternate charging symbol (requires nerd fonts)
* **-n, --no-color**: remove colors
* **-b, --bat-number=(bat_number)**: specify the battery number (default is the lowest one)

### Keybinds:
In live mode you can use some keybinds.

* **d**: toggle digits
* **e**: toggle extra_colors
* **f**: toggle fat
* **c**: toggle alt_charge
* **m**: cycle mode

## Configuration

You can create a file named `~/.config/batc/batc.conf` in order to change the colors and the default flags (the flags then become toggles that do the opposite).

### Default configuration:

```python
# do not put quotes around the values
# colors can be [red | green | yellow | blue | magenta | cyan | white | black | none]
# or a hex code (e.g., #0000ff)

color_high = green   # core color between 60-100%
color_mid = yellow   # core color between 20-60%
color_low = red      # core color between 0-20%

color_temp = magenta # core color in 't' mode
color_full = cyan    # core color in 'm' mode when charging
color_left = blue    # core color in 'm' mode when discharging

color_shell = white  # color of the outer shell
color_charge = cyan  # color of the charging symbol
color_number = NULL  # color of the number inside, remove line for default

mode = c             # c for capacity, t for temperature or m for time in minutes
bat_number = 0       # read from /sys/class/power_supply/BAT0

colors = true 
live = false 
minimal = false
small = false
inline = false
digits = false
fat = false
alt_charge = false
```
