#pragma once

#include "ds/list.hpp"
#include "mm/mm.hpp"
#include "string.hpp"

bool path_equal(string_view, string_view);

class Path {
  list<const char*> data;
  char* str;

 public:
  Path(const char* name) : data{}, str(strdup(name)) {
    for (auto it = str;;) {
      auto sz = strchr_or_e(it, '/') - it;
      auto end = it[sz] == '\0';
      data.push_back(it);
      if (end)
        break;
      it[sz] = 0;
      it += sz + 1;
    }
  }
  ~Path() {
    kfree(str);
  }

  auto begin() const {
    return data.begin();
  }
  auto end() const {
    return data.end();
  }
};
