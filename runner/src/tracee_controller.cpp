#include <utility>

#include "tracee_controller.h"
#include "tracing.h"

TraceeController::TraceeController(Tracee &tracee, std::vector<TraceeInterceptor *> interceptors) :
    interceptors_(std::move(interceptors)),
    tracee_(tracee),
    entered_syscall_(false) {
  // nop
}

int TraceeController::ExecuteTracee() {
  int wait_status = tracee_.Wait(); // catch initial SIGTRAP sent to tracee on exec call.
  auto options = PTRACE_O_TRACEEXIT | PTRACE_O_TRACESYSGOOD;
  tracee_.Ptrace(PTRACE_SETOPTIONS, nullptr, (void *) options);
  tracee_.Ptrace(PTRACE_SYSCALL, nullptr, nullptr);

  for (bool keep_tracing = true; keep_tracing;) {
    wait_status = tracee_.Wait();
    keep_tracing = WIFSTOPPED(wait_status);
    if (keep_tracing) {
      auto stopped_tracee = DetermineStopMoment(wait_status);
      bool tracee_restarted = false;
      for (auto interceptor : interceptors_) {
        tracee_restarted = interceptor->Intercept(stopped_tracee);
        if (!tracee_restarted) {
          tracee_restarted = true;
        }
      }
      if (!tracee_restarted) {
        stopped_tracee->ContinueExecution();
      }
      delete stopped_tracee;
    }
  }
  return wait_status;
}

static bool IsPtraceEventStop(const int wait_status, const __ptrace_eventcodes event_code) {
  return wait_status >> 8 == (SIGTRAP | (event_code << 8));
}

static bool IsNotGroupStopSignal(const int signal_no) {
  return signal_no != SIGSTOP && signal_no != SIGTSTP && signal_no != SIGTTIN && signal_no != SIGTTOU;
}

StoppedTracee *TraceeController::DetermineStopMoment(int wait_status) {
  const int signal_number = WSTOPSIG(wait_status);
  if (signal_number == (SIGTRAP | 0x80)) {
    return SyscallStop();
  } else if (signal_number != SIGTRAP) {
    // either signal-delivery-stop or group-stop
    if (IsNotGroupStopSignal(signal_number)) {
      return SignalDeliveryStop(0);
    } else {
      try {
        siginfo_t siginfo;
        tracee_.Ptrace(PTRACE_GETSIGINFO, nullptr, &siginfo);
        // if ptrace succeeded, it's signal delivery stop
        return SignalDeliveryStop(0);
      } catch (PtraceCallFailed &exc) {
        if (exc.Errno() == EINVAL) {
          return GroupStop();
        } else {
          throw;
        }
      }
    }
  } else /* signal_number == SIGTRAP */ {
    if (IsPtraceEventStop(wait_status, PTRACE_EVENT_EXIT)) {
      return ExitStop();
    } else {
      // either SIGTRAP signal-delivery-stop or syscall-stop.
      siginfo_t signal_info;
      tracee_.Ptrace(PTRACE_GETSIGINFO, nullptr, &signal_info);
      if (signal_info.si_code <= 0 || signal_info.si_code == SI_KERNEL) {
        // tracee received SIGTRAP
        return SignalDeliveryStop(0);
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

StoppedTracee *TraceeController::SyscallStop() {
  auto stopped_tracee = entered_syscall_
      ? (StoppedTracee *) new AfterSyscallStoppedTracee(tracee_)
      : (StoppedTracee *) new BeforeSyscallStoppedTracee(tracee_);
  entered_syscall_ = !entered_syscall_;
  return stopped_tracee;
}

StoppedTracee *TraceeController::SignalDeliveryStop(int signal_number) {
  return new BeforeSignalDeliveryStoppedTracee(tracee_, signal_number);
}

StoppedTracee *TraceeController::GroupStop() {
  return new OnGroupStopStoppedTracee(tracee_);
}

StoppedTracee *TraceeController::ExitStop() {
  return new BeforeTerminationStoppedTracee(tracee_);
}
