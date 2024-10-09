// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// Inspired from Erik Rigtorp
// Significant Modifications / Improvements
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "dro/spsc-queue.hpp"

#if __has_include(<rigtorp/SPSCQueue.h> )
#include <rigtorp/SPSCQueue.h>
#endif

#if __has_include(<boost/lockfree/spsc_queue.hpp> )
#include <boost/lockfree/spsc_queue.hpp>
#endif

#if __has_include(<folly/ProducerConsumerQueue.h>)
#include <folly/ProducerConsumerQueue.h>
#endif

#if __has_include(<readerwriterqueue/readerwriterqueue.h>)
#include <readerwriterqueue/readerwriterqueue.h>
#endif

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <vector>

#include <pthread.h>
#include <sched.h>

void pinThread(int cpu)
{
  if (cpu < 0)
  {
    return;
  }
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(cpu, &cpu_set);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set) == 1)
  {
    perror("pthread_setaffinity_np");
    exit(1);
  }
}

int main(int argc, char* argv[])
{
  int cpu1 {-1};
  int cpu2 {-1};

  if (argc == 3)
  {
    cpu1 = std::stoi(argv[1]);
    cpu2 = std::stoi(argv[2]);
  }

  // Alignas powers of 2 for convenient testing of various sizes
  struct alignas(4) TestSize
  {
    int x_;
    TestSize() = default;
    TestSize(int x) : x_(x) {}
  };

  const std::size_t trialSize {5};
  static_assert(trialSize % 2, "Trial size must be odd");

  const std::size_t queueSize {10'000'000};
  const std::size_t iters {10'000'000};
  std::vector<std::size_t> operations(trialSize);
  std::vector<std::size_t> roundTripTime(trialSize);

  std::cout << "Dro SPSC_Queue: \n";

  for (int i {}; i < trialSize; ++i)
  {
    {
      dro::SPSCQueue<TestSize> queue(queueSize);
      auto thrd = std::thread([&]() {
        pinThread(cpu1);
        for (int i {}; i < iters; ++i)
        {
          TestSize val;
          while (! queue.try_pop(val)) {}
          if (val.x_ != i)
          {
            throw std::runtime_error("Value not equal");
          }
        }
      });

      pinThread(cpu2);

      auto start = std::chrono::steady_clock::now();
      for (int i {}; i < iters; ++i) { queue.emplace(TestSize(i)); }
      thrd.join();
      auto stop = std::chrono::steady_clock::now();

      operations[i] =
          iters * 1'000'000 /
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count();
    }

    {
      dro::SPSCQueue<TestSize> q1(queueSize), q2(queueSize);
      auto thrd = std::thread([&]() {
        pinThread(cpu1);
        for (int i {}; i < iters; ++i)
        {
          TestSize val;
          while (! q1.try_pop(val)) {}
          q2.emplace(val);
        }
      });

      pinThread(cpu2);

      auto start = std::chrono::steady_clock::now();
      for (int i {}; i < iters; ++i)
      {
        q1.emplace(TestSize(i));
        TestSize val;
        while (! q2.try_pop(val)) {}
      }
      auto stop = std::chrono::steady_clock::now();
      thrd.join();
      roundTripTime[i] =
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count() /
          iters;
    }
  }

  std::sort(operations.begin(), operations.end());
  std::sort(roundTripTime.begin(), roundTripTime.end());
  std::cout << "Mean: "
            << std::accumulate(operations.begin(), operations.end(), 0) /
                   trialSize
            << " ops/ms \n";
  std::cout << "Median: " << operations[trialSize / 2] << " ops/ms \n";

  std::cout << "Mean: "
            << std::accumulate(roundTripTime.begin(), roundTripTime.end(), 0) /
                   trialSize
            << " ns RTT \n";
  std::cout << "Median: " << roundTripTime[trialSize / 2] << " ns RTT \n";

#if __has_include(<rigtorp/SPSCQueue.h> )

  std::cout << "Rigtorp SPSCQueue:" << std::endl;

  for (int i {}; i < trialSize; ++i)
  {
    {
      rigtorp::SPSCQueue<TestSize> q(queueSize);
      auto t = std::thread([&] {
        pinThread(cpu1);
        for (int i = 0; i < iters; ++i)
        {
          while (! q.front());
          if (q.front()->x_ != i)
          {
            throw std::runtime_error("");
          }
          q.pop();
        }
      });

      pinThread(cpu2);

      auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < iters; ++i) { q.emplace(TestSize(i)); }
      t.join();
      auto stop = std::chrono::steady_clock::now();
      operations[i] =
          iters * 1000000 /
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count();
    }

    {
      rigtorp::SPSCQueue<TestSize> q1(queueSize), q2(queueSize);
      auto t = std::thread([&] {
        pinThread(cpu1);
        for (int i = 0; i < iters; ++i)
        {
          while (! q1.front());
          q2.emplace(*(q1.front()));
          q1.pop();
        }
      });

      pinThread(cpu2);

      auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < iters; ++i)
      {
        q1.emplace(TestSize(i));
        while (! q2.front());
        q2.pop();
      }
      auto stop = std::chrono::steady_clock::now();
      t.join();
      roundTripTime[i] =
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count() /
          iters;
    }
  }
  std::sort(operations.begin(), operations.end());
  std::sort(roundTripTime.begin(), roundTripTime.end());
  std::cout << "Mean: "
            << std::accumulate(operations.begin(), operations.end(), 0) /
                   trialSize
            << " ops/ms \n";

  std::cout << "Median: " << operations[trialSize / 2] << " ops/ms \n";
  std::cout << "Mean: "
            << std::accumulate(roundTripTime.begin(), roundTripTime.end(), 0) /
                   trialSize
            << " ns RTT \n";
  std::cout << "Median: " << roundTripTime[trialSize / 2] << " ns RTT \n";

