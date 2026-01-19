"""
NASH Network Commands
=====================
Network monitoring and analysis commands
"""

import socket
import subprocess
from typing import List, Dict, Optional


def get_network_interfaces() -> List[Dict]:
    """
    Get list of network interfaces
    
    Returns:
        List of interface information dicts
    """
    interfaces = []
    
    try:
        result = subprocess.run(
            ['ip', '-j', 'addr'],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode == 0:
            import json
            data = json.loads(result.stdout)
            for iface in data:
                interfaces.append({
                    "name": iface.get("ifname"),
                    "state": iface.get("operstate"),
                    "mac": iface.get("address"),
                    "addresses": [
                        addr.get("local") 
                        for addr in iface.get("addr_info", [])
                    ]
                })
    except:
        # Fallback method
        try:
            result = subprocess.run(
                ['ip', 'addr'],
                capture_output=True,
                text=True
            )
            # Parse output manually
        except:
            pass
    
    return interfaces


def get_connections() -> List[Dict]:
    """
    Get active network connections
    
    Returns:
        List of connection information
    """
    connections = []
    
    try:
        result = subprocess.run(
            ['ss', '-tupn'],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode == 0:
            lines = result.stdout.strip().split('\n')[1:]  # Skip header
            for line in lines:
                parts = line.split()
                if len(parts) >= 5:
                    connections.append({
                        "protocol": parts[0],
                        "state": parts[1] if len(parts) > 1 else "",
                        "local": parts[4] if len(parts) > 4 else "",
                        "remote": parts[5] if len(parts) > 5 else "",
                        "process": parts[-1] if len(parts) > 6 else ""
                    })
    except:
        pass
    
    return connections


def resolve_hostname(hostname: str) -> Optional[str]:
    """
    Resolve hostname to IP address
    
    Args:
        hostname: Hostname to resolve
    
    Returns:
        IP address or None
    """
    try:
        return socket.gethostbyname(hostname)
    except:
        return None


def reverse_lookup(ip: str) -> Optional[str]:
    """
    Reverse DNS lookup
    
    Args:
        ip: IP address
    
    Returns:
        Hostname or None
    """
    try:
        return socket.gethostbyaddr(ip)[0]
    except:
        return None


def check_port(host: str, port: int, timeout: float = 2.0) -> bool:
    """
    Check if a port is open
    
    Args:
        host: Target host
        port: Port number
        timeout: Connection timeout
    
    Returns:
        True if port is open
    """
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        result = sock.connect_ex((host, port))
        sock.close()
        return result == 0
    except:
        return False


def get_routing_table() -> List[Dict]:
    """
    Get system routing table
    
    Returns:
        List of routes
    """
    routes = []
    
    try:
        result = subprocess.run(
            ['ip', 'route'],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode == 0:
            for line in result.stdout.strip().split('\n'):
                parts = line.split()
                if parts:
                    routes.append({
                        "destination": parts[0],
                        "via": parts[parts.index('via') + 1] if 'via' in parts else None,
                        "dev": parts[parts.index('dev') + 1] if 'dev' in parts else None
                    })
    except:
        pass
    
    return routes
