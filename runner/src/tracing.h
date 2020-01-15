#ifndef RUNNER_SRC_TRACING_H_
#define RUNNER_SRC_TRACING_H_

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <cerrno>

#include <stdexcept>
#include <cstring>
#include <utility>

#include "logging.h"

class PtraceCallFailed : public std::exception {
 public:
  PtraceCallFailed(int error_code, std::string error_description) :
      errno_(error_code),
      error_description_(std::move(error_description)) {
    // nop
  }

  [[nodiscard]] int Errno() const {
    return errno_;
  }

  [[nodiscard]] const char *what() const noexcept override {
    return error_description_.c_str();
  }

 private:
  std::string error_description_;
  int errno_;
};

class TraceeDead : public PtraceCallFailed {
 public:
  explicit TraceeDead(std::string error_description) :
      PtraceCallFailed(ESRCH, std::move(error_description)) {
    // nop
  }
};

class Tracee {
 public:

  explicit Tracee(pid_t tracee_pid) :
      tracee_pid_(tracee_pid) {
    // nop
  }

  long Ptrace(__ptrace_request request, void *addr, void *data) {
    errno = 0;
    long result = ptrace(request, tracee_pid_, addr, data);
    auto error = errno;
    TRACE("ptrace(req=%d, pid=%d, addr=%p, data=%p) returned %ld", request, tracee_pid_, addr, data, result);
    if (result == -1 && error != 0) {
      char ptrace_call[1024];
      snprintf(ptrace_call,
               sizeof(ptrace_call),
               "ptrace(req=%d,pid=%d,addr=%p,data=%p): ",
               request,
               tracee_pid_,
               addr,
               data
      );
      switch (error) {
        case EBUSY:
          throw PtraceCallFailed(
              error,
              std::string(ptrace_call) + "an error with allocating or freeing a debug register occurred"
          );
        case EFAULT:
        case EIO:
          throw PtraceCallFailed(
              error,
              std::string(ptrace_call) + "attempt to read or write from/to invalid memory area " +
                  "or invalid signal was specified for restart request"
          );
        case EINVAL: {
          throw PtraceCallFailed(error, std::string(ptrace_call) + "invalid option");
        }
        case EPERM: {
          throw PtraceCallFailed(error, std::string(ptrace_call) + "Tracee is not allowed to be traced.");
        }
        default: {
          throw PtraceCallFailed(error, std::string(ptrace_call) + strerror(error));
        }
      }
    } else {
      return result;
    }
  }

  int Wait() {
    int wait_status;
    int options = 0;
    pid_t pid;
    do {
      pid = waitpid(tracee_pid_, &wait_status, options);
    } while (-1 == pid && EINTR == errno);

    if (-1 == pid) {
      int error = errno;
      switch (error) {
        case EINVAL: throw std::invalid_argument("invalid options " + std::to_string(options) + " passed to waitpid");
        case ECHILD: throw TraceeDead("waitpid failed with ECHILD");
        default: throw std::runtime_error(std::string("waitpid: ") + strerror(error));
      }
    } else {
      TRACE("waitpid(pid=%d, options=%d) returned wait status %d", tracee_pid_, options, wait_status);
      // TODO: возвращать удобный wrapper object
      return wait_status;
    }
  }

 private:

  pid_t tracee_pid_;
};

class StoppedTraceeInterceptor;

class StoppedTracee {
 public:
  explicit StoppedTracee(Tracee &tracee) :
      tracee_(tracee) {
    // nop
  }
  virtual ~StoppedTracee() = default;

  virtual void ContinueExecution() {
    tracee_.Ptrace(PTRACE_SYSCALL, nullptr, nullptr);
  }

  /// @return whether this interceptor has restarted the tracee or not. If this method returns true,
  ///         controller should not consider this tracee stopped anymore, but it should wait for the next stop.
  virtual bool Intercept(StoppedTraceeInterceptor &visitor) = 0;

 protected:
  Tracee &tracee_;
};

