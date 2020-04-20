#include <stdio.h>
#include <stdbool.h>
#include "src/regex_light.h"

void
assert_match(char *regexp, char *text, bool expectation)
{
  regex_t preg;
  regcomp(&preg, regexp, 0);
  regmatch_t match[preg.re_nsub + 1]; // number of subexpression + 1
  int size = sizeof(match) / sizeof(regmatch_t);

  int result = regexec(&preg, text, size, match, 0);

  if (result == 1) {
    expectation ? fprintf(stdout, "\n\e[32;1msuccess\e[m") : fprintf(stderr, "\e[31;1mfail   \e[m");
  } else {
    !expectation ? fprintf(stdout, "\n\e[32;1msuccess\e[m") : fprintf(stderr, "\e[31;1mfail   \e[m");
  }
  char not[] = " NOT ";
  if (expectation) not[1] = '\0';
  printf(" (re_nsub: %d)<- /%s/ should%smatch \"%s\"\n", (int)preg.re_nsub, regexp, not, text);
}

int
main(void)
{
  printf("\n");
  {
    assert_match("a", "a", true);
    assert_match("a", "b", false);
    assert_match("a+b*c+", "abc", true);
    assert_match("a+b*c+", "ac", true);
    assert_match("a+b*c+", "bc", false);
    { /* + */
      assert_match(".+", "a", true);
      assert_match(".+", "aa", true);
      assert_match("a+", "", false);
      assert_match("a+", "a", true);
      assert_match("a+", "aaaa", true);
      assert_match("a+", "", false);
      assert_match("ab+c", "abbc", true);
      assert_match("ab+c", "ac", false);
      assert_match("aa+", "a", false);
    }
    { /* ? */
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
      assert_match("aq?c", "aqqc", false);
    }
    assert_match("ab", "abc", true);
    assert_match("ab", "zabc", true);
    assert_match("ab", "zab", true);
    assert_match("^ab", "abc", true);
    assert_match("^ab", "jc", false);
    assert_match("a$", "bca", true);
    assert_match("a$", "ab", false);
    assert_match("a*", "", true);
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
  }
  { /* escape special charactor */
    assert_match("\\\\", "\\", true);
    assert_match("\\(", "(", true);
    assert_match("\\[", "[", true);
    assert_match("\\$", "$", true);
    assert_match("\\^", "^", true);
    assert_match("\\.", ".", true);
    assert_match("\\+", "+", true);
    assert_match("a\\*z", "a*z", true);
    assert_match("a\\?z", "a?z", true);
    assert_match("a\\?z", "az", false);
    assert_match("a\\", "a\\", true);
  }
  { /* [ ] */
    assert_match("[abc]", "a", true);
    assert_match("[abc]d", "a", false);
    assert_match("[abc]", "b", true);
    assert_match("[abc]", "c", true);
    assert_match("[a-z]", "a", true);
    assert_match("[a-z]", "r", true);
    assert_match("[a-z]", "z", true);
    assert_match("[a-z]", "A", false);
    assert_match("[a-zABC0-9]", "A", true);
    assert_match("[a-zABC0-9]", "D", false);
    assert_match("[a-]", "-", true);
    assert_match("z[0-9]a", "6z8a9", true);
    assert_match("z[0-9]+a", "z1508a9", true);
    assert_match("z[0-9]+[b-d]+a", "z1508ddcbda9", true);
    assert_match("z[0-9]+&[b-d]+a", "z1508&ddcbda9", true);
    assert_match("z[0-9]*&[b-d]*a", "z1508&ddcbda9", true);
    assert_match("z[0-9]*&[b-d]*a", "z&a9", true);
    assert_match("z[0-9]*&[b-d]*a", "za9", false);
    assert_match("z[B0-9AC]+[b-d]+a", "z1A5BC08ddcbda9", true);
    assert_match("z[B0-9AC]+[b-d]+a", "z1A5RC08ddcbda9", false);
  }
  { /* * */
    assert_match("ba*", "aba", true);
    assert_match("a*", "aa", true);
    assert_match("a*", "baa", true);
    assert_match("a*$", "baa", true);
    assert_match("a*", "aabaa", true);
    assert_match("a*", "aabaac", true);
  }
  { /* ( ) */
    assert_match("a(b)c", "abcdefhello", true);
    assert_match("a(b)c", "adbc", false);
    assert_match("a(bc)", "abcdefhello", true);
    assert_match("a(bc)d", "abcdefhello", true);
    assert_match("a(bcd)ef", "abcdefhello", true);
    assert_match("a(bcd)ef(hello)", "abcdefhello", true);
    assert_match("a(b+)c", "ac", false);
    assert_match("a(b+)c", "abc", true);
    assert_match("a(b+)c", "abbc", true);
    assert_match("a(b*)c", "ac", true);
    assert_match("a(b*)c", "abc", true);
    assert_match("a(b*)c", "abbc", true);
    assert_match("a([0-9]+)c", "a123c", true);
    assert_match("^(a)", "a123c", true);
    assert_match("^(%[iIwWq][\\]-~!@#$%^&*()_=+\[{};:'\"?])", "%w[a v ", true);
    assert_match("^(%[iIwWq][\\]-~!@#$%^&*()_=+\[{};:'\"?])", "%w]a v ", true);
    assert_match("^(%[iIwWq][\\]-~!@#$%^&*()_=+\[{};:'\"?])", "%w^a v ", true);
    assert_match("^(%[iIwWq][\\]-~!@#$%^&*()_=+\[{};:'\"?])", "%w?a v ", true);
    assert_match("^(%[iIwWq][\\]-~!@#$%^&*()_=+\[{};:'\"?])", "%w\\a v ", false);
  }
  { /* cases which have issue */
    assert_match("a$", "zaa", true); // REPORTing bug
    assert_match("((ab)cd)e", "abcde", true); // can not handle nested PAREN
    assert_match("(ab)?c", "c", true); // ()? doesn't work
    assert_match("(ab)*c", "abababc", true); // ()* doesn't work
    assert_match("(ab)+c", "abababc", true); // ()+ doesn't work
  }
 // assert_match("", "", true);
  printf("\n");
  return 0;
}