#endif

#if __has_include(<boost/lockfree/spsc_queue.hpp> )
  std::cout << "boost::lockfree::spsc:" << std::endl;
  for (int i {}; i < trialSize; ++i)
  {
    {
      boost::lockfree::spsc_queue<TestSize> q(queueSize);
      auto t = std::thread([&] {
        pinThread(cpu1);
        for (int i = 0; i < iters; ++i)
        {
          TestSize val;
          while (q.pop(&val, 1) != 1);
          if (val.x_ != i)
          {
            throw std::runtime_error("");
          }
        }
      });

      pinThread(cpu2);

      auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < iters; ++i) { while (! q.push(TestSize(i))); }
      t.join();
      auto stop = std::chrono::steady_clock::now();
      operations[i] =
          iters * 1000000 /
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count();
    }

    {
      boost::lockfree::spsc_queue<TestSize> q1(queueSize), q2(queueSize);
      auto t = std::thread([&] {
        pinThread(cpu1);
        for (int i = 0; i < iters; ++i)
        {
          TestSize val;
          while (q1.pop(&val, 1) != 1);
          while (! q2.push(val));
        }
      });

      pinThread(cpu2);

      auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < iters; ++i)
      {
        while (! q1.push(TestSize(i)));
        TestSize val;
        while (q2.pop(&val, 1) != 1);
      }
      auto stop = std::chrono::steady_clock::now();
      t.join();
      roundTripTime[i] =
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count() /
          iters;
    }
  }

  std::sort(operations.begin(), operations.end());
  std::sort(roundTripTime.begin(), roundTripTime.end());
  std::cout << "Mean: "
            << std::accumulate(operations.begin(), operations.end(), 0) /
                   trialSize
            << " ops/ms \n";
  std::cout << "Median: " << operations[trialSize / 2] << " ops/ms \n";

  std::cout << "Mean: "
            << std::accumulate(roundTripTime.begin(), roundTripTime.end(), 0) /
                   trialSize
            << " ns RTT \n";
  std::cout << "Median: " << roundTripTime[trialSize / 2] << " ns RTT \n";

#endif

