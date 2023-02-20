#pragma once

#include "include/toy_platform.h"

#ifdef TOY_DEBUG
#include <cassert>
#define TOY_ASSERT(x) assert(x)
#else
#define TOY_ASSERT(x)
#endif
