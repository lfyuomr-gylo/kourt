#include <asm/unistd.h>

#include "read_size_shrink_interceptor.h"
#include "tracing.h"

#include <iostream>

ReadSizeShrinkInterceptor::ReadSizeShrinkInterceptor() {
  ResetSizeRestoreNecessity();
}

bool ReadSizeShrinkInterceptor::Intercept(BeforeSyscallStoppedTracee &tracee) {
  std::cout << "Before syscall " << tracee.SyscallNumber() << std::endl;
  if (__NR_read == tracee.SyscallNumber()) {
    auto size = tracee.Arg3();
    if (size > 1) {
      should_restore_size_ = true;
      size_to_restore_ = size;
      tracee.SetArg3(size - 1);
    }
  }
  return false;
}

bool ReadSizeShrinkInterceptor::Intercept(AfterSyscallStoppedTracee &tracee) {
  if (should_restore_size_) {
    tracee.SetArg3(size_to_restore_);
  }
  ResetSizeRestoreNecessity();
}

void ReadSizeShrinkInterceptor::ResetSizeRestoreNecessity() {
  should_restore_size_ = false;
  size_to_restore_ = 0;
}
