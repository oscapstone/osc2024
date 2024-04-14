#pragma once

#include <concepts>

struct ListItem {
  ListItem *prev, *next;
  bool empty() const {
    return prev == this and this == next;
  }
  ListItem() : prev(this), next(this) {}
};

inline void link(ListItem* prev, ListItem* next) {
  if (prev)
    prev->next = next;
  if (next)
    next->prev = prev;
}

inline void unlink(ListItem* it) {
  link(it->prev, it->next);
}

template <std::derived_from<ListItem> T>
class ListHead : ListItem {
 public:
  class iterator {
   public:
    iterator(T* it) : it_(it) {}
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
    T* operator*() const {
      return it_;
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

   private:
    T* it_;
  };

 private:
  int size_ = 0;
  ListItem head_, tail_;

 public:
  void init() {
    size_ = 0;
    link(&head_, &tail_);
  }

  void insert(iterator it, T* node) {
    size_++;
    link(node, it->next);
    link(*it, node);
  }
  void erase(iterator it) {
    size_--;
    unlink(*it);
  }

  int size() const {
    return size_;
  }
  bool empty() const {
    return size_ == 0;
  }

  iterator begin() {
    return (T*)head_.next;
  }
  iterator end() {
    return (T*)&tail_;
  }
};
