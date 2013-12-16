#ifndef _TESTS_H
#define _TESTS_H 1
/** helpers for tests */

#define _S(x) #x

/** stringify its argument */
#define S(x) _S(x)

extern int __tests_ct;
extern int __tests_success;

#define START_TESTS() __tests_ct=0,__tests_success=0
#define INIT_TESTS() int __tests_ct=0,__tests_success=0

/* don't use this macro directly */
#define MYTEST(_t,fmt,expr,expected) \
        { _t res = (expr); __tests_ct++; \
          if (res != (_t)(expected)) { \
                  printf("[FAIL] '" S(expr) "' -> '" fmt "', expected '" \
                                  fmt "'\n", res, (_t)expected); \
          } else { __tests_success++; \
                  printf("[ OK ] '" S(expr) "' -> '" fmt "'\n", res); } }

#define END_OF_TESTS() \
        { int p=((int)((100*__tests_success/(__tests_ct+0.0))*10)/10); \
          printf("Total: %d%% (%d/%d)\n", p, __tests_success, __tests_ct); }

/** 'int' test. x = expression, e = expected */
#define INTTEST(x,e)  MYTEST(int,  "%d",  x,e)
/** 'long' test. x = expression, e = expected */
#define LONGTEST(x,e) MYTEST(long, "%li", x,e)
/** 'char*' test. x = expression, e = expected */
#define STRTEST(x,e)  MYTEST(char*, "%s", x,e)

/* Add a test suite */
#define ADD_SUITE(N) \
        { printf("--> " S(N) "\n"); N ## _tests(); }

#endif
