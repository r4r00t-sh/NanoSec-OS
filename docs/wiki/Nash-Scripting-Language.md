# Nash Scripting Language

Nash is NanoSec OS's custom scripting language with a unique syntax, distinct from Bash.

## File Extension
Nash scripts use the `.nsh` extension.

## Running Scripts
```bash
nash myscript.nsh
```

---

## Syntax Reference

### Variables

Variables use `@` prefix:

```nash
-- Set a variable
@name = "John"
@count = 42

-- Use in text
print "Hello, @name"

-- Show variable details
show @name
```

### Output

```nash
print "Hello, World!"
print "Value is @myvar"
```

### Comments

```nash
-- This is a comment
:: This is also a comment
```

---

## Conditionals

Nash uses `when`/`otherwise`/`end` instead of if/else/fi:

```nash
@age = 25

when @age gt 18 do
    print "Adult"
otherwise
    print "Minor"
end
```

### Operators

| Operator | Meaning |
|----------|---------|
| `eq` | Equals |
| `ne` | Not equals |
| `gt` | Greater than |
| `lt` | Less than |

### Examples

```nash
when @status eq ok do
    print "All good!"
end

when @count gt 0 do
    print "Positive"
end
```

---

## Loops

### Repeat Loop

```nash
repeat 5 times
    print "Iteration"
end
```

---

## Running Commands

Use `run` to execute shell commands:

```nash
run ls
run pwd
run mkdir mydir
```

Or just write the command directly:

```nash
ls
pwd
cat readme.txt
```

---

## Complete Example

```nash
-- NanoSec OS Demo Script
-- ======================

-- Set variables
@project = "NanoSec"
@version = "1.0"

-- Welcome message
print "Welcome to @project OS!"
print "Version: @version"

-- Conditional
when @version eq 1.0 do
    print "Running stable release"
otherwise
    print "Running development version"
end

-- Loop
print ""
print "Counting to 3:"
repeat 3 times
    print "* count"
end

-- System info
print ""
print "Current directory:"
run pwd

print ""
print "Files:"
run ls

print ""
print "Script complete!"
```

---

## Built-in Variables

| Variable | Description |
|----------|-------------|
| `@shell` | Shell name (nash) |
| `@version` | Nash version |

---

## Tips

1. Always use `.nsh` extension
2. Variables are case-sensitive
3. Strings don't require quotes if no spaces
4. Use `--` for comments, not `#`
5. End blocks with `end`, not `fi` or `done`
