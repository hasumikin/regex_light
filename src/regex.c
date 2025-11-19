#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "./regex.h"

/*
 * You can use stdlib's malloc() and free()
 *   CFLAGS=-DREGEX_USE_ALLOC_LIBC make
 */
#ifndef REGEX_USE_ALLOC_LIBC
  #define  REGEX_ALLOC(size)  RegexAllocProc(size) /* override */
  #define  REGEX_FREE(ptr)    RegexFreeProc(ptr)   /* override */
  void *(* RegexAllocProc)(size_t);
  void (* RegexFreeProc)(void *);
  void RegexSetAllocProcs(void *(*allocProcPtr)(size_t), void (*freeProcPtr)(void *))
  {
    RegexAllocProc = allocProcPtr;
    RegexFreeProc = freeProcPtr;
  }
#else
  #include <stdlib.h>
  #define  REGEX_ALLOC(size)  malloc(size)
  #define  REGEX_FREE(ptr)    free(ptr)
#endif /* REGEX_USE_ALLOC_LIBC */

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
  int *match_index_data;
  int nsub_stack[10];
  int nsub_stack_ptr;
} ReState;

static int match(ReState *rs, ReAtom *regexp, const char *text, ReAtom *start);
static int matchstar(ReState *rs, ReAtom *c, ReAtom *regexp, const char *text, ReAtom *start);
static int matchhere(ReState *rs, ReAtom *regexp, const char *text, ReAtom *start);
static int matchone(ReState *rs, ReAtom *p, const char *text);
static int matchquestion(ReState *rs, ReAtom *regexp, const char *text, ReAtom *start);
static int match_group_question(ReState *rs, ReAtom *lparen, ReAtom *rparen, const char *text, ReAtom *start);
static int match_group_star(ReState *rs, ReAtom *lparen, ReAtom *rparen, const char *text, ReAtom *start);
static int match_group_plus(ReState *rs, ReAtom *lparen, ReAtom *rparen, const char *text, ReAtom *start);
static int match_group_content_once(ReState *rs, ReAtom *lparen, ReAtom *rparen, const char *text, ReAtom *start);
static int matchchars(ReState *rs, const unsigned char *s, const char *text);
static int matchbetween(const unsigned char *s, const char *text);
static ReAtom* find_rparen(ReAtom *lparen);

static ReAtom*
find_rparen(ReAtom *lparen) {
  ReAtom *p = lparen + 1;
  int level = 1;
  while (p->type != RE_TYPE_TERM) {
    if (p->type == RE_TYPE_LPAREN) level++;
    else if (p->type == RE_TYPE_RPAREN) {
      level--;
      if (level == 0) return p;
    }
    p++;
  }
  return NULL;
}

/*
 * report nsub
 */
static void
re_report_nsub(ReState *rs, const char *text)
{
  int pos = (int)((long)text - (long)rs->original_text_top_addr);
  int i;
  int mask = (1 << rs->current_re_nsub) | 1;
  for (i = 0; i < rs->nsub_stack_ptr; i++) {



    mask |= (1 << rs->nsub_stack[i]);
  }
  rs->match_index_data[pos] = mask;
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
  return -1;
}

static int
match_group_content_once(ReState *rs, ReAtom *lparen, ReAtom *rparen, const char *text, ReAtom *start)
{
  int len;
  // Save state
  size_t text_len = strlen(rs->original_text_top_addr);
  int saved_mid[text_len];
  memcpy(saved_mid, rs->match_index_data, sizeof(int) * text_len);

  int saved_nsub_stack_ptr = rs->nsub_stack_ptr;
  int saved_current_re_nsub = rs->current_re_nsub;
  int saved_max_re_nsub = rs->max_re_nsub;

  // Push group state
  if (rs->nsub_stack_ptr >= 10) return -1;
  rs->nsub_stack[rs->nsub_stack_ptr++] = rs->current_re_nsub;
  rs->max_re_nsub++;
  rs->current_re_nsub = rs->max_re_nsub;

  // Temporarily terminate the group
  ReType old_type = rparen->type;
  rparen->type = RE_TYPE_TERM;

  len = matchhere(rs, lparen + 1, text, start);

  rparen->type = old_type; // Restore rparen type

  if (len < 0) {
    // Restore state on failure
    rs->nsub_stack_ptr = saved_nsub_stack_ptr;
    rs->current_re_nsub = saved_current_re_nsub;
    rs->max_re_nsub = saved_max_re_nsub;
    memcpy(rs->match_index_data, saved_mid, sizeof(int) * text_len);
    return -1;
  }

  // Pop group state
  rs->nsub_stack_ptr--;
  rs->current_re_nsub = rs->nsub_stack[rs->nsub_stack_ptr];

  return len;
}

