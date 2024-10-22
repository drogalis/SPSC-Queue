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

#include <dro/spsc-queue.hpp> // for dro::SPSCQueue
#include <thread>             // for std::thread

int main(int argc, char *argv[]) {
  int const iter{10};
  int const size{10};
  dro::SPSCQueue<int> queue(size);
  auto thrd = std::thread([&] {
    for (int i{}; i < iter; ++i) {
      int val{};
      queue.pop(val);
      // Can also try_pop
      // while (!queue.try_pop(val)) {}
    }
  });

  for (int i{}; i < iter; ++i) {
    queue.emplace(i);
  }
  // Can also use try_emplace
  // for (int i {}; i < iter; ++i) { queue.try_emplace(i); }

  // Can also use force_emplace
  // for (int i {}; i < iter; ++i) { queue.force_emplace(i); }

  thrd.join();

  return 0;
}
