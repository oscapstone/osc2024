#pragma once

#include <concepts>

#include "ds/iterator.hpp"
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

template <typename P, auto Tget = nullptr,
          typename T = std::remove_pointer_t<P>>
  requires std::is_convertible_v<P, ListItem<T>*>
class ListHead {
 public:
  using iterator = Iterator<T, [](P x) { return x->next->get(); }, Tget,
                            [](P x) { return x->prev->get(); }>;

 private:
  int size_ = 0;
  ListItem<T> head_{}, tail_{};

 public:
  ListHead() {
    init();
  }

  ListHead(const ListHead& o) : ListHead{} {
    for (auto it : o) {
      push_back(new T(*(T*)it));
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
    link<T>(node, P(it)->next);
    link<T>(it, node);
    restore_DAIF();
  }
  void insert_before(iterator it, P node) {
    insert(--it, node);
  }
  void push_front(P node) {
    insert(head_.get(), node);
  }
  void push_back(P node) {
    insert(tail_.prev->get(), node);
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
      delete (P)it;
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

template <typename T>
class list {
  struct Item : ListItem<Item> {
    T value;
    Item(T value) : ListItem<Item>{}, value(value) {}
    Item(const Item& o) : ListItem<Item>{}, value(value) {}
  };
  using listtype = ListHead<Item*, [](Item* it) { return it->value; }>;
  listtype data;

 public:
  using iterator = listtype::iterator;

  list() : data{} {}
  ~list() {
    data.clear();
  }

  void init() {
    data.init();
  }

  iterator push_front(T value) {
    auto node = new Item{value};
    data.push_front(node);
    return node;
  }
  iterator push_back(T value) {
    auto node = new Item{value};
    data.push_back(node);
    return node;
  }
  T pop_front() {
    auto node = data.pop_front();
    T r = node->value;
    delete node;
    return r;
  }
  T pop_back() {
    auto node = data.pop_back();
    T r = node->value;
    delete node;
    return r;
  }

  void erase(iterator it) {
    data.erase(it);
  }
  auto empty() const {
    return data.empty();
  }
  auto begin() const {
    return data.begin();
  }
  auto end() const {
    return data.end();
  }
  auto clear() {
    return data.clear();
  }
};
