# C23 Features Cheatsheet

Quick reference for C23 language features and standard library additions.
Using `-std=c23` with clang 21+.

---

## New Keywords

### `nullptr`

Type-safe null pointer. Replaces `NULL`.

```c
int *p = nullptr;          // type: nullptr_t, converts to any pointer type

// Why it matters:
func(NULL);                 // may match an int overload / _Generic branch
func(nullptr);              // always matches pointer — correct

int x = NULL;              // compiles — silently assigns 0 (bug!)
int y = nullptr;            // error — can't convert nullptr_t to int
```

Use `nullptr` everywhere in C23. Only use `NULL` for pre-C23 compat.

### `auto`

Type inference from initializer. Same keyword as C++, C rules.

```c
auto i = 42;               // int
auto d = 3.14;              // double
auto fp = fopen("f", "r"); // FILE *
auto len = strlen(s);       // size_t

// Can't use without initializer
auto x;                     // error
static auto y = 1;          // error — can't be static/extern

// Good — obvious from right side
auto dir = opendir("/tmp");
auto p = (struct sockaddr_in *)&storage;

// Bad — reader can't tell the type
auto v = parse_value(input);  // what type is this?
```

Prefer `auto` when the type is obvious. Spell it out when the type carries meaning.

### `typeof` / `typeof_unqual`

Get the type of an expression. Makes macros safer.

```c
int x = 5;
typeof(x) y = 10;              // int y = 10;
typeof_unqual(x) z = 20;       // int z = 20; (strips cvr-qualifiers: const/volatile/restrict)

// Safer MAX macro — single evaluation
#define MAX(a, b) ({             \
    typeof(a) _a = (a);         \
    typeof(b) _b = (b);         \
    _a > _b ? _a : _b;          \
})
```

`typeof_unqual` strips `const`/`volatile`/`restrict` — useful when you want a mutable copy of a const expression's type.

### `bool` / `true` / `false` / `noreturn` / `static_assert`

These are now keywords, not macros from headers.

```c
// No more #include <stdbool.h>
bool flag = true;

// No more #include <stdnoreturn.h>
noreturn void die(const char *msg);

// static_assert without <assert.h>
static_assert(sizeof(int) == 4, "int must be 4 bytes");

// Also works without message (C23)
static_assert(sizeof(int) == 4);
```

### `constexpr`

Type-safe constants. Replaces `#define` for numeric constants.

```c
constexpr int max_buf = 4096;
constexpr double pi = 3.14159;

char buf[max_buf];             // usable in array sizes
switch (x) { case max_buf: }  // usable in switch cases
```

Over `#define`: scoped, typed, visible in debugger, no parenthesization traps.

### Binary literals

```c
int mask = 0b1010;             // 10
int flags = 0b11110000;        // 240
```

No more `0xFF` when you mean bit patterns.

---

## Enums

### Fixed underlying type

```c
// Classic — underlying type is int, always 4 bytes
enum color { RED, GREEN, BLUE };

// C23 — pick the size
enum color : unsigned char { RED, GREEN, BLUE };     // 1 byte
enum status : uint8_t { OK = 0, ERR = 1, FATAL = 255 };
enum big_tag : _BitInt(24) { A, B, C };              // 3 bytes
```

Controls size, signedness, and ABI layout. Essential for structs, file formats, network protocols.

### Other C23 enum changes

- Enum values can be any integer constant expression, not just `int` range
- Enums with fixed type (`: type`) are complete types at the closing `}`
- Without `: type`, still `int` — values outside `int` range are undefined

### Gotchas

- Enum constants have **no scoping** — `RED` collides with any other `RED`. Prefix values: `COLOR_RED`
- No C equivalent of C++ `enum class`
- Fixed-type enums are distinct from their underlying type — not interchangeable

---

## Attributes `[[...]]`

Standardized syntax replacing `__attribute__((...))` (GCC/Clang) and `__declspec(...)` (MSVC).

### `[[noreturn]]`

```c
[[noreturn]] void die(const char *msg) {
    fprintf(stderr, "fatal: %s\n", msg);
    exit(1);
}
```

Same as old `_Noreturn` keyword. `[[noreturn]]` is preferred syntax.

### `[[deprecated]]`

```c
[[deprecated("use process_v2 instead")]]
void process(int fd);

typedef int [[deprecated("use size_t instead")]] old_size;

struct config {
    int port;
    [[deprecated]] int old_flag;
};
```

Compiler warns on any use. String is optional.

### `[[fallthrough]]`

```c
switch (cmd) {
    case 'Y':
        [[fallthrough]];
    case 'y':
        return 1;
    default:
        return 0;
}
```

Suppresses `-Wimplicit-fallthrough` warning. Documents intentional fallthrough.

### `[[maybe_unused]]`

```c
int doit(int x, [[maybe_unused]] int y) {
    return x + 1;     // no "unused parameter" warning
}

[[maybe_unused]] static const char *tag = "v1.0";
```

### `[[nodiscard]]`

```c
[[nodiscard]] int open_file(const char *path);
[[nodiscard]] bool init(void);

open_file("data.txt");    // warning: ignoring return value
```

Don't let callers silently discard error codes or resource handles.

### `[[unsequenced]]` and `[[reproducible]]`

```c
// Pure function — no globals, no side effects, same input → same output
[[unsequenced]] int square(int x) { return x * x; }

// Deterministic — may read globals, but same input → same output
[[reproducible]] int clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
```

`[[unsequenced]]` is the stronger guarantee. Compiler can cache and reorder calls freely.

