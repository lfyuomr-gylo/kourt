#include "interceptors.h"

unsigned long SyscallRegistersManipulator::SyscallNo() {
  return regs_.orig_rax;
}
void SyscallRegistersManipulator::SetSyscallNo(unsigned long syscall_no) {
  regs_.rax = syscall_no;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &regs_);
}
long SyscallRegistersManipulator::ReturnedValue() {
  return regs_.rax;
}
void SyscallRegistersManipulator::SetReturnedValue(long returned_value) {
  regs_.rax = returned_value;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &regs_);
}
unsigned long SyscallRegistersManipulator::Arg1() {
  return regs_.rdi;
}
void SyscallRegistersManipulator::SetArg1(unsigned long arg) {
  regs_.rdi = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &regs_);
}
unsigned long SyscallRegistersManipulator::Arg2() {
  return regs_.rdi;
}
void SyscallRegistersManipulator::SetArg2(unsigned long arg) {
  regs_.rdi = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &regs_);
}
unsigned long SyscallRegistersManipulator::Arg3() {
  return regs_.rdx;
}
void SyscallRegistersManipulator::SetArg3(unsigned long arg) {
  regs_.rdx = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &regs_);
}
unsigned long SyscallRegistersManipulator::Arg4() {
  return regs_.r10;
}
void SyscallRegistersManipulator::SetArg4(unsigned long arg) {
  regs_.r10 = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &regs_);
}
unsigned long SyscallRegistersManipulator::Arg5() {
  return regs_.r8;
}
void SyscallRegistersManipulator::SetArg5(unsigned long arg) {
  regs_.r8 = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &regs_);
}
void SyscallRegistersManipulator::SetArg6(unsigned long arg) {
  regs_.r9 = arg;
  tracee_.Ptrace(PTRACE_SETREGS, nullptr, &regs_);
}
unsigned long SyscallRegistersManipulator::Arg6() {
  return regs_.r9;
}
