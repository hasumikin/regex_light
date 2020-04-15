## Regex Light

### $Lang only
- C.ASCII

### Types only
- regex_t
- regmatch_t

### Functions only
- regcomp() # the 3rd arg will be ignored
- regexec()
- regfree()

### Expressions only
- any literal charactor
- `.` ... any single charactor
- `^` ... beginning of the input
- `$` ... end of the input
- `*` ... zero or more of previous charactor
- `+` ... one or more of previous charactor
- `?` ... zero or one of previous charactor
- `[-]` ... specified charactors, between the two charactors
- `()` ... group for backward reference. subsequent repetitive expressions like `*` are not supported

### [TBD] Expressions possibly
- `(|)` ... group OR group
