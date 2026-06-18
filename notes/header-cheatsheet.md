# C23 Header Cheatsheet

Headers you'll actually use, with C23 notes where things changed.

## I/O and Strings

| Header | Key Functions | Notes |
|--------|--------------|-------|
| `<stdio.h>` | `printf` `fprintf` `snprintf` `fopen` `fclose` `fgets` `scanf` `perror` | All file and console I/O. `snprintf` > `sprintf` always. |
| `<string.h>` | `strlen` `strcmp` `strcpy` `strcat` `memset` `memcpy` `memmove` | String ops and raw memory. Prefer `strncpy`/`snprintf` over `strcpy`/`sprintf`. |
| `<strings.h>` | `strcasecmp` `strncasecmp` | Case-insensitive comparison. POSIX only, not in `<string.h>`. |

## Memory and Errors

| Header | Key Functions | Notes |
|--------|--------------|-------|
| `<stdlib.h>` | `malloc` `calloc` `realloc` `free` `exit` `atoi` `strtol` `rand` `abs` | The catch-all. `strtol` > `atoi` (detects errors). |
| `<errno.h>` | `errno` `ENOMEM` `EINVAL` `ENOENT` | Set by library calls on failure. Check immediately after the call — next call may overwrite it. |

## Types and Limits

| Header | Key Types / Macros | Notes |
|--------|--------------------|-------|
| `<stdint.h>` | `int32_t` `uint64_t` `size_t` `intptr_t` `UINT64_MAX` | Fixed-width integers. Use these over `long`/`long long` when size matters. |
| `<stddef.h>` | `size_t` `nullptr_t` `offsetof` | Usually pulled in implicitly. `nullptr_t` is new in C23. |
| `<limits.h>` | `INT_MAX` `INT_MIN` `CHAR_BIT` `PATH_MAX` | Min/max for built-in types. |

## C23 Makes These Headers Obsolete

These are still valid `#include`s but C23 provides the same thing as keywords:

| Header | What It Gave | C23 Replacement |
|--------|-------------|-----------------|
| `<stdbool.h>` | `bool` `true` `false` | Built-in keywords now. Don't need the include. |
| `<stdnoreturn.h>` | `noreturn` | `noreturn` is a keyword now. Don't need the include. |
| `<assert.h>` (for `static_assert`) | `static_assert(cond, msg)` | `static_assert` is a keyword now. `assert()` still needs the header. |

## POSIX Only (not standard C — won't port to Windows easily)

| Header | Key Functions | Notes |
|--------|--------------|-------|
| `<unistd.h>` | `read` `write` `close` `getpid` `chdir` `execve` `fork` `access` `isatty` | Core POSIX API. Every Linux C program touches this. |
| `<fcntl.h>` | `open` `creat` `O_RDONLY` `O_CREAT` `O_WRONLY` `O_RDWR` | Low-level file control. `open()` flags live here. |
| `<sys/stat.h>` | `stat` `mkdir` `chmod` `S_ISDIR` `S_ISREG` | File metadata and permissions. `stat` fills a struct, check with `S_IS*` macros. |
| `<dirent.h>` | `opendir` `readdir` `closedir` `rewinddir` | Directory traversal. `readdir` returns `.` and `..` — skip them. |
| `<pwd.h>` | `getpwuid` `getpwnam` | User info from `/etc/passwd`. |
| `<grp.h>` | `getgrgid` `getgrnam` | Group info from `/etc/group`. |
| `<signal.h>` | `signal` `raise` `SIGINT` `SIGTERM` `SIGKILL` | Signal handling. `SIGKILL` can't be caught. |

## Quick Inclusion Patterns

Minimal CLI tool:
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
```

Filesystem tool (POSIX):
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
```

System-level tool:
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>
```