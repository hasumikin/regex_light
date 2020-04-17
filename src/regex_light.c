#include <stdlib.h>
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
  RE_TYPE_PAREN,    // ( )
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
  int ref_number;       // backreference number such as \1, \2
  regex_t *child;       // use only if type == RE_TYPE_PAREN
} ReAtom;

static int match(ReAtom **regexp, const char *text);
static int matchstar(ReAtom c, ReAtom **regexp, const char *text);
static int matchhere(ReAtom **regexp, const char *text);
static int matchone(ReAtom p, int c);
static int matchquestion(ReAtom **regexp, const char *text);

static int
matchone(ReAtom p, int c)
{
  if (p.type == RE_TYPE_DOT) return 1;
  return (p.ch == c); // TODO make sure it is RE_TYPE_LIT
}

static int
matchquestion(ReAtom **regexp, const char *text)
{
  return (matchone(*regexp[0], text[0]) && match((regexp + 2), text + 1)) || match((regexp + 2), text);
}

/* matchhere: search for regexp at beginning of text */
static int
matchhere(ReAtom **regexp, const char *text)
{
  if (regexp[0]->type == RE_TYPE_TERM)
    return 1;
  if ((regexp + 1)[0]->type == RE_TYPE_QUESTION)
    return matchquestion(regexp, text);
  if ((regexp + 1)[0]->type == RE_TYPE_STAR)
    return matchstar(*regexp[0], (regexp + 2), text);
  if ((regexp + 1)[0]->type == RE_TYPE_PLUS)
    return matchone(*regexp[0], text[0]) && matchstar(*regexp[0], (regexp + 2), text + 1);
  if (regexp[0]->type == RE_TYPE_END && (regexp + 1)[0]->type == RE_TYPE_TERM)
    return *text == '\0';
  if (*text != '\0' && (regexp[0]->type == RE_TYPE_DOT || (regexp[0]->type == RE_TYPE_LIT && regexp[0]->ch == *text)))
    return matchhere((regexp + 1), text + 1);
  return 0;
}

/* matchstar: search for c*regexp at beginning of text */
static int
matchstar(ReAtom c, ReAtom **regexp, const char *text)
{
  char *t;
  for (t = (char *)text;
       *t != '\0' && ((c.type == RE_TYPE_LIT && *t == c.ch) || c.type == RE_TYPE_DOT);
       t++)
    ;
  do {  /* * matches zero or more */
    if (matchhere(regexp, t))
      return 1;
  } while (t-- > text);
  return 0;
}

static ReAtom
*re_atom_new(ReType type)
{
  ReAtom *atom = malloc(sizeof(ReAtom));
  atom->type = type;
  return atom;
}

/* match: search for regexp anywhere in text */
static int
match(ReAtom **regexp, const char *text)
{
  if (regexp[0]->type == RE_TYPE_BEGIN)
    return matchhere((regexp + 1), text);
  do {    /* must look even if string is empty */
    if (matchhere(regexp, text))
      return 1;
  } while (*text++ != '\0');
  return 0;
}

/* public */

int
regexec(regex_t *preg, const char *text, size_t nmatch, regmatch_t pmatch[], int _eflags)
{
  return match(preg->atoms, text);
}

/*
 * compile regular expression pattern
 * _cflags is dummy
 */
int
regcomp(regex_t *preg, const char *pattern, int _cflags)
{
  char c;       // current char in pattern
  static int ref_num; // current number for backreference
  ref_num = 0;
  static int i; // current position in pattern
  i = 0;
  static int j; // index of atom
  j = 0;
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
        preg->atoms[j] = re_atom_new(RE_TYPE_PAREN);
        regex_t *new_preg = malloc(sizeof(regex_t));
        preg->atoms[j]->child = new_preg;
        regcomp(new_preg, pattern + i + 1, _cflags);
        i++; // ')'
        break;
      default:
        preg->atoms[j] = re_atom_new(RE_TYPE_LIT);
        preg->atoms[j]->ch = c;
        break;
    }
    preg->atoms[j]->ref_number = ref_num;
    i++;
    j++;
    if (j >= MAX_ATOM_SIZE) return 1;
  }
  preg->atoms[j] = re_atom_new(RE_TYPE_TERM);
  return 0;
}
