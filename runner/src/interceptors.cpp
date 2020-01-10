#include <exception>
#include <stdexcept>

#include "interceptors.h"
#include "read_size_shrink_interceptor.h"

SyscallInterceptor *CreateInterceptor(const std::string &interceptor_name, Tracee &tracee) {
  if ("ReadSizeShrinkInterceptor" == interceptor_name) {
    return new ReadSizeShrinkInterceptor(tracee);
  } else {
    throw std::invalid_argument("Unknown interceptor name: '" + interceptor_name + "'");
  }
}