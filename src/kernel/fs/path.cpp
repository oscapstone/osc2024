#include "fs/path.hpp"

bool path_equal(string_view a, string_view b) {
  auto nxt_part = [](const char* s) {
    while (*s == '/')
      s++;
    auto e = strchr(s, '/');
    return string_view{s, e - s};
  };

  auto sa = a.begin(), sb = b.begin();
  while (sa < a.end() and sb < b.end() and *sa and *sb) {
    auto pa = nxt_part(sa);
    auto pb = nxt_part(sb);
    if (pa.empty() or pb.empty())
      break;
    if (pa == pb) {
      sa = pa.end();
      sb = pb.end();
    } else if (pa == ".") {
      sa = pa.end();
    } else if (pb == ".") {
      sb = pb.end();
    } else {
      return false;
    }
  }

  return sa == a.end() and sb == b.end();
}
