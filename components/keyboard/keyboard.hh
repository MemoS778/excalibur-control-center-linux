#ifndef KEYBOARD_HH
#define KEYBOARD_HH

#include <chrono>
#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <string>

enum class ZoneID : uint8_t {
  ALL = 0x00,
  LEFT = 0x50,
  MID = 0x40,
  RIGHT = 0x30
};

enum class KBPattern : uint8_t {
  SOLID = 0x01,
  BREATHE = 0x03,
  CYCLE = 0x06,
  AMBILIGHT = 0x07
};

class KeyboardController {
public:
  KeyboardController();
  ~KeyboardController();
  // Duplicate a supplied I/O descriptor for isolated backend tests.
  using Wait = std::function<void(std::chrono::milliseconds)>;
  explicit KeyboardController(int fd, Wait wait = {});
  const std::string &last_error() const { return error_; }

  bool init();
  bool apply(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness, ZoneID zone,
             KBPattern pattern);
  bool set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness,
                 ZoneID zone);
  bool set_mode(KBPattern p);
  bool restore_original();

private:
  static constexpr const char *EC_PATH =
      "/sys/kernel/debug/ec/ec0/io"; // this is linux embedded controller path

  static constexpr uint16_t REG_RED = 0x89;
  static constexpr uint16_t REG_GREEN = 0x8A;
  static constexpr uint16_t REG_BLUE = 0x8B;
  static constexpr uint16_t REG_PWM = 0x64;
  static constexpr uint16_t REG_ZONE = 0xAC;

  static constexpr uint16_t REG_MODE_CMD = 0xC1;
  static constexpr uint16_t REG_MODE_1 = 0xEB;
  static constexpr uint16_t REG_MODE_2 = 0xE2;

  int ec_fd = -1;
  std::mutex ec_mutex;

  KBPattern current_pattern = KBPattern::SOLID;

  struct {
    uint8_t r, g, b, pwm;
  } orig_state{};
  bool have_original_ = false;
  bool mode_known_ = false;
  std::string error_;
  bool io_error(const char *action, uint16_t offset);

  Wait wait_;
  void wait_for(std::chrono::milliseconds delay);
  bool set_color_internal(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness,
                          ZoneID zone);
  bool read_ec(uint16_t offset, uint8_t &value);
  bool write_ec(uint16_t offset, uint8_t val);
  bool trigger_latch(ZoneID zone);
  bool set_mode_internal(KBPattern p);
};

#endif
