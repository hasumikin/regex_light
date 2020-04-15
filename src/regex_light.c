#include "regex_light.h"

int matchstar(int c, char *regexp, char *text);
int matchhere(char *regexp, char *text);

int
matchone(int p, int c)
{
  if (p == '.') return 1;
  return (p == c);
}

int
matchquestion(char *regexp, char *text)
{
  return (matchone(regexp[0], text[0]) && match(regexp + 2, text + 1)) || match(regexp + 2, text);
}

/* matchhere: search for regexp at beginning of text */
int
matchhere(char *regexp, char *text)
{
  if (regexp[0] == '\0')
    return 1;
  if (regexp[1] == '?')
    return matchquestion(regexp, text);
  if (regexp[1] == '*')
    return matchstar(regexp[0], regexp + 2, text);
  if (regexp[0] == '$' && regexp[1] == '\0')
    return *text == '\0';
  if (*text != '\0' && (regexp[0] == '.' || regexp[0] == *text))
    return matchhere(regexp + 1, text + 1);
  return 0;
}

/* matchstar: search for c*regexp at beginning of text */
int
matchstar(int c, char *regexp, char *text)
{
  char *t;
  for (t = text; *t != '\0' && (*t == c || c == '.'); t++)
    ;
  do {  /* * matches zero or more */
    if (matchhere(regexp, t))
      return 1;
  } while (t-- > text);
  return 0;
}

/* match: search for regexp anywhere in text */
int
match(char *regexp, char *text)
{
  if (regexp[0] == '^')
    return matchhere(regexp+1, text);
  do {    /* must look even if string is empty */
    if (matchhere(regexp, text))
      return 1;
  } while (*text++ != '\0');
  return 0;
}

