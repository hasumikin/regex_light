#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifdef USE_LIBC_REGEX
  #include <regex.h>
#else
  #include "src/regex_light.h"
#endif /* USE_LIBC_REGEX */

void
assert_match(char *regexp, char *text, int num, ...)
{
  va_list list;
  char *expect;
  char actual[100];
  char message[171];
  int i, j, k;

  regex_t preg;
  regcomp(&preg, regexp, REG_EXTENDED|REG_NEWLINE);
  regmatch_t pmatch[preg.re_nsub + 1]; // number of subexpression + 1

  char not[] = " NOT ";
  if (num > 0) not[1] = '\0';
  printf("\n(re_nsub: %d)<- /%s/ should%smatch \"%s\"\n", (int)preg.re_nsub, regexp, not, text);

  if (regexec(&preg, text, preg.re_nsub + 1, pmatch, 0) == 0) {
    if (num != 0) {
      fprintf(stdout, " \e[32;1msucceeded to match\e[m\n");
    } else {
      fprintf(stderr, " \e[31;1msucceeded to match\e[m\n");
    }

    if (num != preg.re_nsub + 1) {
      printf("\e[31;1m%s\e[m\n", "arg `num` does not correspond to preg.re_nsub");
    }

    va_start(list, num);
    for (i = 0; i < num; i++) {
      expect = va_arg(list, char*);
      k = 0;
      for (j = pmatch[i].rm_so; j < pmatch[i].rm_eo; j++) {
        actual[k] = text[j];
        k++;
      }
      actual[k] = '\0';
      sprintf(message, " %d --- expect: '%s', actual: '%s'", i, expect, actual);
      if (strcmp(expect, actual) == 0) {
        printf("\e[32;1m%s\e[m\n", message);
      } else {
        printf("\e[31;1m%s\e[m\n", message);
      }
    }
    va_end(list);
  } else {
    if (num == 0) {
      fprintf(stdout, " \e[32;1mfailed to match\e[m\n");
    } else {
      fprintf(stderr, " \e[31;1mfailed to match\e[m\n");
    }
  }

  regfree(&preg);
}

