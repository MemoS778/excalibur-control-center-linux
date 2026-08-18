#include "cpu.hh"


void CpuController::set_intel_turbo(bool b)
{
  if(b) { std::system("echo 0 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo"); }
  else  { std::system("echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo"); }
}

bool CpuController::set_cpu_freq(uint32_t freq_khz, int core, bool l)
{
  std::string target = l ? "scaling_min_freq" : "scaling_max_freq";

  if(core < 0)
  {
    std::string cmd = "for cpu in /sys/devices/system/cpu/cpu*/cpufreq/; do "
                      "echo " + std::to_string(freq_khz) + " > $cpu/" + target + "; done";
    return std::system(cmd.c_str()) == 0;
  }

  std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(core) + "/cpufreq/" + target;
  std::ofstream f(path);
  if(!f) return false;
  f << freq_khz;
  return true;
}

CpuLimits CpuController::get_cpu_limits(int core)
{
  CpuLimits limits{0, 0, 0, 0};
  std::string base = "/sys/devices/system/cpu/cpu" + std::to_string(core) + "/cpufreq/";

  std::ifstream(base + "cpuinfo_min_freq") >> limits.hw_min;
  std::ifstream(base + "cpuinfo_max_freq") >> limits.hw_max;
  std::ifstream(base + "scaling_min_freq") >> limits.cur_min;
  std::ifstream(base + "scaling_max_freq") >> limits.cur_max;

  return limits;
}

uint8_t CpuController::get_core_count()
{
  uint8_t count = 0;
  for(int i = 0; i < MAX_CPUS; ++i)
  {
    std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_cur_freq";
    std::ifstream f(path);
    if(!f) { break; }
    count++;
  }

  return count;
}

bool CpuController::get_intel_turbo()
{
  std::ifstream file("/sys/devices/system/cpu/intel_pstate/no_turbo");
  if(!file.is_open()) { return false; }
  std::string v;
  std::getline(file, v);
  return v == "0";
}
