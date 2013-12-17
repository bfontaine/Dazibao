#include <stdio.h>
#include "logging.h"
#include "tests.h"

INIT_TESTS();

void log_tests(void) {
        char has_LOGTRACE = 0,
             has_LOGDEBUG = 0,
             has_LOGINFO  = 0,
             has_LOGWARN  = 0,
             has_LOGERROR = 0,
             has_LOGFATAL = 0;

#ifdef LOGTRACE
        has_LOGTRACE = 1;
#endif
#ifdef LOGDEBUG
        has_LOGDEBUG = 1;
#endif
#ifdef LOGINFO
        has_LOGINFO  = 1;
#endif
#ifdef LOGWARN
        has_LOGWARN  = 1;
#endif
#ifdef LOGERROR
        has_LOGERROR = 1;
#endif
#ifdef LOGFATAL
        has_LOGFATAL = 1;
#endif

        INTTEST(has_LOGTRACE, 1);
        INTTEST(has_LOGDEBUG, 1);
        INTTEST(has_LOGINFO, 1);
        INTTEST(has_LOGWARN, 1);
        INTTEST(has_LOGERROR, 1);
        INTTEST(has_LOGFATAL, 1);

}

int main(void) {
        
        START_TESTS();

        ADD_SUITE(log);

        END_OF_TESTS();

        return 0;
}
