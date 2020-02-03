#include <asm/unistd.h>

#include <kourt/runner/read_size_shrink_interceptor.h>
#include <kourt/runner/tracing.h>

#include <iostream>

ReadSizeShrinkInterceptor::ReadSizeShrinkInterceptor() {
  ResetSizeRestoreNecessity();
}

bool ReadSizeShrinkInterceptor::Intercept(BeforeSyscallStoppedTracee &tracee) {
  TRACE("Before syscall %l", tracee.SyscallNumber())
  if (__NR_read == tracee.SyscallNumber()) {
    auto size = tracee.Arg3();
    if (size > 1) {
      should_restore_size_ = true;
      size_to_restore_ = size;
      auto size_to_read = 1UL;
      DEBUG("change third syscall argument from %zu to %zu", size, size_to_read);
      tracee.SetArg3(size_to_read);
    }
  }
  return false;
}

bool ReadSizeShrinkInterceptor::Intercept(AfterSyscallStoppedTracee &tracee) {
  if (should_restore_size_) {
    DEBUG("change third syscall argument to %zu after syscall", size_to_restore_);
    tracee.SetArg3(size_to_restore_);
  }
  ResetSizeRestoreNecessity();
  return false;
}

void ReadSizeShrinkInterceptor::ResetSizeRestoreNecessity() {
  should_restore_size_ = false;
  size_to_restore_ = 0;
}
