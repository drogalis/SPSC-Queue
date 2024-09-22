#include <dro/spsc-queue.hpp>
#include <thread>

int main(int argc, char* argv[])
{
  dro::SPSC_Queue<int> queue(1);
  auto thrd = std::thread([&] {
    while (! queue.front()) {}
    queue.try_pop();
  });
  thrd.join();

  queue.push(1);

  return 0;
}
