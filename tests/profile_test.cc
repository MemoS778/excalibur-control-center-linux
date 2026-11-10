#include "components/ui/profile.hh"
#include <cassert>
#include <sstream>

int main() {
  LightingProfile original{-.25, .72, .35, 0x40, 3, 0};
  std::stringstream data;
  write_profile(data, original);
  LightingProfile restored;
  assert(read_profile(data, restored));
  assert(restored.hue == original.hue &&
         restored.saturation == original.saturation);
  assert(restored.brightness == original.brightness && restored.zone == 0x40);
  assert(restored.pattern == 3 && restored.enabled == 0);
  for (auto text :
       {"", "1 .2 .5", "2 .2 .5 .5 0 1 1", "1 .2 .5 1.5 0 1 1",
        "1 .2 .5 .5 99 1 1", "1 .2 .5 .5 0 99 1", "1 .2 .5 .5 0 1 2",
        "1 nan .5 .5 0 1 1", "1 .2 .5 .5 0 1 1 garbage"}) {
    std::istringstream bad(text);
    restored = original;
    assert(!read_profile(bad, restored));
    assert(restored.hue == original.hue && restored.zone == original.zone);
  }
}
