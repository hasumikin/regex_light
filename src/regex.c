#include <stdbool.h>
#include <string.h>
#include "regex.h"

/*
 * You can use functions other than malloc() and free()
 *
 *   CFLAGS=-DREGEX_NO_ALLOC_LIBC make
 */
#ifdef REGEX_NO_ALLOC_LIBC
  #define  REGEX_ALLOC(size)  allocProc(size) /* override */
  #define  REGEX_FREE(ptr)    freeProc(ptr)   /* override */
  void *(* allocProc)(size_t);
  void (* freeProc)(void *);
  void setAllocProcs(void *(*allocProcPtr)(size_t), void (*freeProcPtr)(void *))
  {
    allocProc = allocProcPtr;
    freeProc = freeProcPtr;
  }
#else
  #include <stdlib.h>
  #define  REGEX_ALLOC(size)  malloc(size)
  #define  REGEX_FREE(ptr)    free(ptr)
#endif /* REGEX_NO_ALLOC_LIBC */

typedef enum {
  RE_TYPE_TERM = 0, // sentinel of finishing expression. It must be 0
  RE_TYPE_LIT,      // literal
  RE_TYPE_DOT,      // .
  RE_TYPE_QUESTION, // ?
  RE_TYPE_STAR,     // *
  RE_TYPE_PLUS,     // +
  RE_TYPE_BEGIN,    // ^
  RE_TYPE_END,      // $
  RE_TYPE_BRACKET,  // [ ]
  RE_TYPE_LPAREN,   // (
  RE_TYPE_RPAREN,   // )
} ReType;

/*
 * An element of Regular Expression Tree
 */
typedef struct re_atom {
  ReType type;
  union {
    unsigned char ch;   // literal in RE_TYPE_LIT
    unsigned char *ccl; // pointer to content in [ ] RE_TYPE_BRACKET
  };
} ReAtom;

typedef struct re_state {
  char *original_text_top_addr;
  int current_re_nsub;
  int max_re_nsub;
  signed char *match_index_data;
} ReState;

static int match(ReState *rs, ReAtom *regexp, const char *text);
static int matchstar(ReState *rs, ReAtom *c, ReAtom *regexp, const char *text);
static int matchhere(ReState *rs, ReAtom *regexp, const char *text);
static int matchone(ReState *rs, ReAtom *p, const char *text);
static int matchquestion(ReState *rs, ReAtom *regexp, const char *text);
static int matchchars(ReState *rs, const unsigned char *s, const char *text);
static int matchbetween(ReState *rs, const unsigned char *s, const char *text);

/*
 * report nsub
 */
static void
re_report_nsub(ReState *rs, const char *text)
{
  int pos = (int)((long)text - (long)rs->original_text_top_addr);
  rs->match_index_data[pos] = rs->current_re_nsub;
}
#define REPORT_WITHOUT_RETURN (re_report_nsub(rs, text))
#define REPORT \
  do { \
    re_report_nsub(rs, text); \
    return 1; \
  } while (0)
/*
 * matcher functions
 */
static int
matchone(ReState *rs, ReAtom *p, const char *text)
{
  if ((p->type == RE_TYPE_LIT && p->ch == text[0]) || (p->type == RE_TYPE_DOT))
    REPORT;
  if (p->type == RE_TYPE_BRACKET) return matchchars(rs, p->ccl, text);
  return 0;
}

static int
matchquestion(ReState *rs, ReAtom *regexp, const char *text)
{
  if ((matchone(rs, regexp, text) && matchhere(rs, (regexp + 2), text + 1))
      || matchhere(rs, (regexp + 2), text)) {
    return 1; // do not REPORT
  } else {
    return 0;
  }
}

