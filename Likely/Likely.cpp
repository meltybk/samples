// Likely.cpp : This file contains the 'main' function. Program execution begins
// and ends there.
//

#include <cstdint>
#include <cstdio>

int main() {
  for (uint64_t i = 0; i < 100; ++i) {
    if (i % 10 != 0) [[likely]] {
      printf("true");
    } else {
      printf("false");
    }
  }
}
