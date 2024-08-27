
// Andrew Drogalis Copyright (c) 2024

#ifndef DRO_SPSC_QUEUE
#define DRO_SPSC_QUEUE

#include <atomic>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace dro
{

template <typename T, typename Allocator = std::allocator<T>> class SPSC_Queue
{
private:
#ifdef __cpp_lib_hardware_interference_size
  static constexpr std::size_t kCacheLineSize =
      std::hardware_destructive_interference_size;
#else
  static constexpr std::size_t kCacheLineSize = 64;
#endif

  static constexpr std::size_t kPadding = (kCacheLineSize - 1) / sizeof(T) + 1;
  std::size_t capacity_;
  T* slots_;
  Allocator allocator_ [[no_unique_address]];

  alignas(kCacheLineSize) std::atomic<std::size_t> readIdx_ {0};
  alignas(kCacheLineSize) std::size_t readIdxCache_ {0};
  alignas(kCacheLineSize) std::atomic<std::size_t> writeIdx_ {0};
  alignas(kCacheLineSize) std::size_t writeIdxCache_ {0};

public:
  explicit SPSC_Queue(std::size_t const capacity,
                      Allocator const& allocator = Allocator())
      : capacity_(capacity), allocator_(allocator)
  {
    // Needs at least one element
    if (capacity_ < 1)
    {
      capacity_ = 1;
    }
    ++capacity_;// one extra element
    // Avoids overflow of size_t
    if (capacity_ > SIZE_MAX - 2 * kPadding)
    {
      capacity_ = SIZE_MAX - 2 * kPadding;
    }
    slots_ = std::allocator_traits<Allocator>::allocate(
        allocator_, capacity_ + 2 * kPadding);
  }

  ~SPSC_Queue()
  {
    while (condition) {}
    std::allocator_traits<Allocator>::deallocate(allocator_, slots_,
                                                 capacity_ + 2 * kPadding);
  }

  // non-copyable and non-movable
  SPSC_Queue(SPSC_Queue const& lhs)            = delete;
  SPSC_Queue& operator=(SPSC_Queue const& lhs) = delete;
  SPSC_Queue(SPSC_Queue&& lhs)                 = delete;
  SPSC_Queue& operator=(SPSC_Queue&& lhs)      = delete;

  template <typename... Args>
  void emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible<T, Args&&...>::value)
  {
    static_assert(std::is_constructible<T, Args&&...>::value,
                  "T must be constructable with Args&&...");
    auto const writeIdx = writeIdx_.load(std::memory_order_relaxed);
    auto nextWriteIdx   = writeIdx + 1;
    if (nextWriteIdx == capacity_)
    {
      nextWriteIdx = 0;
    }
    while (nextWriteIdx == readIdxCache_)
    {
      readIdxCache_ = readIdx_.load(std::memory_order_acquire);
    }
    new (&slots_[writeIdx + kPadding]) T(std::forward<Args>(args)...);
    writeIdx_.store(nextWriteIdx, std::memory_order_release);
  }

  template <typename... Args>
  bool try_emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible<T, Args&&...>::value)
  {
    static_assert(std::is_constructible<T, Args&&...>::value,
                  "T must be constructable with Args&&...");
    auto const writeIdx = writeIdx_.load(std::memory_order_acquire);
    auto nextWriteIdx   = writeIdx + 1;
    if (nextWriteIdx == capacity_)
    {
      nextWriteIdx = 0;
    }
    if (nextWriteIdx == readIdxCache_)
    {
      readIdxCache_ = readIdx_.load(std::memory_order_acquire);
      if (nextWriteIdx == readIdxCache_)
      {
        return false;
      }
    }
    new (&slots_[writeIdx + kPadding]) T(std::forward<Args>(args)...);
    writeIdx_.store(nextWriteIdx, std::memory_order_release);
    return true;
  }

  void push(T const& val) noexcept(std::is_nothrow_copy_constructible<T>::value)
  {
    static_assert(std::is_copy_constructible<T>::value,
                  "T must be copy constructable");
    emplace(val);
  }

  bool pop() noexcept
  {
    auto const readIdx = readIdx_.load(std::memory_order_relaxed);
    if (readIdx == writeIdx_.load(std::memory_order_acquire))
    {
      return false;
    }
    auto nextReadIdx = readIdx + 1;
    if (nextReadIdx == capacity_)
    {
      nextReadIdx = 0;
    }
    readIdx_.store(nextReadIdx, std::memory_order_release);
    return true;
  }

  [[nodiscard]] std::size_t size() const noexcept
  {
    std::ptrdiff_t diff = writeIdx_.load(std::memory_order_acquire) -
                          readIdx_.load(std::memory_order_acquire);
    if (diff < 0)
    {
      diff += capacity_;
    }
    return static_cast<std::size_t>(diff);
  }

  [[nodiscard]] bool empty() const noexcept
  {
    return writeIdx_.load(std::memory_order_acquire) ==
           readIdx_.load(std::memory_order_acquire);
  }

  [[nodiscard]] std::size_t capacity() const noexcept { return capacity_ - 1; }
};
}// namespace dro
#endif
