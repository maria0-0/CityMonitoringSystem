# AI Usage - All Phases

I primarily used AI tools (ChatGPT/Gemini) as a coding assistant across all three phases of the project. This allowed me to focus on the core Operating Systems concepts while speeding up repetitive tasks and boilerplate generation.

### Phase 1: Core System
- **The Filter Command (Required AI usage)**: The assignment explicitly required using AI to generate the parsing and evaluation logic for the `--filter` command. I prompted the AI to write the `parse_condition` and `match_condition` functions.
- **Octal Permissions and Struct Initialization**: I used AI to quickly format the variables for the fixed-size `Report` struct and set the correct octal permissions for `ensure_district`.

### Phase 2: Processes and Signals
- **Using fork and exec**: I used AI to generate the basic C boilerplate for spawning a child process to run `rm -rf` safely.
- **Signal Handling**: I asked for an example of `struct sigaction` setup since the manual page was a bit confusing compared to the older `signal()` function.

### Phase 3: Pipes and Redirects
- **Double Forking and Pipes**: For the `city_hub` program, I needed to create a background process that managed another child process (the monitor) through a pipe. I used AI to help me understand the "double fork" pattern and how to properly close the unused pipe ends in each process to avoid hanging.
- **Redirecting with dup2**: I used AI to get a clear example of how to use `dup2()` to redirect the standard output of the monitor process into the pipe so the Hub could read it.
- **Solving Pipe Buffering**: When I first tested the pipe, the monitor's messages weren't showing up in the Hub immediately. I asked the AI for a solution, and it suggested using `setvbuf(stdout, NULL, _IONBF, 0)` to disable buffering. This fixed the problem and ensured the logs appeared in the Hub terminal in real-time.

---

### Example Prompts and Outcomes

**Example 1: The Filter Command (Phase 1)**
* **The Prompt I gave:** *"I need a C function that takes a filter string like 'severity:>=:2' and safely splits it into the field, the operator, and the value without crashing. I also need another function that compares those strings against an integer or string inside a Report struct."*
* **The Outcome I got:** The AI generated the `parse_condition` function (which used `strchr` to find the colons and split the string) and the `match_condition` function (which generated the logic to compare operators like `<`, `>`, and `==`).

**Example 2: Spawning Processes (Phase 2)**
* **The Prompt I gave:** *"How do I use fork() and execlp() in C to run the 'rm -rf' command on a specific directory safely, and how do I make the main parent process wait for it to finish?"*
* **The Outcome I got:** The AI provided a code snippet showing how to check `if (pid == 0)` for the child process to run `execlp`, and how to use `wait(&status)` combined with `WIFEXITED(status)` in the parent block.

**Example 3: Pipes and Redirects (Phase 3)**
* **The Prompt I gave:** *"I need a C program that starts a background process, which then starts another program and reads its stdout using a pipe. How do I use pipe(), fork(), and dup2() together for this?"*
* **The Outcome I got:** The AI provided a structural example showing how to create the pipe *before* forking, and how the child process must use `dup2(pipe_fd[1], STDOUT_FILENO)` before calling `exec`. It also reminded me to close the read-end in the child and the write-end in the parent.

**Example 4: Pipe Buffering (Phase 3)**
* **The Prompt I gave:** *"I created a pipe between two processes, but the messages from the child aren't showing up in the parent's terminal until the child finishes or writes a lot of text. How do I make the output appear immediately?"*
* **The Outcome I got:** The AI explained that pipes use block-buffering by default. It suggested using `setvbuf(stdout, NULL, _IONBF, 0)` at the start of the program to disable buffering completely, which solved the lag issue.
