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
- `()` ... group for backward reference. subsequent repetitive expressions like `*` are not supported
- `\.` `\^` `\$` `\*` `\+` `\?` `\[` `\(` ... escape special charactors treating them literals

### [TBD] Expressions possibly
- `(|)` ... group OR group
