#ifndef RUNNER_SRC_TRACEE_CONTROLLER_H_
#define RUNNER_SRC_TRACEE_CONTROLLER_H_

#include <vector>

#include "tracing.h"
#include "interceptors.h"

class TraceeController {
 public:
  // TODO: use smart pointers here
  TraceeController(Tracee &tracee, const std::vector<StoppedTraceeInterceptor *> &interceptors);

  /**
   * @return status of the tracee as returned by <code>waitpid</code>
   */
  int ExecuteTracee();
 private:
  StoppedTracee *DetermineStopMoment(int wait_status);
  StoppedTracee *SyscallStop();
  StoppedTracee *SignalDeliveryStop(int signal_number);
  StoppedTracee *GroupStop();
  StoppedTracee *ExitStop();

  std::vector<StoppedTraceeInterceptor *> interceptors_;
  bool entered_syscall_;
  Tracee &tracee_;
};

#endif //RUNNER_SRC_TRACEE_CONTROLLER_H_
