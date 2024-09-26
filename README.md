# SPSC Queue

## Benchmarks

These benchmarks are the average of (11) iterations.

<img src="https://raw.githubusercontent.com/drogalis/SPSC-Queue/refs/heads/main/assets/Operations%20per%20Millisecond.png" alt="Operations Per Millisecond Stats" style="padding-top: 10px;">

<img src="https://raw.githubusercontent.com/drogalis/SPSC-Queue/refs/heads/main/assets/Round%20Trip%20Time%20(ns).png" alt="Round Trip Time Stats" style="padding-top: 10px;">

Full write up TBD.

## Installing

To build and install the shared library, run the commands below.

```
    $ mkdir build && cd build
    $ cmake .. -DCMAKE_BUILD_TYPE=Release
    $ sudo make install
```

## Sources

Inspiration came from these sources. I recommend checking them out:

- [Rigtorp SPSCQueue](https://github.com/rigtorp/SPSCQueue)
- [MoodyCamel ReaderWriterQueue](https://github.com/cameron314/readerwriterqueue)
- [Folly ProducerConsumerQueue](https://github.com/facebook/folly/blob/main/folly/ProducerConsumerQueue.h)
- [Boost SPSC Queue](https://www.boost.org/doc/libs/1_60_0/boost/lockfree/spsc_queue.hpp)
