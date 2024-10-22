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

  std::cout << "Tests Completed!\n";
  return 0;
}
