// //////////////////////////////////////////////////////////
// mywcl.cpp
// Copyright (c) 2013 Stephan Brumme. All rights reserved.
//

// g++ MemoryMapped.cpp mywcl.cpp -o mywcl -O3 -fopenmp

#include "MemoryMapped.h"
#include <cstdio>

int main(int argc, char* argv[])
{
  // syntax check
  if (argc > 2)
  {
    printf("Syntax: ./mywcl filename\n");
    return -1;
  }

  // map file to memory
  MemoryMapped data(argv[1], MemoryMapped::WholeFile, MemoryMapped::SequentialScan);
  if (!data.isValid())
  {
    printf("File not found\n");
    return -2;
  }

  // raw pointer to mapped memory
  char* buffer = (char*)data.getData();
  // store result here
  uint64_t numLines = 0;

  // OpenMP spreads work across CPU cores
#pragma omp parallel for reduction(+:numLines)
  for (uint64_t i = 0; i < data.size(); i++)
    numLines += (buffer[i] == '\n');

  // show result
#ifdef _MSC_VER
  printf("%I64d\n", numLines);
#else
  printf("%lld\n",  numLines);
#endif
  return 0;
}
