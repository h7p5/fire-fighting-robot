// ============================================================================
//  PC simulator for the fire-fighting robot firmware.
//
//  The REAL firmware (firmware/fire_fighting_robot/*.ino) is compiled for the
//  PC against a tiny Arduino stand-in (sim/arduino_shim). A simple 2-D world
//  feeds it fake flame-sensor readings and moves a virtual robot according to
//  the motor pins it drives. Each scenario checks that the robot behaves.
//
//  Build & run:  make -C sim        (writes path drawings to sim/out/*.svg)
// ============================================================================
#include <sys/wait.h>
#include <unistd.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

#include "Arduino.h"

// ---------------------------------------------------------------------------
//  Fake Arduino hardware
// ---------------------------------------------------------------------------
static unsigned long g_now = 0;
static int g_digital[32];
static int g_pwm[32];
int g_servoAngle = 90;
bool g_serialEcho = false;
SimSerial Serial;

int analogReadSim(uint8_t pin);  // implemented by the world below

unsigned long millis() { return g_now; }
void delay(unsigned long ms) { g_now += ms; }  // robot is still during delays
void pinMode(uint8_t, uint8_t) {}
void digitalWrite(uint8_t pin, uint8_t v) { g_digital[pin] = v; }
int digitalRead(uint8_t pin) { return g_digital[pin]; }
void analogWrite(uint8_t pin, int v) { g_pwm[pin] = v; }
int analogRead(uint8_t pin) { return analogReadSim(pin); }

// The firmware itself (setup(), loop(), state machine...)
#include "../firmware/fire_fighting_robot/fire_fighting_robot.ino"

// ---------------------------------------------------------------------------
//  World model
// ---------------------------------------------------------------------------
const double PI_ = 3.14159265358979;
const double DEG = PI_ / 180.0;

struct World {
  double x = 0, y = 0, heading = 0;  // robot centre (m) and heading (rad)
  double fx = 0, fy = 0;             // flame position (m)
  bool flameLit = false;
  double flameHealth = 1.0;          // 0 = extinguished
  bool waterWorks = true;            // false = "stubborn" flame
  unsigned long lightAt = 4000;      // candle is lit after the start beep
  bool collided = false;
  double minDistance = 99;
  std::mt19937 rng{42};
} W;

const double WHEEL_BASE   = 0.13;   // m
const double SENSOR_AHEAD = 0.07;   // flame sensor sits 7 cm ahead of centre
const double MAX_WHEEL_V  = 0.35;   // m/s at PWM 255
const int    PWM_DEADBAND = 90;     // TT motors do not turn below this
const double CHANNEL_AXIS[5] = {60, 30, 0, -30, -60};  // deg, left..right
const int    BASELINE_RAW[5] = {812, 798, 805, 820, 790};

double wrapAngle(double a) {
  while (a > PI_) a -= 2 * PI_;
  while (a < -PI_) a += 2 * PI_;
  return a;
}

void sensorPoint(double &sx, double &sy) {
  sx = W.x + SENSOR_AHEAD * std::cos(W.heading);
  sy = W.y + SENSOR_AHEAD * std::sin(W.heading);
}

double flameDistance() {
  double sx, sy;
  sensorPoint(sx, sy);
  return std::hypot(W.fx - sx, W.fy - sy);
}

int analogReadSim(uint8_t pin) {
  int ch = pin - A0;
  if (ch < 0 || ch > 4) return 0;
  std::uniform_real_distribution<double> noise(-12, 12), flicker(0.85, 1.0);
  double s = 0;
  if (W.flameLit && W.flameHealth > 0) {
    double sx, sy;
    sensorPoint(sx, sy);
    double d = std::hypot(W.fx - sx, W.fy - sy);
    double bearing = wrapAngle(std::atan2(W.fy - sy, W.fx - sx) - W.heading);
    double off = (bearing - CHANNEL_AXIS[ch] * DEG) / (25 * DEG);
    s = 1000.0 * std::exp(-d / 0.4) * std::exp(-off * off) * flicker(W.rng);
  }
  double raw = BASELINE_RAW[ch] - s + noise(W.rng);
  return (int)constrain(raw, 20.0, 1023.0);
}

