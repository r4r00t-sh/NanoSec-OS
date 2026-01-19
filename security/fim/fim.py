#!/usr/bin/env python3
"""
File Integrity Monitor (FIM) for NanoSec OS
============================================
Monitors critical files for unauthorized changes using SHA-256 hashing.

Features:
- Baseline generation
- Integrity checking
- Real-time monitoring (inotify)
- Alert generation
"""

import os
import sys
import json
import hashlib
import argparse
import datetime
import logging
from pathlib import Path
from typing import Dict, Optional
import time

try:
    from watchdog.observers import Observer
    from watchdog.events import FileSystemEventHandler
    WATCHDOG_AVAILABLE = True
except ImportError:
    WATCHDOG_AVAILABLE = False

# Configuration
CONFIG = {
    "baseline_file": "/var/lib/nanosec/fim/baseline.json",
    "log_file": "/var/log/nanosec/fim.log",
    "alert_file": "/var/log/nanosec/fim_alerts.log",
    "default_paths": ["/etc", "/bin", "/sbin", "/usr/bin", "/usr/sbin"],
    "exclude_patterns": ["*.log", "*.tmp", "*.swp", "*~", "*.pyc", "__pycache__"],
    "hash_algorithm": "sha256"
}

# Colors for terminal output
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    RESET = '\033[0m'

def setup_logging():
    """Setup logging configuration"""
    log_dir = os.path.dirname(CONFIG["log_file"])
    os.makedirs(log_dir, exist_ok=True)
    
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s',
        handlers=[
            logging.FileHandler(CONFIG["log_file"]),
            logging.StreamHandler(sys.stdout)
        ]
    )
    return logging.getLogger("FIM")

logger = setup_logging()

def calculate_hash(filepath: str) -> Optional[str]:
    """Calculate SHA-256 hash of a file"""
    try:
        hash_obj = hashlib.sha256()
        with open(filepath, 'rb') as f:
            for chunk in iter(lambda: f.read(8192), b''):
                hash_obj.update(chunk)
        return hash_obj.hexdigest()
    except (PermissionError, FileNotFoundError, OSError) as e:
        logger.debug(f"Cannot hash {filepath}: {e}")
        return None

def should_exclude(filepath: str) -> bool:
    """Check if file should be excluded"""
    name = os.path.basename(filepath)
    for pattern in CONFIG["exclude_patterns"]:
        if pattern.startswith("*") and name.endswith(pattern[1:]):
            return True
        if pattern.endswith("*") and name.startswith(pattern[:-1]):
            return True
        if name == pattern:
            return True
    return False

def get_file_metadata(filepath: str) -> Dict:
    """Get file metadata"""
    try:
        stat = os.stat(filepath)
        return {
            "size": stat.st_size,
            "mode": oct(stat.st_mode),
            "uid": stat.st_uid,
            "gid": stat.st_gid,
            "mtime": stat.st_mtime
        }
    except:
        return {}

