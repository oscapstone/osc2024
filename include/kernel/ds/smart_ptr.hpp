#pragma once

template <class T>
class smart_ptr {
  struct P {
    int cnt;
    T* ptr;
  };
  P* p;

  static P* copy(const smart_ptr& ptr) {
    if (not ptr)
      return nullptr;
    P& p = ptr;
    p.cnt++;
    return &p;
  }
  operator P&() const {
    return *p;
  }

 public:
  smart_ptr() : p{nullptr} {}
  smart_ptr(T* t) : p{t ? new P{1, t} : nullptr} {}
  smart_ptr(const smart_ptr& o) : p{copy(o)} {}
  smart_ptr& operator=(const smart_ptr& o) {
    release();
    p = copy(o);
    return *this;
  }
  ~smart_ptr() {
    release();
  }
  void release() {
    if (p and --p->cnt == 0) {
      // FIXME
      // delete p->ptr;
      // p->ptr = nullptr;
      // delete p;
    }
    p = nullptr;
  }
  T* get() const {
    return p ? p->ptr : nullptr;
  }
  T& operator*() const {
    return *get();
  }
  T* operator->() const {
    return get();
  }
  operator bool() const {
    return p and p->ptr;
  }
  template <typename U>
  bool operator==(smart_ptr<U> o) const {
    return get() == o.get();
  }
  template <typename U>
  bool operator==(U o) const {
    return get() == o;
  }
};
