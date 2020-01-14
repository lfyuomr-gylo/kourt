#ifndef RUNNER_SRC_TRACEE_CONTROLLER_H_
#define RUNNER_SRC_TRACEE_CONTROLLER_H_

#include "tracee.h"

enum class TraceeStopMoment {
  kBeforeSyscallEnter,
  kBeforeSyscallExit,
  kBeforeSignalDelivery,
  kGroupStop,
  /**
   *   Enabled through PTRACE_O_TRACEEXIT option.
   */
      kBeforeTraceeDeath
};

class TraceeController {
 public:
  explicit TraceeController(Tracee &tracee);

 private:
  TraceeStopMoment DetermineStopMoment(int wait_status);
  TraceeStopMoment SyscallStop();
  TraceeStopMoment SignalDeliveryStop();
  TraceeStopMoment GroupStop();
  TraceeStopMoment ExitStop();

  bool entered_syscall_;
  Tracee &tracee_;
};

#endif //RUNNER_SRC_TRACEE_CONTROLLER_H_