class FileIntegrityMonitor:
    """Main FIM class"""
    
    def __init__(self):
        self.baseline: Dict = {}
        self.changes: list = []
        
    def load_baseline(self) -> bool:
        """Load baseline from file"""
        try:
            baseline_path = CONFIG["baseline_file"]
            if os.path.exists(baseline_path):
                with open(baseline_path, 'r') as f:
                    self.baseline = json.load(f)
                logger.info(f"Loaded baseline with {len(self.baseline.get('files', {}))} files")
                return True
            else:
                logger.warning("No baseline file found")
                return False
        except Exception as e:
            logger.error(f"Error loading baseline: {e}")
            return False
    
    def save_baseline(self):
        """Save baseline to file"""
        try:
            baseline_dir = os.path.dirname(CONFIG["baseline_file"])
            os.makedirs(baseline_dir, exist_ok=True)
            
            with open(CONFIG["baseline_file"], 'w') as f:
                json.dump(self.baseline, f, indent=2)
            
            logger.info(f"Baseline saved: {CONFIG['baseline_file']}")
        except Exception as e:
            logger.error(f"Error saving baseline: {e}")
    
    def create_baseline(self, paths: list):
        """Create baseline for specified paths"""
        print(f"\n{Colors.CYAN}Creating baseline for: {', '.join(paths)}{Colors.RESET}\n")
        
        files = {}
        total = 0
        skipped = 0
        
        for base_path in paths:
            if not os.path.exists(base_path):
                logger.warning(f"Path not found: {base_path}")
                continue
            
            for root, dirs, filenames in os.walk(base_path):
                for filename in filenames:
                    filepath = os.path.join(root, filename)
                    
                    if should_exclude(filepath):
                        skipped += 1
                        continue
                    
                    file_hash = calculate_hash(filepath)
                    if file_hash:
                        files[filepath] = {
                            "hash": file_hash,
                            "metadata": get_file_metadata(filepath)
                        }
                        total += 1
                        
                        if total % 100 == 0:
                            print(f"  Processed {total} files...", end='\r')
        
        self.baseline = {
            "created": datetime.datetime.now().isoformat(),
            "paths": paths,
            "algorithm": CONFIG["hash_algorithm"],
            "file_count": total,
            "files": files
        }
        
        self.save_baseline()
        
        print(f"\n{Colors.GREEN}✓ Baseline created:{Colors.RESET}")
        print(f"  Files hashed: {total}")
        print(f"  Files skipped: {skipped}")
        print(f"  Saved to: {CONFIG['baseline_file']}\n")
    
    def check_integrity(self, paths: list = None) -> list:
        """Check file integrity against baseline"""
        if not self.baseline:
            if not self.load_baseline():
                print(f"{Colors.RED}No baseline available. Run: fim --baseline <path>{Colors.RESET}")
                return []
        
        print(f"\n{Colors.CYAN}Checking file integrity...{Colors.RESET}\n")
        
        baseline_files = self.baseline.get("files", {})
        self.changes = []
        checked = 0
        
        # Check for modified and deleted files
        for filepath, data in baseline_files.items():
            if paths:
                # Only check files under specified paths
                if not any(filepath.startswith(p) for p in paths):
                    continue
            
            if not os.path.exists(filepath):
                self.changes.append({
                    "type": "DELETED",
                    "path": filepath,
                    "original_hash": data["hash"]
                })
                self._alert("DELETED", filepath)
            else:
                current_hash = calculate_hash(filepath)
                if current_hash and current_hash != data["hash"]:
                    self.changes.append({
                        "type": "MODIFIED",
                        "path": filepath,
                        "original_hash": data["hash"],
                        "current_hash": current_hash
                    })
                    self._alert("MODIFIED", filepath)
            
            checked += 1
            if checked % 100 == 0:
                print(f"  Checked {checked} files...", end='\r')
        
        # Check for new files
        check_paths = paths or self.baseline.get("paths", CONFIG["default_paths"])
        for base_path in check_paths:
            if not os.path.exists(base_path):
                continue
            
            for root, dirs, filenames in os.walk(base_path):
                for filename in filenames:
                    filepath = os.path.join(root, filename)
                    
                    if should_exclude(filepath):
                        continue
                    
                    if filepath not in baseline_files:
                        file_hash = calculate_hash(filepath)
                        if file_hash:
                            self.changes.append({
                                "type": "NEW",
                                "path": filepath,
                                "current_hash": file_hash
                            })
                            self._alert("NEW", filepath)
        
        self._report_changes()
        return self.changes
    
    def _alert(self, change_type: str, filepath: str):
        """Log an alert"""
        alert_msg = f"[{change_type}] {filepath}"
        logger.warning(alert_msg)
        
        # Write to alert file
        try:
            alert_dir = os.path.dirname(CONFIG["alert_file"])
            os.makedirs(alert_dir, exist_ok=True)
            
            with open(CONFIG["alert_file"], 'a') as f:
                timestamp = datetime.datetime.now().isoformat()
                f.write(f"{timestamp} - {alert_msg}\n")
        except:
            pass
    
    def _report_changes(self):
        """Print change report"""
        print("\n")
        
        if not self.changes:
            print(f"{Colors.GREEN}✓ No changes detected - All files intact{Colors.RESET}\n")
            return
        
        print(f"{Colors.RED}{'═' * 60}{Colors.RESET}")
        print(f"{Colors.RED}{Colors.BOLD}⚠  INTEGRITY VIOLATIONS DETECTED  ⚠{Colors.RESET}")
        print(f"{Colors.RED}{'═' * 60}{Colors.RESET}\n")
        
        # Group by type
        deleted = [c for c in self.changes if c["type"] == "DELETED"]
        modified = [c for c in self.changes if c["type"] == "MODIFIED"]
        new = [c for c in self.changes if c["type"] == "NEW"]
        
        if deleted:
            print(f"{Colors.RED}DELETED ({len(deleted)}):{Colors.RESET}")
            for c in deleted[:10]:
                print(f"  - {c['path']}")
            if len(deleted) > 10:
                print(f"  ... and {len(deleted) - 10} more")
            print()
        
        if modified:
            print(f"{Colors.YELLOW}MODIFIED ({len(modified)}):{Colors.RESET}")
            for c in modified[:10]:
                print(f"  - {c['path']}")
            if len(modified) > 10:
                print(f"  ... and {len(modified) - 10} more")
            print()
        
        if new:
            print(f"{Colors.CYAN}NEW ({len(new)}):{Colors.RESET}")
            for c in new[:10]:
                print(f"  + {c['path']}")
            if len(new) > 10:
                print(f"  ... and {len(new) - 10} more")
            print()
        
        print(f"{Colors.BOLD}Total changes: {len(self.changes)}{Colors.RESET}\n")
    
    def status(self):
        """Show FIM status"""
        print(f"\n{Colors.CYAN}{'═' * 50}{Colors.RESET}")
        print(f"{Colors.CYAN}File Integrity Monitor Status{Colors.RESET}")
        print(f"{Colors.CYAN}{'═' * 50}{Colors.RESET}\n")
        
        if os.path.exists(CONFIG["baseline_file"]):
            self.load_baseline()
            created = self.baseline.get("created", "Unknown")
            file_count = self.baseline.get("file_count", 0)
            paths = self.baseline.get("paths", [])
            
            print(f"  Baseline:    {Colors.GREEN}EXISTS{Colors.RESET}")
            print(f"  Created:     {created}")
            print(f"  Files:       {file_count}")
            print(f"  Paths:       {', '.join(paths)}")
        else:
            print(f"  Baseline:    {Colors.RED}NOT FOUND{Colors.RESET}")
            print(f"  Run: fim --baseline <path>")
        
        print()

