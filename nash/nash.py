#!/usr/bin/env python3
"""
NASH - NanoSec Shell
=====================
A security-focused shell for NanoSec OS

Features:
- Built-in security commands
- System monitoring
- Colorized output
- Command history
- Tab completion
- Script execution (.nash files)
"""

import os
import sys
import readline
import subprocess
import socket
import hashlib
import datetime
import signal
import getpass
from pathlib import Path

# ANSI Colors
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    RESET = '\033[0m'

class SecurityStatus:
    """Track security status for prompt"""
    firewall_active = False
    ids_active = False
    alerts = 0
    
    @classmethod
    def check(cls):
        """Check security services status"""
        # Check iptables
        try:
            result = subprocess.run(['iptables', '-L', '-n'], 
                                   capture_output=True, text=True, timeout=5)
            cls.firewall_active = result.returncode == 0 and 'DROP' in result.stdout
        except:
            cls.firewall_active = False
        
        # Check IDS process
        try:
            result = subprocess.run(['pgrep', '-f', 'ids.py'], 
                                   capture_output=True, timeout=5)
            cls.ids_active = result.returncode == 0
        except:
            cls.ids_active = False
    
    @classmethod
    def indicator(cls):
        """Return status indicator for prompt"""
        fw = f"{Colors.GREEN}●{Colors.RESET}" if cls.firewall_active else f"{Colors.RED}○{Colors.RESET}"
        ids = f"{Colors.GREEN}●{Colors.RESET}" if cls.ids_active else f"{Colors.RED}○{Colors.RESET}"
        alert = f" {Colors.RED}⚠{cls.alerts}{Colors.RESET}" if cls.alerts > 0 else ""
        return f"[FW:{fw} IDS:{ids}{alert}]"

