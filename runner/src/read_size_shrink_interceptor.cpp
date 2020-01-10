#include <asm/unistd.h>

#include "read_size_shrink_interceptor.h"
#include "tracee.h"

#include <iostream>

ReadSizeShrinkInterceptor::ReadSizeShrinkInterceptor(Tracee &tracee) :
    SyscallInterceptor(tracee) {
  ResetSizeRestoreNecessity();
}
bool ReadSizeShrinkInterceptor::BeforeSyscallEntered() {
  auto regs = SyscallRegistersManipulator(tracee_);
  std::cout << "Before Syscall " << regs.SyscallNo() << std::endl;
  if (__NR_read == regs.SyscallNo()) {
    auto size = regs.Arg3();
    if (size > 1) {
      should_restore_size_ = true;
      size_to_restore_ = size;
      regs.SetArg3(size - 1);
    }
  }
  return should_restore_size_;
}

void ReadSizeShrinkInterceptor::AfterSyscallReturned() {
  auto regs = SyscallRegistersManipulator(tracee_);
  if (should_restore_size_) {
    regs.SetArg3(size_to_restore_);
  }
  ResetSizeRestoreNecessity();
}

void ReadSizeShrinkInterceptor::ResetSizeRestoreNecessity() {
  should_restore_size_ = false;
  size_to_restore_ = 0;
}