double wheelSpeed(int pinA, int pinB, int pinEn) {
  int pwm = g_pwm[pinEn];
  if (pwm < PWM_DEADBAND) return 0;
  double v = MAX_WHEEL_V * (pwm - PWM_DEADBAND) / (255.0 - PWM_DEADBAND);
  if (g_digital[pinA] && !g_digital[pinB]) return v;
  if (!g_digital[pinA] && g_digital[pinB]) return -v;
  return 0;
}

bool pumpRunning() {
  bool level = g_digital[PIN_PUMP];
  return RELAY_ACTIVE_LOW ? !level : level;
}

void physicsStep(double dt) {
  if (!W.flameLit && g_now >= W.lightAt) W.flameLit = true;

  double vl = wheelSpeed(PIN_IN1, PIN_IN2, PIN_ENA);
  double vr = wheelSpeed(PIN_IN3, PIN_IN4, PIN_ENB);
  double v = (vl + vr) / 2, w = (vr - vl) / WHEEL_BASE;
  double nx = W.x + v * std::cos(W.heading) * dt;
  double ny = W.y + v * std::sin(W.heading) * dt;
  double oldX = W.x, oldY = W.y;
  W.x = nx;
  W.y = ny;
  W.heading = wrapAngle(W.heading + w * dt);
  if (flameDistance() < 0.05) {  // bumped into the candle
    W.collided = true;
    W.x = oldX;
    W.y = oldY;
  }
  W.minDistance = std::min(W.minDistance, flameDistance());

  // Water hits the flame when the pump runs, the flame is close enough and
  // the nozzle (servo 90 = straight ahead) points at it.
  if (W.flameLit && W.waterWorks && pumpRunning()) {
    double sx, sy;
    sensorPoint(sx, sy);
    double bearing = wrapAngle(std::atan2(W.fy - sy, W.fx - sx) - W.heading);
    double nozzle = (g_servoAngle - 90) * DEG;
    if (flameDistance() < 0.35 && std::fabs(wrapAngle(bearing - nozzle)) < 12 * DEG)
      W.flameHealth -= dt / 0.6;  // 0.6 s of direct hits puts it out
  }
}

// ---------------------------------------------------------------------------
//  Scenarios
// ---------------------------------------------------------------------------
struct Scenario {
  const char *name;
  double startHeadingDeg;
  double flameX, flameY;  // robot starts at (0,0)
  bool flamePresent;
  bool waterWorks;
  enum Expect { PUT_OUT, NEVER_SPRAY, ALARM_STATE } expect;
};

const Scenario SCENARIOS[] = {
    {"flame straight ahead (60 cm)", 0, 0.60, 0.00, true, true, Scenario::PUT_OUT},
    {"flame behind the robot (70 cm)", 0, -0.70, 0.05, true, true, Scenario::PUT_OUT},
    {"flame 45 deg to the left (50 cm)", 0, 0.35, 0.35, true, true, Scenario::PUT_OUT},
    {"flame to the right, robot facing away", 90, 0.55, -0.30, true, true, Scenario::PUT_OUT},
    {"no flame at all", 0, 0, 0, false, true, Scenario::NEVER_SPRAY},
    {"flame too far to see (2 m)", 0, 2.0, 0.0, true, true, Scenario::NEVER_SPRAY},
    {"stubborn flame (water does nothing)", 0, 0.45, 0.10, true, false, Scenario::ALARM_STATE},
};
const int N_SCENARIOS = sizeof(SCENARIOS) / sizeof(SCENARIOS[0]);

