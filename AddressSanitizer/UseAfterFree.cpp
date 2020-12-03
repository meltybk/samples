// UseAfterFree.cpp : This file contains the 'main' function. Program execution
// begins and ends there.
//

#include "malloc.h"
#include <cassert>
#include <cstdint>
#include <sanitizer/asan_interface.h>

static mspace msp = nullptr;
static constexpr size_t kNumFreeList = 2;
static void* free_list[kNumFreeList];
static uint32_t free_list_index = 0;

extern void *malloc(size_t size) { return mspace_malloc(msp, size); }
extern void free(void *ptr) {
  if (free_list_index >= kNumFreeList) {
    free_list_index = 0;
    for (uint32_t i = 0; i < kNumFreeList; ++i) {
      mspace_free(msp, free_list[i]);
    }
  }
  free_list[free_list_index] = ptr;
  ++free_list_index;
  ASAN_POISON_MEMORY_REGION(ptr, mspace_usable_size(ptr));
}

int main() {
  msp = create_mspace(1024 * 1024 * 5, false);

  int *data1 = (int *)malloc(sizeof(int) * 100);
  free(data1);

  int *data2 = (int *)malloc(sizeof(int) * 100);
  data2[0] = 2;
  data1[0] = 1;

  free(data2);
}
