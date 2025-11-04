#ifndef LIBMPOPT_COMMON_ALLOCATOR_HPP
#define LIBMPOPT_COMMON_ALLOCATOR_HPP

namespace mpopt {

class memory_block {
public:  
  static constexpr size_t size_kib = size_t(1) * 1024;
  static constexpr size_t size_mib = size_t(1) * 1024 * 1024;
  static constexpr size_t size_512mib = size_mib * 512;
  static constexpr size_t size_gib = size_mib * 1024;
  static constexpr size_t size_256gib = size_gib * 256;

  static constexpr size_t COMMIT_GRANULARITY = 4 * size_mib; // 4 MiB

  memory_block()
  : memory_(0)
  , size_(size_256gib)
  , current_(0)
  , committed_(0)
  {
    // RESERVE VIRTUAL ADDRESS SPACE
    while (!reserve_memory(size_) && size_ >= size_512mib) {
      size_ -= size_512mib;
    }

#ifndef NDEBUG
    std::cout << "[mem] ctor: size=" << size_ << "B (" << (1.0f * size_ / (size_kib * 1024)) << "MiB) -> memory_=" << reinterpret_cast<void*>(memory_) << std::endl;
#endif

    if (memory_ == 0) {
        std::cerr << "Could not allocate memory while building optimization graph." << std::endl;
        throw std::bad_alloc();
    }

    current_ = memory_;
    committed_ = memory_;
  }

  ~memory_block()
  {
    if (memory_ != 0) {
      #ifdef _WIN32
        VirtualFree(reinterpret_cast<void*>(memory_), 0, MEM_RELEASE);
      #else  
        munmap(reinterpret_cast<void*>(memory_), size_);
      #endif
    }

  }

  void align(size_t a)
  {
    auto misalignment = current_ % a;
    if (misalignment != 0)
      allocate(a - misalignment);
  }

  uintptr_t allocate(size_t s)
  {
    if (current_ + s >= memory_ + size_){
      std::cerr << "Ran out of memory while building optimization graph." << std::endl;
      throw std::bad_alloc();
    }

    commit_memory(s);

    auto result = current_;
    current_ += s;
    return result;
  }

protected:
  uintptr_t memory_;
  size_t size_;
  uintptr_t current_;
  uintptr_t committed_;

private:

  bool reserve_memory(size_t s) {
      // Must differentiated between Windows and MacOS/Linux
      #ifdef _WIN32
        auto result = VirtualAlloc(nullptr, s, MEM_RESERVE, PAGE_READWRITE);
        if (result != NULL)
          memory_ = reinterpret_cast<uintptr_t>(result);
      #else
        auto result = mmap(nullptr, s, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (result != MAP_FAILED)
          memory_ = reinterpret_cast<uintptr_t>(result);
      #endif
    
    return memory_ != 0; // Returns true if memory was successfully reserved.
  }

  void commit_memory(size_t s)
  {
    auto target = current_ + s;
    if (target <= committed_) {
      return;
    }

    // Ensure that we commit memory in multiples of COMMIT_GRANULARITY
    uintptr_t new_commit = ((target + COMMIT_GRANULARITY - 1) / COMMIT_GRANULARITY) * COMMIT_GRANULARITY;

    #ifdef _WIN32
      if (VirtualAlloc(reinterpret_cast<void*>(committed_), new_commit - committed_, MEM_COMMIT, PAGE_READWRITE) == NULL) {
        std::cerr << "Could not commit memory while building optimization graph." << std::endl;
        throw std::bad_alloc();
      }
    #else
      if (mprotect(reinterpret_cast<void*>(committed_), new_commit - committed_, PROT_READ | PROT_WRITE) != 0) {
        std::cerr << "Could not commit memory while building optimization graph." << std::endl;
        throw std::bad_alloc();
      }
    #endif

    committed_ = new_commit;
  }
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