void writeSvg(int index, const Scenario &sc, const std::vector<std::pair<double, double>> &path) {
  std::string file = "out/scenario_" + std::to_string(index + 1) + ".svg";
  FILE *f = std::fopen(file.c_str(), "w");
  if (!f) return;
  // world -> pixels: 1.5 m x 1.5 m area centred on the start point (grid = 25 cm)
  auto px = [](double x) { return 150 + x * 200; };
  auto py = [](double y) { return 150 - y * 200; };
  std::fprintf(f,
               "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 300 330' width='300' "
               "height='330' font-family='sans-serif'>\n<rect width='300' height='330' "
               "fill='#ffffff'/>\n<text x='8' y='318' font-size='11' fill='#333'>%d. %s</text>\n",
               index + 1, sc.name);
  for (double g = -0.75; g <= 0.751; g += 0.25)
    std::fprintf(f,
                 "<line x1='%g' y1='0' x2='%g' y2='300' stroke='#eee'/>"
                 "<line x1='0' y1='%g' x2='300' y2='%g' stroke='#eee'/>\n",
                 px(g), px(g), py(g), py(g));
  if (sc.flamePresent)
    std::fprintf(f, "<circle cx='%g' cy='%g' r='6' fill='%s'/>\n", px(sc.flameX), py(sc.flameY),
                 W.flameHealth > 0 ? "#E24B4A" : "#888780");
  std::fprintf(f, "<polyline fill='none' stroke='#185FA5' stroke-width='1.5' points='");
  for (size_t i = 0; i < path.size(); i += 100)
    std::fprintf(f, "%.1f,%.1f ", px(path[i].first), py(path[i].second));
  std::fprintf(f, "'/>\n<circle cx='150' cy='150' r='4' fill='#1D9E75'/>\n</svg>\n");
  std::fclose(f);
}

int runScenario(int index) {
  const Scenario &sc = SCENARIOS[index];
  W.heading = sc.startHeadingDeg * DEG;
  W.fx = sc.flameX;
  W.fy = sc.flameY;
  W.waterWorks = sc.waterWorks;
  if (!sc.flamePresent) W.lightAt = (unsigned long)-1;

  const unsigned long SIM_MS = 90000;
  unsigned long longestPumpRun = 0, pumpStart = 0, totalPumpMs = 0, putOutAt = 0;
  bool wasPumping = false, sawAlarm = false;
  std::vector<std::pair<double, double>> path;

  setup();
  while (g_now < SIM_MS) {
    loop();
    physicsStep(0.001);
    g_now += 1;

    bool p = pumpRunning();
    if (p && !wasPumping) pumpStart = g_now;
    if (p) {
      totalPumpMs++;
      longestPumpRun = std::max(longestPumpRun, g_now - pumpStart);
    }
    wasPumping = p;
    if (state == ALARM) sawAlarm = true;
    if (!putOutAt && W.flameLit && W.flameHealth <= 0) putOutAt = g_now;
    path.push_back({W.x, W.y});
  }
  writeSvg(index, sc, path);

  bool ok = !W.collided && longestPumpRun <= PUMP_MAX_ON_MS;
  std::string detail;
  switch (sc.expect) {
    case Scenario::PUT_OUT:
      ok = ok && putOutAt > 0 && putOutAt < 60000 && !pumpRunning();
      detail = putOutAt ? "put out after " + std::to_string((putOutAt - W.lightAt) / 1000.0).substr(0, 4) +
                              " s, closest " + std::to_string((int)(W.minDistance * 100)) + " cm"
                        : "NOT put out";
      break;
    case Scenario::NEVER_SPRAY:
      ok = ok && totalPumpMs == 0;
      detail = totalPumpMs ? "pump ran!" : "never sprayed";
      break;
    case Scenario::ALARM_STATE:
      ok = ok && sawAlarm && !pumpRunning() && sprayAttempts == MAX_SPRAY_ATTEMPTS;
      detail = sawAlarm ? "gave up after " + std::to_string(sprayAttempts) + " sprays and raised the alarm"
                        : "no alarm";
      break;
  }
  if (W.collided) detail += ", HIT THE CANDLE";
  std::printf("%s  %d. %-40s %s (longest pump run %lu ms)\n", ok ? "PASS" : "FAIL", index + 1,
              sc.name, detail.c_str(), longestPumpRun);
  return ok ? 0 : 1;
}

int main(int argc, char **argv) {
  if (argc > 1) {  // run a single scenario with the robot's Serial output shown
    g_serialEcho = true;
    return runScenario(std::atoi(argv[1]) - 1);
  }
  int failures = 0;
  for (int i = 0; i < N_SCENARIOS; i++) {
    std::fflush(stdout);
    pid_t pid = fork();  // fresh firmware globals for every scenario
    if (pid == 0) std::exit(runScenario(i));
    int status = 0;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) failures++;
  }
  std::printf("\n%d/%d scenarios passed\n", N_SCENARIOS - failures, N_SCENARIOS);
  return failures ? 1 : 0;
}
