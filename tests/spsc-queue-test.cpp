// Copyright (c) 2024 Andrew Drogalis
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

#include <cassert>   // for assert
#include <iostream>  // for operator<<, basic_ostream, char_traits, cout
#include <memory>    // for std::unique_ptr
#include <stdexcept> // for std::logic_error

#include <dro/spsc-queue.hpp> // for dro::SPSCQueue

int main(int argc, char *argv[]) {

  // Functional Tests Emplace
  {
    const int size{10};
    dro::SPSCQueue<int> queue{size};
    int val{};
    assert(!queue.try_pop(val));
    assert(!queue.size());
    assert(queue.empty());
    assert(queue.capacity() == 10);
    for (int i = 0; i < size; i++) {
      queue.emplace(i);
    }
    assert(queue.try_pop(val));
    assert(queue.size() == 9);
    assert(!queue.empty());
    assert(queue.try_emplace(1));
    assert(!queue.try_emplace(1));
    assert(queue.size() == 10);
    queue.pop(val);
    assert(queue.size() == 9);
    int forceVal{10};
    for (int i{}; i < size; i++) {
      queue.force_emplace(forceVal);
    }
    assert(queue.try_pop(val));
    assert(val == forceVal);
  }

  // Functional Tests Push
  {
    const int size{10};
    dro::SPSCQueue<int> queue{size};
    int val{};
    assert(!queue.try_pop(val));
    for (int i{}; i < size; i++) {
      queue.push(i);
    }
    assert(queue.try_pop(val));
    assert(queue.size() == 9);
    assert(!queue.empty());
    assert(queue.try_push(1));
    assert(!queue.try_push(1));
    assert(queue.size() == 10);
    queue.pop(val);
    assert(queue.size() == 9);
    int forceVal{10};
    for (int i{}; i < size; i++) {
      queue.force_push(forceVal);
    }
    assert(queue.try_pop(val));
    assert(val == forceVal);
  }

  // Stack Allocated Queue
  {
    const int size{10};
    dro::SPSCQueue<int, size> queue;
    int val{};
    assert(!queue.try_pop(val));
    assert(!queue.size());
    assert(queue.empty());
    assert(queue.capacity() == 10);
    for (int i{}; i < size; i++) {
      queue.push(i);
    }
    assert(queue.try_pop(val));
    assert(queue.size() == 9);
    assert(!queue.empty());
    assert(queue.try_push(1));
    assert(!queue.try_push(1));
    assert(queue.size() == 10);
    queue.pop(val);
    assert(queue.size() == 9);
    int forceVal{10};
    for (int i{}; i < size; i++) {
      queue.force_push(forceVal);
    }
    assert(queue.try_pop(val));
    assert(val == forceVal);
  }

  // Copyable Only Object
  {
    struct Test {
      int x_;
      int y_;
      Test() = default;
      Test(int x, int y) : x_(x), y_(y) {}
      ~Test() = default;
      Test(const Test &) = default;
      Test &operator=(const Test &) = default;
      Test(Test &&) = delete;
      Test &operator=(Test &&) = delete;
    };
    const int size{10};
    dro::SPSCQueue<Test> queue{size};
    Test val{};
    queue.emplace(5, 0);
    queue.try_emplace(5, 0);
    queue.force_emplace(val);
    queue.push(val);
    bool discard = queue.try_push(val);
    queue.force_emplace(val);
    assert(queue.size() == 6);
    static_assert(noexcept(queue.emplace(val)));
    static_assert(noexcept(queue.try_emplace(val)));
    static_assert(noexcept(queue.force_emplace(val)));
    static_assert(noexcept(queue.push(val)));
    static_assert(noexcept(queue.try_push(val)));
    static_assert(noexcept(queue.force_push(val)));
    // Test R-Value
    queue.push(Test());
    discard = queue.try_push(Test());
    queue.force_push(Test());
    static_assert(noexcept(queue.push(Test())));
    static_assert(noexcept(queue.try_push(Test())));
    static_assert(noexcept(queue.force_push(Test())));
    assert(queue.size() == 9);
    // Try Pop
    discard = queue.try_pop(val);
    assert(val.x_ == 5);
    assert(val.y_ == 0);
    assert(queue.size() == 8);
  }

  // Moveable Only Object
  {
    const int size{10};
    dro::SPSCQueue<std::unique_ptr<int>> queue{size};
    queue.emplace(std::make_unique<int>(1));
    queue.try_emplace(std::make_unique<int>(1));
    queue.push(std::make_unique<int>(1));
    bool discard = queue.try_push(std::make_unique<int>(1));
    assert(queue.size() == 4);
    auto moveable = std::make_unique<int>(1);
    static_assert(noexcept(queue.emplace(std::move(moveable))));
    static_assert(noexcept(queue.try_emplace(std::move(moveable))));
    static_assert(noexcept(queue.push(std::move(moveable))));
    static_assert(noexcept(queue.try_push(std::move(moveable))));
    // Pop
    std::unique_ptr<int> val;
    discard = queue.try_pop(val);
    assert(queue.size() == 3);
    assert(*val.get() == 1);
  }

  // Constructor Exception
  {
    try {
      dro::SPSCQueue<int> queue(0);
      assert(false); // Should never be called
    } catch (std::logic_error &e) {
      assert(true); // Should always be called
    }
  }
  {
    try {
      dro::SPSCQueue<int, 10> queue(10);
      assert(false); // Should never be called
    } catch (std::invalid_argument &e) {
      assert(true); // Should always be called
    }
  }

  // Wraparound Boundary Testing
  {
    const int size{5};
    dro::SPSCQueue<int> queue{size};
    int val{};

    // Fill the queue
    for (int i = 0; i < size; i++) {
      queue.push(i);
    }
    assert(queue.size() == size);

    // Pop all elements
    for (int i = 0; i < size; i++) {
      queue.pop(val);
      assert(val == i);
    }
    assert(queue.empty());

    // Fill again to test wraparound
    for (int i = 10; i < 10 + size; i++) {
      queue.push(i);
    }
    assert(queue.size() == size);

    // Pop all again
    for (int i = 10; i < 10 + size; i++) {
      queue.try_pop(val);
      assert(val == i);
    }
    assert(queue.empty());

    // Multiple wrap cycles
    for (int cycle = 0; cycle < 3; cycle++) {
      for (int i = 0; i < size; i++) {
        queue.push(cycle * 100 + i);
      }
      for (int i = 0; i < size; i++) {
        queue.pop(val);
        assert(val == cycle * 100 + i);
      }
    }
    assert(queue.empty());
  }

  // Concurrent Edge Cases - Full Queue Boundary
  {
    const int size{3};
    dro::SPSCQueue<int> queue{size};
    int val{};

    // Fill to exactly full
    for (int i = 0; i < size; i++) {
      assert(queue.try_push(i));
    }
    assert(queue.size() == size);
    assert(!queue.try_push(999)); // Should fail when full

    // Pop one, push one (at boundary)
    assert(queue.try_pop(val));
    assert(val == 0);
    assert(queue.try_push(100));
    assert(queue.size() == size);

    // Verify FIFO order maintained
    assert(queue.try_pop(val));
    assert(val == 1);
    assert(queue.try_pop(val));
    assert(val == 2);
    assert(queue.try_pop(val));
    assert(val == 100);
    assert(queue.empty());
  }

  // Rapid Alternating Push/Pop
  {
    const int size{10};
    dro::SPSCQueue<int> queue{size};
    int val{};

    for (int i = 0; i < 100; i++) {
      queue.push(i);
      queue.pop(val);
      assert(val == i);
      assert(queue.empty());
    }
  }

  // Size Accuracy During Wraparound
  {
    const int size{4};
    dro::SPSCQueue<int> queue{size};
    int val{};

    // Fill partially
    queue.push(1);
    queue.push(2);
    assert(queue.size() == 2);

    // Pop and push to cause wraparound
    queue.pop(val);
    queue.pop(val);
    assert(queue.size() == 0);

    for (int i = 0; i < size; i++) {
      queue.push(i);
    }
    assert(queue.size() == size);

    // Pop half
    queue.pop(val);
    queue.pop(val);
    assert(queue.size() == 2);

    // Push to wrap
    queue.push(100);
    queue.push(101);
    assert(queue.size() == 4);
  }

  // Force Operations Overwrite Behavior
  {
    const int size{3};
    dro::SPSCQueue<int> queue{size};
    int val{};

    // Fill the queue
    for (int i = 0; i < size; i++) {
      queue.push(i);
    }
    assert(queue.size() == size);

    // Force push should succeed even when full
    queue.force_push(100);
    // Note: force operations don't update indices to indicate "full"
    // They just overwrite, so we can still read

    // Force emplace multiple times
    queue.force_emplace(200);
    queue.force_emplace(300);

    // Verify we can still pop (though order may be affected by overwrites)
    bool popSuccess = queue.try_pop(val);
    assert(popSuccess);
  }

  // Comprehensive Pop Method Testing
  {
    const int size{5};
    dro::SPSCQueue<int> queue{size};
    int val{};

    // Add some elements
    for (int i = 0; i < 3; i++) {
      queue.push(i * 10);
    }

    // Pop using blocking pop
    queue.pop(val);
    assert(val == 0);
    assert(queue.size() == 2);

    queue.pop(val);
    assert(val == 10);

    queue.pop(val);
    assert(val == 20);
    assert(queue.empty());

    // Pop with wraparound
    for (int i = 0; i < size; i++) {
      queue.push(i);
    }
    for (int i = 0; i < size; i++) {
      queue.pop(val);
      assert(val == i);
    }
    assert(queue.empty());
  }

  // Large Object Testing
  {
    struct LargeObject {
      int data[100];
      double values[50];
      char buffer[256];

      LargeObject() {
        data[0] = 0;
        values[0] = 0.0;
        buffer[0] = '\0';
      }

      LargeObject(int id) {
        data[0] = id;
        values[0] = id * 1.5;
        buffer[0] = 'A' + (id % 26);
      }
    };

    const int size{5};
    dro::SPSCQueue<LargeObject> queue{size};
    LargeObject val;

    // Push and pop large objects
    for (int i = 0; i < size; i++) {
      queue.emplace(i);
    }

    for (int i = 0; i < size; i++) {
      queue.pop(val);
      assert(val.data[0] == i);
      assert(val.values[0] == i * 1.5);
      assert(val.buffer[0] == 'A' + (i % 26));
    }
    assert(queue.empty());
  }

  // Edge Case Capacities - Minimum
  {
    const int size{1};
    dro::SPSCQueue<int> queue{size};
    int val{};

    assert(queue.capacity() == 1);
    assert(queue.empty());

    queue.push(42);
    assert(queue.size() == 1);
    assert(!queue.try_push(99)); // Full

    queue.pop(val);
    assert(val == 42);
    assert(queue.empty());
  }

  // Edge Case Capacities - Large Heap
  {
    const int size{10000};
    dro::SPSCQueue<int> queue{size};
    int val{};

    assert(queue.capacity() == size);

    // Fill partially
    for (int i = 0; i < 1000; i++) {
      queue.push(i);
    }
    assert(queue.size() == 1000);

    // Pop all
    for (int i = 0; i < 1000; i++) {
      queue.pop(val);
      assert(val == i);
    }
    assert(queue.empty());
  }

  // Edge Case Capacities - Stack Allocation Sizes
  {
    dro::SPSCQueue<int, 100> smallStack;
    assert(smallStack.capacity() == 100);

    dro::SPSCQueue<int, 1000> mediumStack;
    assert(mediumStack.capacity() == 1000);

    dro::SPSCQueue<char, 10000> largeStack;
    assert(largeStack.capacity() == 10000);
  }

  // Empty Queue Operations
  {
    const int size{5};
    dro::SPSCQueue<int> queue{size};
    int val{};

    assert(queue.empty());
    assert(queue.size() == 0);

    // Multiple try_pop on empty
    assert(!queue.try_pop(val));
    assert(!queue.try_pop(val));
    assert(!queue.try_pop(val));
    assert(queue.empty());
    assert(queue.size() == 0);

    // Add one, remove one, verify empty again
    queue.push(1);
    assert(!queue.empty());
    queue.pop(val);
    assert(queue.empty());
    assert(queue.size() == 0);

    // Multiple try_pop again
    assert(!queue.try_pop(val));
    assert(!queue.try_pop(val));
  }

  // Full Queue Operations
  {
    const int size{3};
    dro::SPSCQueue<int> queue{size};

    // Fill completely
    for (int i = 0; i < size; i++) {
      assert(queue.try_push(i));
    }
    assert(queue.size() == size);

    // Multiple try_push when full
    assert(!queue.try_push(100));
    assert(!queue.try_push(101));
    assert(!queue.try_push(102));
    assert(queue.size() == size);

    // Same with try_emplace
    assert(!queue.try_emplace(200));
    assert(!queue.try_emplace(201));
    assert(queue.size() == size);
  }

  // Interleaved Operations
  {
    const int size{10};
    dro::SPSCQueue<int> queue{size};
    int val{};

    // push-push-pop-push-pop-pop pattern
    queue.push(1);
    queue.push(2);
    queue.pop(val);
    assert(val == 1);
    queue.push(3);
    queue.pop(val);
    assert(val == 2);
    queue.pop(val);
    assert(val == 3);
    assert(queue.empty());

    // More complex interleaving
    queue.push(10);
    queue.push(20);
    queue.push(30);
    queue.pop(val);
    assert(val == 10);
    queue.push(40);
    queue.push(50);
    queue.pop(val);
    assert(val == 20);
    queue.pop(val);
    assert(val == 30);
    queue.push(60);
    queue.pop(val);
    assert(val == 40);
    queue.pop(val);
    assert(val == 50);
    queue.pop(val);
    assert(val == 60);
    assert(queue.empty());
  }

  // Size Reporting Accuracy at Various Fill Levels
  {
    const int size{100};
    dro::SPSCQueue<int> queue{size};

    // 0%
    assert(queue.size() == 0);
    assert(queue.empty());

    // 25%
    for (int i = 0; i < size / 4; i++) {
      queue.push(i);
    }
    assert(queue.size() == size / 4);

    // 50%
    for (int i = size / 4; i < size / 2; i++) {
      queue.push(i);
    }
    assert(queue.size() == size / 2);

    // 75%
    for (int i = size / 2; i < (3 * size) / 4; i++) {
      queue.push(i);
    }
    assert(queue.size() == (3 * size) / 4);

    // 100%
    for (int i = (3 * size) / 4; i < size; i++) {
      queue.push(i);
    }
    assert(queue.size() == size);

    // Pop half
    int val{};
    for (int i = 0; i < size / 2; i++) {
      queue.pop(val);
    }
    assert(queue.size() == size / 2);

    // Back to 100% with wraparound
    for (int i = 0; i < size / 2; i++) {
      queue.push(i + 1000);
    }
    assert(queue.size() == size);

    // Verify FIFO order after wraparound
    for (int i = size / 2; i < size; i++) {
      queue.pop(val);
      assert(val == i);
    }
    for (int i = 0; i < size / 2; i++) {
      queue.pop(val);
      assert(val == i + 1000);
    }
    assert(queue.empty());
  }

  // Type Trait Tests - Copy Only (already covered above)
  // Type Trait Tests - Move Only (already covered with unique_ptr)
  // Type Trait Tests - Trivial Type
  {
    const int size{5};
    dro::SPSCQueue<int> trivialQueue{size};
    int val{};

    for (int i = 0; i < size; i++) {
      trivialQueue.push(i);
    }

    for (int i = 0; i < size; i++) {
      trivialQueue.pop(val);
      assert(val == i);
    }
    assert(trivialQueue.empty());
  }

  // Type Trait Tests - Non-Trivial Type
  {
    struct NonTrivial {
      std::vector<int> data;
      NonTrivial() : data{1, 2, 3} {}
      NonTrivial(int n) : data(n, 42) {}
    };

    const int size{5};
    dro::SPSCQueue<NonTrivial> queue{size};
    NonTrivial val;

    queue.emplace(10);
    queue.push(NonTrivial(5));

    queue.pop(val);
    assert(val.data.size() == 10);
    assert(val.data[0] == 42);

    queue.pop(val);
    assert(val.data.size() == 5);
    assert(val.data[0] == 42);

    assert(queue.empty());
  }

  std::cout << "All Tests Completed!\n";
  return 0;
}
