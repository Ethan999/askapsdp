
#define CONRAD_SUBPACKAGE_NAME "logging"

#include <conrad_conrad.h>
#include <conrad/ConradLogging.h>

using namespace conrad;

int main() {
  // Now set its level. Normally you do not need to set the
  // level of a logger programmatically. This is usually done
  // in configuration files.
  CONRADLOG_INIT("tLogging.log_cfg");
  int i = 1;
  CONRADLOG_WARN("Warning. This is a warning.");
  CONRADLOG_INFO("This is an automatic (subpackage) log");
  CONRADLOG_INFO_STR("This is " << i << " log stream test.");
  return 0;
}