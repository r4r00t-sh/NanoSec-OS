# Contributing

Thank you for your interest in contributing to NanoSec OS! This guide will help you get started.

---

## Ways to Contribute

- 🐛 **Report bugs** - Open an issue
- 💡 **Suggest features** - Open a feature request
- 📝 **Improve documentation** - Fix typos, add examples
- 🔧 **Submit code** - Bug fixes, new features
- 🧪 **Test** - Try new features, report issues

---

## Getting Started

### 1. Fork the Repository
Click "Fork" on GitHub to create your copy.

### 2. Clone Your Fork
```bash
git clone https://github.com/YOUR-USERNAME/NanoSec-OS.git
cd NanoSec-OS
```

### 3. Create a Branch
```bash
git checkout -b feature/your-feature-name
# or
git checkout -b fix/bug-description
```

### 4. Make Changes
- Write your code
- Test thoroughly
- Follow coding style

### 5. Commit
```bash
git add .
git commit -m "feat: add new feature description"
```

### 6. Push
```bash
git push origin feature/your-feature-name
```

### 7. Open Pull Request
Go to GitHub and click "New Pull Request".

---

## Coding Style

### C Code Style

```c
/* Function comment */
int function_name(int arg1, const char *arg2) {
    /* Local variables at top */
    int result = 0;
    
    /* Logic with clear comments */
    if (condition) {
        do_something();
    } else {
        do_other_thing();
    }
    
    return result;
}
```

### Guidelines

| Rule | Example |
|------|---------|
| Function names | `snake_case` |
| Constants | `UPPER_CASE` |
| Types | `type_name_t` |
| Indentation | 4 spaces (no tabs) |
| Line length | Max 100 characters |
| Braces | Same line as statement |

### Header Files
```c
#ifndef MYHEADER_H
#define MYHEADER_H

/* Declarations here */

#endif /* MYHEADER_H */
```

---

## Commit Messages

Use conventional commits format:

```
type(scope): description

[optional body]
```

### Types
| Type | Description |
|------|-------------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation |
| `style` | Code style (formatting) |
| `refactor` | Code refactoring |
| `test` | Adding tests |
| `chore` | Maintenance |

### Examples
```
feat(shell): add pipe support for commands
fix(fs): handle empty directory correctly
docs(readme): update build instructions
```

---

## Project Structure

When adding new features, follow this structure:

```
kernel/
├── kernel.c        # Main entry point
├── kernel.h        # Global declarations (add yours here)
├── shell.c         # Shell commands
│
├── drivers/        # Hardware drivers
│   └── your_driver.c
├── fs/             # Filesystem
│   └── your_fs_feature.c
├── net/            # Networking
│   └── your_net_feature.c
├── security/       # Security features
│   └── your_security.c
└── nash/           # Scripting language
    └── nash.c
```

---

## Adding a New Command

### 1. Create handler function
```c
// In appropriate file (shell.c or new file)
void cmd_mycommand(const char *args) {
    kprintf("My command executed with: %s\n", args);
}
```

### 2. Add declaration to kernel.h
```c
void cmd_mycommand(const char *args);
```

### 3. Register in shell.c
```c
static command_t commands[] = {
    // ... existing commands ...
    {"mycommand", "Description", cmd_mycommand},
};
```

### 4. Test
```bash
make clean && make && make run
# In OS:
mycommand test
```

---

## Adding a New Driver

### 1. Create driver file
```c
// kernel/drivers/mydriver.c
#include "../kernel.h"

void mydriver_init(void) {
    // Initialize hardware
}

void mydriver_handler(void) {
    // Handle interrupts
}
```

### 2. Add to Makefile
```makefile
C_SOURCES = ... \
            drivers/mydriver.c \
```

### 3. Initialize in kernel.c
```c
void kernel_main(void) {
    // ... other init ...
    mydriver_init();
}
```

---

## Testing

### Manual Testing
```bash
make clean && make full && make run-iso
```

Test your changes in the QEMU environment.

### Checklist Before PR
- [ ] Code compiles without warnings
- [ ] New feature works as expected
- [ ] Existing features still work
- [ ] Documentation updated if needed
- [ ] Commit messages follow convention

---

## Issue Guidelines

### Bug Reports
Include:
- OS and environment (Linux, WSL, etc.)
- Steps to reproduce
- Expected behavior
- Actual behavior
- Screenshots if applicable

### Feature Requests
Include:
- Clear description of feature
- Use cases
- Possible implementation approach

---

## Pull Request Process

1. Ensure PR is focused (one feature/fix per PR)
2. Update documentation if needed
3. Add description of changes
4. Link related issues
5. Wait for review
6. Address feedback
7. Merge! 🎉

---

## Code of Conduct

- Be respectful and inclusive
- Welcome newcomers
- Focus on constructive feedback
- No harassment or discrimination

---

## Questions?

- Open an issue with "question" label
- Check existing issues/wiki first

Thank you for contributing! 🙏
