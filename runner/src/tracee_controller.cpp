#include "tracee_controller.h"

TraceeController::TraceeController(Tracee &tracee) :
    tracee_(tracee),
    entered_syscall_(false) {
  auto options = PTRACE_O_TRACEEXIT | PTRACE_O_TRACESYSGOOD;
  tracee_.Ptrace(PTRACE_SETOPTIONS, nullptr, (void *) options);
}

static bool IsPtraceEventStop(const int wait_status, const __ptrace_eventcodes event_code) {
  return wait_status >> 8 == (SIGTRAP | (event_code << 8));
}

static bool IsNotGroupStopSignal(const int signal_no) {
  return signal_no != SIGSTOP && signal_no != SIGTSTP && signal_no != SIGTTIN && signal_no != SIGTTOU;
}

TraceeStopMoment TraceeController::DetermineStopMoment(int wait_status) {
  const int signal_no = WSTOPSIG(wait_status);
  if (signal_no == (SIGTRAP | 0x80)) {
    return SyscallStop();
  } else if (signal_no != SIGTRAP) {
    // either signal-delivery-stop or group-stop
    if (IsNotGroupStopSignal(signal_no)) {
      return SignalDeliveryStop();
    } else {
      try {
        siginfo_t siginfo;
        tracee_.Ptrace(PTRACE_GETSIGINFO, nullptr, &siginfo);
        // if ptrace succeeded, it's signal delivery stop
        return SignalDeliveryStop();
      } catch (PtraceCallFailed &exc) {
        if (exc.Errno() == EINVAL) {
          return GroupStop();
        } else {
          throw;
        }
      }
    }
  } else /* signal_no == SIGTRAP */ {
    if (IsPtraceEventStop(wait_status, PTRACE_EVENT_EXEC)) {
      return ExitStop();
    } else {
      // either SIGTRAP signal-delivery-stop or syscall-stop.
      siginfo_t signal_info;
      tracee_.Ptrace(PTRACE_GETSIGINFO, nullptr, &signal_info);
      if (signal_info.si_code <= 0 || signal_info.si_code == SI_KERNEL) {
        // tracee received SIGTRAP
        return SignalDeliveryStop();
      } else if (signal_info.si_code == SIGTRAP || signal_info.si_code == (SIGTRAP | 0x80)) {
        // Syscall stops are expected to be detected through PTRACE_O_TRACESYSGOOD option,
        // so execution flow would not enter this clause.
        // However, PTRACE_O_TRACESYSGOOD is not guaranteed to work on all platforms,
        // thus we have to support PTRACE_GETSIGINFO-based syscall stop detection.
        return SyscallStop();
      } else {
        throw std::runtime_error(std::string("Unexpected si_code for SIGTRAP: " + std::to_string(signal_info.si_code)));
      }
    }
  }
}

TraceeStopMoment TraceeController::SyscallStop() {
  TraceeStopMoment stop_moment = entered_syscall_
      ? TraceeStopMoment::kBeforeSyscallEnter
      : TraceeStopMoment::kBeforeSyscallExit;
  entered_syscall_ = !entered_syscall_;
  return stop_moment;
}

TraceeStopMoment TraceeController::SignalDeliveryStop() {
  return TraceeStopMoment::kBeforeSignalDelivery;
}

TraceeStopMoment TraceeController::GroupStop() {
  return TraceeStopMoment::kGroupStop;
}

TraceeStopMoment TraceeController::ExitStop() {
  return TraceeStopMoment::kBeforeTraceeDeath;
}
