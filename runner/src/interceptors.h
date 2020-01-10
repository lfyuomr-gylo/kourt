#ifndef RUNNER_SRC_INTERCEPTORS_H_
#define RUNNER_SRC_INTERCEPTORS_H_

#include <string>

#include "tracee.h"

class SyscallInterceptor {
 public:
  /**
   * \return whether AfterSyscallReturned should be called or not for this syscall.
   */
  virtual bool BeforeSyscallEntered() = 0;
  virtual void AfterSyscallReturned() = 0;

 protected:
 public:
  explicit SyscallInterceptor(Tracee &tracee) :
      tracee_(tracee) {
    // nop
  }
 protected:

  Tracee &tracee_;
};

SyscallInterceptor *CreateInterceptor(const std::string &interceptor_name, Tracee &tracee);

class SyscallRegistersManipulator {
 public:
  explicit SyscallRegistersManipulator(Tracee &tracee) :
      tracee_(tracee),
      regs_{} {
    tracee_.Ptrace(PTRACE_GETREGS, nullptr, &regs_);
  }

  unsigned long SyscallNo();
  void SetSyscallNo(unsigned long syscall_no);

  long ReturnedValue();
  void SetReturnedValue(long returned_value);

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

 private:
  Tracee &tracee_;
  struct user_regs_struct regs_;
};

#endif //RUNNER_SRC_INTERCEPTORS_H_
