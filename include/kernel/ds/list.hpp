#pragma once

#include <concepts>

#include "int/interrupt.hpp"

struct ListItem {
  ListItem *prev, *next;
  ListItem() : prev(nullptr), next(nullptr) {}
  ListItem(const ListItem&) = delete;
};

inline void link(ListItem* prev, ListItem* next) {
  if (prev)
    prev->next = next;
  if (next)
    next->prev = prev;
}

inline void unlink(ListItem* it) {
  link(it->prev, it->next);
  it->prev = it->next = nullptr;
}

template <std::derived_from<ListItem> T>
class ListHead {
 public:
  class iterator {
   public:
    iterator(T* it) : it_(it) {}
    iterator(ListItem* it) : it_((T*)it) {}
    iterator& operator++() {
      it_ = (T*)it_->next;
      return *this;
    }
    iterator operator++(int) {
      iterator copy = *this;
      ++*this;
      return copy;
    }
    iterator& operator--() {
      it_ = (T*)it_->prev;
      return *this;
    }
    iterator operator--(int) {
      iterator copy = *this;
      ++*this;
      return copy;
    }
    T& operator*() const {
      return *it_;
    }
    T* operator->() const {
      return it_;
    }
    bool operator==(const iterator& other) const {
      return other.it_ == it_;
    }
    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }
    operator T*() {
      return it_;
    }

   private:
    T* it_;
  };

 private:
  int size_ = 0;
  ListItem head_{}, tail_{};

 public:
  ListHead() {
    init();
  }

  ListHead(const ListHead& o) : ListHead{} {
    for (auto& it : o) {
      push_back(new T(it));
    }
  }

  ~ListHead() {
    clear();
  }

  void init() {
    size_ = 0;
    head_.prev = tail_.next = nullptr;
    link(&head_, &tail_);
  }

  void insert(iterator it, T* node) {
    save_DAIF_disable_interrupt();
    size_++;
    link(node, it->next);
    link(it, node);
    restore_DAIF();
  }
  void insert_before(iterator it, T* node) {
    insert(--it, node);
  }
  void push_front(T* node) {
    insert(&head_, node);
  }
  void push_back(T* node) {
    insert(tail_.prev, node);
  }
  void erase(iterator it) {
    save_DAIF_disable_interrupt();
    size_--;
    unlink(it);
    restore_DAIF();
  }

  void clear() {
    save_DAIF_disable_interrupt();
    while (size() > 0)
      erase(begin());
    restore_DAIF();
  }

  T* pop_front() {
    if (empty())
      return nullptr;
    auto it = begin();
    erase(it);
    return it;
  }

  T* front() {
    if (empty())
      return nullptr;
    return begin();
  }

  int size() const {
    return size_;
  }
  bool empty() const {
    return size_ == 0;
  }

  iterator head() const {
    return (T*)&head_;
  }
  iterator begin() const {
    return (T*)head_.next;
  }
  iterator end() const {
    return (T*)&tail_;
  }
};
