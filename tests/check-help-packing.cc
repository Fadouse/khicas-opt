// Compare generated compact help with every original compiled field and lookup.
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
namespace original {
  struct entry {
    const char *name, *howto, *syntax, *related, *examples;
  };
  const entry entries[] = {
#include "static_helpen.h"
  };
} // namespace original
namespace packed {
#include "compact_helpen.h"
#include "compact_help.h"
} // namespace packed
int main() {
  const unsigned count = sizeof(original::entries) / sizeof(original::entries[0]);
  assert(count == sizeof(packed::compact_help_rows) / sizeof(packed::compact_help_rows[0]));
  std::vector<original::entry> sorted(original::entries, original::entries + count);
  std::stable_sort(sorted.begin(), sorted.end(),
                   [](const original::entry & a, const original::entry & b) {
                     return std::strcmp(a.name, b.name) < 0;
                   });
  for (unsigned i = 0; i < count; ++i) {
    const auto & a = sorted[i];
    const char * fields[] = {a.name, a.howto, a.syntax, a.related, a.examples};
    for (unsigned j = 0; j < 5; ++j) {
      const char * value = packed::compact_help_field(i, j);
      assert(value >= packed::compact_help_text &&
             value < packed::compact_help_text + sizeof(packed::compact_help_text));
      assert(!std::strcmp(fields[j] ? fields[j] : "", value));
    }
    if (i && !std::strcmp(a.name, sorted[i - 1].name))
      continue;
    const char *h, *s, *r, *e;
    assert(packed::has_static_help(a.name, 2, h, s, r, e));
    assert(!std::strcmp(h, a.howto ? a.howto : ""));
    assert(!std::strcmp(s, a.syntax ? a.syntax : ""));
    assert(!std::strcmp(r, a.related ? a.related : ""));
    assert(!std::strcmp(e, a.examples ? a.examples : ""));
    std::string quoted = "'" + std::string(a.name) + "'";
    assert(packed::has_static_help(quoted.c_str(), 2, h, s, r, e));
  }
  const char *h, *s, *r, *e;
  assert(!packed::has_static_help(nullptr, 2, h, s, r, e));
  assert(!packed::has_static_help("zzzz_missing_command", 2, h, s, r, e));
  std::printf(
      "PASS: %u help entries, all five fields byte-identical; quoted, missing and null lookups\n",
      count);
}
