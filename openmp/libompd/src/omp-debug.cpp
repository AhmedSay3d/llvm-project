/*
 * omp-debug.cpp
 *
 *  Created on: Jan 14, 2015
 *      Author: Ignacio Laguna
 *              Joachim Protze
 *     Contact: ilaguna@llnl.gov
 *              protze@llnl.gov
 */
/*******************************************************************************
 * This implements an OMPD DLL for the LLVM OpenMP runtime library.
 */

#define NDEBUG 1

#include "omp-debug.h"
#include "omp.h"
#include "ompd-private.h"
#include "TargetValue.h"
#include <assert.h>
#include <cstdio>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>

ompd_device_type_sizes_t type_sizes;
uint64_t ompd_state;

/* --- OMPD functions ------------------------------------------------------- */

/* --- Initialization ------------------------------------------------------- */

ompd_rc_t ompd_initialize(ompd_word_t version, const ompd_callbacks_t *table) {
  ompd_rc_t ret = table ? ompd_rc_ok : ompd_rc_bad_input;
  callbacks = table;
  TValue::callbacks = table;
  __ompd_init_icvs(table);
  __ompd_init_states(table);

  return ret;
}

ompd_rc_t ompd_finalize(void) { return ompd_rc_ok; }

ompd_rc_t
ompd_process_initialize(ompd_address_space_context_t
                            *context, /* IN: debugger handle for the target */
                        ompd_address_space_handle_t *
                            *handle /* OUT: ompd handle for the target */
                        ) {
  if (!context)
    return ompd_rc_bad_input;
  if (!handle)
    return ompd_rc_bad_input;

  ompd_rc_t ret = initTypeSizes(context);
  if (ret != ompd_rc_ok)
    return ret;

  ret = TValue(context, "ompd_state")
            .castBase(ompd_type_long_long)
            .getValue(ompd_state);
  if (ret != ompd_rc_ok)
    return ret;
  ret = callbacks->alloc_memory(sizeof(ompd_address_space_handle_t),
                                 (void **)(handle));
  if (ret != ompd_rc_ok)
    return ret;
  if (!handle)
    return ompd_rc_error;
  (*handle)->context = context;
  (*handle)->kind = OMPD_DEVICE_KIND_HOST;

  return ompd_rc_ok;
}

ompd_rc_t
ompd_get_omp_version(ompd_address_space_handle_t
                            *address_space, /* IN: handle for the address space */
                     ompd_word_t *version) {
  if (!address_space)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = address_space->context;
  ompd_rc_t ret;

  if (!context)
    return ompd_rc_stale_handle;

  if (!callbacks) {
    return ompd_rc_callback_error;
  }

  ret = TValue(context, "__kmp_openmp_version")
            .castBase(ompd_type_int)
            .getValue(*version);
  return ret;
}

ompd_rc_t ompd_get_omp_version_string(
    ompd_address_space_handle_t
        *address_space, /* IN: handle for the address space */
    const char **string) {
  if (!address_space)
    return ompd_rc_bad_input;
  static const char *omp_version = "";
  *string = omp_version;
  return ompd_rc_ok;
}

ompd_rc_t ompd_rel_address_space_handle(
    ompd_address_space_handle_t
        *addr_handle /* IN: handle for the address space */
    ) {
  if (!addr_handle)
    return ompd_rc_bad_input;

  ompd_rc_t ret = callbacks->free_memory((void *)(addr_handle));
//  delete addr_handle;
  return ret;
}

ompd_rc_t ompd_device_initialize(
    ompd_address_space_handle_t *process_handle,
    ompd_address_space_context_t *device_context,
    ompd_device_t kind,
    ompd_size_t sizeof_id,
    void *id,
    ompd_address_space_handle_t **device_handle
    )
{
  if (!device_context)
    return ompd_rc_bad_input;

  ompd_rc_t ret;
  uint64_t ompd_num_cuda_devices;

  ret = TValue(process_handle->context, "ompd_num_cuda_devices").
        castBase(ompd_type_long_long).
        getValue(ompd_num_cuda_devices);
  if (ret != ompd_rc_ok)
    return ret;


  for (uint64_t i = 0; i < ompd_num_cuda_devices; i++) {
    uint64_t cuda_ctx;

    ret = TValue(process_handle->context, "ompd_CudaDeviceDataArray").
          cast("DeviceDataTy",1).
          getArrayElement(i).
          access("Context").
          castBase(ompd_type_long_long).
          getValue(cuda_ctx);

    if ( ret != ompd_rc_ok )
      continue;

    if (cuda_ctx == *((uint64_t *)id)) {
      ret = callbacks->alloc_memory(sizeof(ompd_address_space_handle_t),
                                     (void **)(device_handle));
      if (ret != ompd_rc_ok)
        return ret;
      if (!device_handle)
        return ompd_rc_error;
      (*device_handle)->context = device_context;
      (*device_handle)->kind = OMPD_DEVICE_KIND_CUDA;
      (*device_handle)->id = (uint64_t)id;
      return ompd_rc_ok;
    }
  }

  return ompd_rc_unavailable;
}

/* --- Thread Handles ------------------------------------------------------- */

/* thread_handle is of type (kmp_base_info_t) */

