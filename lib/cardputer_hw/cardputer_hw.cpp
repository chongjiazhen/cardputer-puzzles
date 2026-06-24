#include "cardputer_hw.h"
#include "power.h"   // puz::idleBrightness (resolved via -Isrc/puzzles)
#include "esp_sleep.h"

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
static bool     s_swallowNext = false;                  // first key after a wake only wakes

void begin() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);          // inits M5Unified + the Cardputer/ADV keyboard
  M5.Display.setRotation(1);
  M5.Display.setTextSize(2);
  brightness(g_brightness);
  volume(g_volume);
}

// Light-sleep in short bursts, scanning the keyboard on each wake. We deliberately
// do NOT use GPIO level-wake on the keyboard INT (GPIO11): that line stays LOW until
// the TCA8418 FIFO is read, and the driver already owns an ISR on it, so a level-wake
// re-pends into an interrupt storm -> INT_WDT reset. Timer wake side-steps that.
// CPU/RAM are retained across each nap, so the puzzle survives.
static void enterLightSleep() {
  esp_sleep_enable_timer_wakeup(200000);   // 200ms naps; wake to scan, re-sleep if idle
  do {
    esp_light_sleep_start();
    M5Cardputer.update();
  } while (!(M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()));
  s_lastActivity = millis();            // a key arrived -> activity
  s_swallowNext = true;                 // first key only wakes; don't act on it
  M5.Display.setBrightness(g_brightness);
  s_applied = g_brightness;
}

void update() {
  M5Cardputer.update();
  uint8_t target = puz::idleBrightness(millis() - s_lastActivity, g_brightness,
                                       DIM_MS, OFF_MS, DIM_LEVEL);
  if (target != s_applied) { M5.Display.setBrightness(target); s_applied = target; }

  // Once the screen is fully off (OFF_MS idle), drop the CPU into light-sleep.
  // No USB-power inhibit: this board exposes no usable VBUS signal -- isCharging()
  // reads "unknown" and it runs HWCDC (not TinyUSB), so neither the charge state
  // nor tud_mounted distinguishes USB from battery. Trade-off: a reflash after the
  // device has slept on USB needs the BOOT button (light-sleep drops USB-CDC).
  if (millis() - s_lastActivity >= OFF_MS) enterLightSleep();
}

bool keyPressed() {
  return M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed();
}

std::vector<puz::KeyPress> keysJustPressedEx() {
  if (!(M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()))
    return {};
  bool swallow = s_swallowNext || (s_applied == 0);  // waking from light-sleep or a dark screen
  s_swallowNext = false;
  s_lastActivity = millis();
  auto s = M5Cardputer.Keyboard.keysState();
  if (swallow) return {};            // wake key only wakes; no blind move
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