static int
matchquestion(ReState *rs, ReAtom *regexp, const char *text, ReAtom *start)
{
  int len1, len2;
  // Path 1 (greedy): match one and rest
  len1 = matchone(rs, regexp, text);
  if (len1 > 0) {
    len2 = matchhere(rs, regexp + 2, text + len1, start);
    if (len2 >= 0) return len1 + len2;
  }
  // Path 2: match zero and rest
  return matchhere(rs, regexp + 2, text, start);
}

static int
match_group_question(ReState *rs, ReAtom *lparen, ReAtom *rparen, const char *text, ReAtom *start)
{
  int len1, len2;
  size_t text_len = strlen(rs->original_text_top_addr);
  int saved_mid[text_len];
  memcpy(saved_mid, rs->match_index_data, sizeof(int) * text_len);

  // Path 1 (greedy): match group and rest
  ReType old_type = rparen->type;
  rparen->type = RE_TYPE_TERM;
  len1 = matchhere(rs, lparen + 1, text, start);
  rparen->type = old_type;

  if (len1 >= 0) {
    len2 = matchhere(rs, rparen + 2, text + len1, start);
    if (len2 >= 0) return len1 + len2;
  }

  // Path 2: match zero and rest
  memcpy(rs->match_index_data, saved_mid, sizeof(int) * text_len);
  return matchhere(rs, rparen + 2, text, start);
}

static int
match_group_star(ReState *rs, ReAtom *lparen, ReAtom *rparen, const char *text, ReAtom *start)
{
  int len_g, len_b;
  size_t text_len = strlen(rs->original_text_top_addr);

  // Path 1 (greedy): Match G once, then recurse
  // Save state before trying G
  int saved_mid[text_len];
  memcpy(saved_mid, rs->match_index_data, sizeof(int) * text_len);
  int saved_nsub_stack_ptr = rs->nsub_stack_ptr;
  int saved_current_re_nsub = rs->current_re_nsub;
  int saved_max_re_nsub = rs->max_re_nsub;

  len_g = match_group_content_once(rs, lparen, rparen, text, start);
  if (len_g > 0) {
    len_b = match_group_star(rs, lparen, rparen, text + len_g, start);
    if (len_b >= 0) return len_g + len_b;
  }

  // If Path 1 failed, restore state and try Path 2
  rs->nsub_stack_ptr = saved_nsub_stack_ptr;
  rs->current_re_nsub = saved_current_re_nsub;
  rs->max_re_nsub = saved_max_re_nsub;
  memcpy(rs->match_index_data, saved_mid, sizeof(int) * text_len);

  // Path 2: Match B (0 G's)
  return matchhere(rs, rparen + 2, text, start);
}

static int
match_group_plus(ReState *rs, ReAtom *lparen, ReAtom *rparen, const char *text, ReAtom *start)
{
  int len_g, len_b;
  size_t text_len = strlen(rs->original_text_top_addr);

  // Path 1: Match G once
  // Save state before trying G
  int saved_mid[text_len];
  memcpy(saved_mid, rs->match_index_data, sizeof(int) * text_len);
  int saved_nsub_stack_ptr = rs->nsub_stack_ptr;
  int saved_current_re_nsub = rs->current_re_nsub;
  int saved_max_re_nsub = rs->max_re_nsub;

  len_g = match_group_content_once(rs, lparen, rparen, text, start);
  if (len_g > 0) {
    // If G matched, try to match (G)*B from the new position
    len_b = match_group_star(rs, lparen, rparen, text + len_g, start);
    if (len_b >= 0) return len_g + len_b;
  }

  // If G didn't match, or (G)*B failed, restore state and return failure
  rs->nsub_stack_ptr = saved_nsub_stack_ptr;
  rs->current_re_nsub = saved_current_re_nsub;
  rs->max_re_nsub = saved_max_re_nsub;
  memcpy(rs->match_index_data, saved_mid, sizeof(int) * text_len);

  return -1;
}

