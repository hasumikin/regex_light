#ifndef REGEX_LIGHT_H_
#define REGEX_LIGHT_H_

#include <stddef.h>

#define MAX_ATOM_SIZE 100

typedef struct re_atom ReAtom;

typedef struct {
  size_t re_nsub;  // number of parenthesized subexpressions ( )
  ReAtom *atoms[MAX_ATOM_SIZE];
} regex_t;

typedef struct {
  int rm_so; // start position of match
  int rm_eo; // end position of match
} regmatch_t;

int regcomp(regex_t *preg, const char *pattern, int _cflags);
int regexec(regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags);

#endif /* !REGEX_LIGHT_H_ */
