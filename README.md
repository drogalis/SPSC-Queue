# SPSC Queue

A SPSC lock free queue that pre-allocates all memory and transfers messages to and from the queue using copy/move assignment. Achieves a top performance for all message sizes, even higher than [MoodyCamel ReaderWriterQueue](https://github.com/cameron314/readerwriterqueue)
and [Rigtorp SPSCQueue](https://github.com/rigtorp/SPSCQueue).

## Table of Contents

- [Benchmarks](#Benchmarks)
- [Implementation](#Implementation)
- [Installing](#Installing)
- [Sources](#Sources)

## Usage

The queue uses C++20 concepts to overload the read and write operations, by default all reads and writes are moved into and out of
the queue. If the type is non-movable then copy assignment will be used instead.

The full list of template arguments are as follows:

- Type: Must be default constructible and must be copy or move assignable.

- Allocator: Allocator to be passed to the vector, takes the type T as the template parameter.

#### Constructor

- `explicit SPSCQueue(const std::size_t capacity, const Allocator& allocator = Allocator());`

  Capacity must be a positive number, and all memory is allocated in the constructor

#### Methods

- `void emplace(Args&&... args) noexcept(SPSC_NoThrow_Type<T, Args...>);`

  Constructs type in place, and waits on the reader if the queue is full.

- `void force_emplace(Args&&... args) noexcept(SPSC_NoThrow_Type<T, Args...>);`

  Constructs type in place, and writes over the reader if the queue is full.

  **Note: If writer surpasses reader, the entire queue will be erased. Use with caution.**

- `bool try_emplace(Args&&... args) noexcept(SPSC_NoThrow_Type<T, Args...>);`

  Returns bool, and constructs type in place. Fails to write if the queue is full.

- `void push(const T& val) noexcept(SPSC_NoThrow_Type<T>);`

  Waits on the reader if the queue is full.

- `void force_push(const T& val) noexcept(SPSC_NoThrow_Type<T>);`

  Writes over the reader if the queue is full.

  **Note: If writer surpasses reader, the entire queue will be erased. Use with caution.**

- `[[nodiscard]] bool try_push(const T& val) noexcept(SPSC_NoThrow_Type<T>);`

  Returns bool, and fails to write if the queue is full.

- `[[nodiscard]] bool try_pop(T& val) noexcept;`

  Returns bool, and fails to read if the queue is empty.

- `[[nodiscard]] std::size_t size() const noexcept;`

  Returns the number of elements in the SPSC queue.

- `[[nodiscard]] bool empty() const noexcept;`

  Checks whether the container is empty.

- `[[nodiscard]] std::size_t capacity() const noexcept;`

  Returns the number of elements that have been allocated.

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

## Implementation

_Detailed Discussion_

## Installing

**Required C++20 or higher.**

To build and install the shared library, run the commands below.

```
    $ mkdir build && cd build
    $ cmake ..
    $ make
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
