#include <iostream>
#include <chrono>
#include <cstdlib>
#include <algorithm>

///////////////////////
// CACHE LINE LENGTH //
///////////////////////

constexpr unsigned int n = 1024 * 4096;

void stride_access(char *array, int length, int stride) {
    char sum = 0;
    
    auto begin = std::chrono::high_resolution_clock::now();
    
    for (unsigned int i = 0; i < length; i += stride) {
        sum += array[i];
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    std::cout << (int) sum << "\t" << stride << "\t" << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << std::endl;
}

/* int main() {
    char *array = new char[n];
    
    for (unsigned int i = 0; i < n; i++) {
        array[i] = (char) rand();
    }
    
    for (unsigned int stride = 1; stride < 1024; stride += 1) {
        stride_access(array, n, stride);
    }
    
    delete[] array;
} */

/////////////////
// CACHE SIZES //
/////////////////

//Result from analyzing cache line length on my machine (replace for your own)
//Divide by 8 for length in longs
constexpr unsigned int cache_line_length = 128 / 8;

//197 is a prime
constexpr unsigned int pointer_stride = 197;

void pointer_chase_access(uint64_t *array, unsigned int size) {
    //Skip if common multiplier
    if (size % pointer_stride == 0) return;
    
    //Add chaseable pointers
    const int first_elem = pointer_stride % size * cache_line_length;
    uint64_t idx = first_elem;
    const uint64_t first_addr = (uint64_t)&array[idx];
    for (unsigned int i = 1; i < size; i++) {
        uint32_t stride = (i + pointer_stride) % size * cache_line_length;
        array[idx] = (uint64_t)array + stride * 8;
        idx = stride;
    }
    array[idx] = first_addr;
    
    uint64_t*next = array + first_elem;
    // warmup
    for (unsigned int i = 0; i < size; i++) {
        next = (uint64_t *) *next;
    }
    //measure
    auto begin = std::chrono::high_resolution_clock::now();
    
    for (unsigned int i = 0; i < 1000000; i++) {
        next = (uint64_t *) *next;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << *next << "\t" << size << "\t" << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << std::endl;
}

int main () {
    //Pointers are 64 bit on x86-64
    uint64_t *array = new uint64_t[n];
    
    for (unsigned int size = 8; size < 100000; size += 8) {
        pointer_chase_access(array, size);
    }
    
    delete[] array;
}
