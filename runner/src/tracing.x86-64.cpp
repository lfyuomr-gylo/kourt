#include <sys/user.h>

#include "tracee_controller.h"

unsigned long SyscallStoppedTracee::SyscallNumber() {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  return registers.orig_rax;
}

void SyscallStoppedTracee::SetSyscallNumber(unsigned long syscall_no) {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  registers.rax = syscall_no;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &registers);
}

unsigned long SyscallStoppedTracee::Arg1() {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  return registers.rdi;
}

void SyscallStoppedTracee::SetArg1(unsigned long arg) {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  registers.rdi = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &registers);
}

unsigned long SyscallStoppedTracee::Arg2() {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  return registers.rdi;
}

void SyscallStoppedTracee::SetArg2(unsigned long arg) {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  registers.rdi = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &registers);
}

unsigned long SyscallStoppedTracee::Arg3() {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  return registers.rdx;
}

void SyscallStoppedTracee::SetArg3(unsigned long arg) {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  registers.rdx = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &registers);
}

unsigned long SyscallStoppedTracee::Arg4() {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  return registers.r10;
}

void SyscallStoppedTracee::SetArg4(unsigned long arg) {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  registers.r10 = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &registers);
}

unsigned long SyscallStoppedTracee::Arg5() {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  return registers.r8;
}

void SyscallStoppedTracee::SetArg5(unsigned long arg) {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  registers.r8 = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &registers);
}

void SyscallStoppedTracee::SetArg6(unsigned long arg) {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  registers.r9 = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &registers);
}

unsigned long SyscallStoppedTracee::Arg6() {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  return registers.r9;
}

long AfterSyscallStoppedTracee::ReturnedValue() {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  return registers.rax;
}

void AfterSyscallStoppedTracee::SetReturnedValue(long returned_value) {
  user_regs_struct registers{};
  tracee_.Ptrace(PTRACE_GETREGS, nullptr, &registers);
  registers.rax = returned_value;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &registers);
}
