#include <exception>
#include <stdexcept>
#include <memory>

#include <kourt/runner/interceptors.h>
#include <kourt/runner/read_size_shrink_interceptor.h>

std::unique_ptr<StoppedTraceeInterceptor> CreateInterceptor(const std::string &interceptor_name, Tracee &tracee) {
  if ("ReadSizeShrinkInterceptor" == interceptor_name) {
    return std::unique_ptr<StoppedTraceeInterceptor>(new ReadSizeShrinkInterceptor());
  } else {
    throw std::invalid_argument("Unknown interceptor name: '" + interceptor_name + "'");
  }
}
