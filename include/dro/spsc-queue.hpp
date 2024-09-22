// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// Inspired from Erik Rigtorp
// Significant Modifications / Improvements
// 
// 1) Removed Raw Pointers
// 2) Utilized Vector for RAII memory management
// 3) Modified API to use try_pop instead of pop
//     a) This prevents two function calls for one task
//     b) Removing assert from Debug Mode
// 4) Used copy assignment instead of placement new 

#ifndef DRO_SPSC_QUEUE
#define DRO_SPSC_QUEUE

#include <algorithm>
#include <atomic>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace dro
{

#ifdef __cpp_lib_hardware_interference_size
static constexpr std::size_t cacheLineSize =
    std::hardware_destructive_interference_size;
#else
static constexpr std::size_t cacheLineSize = 64;
#endif

template <typename T, typename Allocator = std::allocator<T>> class SPSC_Queue
{
private:
  std::size_t capacity_;
  std::vector<T, Allocator> buffer_;
  static constexpr std::size_t padding = (cacheLineSize - 1) / sizeof(T) + 1;
  static constexpr std::size_t MAX_SIZE_T = std::numeric_limits<std::size_t>::max();
  
  alignas(cacheLineSize) std::atomic<std::size_t> readIdx_ {0};
  alignas(cacheLineSize) std::size_t readIdxCache_ {0};
  alignas(cacheLineSize) std::atomic<std::size_t> writeIdx_ {0};
  alignas(cacheLineSize) std::size_t writeIdxCache_ {0};

public:
  explicit SPSC_Queue(const std::size_t capacity,
                      const Allocator& allocator = Allocator())
      : capacity_(capacity), buffer_(allocator)
  {
    capacity_ = (capacity_ < 1) ? 1 : capacity_;
    if (capacity_ > MAX_SIZE_T - 2 * padding) {
      capacity_ = MAX_SIZE_T - 2 * padding;
    }
    buffer_.resize(capacity_ + 2 * padding);
  }

  ~SPSC_Queue() = default;

  // non-copyable and non-movable
  SPSC_Queue(const SPSC_Queue& lhs)            = delete;
  SPSC_Queue& operator=(const SPSC_Queue& lhs) = delete;
  SPSC_Queue(SPSC_Queue&& lhs)                 = delete;
  SPSC_Queue& operator=(SPSC_Queue&& lhs)      = delete;

  template <typename... Args>
  void emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible<T, Args&&...>::value)
  {
    static_assert(std::is_constructible<T, Args&&...>::value,
                  "T must be constructible with Args&&...");
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
    buffer_[writeIdx + padding] = std::move(T(std::forward<Args>(args)...));
    writeIdx_.store(nextWriteIdx, std::memory_order_release);
  }

  template <typename... Args>
  bool try_emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible<T, Args&&...>::value)
  {
    static_assert(std::is_constructible<T, Args&&...>::value,
                  "T must be constructible with Args&&...");
    auto const writeIdx = writeIdx_.load(std::memory_order_relaxed);
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
    buffer_[writeIdx + padding] = std::move(T(std::forward<Args>(args)...));
    writeIdx_.store(nextWriteIdx, std::memory_order_release);
    return true;
  }

  void push(const T& val) noexcept(std::is_nothrow_copy_constructible<T>::value)
  {
    static_assert(std::is_copy_constructible<T>::value,
                  "T must be copy constructable");
    emplace(val);
  }

  template <typename P, typename = typename std::enable_if<
                            std::is_constructible<T, P&&>::value>::type>
  void push(P&& val) noexcept(std::is_nothrow_constructible<T, P&&>::value)
  {
    emplace(std::forward<P>(val));
  }

  [[nodiscard]] bool try_push(const T& val) noexcept(
      std::is_nothrow_copy_constructible<T>::value)
  {
    static_assert(std::is_copy_constructible<T>::value,
                  "T must be nothrow copy constructible");
    return try_emplace(val);
  }

  template <typename P, typename = typename std::enable_if<
                            std::is_constructible<T, P&&>::value>::type>
  [[nodiscard]] bool try_push(P&& val) noexcept(
      std::is_nothrow_constructible<T, P&&>::value)
  {
    return try_emplace(std::forward<P>(val));
  }

  [[nodiscard]] T* front() noexcept
  {
    const auto readIdx = readIdx_.load(std::memory_order_relaxed);
    if (readIdx == writeIdxCache_)
    {
      writeIdxCache_ = writeIdx_.load(std::memory_order_acquire);
      if (readIdx == writeIdxCache_)
      {
        return nullptr;
      }
    }
    return &buffer_[readIdx + padding];
  }

  [[nodiscard]] bool try_pop() noexcept
  {
    static_assert(std::is_nothrow_destructible<T>::value,
                  "T must be nothrow destructible");
    const auto readIdx = readIdx_.load(std::memory_order_relaxed);
    if (readIdx == writeIdxCache_)
    {
      writeIdxCache_ = writeIdx_.load(std::memory_order_acquire);
      if (readIdx == writeIdxCache_)
      {
        return false;
      }
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
