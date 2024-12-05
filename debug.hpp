#pragma once

#ifdef DBG
#include <cassert>
#include <iostream>
#define ASSERT(x) assert(x)
#define DEBUG(expr) do { std::cout << expr; } while (0)
#else
#define ASSERT(x)
#define DEBUG(expr)
#endif