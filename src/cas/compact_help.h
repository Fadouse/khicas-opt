#ifndef KHICAS_COMPACT_HELP_H
#define KHICAS_COMPACT_HELP_H

// Included inside giac after the generated text pool and offset rows.
static_assert(sizeof(compact_help_rows[0]) == 12, "Help rows require 16-bit shorts");
static const char * compact_help_field(unsigned row, unsigned column) {
  const auto & entry = compact_help_rows[row];
  unsigned position = entry.offsets[column] | (((entry.banks >> (3 * column)) & 7u) << 16);
  return compact_help_text + position;
}

bool has_static_help(const char * cmd_name, int lang, const char *& howto, const char *& syntax,
                     const char *& related, const char *& examples) {
  if (!cmd_name)
    return false;
  unsigned length = cmd_name[0] == '\'' ? strlen(cmd_name) : 0;
  const bool quoted = length > 2 && cmd_name[length - 1] == '\'';
  if (quoted) {
    ++cmd_name;
    length -= 2;
  }
  const unsigned count = sizeof(compact_help_rows) / sizeof(compact_help_rows[0]);
  unsigned first = 0, last = count, found = count;
  while (first < last) {
    unsigned middle = first + (last - first) / 2;
    const char * name = compact_help_field(middle, 0);
    int order = quoted ? strncmp(name, cmd_name, length) : strcmp(name, cmd_name);
    if (quoted && !order && name[length])
      order = 1;
    if (!order)
      found = middle;
    if (order < 0)
      first = middle + 1;
    else
      last = middle;
  }
  if (found == count)
    return false;
  howto = compact_help_field(found, 1);
  syntax = compact_help_field(found, 2);
  related = compact_help_field(found, 3);
  examples = compact_help_field(found, 4);
  return true;
}

#endif
