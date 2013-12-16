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

/* call this at the beginning of your tests */
#define START_TESTS() __tests_ct=0,__tests_success=0

/* call this at the top of your tests file */
#define INIT_TESTS() int __tests_ct=0,__tests_success=0

/** used as a comparison function for numbers. t=type */
#define N_CMP(a,b) ((a) == (b))
/** used as a comparison function for strings. t=char* */
#define S_CMP(a,b) (((a)==NULL && (b)==NULL) \
                        || ((a)!=NULL && (b)!=NULL && strcmp((a), (b)) == 0))

/* don't use this macro directly
 *
 * this stores 'expr' in 'res', and 'expected' in '_exp', where both are
 * variable of type '_t', and 'res' is constant.  It then increments
 * '__tests_ct' and compares 'res' and '_exp' using 'cmp', which should be a
 * 2-parameters comparison macro/function. If the result is truthy, it then
 * increments '__tests_success' and print a message. If the result is falsy
 * (0), it prints an error message.
 * */
#define MYTEST(_t,cmp,fmt,expr,expected) \
        { const _t res = (expr);_t _exp=expected; __tests_ct++; \
          if (cmp(res, (_t)(_exp))) { __tests_success++; \
                  printf(ANSI_COLOR_GREEN "[ OK ] '" S(expr) "' -> '" \
                                  fmt "'" EOTESTLINE, res); \
          } else { printf(ANSI_COLOR_RED "[FAIL] '" S(expr) "' -> '" \
                                  fmt "', expected '" fmt "'" EOTESTLINE, \
                                  res, (_t)_exp); } }

/* call this at the end of the tests */
#define END_OF_TESTS() \
        { int p=((int)((100*__tests_success/(__tests_ct+0.0))*10)/10); \
          printf(ANSI_COLOR_CYAN "Total: %d%% (%d/%d)" EOTESTLINE, \
                          p, __tests_success, __tests_ct); }

/** 'int' test. x = expression, e = expected */
#define INTTEST(x,e)  MYTEST(int, N_CMP, "%d",  x, e)
/** 'long' test. x = expression, e = expected */
#define LONGTEST(x,e) MYTEST(long, N_CMP, "%li", x, e)
/** 'char*' test. x = expression, e = expected */
#define STRTEST(x,e)  MYTEST(char*, S_CMP, "%s", x, e)

/* Add a test suite */
#define ADD_SUITE(N) \
        { printf(ANSI_COLOR_CYAN "--> " S(N) EOTESTLINE); N ## _tests(); }

#endif
