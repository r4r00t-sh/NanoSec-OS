# Commands Reference

## Shell Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `\|` | Pipe output | `ls \| wc` |
| `>` | Redirect to file | `ls > files.txt` |
| `>>` | Append to file | `echo hi >> log` |
| `<` | Input from file | `wc < file.txt` |
| `&&` | Run if success | `mkdir foo && cd foo` |
| `\|\|` | Run if fail | `cd foo \|\| mkdir foo` |
| `;` | Run sequentially | `ls; pwd; date` |

---

## File Operations

### ls - List Files
```bash
ls              # Current directory
ls /bin         # Specific directory
ls | sort       # Sorted output
```

### cd - Change Directory
```bash
cd /home        # Absolute path
cd ..           # Parent directory
cd              # Home directory
```

### mkdir - Create Directory
```bash
mkdir mydir
mkdir -p a/b/c  # Create parents
```

### rm - Remove
```bash
rm file.txt     # Remove file
rm -rf dir/     # Remove directory recursively
```

### cp / mv - Copy / Move
```bash
cp src dst      # Copy file
mv old new      # Rename/move
```

### touch - Create File
```bash
touch newfile.txt
```

### cat - View File
```bash
cat file.txt
cat file | grep pattern
```

---

## Text Processing

### grep - Search
```bash
grep pattern file.txt
ls | grep .txt
```

### head / tail - First/Last Lines
```bash
head 10 file.txt    # First 10 lines
tail 5 file.txt     # Last 5 lines
```

### wc - Word Count
```bash
wc file.txt         # Lines, words, chars
cat file | wc
```

### sort / uniq
```bash
sort file.txt
sort file | uniq    # Remove duplicates
```

### cut - Extract Columns
```bash
cut -d: -f1 /etc/passwd   # First field, : delimiter
```

### tr - Translate
```bash
cat file | tr a-z A-Z     # Uppercase
```

### sed - Stream Editor
```bash
sed s/old/new/ file.txt
sed s/old/new/g file       # Global replace
```

### diff - Compare Files
```bash
diff file1 file2
```

---

## System Commands

### pwd - Current Directory
```bash
pwd
```

### df - Disk Usage
```bash
df
```

### du - Directory Size
```bash
du
```

### env - Environment Variables
```bash
env
export VAR=value
unset VAR
```

### history - Command History
```bash
history
```

### find - Search Files
```bash
find -name txt
find /home -name config
```

### stat - File Information
```bash
stat file.txt
```

### more - Page Through File
```bash
more largefile.txt
```

---

## User Commands

### login / logout
```bash
logout
login
```

### su - Switch User
```bash
su root
```

### passwd - Change Password
```bash
passwd
```

### chmod / chown
```bash
chmod 755 file
chown user file
```

---

## Networking

### nping - Ping
```bash
nping 192.168.1.1
```

### nifconfig - Network Config
```bash
nifconfig
```

### firewall
```bash
firewall status
firewall allow 80
firewall deny 22
```

---

## Scripting

### nash - Run Script
```bash
nash script.nsh
```

See [[Nash Scripting Language]] for full documentation.
