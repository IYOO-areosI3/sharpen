SharpenOS 🐾

The Cool, Lightweight Terminal with Cat Style :3

SharpenOS isn’t just a shell – it’s a purr-sonality.
A tiny, blazing-fast command line written in C, packed with custom feline‑friendly commands and a dash of cat‑style sweetness. Forget ls, cd, and touch — here we lp, ed, and file our way around!

---

```
    ███████╗██╗  ██╗ █████╗ ██████╗ ██████╗ ███████╗███╗   ██╗ ██████╗ ███████╗
    ██╔════╝██║  ██║██╔══██╗██╔══██╗██╔══██╗██╔════╝████╗  ██║██╔═══██╗██╔════╝
    ███████╗███████║███████║██████╔╝██████╔╝█████╗  ██╔██╗ ██║██║   ██║███████╗
    ╚════██║██╔══██║██╔══██║██╔══██╗██╔══██╗██╔══╝  ██║╚██╗██║██║   ██║╚════██║
    ███████║██║  ██║██║  ██║██║  ██║██████╔╝███████╗██║ ╚████║╚██████╔╝███████║
    ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═══╝ ╚═════╝ ╚══════╝
               Cool, Lightweight Terminal  :3
```

---

🐾 Features

· Cat‑style EVERYTHING – from the prompt (╭── :3 SharpenOS) to error messages (:3 Nyā~), it’s all pawsitively adorable.
· Custom built‑in commands – no boring ls or cd; we have lp, ed, file, directory, ram, and more.
· Friendly, readable help – help shows all commands, help <cmd> gives detailed examples.
· Storage bar & percentage – ram shows a colored bar and human‑readable sizes.
· Colorised file listing – directories, executables, symlinks, and pipes each have their own color.
· Blocked legacy commands – tries to use ls? SharpenOS gently paws you away.
· Environment variable & tilde expansion – ~ becomes $HOME, $VAR works everywhere.
· External command execution – any program not built‑in is run via fork/exec.
· Signal handling – Ctrl+C is ignored in the shell, passed normally to child processes.

---

🛠️ Compilation & Running

SharpenOS is a single C file. Just compile and run:

```bash
gcc -o SharpenOS SharpenOS.c
./SharpenOS
```

If you prefer clang compiler it will became
```bash
clang -o SharpenOS SharpenOS.c
./SharpenOS
```

No external dependencies – only standard C/POSIX libraries.

Note for Android / Termux users:
Use gcc (from Termux packages) the same way. Everything works out‑of‑the‑box.

---

📚 Command Reference

ed – Enter Directory

Replaces cd (which is strictly forbidden here!)

```
ed PATH:<directory>
```

Examples:

```
ed PATH:/sdcard/Documents
ed PATH:../projects
ed PATH:          # stays in current directory
```

Tip: always start the argument with PATH: — it’s part of the cutie‑cat charm!

---

lp – List Path

Replaces ls

```
lp [options]
```

Options:

· -nofile – show only directories
· -nofolder – show only files
· -ext .ext – filter by file extension (e.g. -ext .lua)

Colors:

· Blue → directory
· Cyan → symbolic link
· Green → executable
· Magenta → socket / pipe
· Default → regular file

Examples:

```
lp
lp -nofile
lp -ext .txt
```

---

file – Create a New File

Replaces touch

```
file <filename>
```

Examples:

```
file meow.txt
file purr-logs/scratch.log
```

---

directory – Create a New Directory

Replaces mkdir

```
directory <dirname>
```

Examples:

```
directory purr
directory ~/cat_projects/
```

---

ram – Show Storage Usage

See disk usage with a cute bar :3

```
ram [path]
```

If no path is given, the current directory is used.

Output:

· Colored usage bar (red = used, green = free)
· Percentage used
· Human‑readable free / used / total sizes

Examples:

```
ram
ram /sdcard
```

---

clear / cls – Clear the Screen

```
clear
cls
```

Both do exactly the same – wipe the terminal clean.

---

info – System Information

```
info
```

Displays:

· Username
· Hostname
· OS & kernel version
· Current date/time

---

help – Get Help

```
help
help <command>
```

Without arguments – prints a list of all built‑in commands.
With a command name – shows detailed usage and examples.

Examples:

```
help lp
help ed
```

---

exit – Leave SharpenOS

```
exit
```

Closes the terminal with a kind “Goodbye from SharpenOS! Stay purr‑fect! :3” message.

---

External Programs

Any command that is not a built‑in is run as a normal system program:

```
gcc --version
vim mycode.c
python script.py
```

---

😺 The Cat Style

Prompt

Every line starts with a beautiful two‑line prompt:

```
╭── :3 SharpenOS · 05:09:2026 · /home/cat ───╮
╰─> 
```

Shows date, current folder, and the iconic :3 face.

Error Messages

Instead of boring command not found, SharpenOS gives you:

```
:3 Nyā~ Oops! Those commands aren't allowed here!
  × ls   → use lp
  × cd   → use ed PATH:<dir>
  × touch → use file <name>
  × mkdir → use directory <name>
  >^._.^<
```

Even system errors are prefixed with :3 Nyā~.

Legacy Command Blocking

SharpenOS blocks ls, cd, touch, and mkdir entirely, replacing them with a friendly suggestion. Try it – you’ll get a kitty‑scolding!

---

🔍 Examples in Action

```bash
╭── :3 SharpenOS · 05:09:2026 · /home/user ───╮
╰─> lp
Documents
Downloads
meow.txt

╭── :3 SharpenOS · 05:09:2026 · /home/user ───╮
╰─> file new_cat_pic.png
Nyā~ created file: new_cat_pic.png

╭── :3 SharpenOS · 05:09:2026 · /home/user ───╮
╰─> ram
╭── 📦 Storage on . ──╮
  ████████░░░░░░░░░░░░   45.2% used
  Free: 12.3G  |  Used: 10.1G  |  Total: 22.4G
╰──────────────────────────╯

╭── :3 SharpenOS · 05:09:2026 · /home/user ───╮
╰─> help ram
ram – Show storage usage
Usage: ram [path]
If no path given, uses the current directory.
Example: ram /sdcard
```

---

📜 License

SharpenOS is free and open‑source. You may use, share, and modify it as you like. If you add extra fluff, share it back so all kitties can benefit! :3

---

🐱 Contributing

Pull requests and patches are welcome! Keep the cat spirit alive:

· Use the existing code style (simple, readable C)
· Keep errors adorable – always start with :3 Nyā~
· Add new built‑ins with full help <cmd> support
· Test before submitting

---

👏 Credits

Original code by [your name here]
Inspired by cats, terminal fun, and the desire to never type ls again.

---

SharpenOS – because every terminal deserves a little meow :3