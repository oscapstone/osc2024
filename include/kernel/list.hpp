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
struct ListHead : ListItem {
  class iterator {
   public:
    iterator(T* it) : it_(it) {}
    iterator& operator++() {
      it_ = it_->next;
      return *this;
    }
    iterator operator++(int) {
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

  int size = 0;
  void insert(iterator it, T* node) {
    size++;
    link(node, it->next);
    link(*it, node);
  }
  void erase(iterator it) {
    size--;
    unlink(*it);
  }
  iterator begin() {
    return (T*)next;
  }
  iterator end() {
    return this;
  }
};