int
main(void)
{
  printf("\n");
  {
    assert_match("a", "a", 1, "a");
    assert_match("a", "b", 0);
    assert_match("a+b*c+", "abc", 1, "abc");
    assert_match("a+b*c+", "ac", 1, "ac");
    assert_match("a+b*c+", "bc", 0);
  }
    { /* + */
      assert_match(".+", "a", 1, "a");
      assert_match(".+", "aa", 1, "aa");
      assert_match("a+", "", 0);
      assert_match("a+", "a", 1, "a");
      assert_match("a+", "aaaa", 1, "aaaa");
      assert_match("a+", "", 0);
      assert_match("ab+c", "abbc", 1, "abbc");
      assert_match("ab+c", "ac", 0);
      assert_match("aa+", "a", 0);
    }
    { /* ? */
      assert_match("ab?c", "cb", 0);
      assert_match("ab?", "a", 1, "a");
      assert_match("ab?", "abc", 1, "ab");
      assert_match("ab?c", "abc", 1, "abc");
      assert_match("a?b?c?", "abc", 1, "abc");
      assert_match("a?b?c?", "bc", 1, "bc");
      assert_match("a?b?c?", "c", 1, "c");
      assert_match("a?b?c?", "ac", 1, "ac");
      assert_match("a?b?c?", "a", 1, "a");
      assert_match("a?b?c?", "ab", 1, "ab");
      assert_match("a?b?c?", "", 1, "");
      assert_match("a.?c", "abc", 1, "abc");
      assert_match("a.?c", "ac", 1, "ac");
      assert_match("aq?c", "aqqc", 0);
    }
    assert_match("ab", "abc", 1, "ab");
    assert_match("ab", "zabc", 1, "ab");
    assert_match("ab", "zab", 1, "ab");
    assert_match("^ab", "abc", 1, "ab");
    assert_match("^ab", "jc", 0);
    assert_match("a$", "bca", 1, "a");
    assert_match("a$", "ab", 0);
    assert_match("a*", "", 1, "");
    assert_match("^a$", "a", 1, "a");
    assert_match("^a$", "aa", 0);
    assert_match("^a$", "ab", 0);
    assert_match("^a$", "ba", 0);
    assert_match("ab*$", "abb", 1, "abb");
    assert_match("^ab*c$", "abc", 1, "abc");
    assert_match("^ab*c$", "abbbbbbc", 1, "abbbbbbc");
    assert_match(".", "a", 1, "a");
    assert_match("..", "a", 0);
    assert_match(".*", "aaaaaa", 1, "aaaaaa");
  { /* escape special character */
    assert_match("\\\\", "\\", 1, "\\");
    assert_match("\\(", "(", 1, "(");
    assert_match("\\[", "[", 1, "[");
    assert_match("\\$", "$", 1, "$");
    assert_match("\\^", "^", 1, "^");
    assert_match("\\.", ".", 1, ".");
    assert_match("\\+", "+", 1, "+");
    assert_match("a\\*z", "a*z", 1, "a*z");
    assert_match("a\\?z", "a?z", 1, "a?z");
    assert_match("a\\?z", "az", 0);
    assert_match("a\\\\", "a\\", 1, "a\\");
  }
  { /* [ ] */
    assert_match("[abc]", "a", 1, "a");
    assert_match("[abc]d", "a", 0);
    assert_match("[abc]", "b", 1, "b");
    assert_match("[abc]", "c", 1, "c");
    assert_match("[a-z]", "a", 1, "a");
    assert_match("[a-z]", "r", 1, "r");
    assert_match("[a-z]", "z", 1, "z");
    assert_match("[a-z]", "A", 0);
    assert_match("[a-zABC0-9]", "A", 1, "A");
    assert_match("[a-zABC0-9]", "D", 0);
    assert_match("[a-]", "-", 1, "-");
    assert_match("z[0-9]a", "6z8a9", 1, "z8a");
    assert_match("z[0-9]+a", "z1508a9", 1, "z1508a");
    assert_match("z[0-9]+[b-d]+a", "z1508ddcbda9", 1, "z1508ddcbda");
    assert_match("z[0-9]+&[b-d]+a", "z1508&ddcbda9", 1, "z1508&ddcbda");
    assert_match("z[0-9]*&[b-d]*a", "z1508&ddcbda9", 1, "z1508&ddcbda");
    assert_match("z[0-9]*&[b-d]*a", "z&a9", 1, "z&a");
    assert_match("z[0-9]*&[b-d]*a", "za9", 0);
    assert_match("z[B0-9AC]+[b-d]+a", "z1A5BC08ddcbda9", 1, "z1A5BC08ddcbda");
    assert_match("z[B0-9AC]+[b-d]+a", "z1A5RC08ddcbda9", 0);
  }
  { /* * */
    assert_match("ba*", "aba", 1, "ba");
    assert_match("a*", "aa", 1, "aa");
    assert_match("a*$", "baa", 1, "aa");
    assert_match("a*", "aabaa", 1, "aa");
    assert_match("a*", "aabaac", 1, "aa");
  }
  { /* ( ) */
    assert_match("a(b)c", "abcdefhello", 2, "abc", "b");
    assert_match("a(b)c", "adbc", 0);
    assert_match("a(bc)", "abcdefhello", 2, "abc", "bc");
    assert_match("a(bc)d", "abcdefhello", 2, "abcd", "bc");
    assert_match("a(bcd)ef", "abcdefhello", 2, "abcdef", "bcd");
    assert_match("a(bcd)ef(hello)", "abcdefhello", 3, "abcdefhello", "bcd", "hello");
    assert_match("a(b+)c", "ac", 0);
    assert_match("a(b+)c", "abc", 2, "abc", "b");
    assert_match("a(b+)c", "abbc", 2, "abbc", "bb");
    assert_match("a(b*)c", "ac", 2, "ac", "");
    assert_match("a(b*)c", "abc", 2, "abc", "b");
    assert_match("a(b*)c", "abbc", 2, "abbc", "bb");
    assert_match("a([0-9]+)c", "a123c", 2, "a123c", "123");
    assert_match("^(a)", "a123c", 2, "a", "a");
    assert_match("^(%[iIwWq][]-~!@#$%^&*()_=+\[{};:'\"?])", "%w[a v ", 2, "%w[", "%w[");
    assert_match("^(%[iIwWq][]-~!@#$%^&*()_=+\[{};:'\"?])", "%w]a v ", 2, "%w]", "%w]");
    assert_match("^(%[iIwWq][]-~!@#$%^&*()_=+\[{};:'\"?])", "%w^a v ", 2, "%w^", "%w^");
    assert_match("^(%[iIwWq][]-~!@#$%^&*()_=+\[{};:'\"?])", "%w?a v ", 2, "%w?", "%w?");
    assert_match("^(%[iIwWq][]-~!@#$%^&*()_=+\[{};:'\"?])", "%w\\a v ", 0);
    assert_match("a[]]b", "a]b", 1, "a]b");
    assert_match("a$", "zaza", 1, "a"); // this case can avoid backref bug
  }
  {
    assert_match("\\d+", "a9853_z", 1, "9853");
    assert_match("\\s+", "a \t\n\rz", 1, " \t\n\r");
    assert_match("\\w+", "_abcd0AZ9v-", 1, "_abcd0AZ9v");
  }
  { /* cases which have issue */
    assert_match("a$", "zaa", 1, "a"); // backref bug
    assert_match("((ab)cd)e", "abcde", 3, "abcde", "abcd", "ab"); // can not handle nested PAREN
    assert_match("(ab)?c", "c", 2, "c", ""); // ()? doesn't work
    assert_match("(ab)*c", "abababc", 2, "abababc", "ab"); // ()* doesn't work
    assert_match("(ab)+c", "abababc", 2, "abababc", "ab"); // ()+ doesn't work
    {
      assert_match("a?", "c", 1, ""); // 
      assert_match("a?", "cd", 1, ""); // ????????
      assert_match("a*", "bd", 1, "");
      assert_match("a*", "bad", 1, "");
      assert_match("a*", "baad", 1, "");
      assert_match("a*", "baaaaaad", 1, "");
      assert_match("^a?", "c", 1, ""); // adding ^ can avoid the problem above
    }
  }
 // assert_match("", "", 1, "");
  printf("\n");
  return 0;
}
