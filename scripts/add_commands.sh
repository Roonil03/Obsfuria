#!/bin/bash

OUT_DIR="include"
OUT_FILE="$OUT_DIR/arsenal.hpp"

mkdir -p "$OUT_DIR"

# Source config file if provided, otherwise use default asset
CONFIG_FILE="${1:-assets/default_commands.txt}"

if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "Config file $CONFIG_FILE not found, using built-in defaults"
    CONFIG_FILE="assets/default_commands.txt"
fi

echo "Generating Obsfuria Arsenal from $CONFIG_FILE..."

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

while IFS='|' read -r name cmd; do
    # Skip comments and empty lines
    [[ -z "$name" || "$name" =~ ^[[:space:]]*# ]] && continue
    # Trim whitespace
    name=$(echo "$name" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    cmd=$(echo "$cmd" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    [[ -z "$name" ]] && continue
    
    echo "  -> Capturing: $name"
    
    output=$(bash -c "$cmd" 2>/dev/null | head -n 25)
    
    if [ -z "$output" ]; then
        output="bash: ${cmd%% *}: command not found or output restricted by local system."
    fi

    echo "            {\"$name\", R\"obsfuria(" >> "$OUT_FILE"
    echo "$output" >> "$OUT_FILE"
    echo ")obsfuria\"}," >> "$OUT_FILE"
done < "$CONFIG_FILE"

cat << 'EOF' >> "$OUT_FILE"
            {"obsfuria_version", "1.0.0"}
        };
        return commands;
    }
};

#endif
EOF

echo "Success! $OUT_FILE generated successfully."