/* matchhere: search for regexp at beginning of text */
static int
matchhere(ReState *rs, ReAtom *regexp, const char *text, ReAtom *start)
{
  int len;
  if (regexp->type == RE_TYPE_TERM) return 0;

  if ((regexp + 1)->type == RE_TYPE_QUESTION)
    return matchquestion(rs, regexp, text, start);
  if ((regexp + 1)->type == RE_TYPE_STAR)
    return matchstar(rs, regexp, (regexp + 2), text, start);
  if ((regexp + 1)->type == RE_TYPE_PLUS) {
    int len1 = matchone(rs, regexp, text);
    if (len1 < 0) return -1;
    int len2 = matchstar(rs, regexp, (regexp + 2), text + len1, start);
    if (len2 < 0) return -1;
    return len1 + len2;
  }

  if (regexp->type == RE_TYPE_END && (regexp + 1)->type == RE_TYPE_TERM)
    return text[0] == '\0' ? 0 : -1;

  if (regexp->type == RE_TYPE_LPAREN) {
    ReAtom *rparen = find_rparen(regexp);
    if (rparen) {
      if ((rparen + 1)->type == RE_TYPE_QUESTION)
        return match_group_question(rs, regexp, rparen, text, start);
      if ((rparen + 1)->type == RE_TYPE_STAR)
        return match_group_star(rs, regexp, rparen, text, start);
      if ((rparen + 1)->type == RE_TYPE_PLUS)
        return match_group_plus(rs, regexp, rparen, text, start);
    }
    int len, len2;
    if (rs->nsub_stack_ptr >= 10) return -1;
    rs->nsub_stack[rs->nsub_stack_ptr++] = rs->current_re_nsub;
    rs->max_re_nsub++;
    rs->current_re_nsub = rs->max_re_nsub;

    if (!rparen) return -1;

    ReType old_type = rparen->type;
    rparen->type = RE_TYPE_TERM;

    len = matchhere(rs, regexp + 1, text, start);

    rparen->type = old_type;

    if (len < 0) {
      rs->max_re_nsub--;
      rs->nsub_stack_ptr--;
      rs->current_re_nsub = rs->nsub_stack[rs->nsub_stack_ptr];
      return -1;
    }

    rs->nsub_stack_ptr--;
    rs->current_re_nsub = rs->nsub_stack[rs->nsub_stack_ptr];

    len2 = matchhere(rs, rparen + 1, text + len, start);
    if (len2 < 0) return -1;

    return len + len2;
  }

  if (text[0] != '\0') {
    len = matchone(rs, regexp, text);
    if (len > 0) {
      int next_len = matchhere(rs, regexp + 1, text + len, start);
      if (next_len >= 0) {
        return len + next_len;
      }
    }
  }
  return -1;
}

static int
matchstar(ReState *rs, ReAtom *c, ReAtom *regexp, const char *text, ReAtom *start)
{
  const char *t;
  int len;

  for (t = text; *t != '\0' && matchone(rs, c, t) > 0; t++)
    ;

  for (;; t--) {
    len = matchhere(rs, regexp, t, start);
    if (len >= 0) {
      return (t - text) + len;
    }
    if (t == text) break;
  }

  return -1;
}

static int
matchbetween(const unsigned char* s, const char *text)
{
  if ((text[0] != '-') && (s[0] != '\0') && (s[0] != '-') &&
      (s[1] == '-') && (s[1] != '\0') &&
      (s[2] != '\0') && ((text[0] >= s[0]) && (text[0] <= s[2]))) {
    return 1;
  } else {
    return -1;
  }
}

static int
matchchars(ReState *rs, const unsigned char* s, const char *text)
{
  do {
    // Check for ranges first
    if (s[0] != '\0' && s[1] == '-' && s[2] != '\0') { // Potential range
      if (matchbetween(s, text) > 0) {
        REPORT;
        return 1;
      }
      s += 2; // Skip the range (e.g., 'a-z')
    }
    // Handle escaped characters
    else if (s[0] == '\\') {
      s += 1;
      if (text[0] == s[0]) {
        REPORT;
        return 1;
      }
    }
    // Handle literal characters
    else if (text[0] == s[0]) {
      REPORT;
      return 1;
    }
  } while (*s++ != '\0');
  return -1;
}

static int
match(ReState *rs, ReAtom *regexp, const char *text, ReAtom *start)
{
  if (regexp->type == RE_TYPE_BEGIN) {
    if (matchhere(rs, (regexp + 1), text, start) >= 0) return 0;
    else return -1;
  }
  do {    /* must look even if string is empty */
    if (matchhere(rs, regexp, text, start) >= 0) {
      return 0;
    } else {
      /* reset match_index_data */
      memset(rs->match_index_data, 0, sizeof(int) * strlen(text));
      rs->current_re_nsub = 0;
      rs->max_re_nsub = 0;
      rs->nsub_stack_ptr = 0;
    }
  } while (*text++ != '\0');
  return -1;
}

