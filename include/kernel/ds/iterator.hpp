#pragma once

#include <concepts>

template <typename T, auto f>
concept transform = std::is_invocable_r_v<T, decltype(f), T>;

template <typename T, auto next, auto get = nullptr, auto prev = nullptr>
  requires transform<T*, next>
class Iterator {
 public:
  Iterator(T* p = nullptr) : p_(p) {}
  Iterator& operator++() {
    p_ = next(p_);
    return *this;
  }
  Iterator operator++(int) {
    Iterator copy = *this;
    ++*this;
    return copy;
  }
  Iterator& operator--()
    requires transform<T*, prev>
  {
    p_ = prev(p_);
    return *this;
  }
  Iterator operator--(int)
    requires transform<T*, prev>
  {
    Iterator copy = *this;
    --*this;
    return copy;
  }
  T* operator*() const
    requires(!std::invocable<decltype(get), T*>)
  {
    return p_;
  }
  T* operator->() const
    requires(!std::invocable<decltype(get), T*>)
  {
    return p_;
  }
  // XXX: ???
  template <typename R = std::invoke_result_t<
                std::conditional_t<std::is_invocable_v<decltype(get), T*>,
                                   decltype(get), void(T*)>,
                T*>>
  R operator*() const
    requires(std::invocable<decltype(get), T*>)
  {
    return get(p_);
  }
  template <typename R = std::invoke_result_t<
                std::conditional_t<std::is_invocable_v<decltype(get), T*>,
                                   decltype(get), void(T*)>,
                T*>>
  R operator->() const
    requires(std::invocable<decltype(get), T*>)
  {
    return get(p_);
  }
  bool operator==(const Iterator& other) const {
    return other.p_ == p_;
  }
  bool operator!=(const Iterator& other) const {
    return !(*this == other);
  }
  operator T*() {
    return p_;
  }

 private:
  T* p_;
};
