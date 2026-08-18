#ifndef CPU_HH
#define CPU_HH

#include <cstdint>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>

struct CpuLimits {
  uint32_t hw_min, hw_max, cur_min, cur_max;
};

class CpuController {
public:
  static constexpr int MAX_CPUS = 48;

  bool set_cpu_freq(uint32_t freq_khz, int core, bool l);

  void set_intel_turbo(bool b);
  bool get_intel_turbo();

  uint8_t get_core_count();

  CpuLimits get_cpu_limits(int core);
};


#endif