/* matchhere: search for regexp at beginning of text */
static int
matchhere(ReState *rs, ReAtom *regexp, const char *text)
{
  do {
    if (regexp->type == RE_TYPE_TERM)
      return 1; // do not REPORT;
    if (regexp->type == RE_TYPE_LPAREN) {
      rs->max_re_nsub++;
      rs->current_re_nsub = rs->max_re_nsub;
      return matchhere(rs, (regexp + 1), text);
    }
    if (regexp->type == RE_TYPE_RPAREN) {
      rs->current_re_nsub = 0; /* Nested paren doesn't work. eg) `ab(c(de))fg` */
      return matchhere(rs, (regexp + 1), text);
    }
    if ((regexp + 1)->type == RE_TYPE_QUESTION)
      return matchquestion(rs, regexp, text);
    if ((regexp + 1)->type == RE_TYPE_STAR)
      return matchstar(rs, regexp, (regexp + 2), text);
    if ((regexp + 1)->type == RE_TYPE_PLUS)
      return matchone(rs, regexp, text) && matchstar(rs, regexp, (regexp + 2), text + 1);
    if (regexp->type == RE_TYPE_END && (regexp + 1)->type == RE_TYPE_TERM)
      return text[0] == '\0';
    if (text[0] != '\0' && (regexp->type == RE_TYPE_DOT || (regexp->type == RE_TYPE_LIT && regexp->ch == text[0]))) {
      REPORT_WITHOUT_RETURN;
      return matchhere(rs, (regexp + 1), text + 1);
    }
  } while (text[0] != '\0' && matchone(rs, regexp++, text++));
  return 0;
}

static int
matchstar(ReState *rs, ReAtom *c, ReAtom *regexp, const char *text_)
{
  char *text;
  /* leftmost && longest */
  for (text = (char *)text_;
       text[0] != '\0' && (
         (c->type == RE_TYPE_LIT && text[0] == c->ch) ||
         (c->type == RE_TYPE_BRACKET && matchchars(rs, c->ccl, text)) ||
         c->type == RE_TYPE_DOT
       );
       text++)
    REPORT_WITHOUT_RETURN;
  do {  /* * matches zero or more */
    if (matchhere(rs, regexp, text))
      return 1; //REPORT;
  } while (text-- > text_);
  return 0;
}

static int
matchbetween(ReState *rs, const unsigned char* s, const char *text)
{
  if ((text[0] != '-') && (s[0] != '\0') && (s[0] != '-') &&
      (s[1] == '-') && (s[1] != '\0') &&
      (s[2] != '\0') && ((text[0] >= s[0]) && (text[0] <= s[2]))) {
    REPORT;
  } else {
    return 0;
  }
}

static int
matchchars(ReState *rs, const unsigned char* s, const char *text)
{
  do {
    if (matchbetween(rs, s, text)) {
      REPORT;
    } else if (s[0] == '\\') {
      s += 1;
      if (text[0] == s[0]) {
        REPORT;
      }
    } else if (text[0] == s[0]) {
      if (text[0] == '-') {
        return (s[-1] == '\0') || (s[1] == '\0');
      } else {
        REPORT;
      }
    }
  } while (*s++ != '\0');
  return 0;
}

static int
match(ReState *rs, ReAtom *regexp, const char *text)
{
  if (regexp->type == RE_TYPE_BEGIN)
    return (matchhere(rs, (regexp + 1), text));
  do {    /* must look even if string is empty */
    if (matchhere(rs, regexp, text)) {
      return 1;
    } else {
      /* reset match_index_data */
      memset(rs->match_index_data, -1, strlen(text));
      rs->current_re_nsub = 0;
      rs->max_re_nsub = 0;
    }
  } while (*text++ != '\0');
  return 0;
}

void
set_match_data(ReState *rs, size_t nmatch, regmatch_t *pmatch, size_t len)
{
  int i;
  for (i = 0; i < nmatch; i++) {
    (pmatch + i)->rm_so = -1;
    (pmatch + i)->rm_eo = -1;
  }
  bool scanning = false;
  for (i = len - 1; i > -1; i--) {
    if (rs->match_index_data[i] < 0) {
      if (scanning) break;
      continue;
    } else {
      scanning = true;
    }
    if (pmatch->rm_eo < 0)
      pmatch->rm_eo = i + 1;
    if ( (pmatch + rs->match_index_data[i])->rm_eo < 0 )
      (pmatch + rs->match_index_data[i])->rm_eo = i + 1;
    (pmatch + rs->match_index_data[i])->rm_so = i;
  }
  pmatch[0].rm_so = i + 1;
}

/*
 * public functions
 */
