#include "logging.h"

int _log_level =
#ifdef DEBUG
        LOG_LVL_DEBUG;
#else
        LOG_LVL_INFO;
#endif

char _log_newline = 1;
