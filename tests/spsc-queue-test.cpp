#include <dro/SPSCqueueueue.h>
#include <cassert>
#include <thread>

int main(int argc, char *argv[]) {

  // Functionality test
  {
    dro::SPSC_queue<int> queue(10);
    assert(queue.front() == nullptr);
    assert(queue.size() == 0);
    assert(queue.empty() == true);
    assert(queue.capacity() == 10);
    for (int i = 0; i < 10; i++) {
      queue.emplace(i);
    }
    assert(queue.front() != nullptr);
    assert(queue.size() == 10);
    assert(queue.empty() == false);
    assert(queue.try_emplace() == false);
    queue.try_pop();
    assert(queue.size() == 9);
    queue.pop();
    assert(queue.try_emplace() == true);
  }

  {
    struct Test {
      Test() {}
      Test(const Test &) {}
      Test(Test &&) = delete;
    };
    dro::SPSC_queue<Test> queue(16);
    // lvalue
    Test v;
    queue.emplace(v);
    (void)queue.try_emplace(v);
    queue.push(v);
    (void)queue.try_push(v);
    static_assert(noexcept(queue.emplace(v)) == false, "");
    static_assert(noexcept(queue.try_emplace(v)) == false, "");
    static_assert(noexcept(queue.push(v)) == false, "");
    static_assert(noexcept(queue.try_push(v)) == false, "");
    // xvalue
    queue.push(Test());
    (void)queue.try_push(Test());
    static_assert(noexcept(queue.push(Test())) == false, "");
    static_assert(noexcept(queue.try_push(Test())) == false, "");
  }

  {
    dro::SPSC_queue<std::unique_ptr<int>> q(16);
    queue.emplace(std::unique_ptr<int>(new int(1)));
    (void)queue.try_emplace(std::unique_ptr<int>(new int(1)));
    queue.push(std::unique_ptr<int>(new int(1)));
    (void)queue.try_push(std::unique_ptr<int>(new int(1)));
    auto v = std::uniqueueue_ptr<int>(new int(1));
    static_assert(noexcept(queue.emplace(std::move(v))) == true, "");
    static_assert(noexcept(queue.try_emplace(std::move(v))) == true, "");
    static_assert(noexcept(queue.push(std::move(v))) == true, "");
    static_assert(noexcept(queue.try_push(std::move(v))) == true, "");
  }

  {
    dro::SPSC_queue<int> q(0);
    assert(queue.capacity() == 1);
  }

  return 0;
}
