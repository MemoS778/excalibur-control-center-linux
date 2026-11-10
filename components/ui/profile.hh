#ifndef RGB_PROFILE_HH
#define RGB_PROFILE_HH
#include <cmath>
#include <istream>
#include <ostream>

// A preset describes one selected zone (or all zones), not live hardware state.
struct LightingProfile {
  double hue = .07, saturation = .85, brightness = .5;
  int zone = 0, pattern = 1, enabled = 1;
};
inline bool read_profile(std::istream &in, LightingProfile &out) {
  int version;
  LightingProfile p;
  if (!(in >> version >> p.hue >> p.saturation >> p.brightness >> p.zone >>
        p.pattern >> p.enabled))
    return false;
  if (version != 1 || !std::isfinite(p.hue) || !std::isfinite(p.saturation) ||
      !std::isfinite(p.brightness) || p.hue < -1 || p.hue > 1 ||
      p.saturation < 0 || p.saturation > 1 || p.brightness < 0 ||
      p.brightness > 1 ||
      (p.zone != 0 && p.zone != 0x50 && p.zone != 0x40 && p.zone != 0x30) ||
      (p.pattern != 1 && p.pattern != 3 && p.pattern != 6 && p.pattern != 7) ||
      (p.enabled != 0 && p.enabled != 1))
    return false;
  in >> std::ws;
  if (!in.eof())
    return false;
  out = p;
  return true;
}
inline void write_profile(std::ostream &out, const LightingProfile &p) {
  out.precision(17);
  out << "1 " << p.hue << ' ' << p.saturation << ' ' << p.brightness << ' '
      << p.zone << ' ' << p.pattern << ' ' << p.enabled << '\n';
}
#endif
