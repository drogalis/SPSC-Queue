# SPSC Queue

<!-- A STL compliant map and set that uses a red black tree under the hood. Much faster than [boost::flat_map](https://www.boost.org/doc/libs/1_76_0/boost/container/flat_map.hpp) for any insert or delete heavy workload over ~250 elements. -->
<!-- Beats [std::map](https://en.cppreference.com/w/cpp/container/map) for all workloads when full optimizations are enabled. -->

## Table of Contents

- [Usage](#Usage)
- [Benchmarks](#Benchmarks)
- [Installing](#Installing)
- [Sources](#Sources)

## Implementation

<!-- This flat map uses a vector to store the tree nodes, and maintains an approximate [heap](<https://en.wikipedia.org/wiki/Heap_(data_structure)#:~:text=In%20computer%20science%2C%20a%20heap,The%20node%20at%20the%20%22top%22>) -->
<!-- structure for a cache optimized [binary search](https://en.wikipedia.org/wiki/Binary_search#:~:text=Binary%20search%20compares%20the%20target,the%20target%20value%20is%20found.). -->
<!---->
<!-- In order to validate the correctness of the balancing algorithm, a full tree traversal is performed comparing the dro::FlatMap to the [STL Tree](https://github.com/gcc-mirror/gcc/blob/master/libstdc++-v3/include/bits/stl_tree.h) -->
<!-- implementation. Many red black trees have subtle errors due to lack of validation. -->

_Detailed Discussion_

## Usage

Main points:

- The key and value must be default constructible.
- The key and value must be copy or move assignable.

#### Constructor

The full list of template arguments are as follows:

- T: Must be default constructible and a copyable or moveable type
- Allocator: Allocator passed to the vector, takes the type T as the template parameter.

- `explicit SPSCQueue(const std::size_t capacity, const Allocator& allocator = Allocator());`

   Capacity must be a positive number

#### Methods

- `void emplace(Args&&... args) noexcept(SPSC_NoThrow_Type<T, Args...>);`

- `void force_emplace(Args&&... args) noexcept(SPSC_NoThrow_Type<T, Args...>);`

- `bool try_emplace(Args&&... args) noexcept(SPSC_NoThrow_Type<T, Args...>);`

- `void push(const T& val) noexcept(SPSC_NoThrow_Type<T>);`

- `void force_push(const T& val) noexcept(SPSC_NoThrow_Type<T>);`

- `[[nodiscard]] bool try_push(const T& val) noexcept(SPSC_NoThrow_Type<T>);`

- `[[nodiscard]] bool try_pop(T& val) noexcept;`

- `[[nodiscard]] std::size_t size() const noexcept;`

- `[[nodiscard]] bool empty() const noexcept;`

- `[[nodiscard]] std::size_t capacity() const noexcept;`

## Benchmarks

These benchmarks were taken on a (4) core Intel(R) Core(TM) i5-9300H CPU @ 2.40GHz with isolcpus on cores 2 and 3.
The linux kernel is v6.10.11-200.fc40.x86_64 and compiled with gcc version 14.2.1.

Most important aspects of benchmarking:

- Have at least two cores isolated with isolcpus enabled in Grub.
- Compile with -DCMAKE_BUILD_TYPE=Release
- Pass isolated cores ID number as an executable argument i.e. ./SPSC-Queue-Benchmark 2 3

These benchmarks are the average of (11) iterations.

<img src="https://raw.githubusercontent.com/drogalis/SPSC-Queue/refs/heads/main/assets/Operations%20per%20Millisecond.png" alt="Operations Per Millisecond Stats" style="padding-top: 10px;">

<img src="https://raw.githubusercontent.com/drogalis/SPSC-Queue/refs/heads/main/assets/Round%20Trip%20Time%20(ns).png" alt="Round Trip Time Stats" style="padding-top: 10px;">

## Installing

**Required C++20 or higher.**

To build and install the shared library, run the commands below.

```
    $ mkdir build && cd build
    $ cmake .. -DCMAKE_BUILD_TYPE=Release
    $ sudo make install
```

## Sources

Inspiration for the SPSC Queue came from the following sources:

- [Rigtorp SPSCQueue](https://github.com/rigtorp/SPSCQueue)
- [MoodyCamel ReaderWriterQueue](https://github.com/cameron314/readerwriterqueue)
- [Folly ProducerConsumerQueue](https://github.com/facebook/folly/blob/main/folly/ProducerConsumerQueue.h)
- [Boost SPSC Queue](https://www.boost.org/doc/libs/1_60_0/boost/lockfree/spsc_queue.hpp)

## License

This software is distributed under the MIT license. Please read [LICENSE](https://github.com/drogalis/Flat-Map-RB-Tree/blob/main/LICENSE) for information on the software availability and distribution.
