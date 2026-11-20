# Autonomous Fire-Fighting Robot - Project Report

*Draft. Sections marked **[to complete]** are filled in after the hardware tests.*

## 1. Abstract

This project is a low-cost (about US $21 of electronics) robot that detects a small flame,
drives to it on its own and extinguishes it with water. It is built around an Arduino UNO, a
5-channel infrared flame sensor, a two-wheel chassis driven through an L298N motor driver, and a
relay-switched water pump whose nozzle is swept by a servo. The control program is a
seven-state machine. Before building the hardware, the unchanged firmware was compiled for a PC
and tested against a simulated world; it passed all seven test scenarios, including cases with no
flame, an out-of-range flame and a flame that cannot be extinguished.

## 2. Problem

Small fires grow quickly, and approaching them is dangerous. The goal was a robot that can:

1. notice a flame anywhere around it,
2. move close enough to reach it with water without touching it,
3. spray until the flame is out and confirm it,
4. never spray when there is no fire, and stop safely when it cannot succeed.

Constraints: a total budget of about US $21 for electronics, parts available online, no soldering,
and a build time of about six weeks for a beginner. The test fire is a candle on a metal tray.

## 3. Design

### 3.1 Hardware

| Function | Choice | Reason |
|---|---|---|
| Controller | Arduino UNO | cheap, robust, 6 analog inputs, large community |
| Flame detection | 5-channel IR flame sensor (photodiodes, 760-1100 nm) | flames emit strongly in near-IR; five channels give direction, not just presence |
| Movement | 2WD chassis + L298N | differential drive can turn on the spot; L298N lets 5 V logic control 7.4 V motors |
| Extinguishing | 3-5 V submersible pump through a relay | the relay isolates the pump current from the Arduino |
| Aiming | SG90 servo sweeping the nozzle +/-30 deg | covers the flame even if the robot stops slightly off-axis |
| Power | 2x 18650 Li-ion (7.4 V) | enough voltage for the Arduino regulator and the motors; rechargeable |

The wiring is shown in `docs/images/wiring.svg` and listed pin by pin in `docs/wiring.md`.
High-current parts (servo, pump) take their 5 V from the L298N regulator rather than from the
Arduino, to avoid resets when they start.

### 3.2 Sensing

Each channel's analog value drops when it sees more infrared light. At start-up the robot averages
40 readings per channel with no flame in view and stores them as a **baseline**. During operation
the *strength* of each channel is the difference from its baseline, so ordinary room light is
ignored.

Two numbers are derived from the five strengths:

- **maximum strength**, compared with three thresholds: *detected* (120), *lost* (80, lower than
  *detected* to avoid flickering between states) and *near* (500, which corresponds to about
  20 cm in the simulation);
- **flame position**, the strength-weighted average of the channel indices -2 ... +2, which tells
  how far left or right the flame is.

### 3.3 Control: a state machine

| State | Behaviour | Transition |
|---|---|---|
| STARTING | wait 3 s, calibrate | -> SEARCH |
| SEARCH | rotate in short steps (spin 160 ms, look 120 ms) | flame detected -> APPROACH |
| APPROACH | position < -1.2 or > 1.2: turn on the spot; +/-0.4 to 1.2: curve; otherwise straight | strength >= near -> EXTINGUISH; unseen for 0.8 s -> SEARCH |
| EXTINGUISH | stop, pump on, sweep nozzle for 2.5 s | -> CHECK |
| CHECK | pump off, wait 1.2 s, look again | out -> BACK_OFF; still near -> EXTINGUISH; farther -> APPROACH; 4 sprays used -> ALARM |
| BACK_OFF | reverse for 0.5 s, beep | -> SEARCH |
| ALARM | stop everything, beep every second | (needs a human) |

The program never blocks: timing uses `millis()`, so sensors are read 50 times a second whatever
the robot is doing. All pins, thresholds, speeds and timings are in a single file (`config.h`) so
the robot can be tuned without touching the logic.

**Safety.** The pump has an independent 4 s maximum on-time, the relay pin is set to OFF before it
is enabled (no spray at power-up) and the ALARM state limits the robot to four attempts.

## 4. Verification in simulation

To find logic errors before buying parts, the firmware was compiled for a PC together with a small
replacement for the Arduino library and a 2-D world model:

- **Motion:** each wheel's speed is computed from the L298N direction pins and PWM value,
  including a dead band (TT motors do not move below PWM 90). The robot moves as a
  differential-drive vehicle with a 13 cm wheel base.
- **Sensor:** each channel's response falls with distance (`exp(-d / 0.4 m)`) and with the angle
  from the channel's axis (Gaussian, about 25 deg wide), plus random noise and 15 % flicker.
- **Fire:** the candle is lit after the start-up beep. It goes out after 0.6 s of water hitting it,
  which requires the pump to run, the flame to be within 35 cm and the nozzle to point within
  12 deg of it.

| # | Scenario | Expected | Result |
|---|---|---|---|
| 1 | flame 60 cm straight ahead | put out | put out in 4.5 s, closest 21 cm |
| 2 | flame 70 cm behind | put out | put out in 7.5 s, closest 19 cm |
| 3 | flame 50 cm at 45 deg left | put out | put out in 7.5 s, closest 19 cm |
| 4 | flame to the right, robot facing away | put out | put out in 5.7 s, closest 18 cm |
| 5 | no flame | never spray | never sprayed |
| 6 | flame 2 m away (out of range) | never spray | never sprayed |
| 7 | flame that water cannot put out | alarm after 4 sprays | alarm after 4 sprays |

In every run the robot never touched the candle and no pump run exceeded 2.5 s. One design change
came from the simulation: with the first *near* threshold (650) the robot stopped only 7-9 cm from
the flame, which is too close for a real candle; lowering it to 500 gives about 20 cm.

**Limits of the simulation.** It checks the decision logic, not the real physics: actual sensor
curves, motor speeds, battery sag and spray range differ and must be measured.

## 5. Hardware tests **[to complete]**

Planned procedure, one part at a time, using the sketches in `firmware/tests/`:

1. Motors: each wheel's direction; minimum PWM that moves the robot.
2. Flame sensor: raw values with no flame, and with a candle at 20, 40, 60, 80 cm on each channel;
   choose the thresholds from these numbers.
3. Pump and relay: relay logic (active-low or active-high); spray distance.
4. Servo: centre position aligned with the robot's axis.
5. Full robot: 10 runs per starting position (ahead, left, right, behind); record success rate and
   time to extinguish.

| Test | Measured | Notes |
|---|---|---|
| Baseline values | | |
| Strength at 20 / 40 / 60 / 80 cm | | |
| Spray distance | | |
| Success rate (40 runs) | | |
| Average time to extinguish | | |

## 6. Discussion **[to complete after tests]**

Expected issues to check: false detections from sunlight or incandescent lamps (both emit
infrared), water splashing on the electronics, and the 5 V supply dropping when the pump and
servo start together (fix: separate buck converter).

## 7. Future work

- **Drone version.** The detection-and-spray module is independent of the wheels. On a drone, a
  dedicated flight controller would fly while the Arduino keeps the same role. The main obstacles
  are payload (water is heavy), spray recoil and drone regulations.
- A thermal camera for longer range and fewer false positives.
- An ultrasonic distance sensor, so stopping distance does not depend on flame brightness.
- Bluetooth telemetry to a phone.

## 8. Conclusion

The project shows that a reliable fire-fighting behaviour can be obtained with very cheap parts by
combining a simple directional sensor with a clear state machine, and that testing the firmware in
simulation before building catches design problems early (here, the stopping distance).
