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
#include <thread>

int main(int argc, char* argv[])
{
  int const iter {10};
  dro::SPSCQueue<int> queue(1);
  auto thrd = std::thread([&] {
    for (int i {}; i < iter; ++i)
    {
      int val {};
      while (! queue.try_pop(val)) {}
    }
  });

  for (int i {}; i < iter; ++i) { queue.push(i); }
  thrd.join();

  return 0;
}
