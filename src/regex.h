#ifndef REGEX_LIGHT_H_
#define REGEX_LIGHT_H_

#include <stdint.h>
#include <stddef.h>

typedef struct re_atom ReAtom;

typedef void *(*regex_alloc_fn_t)(void *ctx, size_t size);
typedef void (*regex_free_fn_t)(void *ctx, void *ptr);

typedef struct {
  size_t re_nsub;  // number of parenthesized subexpressions ( )
  ReAtom *atoms;
  void *alloc_ctx;
  regex_alloc_fn_t alloc_fn;
  regex_free_fn_t free_fn;
} regex_t;

typedef struct {
  int16_t rm_so; // start position of match
  int16_t rm_eo; // end position of match
} regmatch_t;

/* regcomp() flags */
#define	REG_BASIC       0000
#define	REG_EXTENDED    0001
#define	REG_ICASE       0002
#define	REG_NOSUB       0004
#define	REG_NEWLINE     0010
#define	REG_NOSPEC      0020
#define	REG_PEND        0040
#define	REG_DUMP        0200

int regcomp(regex_t *preg, const char *pattern, int cflags,
            void *alloc_ctx, regex_alloc_fn_t alloc_fn, regex_free_fn_t free_fn);
void regfree(regex_t *preg);
int regexec(regex_t *preg, const char *string, size_t nmatch, regmatch_t *pmatch, int eflags);

#endif /* !REGEX_LIGHT_H_ */
