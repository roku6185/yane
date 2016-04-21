#ifndef _UTILS_H_
#define _UTILS_H_

#include <ctime>

namespace utils
{
  timespec timespecDiff(timespec *start, timespec *end);
}

#endif
