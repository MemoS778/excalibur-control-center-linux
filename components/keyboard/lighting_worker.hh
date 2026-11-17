#ifndef LIGHTING_WORKER_HH
#define LIGHTING_WORKER_HH

#include "keyboard.hh"
#include <future>
#include <optional>
#include <tuple>

struct LightingRequest {
  uint8_t r, g, b, brightness;
  ZoneID zone;
  KBPattern pattern;
  bool operator==(const LightingRequest &other) const {
    return std::tie(r, g, b, brightness, zone, pattern) ==
           std::tie(other.r, other.g, other.b, other.brightness,
                    other.zone, other.pattern);
  }
};

// Submit/poll run on the UI thread. Only the supplied operation runs on a
// worker. One active transaction and at most one (latest) pending request.
class LightingWorker {
public:
  using Operation = std::function<std::string(LightingRequest)>;
  explicit LightingWorker(Operation operation) : operation_(std::move(operation)) {}
  ~LightingWorker() { if (result_.valid()) result_.wait(); }
  void submit(LightingRequest request) {
    // Clicking the active setting again also cancels an obsolete queued setting.
    if (active_ && request == *active_) pending_.reset();
    else pending_ = request;
  }
  bool busy() const { return active_.has_value() || pending_.has_value(); }
  std::optional<std::string> poll() {
    std::optional<std::string> completed;
    if (result_.valid()) {
      if (result_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        return {};
      completed = result_.get();
      active_.reset();
    }
    if (pending_) {
      auto request = *pending_;
      pending_.reset();
      try {
        result_ = std::async(std::launch::async, [operation = operation_, request] {
          try { return operation(request); }
          catch (const std::exception &e) { return std::string(e.what()); }
          catch (...) { return std::string("Bilinmeyen donanım işlem hatası."); }
        });
        active_ = request;
      } catch (const std::exception &e) {
        completed = e.what();
      }
    }
    return completed;
  }
private:
  Operation operation_;
  std::optional<LightingRequest> active_, pending_;
  std::future<std::string> result_;
};
#endif
