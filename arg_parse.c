/* CS 352 -- arg_parse.c 
 *
 *   January 17, 2017, Austin Corotan
 *   Modified January 18, 2017
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "proto.h"
#include "global.h"

/* function used to remove quotations characters from a string */
static void removeQuotes(char *args) {
    char *src, *dst;
    src = dst = args;
    while(*src != '\0'){
      *dst = *src;
      if(*dst != '\"') {
        dst++;
      }
      src++;
    }
    *dst = '\0';
}

/* function will parse a string input and output an 
 * array of strings containing each argument 
 */
char ** arg_parse (char *line, int *argcp)
{
  char **argv; /* array of strings (to be returned) */
  int count = 0; /* counter for indexing argv */
  typedef enum { false, true } bool; /* boolean type */
  bool is_word = false; /* boolean flag for characters */
  bool is_quote = false; /* boolean flag for quotations */
  int length = 0; /* length of *line */
  int qcount = 0; /* counter for number of quotes */

  /* count arguments */
  while(*line != '\0'){
    if(*line == '\"'){
      if(is_quote == false){
         is_quote = true;
         if(is_word == false){
           (*argcp)++;
         }
      } else {
         is_quote = false;
      }
      is_word = true;
      qcount++;
      line++;
    } else if(*line == ' '){
        if(is_quote == false && is_word == true){
           is_word = false;
        } 
        line++;
    } else {
        if(is_word == false && is_quote == false){
          (*argcp)++;
        }
        is_word = true;
        line++;
    }
    length++;
  }

  /* check for proper number of quotations */
  if(qcount%2 != 0){
    dprintf(2,"Invalid Quotation.\n");
    *argcp = 0;
    return NULL;
  }  

  /* array allocation */
  argv = (char **)malloc(((*argcp) + 1) * sizeof(char *));
  if (argv == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
  }

  /* reset */
  line = line-length;
  is_word = false;
  is_quote = false;

  /* add null characters while assigning pointers */
  while(*line != '\0'){
    if(*line == '\"'){
      if(is_quote == false){
       is_quote = true;
       if(is_word == false){
          argv[count] = line;
          count++;
       }
      } else {
         is_quote = false;
      }
      is_word = true;
      line++;
    }else if(*line == ' '){
      if(is_quote == false && is_word == true){
        *line = '\0';
        is_word = false;
      } 
      line++;
    } else {
      if(is_word == false && is_quote == false){
        argv[count] = line;
        count++;
      }
      is_word = true;
      line++;
    }
  }

  /* remove quotes from strings */
  int i;
  for (i = 0; i < *argcp; ++i)
  {
     removeQuotes(argv[i]);
  }

  /* add null to end of string array */
  argv[*argcp] = '\0';

  return(argv);
}

