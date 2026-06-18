# C Footguns

C trusts you completely. That trust is almost always misplaced.

---

## Memory Management

### Use After Free

Accessing memory after `free()`. Might crash, might silently corrupt.

```c
int *p = malloc(sizeof(int));
free(p);
*p = 42;         // undefined behavior
```

Fix: null the pointer after free.

```c
free(p);
p = nullptr;
```

### Double Free

```c
free(p);
free(p);         // heap corruption, likely crash
```

Null-after-free also prevents this — `free(nullptr)` is a no-op.

### Memory Leaks

Every `malloc` needs a matching `free` on every exit path — `return`, `break`, error handling.

```c
int *data = malloc(n * sizeof(int));
if (!data) return -1;

int *extra = malloc(m * sizeof(int));
if (!extra) {
    free(data);     // don't forget this
    return -1;
}
// ...
free(data);
free(extra);
```

This is where `goto cleanup` patterns come from — centralized error cleanup.

### Wrong Allocator Size

`sizeof` on the wrong type.

```c
int *arr = malloc(n * sizeof(int *));    // wrong — pointer size, not int size
int *arr = malloc(n * sizeof(*arr));     // right — robust to type changes
```

If you change `int *arr` to `long *arr`, the first version silently allocates the wrong amount on platforms where `int *` and `int` differ in size. The second version always works.

### Realloc Losing the Pointer

If `realloc` fails, the original pointer is still valid — but you've overwritten it with NULL and leaked the memory.

```c
p = realloc(p, new_size);     // if realloc returns NULL, original p is leaked

// Correct:
void *tmp = realloc(p, new_size);
if (!tmp) { free(p); return -1; }
p = tmp;
```

### Stack Allocation of Large Buffers

```c
void process(void) {
    char buf[1048576];    // 1MB on the stack — likely a crash
}
```

Anything over ~8-16KB should be `malloc`'d.

### Zero-Length Allocations

`malloc(0)` is implementation-defined. It may return NULL or a unique pointer you can't dereference. Always check your size before calling malloc.

---

## Strings

### No Bounds Checking by Default

`strcpy`, `strcat`, `sprintf` don't know the destination size.

```c
char buf[32];
strcpy(buf, user_input);        // buffer overflow if > 31 chars
strcat(buf, more_data);         // same, appends to whatever's there
sprintf(buf, "%s:%d", name, id); // same

// Use the bounded versions
snprintf(buf, sizeof(buf), "%s:%d", name, id);
strncpy(buf, src, sizeof(buf) - 1); buf[sizeof(buf) - 1] = '\0';
```

Note: `strncpy` doesn't null-terminate if the source is longer than `n`. Always set the last byte.

### Off-by-One in Null Terminator

C strings need space for `\0`.

```c
char name[5] = "hello";    // truncates — no room for null terminator
char name[6] = "hello";    // correct — 5 chars + 1 null
```

### strlen Doesn't Count the Null

But `malloc` needs space for it.

```c
char *copy = malloc(strlen(s));      // wrong — no room for '\0'
char *copy = malloc(strlen(s) + 1);  // correct
```

### Comparing Strings with ==

Compares pointers, not contents.

```c
if (s == "hello")    // compares addresses, almost always false

if (strcmp(s, "hello") == 0)  // correct — compares contents
```

### strtok Is Not Reentrant

Uses a static internal pointer. Can't nest calls, not thread-safe.

```c
// strtok_r is the reentrant version (POSIX)
char *save;
char *tok = strtok_r(buf, " ", &save);
```

### Modifying String Literals

Undefined behavior.

```c
char *s = "hello";
s[0] = 'H';           // crash — string literals may be in read-only memory

char s[] = "hello";
s[0] = 'H';           // fine — s is a stack copy
```

---

## Signed/Unsigned Mismatches

### The Infinite Loop

`size_t` is unsigned. Decrementing past 0 wraps to `SIZE_MAX`.

```c
for (size_t i = n - 1; i >= 0; i--) {   // i >= 0 is always true — infinite loop
}

// Fix: use the underflow behavior
for (size_t i = n; i-- > 0; ) {   // decrements, then checks > 0
}
```

### Negative Values Becoming Huge

Implicit signed-to-unsigned conversion.

```c
ssize_t len = read(fd, buf, sizeof(buf));
if (len < 0) { /* handle error */ }

size_t cap = 1024;
if (len > cap) { }    // if len == -1, it becomes SIZE_MAX — true!
```

### Array Index Underflow

```c
int arr[10];
int idx = -1;
arr[idx];    // arr + (size_t)-1 — accesses way out of bounds
```

`-Wconversion` (in compile flags) catches most of these.