void
set_match_data(ReState *rs, size_t nmatch, regmatch_t *pmatch, size_t len)
{
  int i, j;
  for (i = 0; i < nmatch; i++) {
    (pmatch + i)->rm_so = -1;
    (pmatch + i)->rm_eo = -1;
  }
  bool scanning = false;
  for (i = len - 1; i > -1; i--) {
    if (rs->match_index_data[i] == 0) {
      if (scanning) break;
      continue;
    } else {
      scanning = true;
    }
    for (j = 0; j < nmatch; j++) {
      if (rs->match_index_data[i] & (1 << j)) {
        if ((pmatch + j)->rm_eo < 0)
          (pmatch + j)->rm_eo = i + 1;
        (pmatch + j)->rm_so = i;
      }
    }
  }
  if (scanning) {
    pmatch[0].rm_so = i + 1;
  }
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
  int mid[len];
  rs.match_index_data = mid;
  memset(rs.match_index_data, 0, sizeof(int) * len);
  rs.current_re_nsub = 0;
  rs.max_re_nsub = 0;
  rs.nsub_stack_ptr = 0;
  if (match(&rs, preg->atoms, text, preg->atoms) >= 0) {
    set_match_data(&rs, nmatch, pmatch, len);
    return 0; /* success */
  } else {
    return -1; /* to be correct, it should be a thing like REG_NOMATCH */
  }
}

size_t
gen_ccl(ReAtom *atom, unsigned char **ccl, const char *snippet, size_t len, bool dry_run)
{
  if (len == 0) len = strlen(snippet);
  if (!dry_run) {
    memcpy(*ccl, snippet, len);
    (*ccl)[len] = '\0';
    atom->ccl = *ccl;
    atom->type = RE_TYPE_BRACKET;
    *ccl += len + 1;
  }
  return len + 1;
}
#define gen_ccl_const(atom, ccl, snippet, dry_run) gen_ccl(atom, ccl, snippet, 0, dry_run)

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
  ReAtom *atoms = REGEX_ALLOC(sizeof(ReAtom));
  preg->re_nsub = 0;
  size_t ccl_len = 0; // total length of ccl(s)
  size_t len;
  bool dry_run = true;
  char *pattern_index = (char *)pattern;
  uint16_t atoms_count = 1;
  unsigned char *ccl = '\0';
  /*
   * Just calculates size of atoms as a dry-run,
   * then makes atoms and ccl(s) at the second time
   */
  for (;;) {
    while (pattern_index[0] != '\0') {
      switch (pattern_index[0]) {
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
          if (!dry_run) preg->re_nsub++;
          break;
        case ')':
          atoms->type = RE_TYPE_RPAREN;
          break;
        case '\\':
          switch (pattern_index[1]) {
            case '\0':
              atoms->type = RE_TYPE_LIT;
              atoms->ch = '\\';
              break;
            case 'w':
              pattern_index++;
              ccl_len += gen_ccl_const(atoms, &ccl, REGEX_DEF_w, dry_run);
              break;
            case 's':
              pattern_index++;
              ccl_len += gen_ccl_const(atoms, &ccl, REGEX_DEF_s, dry_run);
              break;
            case 'd':
              pattern_index++;
              ccl_len += gen_ccl_const(atoms, &ccl, REGEX_DEF_d, dry_run);
              break;
            default:
              pattern_index++;
              atoms->type = RE_TYPE_LIT;
              atoms->ch = pattern_index[0];
          }
          break;
        case '[':
          pattern_index++;
           /*
            * pattern [] must contain at least one letter.
            * first letter of the content should be ']' if you want to match literal ']'
            */
          for (len = 1;
              pattern_index[len] != '\0' && (pattern_index[len] != ']');
              len++)
            ;
          ccl_len += gen_ccl(atoms, &ccl, pattern_index, len, dry_run);
          pattern_index += len;
          break;
        default:
          atoms->type = RE_TYPE_LIT;
          atoms->ch = pattern_index[0];
          break;
      }
      pattern_index++;
      if (dry_run) {
        atoms_count++;
      } else {
        atoms++;
      } 
    }
    if (dry_run) {
      dry_run = false;
      pattern_index = (char *)pattern;
      REGEX_FREE(atoms);
      atoms = (ReAtom *)REGEX_ALLOC(sizeof(ReAtom) * atoms_count + ccl_len);
      ccl = (unsigned char *)(atoms + atoms_count);
    } else {
      atoms->type = RE_TYPE_TERM;
      preg->atoms = atoms - atoms_count + 1;
      break;
    }
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

