#include "cardputer_hw.h"
#include "power.h"   // puz::idleBrightness (resolved via -Isrc/puzzles)

namespace cardputer {

static uint8_t g_brightness = 128;
static uint8_t g_volume = 128;

// Idle backlight power-save. update() dims the screen after inactivity;
// any key (via keysJustPressedEx) restores it. Encapsulated here so app code
// needs no changes — update() and keysJustPressedEx() are already on every path.
static const uint32_t DIM_MS = 30000, OFF_MS = 90000;   // dim at 30s, off at 90s
static const uint8_t  DIM_LEVEL = 20;                   // low-but-visible
static uint32_t s_lastActivity = 0;
static uint8_t  s_applied = 128;                        // brightness currently on screen

void begin() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);          // inits M5Unified + the Cardputer/ADV keyboard
  M5.Display.setRotation(1);
  M5.Display.setTextSize(2);
  brightness(g_brightness);
  volume(g_volume);
}

void update() {
  M5Cardputer.update();
  uint8_t target = puz::idleBrightness(millis() - s_lastActivity, g_brightness,
                                       DIM_MS, OFF_MS, DIM_LEVEL);
  if (target != s_applied) { M5.Display.setBrightness(target); s_applied = target; }
}

bool keyPressed() {
  return M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed();
}

std::vector<puz::KeyPress> keysJustPressedEx() {
  if (!(M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()))
    return {};
  bool wasOff = (s_applied == 0);   // screen fully off -> this key only wakes it
  s_lastActivity = millis();
  auto s = M5Cardputer.Keyboard.keysState();
  if (wasOff) return {};            // swallow the wake key (no blind move on a dark screen)
  return puz::buildKeyPresses(s.word, s.ctrl, s.enter, s.del, s.tab);
}

void brightness(uint8_t level) {
  g_brightness = level; M5.Display.setBrightness(level);
  s_applied = level; s_lastActivity = millis();   // user set it -> that's activity
}
uint8_t brightness() { return g_brightness; }

void volume(uint8_t level) { g_volume = level; M5.Speaker.setVolume(level); }
uint8_t volume() { return g_volume; }

bool imuRead(Imu& out) {
  if (!M5.Imu.isEnabled()) return false;
  M5.Imu.update();
  auto d = M5.Imu.getImuData();
  out = {d.accel.x, d.accel.y, d.accel.z, d.gyro.x, d.gyro.y, d.gyro.z};
  return true;
}

Battery battery() {
  // getBatteryLevel: 0..100, or -1 when the gauge can't report. isCharging
  // returns the is_charging_t enum; is_charging == 1.
  return { (int)M5.Power.getBatteryLevel(), M5.Power.isCharging() == 1 };
}

}  // namespace cardputer
