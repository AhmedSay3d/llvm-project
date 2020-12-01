// RUN: %gdb-compile 2>&1 | tee %t.compile
// RUN: %gdb-test -x %S/test_ompd_rel_address_space_handle.cmd %t 2>&1 | tee %t.out | FileCheck %s
#include <stdio.h>
#include <omp.h>

int main () {
	omp_set_num_threads(4);
	#pragma omp parallel
	{
		printf ("Parallel level 1, thread num = %d.\n", omp_get_thread_num());
	}
	return 0;
}
// CHECK-NOT: Failed
// CHECK-NOT: Skip
