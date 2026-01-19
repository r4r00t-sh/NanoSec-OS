#!/bin/bash
#
# NanoSec Firewall (nfw) - Auto-configure iptables
# ================================================
# Automatically configures a secure firewall on boot
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
ALLOWED_TCP_PORTS="22 80 443"
ALLOWED_UDP_PORTS="53"
SSH_RATE_LIMIT="3/minute"
LOG_PREFIX="[NFW] "

log() {
    echo -e "${GREEN}[NFW]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[NFW]${NC} $1"
}

error() {
    echo -e "${RED}[NFW]${NC} $1"
}

# Check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        error "Please run as root"
        exit 1
    fi
}

# Flush existing rules
flush_rules() {
    log "Flushing existing rules..."
    iptables -F
    iptables -X
    iptables -t nat -F
    iptables -t nat -X
    iptables -t mangle -F
    iptables -t mangle -X
}

# Set default policies (deny all)
set_default_policies() {
    log "Setting default policies (DROP all)..."
    iptables -P INPUT DROP
    iptables -P FORWARD DROP
    iptables -P OUTPUT ACCEPT
}

# Allow loopback
allow_loopback() {
    log "Allowing loopback interface..."
    iptables -A INPUT -i lo -j ACCEPT
    iptables -A OUTPUT -o lo -j ACCEPT
}

# Allow established connections
allow_established() {
    log "Allowing established/related connections..."
    iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
}

# Allow specific ports
allow_ports() {
    log "Configuring allowed ports..."
    
    # TCP ports
    for port in $ALLOWED_TCP_PORTS; do
        log "  Allowing TCP port $port"
        iptables -A INPUT -p tcp --dport $port -m conntrack --ctstate NEW -j ACCEPT
    done
    
    # UDP ports
    for port in $ALLOWED_UDP_PORTS; do
        log "  Allowing UDP port $port"
        iptables -A INPUT -p udp --dport $port -j ACCEPT
    done
}

# SSH rate limiting
ssh_rate_limit() {
    log "Configuring SSH rate limiting ($SSH_RATE_LIMIT)..."
    
    # Remove existing SSH rule and add rate limited version
    iptables -D INPUT -p tcp --dport 22 -m conntrack --ctstate NEW -j ACCEPT 2>/dev/null || true
    
    # Rate limit new SSH connections
    iptables -A INPUT -p tcp --dport 22 -m conntrack --ctstate NEW \
             -m recent --set --name SSH
    iptables -A INPUT -p tcp --dport 22 -m conntrack --ctstate NEW \
             -m recent --update --seconds 60 --hitcount 4 --name SSH -j DROP
    iptables -A INPUT -p tcp --dport 22 -m conntrack --ctstate NEW -j ACCEPT
}

# DDoS protection
ddos_protection() {
    log "Enabling DDoS protection..."
    
    # Drop invalid packets
    iptables -A INPUT -m conntrack --ctstate INVALID -j DROP
    
    # Drop NULL packets
    iptables -A INPUT -p tcp --tcp-flags ALL NONE -j DROP
    
    # Drop XMAS packets
    iptables -A INPUT -p tcp --tcp-flags ALL ALL -j DROP
    
    # Drop SYN flood
    iptables -A INPUT -p tcp --syn -m limit --limit 1/s --limit-burst 3 -j ACCEPT
    
    # ICMP rate limit
    iptables -A INPUT -p icmp --icmp-type echo-request -m limit --limit 1/s -j ACCEPT
    iptables -A INPUT -p icmp --icmp-type echo-request -j DROP
    
    # Port scan protection
    iptables -A INPUT -p tcp --tcp-flags SYN,ACK,FIN,RST RST -m limit --limit 1/s -j ACCEPT
}

# Log dropped packets
log_drops() {
    log "Enabling drop logging..."
    iptables -A INPUT -m limit --limit 5/min -j LOG --log-prefix "$LOG_PREFIX Dropped: " --log-level 4
}

# ICMP rules
icmp_rules() {
    log "Configuring ICMP..."
    iptables -A INPUT -p icmp --icmp-type destination-unreachable -j ACCEPT
    iptables -A INPUT -p icmp --icmp-type time-exceeded -j ACCEPT
}

# Save rules
save_rules() {
    log "Saving rules..."
    if command -v iptables-save &> /dev/null; then
        iptables-save > /etc/iptables.rules
        log "Rules saved to /etc/iptables.rules"
    else
        warn "iptables-save not available"
    fi
}

# Show status
show_status() {
    echo ""
    echo -e "${GREEN}═══════════════════════════════════════════${NC}"
    echo -e "${GREEN}       NanoSec Firewall Status${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════${NC}"
    echo ""
    iptables -L -n --line-numbers
}

# Start firewall
start() {
    log "Starting NanoSec Firewall..."
    check_root
    flush_rules
    set_default_policies
    allow_loopback
    allow_established
    icmp_rules
    allow_ports
    ssh_rate_limit
    ddos_protection
    log_drops
    save_rules
    log "Firewall started successfully!"
    show_status
}

# Stop firewall
stop() {
    log "Stopping NanoSec Firewall..."
    check_root
    flush_rules
    iptables -P INPUT ACCEPT
    iptables -P FORWARD ACCEPT
    iptables -P OUTPUT ACCEPT
    log "Firewall stopped (all traffic allowed)"
}

# Reload rules
reload() {
    log "Reloading NanoSec Firewall..."
    check_root
    start
}

# Print usage
usage() {
    echo "NanoSec Firewall (nfw)"
    echo ""
    echo "Usage: $0 {start|stop|reload|status}"
    echo ""
    echo "Commands:"
    echo "  start   - Enable firewall with security rules"
    echo "  stop    - Disable firewall (allow all)"
    echo "  reload  - Reload firewall rules"
    echo "  status  - Show current rules"
}

# Main
case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    reload)
        reload
        ;;
    status)
        check_root
        show_status
        ;;
    *)
        usage
        exit 1
        ;;
esac

exit 0
