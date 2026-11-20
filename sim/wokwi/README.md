# Wokwi version (run it in the browser)

An interactive version of the robot for [Wokwi](https://wokwi.com), the online Arduino simulator.
It runs the real firmware on a simulated Arduino UNO. Parts Wokwi doesn't have are replaced by
stand-ins:

| Real robot | In Wokwi |
|---|---|
| 5-channel flame sensor | 5 slide potentiometers: slide one **left** = that eye sees a flame |
| Left / right motor (L298N) | 2 LEDs per wheel: green = forward, red = backward |
| Water pump + relay | Relay module + blue "WATER" LED |
| SG90 nozzle servo | Servo (same part) |
| Buzzer | Buzzer (same part) |

## How to run it

1. Run `./make_sketch.sh` to rebuild `sketch.ino` from `firmware/` (one file: `config.h` + the `.ino`).
2. Open <https://wokwi.com/projects/new/arduino-uno>.
3. Replace the contents of `sketch.ino` and `diagram.json` with the files from this folder.
4. Add a `libraries.txt` tab containing `Servo`, or click **Install "Servo" library** when Wokwi asks.
5. Press ▶. Keep all sliders on the right for the first 3 seconds: that's when the robot calibrates.
6. Slide an eye to the left and watch the LEDs, the servo and the Serial Monitor.

## What you should see (all checked)

| You do | Robot state | What happens |
|---|---|---|
| Nothing | `SEARCH` | Wheels spin in short steps (turns on the spot) |
| eye 2 a little left | `APPROACH` | Both green LEDs on: drives straight |
| eye 0 a little left | `APPROACH` | Left red + right green: turns left |
| Slide further left (strength ≥ 500) | `EXTINGUISH` | Wheels stop, WATER LED on, servo sweeps |
| Slide back right | `CHECK` → `BACK_OFF` → `SEARCH` | Beep, short reverse, starts searching again |
| Keep the flame on | `ALARM` after 4 sprays | Pump off, buzzer alarm |

Note: Wokwi's relay module is active-high, but the firmware is written for the usual active-low
module (`RELAY_ACTIVE_LOW = true`). So the WATER LED is wired to the relay's **NC** contact here,
which makes the LED behave like the real pump would.
