#pragma once

#include <concepts>

#include "int/interrupt.hpp"

template <typename T>
struct ListItem {
  ListItem *prev, *next;
  ListItem() : prev(nullptr), next(nullptr) {}
  ListItem(const ListItem&) = delete;
  T* get() const {
    return (T*)this;
  }
};

template <typename T>
inline void link(ListItem<T>* prev, ListItem<T>* next) {
  if (prev)
    prev->next = next;
  if (next)
    next->prev = prev;
}

template <typename T>
inline void unlink(ListItem<T>* it) {
  link(it->prev, it->next);
  it->prev = it->next = nullptr;
}

template <typename P, typename T = std::remove_pointer_t<P>>
  requires std::is_convertible_v<P, ListItem<T>*>
class ListHead {
 public:
  class iterator {
   public:
    iterator(P it) : it_(it) {}
    iterator(ListItem<T>* it) : it_((P)it) {}
    iterator& operator++() {
      it_ = (P)it_->next;
      return *this;
    }
    iterator operator++(int) {
      iterator copy = *this;
      ++*this;
      return copy;
    }
    iterator& operator--() {
      it_ = (P)it_->prev;
      return *this;
    }
    iterator operator--(int) {
      iterator copy = *this;
      ++*this;
      return copy;
    }
    P operator*() const {
      return it_;
    }
    P operator->() const {
      return it_;
    }
    bool operator==(const iterator& other) const {
      return other.it_ == it_;
    }
    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }
    operator P() {
      return it_;
    }
    operator ListItem<T>*() {
      return it_;
    }

   private:
    P it_;
  };

 private:
  int size_ = 0;
  ListItem<T> head_{}, tail_{};

 public:
  ListHead() {
    init();
  }

  ListHead(const ListHead& o) : ListHead{} {
    for (auto it : o) {
      push_back(new T(*it));
    }
  }

  ~ListHead() {
    clear();
  }

  void init() {
    size_ = 0;
    head_.prev = tail_.next = nullptr;
    link<T>(&head_, &tail_);
  }

  void insert(iterator it, P node) {
    save_DAIF_disable_interrupt();
    size_++;
    link<T>(node, it->next);
    link<T>(it, node);
    restore_DAIF();
  }
  void insert_before(iterator it, P node) {
    insert(--it, node);
  }
  void push_front(P node) {
    insert(&head_, node);
  }
  void push_back(P node) {
    insert(tail_.prev, node);
  }
  void erase(iterator it) {
    save_DAIF_disable_interrupt();
    size_--;
    unlink<T>(it);
    restore_DAIF();
  }

  void clear() {
    save_DAIF_disable_interrupt();
    while (size() > 0) {
      auto it = begin();
      erase(it);
      delete *it;
    }
    restore_DAIF();
  }

  P pop_front() {
    if (empty())
      return nullptr;
    auto it = begin();
    erase(it);
    return it;
  }

  P front() {
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
    return (P)&head_;
  }
  iterator begin() const {
    return (P)head_.next;
  }
  iterator end() const {
    return (P)&tail_;
  }
};