#if __has_include(<folly/ProducerConsumerQueue.h>)
  std::cout << "folly::ProducerConsumerQueue:" << std::endl;
  for (int i {}; i < trialSize; ++i)
  {
    {
      folly::ProducerConsumerQueue<TestSize> q(queueSize);
      auto t = std::thread([&] {
        pinThread(cpu1);
        for (int i = 0; i < iters; ++i)
        {
          TestSize val;
          while (! q.read(val));
          if (val.x_ != i)
          {
            throw std::runtime_error("");
          }
        }
      });

      pinThread(cpu2);

      auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < iters; ++i) { while (! q.write(TestSize(i))); }
      t.join();
      auto stop = std::chrono::steady_clock::now();
      operations[i] =
          iters * 1000000 /
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count();
    }

    {
      folly::ProducerConsumerQueue<TestSize> q1(queueSize), q2(queueSize);
      auto t = std::thread([&] {
        pinThread(cpu1);
        for (int i = 0; i < iters; ++i)
        {
          TestSize val;
          while (! q1.read(val));
          q2.write(val);
        }
      });

      pinThread(cpu2);

      auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < iters; ++i)
      {
        while (! q1.write(TestSize(i)));
        TestSize val;
        while (! q2.read(val));
      }
      auto stop = std::chrono::steady_clock::now();
      t.join();
      roundTripTime[i] =
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count() /
          iters;
    }
  }

  std::sort(operations.begin(), operations.end());
  std::sort(roundTripTime.begin(), roundTripTime.end());
  std::cout << "Mean: "
            << std::accumulate(operations.begin(), operations.end(), 0) /
                   trialSize
            << " ops/ms \n";
  std::cout << "Median: " << operations[trialSize / 2] << " ops/ms \n";

  std::cout << "Mean: "
            << std::accumulate(roundTripTime.begin(), roundTripTime.end(), 0) /
                   trialSize
            << " ns RTT \n";
  std::cout << "Median: " << roundTripTime[trialSize / 2] << " ns RTT \n";

#endif

#if __has_include(<readerwriterqueue/readerwriterqueue.h>)
  std::cout << "moodycamel::ReaderWriterQueue" << std::endl;
  for (int i {}; i < trialSize; ++i)
  {
    {
      moodycamel::ReaderWriterQueue<TestSize> q(queueSize);
      auto t = std::thread([&] {
        pinThread(cpu1);
        for (int i = 0; i < iters; ++i)
        {
          TestSize val;
          while (! q.peek());
          q.try_dequeue(val);
          if (val.x_ != i)
          {
            throw std::runtime_error("");
          }
        }
      });

      pinThread(cpu2);

      auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < iters; ++i) { q.try_enqueue(TestSize(i)); }
      t.join();
      auto stop = std::chrono::steady_clock::now();
      operations[i] =
          iters * 1000000 /
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count();
    }

    {
      moodycamel::ReaderWriterQueue<TestSize> q1(queueSize), q2(queueSize);
      auto t = std::thread([&] {
        pinThread(cpu1);
        for (int i = 0; i < iters; ++i)
        {
          TestSize val;
          while (! q1.peek());
          q1.try_dequeue(val);
          q2.try_enqueue(val);
        }
      });

      pinThread(cpu2);

      auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < iters; ++i)
      {
        q1.try_enqueue(TestSize(i));
        TestSize val;
        while (! q2.peek());
        q2.try_dequeue(val);
      }
      auto stop = std::chrono::steady_clock::now();
      t.join();
      roundTripTime[i] =
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count() /
          iters;
    }
  }

  std::sort(operations.begin(), operations.end());
  std::sort(roundTripTime.begin(), roundTripTime.end());
  std::cout << "Mean: "
            << std::accumulate(operations.begin(), operations.end(), 0) /
                   trialSize
            << " ops/ms \n";
  std::cout << "Median: " << operations[trialSize / 2] << " ops/ms \n";

  std::cout << "Mean: "
            << std::accumulate(roundTripTime.begin(), roundTripTime.end(), 0) /
                   trialSize
            << " ns RTT \n";
  std::cout << "Median: " << roundTripTime[trialSize / 2] << " ns RTT \n";

#endif

  return 0;
}
