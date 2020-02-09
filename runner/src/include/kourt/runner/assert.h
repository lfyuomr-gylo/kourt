
#ifndef RUNNER_SRC_INCLUDE_KOURT_RUNNER_ASSERT_H_
#define RUNNER_SRC_INCLUDE_KOURT_RUNNER_ASSERT_H_

#include <stdexcept>
#include <string>

#ifndef ASSERT
#define ASSERT(condition, message) \
    if (condition) { \
      throw std::runtime_error(__FILE__ + std::to_string(__LINE__) + ": " + message); \
    }
#else
#error ASSERT macro is already defined
#endif // ASSERT

#endif //RUNNER_SRC_INCLUDE_KOURT_RUNNER_ASSERT_H_
