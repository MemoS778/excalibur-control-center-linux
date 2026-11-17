#include "keyboard.hh"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

const std::map<std::string, std::array<uint8_t, 3>> preset_colors = {
    {"red", {255, 0, 0}}, {"green", {0, 255, 0}}, {"blue", {0, 0, 255}}};

KeyboardController::KeyboardController() = default;
KeyboardController::KeyboardController(int fd, Wait wait)
    : ec_fd(dup(fd)), wait_(std::move(wait)) {}

void KeyboardController::wait_for(std::chrono::milliseconds delay) {
  if (wait_)
    wait_(delay);
  else
    std::this_thread::sleep_for(delay);
}

KeyboardController::~KeyboardController() {
  if (ec_fd != -1)
    close(ec_fd);
}

bool KeyboardController::init() {
  std::system("modprobe ec_sys write_support=1 2>/dev/null");
  std::ofstream ws("/sys/module/ec_sys/parameters/write_support");
  if (ws) {
    ws << "1";
  }

  if (ec_fd == -1) {
    ec_fd = open(EC_PATH, O_RDWR);
  }

  if (ec_fd == -1) {
    std::cerr
        << "[ERROR] Cannot access Embedded Controller. Please run as root."
        << std::endl;
    return false;
  }

  have_original_ =
      read_ec(REG_RED, orig_state.r) && read_ec(REG_GREEN, orig_state.g) &&
      read_ec(REG_BLUE, orig_state.b) && read_ec(REG_PWM, orig_state.pwm);
  return have_original_;
}

bool KeyboardController::io_error(const char *action, uint16_t offset) {
  int saved_errno = errno;
  std::ostringstream message;
  message << action << " at EC register 0x" << std::hex << offset << ": "
          << (saved_errno ? std::strerror(saved_errno) : "short I/O");
  error_ = message.str();
  std::cerr << "[ERROR] " << error_ << std::endl;
  mode_known_ = false;
  return false;
}

bool KeyboardController::read_ec(uint16_t offset, uint8_t &value) {
  errno = 0;
  if (lseek(ec_fd, offset, SEEK_SET) == -1 || read(ec_fd, &value, 1) != 1)
    return io_error("Read failed", offset);
  return true;
}

bool KeyboardController::write_ec(uint16_t offset, uint8_t value) {
  errno = 0;
  if (lseek(ec_fd, offset, SEEK_SET) == -1 || write(ec_fd, &value, 1) != 1)
    return io_error("Write failed", offset);
  return true;
}

bool KeyboardController::trigger_latch(ZoneID zone) {
  uint8_t base = static_cast<uint8_t>(zone);

  // init byte ends with 0x01
  if (!write_ec(REG_ZONE, base | 0x01))
    return false;
  wait_for(std::chrono::milliseconds(10));

  // commit byte ends with 0x08
  if (!write_ec(REG_ZONE, base | 0x08))
    return false;
  wait_for(std::chrono::milliseconds(10));

  // state byte ends with 0x02
  if (!write_ec(REG_ZONE, base | 0x02))
    return false;
  wait_for(std::chrono::milliseconds(10));
  return true;
}

bool KeyboardController::set_color(uint8_t r, uint8_t g, uint8_t b,
                                   uint8_t brightness, ZoneID zone) {
  std::lock_guard<std::mutex> lock(ec_mutex);

  error_.clear();
  return set_color_internal(r, g, b, brightness, zone);
}

bool KeyboardController::apply(uint8_t r, uint8_t g, uint8_t b,
                               uint8_t brightness, ZoneID zone,
                               KBPattern pattern) {
  // Keep the complete color + animation transaction serialized.
  std::lock_guard<std::mutex> lock(ec_mutex);
  error_.clear();
  if (!set_color_internal(r, g, b, brightness, zone))
    return false;
  return brightness == 0 || pattern == KBPattern::SOLID ||
         set_mode_internal(pattern);
}

bool KeyboardController::set_color_internal(uint8_t r, uint8_t g, uint8_t b,
                                            uint8_t brightness, ZoneID zone) {
  if ((!mode_known_ || current_pattern != KBPattern::SOLID) &&
      !set_mode_internal(KBPattern::SOLID))
    return false;
  return write_ec(REG_RED, r) && write_ec(REG_GREEN, g) &&
         write_ec(REG_BLUE, b) && write_ec(REG_PWM, brightness) &&
         trigger_latch(zone);
}

bool KeyboardController::set_mode_internal(KBPattern p) {
  if (!write_ec(REG_MODE_1, static_cast<uint8_t>(p)) ||
      !write_ec(REG_MODE_2, static_cast<uint8_t>(p)) ||
      !write_ec(REG_MODE_CMD, (static_cast<uint8_t>(p) << 4) | 0x01))
    return false;

  // Conservative settling interval, not a firmware acknowledgement.
  // Without this, the first RGB write can race the asynchronous mode change.
  wait_for(std::chrono::milliseconds(50));
  current_pattern = p;
  mode_known_ = true;
  return true;
}

bool KeyboardController::set_mode(KBPattern p) {
  std::lock_guard<std::mutex> lock(ec_mutex);
  error_.clear();
  return set_mode_internal(p);
}

bool KeyboardController::restore_original() {
  return have_original_ && set_color(orig_state.r, orig_state.g, orig_state.b,
                                     orig_state.pwm, ZoneID::ALL);
}
