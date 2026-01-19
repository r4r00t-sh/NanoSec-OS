"""
NASH System Commands
====================
System monitoring and management commands
"""

import os
import subprocess
from typing import Dict, List, Optional
from datetime import datetime


def get_system_info() -> Dict:
    """
    Get comprehensive system information
    
    Returns:
        Dict with system info
    """
    info = {}
    
    # Hostname
    try:
        import socket
        info['hostname'] = socket.gethostname()
    except:
        info['hostname'] = 'unknown'
    
    # Kernel
    try:
        with open('/proc/version', 'r') as f:
            info['kernel'] = f.read().strip().split()[2]
    except:
        info['kernel'] = 'unknown'
    
    # Uptime
    try:
        with open('/proc/uptime', 'r') as f:
            uptime_seconds = float(f.read().split()[0])
            hours = int(uptime_seconds // 3600)
            minutes = int((uptime_seconds % 3600) // 60)
            info['uptime'] = f"{hours}h {minutes}m"
    except:
        info['uptime'] = 'unknown'
    
    # Load average
    try:
        with open('/proc/loadavg', 'r') as f:
            parts = f.read().split()
            info['load_avg'] = {
                '1min': float(parts[0]),
                '5min': float(parts[1]),
                '15min': float(parts[2])
            }
    except:
        pass
    
    # Memory
    try:
        with open('/proc/meminfo', 'r') as f:
            meminfo = {}
            for line in f:
                parts = line.split(':')
                if len(parts) == 2:
                    key = parts[0].strip()
                    value = parts[1].strip()
                    meminfo[key] = value
            
            info['memory'] = {
                'total': meminfo.get('MemTotal', 'unknown'),
                'free': meminfo.get('MemFree', 'unknown'),
                'available': meminfo.get('MemAvailable', 'unknown'),
                'buffers': meminfo.get('Buffers', 'unknown'),
                'cached': meminfo.get('Cached', 'unknown')
            }
    except:
        pass
    
    # CPU
    try:
        with open('/proc/cpuinfo', 'r') as f:
            for line in f:
                if 'model name' in line:
                    info['cpu'] = line.split(':')[1].strip()
                    break
    except:
        pass
    
    return info


def get_processes(sort_by: str = 'cpu') -> List[Dict]:
    """
    Get list of running processes
    
    Args:
        sort_by: Sort field ('cpu', 'mem', 'pid')
    
    Returns:
        List of process info dicts
    """
    processes = []
    
    try:
        result = subprocess.run(
            ['ps', 'aux', '--sort=-%cpu'],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode == 0:
            lines = result.stdout.strip().split('\n')[1:]  # Skip header
            for line in lines[:50]:  # Limit to 50
                parts = line.split(None, 10)
                if len(parts) >= 11:
                    processes.append({
                        'user': parts[0],
                        'pid': parts[1],
                        'cpu': parts[2],
                        'mem': parts[3],
                        'command': parts[10][:50]  # Truncate command
                    })
    except:
        pass
    
    return processes


def get_disk_usage() -> List[Dict]:
    """
    Get disk usage information
    
    Returns:
        List of disk info dicts
    """
    disks = []
    
    try:
        result = subprocess.run(
            ['df', '-h'],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode == 0:
            lines = result.stdout.strip().split('\n')[1:]
            for line in lines:
                parts = line.split()
                if len(parts) >= 6:
                    disks.append({
                        'filesystem': parts[0],
                        'size': parts[1],
                        'used': parts[2],
                        'available': parts[3],
                        'percent': parts[4],
                        'mounted_on': parts[5]
                    })
    except:
        pass
    
    return disks


def get_users() -> List[Dict]:
    """
    Get logged in users
    
    Returns:
        List of user info dicts
    """
    users = []
    
    try:
        result = subprocess.run(
            ['who'],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode == 0:
            for line in result.stdout.strip().split('\n'):
                if line:
                    parts = line.split()
                    users.append({
                        'user': parts[0] if parts else 'unknown',
                        'tty': parts[1] if len(parts) > 1 else '',
                        'time': ' '.join(parts[2:4]) if len(parts) > 3 else ''
                    })
    except:
        pass
    
    return users


def get_last_logins(count: int = 10) -> List[Dict]:
    """
    Get recent login history
    
    Args:
        count: Number of entries to return
    
    Returns:
        List of login info dicts
    """
    logins = []
    
    try:
        result = subprocess.run(
            ['last', '-n', str(count)],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode == 0:
            for line in result.stdout.strip().split('\n'):
                if line and not line.startswith('wtmp'):
                    parts = line.split()
                    if parts:
                        logins.append({
                            'user': parts[0],
                            'tty': parts[1] if len(parts) > 1 else '',
                            'from': parts[2] if len(parts) > 2 else '',
                            'time': ' '.join(parts[3:7]) if len(parts) > 6 else ''
                        })
    except:
        pass
    
    return logins


def get_failed_logins(count: int = 10) -> List[Dict]:
    """
    Get failed login attempts
    
    Args:
        count: Number of entries to return
    
    Returns:
        List of failed login info
    """
    failed = []
    
    try:
        result = subprocess.run(
            ['lastb', '-n', str(count)],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode == 0:
            for line in result.stdout.strip().split('\n'):
                if line and not line.startswith('btmp'):
                    parts = line.split()
                    if parts:
                        failed.append({
                            'user': parts[0],
                            'tty': parts[1] if len(parts) > 1 else '',
                            'from': parts[2] if len(parts) > 2 else '',
                            'time': ' '.join(parts[3:7]) if len(parts) > 6 else ''
                        })
    except:
        pass
    
    return failed
