
#ifndef RUNNER_SRC_INCLUDE_KOURT_RUNNER_DEFER_H_
#define RUNNER_SRC_INCLUDE_KOURT_RUNNER_DEFER_H_

#include <functional>

class DeferredAction {
 public:
  explicit DeferredAction(std::function<void()> destructor_body) noexcept :
      action_(std::move(destructor_body)) {
    // nop
  }
  DeferredAction(const DeferredAction &that) = delete;
  DeferredAction(DeferredAction &&other) noexcept {
    action_ = other.action_;
    other.action_ = []() { /* nop */ };
  }

  ~DeferredAction() {
    action_();
  }
 private:
  std::function<void()> action_;
};

DeferredAction &&DeferCloseFileDescriptor(int fd) {
  return std::move(DeferredAction([=]() { close(fd); }));
}

#endif //RUNNER_SRC_INCLUDE_KOURT_RUNNER_DEFER_H_
