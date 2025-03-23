# OS_HW1
Operating Systems Course 234123 - First Wet HomeWork.

# smash — A Linux Shell in C

`smash` is a lightweight Unix shell developed in C as part of Operating Systems coursework. It replicates core shell functionality with features like:

- ⚙️ Built-in command execution
- 🧠 Job control (foreground/background)
- 🚦 Signal handling (e.g., `Ctrl+C`, `Ctrl+Z`)

## 🧩 Features

### ✅ Built-in Commands
- `cd`, `pwd`, `jobs`, `fg`, `bg`, `kill`, `quit`, and more
- Simple command parsing and dispatching

### 🏃 Job Control
- Run processes in background with `&`
- Bring background jobs to foreground
- Manage job list with `jobs`, `fg`, `bg`

### ⚡ Signal Handling
- Custom handlers for `SIGINT` (Ctrl+C), `SIGTSTP` (Ctrl+Z), and `SIGCHLD`
- Clean handling of zombie processes and user interrupts

## 🔧 Build & Run

```bash
gcc -Wall -o smash smash.c
./smash
