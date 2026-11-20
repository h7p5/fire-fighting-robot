#pragma once
#include <cstdint>
extern int g_servoAngle;
class Servo {
 public:
  void attach(int) {}
  void write(int angle) { g_servoAngle = angle; }
};
