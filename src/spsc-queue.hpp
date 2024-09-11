// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// Inspired from Erik Rigtorp
// Significant Modifications / Improvements
// E.G
// 1) Removed Raw Pointers
// 2) Utilized Vector for RAII memory management
// 3) Modified API to use try_pop instead of pop
//     a) This prevents two function calls for one task
//     b) 2x performance boost for Debug by removing assert

#ifndef DRO_SPSC_QUEUE
#define DRO_SPSC_QUEUE

#include <atomic>
#include <cassert>
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

#ifdef __cpp_lib_hardware_interference_size
static constexpr std::size_t cacheLineSize =
    std::hardware_destructive_interference_size;
#else
static constexpr std::size_t cacheLineSize = 64;
#endif

template <typename K> struct alignas(cacheLineSize) Slot
{
  K data_;

  template <typename... Args>
  explicit Slot(Args&&... args) : data_(K(std::forward<Args>(args)...))
  {
  }

  void destroy() { data_.~K(); }

  ~Slot() { destroy(); }
  // These are required for vector static assert, but are never invoked
  Slot(const Slot& lhs) noexcept(std::is_nothrow_copy_constructible<K>::value)
      : data_(lhs.data_)
  {
  }
  Slot& operator=(const Slot& lhs) noexcept(
      std::is_nothrow_copy_assignable<K>::value)
  {
    if (*this != lhs)
    {
      data_ = lhs.data_;
    }
    return *this;
  };
  Slot(Slot&& lhs) noexcept(std::is_nothrow_move_constructible<K>::value)
      : data_(std::move(lhs.data_))
  {
  }
  Slot& operator=(Slot&& lhs) noexcept(
      std::is_nothrow_move_assignable<K>::value)
  {
    if (*this != lhs)
    {
      data_ = std::move(lhs.data_);
    }
    return *this;
  };
  bool operator!=(Slot& lhs) { return data_ != lhs.data_; }
};

template <typename T, typename Allocator = std::allocator<Slot<T>>>
class SPSC_Queue
{
private:
  std::size_t capacity_;
  std::vector<Slot<T>, Allocator> buffer_;

  alignas(cacheLineSize) std::atomic<std::size_t> readIdx_ {0};
  alignas(cacheLineSize) std::size_t readIdxCache_ {0};
  alignas(cacheLineSize) std::atomic<std::size_t> writeIdx_ {0};
  alignas(cacheLineSize) std::size_t writeIdxCache_ {0};

public:
  explicit SPSC_Queue(const std::size_t capacity,
                      const Allocator& allocator = Allocator())
      : capacity_(capacity), buffer_(allocator)
  {
    // Needs at least one element
    if (capacity_ < 1)
    {
      capacity_ = 1;
    }
    ++capacity_;// one extra element
    // Avoids overflow of size_t
    if (capacity_ > SIZE_MAX)
    {
      capacity_ = SIZE_MAX;
    }
    buffer_.resize(capacity_);
  }

  ~SPSC_Queue()
  {
    while (try_pop()) {}
  }

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
    buffer_[writeIdx].data_ = std::move(T(std::forward<Args>(args)...));
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
    buffer_[writeIdx].data_ = std::move(T(std::forward<Args>(args)...));
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
    return &buffer_[readIdx].data_;
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
    buffer_[readIdx].destroy();
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
