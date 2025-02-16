// RUN: %gdb-compile 2>&1 | tee %t.compile
// RUN: env OMP_SCHEDULE=guided %gdb-run 2>&1 | tee %t.out | FileCheck %s

#include <stdio.h>
#include <unistd.h>
#include <omp.h>
#include <pthread.h>
#include "../ompt_plugin.h"

int main()
{
  printf("Application: Process %d started.\n", getpid());
  
  omp_set_num_threads(3);
  omp_set_max_active_levels(10);

  #pragma omp parallel // parallel region begins
  {
    printf("Outer region - thread_ID: %d\n", omp_get_thread_num());

    #pragma omp parallel num_threads(1) //nested parallel region 1
    {
      printf("Inner region - thread_ID: %d\n", omp_get_thread_num());

      #pragma omp parallel num_threads(2) //nested parallel region 2
      {
        int i;
        #pragma omp for
        for(i=0; i<15; i++)
          ompd_tool_test(0);
      }
    }
  }

  return 0;
}

// CHECK-NOT: OMPT-OMPD mismatch
// CHECK-NOT: Python Exception
// CHECK-NOT: The program is not being run.