---

## Pointers

### Dangling Pointers

Pointer to local variable that goes out of scope.

```c
int *get_thing(void) {
    int x = 42;
    return &x;     // x is on the stack — gone after return
}
```

### Pointer Arithmetic Scale

Pointer math scales by `sizeof(*p)` automatically.

```c
int *p = malloc(10 * sizeof(int));
int *q = p + 5;       // p + 5 * sizeof(int), not p + 5 bytes
memset(p, 0, 10);      // WRONG — 10 bytes, not 10 ints
memset(p, 0, 10 * sizeof(int));  // correct
memset(p, 0, 10 * sizeof(*p));    // also correct, robust to type changes
```

### Array Decay in Function Parameters

`sizeof` gives pointer size, not array size.

```c
void process(int arr[]) {           // arr is really int *
    size_t n = sizeof(arr);         // sizeof(int *) — probably 8, not the array
}

// Fix: pass the size
void process(int *arr, size_t n) {
    // ...
}
```

### Aliasing Violations (Strict Aliasing)

Accessing memory through an incompatible pointer type is undefined behavior.

```c
float f = 1.0f;
int *ip = (int *)&f;    // strict aliasing violation
*ip;                     // UB — compiler may optimize assuming f wasn't modified

// Correct way to reinterpret bits:
int i;
memcpy(&i, &f, sizeof(i));    // type-punning via memcpy is defined
```

Exception: `char *` can alias anything. That's why `memcpy` works — it uses `char *` internally.

---

## Undefined Behavior

The big ones people hit:

| UB | Example | What happens |
|---|---|---|
| Buffer overflow | `arr[5]` with `int arr[3]` | Silent corruption, crashes, exploitable |
| Use after free | `*p` after `free(p)` | Works until it doesn't. Heisenbug. |
| Null dereference | `*p` where `p == NULL` | Crash on most platforms, but compiler can assume it never happens and optimize around it |
| Signed integer overflow | `INT_MAX + 1` | Wraps on most hardware, but compiler can assume it never happens and delete overflow checks |
| Signed shift | `1 << 31` (for 32-bit int) | UB — may shift into sign bit |
| Division by zero | `x / 0` | Crash or nothing, depends on platform |
| Uninitialized reads | `int x; if (x > 0)` | Garbage value. Compiler may optimize checks away |
| Missing return | non-void function falls off | May "work" in debug, crash in release |

### Signed Overflow Is the Sneaky One

The compiler can assume it never happens, so it deletes overflow checks:

```c
// This check gets optimized away in -O2:
if (x + y < x) { /* overflow happened */ }
// Compiler: "x + y can't overflow because that's UB, and UB doesn't happen.
//            Therefore x + y >= x, so this branch is unreachable. Deleted."
```

Fix: check before the operation.

```c
if (x > INT_MAX - y) { /* would overflow */ }
```

---

## POSIX / System Programming

### Not Checking Return Values

Every syscall can fail.

```c
open(path, O_RDONLY);       // can return -1
malloc(n);                    // can return NULL
read(fd, buf, sizeof(buf));  // can return -1, can return less than sizeof(buf)
close(fd);                    // can return -1 (and leak the fd)
```

### Partial Reads

`read()` may return fewer bytes than requested.

```c
// Wrong — may read fewer bytes than requested
ssize_t n = read(fd, buf, sizeof(buf));

// Right — loop until done or error
ssize_t total = 0;
while (total < sizeof(buf)) {
    ssize_t n = read(fd, (char *)buf + total, sizeof(buf) - total);
    if (n <= 0) break;
    total += n;
}
```

### File Descriptor Leaks

Forgetting to close on error paths.

```c
int fd = open(path, O_RDONLY);
if (fd < 0) return -1;

int other_fd = open(other, O_RDONLY);
if (other_fd < 0) {
    close(fd);     // don't forget
    return -1;
}
// ...
close(fd);
close(other_fd);
```

### errno Not Reset Before Call

`errno` is only meaningful after a function that's documented to set it, and only if it returned an error.

```c
errno = 0;                    // clear before the call
int result = strtol(input, &end, 10);
if (errno != 0) {             // check errno only if result indicates error
    // handle error
}
```

### Signal Handlers — Only Async-Signal-Safe Functions

`printf`, `malloc`, and most libc functions are NOT async-signal-safe.

```c
void handler(int sig) {
    printf("got signal\n");   // undefined behavior
    malloc(64);                // undefined behavior
}

// Only async-signal-safe functions are allowed:
// write, read, _exit, signal, etc.
void handler(int sig) {
    write(STDOUT_FILENO, "got signal\n", 12);  // ok
}
```

