#include <stdlib.h>
#include <string.h>
#include "regex_light.h"

typedef enum {
  RE_TYPE_TERM,     // sentinel of finishing expression
  RE_TYPE_LIT,      // literal
  RE_TYPE_DOT,      // .
  RE_TYPE_QUESTION, // ?
  RE_TYPE_STAR,     // *
  RE_TYPE_PLUS,     // +
  RE_TYPE_BEGIN,    // ^
  RE_TYPE_END,      // $
  RE_TYPE_BRACKET,  // [ ]
//  RE_TYPE_PAREN,    // ( )
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
  regex_t *child;       // use only if type == RE_TYPE_PAREN
} ReAtom;

static int match(ReAtom **regexp, const char *text);
static int matchstar(ReAtom c, ReAtom **regexp, const char *text);
static int matchhere(ReAtom **regexp, const char *text);
static int matchone(ReAtom p, const char *text);
static int matchquestion(ReAtom **regexp, const char *text);
static int matchchars(const unsigned char *s, const char *text);
static int matchbetween(const unsigned char *s, const char *text);

/*
 * constractor of ReAtom
 */
static ReAtom
*re_atom_new(ReType type)
{
  ReAtom *atom = malloc(sizeof(ReAtom));
  atom->type = type;
  return atom;
}

typedef struct re_state {
  char *original_text_top_addr;
  int current_re_nsub;
  int max_re_nsub;
} ReState;
static ReState rs;
/*
 * report nsub
 */
#include <stdio.h>
static void
re_report_nsub(const char *text)
{
  printf(
    "%ld/%d_",
    (long)&text[0] - (long)&rs.original_text_top_addr[0],
    rs.current_re_nsub
  );
}
#define REPORT_WITHOUT_RETURN (re_report_nsub(text))
#define REPORT \
  do { \
    re_report_nsub(text); \
    return 1; \
  } while (0)
/*
 * matcher functions
 */
static int
//matchone(ReAtom p, int c)
matchone(ReAtom p, const char *text)
{
  if ((p.type == RE_TYPE_LIT && p.ch == *text) || (p.type == RE_TYPE_DOT))
    REPORT;
  if (p.type == RE_TYPE_BRACKET) return matchchars(p.ccl, text);
//  if (p.type == RE_TYPE_PAREN) {
//    rs.current_re_nsub++;
//    int res = match(p.child->atoms, text);
//    rs.current_re_nsub--;
//    return res;
//  }
  return 0;
}

static int
matchquestion(ReAtom **regexp, const char *text)
{
  if ((matchone(*regexp[0], text) && matchhere((regexp + 2), text + 1))
      || matchhere((regexp + 2), text)) {
    return 1; // do not REPORT
  } else {
    return 0;
  }
}

/* matchhere: search for regexp at beginning of text */
static int
matchhere(ReAtom **regexp, const char *text)
{
  do {
    if (regexp[0]->type == RE_TYPE_TERM)
      return 1; // do not REPORT;
    if (regexp[0]->type == RE_TYPE_LPAREN) {
      rs.max_re_nsub++;
      rs.current_re_nsub = rs.max_re_nsub;
      return matchhere((regexp + 1), text);
    }
    if (regexp[0]->type == RE_TYPE_RPAREN) {
      rs.current_re_nsub--;
      return matchhere((regexp + 1), text);
    }
    if ((regexp + 1)[0]->type == RE_TYPE_QUESTION)
      return matchquestion(regexp, text);
    if ((regexp + 1)[0]->type == RE_TYPE_STAR)
      return matchstar(*regexp[0], (regexp + 2), text);
    if ((regexp + 1)[0]->type == RE_TYPE_PLUS)
      return matchone(*regexp[0], text) && matchstar(*regexp[0], (regexp + 2), text + 1);
    if (regexp[0]->type == RE_TYPE_END && (regexp + 1)[0]->type == RE_TYPE_TERM)
      return *text == '\0';
    if (*text != '\0' && (regexp[0]->type == RE_TYPE_DOT || (regexp[0]->type == RE_TYPE_LIT && regexp[0]->ch == *text))) {
      REPORT_WITHOUT_RETURN;
      return matchhere((regexp + 1), text + 1);
    }
  } while (text[0] != '\0' && matchone(**regexp++, text++));
  return 0;
}

