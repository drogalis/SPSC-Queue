#include <cassert>
#include <dro/spsc-queue.hpp>
#include <memory>

int main(int argc, char* argv[])
{

  // Functional Test
  {
    dro::SPSC_Queue<int> queue {10};
    assert(queue.front() == nullptr);
    assert(queue.size() == 0);
    assert(queue.empty() == true);
    assert(queue.capacity() == 10);
    for (int i = 0; i < 10; i++) { queue.emplace(i); }
    assert(queue.front() != nullptr);
    assert(queue.size() == 10);
    assert(queue.empty() == false);
    assert(queue.try_emplace(1) == false);
    queue.try_pop();
    assert(queue.size() == 9);
    queue.try_pop();
    assert(queue.try_emplace(1) == true);
  }

  // Copyable Only
  {
    struct Test
    {
      Test()  = default;
      ~Test() = default;
      Test(const Test&) = default;
      Test& operator=(const Test&) = default;
      Test(Test&&) = delete;
      Test& operator=(Test&&) = delete;
    };
    dro::SPSC_Queue<Test> queue {16};
    // lvalue
    Test v;
    queue.emplace(v);
    (void)queue.try_emplace(v);
    queue.push(v);
    (void)queue.try_push(v);
    static_assert(noexcept(queue.emplace(v)));
    static_assert(noexcept(queue.try_emplace(v)));
    static_assert(noexcept(queue.push(v)));
    static_assert(noexcept(queue.try_push(v)));
    // xvalue
    queue.push(Test());
    (void)queue.try_push(Test());
    static_assert(noexcept(queue.push(Test())));
    static_assert(noexcept(queue.try_push(Test())));
  }

  // Moveable Only
  {
    dro::SPSC_Queue<std::unique_ptr<int>> queue {16};
    queue.emplace(std::make_unique<int>(1));
    queue.try_emplace(std::make_unique<int>(1));
    queue.push(std::make_unique<int>(1));
    queue.try_push(std::make_unique<int>(1));
    auto v = std::make_unique<int>(1);
    static_assert(noexcept(queue.emplace(std::move(v))));
    static_assert(noexcept(queue.try_emplace(std::move(v))));
    static_assert(noexcept(queue.push(std::move(v))));
    static_assert(noexcept(queue.try_push(std::move(v))));
  }

  {
    dro::SPSC_Queue<int> queue(0);
    assert(queue.capacity() == 1);
  }

  return 0;
}
