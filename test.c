#include <stdio.h>
#include <stdbool.h>
#include "src/regex_light.h"

void
assert_match(char *regexp, char *text, bool expectation)
{
  int result = match(regexp, text);
  if (result == 1) {
    expectation ? fprintf(stdout, "\e[32;1msuccess\e[m") : fprintf(stderr, "\e[31;1mfail   \e[m");
  } else {
    !expectation ? fprintf(stdout, "\e[32;1msuccess\e[m") : fprintf(stderr, "\e[31;1mfail   \e[m");
  }
  char not[] = " NOT ";
  if (expectation) not[1] = '\0';
  printf(" <- /%s/ should%smatch \"%s\"\n", regexp, not, text);
}

int
main(void)
{
  printf("\n");
  assert_match("a?", "c", true);
  assert_match("ab?c", "cb", false);
  assert_match("ab?", "a", true);
  assert_match("ab?", "abc", true);
  assert_match("ab?c", "abc", true);
  assert_match("a?b?c?", "abc", true);
  assert_match("a?b?c?", "bc", true);
  assert_match("a?b?c?", "c", true);
  assert_match("a?b?c?", "ac", true);
  assert_match("a?b?c?", "a", true);
  assert_match("a?b?c?", "ab", true);
  assert_match("a?b?c?", "", true);
  assert_match("a.?c", "abc", true);
  assert_match("a.?c", "ac", true);
  assert_match("ab", "abc", true);
  assert_match("ab", "zabc", true);
  assert_match("ab", "zab", true);
  assert_match("^ab", "abc", true);
  assert_match("^ab", "jc", false);
  assert_match("a$", "abca", true);
  assert_match("a*", "bd", true);
  assert_match("a*", "bad", true);
  assert_match("a*", "baad", true);
  assert_match("a*", "baaaaaad", true);
  assert_match("^a$", "a", true);
  assert_match("^a$", "aa", false);
  assert_match("^a$", "ab", false);
  assert_match("^a$", "ba", false);
  assert_match("ab*$", "abb", true);
  assert_match("^ab*c$", "abc", true);
  assert_match("^ab*c$", "abbbbbbc", true);
  assert_match(".", "a", true);
  assert_match("..", "a", false);
  assert_match(".*", "aaaaaa", true);
 // assert_match("", "", true);
  printf("\n");
  return 0;
}
