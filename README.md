[![C/C++ CI](https://github.com/hasumikin/regex_light/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/hasumikin/regex_light/actions/workflows/c-cpp.yml)

## Regex Light

### Acknowledgement

The implementation of regex_light started from this great article:
[https://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html](https://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html)

### Features
- `()`: You can make parenthesized groups for backward reference, including nested groups and quantifiers (`?`, `*`, `+`).
- Character class (`[]`) literal hyphens (e.g., `[-a]` or `[a-]`) are now correctly handled.
- Small and fast
- Portablity: Similar API to stdlib's regex

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
- `(|)` ... group OR group
- `\1` `\2` ... backreference in pattern


