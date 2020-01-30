#ifndef RUNNER_SRC_TRACEE_CONTROLLER_H_
#define RUNNER_SRC_TRACEE_CONTROLLER_H_

#include <vector>
#include <memory>

#include "tracing.h"
#include "interceptors.h"

class TraceeController {
 public:
  // TODO: use smart pointers here
  TraceeController(Tracee &tracee, std::vector<std::unique_ptr<StoppedTraceeInterceptor>> &&interceptors);

  /**
   * @return status of the tracee as returned by <code>waitpid</code>
   */
  int ExecuteTracee();
 private:
  std::unique_ptr<StoppedTracee> DetermineStopMoment(int wait_status);
  std::unique_ptr<StoppedTracee> SyscallStop();
  std::unique_ptr<StoppedTracee> SignalDeliveryStop(int signal_number);
  std::unique_ptr<StoppedTracee> GroupStop();
  std::unique_ptr<StoppedTracee> ExitStop();

  std::vector<std::unique_ptr<StoppedTraceeInterceptor>> interceptors_;
  bool entered_syscall_;
  Tracee &tracee_;
};

#endif //RUNNER_SRC_TRACEE_CONTROLLER_H_
