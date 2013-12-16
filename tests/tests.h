#ifndef _TESTS_H
#define _TESTS_H 1
/** helpers for tests */

/* from http://stackoverflow.com/a/3219471/735926 */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define EOTESTLINE         "\x1b[0m\n"

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
                  printf(ANSI_COLOR_RED "[FAIL] '" S(expr) "' -> '" \
                                  fmt "', expected '" fmt "'" EOTESTLINE, \
                                  res, (_t)expected); \
          } else { __tests_success++; \
                  printf(ANSI_COLOR_GREEN "[ OK ] '" S(expr) "' -> '" \
                                  fmt "'" EOTESTLINE, res); } }

#define END_OF_TESTS() \
        { int p=((int)((100*__tests_success/(__tests_ct+0.0))*10)/10); \
          printf(ANSI_COLOR_CYAN "Total: %d%% (%d/%d)" EOTESTLINE, \
                          p, __tests_success, __tests_ct); }

/** 'int' test. x = expression, e = expected */
#define INTTEST(x,e)  MYTEST(int,  "%d",  x,e)
/** 'long' test. x = expression, e = expected */
#define LONGTEST(x,e) MYTEST(long, "%li", x,e)
/** 'char*' test. x = expression, e = expected */
#define STRTEST(x,e)  MYTEST(char*, "%s", x,e)

/* Add a test suite */
#define ADD_SUITE(N) \
        { printf(ANSI_COLOR_CYAN "--> " S(N) EOTESTLINE); N ## _tests(); }

#endif
