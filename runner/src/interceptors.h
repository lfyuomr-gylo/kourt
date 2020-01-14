#ifndef RUNNER_SRC_INTERCEPTORS_H_
#define RUNNER_SRC_INTERCEPTORS_H_

#include <string>
#include <unordered_map>

#include "tracing.h"

class TraceeInterceptor {
 public:
  /// @return whether additional interceptors may be applied to the tracee
  ///         or this interceptor has continued tracee execution.
  ///         In the latter case, execution controller should await the next Tracee stop.
  virtual bool Intercept(StoppedTracee *a_tracee) {
    if (auto tracee = dynamic_cast<BeforeSyscallStoppedTracee *>(a_tracee); tracee) {
      return Intercept(*tracee);
    }
    if (auto tracee = dynamic_cast<AfterSyscallStoppedTracee *>(a_tracee); tracee) {
      return Intercept(*tracee);
    }
    if (auto tracee = dynamic_cast<BeforeSignalDeliveryStoppedTracee *>(a_tracee); tracee) {
      return Intercept(*tracee);
    }
    if (auto tracee = dynamic_cast<OnGroupStopStoppedTracee *>(a_tracee); tracee) {
      return Intercept(*tracee);
    }
    if (auto tracee = dynamic_cast<BeforeTerminationStoppedTracee *>(a_tracee); tracee) {
      return Intercept(*tracee);
    }
    WARN("Unknown StoppedTracee type: %s", typeid(*a_tracee).name());
    return true;
  }

 protected:
  virtual bool Intercept(BeforeSyscallStoppedTracee &tracee) {
    return true;
  }
  virtual bool Intercept(AfterSyscallStoppedTracee &tracee) {
    return true;
  }
  virtual bool Intercept(BeforeSignalDeliveryStoppedTracee &tracee) {
    return true;
  }
  virtual bool Intercept(OnGroupStopStoppedTracee &tracee) {
    return true;
  }
  virtual bool Intercept(BeforeTerminationStoppedTracee &tracee) {
    return true;
  }
};

TraceeInterceptor *CreateInterceptor(const std::string &interceptor_name, Tracee &tracee);

#endif //RUNNER_SRC_INTERCEPTORS_H_
