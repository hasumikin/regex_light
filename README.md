## Regex Light

### $Lang
- C.ASCII

### Types
- regex_t
- regmatch_t

### Functions
- regcomp() # the 3rd arg will be ignored
- regexec()
- regfree()

### Expressions
- any literal character
- `.` ... any single character
- `^` ... beginning of the input
- `$` ... end of the input
- `*` ... zero or more of previous character
- `+` ... one or more of previous character
- `?` ... zero or one of previous character
- `[-]` ... specified characters, between the two characters
- `()` ... group for backward reference in regmatch_t
- `\.` `\^` `\$` `\*` `\+` `\?` `\[` `\(` ... escape special characters treating them literals

### Expressions which don't work
- `()*` `()+` `()?` ... but I want to make them work
- `(|)` ... group OR group
- `\1` `\2` ... backreference in pattern

### Known issues
```
assert_match("[-a]", "-", 1, "-");
assert_match("[a-]", "-", 1, "-");
assert_match("((ab)cd)e", "abcde", 3, "abcde", "abcd", "ab"); // can not handle nested PAREN
assert_match("(ab)?c", "c", 2, "c", ""); // ()? doesn't work
assert_match("(ab)*c", "abababc", 2, "abababc", "ab"); // ()* doesn't work
assert_match("(ab)+c", "abababc", 2, "abababc", "ab"); // ()+ doesn't work
```
