#include <kourt/runner/tracing.h>

bool AfterSyscallStoppedTracee::Intercept(StoppedTraceeInterceptor &visitor) {
  return visitor.Intercept(*this);
}
bool BeforeSyscallStoppedTracee::Intercept(StoppedTraceeInterceptor &visitor) {
  return visitor.Intercept(*this);
}
bool BeforeSignalDeliveryStoppedTracee::Intercept(StoppedTraceeInterceptor &visitor) {
  return visitor.Intercept(*this);
}
bool OnGroupStopStoppedTracee::Intercept(StoppedTraceeInterceptor &visitor) {
  return visitor.Intercept(*this);
}
bool BeforeTerminationStoppedTracee::Intercept(StoppedTraceeInterceptor &visitor) {
  return visitor.Intercept(*this);
}
