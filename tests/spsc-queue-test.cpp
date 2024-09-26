// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// Inspired from Erik Rigtorp
// Significant Modifications / Improvements
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <dro/spsc-queue.hpp>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>

int main(int argc, char* argv[])
{

  // Functional Tests
  {
    dro::SPSCQueue<int> queue {10};
    int val {};
    assert(! queue.try_pop(val));
    assert(! queue.size());
    assert(queue.empty());
    assert(queue.capacity() == 10);
    for (int i = 0; i < 10; i++) { queue.emplace(i); }
    assert(queue.try_pop(val));
    assert(queue.size() == 9);
    assert(! queue.empty());
    assert(queue.try_emplace(1));
    assert(! queue.try_emplace(1));
    bool discard = queue.try_pop(val);
    assert(queue.size() == 9);
  }

  // Copyable Only Object
  {
    struct Test
    {
      Test()                       = default;
      ~Test()                      = default;
      Test(const Test&)            = default;
      Test& operator=(const Test&) = default;
      Test(Test&&)                 = delete;
      Test& operator=(Test&&)      = delete;
    };
    dro::SPSCQueue<Test> queue {16};
    Test v;
    queue.emplace(v);
    queue.try_emplace(v);
    queue.push(v);
    bool discard = queue.try_push(v);
    assert(queue.size() == 4);
    static_assert(noexcept(queue.emplace(v)));
    static_assert(noexcept(queue.try_emplace(v)));
    static_assert(noexcept(queue.push(v)));
    static_assert(noexcept(queue.try_push(v)));
    // rvalue
    queue.push(Test());
    discard = queue.try_push(Test());
    static_assert(noexcept(queue.push(Test())));
    static_assert(noexcept(queue.try_push(Test())));
    assert(queue.size() == 6);
    // Pop
    Test val;
    discard = queue.try_pop(val);
    assert(queue.size() == 5);
  }

  // Moveable Only Object
  {
    dro::SPSCQueue<std::unique_ptr<int>> queue {16};
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
    try
    {
      dro::SPSCQueue<int> queue(0);
      assert(false);// Should never be called
    }
    catch (std::logic_error& e)
    {
      assert(true);// Should always be called
    }
  }

  std::cout << "Tests Completed!\n";
  return 0;
}
