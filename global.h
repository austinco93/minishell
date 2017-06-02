/* CS 352 -- global.h 
 *
 *   February 4, 2017, Austin Corotan
 */

#ifdef MAIN

#define GLOBAL_VAR(tp, nm, init)  extern tp nm ;  tp nm = init

#else

#define GLOBAL_VAR(tp, nm, init) extern tp nm

#endif

/* Global variable initialization */

GLOBAL_VAR (char**, mainargv, NULL);

GLOBAL_VAR (int,  mainargc,  0);

GLOBAL_VAR (const int,  LINELEN,  200000);

GLOBAL_VAR (int,  shift,  1);

GLOBAL_VAR (int,  statuscheck,  0);

GLOBAL_VAR (int,  sig_signaled,  0);

GLOBAL_VAR (FILE*,  fstream,  NULL);

GLOBAL_VAR (const int,  WAIT,  1);

GLOBAL_VAR (const int,  EXPAND,  2);

GLOBAL_VAR (const int,  STMTS,  4);