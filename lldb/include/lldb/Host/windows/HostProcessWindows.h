//===-- HostProcessWindows.h ------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef lldb_Host_HostProcessWindows_h_
#define lldb_Host_HostProcessWindows_h_

#include "lldb/Host/HostNativeProcessBase.h"
#include "lldb/lldb-types.h"

namespace lldb_private {

class FileSpec;

class HostProcessWindows : public HostNativeProcessBase {
public:
  HostProcessWindows();
  explicit HostProcessWindows(lldb::process_t process);
  ~HostProcessWindows();

  void SetOwnsHandle(bool owns);

  Status Terminate() override;
  Status GetMainModule(FileSpec &file_spec) const override;

  lldb::pid_t GetProcessId() const override;
  bool IsRunning() const override;

  HostThread StartMonitoring(const Host::MonitorChildProcessCallback &callback,
                             bool monitor_signals) override;

private:
  static lldb::thread_result_t MonitorThread(void *thread_arg);

  void Close();

  bool m_owns_handle;
};
}

#endif
