#!/bin/bash
#
# SSH Hardening Script for NanoSec OS
# ====================================
# Applies security best practices to SSH configuration
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

SSH_CONFIG="/etc/ssh/sshd_config"
BACKUP_DIR="/var/backup/ssh"
BANNER_FILE="/etc/ssh/banner"

log() { echo -e "${GREEN}[SSH-HARDEN]${NC} $1"; }
warn() { echo -e "${YELLOW}[SSH-HARDEN]${NC} $1"; }
error() { echo -e "${RED}[SSH-HARDEN]${NC} $1"; exit 1; }

check_root() {
    [ "$EUID" -eq 0 ] || error "Please run as root"
}

backup_config() {
    log "Backing up SSH configuration..."
    mkdir -p "$BACKUP_DIR"
    cp "$SSH_CONFIG" "$BACKUP_DIR/sshd_config.$(date +%Y%m%d%H%M%S)"
}

set_option() {
    local key="$1"
    local value="$2"
    
    if grep -q "^#*${key}" "$SSH_CONFIG"; then
        sed -i "s/^#*${key}.*/${key} ${value}/" "$SSH_CONFIG"
    else
        echo "${key} ${value}" >> "$SSH_CONFIG"
    fi
    log "  Set: ${key} ${value}"
}

create_banner() {
    log "Creating SSH banner..."
    cat > "$BANNER_FILE" << 'EOF'
╔═══════════════════════════════════════════════════════════════╗
║                     NANOSEC OS                                 ║
║                                                                 ║
║  UNAUTHORIZED ACCESS IS PROHIBITED                             ║
║                                                                 ║
║  All connections are monitored and logged.                     ║
║  Disconnect IMMEDIATELY if you are not authorized.             ║
╚═══════════════════════════════════════════════════════════════╝
EOF
}

harden_ssh() {
    log "Applying SSH hardening..."
    
    # Disable root login
    set_option "PermitRootLogin" "no"
    
    # Disable password authentication (key only)
    set_option "PasswordAuthentication" "no"
    set_option "PermitEmptyPasswords" "no"
    set_option "ChallengeResponseAuthentication" "no"
    
    # Enable public key authentication
    set_option "PubkeyAuthentication" "yes"
    
    # Disable X11 forwarding
    set_option "X11Forwarding" "no"
    
    # Disable TCP forwarding (can be enabled if needed)
    set_option "AllowTcpForwarding" "no"
    set_option "AllowAgentForwarding" "no"
    
    # Set max auth attempts
    set_option "MaxAuthTries" "3"
    
    # Set login grace time
    set_option "LoginGraceTime" "30"
    
    # Set client alive interval
    set_option "ClientAliveInterval" "300"
    set_option "ClientAliveCountMax" "2"
    
    # Limit concurrent sessions
    set_option "MaxSessions" "3"
    set_option "MaxStartups" "3:50:10"
    
    # Use strong ciphers only
    set_option "Ciphers" "chacha20-poly1305@openssh.com,aes256-gcm@openssh.com,aes128-gcm@openssh.com,aes256-ctr,aes192-ctr,aes128-ctr"
    
    # Use strong MACs only
    set_option "MACs" "hmac-sha2-512-etm@openssh.com,hmac-sha2-256-etm@openssh.com,hmac-sha2-512,hmac-sha2-256"
    
    # Use strong key exchange
    set_option "KexAlgorithms" "curve25519-sha256,curve25519-sha256@libssh.org,diffie-hellman-group16-sha512,diffie-hellman-group18-sha512"
    
    # Enable strict mode
    set_option "StrictModes" "yes"
    
    # Show banner
    set_option "Banner" "$BANNER_FILE"
    
    # Log level
    set_option "LogLevel" "VERBOSE"
    
    # Disable tunneling
    set_option "PermitTunnel" "no"
    
    # Disable user environment
    set_option "PermitUserEnvironment" "no"
}

restart_ssh() {
    log "Restarting SSH service..."
    if command -v systemctl &> /dev/null; then
        systemctl restart sshd || systemctl restart ssh
    elif command -v service &> /dev/null; then
        service sshd restart || service ssh restart
    else
        warn "Could not restart SSH - please restart manually"
    fi
}

verify_config() {
    log "Verifying SSH configuration..."
    if sshd -t; then
        log "Configuration is valid"
    else
        error "Configuration is invalid! Restoring backup..."
        latest_backup=$(ls -t "$BACKUP_DIR"/sshd_config.* | head -1)
        cp "$latest_backup" "$SSH_CONFIG"
        error "Restored from backup. Please check your settings."
    fi
}

show_status() {
    echo ""
    echo -e "${CYAN}═══════════════════════════════════════════${NC}"
    echo -e "${CYAN}       SSH Hardening Status${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════${NC}"
    echo ""
    echo -e "${GREEN}Key Settings:${NC}"
    grep -E "^(PermitRootLogin|PasswordAuthentication|PubkeyAuthentication|MaxAuthTries)" "$SSH_CONFIG" | while read line; do
        echo "  $line"
    done
    echo ""
}

usage() {
    echo "NanoSec SSH Hardening Script"
    echo ""
    echo "Usage: $0 {apply|status|restore}"
    echo ""
    echo "Commands:"
    echo "  apply   - Apply SSH hardening settings"
    echo "  status  - Show current SSH configuration"
    echo "  restore - Restore from latest backup"
}

main() {
    case "$1" in
        apply)
            check_root
            backup_config
            create_banner
            harden_ssh
            verify_config
            restart_ssh
            show_status
            log "SSH hardening complete!"
            warn "Make sure you have SSH key access before logging out!"
            ;;
        status)
            show_status
            ;;
        restore)
            check_root
            if [ -d "$BACKUP_DIR" ]; then
                latest=$(ls -t "$BACKUP_DIR"/sshd_config.* 2>/dev/null | head -1)
                if [ -n "$latest" ]; then
                    cp "$latest" "$SSH_CONFIG"
                    restart_ssh
                    log "Restored from: $latest"
                else
                    error "No backups found"
                fi
            else
                error "No backup directory found"
            fi
            ;;
        *)
            usage
            exit 1
            ;;
    esac
}

main "$@"
