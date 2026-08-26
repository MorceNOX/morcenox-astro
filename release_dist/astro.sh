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

APP_NAME="MorceNOX-Astro"
XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
APP_CONFIG_DIR="$XDG_CONFIG_HOME/$APP_NAME"
DB_FILE="$APP_CONFIG_DIR/astro.db"

# Get the directory where this script is located
SELF_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Launching $APP_NAME..."

# 1. Check if the config directory exists, if not, initialize it from the package
if [ ! -d "$APP_CONFIG_DIR" ]; then
    echo "First run detected. Initializing configuration in $APP_CONFIG_DIR..."
    mkdir -p "$APP_CONFIG_DIR/ephe"
    sleep 0.1 # Garante a consolidação do diretório no disco
    
    # Copy Ephemeris files from the 'assets' folder in the package
    if [ -d "$SELF_DIR/assets/ephe" ]; then
        cp -r "$SELF_DIR/assets/ephe/"* "$APP_CONFIG_DIR/ephe/"
    fi

    # Copy Text/Env files from the 'assets' folder (CORRIGIDO: Asteriscos fora das aspas)
    cp "$SELF_DIR/assets/.env" "$APP_CONFIG_DIR/" 2>/dev/null
    cp "$SELF_DIR/assets"/help_*.txt "$APP_CONFIG_DIR/" 2>/dev/null
    cp "$SELF_DIR/assets"/topics_*.txt "$APP_CONFIG_DIR/" 2>/dev/null  # CORRIGIDO: APP_DIR para APP_CONFIG_DIR
    
    # NOVO: Inicialização do Banco de Dados via SQLite3 no modo portátil
    if [ ! -f "$DB_FILE" ]; then
        if [ -f "$SELF_DIR/assets/astro.db.sql" ]; then
            echo "Creating and structuring database from assets..."
            sqlite3 "$DB_FILE" < "$SELF_DIR/assets/astro.db.sql"
        else
            echo "Warning: astro.db.sql not found in assets. Creating an empty database."
            sqlite3 "$DB_FILE" "VACUUM;"
        fi
    fi

    echo "Initialization complete."
else
    echo "Configuration already exists in $APP_CONFIG_DIR. Using existing files."
    
    # GARANTIA DE SEGURANÇA: Se o usuário já abriu o app antes, mas por algum bug o banco 
    # não foi criado ou sumiu, cria ele aqui para o binário nunca quebrar.
    if [ ! -f "$DB_FILE" ]; then
        echo "Database missing! Recovering..."
        if [ -f "$SELF_DIR/assets/astro.db.sql" ]; then
            sqlite3 "$DB_FILE" < "$SELF_DIR/assets/astro.db.sql"
        else
            sqlite3 "$DB_FILE" "VACUUM;"
        fi
    fi
fi

# 2. Run the actual binary
# We use the absolute path to the binary inside the package
"$SELF_DIR/bin/astro"

