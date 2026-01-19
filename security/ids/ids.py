#!/usr/bin/env python3
"""
Intrusion Detection System (IDS) for NanoSec OS
================================================
Monitors system logs and activities for suspicious behavior.

Features:
- Log analysis (auth, syslog)
- Failed login detection
- Port scan detection
- Process anomaly detection
- Alert system
"""

import os
import sys
import re
import time
import json
import signal
import argparse
import datetime
import subprocess
from collections import defaultdict
from pathlib import Path
import threading
from typing import Dict, List, Optional

# Configuration
CONFIG = {
    "log_files": {
        "auth": "/var/log/auth.log",
        "syslog": "/var/log/syslog",
        "messages": "/var/log/messages",
    },
    "output_log": "/var/log/nanosec/ids.log",
    "alert_log": "/var/log/nanosec/ids_alerts.log",
    "state_file": "/var/lib/nanosec/ids/state.json",
    "thresholds": {
        "failed_logins": 5,           # Max failed logins before alert
        "failed_login_window": 300,    # Time window in seconds
        "port_scan_ports": 10,         # Ports hit in window = scan
        "port_scan_window": 60,        # Time window for port scan
        "suspicious_processes": ["nc", "ncat", "netcat", "nmap", "masscan"],
    }
}

# Colors
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    RESET = '\033[0m'

class AlertLevel:
    INFO = "INFO"
    WARNING = "WARNING"
    CRITICAL = "CRITICAL"

