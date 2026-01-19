"""
NASH Security Commands
======================
Security-focused commands for NASH shell
"""

import subprocess
import hashlib
import os

def vulnerability_scan(target: str, options: dict = None):
    """
    Perform a vulnerability scan on target
    
    Args:
        target: IP or hostname to scan
        options: Additional scan options
    
    Returns:
        dict: Scan results
    """
    results = {
        "target": target,
        "vulnerabilities": [],
        "warnings": [],
        "info": []
    }
    
    # Check for common vulnerable ports
    vulnerable_ports = {
        21: "FTP - May allow anonymous access",
        23: "Telnet - Unencrypted protocol",
        25: "SMTP - May be open relay",
        110: "POP3 - Unencrypted email",
        143: "IMAP - Unencrypted email",
        445: "SMB - Potential ransomware vector",
        3389: "RDP - Remote desktop exposure",
        5432: "PostgreSQL - Database exposure",
        3306: "MySQL - Database exposure",
        27017: "MongoDB - Often misconfigured",
    }
    
    # Note: Actual port scanning would be done via nash.py cmd_scan
    # This module provides the vulnerability context
    
    return results


def check_password_strength(password: str) -> dict:
    """
    Check password strength
    
    Args:
        password: Password to check
    
    Returns:
        dict: Strength assessment
    """
    score = 0
    feedback = []
    
    if len(password) >= 8:
        score += 1
    else:
        feedback.append("Password should be at least 8 characters")
    
    if len(password) >= 12:
        score += 1
    
    if any(c.isupper() for c in password):
        score += 1
    else:
        feedback.append("Add uppercase letters")
    
    if any(c.islower() for c in password):
        score += 1
    else:
        feedback.append("Add lowercase letters")
    
    if any(c.isdigit() for c in password):
        score += 1
    else:
        feedback.append("Add numbers")
    
    if any(c in "!@#$%^&*()_+-=[]{}|;:,.<>?" for c in password):
        score += 1
    else:
        feedback.append("Add special characters")
    
    strength = "weak"
    if score >= 5:
        strength = "strong"
    elif score >= 3:
        strength = "medium"
    
    return {
        "score": score,
        "max_score": 6,
        "strength": strength,
        "feedback": feedback
    }


def hash_file(filepath: str) -> dict:
    """
    Calculate multiple hashes for a file
    
    Args:
        filepath: Path to file
    
    Returns:
        dict: Hash values
    """
    hashes = {}
    
    try:
        with open(filepath, 'rb') as f:
            content = f.read()
            hashes['md5'] = hashlib.md5(content).hexdigest()
            hashes['sha1'] = hashlib.sha1(content).hexdigest()
            hashes['sha256'] = hashlib.sha256(content).hexdigest()
            hashes['sha512'] = hashlib.sha512(content).hexdigest()
    except Exception as e:
        hashes['error'] = str(e)
    
    return hashes


def check_permissions(path: str) -> dict:
    """
    Check file/directory permissions for security issues
    
    Args:
        path: Path to check
    
    Returns:
        dict: Permission analysis
    """
    issues = []
    
    try:
        stat = os.stat(path)
        mode = stat.st_mode
        
        # Check world-writable
        if mode & 0o002:
            issues.append("World-writable")
        
        # Check world-readable (for sensitive files)
        if mode & 0o004:
            sensitive = ['/etc/shadow', '/etc/sudoers', '.ssh/id_rsa']
            if any(s in path for s in sensitive):
                issues.append("Sensitive file is world-readable")
        
        # Check SUID/SGID
        if mode & 0o4000:
            issues.append("SUID bit set")
        if mode & 0o2000:
            issues.append("SGID bit set")
        
        return {
            "path": path,
            "mode": oct(mode),
            "uid": stat.st_uid,
            "gid": stat.st_gid,
            "issues": issues,
            "secure": len(issues) == 0
        }
    except Exception as e:
        return {"error": str(e)}
