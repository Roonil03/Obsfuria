#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
fingerprint=${RECIPIENT:-}
if [[ ! $fingerprint =~ ^([[:xdigit:]]{40}|[[:xdigit:]]{64})$ ]]; then
    echo "Set RECIPIENT to the full fingerprint of the recipient's OpenPGP public key." >&2
    exit 2
fi
binary=${1:?expected path to release binary}
out="$binary.${fingerprint^^}.gpg"
tmp=$(mktemp -- "$out.XXXXXX")
trap 'rm -f -- "$tmp"' EXIT
gpg --no-options --batch --no-auto-key-locate --trust-model always \
    --recipient "$fingerprint" --output - --encrypt -- "$binary" > "$tmp"
mv -T -- "$tmp" "$out"
printf 'Encrypted release: %s\n' "$out"
