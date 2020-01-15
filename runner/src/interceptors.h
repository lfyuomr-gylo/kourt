#ifndef RUNNER_SRC_INTERCEPTORS_H_
#define RUNNER_SRC_INTERCEPTORS_H_

#include <string>
#include <unordered_map>

#include "tracing.h"

class NoOpStoppedTraceeInterceptor : public virtual StoppedTraceeInterceptor {
 public:
  bool Intercept(BeforeSyscallStoppedTracee &tracee) override {
    return false;
  }
  bool Intercept(AfterSyscallStoppedTracee &tracee) override {
    return false;
  }
  bool Intercept(BeforeSignalDeliveryStoppedTracee &tracee) override {
    return false;
  }
  bool Intercept(OnGroupStopStoppedTracee &tracee) override {
    return false;
  }
  bool Intercept(BeforeTerminationStoppedTracee &tracee) override {
    return false;
  }
};

StoppedTraceeInterceptor *CreateInterceptor(const std::string &interceptor_name, Tracee &tracee);

#endif //RUNNER_SRC_INTERCEPTORS_H_
