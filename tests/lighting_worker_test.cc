#include "components/keyboard/lighting_worker.hh"
#include <cassert>
#include <thread>
#include <vector>

int main() {
  const LightingRequest first{255, 0, 0, 128, ZoneID::ALL, KBPattern::SOLID};
  auto second = first;
  second.pattern = KBPattern::BREATHE;
  auto last = first;
  last.pattern = KBPattern::CYCLE;
  std::promise<void> started, release;
  auto gate = release.get_future().share();
  std::vector<LightingRequest> writes;
  LightingWorker worker([&](LightingRequest request) {
    writes.push_back(request);
    if (writes.size() == 1) {
      started.set_value();
      gate.wait();
    }
    return request.pattern == KBPattern::CYCLE ? std::string("I/O failed") : std::string{};
  });
  worker.submit(first);
  assert(!worker.poll()); // Starts asynchronously, does not wait for hardware.
  started.get_future().wait();
  for (int i = 0; i < 100; ++i) {
    worker.submit(first);
    assert(!worker.poll()); // UI can keep processing events while hardware waits.
  }
  worker.submit(second);
  worker.submit(last); // Only the last queued request may reach hardware.
  release.set_value();
  std::vector<std::string> results;
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (worker.busy() && std::chrono::steady_clock::now() < deadline) {
    if (auto result = worker.poll()) results.push_back(*result);
    std::this_thread::yield();
  }
  assert(!worker.busy());
  assert(writes.size() == 2 && writes[0] == first && writes[1] == last);
  assert(results.size() == 2 && results[0].empty() && results[1] == "I/O failed");

  LightingWorker failing([](LightingRequest) -> std::string {
    throw std::runtime_error("worker exception");
  });
  failing.submit(first);
  bool reported = false;
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (failing.busy() && std::chrono::steady_clock::now() < deadline) {
    if (auto result = failing.poll()) reported = *result == "worker exception";
    std::this_thread::yield();
  }
  assert(reported && !failing.busy());

  // Returning to the active selection removes an obsolete pending selection.
  std::promise<void> unblock;
  auto blocked = unblock.get_future().share();
  int count = 0;
  {
    LightingWorker closing([&](LightingRequest) {
      blocked.wait();
      ++count;
      return std::string{};
    });
    closing.submit(first);
    closing.poll();
    closing.submit(second);
    closing.submit(first);
    unblock.set_value();
    // Destruction joins the active operation; queued work is not started.
  }
  assert(count == 1);
}
