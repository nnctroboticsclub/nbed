#include <cstdio>

#include "nbed.h"

// Connect a 3.3 V digital signal, or a switch from PA0 to GND.
// No debounce: mechanical contacts can generate several events per press.
namespace {
volatile uint32_t callback_count = 0;
}

int main() {
  System::Initialize();
  InterruptIn input(kPA0, InterruptEdge::kBoth, GpioPull::kUp);
  input.SetCallback([] { callback_count = callback_count + 1U; });
  if (!input.Initialize()) {
    System::ErrorHandler();
  }
  while (true) {
    std::printf("PA0 level=%u, events=%lu, callbacks=%lu\n",
                static_cast<unsigned>(input.Read()),
                static_cast<unsigned long>(input.GetEdgeCount()),
                static_cast<unsigned long>(callback_count));
    SleepFor(100ms);
  }
}