### Still needs `__attribute__`

Some attributes have no standard `[[ ]]` form yet:

```c
__attribute__((format(printf, 1, 2)))     // format string checking
__attribute__((aligned(64)))               // alignment (alignas covers some)
__attribute__((packed))                    // packed structs
```

---

## `memset_explicit`

Zeroes memory that the compiler **cannot optimize away**.

```c
char password[64];
fgets(password, sizeof(password), stdin);
verify(password);
memset_explicit(password, 0, sizeof(password));  // guaranteed to happen
```

Regular `memset` can be eliminated as a dead store if the buffer is never read again. `memset_explicit` forbids that optimization — every byte is written, in order, non-removable.

**Use when:** clearing passwords, keys, tokens, decrypted data, any sensitive buffer.
**Don't use when:** normal initialization or resetting a buffer you'll use again — just `memset`.

Pre-C23 alternatives: `explicit_bzero()` (Linux/BSD), `SecureZeroMemory()` (Windows), or manual volatile writes.

---

## `#embed`

Embed binary files directly — no more `xxd -i` or `incbin` hacks.

```c
const unsigned char icon[] = {
    #embed "icon.png"
};

// With size limit
const unsigned char font[] = {
    #embed "font.bin" limit(4096)
};

// Use in initializers
struct resource res = {
    .size = sizeof((unsigned char[]){ #embed "data.bin" }),
    .data = &(unsigned char){ #embed "data.bin" },
};
```

---

## `__VA_OPT__`

Standardized (was a GNU extension). Handles the comma in variadic macros when `__VA_ARGS__` is empty.

```c
// Without __VA_OPT__ — extra comma when empty
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)
LOG("done");              // → printf("done",) — broken!

// With __VA_OPT__ — comma included only when args exist
#define LOG(fmt, ...) printf(fmt __VA_OPT__(,) __VA_ARGS__)
LOG("done");              // → printf("done");
LOG("val: %d", 42);       // → printf("val: %d", 42);
```

---

## Integer Type Interactions

Mixing `stdint.h` types works — implicit promotions handle it. But signed/unsigned of same width is a footgun.

### Rules

1. **Integer promotion**: types smaller than `int` → `int`
2. **Usual arithmetic conversion**: narrower → wider; if same width, signed → unsigned

### Safe mixes

```c
uint8_t  a = 200;
int16_t  b = -50;
auto result = a + b;         // int — both promoted to int, no issues

int32_t s = 1;
int64_t d = 2;
auto r = s + d;              // int64_t — widened
```

### Danger zone: same-width signed + unsigned

```c
int32_t  s = -1;
uint32_t u = 1;
auto r = s + u;              // uint32_t! — -1 becomes 4294967295

// Fix: cast to wider signed type
auto r = (int64_t)s + (int64_t)u;  // int64_t, result is 0
```

### Quick reference

| Left | Right | Result | Safe? |
|------|-------|--------|-------|
| `int8_t` | `int8_t` | `int` | ✅ |
| `uint8_t` | `uint8_t` | `int` | ✅ |
| `int16_t` | `uint16_t` | `int` | ✅ |
| `int32_t` | `int32_t` | `int32_t` | ✅ |
| `uint32_t` | `uint32_t` | `uint32_t` | ✅ |
| `int32_t` | `uint32_t` | `uint32_t` | ⚠️ signed → unsigned |
| `int64_t` | `uint32_t` | `int64_t` | ✅ (64-bit holds both) |
| `int64_t` | `uint64_t` | `uint64_t` | ⚠️ signed → unsigned |

**Rule of thumb:** mixing signed and unsigned of the same width is the danger zone. Cast to a wider signed type if you need correct arithmetic. `-Wconversion` (in compile flags) catches most cases.

---

## Anonymous Structs and Unions

Inner members flatten into the outer scope — no intermediate name.

```c
// Without — nested access
struct player {
    struct { float x, y, z; } pos;
};
p.pos.x = 1.0f;

// With anonymous — flat access
struct player {
    struct { float x, y, z; };     // no name
};
p.x = 1.0f;                        // direct
```

### Tagged union pattern

```c
enum value_tag : uint8_t { VAL_INT, VAL_FLOAT, VAL_STRING };

struct value {
    enum value_tag tag;
    union {
        int32_t  i32;
        float    f32;
        char     str[16];
    };                               // anonymous union
};

struct value v = { .tag = VAL_INT, .i32 = 42 };
printf("%d\n", v.i32);              // direct, no .data. prefix
```

### Bitfield registers

```c
union status_reg {
    uint32_t raw;
    struct {
        uint32_t busy    : 1;
        uint32_t error   : 1;
        uint32_t ready   : 1;
        uint32_t         : 5;
        uint32_t channel : 8;
    };                               // anonymous struct
};

union status_reg sr;
sr.raw = read_reg(0x4000);
if (sr.error) { /* ... */ }
sr.channel = 3;
write_reg(0x4000, sr.raw);
```

Two anonymous structs/unions in the same scope overlap like a union — only use when you want flattening.

---

## Obsoleted Headers in C23

These still compile but the keywords make them unnecessary:

| Header | What it provided | C23 replacement |
|--------|-----------------|-----------------|
| `<stdbool.h>` | `bool` `true` `false` | Built-in keywords |
| `<stdnoreturn.h>` | `noreturn` | Built-in keyword |
| `<assert.h>` (for `static_assert`) | `static_assert(cond, msg)` | Built-in keyword (message optional) |