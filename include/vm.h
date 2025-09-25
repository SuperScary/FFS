#pragma once

#include "program.h"

int run (const Program &p,
         int            initCells,
         bool           elastic,
         bool           strict,
         int            dbgWidth,
         bool           trace,
         FILE *         fin,
         FILE *         file_out,
         FILE *         file_err);