### fork() Without exec() in Multithreaded Programs

Only the calling thread survives, but mutexes from other threads may be held, causing deadlock.

```c
// In a multithreaded program, after fork():
// - Only the forking thread exists in the child
// - Any mutex held by another thread is locked forever
// - Only async-signal-safe functions are safe to call

// Fix: use exec() immediately after fork() in multithreaded programs
// Or use pthread_atfork() to prepare mutex state
```

### TOCTOU (Time-of-Check to Time-of-Use)

Checking a condition then acting on it, where the condition can change between check and use.

```c
if (access(path, F_OK) == 0) {
    // file might be deleted or swapped here
    int fd = open(path, O_RDONLY);   // may fail or open wrong file
}

// Fix: open first, then check with fstat if needed
int fd = open(path, O_RDONLY);
if (fd < 0) { /* doesn't exist or can't open */ }
```

---

## Concurrency

### Data Races

Unsynchronized access to shared data from multiple threads.

```c
int counter;   // shared between threads

// Thread 1
counter++;     // data race — not atomic

// Thread 2
counter++;     // data race — not atomic
```

Fix: use `stdatomic.h` (C11):

```c
#include <stdatomic.h>
atomic_int counter;

atomic_fetch_add(&counter, 1);   // atomic increment
```

---

## Preprocessor

### Macro Operator Precedence

Missing parentheses.

```c
#define DOUBLE(x) x * 2
DOUBLE(3 + 4);     // → 3 + 4 * 2 = 11, not 14

#define DOUBLE(x) ((x) * 2)
DOUBLE(3 + 4);     // → ((3 + 4) * 2) = 14
```

Always parenthesize macro arguments and the whole expression.

### Double Evaluation

Macro arguments evaluated multiple times.

```c
#define MAX(a, b) ((a) > (b) ? (a) : (b))
int i = 1;
MAX(i++, 5);       // i incremented twice if it's the max

// C23 fix with typeof:
#define MAX(a, b) ({ \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a > _b ? _a : _b; \
})
```

### Semicolons After Macros

Multi-statement macros need `do/while(0)`.

```c
#define SWAP(a, b) do { typeof(a) _tmp = (a); (a) = (b); (b) = _tmp; } while(0)

if (x)
    SWAP(a, b);     // works
else
    SWAP(c, d);     // works
```

Without `do/while(0)`, the `if` grabs only the first statement and the `else` is orphaned.

---

## Initialization and Declarations

### Uninitialized Variables

Contain garbage, not zero.

```c
int x;                 // garbage value
printf("%d\n", x);     // undefined behavior

int arr[10];           // all elements are garbage
static int y;           // zero — static/global vars are zero-initialized
```

### Struct Zero-Initialization

```c
struct point p1 = { .x = 1, .y = 2 };   // unspecified fields are 0
struct point p2 = { 0 };                  // zero-initialize all fields
struct point p3;                          // garbage — uninitialized
```

### Mixed Declarations and Code

Fine in C99+ and C23, but not in C89.

```c
int x = 5;
printf("%d\n", x);
int y = 10;        // fine in C99+/C23, error in C89
```

---

## Type System

### char Signedness Is Implementation-Defined

On some platforms `char` is signed, on others unsigned.

```c
char c = 200;          // might be -56 or 200, depending on platform
unsigned char uc = 200; // always 200
signed char sc = 200;   // overflow — implementation-defined / UB

// Use unsigned char for raw bytes, signed char for text where sign matters
```

### sizeof on Array Parameter

Arrays decay to pointers in function parameters.

```c
void foo(int arr[10]) {
    sizeof(arr);    // sizeof(int *) — 8, not 40
}
```

### Function Pointer Syntax

```c
int (*fp)(int, int);              // pointer to function
fp = &add;                        // & is optional
fp = add;                         // same thing
int result = fp(1, 2);           // call via pointer
int result = (*fp)(1, 2);         // also valid, more explicit
```

---

## The Big Picture

Your compile flags catch a lot of this:

- `-Wall -Wconversion -Wformat=2 -Wimplicit-fallthrough` — static warnings
- `-Werror=format-security` — format string vulnerabilities
- `-fstack-protector-strong -fstack-clash-protection` — stack smashing
- `-D_FORTIFY_SOURCE=3` — runtime buffer overflow checks in libc
- `-Wl,-z,relro,-z,now` — GOT/PLT hardening

At debug time, add:

```
-fsanitize=address,undefined
```

Catches use-after-free, buffer overflows, signed overflow, null derefs, and more — at runtime, with diagnostics. Don't ship with it (it's slow), but run it constantly during development.