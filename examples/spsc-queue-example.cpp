#include <dro/spsc-queue.hpp>
#include <thread>

int main(int argc, char* argv[])
{
  int const iter {10};
  dro::SPSC_Queue<int> queue(1);
  auto thrd = std::thread([&] {
    for (int i {}; i < iter; ++i)
    {
      while (! queue.try_pop()) {}
    }
  });

  for (int i {}; i < iter; ++i) { queue.push(i); }
  thrd.join();

  return 0;
}
