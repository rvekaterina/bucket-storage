# STL-Compatible Bucket Storage Container

A custom STL-compatible container implementation with bucket-based storage and efficient memory management.

## Features
- Bucket-based memory management
- Iterators
- Exception-safe operations
- Configurable block size

## Key operations

### Core functionality
- **Constructors**: Default, sized, copy/move
- **Destructor**: Clean memory deallocation
- **Iterators**: `begin()/end()`, `cbegin()/cend()`
- **Capacity**: `size()`, `capacity()`, `empty()`

### Modifiers
- **Insert**: `insert(value)` - O(1) amortized
- **Erase**: `erase(iterator)` - O(1)
- **Memory**: `shrink_to_fit()`, `clear()`
- **Utility**: `swap()`

## Usage

```cpp
#include "bucket_storage.hpp"

// Create container with default block capacity (64)
BucketStorage<int> storage;

// Insert elements
storage.insert(42);
storage.insert(100);

// Iterate through elements
for (auto it = storage.begin(); it != storage.end(); ++it) {
    std::cout << *it << std::endl;
}

// Erase elements
storage.erase(storage.begin());

// Check size and capacity
std::cout << "Size: " << storage.size() << std::endl;
std::cout << "Capacity: " << storage.capacity() << std::endl;