#pragma once

#include <gtest/gtest.h>

extern "C"
{
#include "lillib.h"
#undef printf
}

extern std::vector<char> *g_com_putc_data;
extern std::vector<char> *g_com_getc_data;

