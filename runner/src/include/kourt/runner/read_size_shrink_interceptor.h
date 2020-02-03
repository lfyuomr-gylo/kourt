#ifndef RUNNER_SRC_READ_SIZE_SHRINK_INTERCEPTOR_H_
#define RUNNER_SRC_READ_SIZE_SHRINK_INTERCEPTOR_H_

#include <sys/types.h>

#include "interceptors.h"

class ReadSizeShrinkInterceptor : public virtual NoOpStoppedTraceeInterceptor {
 public:
  ReadSizeShrinkInterceptor();

 protected:
  bool Intercept(BeforeSyscallStoppedTracee &tracee) override;
  bool Intercept(AfterSyscallStoppedTracee &tracee) override;

 private:
  void ResetSizeRestoreNecessity();

  bool should_restore_size_{false};
  size_t size_to_restore_{0};
};

#endif //RUNNER_SRC_READ_SIZE_SHRINK_INTERCEPTOR_H_