static int
matchstar(ReAtom c, ReAtom **regexp, const char *text_)
{
  char *text;
  /* leftmost && longest */
  for (text = (char *)text_;
       *text != '\0' && (
         (c.type == RE_TYPE_LIT && *text == c.ch) ||
         (c.type == RE_TYPE_BRACKET && matchchars(c.ccl, text)) ||
         c.type == RE_TYPE_DOT
       );
       text++)
    REPORT_WITHOUT_RETURN;
  do {  /* * matches zero or more */
    if (matchhere(regexp, text))
      return 1; //REPORT;
  } while (text-- > text_);
  return 0;
}

static int
matchbetween(const unsigned char* s, const char *text)
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
matchchars(const unsigned char* s, const char *text)
{
  do {
    if (matchbetween(s, text)) {
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
match(ReAtom **regexp, const char *text)
{
  if (regexp[0]->type == RE_TYPE_BEGIN)
    return (matchhere((regexp + 1), text));
  do {    /* must look even if string is empty */
    if (matchhere(regexp, text))
      REPORT;
  } while (*text++ != '\0');
  return 0;
}

/*
 * public functions
 */
int
regexec(regex_t *preg, const char *text, size_t nmatch, regmatch_t pmatch[], int _eflags)
{
  rs.original_text_top_addr = (void *)text;
  rs.current_re_nsub = 0;
  rs.max_re_nsub = 0;
  return match(preg->atoms, text);
}

/*
 * compile regular expression pattern
 * _cflags is dummy
 */
int
regcomp(regex_t *preg, const char *pattern, int _cflags)
{
  preg->re_nsub = 0;
  char c; // current char in pattern
  int i = 0; // current position in pattern
  int j = 0; // index of atom
  while (pattern[i] != '\0') {
    c = pattern[i];
    switch (c) {
      case '.':
        preg->atoms[j] = re_atom_new(RE_TYPE_DOT);
        break;
      case '?':
        preg->atoms[j] = re_atom_new(RE_TYPE_QUESTION);
        break;
      case '*':
        preg->atoms[j] = re_atom_new(RE_TYPE_STAR);
        break;
      case '+':
        preg->atoms[j] = re_atom_new(RE_TYPE_PLUS);
        break;
      case '^':
        preg->atoms[j] = re_atom_new(RE_TYPE_BEGIN);
        break;
      case '$':
        preg->atoms[j] = re_atom_new(RE_TYPE_END);
        break;
      case '(':
//        preg->re_nsub++;
//        preg->atoms[j] = re_atom_new(RE_TYPE_PAREN);
//        regex_t *new_preg = malloc(sizeof(regex_t));
//        preg->atoms[j]->child = new_preg;
//        regcomp(new_preg, pattern + i + 1, _cflags);
//        for (; pattern[i - 1] != '\\' && pattern[i] != ')' && pattern[i] != '\0'; i++)
//          ;
        preg->atoms[j] = re_atom_new(RE_TYPE_LPAREN);
        preg->re_nsub++;
        break;
      case ')':
        preg->atoms[j] = re_atom_new(RE_TYPE_RPAREN);
        break;
//        preg->atoms[j] = re_atom_new(RE_TYPE_TERM);
//        return 0;
      case '\\':
        preg->atoms[j] = re_atom_new(RE_TYPE_LIT);
        if (pattern[i + 1] == '\0') {
          preg->atoms[j]->ch = '\\';
        } else {
          i++;
          preg->atoms[j]->ch = pattern[i];
        }
        break;
      case '[':
        i++;
        int len;
        for (len = 0;
            pattern[i + len] != '\0'
              && (pattern[i + len] != ']'
                   || (pattern[i + len] == ']' && pattern[i + len - 1] == '\\')
                 );
            len++)
          ;
        unsigned char *ccl = malloc(len + 1); // possibly longer than actual
        int k = 0;
        for (len = 0; pattern[i + len] != '\0'; len++) {
          if (pattern[i + len] == ']') break;
          if (pattern[i + len] == '\\' && pattern[i + len + 1] == ']') {
            ccl[k] = pattern[i + len + 1];
            len++;
          } else {
            ccl[k] = pattern[i + len];
          }
          k++;
        }
        ccl[k] = '\0';
        preg->atoms[j] = re_atom_new(RE_TYPE_BRACKET);
        preg->atoms[j]->ccl = ccl;
        i += len;
        break;
      default:
        preg->atoms[j] = re_atom_new(RE_TYPE_LIT);
        preg->atoms[j]->ch = c;
        break;
    }
    i++;
    j++;
    if (j >= MAX_ATOM_SIZE) return 1;
  }
  preg->atoms[j] = re_atom_new(RE_TYPE_TERM);
  return 0;
}
