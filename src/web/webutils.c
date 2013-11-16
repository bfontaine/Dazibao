#include "webutils.h"
int _wlog_level =
#ifdef DEBUG
WLOG_LVL_DEBUG;
#else
WLOG_LVL_INFO;
#endif
