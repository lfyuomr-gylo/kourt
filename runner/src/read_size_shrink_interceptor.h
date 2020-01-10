#ifndef RUNNER_SRC_READ_SIZE_SHRINK_INTERCEPTOR_H_
#define RUNNER_SRC_READ_SIZE_SHRINK_INTERCEPTOR_H_

#include <sys/types.h>

#include "interceptors.h"

class ReadSizeShrinkInterceptor : public SyscallInterceptor {
 public:
  explicit ReadSizeShrinkInterceptor(Tracee &tracee);

  bool BeforeSyscallEntered() override;
  void AfterSyscallReturned() override;

 private:
  void ResetSizeRestoreNecessity();
  bool should_restore_size_;
  unsigned long size_to_restore_;
};

#endif //RUNNER_SRC_READ_SIZE_SHRINK_INTERCEPTOR_H_