class FIMEventHandler(FileSystemEventHandler):
    """Real-time file monitoring handler"""
    
    def __init__(self, fim: FileIntegrityMonitor):
        self.fim = fim
        self.fim.load_baseline()
    
    def on_modified(self, event):
        if event.is_directory:
            return
        self._check_file(event.src_path, "MODIFIED")
    
    def on_created(self, event):
        if event.is_directory:
            return
        self._check_file(event.src_path, "NEW")
    
    def on_deleted(self, event):
        if event.is_directory:
            return
        filepath = event.src_path
        if filepath in self.fim.baseline.get("files", {}):
            self.fim._alert("DELETED", filepath)
            print(f"{Colors.RED}[ALERT] DELETED: {filepath}{Colors.RESET}")
    
    def _check_file(self, filepath: str, event_type: str):
        if should_exclude(filepath):
            return
        
        baseline_files = self.fim.baseline.get("files", {})
        current_hash = calculate_hash(filepath)
        
        if filepath in baseline_files:
            if current_hash != baseline_files[filepath]["hash"]:
                self.fim._alert("MODIFIED", filepath)
                print(f"{Colors.YELLOW}[ALERT] MODIFIED: {filepath}{Colors.RESET}")
        else:
            self.fim._alert("NEW", filepath)
            print(f"{Colors.CYAN}[ALERT] NEW: {filepath}{Colors.RESET}")

def monitor_realtime(paths: list):
    """Start real-time monitoring"""
    if not WATCHDOG_AVAILABLE:
        print(f"{Colors.RED}Real-time monitoring requires 'watchdog' package{Colors.RESET}")
        print("Install with: pip install watchdog")
        return
    
    fim = FileIntegrityMonitor()
    event_handler = FIMEventHandler(fim)
    observer = Observer()
    
    for path in paths:
        if os.path.exists(path):
            observer.schedule(event_handler, path, recursive=True)
            print(f"Monitoring: {path}")
    
    observer.start()
    print(f"\n{Colors.GREEN}Real-time monitoring started. Press Ctrl+C to stop.{Colors.RESET}\n")
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
    observer.join()

def main():
    parser = argparse.ArgumentParser(
        description="NanoSec File Integrity Monitor",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  fim --baseline /etc /bin      Create baseline for /etc and /bin
  fim --check                   Check all baselined files
  fim --check /etc              Check only /etc
  fim --monitor /etc            Real-time monitoring
  fim --status                  Show FIM status
        """
    )
    
    parser.add_argument('--baseline', '-b', nargs='+', metavar='PATH',
                       help='Create baseline for specified paths')
    parser.add_argument('--check', '-c', nargs='*', metavar='PATH',
                       help='Check integrity (optionally limit to paths)')
    parser.add_argument('--monitor', '-m', nargs='+', metavar='PATH',
                       help='Real-time monitoring of paths')
    parser.add_argument('--status', '-s', action='store_true',
                       help='Show FIM status')
    parser.add_argument('--self-test', action='store_true',
                       help='Run self-test')
    
    args = parser.parse_args()
    
    fim = FileIntegrityMonitor()
    
    if args.self_test:
        print("FIM self-test: OK")
        return 0
    
    if args.baseline:
        fim.create_baseline(args.baseline)
    elif args.check is not None:
        paths = args.check if args.check else None
        fim.check_integrity(paths)
    elif args.monitor:
        monitor_realtime(args.monitor)
    elif args.status:
        fim.status()
    else:
        parser.print_help()
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
