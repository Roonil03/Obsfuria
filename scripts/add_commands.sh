#!/bin/bash

OUT_DIR="include"
OUT_FILE="$OUT_DIR/arsenal.hpp"

mkdir -p "$OUT_DIR"

COMMANDS=(
    "ls" "ls -l" "ls -a" "ls -la" "ls -lh" "ls -lt" "ls -lS" "ls -1" "ls -R | head -n 15"
    "pwd" "pwd -L" "pwd -P"
    "tree -L 1" "tree -d -L 1"
    "stat ." "file ." 
    "du -sh ." "du -h -d 1 ." "df -h" "df -T" "df -i" "df -a | head -n 10"
    "uname" "uname -a" "uname -r" "uname -m" "uname -o" "uname -v"
    "hostname" "hostname -i" "hostname -I"
    "arch" "nproc" "getconf LONG_BIT" "getconf PAGE_SIZE"
    "lscpu" "lscpu -e" "lshw -short | head -n 15"
    "lsblk" "lsblk -f" "lsblk -t"
    "lsusb" "lspci | head -n 15" "lsmod | head -n 15"
    "whoami" "id" "id -u" "id -g" "id -G" 
    "groups" "groups \$USER" "users" 
    "who" "w" "last -n 5" "lastlog | head -n 10"
    "uptime" "uptime -p" "uptime -s"
    "free" "free -m" "free -g" "free -h" "free -t"
    "vmstat" "vmstat -s" "vmstat -d" "vmstat -m | head -n 10"
    "ps" "ps -e | head -n 10" "ps aux | head -n 10" "ps -ef | head -n 10" 
    "ps -u \$USER | head -n 10" "top -b -n 1 | head -n 15" 
    "pgrep bash" "pidof init" "pidof systemd"
    "date" "date -u" "date -R" "date -I" "date +%s" 
    "cal" "cal -3" "cal -y" 
    "locale" "locale -a | head -n 10"
    "env | head -n 15" "printenv | head -n 15" 
    "echo \$USER" "echo \$SHELL" "echo \$TERM" "echo \$PATH | tr ':' '\n' | head -n 10"
    "ping -c 1 127.0.0.1" "ping -c 3 localhost" 
    "ip a" "ip addr show" "ip route" "ip link" "ip -s link"
    "ss -tuln" "ss -s" "ss -ua" 
    "netstat -i" "netstat -s | head -n 15" "arp -a" "route -n"
    "cat /etc/os-release" "cat /etc/issue" "cat /etc/hostname" 
    "cat /proc/version" "cat /proc/cmdline"
    "cat /proc/cpuinfo | head -n 15" "cat /proc/meminfo | head -n 15" 
    "cat /proc/loadavg" "cat /proc/uptime" "cat /proc/partitions"
    "dmesg | tail -n 15"
    "head -n 10 /etc/passwd" "tail -n 10 /etc/passwd"
    "wc -l /etc/passwd" "wc -c /etc/passwd" 
    "md5sum /etc/passwd" "sha1sum /etc/passwd" "sha256sum /etc/passwd" 
    "basename /etc/passwd" "dirname /var/log/syslog"
    "type -a echo" "whereis bash" "which ls"
    "tty" "stty -a | head -n 5" 
    "echo 'Obsfuria Node Active'" "printf 'System Integrity: %%d\\n' 42"
    "seq 1 5" "seq 10 -1 1" "factor 1024" "factor 73856093"
    "yes 'Null' | head -n 5" "rev <<< 'Obsfuria Engine'" "base64 <<< 'Obsfuria Core'"
    "cowsay 'Memory Fault'" "fortune -s"
)

cat << 'EOF' > "$OUT_FILE"
#ifndef ARSENAL_HPP
#define ARSENAL_HPP

#include <string>
#include <map>

class Arsenal {
public:
    static const std::map<std::string, std::string>& getMockCommands() {
        static const std::map<std::string, std::string> commands = {
EOF

echo "Generating Obsfuria Arsenal from host system..."

for cmd in "${COMMANDS[@]}"; do
    echo "  -> Capturing: $cmd"
    
    output=$(bash -c "$cmd" 2>/dev/null | head -n 25)
    
    if [ -z "$output" ]; then
        output="bash: ${cmd%% *}: command not found or output restricted by local system."
    fi

    echo "            {\"$cmd\", R\"obsfuria(" >> "$OUT_FILE"
    echo "$output" >> "$OUT_FILE"
    echo ")obsfuria\"}," >> "$OUT_FILE"
done

cat << 'EOF' >> "$OUT_FILE"
            {"obsfuria_version", "1.0.0"}
        };
        return commands;
    }
};

#endif
EOF

echo "Success! $OUT_FILE generated successfully with ${#COMMANDS[@]} command variants."