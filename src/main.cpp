#include <stdio.h>
#include <inttypes.h>
#include <stddef.h>
#include <string>

#include "range_tree.h"

int32_t main(int32_t argc, char **pArgv)
{
  (void)argc;
  (void)pArgv;

  const dim_point<int64_t, 2, std::string> values[] = {
    { { 41, 95 }, "j" },
    { { 12, 3 }, "e" },
    { { 93, 70 }, "n" },
    { { 33, 30 }, "i" },
    { { 67, 89 }, "m" },
    { { 15, 99 }, "f" },
    { { 17, 62 }, "g" },
    { { 2, 19 }, "a" },
    { { 52, 23 }, "k" },
    { { 7, 10 }, "c" },
    { { 21, 49 }, "h" },
    { { 5, 80 }, "b" },
    { { 58, 59 }, "l" },
    { { 8, 37 }, "d" },
  };

  range_tree<int64_t, std::string> tree;
  range_tree<int64_t, std::string>::create(values, std::size(values), tree);
  
  const int64_t min[] = { 16, 53 };
  const int64_t max[] = { 18, 60 };

  std::vector<dim_point<int64_t, 2, std::string>> results = tree.in_range(min, max);

  for (const auto &_p : results)
    printf("%" PRIu64 ", %" PRIu64 ": '%s'\n", _p.p[0], _p.p[1], _p.value.c_str());
}
