#pragma once

#include <string>

#include "program.h"

Program compile_src(const std::string &raw, int dbgWidth, const std::string &filename = "");
