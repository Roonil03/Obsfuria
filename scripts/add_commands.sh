#!/usr/bin/env bash
set -o pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
OUT_DIR="$SCRIPT_DIR/../include"
OUT_FILE="$OUT_DIR/arsenal.hpp"
mkdir -p "$OUT_DIR"
TMP_FILE=$(mktemp "$OUT_FILE.tmp.XXXXXX")
trap 'rm -f "$TMP_FILE"' EXIT

if [[ -n "${1:-}" ]]; then
    CONFIG_FILE=$1
    CUSTOM_CONFIG=1
else
    # Use only the repository's bundled list, independent of caller cwd or
    # user configuration. This keeps normal builds reproducible and private.
    CONFIG_FILE="$SCRIPT_DIR/../assets/default_commands.txt"
    CUSTOM_CONFIG=0
fi

if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "Command config not found: $CONFIG_FILE" >&2
    exit 1
fi

if (( CUSTOM_CONFIG )); then
    echo "Generating CUSTOM arsenal from an explicit file (entry names will be embedded)."
else
    echo "Generating built-in static Obsfuria Arsenal."
fi

cat <<'EOF' > "$TMP_FILE"
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
    local value=$1 base="obsfuria" delimiter="obsfuria" suffix=0
    while [[ "$value" == *")${delimiter}\""* ]]; do
        suffix=$((suffix + 1))
        if (( suffix > 99999999 )); then
            echo "Cannot encode a string with a valid C++ raw-string delimiter." >&2
            return 1
        fi
        delimiter="${base}${suffix}"
    done
    printf 'R"%s(%s)%s"' "$delimiter" "$value" "$delimiter"
}

line_number=0
while IFS='|' read -r name cmd || [[ -n "$name$cmd" ]]; do
    ((line_number += 1))
    [[ -z "$name" || "$name" == \#* ]] && continue

    if (( ${#name} > 128 )) || [[ "$name" == *"|"* || "$name" =~ [[:cntrl:]] || -z "$cmd" ]]; then
        echo "Invalid command name on line $line_number: names must be at most 128 characters and contain no control characters." >&2
        exit 1
    fi

    # Generation happens during builds and installs. Never execute configured
    # commands here: the command list may come from a user's private config.
    # Runtime capture is handled by the game when a session starts.
    output="[Output is captured when a game starts.]"

    printf '            {' >> "$TMP_FILE"
    emit_raw_string "$name" >> "$TMP_FILE" || exit 1
    printf ', ' >> "$TMP_FILE"
    emit_raw_string "$output" >> "$TMP_FILE" || exit 1
    printf '},\n' >> "$TMP_FILE"
done < "$CONFIG_FILE"

cat <<'EOF' >> "$TMP_FILE"
            {"obsfuria_version", "1.0.0"}
        };
        return commands;
    }
};

#endif
EOF

mv -- "$TMP_FILE" "$OUT_FILE"
trap - EXIT
echo "Success! $OUT_FILE generated successfully."
