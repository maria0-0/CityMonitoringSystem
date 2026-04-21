# AI Usage Documentation

**Tool Used:** Google Gemini 3.1 Pro (High)

## Function 1: `parse_condition`

### Prompt
> I am writing a C program to manage a city infrastructure report system. I have a command-line utility where a user can enter a filter condition string in the format `field:operator:value`. For example, `severity:>=:2`.
> Please write a C function with the signature `int parse_condition(const char *input, char *field, char *op, char *value);` that splits this input string into its three components. Assume the input is well-formed. Return 1 on success, 0 on failure. Wait, I also need to make sure the original string isn't mutated if it's read-only.

### AI Generated Code
```c
#include <string.h>

int parse_condition(const char *input, char *field, char *op, char *value) {
    char buffer[256];
    strncpy(buffer, input, sizeof(buffer)-1);
    buffer[255] = '\0';
    
    char *colon1 = strchr(buffer, ':');
    if (!colon1) return 0;
    
    char *colon2 = strchr(colon1 + 1, ':');
    if (!colon2) return 0;
    
    *colon1 = '\0';
    *colon2 = '\0';
    
    strcpy(field, buffer);
    strcpy(op, colon1 + 1);
    strcpy(value, colon2 + 1);
    return 1;
}
```

### Changes Made & Why
I reviewed the code line-by-line. The AI appropriately allocated a local `buffer` to prevent segfaults from modifying read-only string literals (since `strchr` and pointer manipulation is used to inject `\0`). I kept the function exactly as generated, as it safely handles string splitting and meets all requirements.

---

## Function 2: `match_condition`

### Prompt
> My program uses a struct to store reports. The struct has the following fields:
> ```c
> typedef struct {
>     int id;
>     char inspector[32];
>     float lat;
>     float lon;
>     char category[32];
>     int severity;
>     time_t timestamp;
>     char description[120];
> } Report;
> ```
> Based on the fields `severity`, `category`, `inspector`, and `timestamp`, generate a C function `int match_condition(Report *r, const char *field, const char *op, const char *value);` which returns 1 if the record satisfies the condition and 0 otherwise. Supported operators are `==`, `!=`, `<`, `<=`, `>`, `>=` for integers and timestamps, and `==`, `!=` for strings.

### AI Generated Code
```c
#include <string.h>
#include <stdlib.h>

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int v = atoi(value), r_v = r->severity;
        if (strcmp(op, "==") == 0) return r_v == v;
        if (strcmp(op, "!=") == 0) return r_v != v;
        if (strcmp(op, "<") == 0) return r_v < v;
        if (strcmp(op, "<=") == 0) return r_v <= v;
        if (strcmp(op, ">") == 0) return r_v > v;
        if (strcmp(op, ">=") == 0) return r_v >= v;
    } else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    } else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector, value) != 0;
    } else if (strcmp(field, "timestamp") == 0) {
        time_t v = atoll(value), r_v = r->timestamp;
        if (strcmp(op, "==") == 0) return r_v == v;
        if (strcmp(op, "!=") == 0) return r_v != v;
        if (strcmp(op, "<") == 0) return r_v < v;
        if (strcmp(op, "<=") == 0) return r_v <= v;
        if (strcmp(op, ">") == 0) return r_v > v;
        if (strcmp(op, ">=") == 0) return r_v >= v;
    }
    return 0;
}
```

### Changes Made & Why
I reviewed the string conversion logic. `atoi` is used for integers and `atoll` for `time_t` which is a 64-bit integer on our system. String equality correctly falls back to `strcmp() == 0`. I incorporated this directly into `city_manager.c`. The AI handled typing effectively.

### What I Learned
Using AI for repetitive logic branching in C saves significant manual typing, particularly for operators mapping. However, ensuring the correct data type conversion (`atoi` vs `atoll` vs `strcmp`) had to be double-checked because AI sometimes assumes standard 32-bit types leading to subtle truncation bugs. Here, its use of `atoll` for `timestamp` was optimal.