ompd_rc_t ompd_get_thread_in_parallel(
    ompd_parallel_handle_t *parallel_handle, /* IN: OpenMP parallel handle */
    int thread_num, /* OUT: number of handles in the array */
    ompd_thread_handle_t **thread_handle /* OUT: handle */
    ) {
  if (!parallel_handle)
    return ompd_rc_stale_handle;
  if (!parallel_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = parallel_handle->ah->context;
  ompd_rc_t ret;

  if (!context)
    return ompd_rc_stale_handle;

  if (!callbacks) {
    return ompd_rc_callback_error;
  }

  ompd_address_t taddr={OMPD_SEGMENT_UNSPECIFIED,0};

  if (parallel_handle->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    uint16_t thread_idx;
    // We cannot use the task descriptor associated with the parallel info as
    // their task might not be currently active
    // So to get the current thread, we access the tasks thread info and get
    // get its threadIdx.x
    auto TaskDescr  = TValue(context, parallel_handle->th)
                        .cast("ompd_nvptx_parallel_info_t", 0,
                              OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                        .access("parallel_tasks")
                        .cast("omptarget_nvptx_TaskDescr", 1,
                              OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                        .getArrayElement(thread_num);

    ret = TaskDescr.access("ompd_thread_info")
                   .cast("ompd_nvptx_thread_info_t", 0,
                         OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                   .access("threadIdx_x")
                  .castBase(ompd_type_short)
                  .getValue(thread_idx);

    if (ret != ompd_rc_ok) {
      return ret;
    }

    ret = TValue(context, NULL,
                 "omptarget_nvptx_threadPrivateContext",
                 OMPD_SEGMENT_CUDA_PTX_SHARED)
            .cast("omptarget_nvptx_ThreadPrivateContext", 1,
                  OMPD_SEGMENT_CUDA_PTX_SHARED)
            .access("topTaskDescr")
            .cast("omptarget_nvptx_TaskDescr", 2,
                  OMPD_SEGMENT_CUDA_PTX_GLOBAL)
            .getPtrArrayElement(thread_idx)
            .dereference()
            .getAddress(&taddr);

    if (taddr.address == 0 && thread_idx % 32 == 0) {
      ret = TaskDescr.getAddress(&taddr);
    }
  } else {
    ret = TValue(context, parallel_handle->th) /* t */
              .cast("kmp_base_team_t", 0)
              .access("t_threads") /*t.t_threads*/
              .cast("kmp_info_t", 2)
              .getArrayElement(thread_num) /*t.t_threads[nth_handle]*/
              .access("th")                /*t.t_threads[i]->th*/
              .getAddress(&taddr);
  }

  if (ret != ompd_rc_ok)
    return ret;

  ret = callbacks->alloc_memory(sizeof(ompd_thread_handle_t),
                                 (void **)(thread_handle));
  if (ret != ompd_rc_ok)
    return ret;

  (*thread_handle)->th = taddr;
  (*thread_handle)->ah = parallel_handle->ah;
  (*thread_handle)->cuda_kernel_info = parallel_handle->cuda_kernel_info;
  return ret;
}

ompd_rc_t ompd_rel_thread_handle(
    ompd_thread_handle_t *thread_handle /* IN: OpenMP parallel handle */
    ) {
  if (!thread_handle)
    return ompd_rc_stale_handle;
  ompd_rc_t ret = callbacks->free_memory((void *)(thread_handle));
  if (ret != ompd_rc_ok)
    return ret;
  return ompd_rc_ok;
}

ompd_rc_t ompd_thread_handle_compare(ompd_thread_handle_t *thread_handle_1,
                                     ompd_thread_handle_t *thread_handle_2,
                                     int *cmp_value) {
  if (!thread_handle_1)
    return ompd_rc_stale_handle;
  if (!thread_handle_2)
    return ompd_rc_stale_handle;
  if (thread_handle_1->ah->kind != thread_handle_2->ah->kind)
    return ompd_rc_bad_input;
  *cmp_value = thread_handle_1->th.address - thread_handle_2->th.address;
  if (*cmp_value == 0 && thread_handle_1->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    *cmp_value = thread_handle_1->cuda_kernel_info->cudaDevId -
        thread_handle_2->cuda_kernel_info->cudaDevId;
    if (*cmp_value == 0) {
      *cmp_value = thread_handle_1->cuda_kernel_info->cudaContext -
          thread_handle_2->cuda_kernel_info->cudaContext;
    }
    if (*cmp_value == 0) {
      *cmp_value = thread_handle_1->cuda_kernel_info->warpSize -
          thread_handle_2->cuda_kernel_info->warpSize;
    }
    if (*cmp_value == 0) {
      *cmp_value = thread_handle_1->cuda_kernel_info->gridId -
          thread_handle_2->cuda_kernel_info->gridId;
    }
  }

  return ompd_rc_ok;
}

/* --- Parallel Region Handles----------------------------------------------- */

/* parallel_handle is of type (kmp_base_team_t)*/

ompd_rc_t ompd_get_curr_parallel_handle(
    ompd_thread_handle_t *thread_handle,     /* IN: OpenMP thread handle*/
    ompd_parallel_handle_t **parallel_handle /* OUT: OpenMP parallel handle */
    ) {
  if (!thread_handle)
    return ompd_rc_stale_handle;
  if (!thread_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = thread_handle->ah->context;
  ompd_thread_context_t *thread_context = thread_handle->thread_context;
  if (!context || !thread_context)
    return ompd_rc_stale_handle;

  if (!callbacks) {
    return ompd_rc_callback_error;
  }

  ompd_rc_t ret;

  if (thread_handle->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    ompd_address_t taddr;
    TValue ph;
    // The ompd_parallel_info_t we need is only present in the previous task
    // of an implicit task.
    uint16_t task_is_implicit = 0;
    ret = ompd_rc_ok;
    auto possibleTaskDescr = TValue(context, thread_handle->th)
                              .cast("omptarget_nvptx_TaskDescr", 0,
                                     OMPD_SEGMENT_CUDA_PTX_GLOBAL);

    while (!task_is_implicit && ret == ompd_rc_ok) {
      ret = possibleTaskDescr.access("ompd_thread_info")
                             .cast("ompd_nvptx_thread_info_t", 0,
                                   OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                             .access("task_implicit")
                             .castBase()
                             .getValue(task_is_implicit);
      possibleTaskDescr = possibleTaskDescr.access("prev")
                                           .cast("omptarget_nvptx_TaskDescr",
                                                 1, OMPD_SEGMENT_CUDA_PTX_GLOBAL);
      ret = possibleTaskDescr.dereference().getAddress(&taddr);
    }

    if (ret != ompd_rc_ok) {
      if (taddr.address == 0) {
        ph = TValue(context, NULL, "omptarget_nvptx_threadPrivateContext")
                 .cast("omptarget_nvptx_ThreadPrivateContext", 1,
                       OMPD_SEGMENT_CUDA_PTX_SHARED)
                  .access("ompd_levelZeroParallelInfo")
                  .cast("ompd_nvptx_parallel_info_t", 0,
                        OMPD_SEGMENT_CUDA_PTX_GLOBAL);
      } else {
        return ret;
      }
    } else {
      ph = possibleTaskDescr.access("ompd_thread_info")
               .cast("ompd_nvptx_thread_info_t", 0,
                      OMPD_SEGMENT_CUDA_PTX_GLOBAL)
               .access("enclosed_parallel")
               .cast("ompd_nvptx_parallel_info_t", 0,
                     OMPD_SEGMENT_CUDA_PTX_GLOBAL);
    }

    ret = ph.getAddress(&taddr);
    if (ret != ompd_rc_ok)
      return ret;

    ret = callbacks->alloc_memory(sizeof(ompd_parallel_handle_t),
                                 (void **)(parallel_handle));
    if (ret != ompd_rc_ok)
      return ret;

    (*parallel_handle)->ah = thread_handle->ah;
    (*parallel_handle)->th = taddr;
    (*parallel_handle)->cuda_kernel_info = thread_handle->cuda_kernel_info;
  } else {
    ompd_address_t taddr={OMPD_SEGMENT_UNSPECIFIED,0}, lwt={OMPD_SEGMENT_UNSPECIFIED,0};

    TValue teamdata = TValue(context, thread_handle->th) /*__kmp_threads[t]->th*/
                          .cast("kmp_base_info_t")
                          .access("th_team") /*__kmp_threads[t]->th.th_team*/
                          .cast("kmp_team_p", 1)
                          .access("t"); /*__kmp_threads[t]->th.th_team->t*/

    ret = teamdata.getAddress(&taddr);
    if (ret != ompd_rc_ok)
      return ret;

    lwt.segment = OMPD_SEGMENT_UNSPECIFIED;
    ret = teamdata.cast("kmp_base_team_t", 0)
              .access("ompt_serialized_team_info")
              .castBase()
              .getValue(lwt.address);
    if (ret != ompd_rc_ok)
      return ret;

    ret = callbacks->alloc_memory(sizeof(ompd_parallel_handle_t),
                                   (void **)(parallel_handle));
    if (ret != ompd_rc_ok)
      return ret;

    (*parallel_handle)->ah = thread_handle->ah;
    (*parallel_handle)->th = taddr;
    (*parallel_handle)->lwt = lwt;
  }
  return ompd_rc_ok;
}

ompd_rc_t ompd_get_enclosing_parallel_handle(
    ompd_parallel_handle_t *parallel_handle, /* IN: OpenMP parallel handle */
    ompd_parallel_handle_t *
        *enclosing_parallel_handle /* OUT: OpenMP parallel handle */
    ) {
  if (!parallel_handle)
    return ompd_rc_stale_handle;
  if (!parallel_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = parallel_handle->ah->context;

  if (!context)
    return ompd_rc_stale_handle;

  if (!callbacks) {
    return ompd_rc_callback_error;
  }

  ompd_address_t taddr = parallel_handle->th, lwt={OMPD_SEGMENT_UNSPECIFIED,0};
  ompd_rc_t ret;

  if (parallel_handle->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    uint16_t level;
    TValue curParallelInfo = TValue(context, taddr)
                             .cast("ompd_nvptx_parallel_info_t", 0,
                                   OMPD_SEGMENT_CUDA_PTX_SHARED);

    ret = curParallelInfo
            .cast("ompd_nvptx_parallel_info_t", 0,
                  OMPD_SEGMENT_CUDA_PTX_GLOBAL)
            .access("level")
            .castBase(ompd_type_short)
            .getValue(level);

    if (ret != ompd_rc_ok)
      return ret;

    TValue prevTaskDescr = curParallelInfo.cast("ompd_nvptx_parallel_info_t", 0,
                                                OMPD_SEGMENT_CUDA_PTX_SHARED)
                                          .access("parallel_tasks")
                                          .cast("omptarget_nvptx_TaskDescr", 1,
                                                OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                                          .access("prev")
                                          .cast("omptarget_nvptx_TaskDescr", 1,
                                                OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                                          .dereference();

    ret = prevTaskDescr.getAddress(&taddr);

    // If the previous task of the tasks of the current parallel region is
    // NULL, then we got the parallel handle for the (implicit?) top level
    // task which has no enclosing task.
    if (ret != ompd_rc_ok) {
      return ret;
    }

    // The instance of TaskDescr for the previous task contains the parallel
    // info for the current parallel region. So we have to go back to the
    // previous task of the previous task
    prevTaskDescr = prevTaskDescr.access("prev")
                                 .cast("omptarget_nvptx_TaskDescr", 1,
                                       OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                                 .dereference();

    ret = prevTaskDescr.getAddress(&taddr);

      if (ret != ompd_rc_ok) {
      if (taddr.address == 0 && level == 1) {
        // If we are in generic mode, there is an implicit parallel region
        // around the master thread
        prevTaskDescr = TValue(context, NULL, "omptarget_nvptx_threadPrivateContext",
                               OMPD_SEGMENT_CUDA_PTX_SHARED)
                            .cast("omptarget_nvptx_ThreadPrivateContext", 1,
                                  OMPD_SEGMENT_CUDA_PTX_SHARED)
                            .access("ompd_levelZeroParallelInfo");
      } else {
        return ret;
      }
    } else {
      prevTaskDescr = prevTaskDescr.access("ompd_thread_info")
                                   .cast("ompd_nvptx_thread_info_t", 0,
                                         OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                                   .access("enclosed_parallel");
    }

    ret = prevTaskDescr.cast("ompd_nvptx_parallel_info_t", 0,
                             OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                       .getAddress(&taddr);

    if (ret != ompd_rc_ok) {
      return ret;
    }
  } else {
    ret = ompd_rc_stale_handle;
    TValue lwtValue = TValue(context, parallel_handle->lwt);
    if (lwtValue.getError() == ompd_rc_ok) // lwt == 0x0
    {                                      // if we are in lwt, get parent
      ret = lwtValue.cast("ompt_lw_taskteam_t", 0)
                .access("parent")
                .cast("ompt_lw_taskteam_t", 1)
                .dereference()
                .getAddress(&lwt);
    }
    if (ret != ompd_rc_ok) { // no lwt or parent==0x0

      TValue teamdata =
          TValue(context, parallel_handle->th) /*__kmp_threads[t]->th*/
              .cast("kmp_base_team_t", 0)      /*t*/
              .access("t_parent")              /*t.t_parent*/
              .cast("kmp_team_p", 1)
              .access("t"); /*t.t_parent->t*/

      ret = teamdata.getAddress(&taddr);
      if (ret != ompd_rc_ok)
        return ret;

      lwt.segment = OMPD_SEGMENT_UNSPECIFIED;
      ret = teamdata.cast("kmp_base_team_t", 0)
                .access("ompt_serialized_team_info")
                .castBase()
                .getValue(lwt.address);
      if (ret != ompd_rc_ok)
        return ret;
    }
  }

  ret = callbacks->alloc_memory(sizeof(ompd_parallel_handle_t),
                                 (void **)(enclosing_parallel_handle));
  if (ret != ompd_rc_ok)
    return ret;
  (*enclosing_parallel_handle)->th = taddr;
  (*enclosing_parallel_handle)->lwt = lwt;
  (*enclosing_parallel_handle)->ah = parallel_handle->ah;
  (*enclosing_parallel_handle)->cuda_kernel_info =
      parallel_handle->cuda_kernel_info;
  return ompd_rc_ok;
}

ompd_rc_t ompd_get_task_parallel_handle(
    ompd_task_handle_t *task_handle, /* IN: OpenMP task handle */
    ompd_parallel_handle_t *
        *task_parallel_handle /* OUT: OpenMP parallel handle */
    ) {
  if (!task_handle)
    return ompd_rc_stale_handle;
  if (!task_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = task_handle->ah->context;

  if (!context)
    return ompd_rc_stale_handle;

  if (!callbacks) {
    return ompd_rc_callback_error;
  }

  ompd_address_t taddr={OMPD_SEGMENT_UNSPECIFIED,0};

  ompd_rc_t ret;

  if (task_handle->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    TValue parallelHandle;
    // The ompd_parallel_info_t we need is only present in the previous task
    // of an implicit task.
    uint16_t task_is_implicit = 0;
    ret = ompd_rc_ok;
    auto possibleTaskDescr = TValue(context, task_handle->th)
                              .cast("omptarget_nvptx_TaskDescr", 0,
                                     OMPD_SEGMENT_CUDA_PTX_GLOBAL);

    while (!task_is_implicit && ret == ompd_rc_ok) {
      ret = possibleTaskDescr.access("ompd_thread_info")
                             .cast("ompd_nvptx_thread_info_t", 0,
                                   OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                             .access("task_implicit")
                             .castBase()
                             .getValue(task_is_implicit);
      possibleTaskDescr = possibleTaskDescr.access("prev")
                                           .cast("omptarget_nvptx_TaskDescr",
                                                 1, OMPD_SEGMENT_CUDA_PTX_GLOBAL);
      ret = possibleTaskDescr.dereference().getAddress(&taddr);
    }

    if (ret != ompd_rc_ok) {
      if (taddr.address == 0) {
        parallelHandle = TValue(context, NULL,
                                "omptarget_nvptx_threadPrivateContext")
                            .cast("omptarget_nvptx_ThreadPrivateContext", 1,
                                  OMPD_SEGMENT_CUDA_PTX_SHARED)
                            .access("ompd_levelZeroParallelInfo")
                            .cast("ompd_nvptx_parallel_info_t", 0,
                                  OMPD_SEGMENT_CUDA_PTX_GLOBAL);
      } else {
        return ret;
      }
    } else {
      parallelHandle = possibleTaskDescr.access("ompd_thread_info")
                                        .cast("ompd_nvptx_thread_info_t", 0,
                                              OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                                        .access("enclosed_parallel")
                                        .cast("ompd_nvptx_parallel_info_t", 0,
                                              OMPD_SEGMENT_CUDA_PTX_GLOBAL);
    }
    ret = parallelHandle.getAddress(&taddr);
  } else {
    ret = TValue(context, task_handle->th)
              .cast("kmp_taskdata_t") /*td*/
              .access("td_team")      /*td.td_team*/
              .cast("kmp_team_p", 1)
              .access("t") /*td.td_team->t*/
              .getAddress(&taddr);
  }
  if (ret != ompd_rc_ok)
    return ret;

  ret = callbacks->alloc_memory(sizeof(ompd_parallel_handle_t),
                                 (void **)(task_parallel_handle));
  if (ret != ompd_rc_ok)
    return ret;

  (*task_parallel_handle)->ah = task_handle->ah;
  (*task_parallel_handle)->lwt = task_handle->lwt;
  (*task_parallel_handle)->th = taddr;
  (*task_parallel_handle)->cuda_kernel_info = task_handle->cuda_kernel_info;
  return ompd_rc_ok;
}

ompd_rc_t ompd_rel_parallel_handle(
    ompd_parallel_handle_t *parallel_handle /* IN: OpenMP parallel handle */
    ) {
  if (!parallel_handle)
    return ompd_rc_stale_handle;
  ompd_rc_t ret = callbacks->free_memory((void *)(parallel_handle));
  if (ret != ompd_rc_ok)
    return ret;
  return ompd_rc_ok;
}

ompd_rc_t
ompd_parallel_handle_compare(ompd_parallel_handle_t *parallel_handle_1,
                             ompd_parallel_handle_t *parallel_handle_2,
                             int *cmp_value) {
  if (!parallel_handle_1)
    return ompd_rc_stale_handle;
  if (!parallel_handle_2)
    return ompd_rc_stale_handle;
  if (parallel_handle_1->ah->kind != parallel_handle_2->ah->kind)
    return ompd_rc_bad_input;
  if (parallel_handle_1->ah->kind == OMPD_DEVICE_KIND_HOST) {
    if (parallel_handle_1->th.address - parallel_handle_2->th.address)
      *cmp_value = parallel_handle_1->th.address - parallel_handle_2->th.address;
    else
      *cmp_value =
          parallel_handle_1->lwt.address - parallel_handle_2->lwt.address;
  } else {
    *cmp_value = parallel_handle_1->th.address - parallel_handle_2->th.address;
  }
  return ompd_rc_ok;
}

/* ------- Task Handles ----------------------------------------------------- */

/* task_handle is of type (kmp_taskdata_t) */

ompd_rc_t ompd_get_curr_task_handle(
    ompd_thread_handle_t *thread_handle, /* IN: OpenMP thread handle*/
    ompd_task_handle_t **task_handle     /* OUT: OpenMP task handle */
    ) {
  if (!thread_handle)
    return ompd_rc_stale_handle;
  if (!thread_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = thread_handle->ah->context;
  if (!context)
    return ompd_rc_stale_handle;

  if (!callbacks) {
    return ompd_rc_callback_error;
  }

  ompd_address_t taddr={OMPD_SEGMENT_UNSPECIFIED,0}, lwt={OMPD_SEGMENT_UNSPECIFIED,0};
  ompd_rc_t ret = ompd_rc_ok;

  lwt.segment = OMPD_SEGMENT_UNSPECIFIED;

  if (thread_handle->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    lwt.address = 0;
    taddr = thread_handle->th;
  } else {
    TValue taskdata =
        TValue(context, thread_handle->th) /*__kmp_threads[t]->th*/
            .cast("kmp_base_info_t")
            .access("th_current_task") /*__kmp_threads[t]->th.th_current_task*/
            .cast("kmp_taskdata_t", 1);

    ret = taskdata.dereference().getAddress(&taddr);
    if (ret != ompd_rc_ok)
      return ret;

    ret = taskdata
              .access("td_team") /*td.td_team*/
              .cast("kmp_team_p", 1)
              .access("t") /*td.td_team->t*/
              .cast("kmp_base_team_t", 0)
              .access("ompt_serialized_team_info")
              .castBase()
              .getValue(lwt.address);
  }
  if (ret != ompd_rc_ok)
    return ret;

  ret = callbacks->alloc_memory(sizeof(ompd_task_handle_t),
                                 (void **)(task_handle));
  if (ret != ompd_rc_ok)
    return ret;

  (*task_handle)->th = taddr;
  (*task_handle)->lwt = lwt;
  (*task_handle)->ah = thread_handle->ah;
  (*task_handle)->cuda_kernel_info = thread_handle->cuda_kernel_info;
  return ompd_rc_ok;
}

ompd_rc_t ompd_get_generating_task_handle(
    ompd_task_handle_t *task_handle,        /* IN: OpenMP task handle */
    ompd_task_handle_t **parent_task_handle /* OUT: OpenMP task handle */
    ) {
  // Generating and Scheduling task are the same on cuda?
  if (task_handle->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    return ompd_get_scheduling_task_handle(task_handle, parent_task_handle);
  }

  if (!task_handle)
    return ompd_rc_stale_handle;
  if (!task_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = task_handle->ah->context;
  if (!context)
    return ompd_rc_stale_handle;
  if (!callbacks) {
    return ompd_rc_callback_error;
  }

  ompd_address_t taddr = task_handle->th, lwt={OMPD_SEGMENT_UNSPECIFIED,0};

  ompd_rc_t ret = ompd_rc_stale_handle;
  TValue lwtValue = TValue(context, task_handle->lwt);
  if (lwtValue.getError() == ompd_rc_ok) // lwt == 0x0
  {                                      // if we are in lwt, get parent
    ret = lwtValue.cast("ompt_lw_taskteam_t", 0)
              .access("parent")
              .cast("ompt_lw_taskteam_t", 1)
              .dereference()
              .getAddress(&lwt);
  }
  if (ret != ompd_rc_ok) { // no lwt or parent==0x0

    TValue taskdata = TValue(context, task_handle->th) /*__kmp_threads[t]->th*/
                          .cast("kmp_taskdata_t")      /*td*/
                          .access("td_parent")         /*td->td_parent*/
                          .cast("kmp_taskdata_t", 1);

    ret = taskdata.dereference().getAddress(&taddr);
    if (ret != ompd_rc_ok)
      return ret;

    lwt.segment = OMPD_SEGMENT_UNSPECIFIED;
    ret = taskdata
              .access("td_team") /*td.td_team*/
              .cast("kmp_team_p", 1)
              .access("t") /*td.td_team->t*/
              .cast("kmp_base_team_t", 0)
              .access("ompt_serialized_team_info")
              .castBase()
              .getValue(lwt.address);
    if (ret != ompd_rc_ok)
      return ret;
  }

  ret = callbacks->alloc_memory(sizeof(ompd_task_handle_t),
                                 (void **)(parent_task_handle));
  if (ret != ompd_rc_ok)
    return ret;

  (*parent_task_handle)->th = taddr;
  (*parent_task_handle)->lwt = lwt;
  (*parent_task_handle)->ah = task_handle->ah;
  return ret;
}

ompd_rc_t ompd_get_scheduling_task_handle(
    ompd_task_handle_t *task_handle,        /* IN: OpenMP task handle */
    ompd_task_handle_t **parent_task_handle /* OUT: OpenMP task handle */
    ) {
  if (!task_handle)
    return ompd_rc_stale_handle;
  if (!task_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = task_handle->ah->context;
  if (!context)
    return ompd_rc_stale_handle;

  if (!callbacks) {
    return ompd_rc_callback_error;
  }

  ompd_address_t taddr={OMPD_SEGMENT_UNSPECIFIED,0};
  ompd_rc_t ret;

  if (task_handle->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    ret = TValue(context, task_handle->th)
            .cast("omptarget_nvptx_TaskDescr", 0,
                  OMPD_SEGMENT_CUDA_PTX_GLOBAL)
            .access("prev")
            .cast("omptarget_nvptx_TaskDescr", 1,
                  OMPD_SEGMENT_CUDA_PTX_GLOBAL)
            .dereference()
            .getAddress(&taddr);
    if (taddr.address == 0) {
      return ompd_rc_unavailable;
    }
  } else {
    ret =
        TValue(context, task_handle->th)
            .cast("kmp_taskdata_t")   /*td*/
            .access("ompt_task_info") // td->ompt_task_info
            .cast("ompt_task_info_t")
            .access("scheduling_parent") // td->ompd_task_info.scheduling_parent
            .cast("kmp_taskdata_t", 1)
            .castBase()
            .getValue(taddr.address);
    if (taddr.address == 0) {
      return ompd_rc_unavailable;
    }
  }

  if (ret != ompd_rc_ok)
    return ret;
  ret = callbacks->alloc_memory(sizeof(ompd_task_handle_t),
                                 (void **)(parent_task_handle));
  if (ret != ompd_rc_ok)
    return ret;

  (*parent_task_handle)->th = taddr;
  (*parent_task_handle)->lwt = {OMPD_SEGMENT_UNSPECIFIED,0};
  (*parent_task_handle)->ah = task_handle->ah;
  (*parent_task_handle)->cuda_kernel_info = task_handle->cuda_kernel_info;
  return ret;
}

ompd_rc_t ompd_get_task_in_parallel(
    ompd_parallel_handle_t *parallel_handle, /* IN: OpenMP parallel handle */
    int thread_num,                  /* OUT: number of the task handle */
    ompd_task_handle_t **task_handle /* OUT: OpenMP task handle */
    ) {
  if (!parallel_handle)
    return ompd_rc_stale_handle;
  if (!parallel_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = parallel_handle->ah->context;
  if (!context)
    return ompd_rc_stale_handle;

  if (!callbacks) {
    return ompd_rc_callback_error;
  }

  ompd_rc_t ret;
  ompd_address_t taddr={OMPD_SEGMENT_UNSPECIFIED,0};

  if (parallel_handle->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    ret = TValue(context, parallel_handle->th)
              .cast("ompd_nvptx_parallel_info_t", 0,
                    OMPD_SEGMENT_CUDA_PTX_GLOBAL)
              .access("parallel_tasks")
              .cast("omptarget_nvptx_TaskDescr", 1,
                    OMPD_SEGMENT_CUDA_PTX_GLOBAL)
              .getArrayElement(thread_num)
              .getAddress(&taddr);
  } else {
    ret = TValue(context, parallel_handle->th) /* t */
              .cast("kmp_base_team_t", 0)
              .access("t_implicit_task_taskdata") /*t.t_implicit_task_taskdata*/
              .cast("kmp_taskdata_t", 1)
              .getArrayElement(
                  thread_num) /*t.t_implicit_task_taskdata[nth_handle]*/
              .getAddress(&taddr);
  }

  if (ret != ompd_rc_ok)
    return ret;
  ret = callbacks->alloc_memory(sizeof(ompd_task_handle_t),
                                 (void **)(task_handle));
  if (ret != ompd_rc_ok)
    return ret;

  (*task_handle)->th = taddr;
  (*task_handle)->ah = parallel_handle->ah;
  (*task_handle)->lwt = {OMPD_SEGMENT_UNSPECIFIED,0};
  (*task_handle)->cuda_kernel_info = parallel_handle->cuda_kernel_info;
  return ret;
}

ompd_rc_t ompd_rel_task_handle(
    ompd_task_handle_t *task_handle /* IN: OpenMP task handle */
    ) {
  if (!task_handle)
    return ompd_rc_stale_handle;
  ompd_rc_t ret = callbacks->free_memory((void *)(task_handle));
  if (ret != ompd_rc_ok)
    return ret;
  return ompd_rc_ok;
}

ompd_rc_t ompd_task_handle_compare(ompd_task_handle_t *task_handle_1,
                                   ompd_task_handle_t *task_handle_2,
                                   int *cmp_value) {
  if (!task_handle_1)
    return ompd_rc_stale_handle;
  if (!task_handle_2)
    return ompd_rc_stale_handle;
  if (task_handle_1->ah->kind != task_handle_2->ah->kind)
    return ompd_rc_bad_input;
  if (task_handle_1->th.address - task_handle_2->th.address ||
        task_handle_1->ah->kind == OMPD_DEVICE_KIND_CUDA)
    *cmp_value = task_handle_1->th.address - task_handle_2->th.address;
  else
    *cmp_value = task_handle_1->lwt.address - task_handle_2->lwt.address;
  return ompd_rc_ok;
}


ompd_rc_t
ompd_get_thread_handle(ompd_address_space_handle_t
                           *handle, /* IN: handle for the address space */
                       ompd_thread_id_t kind,
                       ompd_size_t sizeof_thread_id, const void *thread_id,
                       ompd_thread_handle_t **thread_handle) {
  if (!handle)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = handle->context;
  ompd_rc_t ret;

  if (!context)
    return ompd_rc_stale_handle;

  if (!callbacks) {
    return ompd_rc_callback_error;
  }
  ompd_thread_context_t *tcontext;
  ret = callbacks->get_thread_context_for_thread_id(
      context, kind, sizeof_thread_id, thread_id, &tcontext);
  if (ret != ompd_rc_ok)
    return ret;

  int tId;

  if (kind == OMPD_THREAD_ID_CUDALOGICAL) {
    ompd_cudathread_coord_t *p = (ompd_cudathread_coord_t *)thread_id;

    // omptarget_nvptx_threadPrivateContext->topTaskDescr[p->threadIdx.x]
    TValue th = TValue(context, tcontext,
                       "omptarget_nvptx_threadPrivateContext",
                       OMPD_SEGMENT_CUDA_PTX_SHARED)
                .cast("omptarget_nvptx_ThreadPrivateContext", 1,
                      OMPD_SEGMENT_CUDA_PTX_SHARED)
                .access("topTaskDescr")
                .cast("omptarget_nvptx_TaskDescr", 1,
                      OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                .getPtrArrayElement(p->threadIdx.x)
                .dereference();

    ompd_address_t taddr={OMPD_SEGMENT_UNSPECIFIED,0};
    ret = th.getAddress(&taddr);

    if (ret != ompd_rc_ok) {
      if (taddr.address == 0 && p->threadIdx.x % 32 == 0) {
        // check for the master task/thread instead
        // The master thread should never have the threadIdx.x of zero, so
        // checking it this way should be safe

        th = TValue(context, tcontext,
                    "omptarget_nvptx_threadPrivateContext",
                    OMPD_SEGMENT_CUDA_PTX_SHARED)
            .cast("omptarget_nvptx_ThreadPrivateContext", 1,
                   OMPD_SEGMENT_CUDA_PTX_SHARED)
            .access("teamContext")
            .cast("omptarget_nvptx_TeamDescr", 0,
                  OMPD_SEGMENT_CUDA_PTX_SHARED)
            .access("levelZeroTaskDescr");

        ret = th.getAddress(&taddr);

        if (ret != ompd_rc_ok)
          return ret;
      } else {
        return ret;
      }
    }

    // omptarget_nvptx_threadPrivateContext->topTaskDescr[p->threadIdx.x]
    //                                     ->ompd_thread_info.threadIdx_x
    ret = th.cast("omptarget_nvptx_TaskDescr", 0, OMPD_SEGMENT_CUDA_PTX_SHARED)
            .access("ompd_thread_info")
            .cast("ompd_nvptx_thread_info_t", 0, OMPD_SEGMENT_CUDA_PTX_GLOBAL)
            .access("threadIdx_x")
            .castBase(ompd_type_short)
            .getValue(tId);

    if (ret != ompd_rc_ok)
      return ret;

    if (tId != p->threadIdx.x) {
        return ompd_rc_stale_handle;
    }

    // allocate both the thread handle and the cuda kernel info in one go
    ret = callbacks->alloc_memory(sizeof(ompd_thread_handle_t) +
                                  sizeof(ompd_cuda_thread_kernel_info_t),
                                  (void **)(thread_handle));
    if (ret != ompd_rc_ok)
      return ret;

    (*thread_handle)->ah = handle;
    (*thread_handle)->th = taddr;
    (*thread_handle)->cuda_kernel_info =
        (ompd_cuda_thread_kernel_info_t*)((*thread_handle) + 1);

    (*thread_handle)->cuda_kernel_info->cudaDevId = p->cudaDevId;
    (*thread_handle)->cuda_kernel_info->cudaContext = p->cudaContext;
    (*thread_handle)->cuda_kernel_info->warpSize = p->warpSize;
    (*thread_handle)->cuda_kernel_info->gridId = p->gridId;
    (*thread_handle)->cuda_kernel_info->gridDim = p->gridDim;
    (*thread_handle)->cuda_kernel_info->blockDim = p->blockDim;
  } else {
    ret = TValue(context, tcontext, "__kmp_gtid")
              .castBase("__kmp_gtid")
              .getValue(tId);
    if (ret != ompd_rc_ok)
      return ret;

    if (tId < 0) // thread is no omp worker
      return ompd_rc_unavailable;

    TValue th = TValue(context, "__kmp_threads") // __kmp_threads
                    .cast("kmp_info_t", 2)
                    .getArrayElement(tId) /*__kmp_threads[t]*/
                    .access("th");        /*__kmp_threads[t]->th*/

    ompd_address_t taddr={OMPD_SEGMENT_UNSPECIFIED,0};
    ret = th.getAddress(&taddr);
    if (ret != ompd_rc_ok)
      return ret;
    ret = callbacks->alloc_memory(sizeof(ompd_thread_handle_t),
                                   (void **)(thread_handle));
    if (ret != ompd_rc_ok)
      return ret;
    (*thread_handle)->ah = handle;
    (*thread_handle)->th = taddr;
    (*thread_handle)->cuda_kernel_info = NULL;

#ifndef NDEBUG
    if (ret != ompd_rc_ok)
      return ret;

    pthread_t oshandle;
    TBaseValue ds_handle =
        th.cast("kmp_base_info_t")
            .access("th_info") /*__kmp_threads[t]->th.th_info*/
            .cast("kmp_desc_t")
            .access("ds") /*__kmp_threads[t]->th.th_info.ds*/
            .cast("kmp_desc_base_t")
            .access("ds_thread") /*__kmp_threads[t]->th.th_info.ds.ds_thread*/
            .castBase();

    assert(ompd_rc_ok == ds_handle.getValue(oshandle) &&
           oshandle == *(pthread_t *)(thread_id) &&
           "Callback table not initialized!");
#endif
  }
  (*thread_handle)->thread_context = tcontext;
  return ret;
}

ompd_rc_t ompd_get_thread_id(
    ompd_thread_handle_t *thread_handle, /* IN: OpenMP thread handle*/
    ompd_thread_id_t kind, ompd_size_t sizeof_thread_id, void *thread_id) {
  if (kind != OMPD_THREAD_ID_PTHREAD && kind != OMPD_THREAD_ID_CUDALOGICAL)
    return ompd_rc_bad_input;
  if (!thread_handle)
    return ompd_rc_stale_handle;
  if (!thread_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = thread_handle->ah->context;
  if (!context)
    return ompd_rc_stale_handle;
  ompd_rc_t ret;

  if (kind == OMPD_THREAD_ID_CUDALOGICAL) {
    if (sizeof_thread_id != sizeof(ompd_cudathread_coord_t)) {
      return ompd_rc_bad_input;
    }
    ompd_cudathread_coord_t *cuda_thread_id =
        (ompd_cudathread_coord_t*)thread_id;
    cuda_thread_id->cudaDevId = thread_handle->cuda_kernel_info->cudaDevId;
    cuda_thread_id->cudaContext = thread_handle->cuda_kernel_info->cudaContext;
    cuda_thread_id->warpSize = thread_handle->cuda_kernel_info->warpSize;
    cuda_thread_id->gridId = thread_handle->cuda_kernel_info->gridId;

    auto threadInfo = TValue(context, thread_handle->th)
                        .cast("omptarget_nvptx_TaskDescr", 0,
                              OMPD_SEGMENT_CUDA_PTX_GLOBAL)
                        .access("ompd_thread_info")
                        .cast("ompd_nvptx_thread_info_t", 0,
                              OMPD_SEGMENT_CUDA_PTX_GLOBAL);

    ret = threadInfo.access("threadIdx_x")
                    .castBase()
                    .getValue(cuda_thread_id->threadIdx.x);

    if (ret != ompd_rc_ok)
      return ret;

    cuda_thread_id->threadIdx.y = cuda_thread_id->threadIdx.z = 0;

    ret = threadInfo.access("blockIdx_x")
                    .castBase()
                    .getValue(cuda_thread_id->blockIdx.x);

    if (ret != ompd_rc_ok)
      return ret;

    cuda_thread_id->blockIdx.y = cuda_thread_id->blockIdx.z = 0;

    cuda_thread_id->gridDim = thread_handle->cuda_kernel_info->gridDim;
    cuda_thread_id->blockDim = thread_handle->cuda_kernel_info->blockDim;

    return ompd_rc_ok;
  } else {
    ompd_size_t size;
    ret = tf.getType(context, "kmp_thread_t").getSize(&size);
    if (ret != ompd_rc_ok)
      return ret;
    if (sizeof_thread_id != size)
      return ompd_rc_bad_input;

    if (!callbacks) {
      return ompd_rc_callback_error;
    }

    ret = TValue(context, thread_handle->th) /*__kmp_threads[t]->th*/
              .cast("kmp_base_info_t")
              .access("th_info") /*__kmp_threads[t]->th.th_info*/
              .cast("kmp_desc_t")
              .access("ds") /*__kmp_threads[t]->th.th_info.ds*/
              .cast("kmp_desc_base_t")
              .access("ds_thread") /*__kmp_threads[t]->th.th_info.ds.ds_thread*/
              .cast("kmp_thread_t")
              .getRawValue(thread_id, 1);
  }
  return ret;
}

/* --- OMPT Thread State Inquiry Analogue ----------------------------------- */

ompd_rc_t ompd_get_state(
    ompd_thread_handle_t *thread_handle, /* IN: OpenMP thread handle*/
    ompd_word_t *state,                  /* OUT: State of this thread */
    ompd_wait_id_t *wait_id              /* OUT: Wait ID */
    ) {
  if (!thread_handle)
    return ompd_rc_stale_handle;
  if (!thread_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = thread_handle->ah->context;
  if (!context)
    return ompd_rc_stale_handle;
  if (!ompd_state)
    return ompd_rc_needs_state_tracking;

  if (!callbacks) {
    return ompd_rc_callback_error;
  }
  ompd_rc_t ret;

  if (thread_handle->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    if (wait_id)
      *wait_id = 0;
    ret  = TValue(context, thread_handle->th)
            .cast("omptarget_nvptx_TaskDescr", 0, OMPD_SEGMENT_CUDA_PTX_SHARED)
            .access("ompd_thread_info")
            .cast("ompd_nvptx_thread_info_t", 0, OMPD_SEGMENT_CUDA_PTX_GLOBAL)
            .access("state")
            .castBase(ompd_type_long_long)
            .getValue(*state);
  } else {
    TValue ompt_thread_info =
        TValue(context, thread_handle->th) /*__kmp_threads[t]->th*/
            .cast("kmp_base_info_t")
            .access("ompt_thread_info") /*__kmp_threads[t]->th.ompt_thread_info*/
            .cast("ompt_thread_info_t");
    if (ompt_thread_info.gotError())
      return ompt_thread_info.getError();
    ret = ompt_thread_info
            .access("state") /*__kmp_threads[t]->th.ompt_thread_info.state*/
            .castBase()
            .getValue(*state);
    if (ret != ompd_rc_ok)
      return ret;
    ret = ompt_thread_info
              .access("wait_id") /*__kmp_threads[t]->th.ompt_thread_info.state*/
              .castBase()
              .getValue(*wait_id);
  }
  return ret;
}

/* ---  Task Inquiry -------------------------------------------------------- */

/* ---  Task Settings ------------------------------------------------------- */

/* ---  OMPT Task Inquiry Analogues ----------------------------------------- */

ompd_rc_t ompd_get_task_frame(
    ompd_task_handle_t *task_handle, /* IN: OpenMP task handle*/
    ompd_frame_info_t *exit_frame,
    ompd_frame_info_t *enter_frame
    ) {
  if (!task_handle)
    return ompd_rc_stale_handle;
  if (!task_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = task_handle->ah->context;
  if (!context)
    return ompd_rc_stale_handle;
  if (!ompd_state)
    return ompd_rc_needs_state_tracking;

  if (!callbacks) {
    return ompd_rc_callback_error;
  }

  ompd_rc_t ret;

  if (task_handle->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    return ompd_rc_unsupported;
  } else {
    TValue taskInfo;
    if (task_handle->lwt.address != 0)
      taskInfo =
          TValue(context, task_handle->lwt).cast("ompt_lw_taskteam_t", 0); /*lwt*/
    else
      taskInfo = TValue(context, task_handle->th).cast("kmp_taskdata_t", 0); /*t*/
    TValue frame = taskInfo
                       .access("ompt_task_info") // td->ompt_task_info
                       .cast("ompt_task_info_t")
                       .access("frame") // td->ompd_task_info.frame
                       .cast("ompt_frame_t", 0);
    enter_frame->frame_address.segment = OMPD_SEGMENT_UNSPECIFIED;
    ret = frame
            .access("enter_frame") // td->ompt_task_info.frame.enter_frame
            .castBase()
            .getValue(enter_frame->frame_address.address);
  
    if (ret != ompd_rc_ok)
      return ret;
  
    exit_frame->frame_address.segment = OMPD_SEGMENT_UNSPECIFIED;
    ret = frame
              .access("exit_frame") // td->ompt_task_info.frame.exit_frame
              .castBase()
              .getValue(exit_frame->frame_address.address);
  }
  return ret;
}

ompd_rc_t ompd_get_task_function(
    ompd_task_handle_t *task_handle, /* IN: OpenMP task handle */
    ompd_address_t *task_addr /* OUT: first instruction in the task region */
    )
{
  if (!task_handle)
    return ompd_rc_stale_handle;
  if (!task_handle->ah)
    return ompd_rc_stale_handle;
  ompd_address_space_context_t *context = task_handle->ah->context;
  if (!context)
    return ompd_rc_stale_handle;
  if (!ompd_state)
    return ompd_rc_needs_state_tracking;
  if (!callbacks) {
    return ompd_rc_callback_error;
  }

  ompd_rc_t ret;

  if (task_handle->ah->kind == OMPD_DEVICE_KIND_CUDA) {
    task_addr->segment = OMPD_SEGMENT_UNSPECIFIED;
    ret = TValue(context, task_handle->th)
            .cast("omptarget_nvptx_TaskDescr", 0,
                  OMPD_SEGMENT_CUDA_PTX_GLOBAL)
            .access("ompd_thread_info")
            .cast("ompd_nvptx_thread_info_t", 0,
                  OMPD_SEGMENT_CUDA_PTX_GLOBAL)
            .access("task_function")
            .castBase()
            .getValue(task_addr->address);

  } else {
    task_addr->segment = OMPD_SEGMENT_UNSPECIFIED;
    TValue taskInfo;
    if(task_handle->lwt.address!=0)
      return ompd_rc_bad_input; // We need to decide what we do here.
    else
    {
      ompd_word_t val;
      ret = TValue(context, task_handle->th)
                      .cast("kmp_taskdata_t") // td
                      .access("td_flags")     // td->td_flags
                      .cast("kmp_tasking_flags_t")
                      .check("tasktype", &val); // td->td_flags.tasktype

      if (ret != ompd_rc_ok)
        return ret;
      
      if (val==1) { // tasktype: explicit = 1, implicit = 0
      
        ret = TValue(context, task_handle->th)
              .cast("kmp_taskdata_t",0)		/*t*/
              .getArrayElement(1)                   /* see kmp.h: #define KMP_TASKDATA_TO_TASK(taskdata) (kmp_task_t *)(taskdata + 1) */
              .cast("kmp_task_t",0)                 /* (kmp_task_t *) */
              .access("routine")             /*td->ompt_task_info*/
              .castBase()
              .getValue(task_addr->address);

      } else {

        ret = TValue(context, task_handle->th)
              .cast("kmp_taskdata_t") /*td*/
              .access("td_team")      /*td.td_team*/
              .cast("kmp_team_p", 1)
              .access("t")            /*td.td_team->t*/
              .cast("kmp_base_team_t", 0)
              .access("t_pkfn")       /*td.td_team->t.t_pkfn*/
              .castBase()
              .getValue(task_addr->address);

      }
    }
  }
  return ret;
}

/* ------- OMPD Version and Compatibility Information ----------------------- */

ompd_rc_t ompd_get_api_version(ompd_word_t *version) {
  *version = OMPD_VERSION;
  return ompd_rc_ok;
}

ompd_rc_t
ompd_get_version_string(const char **string /* OUT: OMPD version string */
                            ) {
  static const char version_string[] =
      "LLVM OpenMP " STR(OMPD_IMPLEMENTS_OPENMP) "." STR(
          OMPD_IMPLEMENTS_OPENMP_SUBVERSION) " Debugging Library implmenting "
                                             "TR " STR(OMPD_TR_VERSION) "" STR(OMPD_TR_SUBVERSION);
  *string = version_string;
  return ompd_rc_ok;
}

/* ------ Display Control Variables ----------------------------------------- */

ompd_rc_t ompd_get_display_control_vars(ompd_address_space_handle_t *handle,
                                        const char *const **control_vars) {
  if (!handle)
    return ompd_rc_stale_handle;
  if (!control_vars)
    return ompd_rc_bad_input;

  ompd_address_space_context_t *context = handle->context;
  if (!context)
    return ompd_rc_stale_handle;

  // runtime keeps a full dump of OMP/KMP definitions in this format
  // <var1 name>=<var1 value>\n<var2 name>=<var2 value>\n...
  ompd_address_t block_addr = {ompd_segment_none, 0};
  OMPD_GET_VALUE(context, NULL, "ompd_env_block", type_sizes.sizeof_pointer,
                 &block_addr.address);

  // query size of the block
  ompd_size_t block_size;
  OMPD_GET_VALUE(context, NULL, "ompd_env_block_size", sizeof(ompd_size_t),
                 &block_size);

  // copy raw data from the address space
  char *block;
  OMPD_CALLBACK(alloc_memory, block_size, (void **)&block);
  OMPD_CALLBACK(read_memory, context, NULL, &block_addr, block_size, block);

  // count number of items, replace new line to zero.
  int block_items = 1; // also count the last "NULL" item
  for (ompd_size_t i = 0; i < block_size; i++) {
    if (block[i] == '\n') {
      block_items++;
      block[i] = '\0';
    }
  }

  // create vector of char*
  const char **ctl_vars;
  OMPD_CALLBACK(alloc_memory, block_items * sizeof(char *),
                (void **)(&ctl_vars));
  char *pos = block;
  ctl_vars[0] = block;

  // ctl_vars[0] points to the entire block, ctl_vars[1]... points to the
  // smaller subsets of the block, and ctl_vars[block_items-2] points to the
  // last string in the block.
  for (int i = 1; i < block_items - 1; i++) {
    while (*pos++ != '\0')
      ;
    if (pos > block + block_size)
      return ompd_rc_error;
    ctl_vars[i] = pos;
  }
  // last item must be NULL
  ctl_vars[block_items - 1] = NULL;

  *control_vars = ctl_vars;

  return ompd_rc_ok;
}

ompd_rc_t ompd_rel_display_control_vars(const char *const **control_vars) {
  if (!control_vars)
    return ompd_rc_bad_input;

  char **ctl_vars = const_cast<char **>(*control_vars);

  // remove the raw block first
  OMPD_CALLBACK(free_memory, (void *)ctl_vars[0]);
  // remove the vector
  OMPD_CALLBACK(free_memory, (void *)ctl_vars);

  return ompd_rc_ok;
}

/* --- Helper functions ----------------------------------------------------- */

ompd_rc_t initTypeSizes(ompd_address_space_context_t *context) {
  static int inited = 0;
  static ompd_rc_t ret;
  if (inited)
    return ret;
  ret = callbacks->sizeof_type(context, &type_sizes);
  if (ret != ompd_rc_ok)
    return ret;
  if (!(type_sizes.sizeof_pointer > 0))
    return ompd_rc_error;
  ret = callbacks->sizeof_type(context, &TValue::type_sizes);
  if (ret != ompd_rc_ok)
    return ret;
  inited = 1;
  return ret;
}
