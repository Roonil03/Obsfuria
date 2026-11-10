#!/usr/bin/env bash
set -o pipefail

OUT_DIR="include"
OUT_FILE="$OUT_DIR/arsenal.hpp"
mkdir -p "$OUT_DIR"

# An explicit path is the most predictable option for one-off generation.
XDG_CONFIG_HOME=${XDG_CONFIG_HOME:-}
if [[ -n "$XDG_CONFIG_HOME" ]]; then
    USER_CONFIG="$XDG_CONFIG_HOME/obsfuria/commands.conf"
elif [[ -n "${HOME:-}" ]]; then
    USER_CONFIG="$HOME/.config/obsfuria/commands.conf"
else
    USER_CONFIG=""
fi

if [[ -n "${1:-}" ]]; then
    CONFIG_FILE=$1
elif [[ -n "$USER_CONFIG" && -f "$USER_CONFIG" ]]; then
    CONFIG_FILE=$USER_CONFIG
elif [[ -f "assets/default_commands.txt" ]]; then
    CONFIG_FILE="assets/default_commands.txt"
else
    echo "No command config found (checked user config and assets/default_commands.txt)." >&2
    exit 1
fi

if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "Command config not found: $CONFIG_FILE" >&2
    exit 1
fi

echo "Generating Obsfuria Arsenal from $CONFIG_FILE..."

cat <<'EOF' > "$OUT_FILE"
#ifndef ARSENAL_HPP
#define ARSENAL_HPP

#include <string>
#include <map>

class Arsenal {
public:
    static const std::map<std::string, std::string>& getMockCommands() {
        static const std::map<std::string, std::string> commands = {
EOF

# Emit a C++ raw string with a delimiter that cannot terminate inside VALUE.
emit_raw_string() {
    local value=$1 delimiter="obsfuria" suffix=0
    while [[ "$value" == *")${delimiter}\""* ]]; do
        delimiter="obsfuria${suffix}"
        ((suffix += 1))
    done
    printf 'R"%s(%s)%s"' "$delimiter" "$value" "$delimiter"
}

while IFS='|' read -r name cmd || [[ -n "$name$cmd" ]]; do
    [[ -z "$name" || "$name" =~ ^[[:space:]]*# ]] && continue
    name=${name#"${name%%[![:space:]]*}"}
    name=${name%"${name##*[![:space:]]}"}
    cmd=${cmd#"${cmd%%[![:space:]]*}"}
    cmd=${cmd%"${cmd##*[![:space:]]}"}
    [[ -z "$name" ]] && continue

    echo "  -> Capturing: $name"

    # Preserve command output's trailing newlines while command substitution
    # is in use. Configured commands intentionally run to capture their mock.
    captured=$(bash -c "$cmd" 2>/dev/null | head -n 25; printf '\034')
    output=${captured%$'\034'}
    if [[ -z "$output" ]]; then
        command_name=${cmd%%[[:space:]]*}
        output="bash: ${command_name}: command not found or output restricted by local system."
    fi

    printf '            {' >> "$OUT_FILE"
    emit_raw_string "$name" >> "$OUT_FILE"
    printf ', ' >> "$OUT_FILE"
    emit_raw_string "$output" >> "$OUT_FILE"
    printf '},\n' >> "$OUT_FILE"
done < "$CONFIG_FILE"

cat <<'EOF' >> "$OUT_FILE"
            {"obsfuria_version", "1.0.0"}
        };
        return commands;
    }
};

#endif
EOF

echo "Success! $OUT_FILE generated successfully."
