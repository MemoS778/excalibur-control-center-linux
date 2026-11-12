#include "components/keyboard/keyboard.hh"
#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

int main() {
  // No init(): never opens the real EC or invokes modprobe.
  KeyboardController absent;
  assert(!absent.set_color(1, 2, 3, 128, ZoneID::ALL));
  assert(!absent.last_error().empty());
  assert(!absent.set_mode(KBPattern::CYCLE));
  FILE *file = tmpfile();
  assert(file);
  int fd = fileno(file);
  assert(ftruncate(fd, 256) == 0);
  auto reg = [fd](int offset) {
    unsigned char byte = 0;
    assert(pread(fd, &byte, 1, offset) == 1);
    return byte;
  };
  {
    KeyboardController keyboard(fd);
    assert(keyboard.set_color(20, 40, 60, 128, ZoneID::LEFT));
    assert(reg(0xEB) == 1 && reg(0xE2) == 1 && reg(0xC1) == 0x11);
    assert(reg(0x89) == 20 && reg(0x8A) == 40 && reg(0x8B) == 60);
    assert(reg(0x64) == 128 && reg(0xAC) == 0x52);
    assert(keyboard.set_mode(KBPattern::CYCLE));
    assert(reg(0xEB) == 6 && reg(0xC1) == 0x61);
    assert(keyboard.set_color(1, 2, 3, 0, ZoneID::ALL));
    assert(reg(0xEB) == 1 && reg(0x64) == 0);
  }
  {
    // Simulate firmware observing the command stream at each settling point.
    struct Observation {
      long delay;
      unsigned char mode, red, latch;
    };
    std::vector<Observation> seen;
    unsigned char sentinel = 99;
    assert(pwrite(fd, &sentinel, 1, 0x89) == 1);
    KeyboardController keyboard(fd, [&](std::chrono::milliseconds delay) {
      seen.push_back({delay.count(), reg(0xEB), reg(0x89), reg(0xAC)});
    });
    assert(keyboard.apply(25, 50, 75, 255, ZoneID::RIGHT, KBPattern::CYCLE));
    assert(seen.size() == 5);
    // Mode settles before RGB data is changed; color latches before animation.
    assert(seen[0].delay == 50 && seen[0].mode == 1 && seen[0].red == 99);
    assert(seen[1].red == 25 && seen[1].latch == 0x31);
    assert(seen[2].latch == 0x38 && seen[3].latch == 0x32);
    assert(seen[4].delay == 50 && seen[4].mode == 6 && seen[4].red == 25);
    seen.clear();
    assert(keyboard.apply(12, 34, 56, 0, ZoneID::ALL, KBPattern::CYCLE));
    assert(seen.size() == 4); // Off must not restart an animated effect.
    assert(reg(0xEB) == 1 && reg(0x64) == 0);
  }
  fclose(file);
  int readonly = open("/dev/null", O_RDONLY);
  assert(readonly >= 0);
  {
    KeyboardController keyboard(readonly);
    assert(!keyboard.set_color(1, 2, 3, 255, ZoneID::ALL));
    assert(!keyboard.last_error().empty());
    assert(!keyboard.apply(1, 2, 3, 255, ZoneID::ALL, KBPattern::CYCLE));
  }
  close(readonly);
}