class SyscallStoppedTracee : public StoppedTracee {
 public:
  using StoppedTracee::StoppedTracee;
  ~SyscallStoppedTracee() override = default;

  unsigned long SyscallNumber();
  void SetSyscallNumber(unsigned long syscall_no);
  unsigned long Arg1();
  void SetArg1(unsigned long arg);
  unsigned long Arg2();
  void SetArg2(unsigned long arg);
  unsigned long Arg3();
  void SetArg3(unsigned long arg);
  unsigned long Arg4();
  void SetArg4(unsigned long arg);
  unsigned long Arg5();
  void SetArg5(unsigned long arg);
  unsigned long Arg6();
  void SetArg6(unsigned long arg);
};

class BeforeSyscallStoppedTracee : public SyscallStoppedTracee {
 public:
  using SyscallStoppedTracee::SyscallStoppedTracee;

  bool Intercept(StoppedTraceeInterceptor &visitor) override;

  // TODO: add 'EnterSyscall'
};

class AfterSyscallStoppedTracee : public SyscallStoppedTracee {
 public:
  using SyscallStoppedTracee::SyscallStoppedTracee;

  bool Intercept(StoppedTraceeInterceptor &visitor) override;

  long ReturnedValue();
  void SetReturnedValue(long returned_value);

  // TODO: add 'ReturnValue', 'SetReturnValue'
};

class BeforeSignalDeliveryStoppedTracee : public StoppedTracee {
 public:
  BeforeSignalDeliveryStoppedTracee(Tracee &tracee, int signal_number) :
      StoppedTracee(tracee),
      signal_number_(signal_number) {
    // nop
  }

  bool Intercept(StoppedTraceeInterceptor &visitor) override;

  void ContinueExecution() override {
    tracee_.Ptrace(PTRACE_SYSCALL, nullptr, (void *) signal_number_);
  }

  int SignalNumber() {
    return signal_number_;
  }
  // TODO: add 'SetSignalNumber', 'SuppressSignal'
 private:
  int signal_number_;
};

class OnGroupStopStoppedTracee : public StoppedTracee {
 public:
  using StoppedTracee::StoppedTracee;

  bool Intercept(StoppedTraceeInterceptor &visitor) override;
};

class BeforeTerminationStoppedTracee : public StoppedTracee {
 public:
  using StoppedTracee::StoppedTracee;

  bool Intercept(StoppedTraceeInterceptor &visitor) override;

  void ContinueExecution() override {
    tracee_.Ptrace(PTRACE_CONT, nullptr, nullptr);
  }
};

class StoppedTraceeInterceptor {
 public:
  /// @return whether this interceptor has restarted the tracee or not. If this method returns true,
  ///         controller should not consider this tracee stopped anymore, but it should wait for the next stop.
  virtual bool Intercept(BeforeSyscallStoppedTracee &stopped_tracee) = 0;

  /// @return whether this interceptor has restarted the tracee or not. If this method returns true,
  ///         controller should not consider this tracee stopped anymore, but it should wait for the next stop.
  virtual bool Intercept(AfterSyscallStoppedTracee &stopped_tracee) = 0;

  /// @return whether this interceptor has restarted the tracee or not. If this method returns true,
  ///         controller should not consider this tracee stopped anymore, but it should wait for the next stop.
  virtual bool Intercept(BeforeSignalDeliveryStoppedTracee &stopped_tracee) = 0;

  /// @return whether this interceptor has restarted the tracee or not. If this method returns true,
  ///         controller should not consider this tracee stopped anymore, but it should wait for the next stop.
  virtual bool Intercept(OnGroupStopStoppedTracee &stopped_tracee) = 0;

  /// @return whether this interceptor has restarted the tracee or not. If this method returns true,
  ///         controller should not consider this tracee stopped anymore, but it should wait for the next stop.
  virtual bool Intercept(BeforeTerminationStoppedTracee &stopped_tracee) = 0;
};

#endif //RUNNER_SRC_TRACING_H_
