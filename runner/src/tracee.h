#ifndef RUNNER_SRC_TRACEE_H_
#define RUNNER_SRC_TRACEE_H_

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <cerrno>

#include <stdexcept>
#include <cstring>

#include "logging.h"

class TraceeDead : public std::exception {
 public:
  explicit TraceeDead(std::string error_description) :
      error_description_(std::move(error_description)) {
    // nop
  }
 private:
  std::string error_description_;
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
          throw std::runtime_error(std::string(ptrace_call) +
              "an error with allocating or freeing a debug register occurred"
          );
        case EFAULT:
        case EIO:
          throw std::invalid_argument(std::string(ptrace_call) +
              "attempt to read or write from/to invalid memory area or invalid signal was specified for restart request"
          );
        case EINVAL: {
          throw std::invalid_argument(std::string(ptrace_call) + "invalid option");
        }
        case EPERM: {
          throw std::runtime_error(std::string(ptrace_call) + "Tracee is not allowed to be traced.");
        }
        case ESRCH: {
          throw TraceeDead(ptrace_call);
        }
        default: {
          throw std::runtime_error(std::string(ptrace_call) + strerror(error));
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

#endif //RUNNER_SRC_TRACEE_H_
