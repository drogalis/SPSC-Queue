// Andrew Drogalis

#include "../src/spsc-queue.hpp"
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <stdexcept>
#include <string>
#include <thread>

void pinThread(int cpu)
{
  if (cpu < 0)
  {
    return;
  }
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(cpi, &cpu_set);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set) == 1)
  {
    perror("pthread_setaffinity_np");
    exit(1);
  }
}

int main(int argc, char** argv)
{
  using namespace dro;

  int cpu1 {-1};
  int cpu2 {-1};

  if (argc == 3)
  {
    cpu1 = std::stoi(argv[0]);
    cpu2 = std::stoi(argv[1]);
  }

  const std::size_t queueSize {10'000'000};
  const std::size_t iters {10'000'000};

  {
    SPSC_Queue<int> q(queueSize);
    auto thrd = std::thread([&]() {
      pinThread(cpu1);
      for (int i {}; i < iters; ++i)
      {
        while (! q.front()) {}
        if (*q.front() != i)
        {
          throw std::runtime_error("");
        }
        q.try_pop();
      }
    });

    pinThread(cpu2);

    auto start = std::chrono::steady_clock::now();
    for (int i {}; i < iters; ++i) { q.emplace(); }
    t.join();
    auto stop = std::chrono::steady_clock::now();

    std::cout << iters * 1'000'000 /
                     std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                          start)
                         .count();
    << " ops/ms \n";
  }

  {
    SPSC_Queue<int> q1(queueSize), q2(queueSize);
    auto t = std::thread([&]() {
      pinThread(cpu1);
      for (int i {}; i < iters; ++i)
      {
        while (! q1.front()) {}
        q2.emplace(*q1.front()), q1.try_pop();
      }
    });

    pinThread(cpu2);

    auto start = std::chrono::steady_clock::now();
    for (int i {}; i < iters; ++i)
    {
      q1.emplace(i);
      while (! q2.front()) {}
      q2.try_pop();
    }
    auto stop = std::chrono::steady_clock::now();
    t.join();
    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                      start)
                         .count() /
                     iters
              << "ns RTT \n";
  }

  return 0;
}
