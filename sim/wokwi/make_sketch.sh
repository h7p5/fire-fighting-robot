#!/bin/sh
# Builds sim/wokwi/sketch.ino = config.h + the robot firmware in ONE file,
# so it can be pasted into a Wokwi project in one go. The robot code is unchanged.
cd "$(dirname "$0")/../.." || exit 1
{
  echo "// ===== Generated for the Wokwi simulator from firmware/fire_fighting_robot ====="
  echo "// ===== (config.h + fire_fighting_robot.ino). Edit those files, not this one. ====="
  grep -v '^#pragma once' firmware/fire_fighting_robot/config.h
  echo
  grep -v '#include "config.h"' firmware/fire_fighting_robot/fire_fighting_robot.ino
} > sim/wokwi/sketch.ino
echo "wrote sim/wokwi/sketch.ino ($(wc -l < sim/wokwi/sketch.ino) lines)"
