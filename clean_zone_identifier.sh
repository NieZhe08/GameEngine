#!/usr/bin/env bash
set -euo pipefail

# Recursively delete all files matching *Zone.Identifier under target directory.
# Usage:
#   ./clean_zone_identifier.sh                # current directory
#   ./clean_zone_identifier.sh /path/to/dir   # specific directory

target_dir="${1:-.}"

if [[ ! -d "$target_dir" ]]; then
  echo "Error: directory not found: $target_dir" >&2
  exit 1
fi

echo "Searching in: $target_dir"
find "$target_dir" -type f -name '*Zone.Identifier' -print -delete

echo "Done."
