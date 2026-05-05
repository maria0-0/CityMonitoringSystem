# AI Usage - Phases 1 and 2

I primarily used AI tools (ChatGPT/Gemini) as a coding assistant to speed up repetitive tasks, generate standard C boilerplate, and help catch minor syntax errors faster. This allowed me to focus my time on applying the core Operating Systems concepts taught in the course material.

### Phase 1: Core System

- **The Filter Command (Required AI usage)**: The assignment explicitly required using AI to generate the parsing and evaluation logic for the `--filter` command. I prompted the AI to write the `parse_condition` and `match_condition` functions. It successfully generated the C code to use `strchr` to split dynamic strings (like `severity:>=:2`) by the colon character, and it generated the repetitive `if/else` branches and `strcmp` operations to evaluate the condition against the struct fields.
- **Octal Permissions and Struct Initialization**: I used AI to quickly write out the repetitive C code needed to initialize the `ensure_district` files with the correct `0664`, `0640`, and `0644` permissions, and to rapidly format the variables for the fixed-size `Report` struct.

### Phase 2: Processes and Signals

- **Boilerplate for System Calls**: While I understood the core concepts of `fork()`, `exec()`, and `wait()` from the course material, I used AI to generate the basic C boilerplate for the child/parent process branching. This was very helpful for writing code faster so I didn't have to constantly look up the exact required header files (like `<sys/wait.h>` and `<unistd.h>`).
- **Syntax Formatting**: I used the AI to help format the `struct sigaction` setup and the `printf` logging strings in `monitor_reports.c`. Writing out the `memset` and `sigfillset` lines can be repetitive, so having the AI type it out sped up development.
- **Debugging Compiler Warnings**: I occasionally pasted my compiler warnings into the AI so it could instantly point out where I missed a `#include` or had a typo, rather than spending time hunting for it manually.

---

### Example Prompts and Outcomes

To fulfill the requirement of showing exactly what prompts I gave the AI and what I received back, here are two examples:

**Example 1: The Filter Command (Phase 1)**
* **The Prompt I gave:** *"I need a C function that takes a filter string like 'severity:>=:2' and safely splits it into the field, the operator, and the value without crashing. I also need another function that compares those strings against an integer or string inside a Report struct."*
* **The Outcome I got:** The AI generated the `parse_condition` function (which cleverly used `strchr` to find the colons and split the string) and the `match_condition` function (which generated the long `if/else` ladder to compare operators like `<`, `>`, and `==`).

**Example 2: Spawning Processes (Phase 2)**
* **The Prompt I gave:** *"How do I use fork() and execlp() in C to run the 'rm -rf' command on a specific directory safely, and how do I make the main parent process wait for it to finish?"*
* **The Outcome I got:** The AI provided a small code snippet demonstrating the correct boilerplate. It showed me how to check `if (pid == 0)` for the child process to run `execlp`, and how to use `wait(&status)` combined with `WIFEXITED(status)` in the `else` block so the parent correctly synchronizes with the deletion.