int
regexec(regex_t *preg, const char *text, size_t nmatch, regmatch_t *pmatch, int _eflags)
{
  ReState rs;
  rs.original_text_top_addr = (void *)text;
  size_t len = strlen(text);
  signed char mid[len];
  rs.match_index_data = mid;
  memset(rs.match_index_data, -1, len);
  rs.current_re_nsub = 0;
  rs.max_re_nsub = 0;
  if (match(&rs, preg->atoms, text)) {
    set_match_data(&rs, nmatch, pmatch, len);
    return 0; /* success */
  } else {
    return -1; /* to be correct, it should be a thing like REG_NOMATCH */
  }
}

size_t
gen_ccl(ReAtom *atom, const char *snippet, size_t len)
{
  if (len == 0) len = strlen(snippet);
  unsigned char *ccl;
  atom->type = RE_TYPE_BRACKET;
  ccl = REGEX_ALLOC(len + 1);
  memcpy(ccl, snippet, len);
  ccl[len] = '\0';
  atom->ccl = ccl;
  return len + 1;
}
#define gen_ccl_const(atom, snippet) gen_ccl(atom, snippet, 0)

/*
 * compile regular expression pattern
 * _cflags is dummy
 */
#define REGEX_DEF_w "a-zA-Z0-9_"
#define REGEX_DEF_s " \t\f\r\n"
#define REGEX_DEF_d "0-9"
int
regcomp(regex_t *preg, const char *pattern, int _cflags)
{
  ReAtom *atoms = (ReAtom *)REGEX_ALLOC(sizeof(ReAtom) * strlen(pattern));
  ReAtom *atoms_head = atoms;
  preg->re_nsub = 0;
  size_t ccl_len = 0; // total length of ccl(s)
  size_t len;
  while (pattern[0] != '\0') {
    switch (pattern[0]) {
      case '.':
        atoms->type = RE_TYPE_DOT;
        break;
      case '?':
        atoms->type = RE_TYPE_QUESTION;
        break;
      case '*':
        atoms->type = RE_TYPE_STAR;
        break;
      case '+':
        atoms->type = RE_TYPE_PLUS;
        break;
      case '^':
        atoms->type = RE_TYPE_BEGIN;
        break;
      case '$':
        atoms->type = RE_TYPE_END;
        break;
      case '(':
        atoms->type = RE_TYPE_LPAREN;
        preg->re_nsub++;
        break;
      case ')':
        atoms->type = RE_TYPE_RPAREN;
        break;
      case '\\':
        switch (pattern[1]) {
          case '\0':
            atoms->type = RE_TYPE_LIT;
            atoms->ch = '\\';
            break;
          case 'w':
            pattern++;
            ccl_len += gen_ccl_const(atoms, REGEX_DEF_w);
            break;
          case 's':
            pattern++;
            ccl_len += gen_ccl_const(atoms, REGEX_DEF_s);
            break;
          case 'd':
            pattern++;
            ccl_len += gen_ccl_const(atoms, REGEX_DEF_d);
            break;
          default:
            pattern++;
            atoms->type = RE_TYPE_LIT;
            atoms->ch = pattern[0];
        }
        break;
      case '[':
        pattern++;
         /*
          * pattern [] must contain at least one letter.
          * first letter of the content should be ']' if you want to match literal ']'
          */
        for (len = 1;
            pattern[len] != '\0' && (pattern[len] != ']');
            len++)
          ;
        ccl_len += gen_ccl(atoms, pattern, len);
        pattern += len;
        break;
      default:
        atoms->type = RE_TYPE_LIT;
        atoms->ch = pattern[0];
        break;
    }
    pattern++;
    atoms++;
  }
  { /* gather scattered objects into one object */
    size_t atoms_size = (size_t)atoms - (size_t)atoms_head;
    preg->atoms = REGEX_ALLOC(atoms_size + ccl_len);
    memset(preg->atoms, 0, atoms_size + ccl_len);
    memcpy(preg->atoms, atoms_head, atoms_size);
    unsigned char *ccl = (unsigned char *)atoms;
    ReAtom *a = preg->atoms;
    while (a->type != RE_TYPE_TERM) {
      if (a->type == RE_TYPE_BRACKET) {
        len = strlen((const char *)a->ccl) + 1;
        memcpy(ccl, a->ccl, len);
        REGEX_FREE(a->ccl);
        a->ccl = ccl;
        ccl += len;
      }
      a++;
    }
    REGEX_FREE(atoms_head);
  }
  return 0;
}

/*
 * free regex_t object
 */
void
regfree(regex_t *preg)
{
  REGEX_FREE(preg->atoms);
}
