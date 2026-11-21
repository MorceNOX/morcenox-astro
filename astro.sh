#!/bin/bash
#
# MorceNOX-ASTRO™
# Copyright (C) 2026 Amilcar Antonio Mesquita Rizk amilcar.rizk@gmail.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://gnu.org>.
#
# astro.sh - Entry point for portable MorceNOX-Astro

#!/usr/bin/env bash

APP_NAME="MorceNOX-Astro"
CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
APP_CONFIG_DIR="$CONFIG_HOME/$APP_NAME"
DB_FILE="$APP_CONFIG_DIR/astro.db"

SELF_DIR="$(cd "$(dirname "${BASH_SOURCE}")" && pwd)"

echo "Launching $APP_NAME..."

if [ ! -d "$APP_CONFIG_DIR" ]; then
    echo "First run detected. Initializing configuration in $APP_CONFIG_DIR..."
    mkdir -p "$APP_CONFIG_DIR/ephe" "$APP_CONFIG_DIR/database" "$APP_CONFIG_DIR/export"

    [ -d "$SELF_DIR/assets/ephe" ]     && cp -r "$SELF_DIR/assets/ephe/"* "$APP_CONFIG_DIR/ephe/"
    [ -d "$SELF_DIR/assets/database" ] && cp -r "$SELF_DIR/assets/database/"* "$APP_CONFIG_DIR/database/"
    [ -f "$SELF_DIR/assets/.env" ]     && cp "$SELF_DIR/assets/.env" "$APP_CONFIG_DIR/"
    cp "$SELF_DIR/assets"/help_*.txt "$APP_CONFIG_DIR/" 2>/dev/null || true
    cp "$SELF_DIR/assets"/topics_*.txt "$APP_CONFIG_DIR/" 2>/dev/null || true
fi

USER_APPS_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
USER_ICONS_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/icons/hicolor/256x256/apps"

if [ -f "$SELF_DIR/assets/morcenox-astro.desktop" ] && [ -f "$SELF_DIR/assets/morcenox-astro.png" ]; then
    mkdir -p "$USER_APPS_DIR" "$USER_ICONS_DIR"
    cp -f "$SELF_DIR/assets/morcenox-astro.png" "$USER_ICONS_DIR/morcenox-astro.png"
    sed "s|^Exec=.*|Exec=\"$SELF_DIR/astro.sh\"|" "$SELF_DIR/assets/morcenox-astro.desktop" > "$USER_APPS_DIR/morcenox-astro.desktop"
    chmod +x "$USER_APPS_DIR/morcenox-astro.desktop"
    command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$USER_APPS_DIR" 2>/dev/null || true
fi


if [ ! -f "$DB_FILE" ]; then
    if command -v sqlite3 >/dev/null 2>&1 && [ -f "$SELF_DIR/assets/astro.db.sql" ]; then
        echo "Creating and structuring database from assets..."
        sqlite3 "$DB_FILE" < "$SELF_DIR/assets/astro.db.sql"
    fi
fi

# Substitui o processo do script pelo binário nativo
exec "$SELF_DIR/bin/astro" "$@"


