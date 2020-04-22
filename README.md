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
- any literal charactor
- `.` ... any single charactor
- `^` ... beginning of the input
- `$` ... end of the input
- `*` ... zero or more of previous charactor
- `+` ... one or more of previous charactor
- `?` ... zero or one of previous charactor
- `[-]` ... specified charactors, between the two charactors
- `()` ... group for backward reference in regmatch_t
- `\.` `\^` `\$` `\*` `\+` `\?` `\[` `\(` ... escape special charactors treating them literals

### Expressions which don't work
- `()*` `()+` `()?` ... but I want to make them work
- `(|)` ... group OR group
- `\1` `\2` ... backreference in pattern

### Known issues
#### matched-index in regmatch_t works incompletely
- eg) `a$` for `"aba"` will reports `{rm_so: 0, rm_eo:3}`, though it should be `{rm_so: 2, rm_eo:3}`

#### Furthermore, there may exist many edge cases which don't work as we expect
