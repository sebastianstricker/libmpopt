#ifndef LIBMPOPT_COMMON_ALLOCATOR_HPP
#define LIBMPOPT_COMMON_ALLOCATOR_HPP

namespace mpopt {

class memory_block {
public:
  static constexpr size_t size_kib = size_t(1) * 1024;

  memory_block(size_t memory_kb)
  : memory_(0)
  , size_(memory_kb * size_kib)
  , current_(0)
  {
    memory_ = reinterpret_cast<uintptr_t>(std::malloc(size_));
    if (memory_ == 0){
        std::cerr << "Memory allocation failed. Attempted to allocate" << size_ << "KiB of memory." << std::endl;
        throw std::bad_alloc();
    }

#ifndef NDEBUG
      std::cout << "[mem] ctor: size=" << size_ << "B (" << (1.0f * size_ / (size_kib * 1024)) << "MiB) -> memory_=" << reinterpret_cast<void*>(memory_) << std::endl;
#endif
    current_ = memory_;
  }

  ~memory_block()
  {
    if (memory_ != 0) {
      std::free(reinterpret_cast<void*>(memory_));
    }
  }

  void align(size_t a)
  {
    if (current_ % a != 0)
      allocate(a - current_ % a);
  }

  uintptr_t allocate(size_t s)
  {
    if (current_ + s >= memory_ + size_){
        std::cerr << "Ran out of memory while building optimization graph." << std::endl;
        throw std::bad_alloc();
    }

    auto result = current_;
    current_ += s;
    return result;
  }

protected:
  uintptr_t memory_;
  size_t size_;
  uintptr_t current_;
};

template<typename T>
class block_allocator {
public:
  using value_type = T;
  template<typename U> struct rebind { using other = block_allocator<U>; };

  block_allocator(memory_block& block)
  : block_(&block)
  { }

  template<typename U>
  block_allocator(const block_allocator<U>& other)
  : block_(other.block_)
  { }

  T* allocate(size_t n = 1)
  {
    block_->align(alignof(T));
    auto mem = block_->allocate(sizeof(T) * n);
    assert(reinterpret_cast<std::uintptr_t>(mem) % alignof(T) == 0);
    return reinterpret_cast<T*>(mem);
  }

  void deallocate([[maybe_unused]] T* ptr, [[maybe_unused]] size_t n = 1) { }

protected:
  memory_block* block_;

  template<typename U> friend class block_allocator;
};

}

#endif

/* vim: set ts=8 sts=2 sw=2 et ft=cpp: */
