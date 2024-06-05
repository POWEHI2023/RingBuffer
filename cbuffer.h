#ifndef __CBUFFER__
#define __CBUFFER__

#include <iostream>
#include <type_traits>
#include <exception>
#include <string>
#include <mutex>

#include <string.h>

/**
 *  cpu time
*/
uint64_t rdtsc() {
    unsigned int lo, hi;
    __asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

template <typename T>
class c_buffer {
    using _ElemType = typename std::decay<T>::type;
public:
    explicit c_buffer(size_t _size) noexcept
        : _capacity(_size + 1), _size(0), _mem((_ElemType*)(malloc(sizeof(_ElemType) * (_size + 1)))), _head(0), _tail(0) {}

#if __cplusplus >= 201103L
    // deep copy
    c_buffer(const c_buffer& buffer);
    c_buffer(c_buffer&& buffer) noexcept;
    c_buffer& operator=(const c_buffer&) = delete;
    c_buffer& operator=(c_buffer&&) = delete;
#endif

    virtual ~c_buffer() noexcept;

public:
    _ElemType front() const;

    size_t size() const noexcept;
    size_t capacity() const noexcept;
    bool empty() const noexcept;
    bool full() const noexcept;

    template <typename Y>
    bool insert(Y&& _elem) noexcept;
    void pop() noexcept;

    /**
     * atomic insert an element in the cycle buffer.
     * @return bool this operation is successed or failed
    */
    bool atomic_insert(const _ElemType& _elem) noexcept;
    /**
     * atomic pop the front element of the cycle buffer.
     * @return _ElemType without reference and is const, 
     *         eg. _ElemType e = operation(atomic_pop(), ...);
     *         that's not clear what e exactly is.
     * @exception PopEnptyBufferError()
    */
    const _ElemType atomic_pop();

private:
    template <typename Y>
    inline bool __insert(Y&& _elem) noexcept;
    inline void __pop() noexcept;
    template <typename Y>
    inline bool __atomic_insert(Y&& _elem) noexcept;
    inline const _ElemType __atomic_pop();

    // weak interface
    template <typename Y, bool _is_atomic> 
    // requires { std::same_as<Y, _ElemType>; }
    inline bool __proxy_insert(Y&& _elem) noexcept {
        bool _ret = false;
        if constexpr (_is_atomic)
            _ret = __atomic_insert(std::forward<decltype(_elem)>(_elem));
        else 
            _ret = __insert(std::forward<decltype(_elem)>(_elem));
        
        if (_ret) _size++;
        return _ret;
    }

private:
    size_t _size, _capacity;
    size_t _head, _tail;
    _ElemType *_mem;

    std::mutex mtx;
};

class PopEmptyBufferError : public std::exception {
public:
    PopEmptyBufferError() noexcept 
        : _message("PopEmptyBufferError") {}
    PopEmptyBufferError(const std::string& _msg) noexcept 
        : _message(_msg) {}
    ~PopEmptyBufferError() noexcept {}

#if __cplusplus >= 201103L
    PopEmptyBufferError(const PopEmptyBufferError&) = default;
    PopEmptyBufferError(PopEmptyBufferError&&) = default;
    PopEmptyBufferError& operator=(const PopEmptyBufferError&) = default;
    PopEmptyBufferError& operator=(PopEmptyBufferError&&) = default;
#endif

    const char *what() const noexcept override {
        return _message.c_str();
    }
private:
    std::string _message;
};

#endif