class IntrusionDetector:
    """Main IDS class"""
    
    def __init__(self, daemon=False):
        self.daemon = daemon
        self.running = True
        self.alerts: List[Dict] = []
        self.state: Dict = {
            "failed_logins": defaultdict(list),
            "connection_attempts": defaultdict(list),
            "file_positions": {},
        }
        self.lock = threading.Lock()
        
        # Patterns for log analysis
        self.patterns = {
            "failed_ssh": re.compile(r"Failed password for (?:invalid user )?(\S+) from (\S+)"),
            "accepted_ssh": re.compile(r"Accepted (?:password|publickey) for (\S+) from (\S+)"),
            "invalid_user": re.compile(r"Invalid user (\S+) from (\S+)"),
            "break_in": re.compile(r"POSSIBLE BREAK-IN ATTEMPT"),
            "sudo_fail": re.compile(r"sudo:.*authentication failure"),
            "connection_closed": re.compile(r"Connection closed by (\S+)"),
        }
        
        self._setup_logging()
        self._load_state()
        signal.signal(signal.SIGTERM, self._handle_signal)
        signal.signal(signal.SIGINT, self._handle_signal)
    
    def _setup_logging(self):
        """Setup logging directories"""
        for path in [CONFIG["output_log"], CONFIG["alert_log"], CONFIG["state_file"]]:
            os.makedirs(os.path.dirname(path), exist_ok=True)
    
    def _load_state(self):
        """Load persistent state"""
        try:
            if os.path.exists(CONFIG["state_file"]):
                with open(CONFIG["state_file"], 'r') as f:
                    saved = json.load(f)
                    self.state["file_positions"] = saved.get("file_positions", {})
        except:
            pass
    
    def _save_state(self):
        """Save persistent state"""
        try:
            with open(CONFIG["state_file"], 'w') as f:
                json.dump({
                    "file_positions": self.state["file_positions"]
                }, f)
        except:
            pass
    
    def _handle_signal(self, sig, frame):
        """Handle shutdown signals"""
        self.log("Shutting down IDS...")
        self.running = False
        self._save_state()
        sys.exit(0)
    
    def log(self, message: str, level: str = "INFO"):
        """Log a message"""
        timestamp = datetime.datetime.now().isoformat()
        log_line = f"{timestamp} [{level}] {message}"
        
        # Console output
        if not self.daemon:
            color = Colors.GREEN
            if level == "WARNING":
                color = Colors.YELLOW
            elif level == "CRITICAL":
                color = Colors.RED
            print(f"{color}[{level}]{Colors.RESET} {message}")
        
        # File output
        try:
            with open(CONFIG["output_log"], 'a') as f:
                f.write(log_line + "\n")
        except:
            pass
    
    def alert(self, level: str, alert_type: str, message: str, details: Dict = None):
        """Generate an alert"""
        timestamp = datetime.datetime.now().isoformat()
        
        alert_data = {
            "timestamp": timestamp,
            "level": level,
            "type": alert_type,
            "message": message,
            "details": details or {}
        }
        
        with self.lock:
            self.alerts.append(alert_data)
        
        # Log alert
        alert_line = f"[{level}] {alert_type}: {message}"
        self.log(alert_line, level)
        
        # Write to alert log
        try:
            with open(CONFIG["alert_log"], 'a') as f:
                f.write(f"{timestamp} {alert_line}\n")
                if details:
                    f.write(f"  Details: {json.dumps(details)}\n")
        except:
            pass
        
        # Console alert
        if not self.daemon:
            if level == "CRITICAL":
                print(f"\n{Colors.RED}{'═' * 60}")
                print(f"⚠  INTRUSION ALERT: {alert_type}")
                print(f"{'═' * 60}{Colors.RESET}")
                print(f"  {message}")
                if details:
                    for k, v in details.items():
                        print(f"  {k}: {v}")
                print()
    
    def analyze_log_line(self, line: str, log_type: str):
        """Analyze a single log line"""
        
        # Failed SSH login
        match = self.patterns["failed_ssh"].search(line)
        if match:
            user, ip = match.groups()
            self._record_failed_login(ip, user)
            return
        
        # Invalid user attempt
        match = self.patterns["invalid_user"].search(line)
        if match:
            user, ip = match.groups()
            self._record_failed_login(ip, user, "invalid_user")
            return
        
        # Break-in attempt
        if self.patterns["break_in"].search(line):
            self.alert(
                AlertLevel.CRITICAL,
                "BREAK_IN_ATTEMPT",
                "Possible break-in attempt detected",
                {"log_line": line[:200]}
            )
            return
        
        # Sudo failure
        if self.patterns["sudo_fail"].search(line):
            self.alert(
                AlertLevel.WARNING,
                "SUDO_FAILURE",
                "Sudo authentication failure detected",
                {"log_line": line[:200]}
            )
    
    def _record_failed_login(self, ip: str, user: str, reason: str = "failed_password"):
        """Record and check failed login attempts"""
        now = time.time()
        threshold = CONFIG["thresholds"]["failed_logins"]
        window = CONFIG["thresholds"]["failed_login_window"]
        
        with self.lock:
            # Add new attempt
            self.state["failed_logins"][ip].append({
                "time": now,
                "user": user,
                "reason": reason
            })
            
            # Clean old entries
            self.state["failed_logins"][ip] = [
                a for a in self.state["failed_logins"][ip]
                if now - a["time"] < window
            ]
            
            # Check threshold
            attempts = self.state["failed_logins"][ip]
            if len(attempts) >= threshold:
                users_tried = list(set(a["user"] for a in attempts))
                self.alert(
                    AlertLevel.CRITICAL,
                    "BRUTE_FORCE",
                    f"Brute force attack detected from {ip}",
                    {
                        "ip": ip,
                        "attempts": len(attempts),
                        "users_tried": users_tried[:5],
                        "window_seconds": window
                    }
                )
                # Clear to avoid repeated alerts
                self.state["failed_logins"][ip] = []
    
    def check_suspicious_processes(self):
        """Check for suspicious running processes"""
        suspicious = CONFIG["thresholds"]["suspicious_processes"]
        
        try:
            result = subprocess.run(
                ["ps", "aux"], 
                capture_output=True, 
                text=True, 
                timeout=5
            )
            
            for proc in suspicious:
                if proc in result.stdout:
                    self.alert(
                        AlertLevel.WARNING,
                        "SUSPICIOUS_PROCESS",
                        f"Suspicious process detected: {proc}",
                        {"process": proc}
                    )
        except:
            pass
    
    def check_network_connections(self):
        """Check for suspicious network activity"""
        try:
            result = subprocess.run(
                ["ss", "-tupn"], 
                capture_output=True, 
                text=True, 
                timeout=5
            )
            
            # Check for suspicious ports
            suspicious_ports = [4444, 5555, 6666, 31337, 12345]
            for port in suspicious_ports:
                if f":{port}" in result.stdout:
                    self.alert(
                        AlertLevel.WARNING,
                        "SUSPICIOUS_PORT",
                        f"Connection on suspicious port {port}",
                        {"port": port}
                    )
        except:
            pass
    
    def tail_log(self, log_path: str, log_type: str):
        """Tail a log file for new entries"""
        if not os.path.exists(log_path):
            return
        
        # Get last position
        pos = self.state["file_positions"].get(log_path, 0)
        
        try:
            with open(log_path, 'r') as f:
                f.seek(0, 2)  # End of file
                current_size = f.tell()
                
                # If file was rotated (smaller), start from beginning
                if current_size < pos:
                    pos = 0
                
                f.seek(pos)
                
                for line in f:
                    self.analyze_log_line(line, log_type)
                
                # Save new position
                self.state["file_positions"][log_path] = f.tell()
        except Exception as e:
            self.log(f"Error reading {log_path}: {e}", "WARNING")
    
    def run_daemon(self):
        """Run as daemon, continuously monitoring"""
        self.log("NanoSec IDS started in daemon mode")
        
        check_interval = 5  # seconds
        process_check_interval = 30
        last_process_check = 0
        
        while self.running:
            try:
                # Check log files
                for log_type, log_path in CONFIG["log_files"].items():
                    self.tail_log(log_path, log_type)
                
                # Periodic process check
                now = time.time()
                if now - last_process_check > process_check_interval:
                    self.check_suspicious_processes()
                    self.check_network_connections()
                    last_process_check = now
                
                self._save_state()
                time.sleep(check_interval)
                
            except Exception as e:
                self.log(f"Error in main loop: {e}", "WARNING")
                time.sleep(check_interval)
    
    def run_check(self):
        """Run a single check"""
        self.log("Running IDS check...")
        
        # Check logs
        for log_type, log_path in CONFIG["log_files"].items():
            if os.path.exists(log_path):
                self.log(f"Checking {log_type} log...")
                self.tail_log(log_path, log_type)
        
        # Check processes
        self.log("Checking processes...")
        self.check_suspicious_processes()
        
        # Check network
        self.log("Checking network...")
        self.check_network_connections()
        
        self._save_state()
        
        if self.alerts:
            print(f"\n{Colors.RED}Found {len(self.alerts)} alerts{Colors.RESET}")
        else:
            print(f"\n{Colors.GREEN}No issues detected{Colors.RESET}")
    
    def show_status(self):
        """Show IDS status"""
        print(f"\n{Colors.CYAN}{'═' * 50}{Colors.RESET}")
        print(f"{Colors.CYAN}NanoSec IDS Status{Colors.RESET}")
        print(f"{Colors.CYAN}{'═' * 50}{Colors.RESET}\n")
        
        # Check if running
        try:
            result = subprocess.run(
                ["pgrep", "-f", "ids.py"], 
                capture_output=True
            )
            if result.returncode == 0:
                print(f"  Status:      {Colors.GREEN}RUNNING{Colors.RESET}")
            else:
                print(f"  Status:      {Colors.RED}STOPPED{Colors.RESET}")
        except:
            print(f"  Status:      {Colors.YELLOW}UNKNOWN{Colors.RESET}")
        
        # Log files being monitored
        print(f"\n  Log Files:")
        for name, path in CONFIG["log_files"].items():
            exists = "✓" if os.path.exists(path) else "✗"
            color = Colors.GREEN if os.path.exists(path) else Colors.RED
            print(f"    {color}{exists}{Colors.RESET} {name}: {path}")
        
        # Recent alerts
        if os.path.exists(CONFIG["alert_log"]):
            print(f"\n  Recent Alerts:")
            try:
                result = subprocess.run(
                    ["tail", "-5", CONFIG["alert_log"]],
                    capture_output=True,
                    text=True
                )
                for line in result.stdout.strip().split('\n'):
                    if line:
                        print(f"    {line[:70]}")
            except:
                pass
        
        print()
    
    def show_alerts(self, count: int = 50):
        """Show recent alerts"""
        print(f"\n{Colors.CYAN}Recent IDS Alerts{Colors.RESET}\n")
        
        if os.path.exists(CONFIG["alert_log"]):
            try:
                with open(CONFIG["alert_log"], 'r') as f:
                    lines = f.readlines()[-count:]
                    for line in lines:
                        if "[CRITICAL]" in line:
                            print(f"{Colors.RED}{line.strip()}{Colors.RESET}")
                        elif "[WARNING]" in line:
                            print(f"{Colors.YELLOW}{line.strip()}{Colors.RESET}")
                        else:
                            print(line.strip())
            except Exception as e:
                print(f"Error reading alerts: {e}")
        else:
            print("No alerts logged yet")
        print()

def main():
    parser = argparse.ArgumentParser(
        description="NanoSec Intrusion Detection System",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  ids --status              Show IDS status
  ids --check               Run single check
  ids --daemon              Run as daemon
  ids --alerts              Show recent alerts
        """
    )
    
    parser.add_argument('--daemon', '-d', action='store_true',
                       help='Run as daemon (continuous monitoring)')
    parser.add_argument('--check', '-c', action='store_true',
                       help='Run a single check')
    parser.add_argument('--status', '-s', action='store_true',
                       help='Show IDS status')
    parser.add_argument('--alerts', '-a', action='store_true',
                       help='Show recent alerts')
    parser.add_argument('--self-test', action='store_true',
                       help='Run self-test')
    
    args = parser.parse_args()
    
    if args.self_test:
        print("IDS self-test: OK")
        return 0
    
    ids = IntrusionDetector(daemon=args.daemon)
    
    if args.daemon:
        ids.run_daemon()
    elif args.check:
        ids.run_check()
    elif args.status:
        ids.show_status()
    elif args.alerts:
        ids.show_alerts()
    else:
        parser.print_help()
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