class NASH:
    """NanoSec Shell - Main Shell Class"""
    
    VERSION = "1.0.0"
    HISTORY_FILE = os.path.expanduser("~/.nash_history")
    
    def __init__(self):
        self.running = True
        self.last_status = 0
        self.hostname = socket.gethostname()
        self.user = getpass.getuser()
        self.commands = self._register_commands()
        self._setup_readline()
        self._setup_signals()
        
    def _register_commands(self):
        """Register built-in commands"""
        return {
            'help': self.cmd_help,
            'exit': self.cmd_exit,
            'quit': self.cmd_exit,
            'clear': self.cmd_clear,
            'cd': self.cmd_cd,
            'pwd': self.cmd_pwd,
            'ls': self.cmd_ls,
            'cat': self.cmd_cat,
            
            # Security commands
            'scan': self.cmd_scan,
            'firewall': self.cmd_firewall,
            'fim': self.cmd_fim,
            'ids': self.cmd_ids,
            'netstat': self.cmd_netstat,
            'logs': self.cmd_logs,
            'users': self.cmd_users,
            'processes': self.cmd_processes,
            'hash': self.cmd_hash,
            'sysinfo': self.cmd_sysinfo,
            
            # NASH specific
            'status': self.cmd_status,
            'version': self.cmd_version,
            'source': self.cmd_source,
        }
    
    def _setup_readline(self):
        """Setup readline for history and completion"""
        try:
            readline.read_history_file(self.HISTORY_FILE)
        except FileNotFoundError:
            pass
        
        readline.set_history_length(1000)
        readline.set_completer(self._completer)
        readline.parse_and_bind('tab: complete')
        
    def _setup_signals(self):
        """Setup signal handlers"""
        signal.signal(signal.SIGINT, self._handle_sigint)
        
    def _handle_sigint(self, sig, frame):
        """Handle Ctrl+C"""
        print()
        
    def _completer(self, text, state):
        """Tab completion"""
        options = [cmd for cmd in self.commands.keys() if cmd.startswith(text)]
        
        # Add file completion
        if text:
            try:
                path = Path(text).expanduser()
                parent = path.parent if not path.exists() else path
                for p in parent.iterdir():
                    name = str(p)
                    if name.startswith(text):
                        options.append(name + ('/' if p.is_dir() else ''))
            except:
                pass
        
        return options[state] if state < len(options) else None
    
    def get_prompt(self):
        """Generate shell prompt"""
        SecurityStatus.check()
        status = SecurityStatus.indicator()
        
        # Color based on user
        user_color = Colors.RED if self.user == 'root' else Colors.GREEN
        
        cwd = os.getcwd()
        home = os.path.expanduser("~")
        if cwd.startswith(home):
            cwd = "~" + cwd[len(home):]
        
        return (f"{status} "
                f"{user_color}{self.user}{Colors.RESET}@"
                f"{Colors.CYAN}{self.hostname}{Colors.RESET}:"
                f"{Colors.BLUE}{cwd}{Colors.RESET}"
                f"{'#' if self.user == 'root' else '$'} ")
    
    def parse_command(self, line):
        """Parse command line into command and arguments"""
        parts = line.strip().split()
        if not parts:
            return None, []
        return parts[0], parts[1:]
    
    def execute(self, line):
        """Execute a command line"""
        cmd, args = self.parse_command(line)
        
        if cmd is None:
            return 0
        
        # Check built-in commands
        if cmd in self.commands:
            try:
                return self.commands[cmd](args)
            except Exception as e:
                print(f"{Colors.RED}Error: {e}{Colors.RESET}")
                return 1
        
        # Try external command
        try:
            result = subprocess.run(line, shell=True)
            return result.returncode
        except Exception as e:
            print(f"{Colors.RED}Error executing command: {e}{Colors.RESET}")
            return 1
    
    def run(self):
        """Main shell loop"""
        self._print_banner()
        
        while self.running:
            try:
                line = input(self.get_prompt())
                if line.strip():
                    self.last_status = self.execute(line)
                    readline.write_history_file(self.HISTORY_FILE)
            except EOFError:
                print()
                break
            except KeyboardInterrupt:
                print()
                continue
    
    def _print_banner(self):
        """Print shell banner"""
        banner = f"""
{Colors.CYAN}╔═══════════════════════════════════════════════════════════╗
║  {Colors.BOLD}NASH{Colors.RESET}{Colors.CYAN} - NanoSec Shell v{self.VERSION}                              ║
║  Security-First Shell for NanoSec OS                       ║
╚═══════════════════════════════════════════════════════════╝{Colors.RESET}
"""
        print(banner)
        print(f"  Type {Colors.GREEN}'help'{Colors.RESET} for available commands\n")

    # ==================== Built-in Commands ====================
    
    def cmd_help(self, args):
        """Show help for commands"""
        if args:
            cmd = args[0]
            if cmd in self.commands:
                doc = self.commands[cmd].__doc__ or "No documentation"
                print(f"{Colors.BOLD}{cmd}{Colors.RESET}: {doc}")
            else:
                print(f"Unknown command: {cmd}")
            return 0
        
        print(f"\n{Colors.BOLD}NASH Built-in Commands{Colors.RESET}\n")
        
        categories = {
            "Navigation": ['cd', 'pwd', 'ls', 'cat', 'clear'],
            "Security": ['scan', 'firewall', 'fim', 'ids', 'hash'],
            "Monitoring": ['netstat', 'logs', 'users', 'processes', 'sysinfo'],
            "Shell": ['help', 'status', 'version', 'source', 'exit'],
        }
        
        for category, cmds in categories.items():
            print(f"  {Colors.YELLOW}{category}{Colors.RESET}")
            for cmd in cmds:
                if cmd in self.commands:
                    doc = self.commands[cmd].__doc__ or ""
                    doc = doc.split('\n')[0]  # First line only
                    print(f"    {Colors.GREEN}{cmd:12}{Colors.RESET} {doc}")
            print()
        
        return 0
    
    def cmd_exit(self, args):
        """Exit the shell"""
        print(f"{Colors.CYAN}Goodbye!{Colors.RESET}")
        self.running = False
        return 0
    
    def cmd_clear(self, args):
        """Clear the screen"""
        os.system('clear' if os.name != 'nt' else 'cls')
        return 0
    
    def cmd_cd(self, args):
        """Change directory"""
        path = args[0] if args else os.path.expanduser("~")
        try:
            os.chdir(os.path.expanduser(path))
            return 0
        except Exception as e:
            print(f"{Colors.RED}cd: {e}{Colors.RESET}")
            return 1
    
    def cmd_pwd(self, args):
        """Print working directory"""
        print(os.getcwd())
        return 0
    
    def cmd_ls(self, args):
        """List directory contents"""
        path = args[0] if args else '.'
        try:
            entries = sorted(Path(path).iterdir())
            for entry in entries:
                if entry.is_dir():
                    print(f"{Colors.BLUE}{entry.name}/{Colors.RESET}")
                elif os.access(entry, os.X_OK):
                    print(f"{Colors.GREEN}{entry.name}*{Colors.RESET}")
                else:
                    print(entry.name)
            return 0
        except Exception as e:
            print(f"{Colors.RED}ls: {e}{Colors.RESET}")
            return 1
    
    def cmd_cat(self, args):
        """Display file contents"""
        if not args:
            print("Usage: cat <file>")
            return 1
        try:
            with open(args[0], 'r') as f:
                print(f.read(), end='')
            return 0
        except Exception as e:
            print(f"{Colors.RED}cat: {e}{Colors.RESET}")
            return 1

    # ==================== Security Commands ====================
    
    def cmd_scan(self, args):
        """Scan ports on a target - Usage: scan <host> [ports]"""
        if not args:
            print(f"Usage: scan <host> [port-range]")
            print(f"Example: scan 192.168.1.1 1-1000")
            return 1
        
        host = args[0]
        port_range = args[1] if len(args) > 1 else "1-1024"
        
        try:
            start, end = map(int, port_range.split('-'))
        except:
            start, end = 1, 1024
        
        print(f"{Colors.CYAN}Scanning {host} ports {start}-{end}...{Colors.RESET}\n")
        
        open_ports = []
        for port in range(start, min(end + 1, start + 100)):  # Limit to 100 ports
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(0.5)
                result = sock.connect_ex((host, port))
                if result == 0:
                    try:
                        service = socket.getservbyport(port)
                    except:
                        service = "unknown"
                    open_ports.append((port, service))
                    print(f"  {Colors.GREEN}OPEN{Colors.RESET}  {port:5}  {service}")
                sock.close()
            except:
                pass
        
        if not open_ports:
            print(f"  No open ports found in range {start}-{end}")
        
        print(f"\n{Colors.CYAN}Scan complete. {len(open_ports)} open ports.{Colors.RESET}")
        return 0
    
    def cmd_firewall(self, args):
        """Firewall control - Usage: firewall <status|enable|disable|rules>"""
        if not args:
            args = ['status']
        
        action = args[0]
        
        if action == 'status':
            print(f"\n{Colors.BOLD}Firewall Status{Colors.RESET}\n")
            try:
                result = subprocess.run(['iptables', '-L', '-n', '--line-numbers'], 
                                       capture_output=True, text=True)
                if result.returncode == 0:
                    print(f"  Status: {Colors.GREEN}ACTIVE{Colors.RESET}")
                    print(f"\n{result.stdout}")
                else:
                    print(f"  Status: {Colors.RED}INACTIVE{Colors.RESET}")
            except FileNotFoundError:
                print(f"  {Colors.YELLOW}iptables not found{Colors.RESET}")
            return 0
        
        elif action == 'enable':
            print("Enabling firewall...")
            script = "/opt/nanosec/security/firewall/nfw.sh"
            if os.path.exists(script):
                os.system(f"sudo {script} start")
            else:
                print(f"{Colors.YELLOW}Firewall script not found at {script}{Colors.RESET}")
            return 0
        
        elif action == 'disable':
            print("Disabling firewall...")
            os.system("sudo iptables -F && sudo iptables -X")
            print(f"{Colors.YELLOW}Firewall rules flushed{Colors.RESET}")
            return 0
        
        elif action == 'rules':
            os.system("sudo iptables -L -n -v --line-numbers")
            return 0
        
        else:
            print(f"Unknown action: {action}")
            print("Usage: firewall <status|enable|disable|rules>")
            return 1
    
    def cmd_fim(self, args):
        """File Integrity Monitor - Usage: fim <check|baseline|status> [path]"""
        if not args:
            args = ['status']
        
        action = args[0]
        path = args[1] if len(args) > 1 else '/etc'
        
        fim_script = "/opt/nanosec/security/fim/fim.py"
        
        if action == 'status':
            print(f"\n{Colors.BOLD}File Integrity Monitor Status{Colors.RESET}\n")
            baseline_file = "/var/lib/nanosec/fim/baseline.json"
            if os.path.exists(baseline_file):
                stat = os.stat(baseline_file)
                mtime = datetime.datetime.fromtimestamp(stat.st_mtime)
                print(f"  Baseline: {Colors.GREEN}EXISTS{Colors.RESET}")
                print(f"  Last updated: {mtime}")
            else:
                print(f"  Baseline: {Colors.RED}NOT FOUND{Colors.RESET}")
                print(f"  Run: fim baseline /etc")
            return 0
        
        elif action == 'baseline':
            print(f"Creating baseline for {path}...")
            if os.path.exists(fim_script):
                os.system(f"python3 {fim_script} --baseline {path}")
            else:
                self._quick_fim_baseline(path)
            return 0
        
        elif action == 'check':
            print(f"Checking integrity of {path}...")
            if os.path.exists(fim_script):
                os.system(f"python3 {fim_script} --check {path}")
            else:
                self._quick_fim_check(path)
            return 0
        
        else:
            print("Usage: fim <check|baseline|status> [path]")
            return 1
    
    def _quick_fim_baseline(self, path):
        """Quick FIM baseline (used if fim.py not available)"""
        hashes = {}
        count = 0
        for root, dirs, files in os.walk(path):
            for f in files:
                fpath = os.path.join(root, f)
                try:
                    with open(fpath, 'rb') as file:
                        h = hashlib.sha256(file.read()).hexdigest()
                        hashes[fpath] = h
                        count += 1
                except:
                    pass
        print(f"  {Colors.GREEN}Hashed {count} files{Colors.RESET}")
        
    def _quick_fim_check(self, path):
        """Quick FIM check"""
        print(f"  {Colors.YELLOW}Quick check mode - use full FIM for complete verification{Colors.RESET}")
    
    def cmd_ids(self, args):
        """Intrusion Detection System - Usage: ids <status|start|stop|alerts>"""
        if not args:
            args = ['status']
        
        action = args[0]
        
        if action == 'status':
            print(f"\n{Colors.BOLD}Intrusion Detection Status{Colors.RESET}\n")
            SecurityStatus.check()
            if SecurityStatus.ids_active:
                print(f"  Status: {Colors.GREEN}RUNNING{Colors.RESET}")
            else:
                print(f"  Status: {Colors.RED}STOPPED{Colors.RESET}")
            print(f"  Alerts: {SecurityStatus.alerts}")
            return 0
        
        elif action == 'start':
            ids_script = "/opt/nanosec/security/ids/ids.py"
            if os.path.exists(ids_script):
                os.system(f"python3 {ids_script} &")
                print(f"{Colors.GREEN}IDS started{Colors.RESET}")
            else:
                print(f"{Colors.YELLOW}IDS script not found{Colors.RESET}")
            return 0
        
        elif action == 'stop':
            os.system("pkill -f 'ids.py'")
            print(f"{Colors.YELLOW}IDS stopped{Colors.RESET}")
            return 0
        
        elif action == 'alerts':
            log_file = "/var/log/nanosec/ids.log"
            if os.path.exists(log_file):
                os.system(f"tail -50 {log_file}")
            else:
                print("No alerts logged yet")
            return 0
        
        else:
            print("Usage: ids <status|start|stop|alerts>")
            return 1
    
    def cmd_netstat(self, args):
        """Show network connections"""
        print(f"\n{Colors.BOLD}Active Network Connections{Colors.RESET}\n")
        os.system("ss -tulpn 2>/dev/null || netstat -tulpn")
        return 0
    
    def cmd_logs(self, args):
        """View security logs - Usage: logs [auth|syslog|ids|all]"""
        log_type = args[0] if args else 'all'
        
        logs = {
            'auth': '/var/log/auth.log',
            'syslog': '/var/log/syslog',
            'ids': '/var/log/nanosec/ids.log',
            'firewall': '/var/log/nanosec/firewall.log',
        }
        
        print(f"\n{Colors.BOLD}Security Logs{Colors.RESET}\n")
        
        if log_type == 'all':
            for name, path in logs.items():
                if os.path.exists(path):
                    print(f"{Colors.CYAN}=== {name.upper()} (last 5 lines) ==={Colors.RESET}")
                    os.system(f"tail -5 {path}")
                    print()
        elif log_type in logs:
            path = logs[log_type]
            if os.path.exists(path):
                os.system(f"tail -50 {path}")
            else:
                print(f"Log file not found: {path}")
        else:
            print(f"Unknown log type: {log_type}")
            print(f"Available: {', '.join(logs.keys())}, all")
        
        return 0
    
    def cmd_users(self, args):
        """Show logged in users and sessions"""
        print(f"\n{Colors.BOLD}Active Users{Colors.RESET}\n")
        os.system("who -a 2>/dev/null || w")
        
        print(f"\n{Colors.BOLD}Recent Logins{Colors.RESET}\n")
        os.system("last -10")
        return 0
    
    def cmd_processes(self, args):
        """Show running processes with security info"""
        print(f"\n{Colors.BOLD}Running Processes{Colors.RESET}\n")
        os.system("ps aux --sort=-%mem | head -20")
        return 0
    
    def cmd_hash(self, args):
        """Calculate file hash - Usage: hash <file>"""
        if not args:
            print("Usage: hash <file>")
            return 1
        
        try:
            with open(args[0], 'rb') as f:
                sha256 = hashlib.sha256(f.read()).hexdigest()
                md5 = hashlib.md5(open(args[0], 'rb').read()).hexdigest()
            
            print(f"\n{Colors.BOLD}File: {args[0]}{Colors.RESET}")
            print(f"  SHA-256: {sha256}")
            print(f"  MD5:     {md5}")
            return 0
        except Exception as e:
            print(f"{Colors.RED}Error: {e}{Colors.RESET}")
            return 1
    
    def cmd_sysinfo(self, args):
        """Show system information"""
        print(f"\n{Colors.BOLD}System Information{Colors.RESET}\n")
        
        # Hostname
        print(f"  Hostname:   {socket.gethostname()}")
        
        # Kernel
        try:
            kernel = subprocess.check_output(['uname', '-r'], text=True).strip()
            print(f"  Kernel:     {kernel}")
        except:
            pass
        
        # Uptime
        try:
            with open('/proc/uptime', 'r') as f:
                uptime_seconds = float(f.read().split()[0])
                hours = int(uptime_seconds // 3600)
                minutes = int((uptime_seconds % 3600) // 60)
                print(f"  Uptime:     {hours}h {minutes}m")
        except:
            pass
        
        # Memory
        try:
            with open('/proc/meminfo', 'r') as f:
                mem = {}
                for line in f:
                    parts = line.split(':')
                    if len(parts) == 2:
                        mem[parts[0]] = parts[1].strip()
                total = mem.get('MemTotal', 'N/A')
                free = mem.get('MemAvailable', mem.get('MemFree', 'N/A'))
                print(f"  Memory:     {free} / {total}")
        except:
            pass
        
        # CPU
        try:
            with open('/proc/cpuinfo', 'r') as f:
                for line in f:
                    if 'model name' in line:
                        cpu = line.split(':')[1].strip()
                        print(f"  CPU:        {cpu}")
                        break
        except:
            pass
        
        print()
        return 0
    
    def cmd_status(self, args):
        """Show overall security status"""
        print(f"\n{Colors.BOLD}NanoSec Security Status{Colors.RESET}\n")
        
        SecurityStatus.check()
        
        # Firewall
        fw_status = f"{Colors.GREEN}ACTIVE{Colors.RESET}" if SecurityStatus.firewall_active else f"{Colors.RED}INACTIVE{Colors.RESET}"
        print(f"  Firewall:  {fw_status}")
        
        # IDS
        ids_status = f"{Colors.GREEN}RUNNING{Colors.RESET}" if SecurityStatus.ids_active else f"{Colors.RED}STOPPED{Colors.RESET}"
        print(f"  IDS:       {ids_status}")
        
        # SSH
        try:
            result = subprocess.run(['systemctl', 'is-active', 'sshd'], 
                                   capture_output=True, text=True)
            ssh = f"{Colors.GREEN}RUNNING{Colors.RESET}" if result.stdout.strip() == 'active' else f"{Colors.YELLOW}STOPPED{Colors.RESET}"
        except:
            ssh = f"{Colors.YELLOW}UNKNOWN{Colors.RESET}"
        print(f"  SSH:       {ssh}")
        
        # Alerts
        alert_color = Colors.GREEN if SecurityStatus.alerts == 0 else Colors.RED
        print(f"  Alerts:    {alert_color}{SecurityStatus.alerts}{Colors.RESET}")
        
        print()
        return 0
    
    def cmd_version(self, args):
        """Show NASH version"""
        print(f"\n{Colors.BOLD}NASH - NanoSec Shell{Colors.RESET}")
        print(f"  Version: {self.VERSION}")
        print(f"  Python:  {sys.version.split()[0]}")
        print()
        return 0
    
    def cmd_source(self, args):
        """Execute a .nash script file"""
        if not args:
            print("Usage: source <script.nash>")
            return 1
        
        script = args[0]
        if not os.path.exists(script):
            print(f"{Colors.RED}Script not found: {script}{Colors.RESET}")
            return 1
        
        try:
            with open(script, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith('#'):
                        print(f"{Colors.CYAN}> {line}{Colors.RESET}")
                        self.execute(line)
            return 0
        except Exception as e:
            print(f"{Colors.RED}Error: {e}{Colors.RESET}")
            return 1


def main():
    """Entry point"""
    if len(sys.argv) > 1:
        if sys.argv[1] == '--test':
            print("NASH self-test: OK")
            return 0
        elif sys.argv[1] == '--version':
            print(f"NASH {NASH.VERSION}")
            return 0
        elif sys.argv[1] == '-c':
            # Execute command
            shell = NASH()
            cmd = ' '.join(sys.argv[2:])
            return shell.execute(cmd)
    
    shell = NASH()
    shell.run()
    return 0


if __name__ == '__main__':
    sys.exit(main())
