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
#
VERSION = 1.11.1

# Variáveis de compilação (Precisão estrita e depuração ativadas)
CC       = gcc
CFLAGS   = -Wall -Wextra -O0 -frounding-math -fexcess-precision=standard -ffloat-store -fno-associative-math -fno-fast-math -Iinclude
LDFLAGS  = -Llib -Wl,-rpath,lib:/usr/local/lib:'$(LIBEXECDIR)'
LIBS     = -lm -lswe -lncursesw -lsqlite3 -licui18n -licuuc -licudata -lpthread -ldl

APP_NAME = MorceNOX-Astro

# Nome do executável final e do script SQL
TARGET   = astro
DB_SQL   = astro.db.sql

# ==============================================================================
# CONFIGURAÇÕES DO GETTEXT
# ==============================================================================
DOMAIN     = astro
PO_DIR     = po
LOCALES    = pt es en
POT_FILE   = $(PO_DIR)/$(DOMAIN).pot
# ==============================================================================


# Caminhos de Instalação do Sistema
PREFIX   ?= /usr/local
BINDIR   := $(PREFIX)/bin
LIBEXECDIR ?= $(PREFIX)/libexec/$(APP_NAME)

# Caminhos do Usuário (Configuração e Banco de Dados no XDG)
XDG_CONFIG_HOME ?= $(HOME)/.config
APP_CONFIG_DIR  := $(XDG_CONFIG_HOME)/$(APP_NAME)
DB_FILE         := $(APP_CONFIG_DIR)/astro.db

CFLAGS +=  -DVERSION=\"$(VERSION)\"
CFLAGS +=  -DAPPLICATION_NAME=\"$(APP_NAME)\"

# Encontra automaticamente todos os arquivos .c na pasta src/
SRCS     = $(wildcard src/*.c)
OBJS     = $(SRCS:.c=.o)

PACKAGE_NAME = $(APP_NAME)-v$(VERSION)-linux.tar.gz
PKG_DIR      = release_dist

.PHONY: all clean init-db reset-db install uninstall setup-dir translate

# Regra principal (padrão - apenas compila o binário localmente)
all: $(TARGET) translate

# Regra para linkar o binário final
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)

# Regra genérica para compilar arquivos .c em .o
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para criar a estrutura de diretórios e injetar o banco no $HOME do usuário
setup-dir:
	@echo "Criando diretórios de configuração em $(APP_CONFIG_DIR)..."
	@mkdir -p $(APP_CONFIG_DIR)/ephe
	@sleep 0.1 # Pequena pausa de segurança para o sistema operacional consolidar as pastas no disco
	@if [ -z "$$(ls -A $(APP_CONFIG_DIR)/ephe 2>/dev/null)" ]; then \
	    echo "Pasta ephe vazia. Copiando arquivos da Swiss Ephemeris..." ; \
	    cp -r ephe/* $(APP_CONFIG_DIR)/ephe/ ; \
	else \
	    echo "Arquivos da Swiss Ephemeris já existem em $(APP_CONFIG_DIR)/ephe/. Pulando cópia." ; \
	fi
	@if [ ! -f "$(APP_CONFIG_DIR)/help_en.txt" ]; then \
	    cp help_en.txt $(APP_CONFIG_DIR)/; \
	    cp topics_en.txt $(APP_CONFIG_DIR)/; \
	    cp help_pt.txt $(APP_CONFIG_DIR)/; \
	    cp topics_pt.txt $(APP_CONFIG_DIR)/; \
	fi
	@if [ ! -f "$(APP_CONFIG_DIR)/.env" ]; then \
	    cp .env $(APP_CONFIG_DIR)/; \
	fi
	@if [ ! -f "$(DB_FILE)" ]; then \
	    echo "Criando o banco de dados estruturado em $(DB_FILE)..."; \
	    if [ -f "$(DB_SQL)" ]; then \
	        sqlite3 "$(DB_FILE)" "VACUUM;" && sqlite3 "$(DB_FILE)" < "$(DB_SQL)"; \
	        echo "Banco de dados inicializado com sucesso a partir de $(DB_SQL)."; \
	    else \
	        echo "ERRO CRÍTICO: $(DB_SQL) não encontrado! Impossível estruturar as tabelas."; \
	        exit 1; \
	    fi \
	else \
	    echo "Banco de dados já existe em $(DB_FILE). Mantendo arquivo atual."; \
	fi


# ==============================================================================
# SEÇÃO AUTOMATIZADA: TRADUÇÕES E INSTALAÇÃO (GETTEXT FIXO)
# ==============================================================================

# Compila os textos (.po) gerando os binários locais com o nome da língua (ex: pt.mo)
translate: $(POT_FILE)
	@mkdir -p locale/pt/LC_MESSAGES
	@mkdir -p locale/en/LC_MESSAGES
	@mkdir -p locale/es/LC_MESSAGES
	@if [ -f $(PO_DIR)/pt.po ]; then msgfmt $(PO_DIR)/pt.po -o locale/pt/LC_MESSAGES/pt.mo; fi
	@if [ -f $(PO_DIR)/en.po ]; then msgfmt $(PO_DIR)/en.po -o locale/en/LC_MESSAGES/en.mo; fi
	@if [ -f $(PO_DIR)/es.po ]; then msgfmt $(PO_DIR)/es.po -o locale/es/LC_MESSAGES/es.mo; fi

# Atualiza ou cria o modelo (.pot) escaneando os códigos dentro de src/
$(POT_FILE): $(SRCS)
	@mkdir -p $(PO_DIR)
	@echo "Atualizando modelo de tradução (POT)..."
	xgettext --keyword=_ --language=C --from-code=UTF-8 --output=$(POT_FILE) $(SRCS)
	@echo "Mesclando novidades nos arquivos .po..."
	@if [ -f $(PO_DIR)/pt.po ]; then msgmerge --update $(PO_DIR)/pt.po $(POT_FILE); else msginit --no-translator --input=$(POT_FILE) --locale=pt --output=$(PO_DIR)/pt.po; fi
	@if [ -f $(PO_DIR)/en.po ]; then msgmerge --update $(PO_DIR)/en.po $(POT_FILE); else msginit --no-translator --input=$(POT_FILE) --locale=en --output=$(PO_DIR)/en.po; fi
	@if [ -f $(PO_DIR)/es.po ]; then msgmerge --update $(PO_DIR)/es.po $(POT_FILE); else msginit --no-translator --input=$(PO_DIR)/es.po --locale=es --output=$(PO_DIR)/es.po; fi

# Regra de instalação atualizada para copiar os arquivos pt.mo e en.mo fixos
install: all setup-dir
	@echo "Instalando o executável em $(DESTDIR)$(BINDIR)..."
	@mkdir -p $(DESTDIR)$(BINDIR)
	@cp $(TARGET) $(DESTDIR)$(BINDIR)/
	@chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)
	@install -d $(DESTDIR)$(LIBEXECDIR)
	@cp -r lib/* $(DESTDIR)$(LIBEXECDIR)/
	
	@echo "Instalando arquivos de tradução fixa do Gettext..."
	@if [ -f locale/pt/LC_MESSAGES/pt.mo ]; then \
		install -d $(DESTDIR)$(LOCALEDIR)/pt/LC_MESSAGES; \
		install -m 644 locale/pt/LC_MESSAGES/pt.mo $(DESTDIR)$(LOCALEDIR)/pt/LC_MESSAGES/pt.mo; \
	fi
	@if [ -f locale/en/LC_MESSAGES/en.mo ]; then \
		install -d $(DESTDIR)$(LOCALEDIR)/en/LC_MESSAGES; \
		install -m 644 locale/en/LC_MESSAGES/en.mo $(DESTDIR)$(LOCALEDIR)/en/LC_MESSAGES/en.mo; \
	fi
	@if [ -f locale/es/LC_MESSAGES/es.mo ]; then \
		install -d $(DESTDIR)$(LOCALEDIR)/es/LC_MESSAGES; \
		install -m 644 locale/es/LC_MESSAGES/es.mo $(DESTDIR)$(LOCALEDIR)/es/LC_MESSAGES/es.mo; \
	fi

	@echo ""
	@echo "======================================================================="
	@echo " Instalação do AstroMorce concluída com sucesso!"
	@echo "======================================================================="

# Regra de desinstalação limpa
uninstall:
	@echo "Removendo o executável de $(DESTDIR)$(BINDIR)..."
	@rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo "Removendo arquivos de tradução..."
	@rm -f $(DESTDIR)$(LOCALEDIR)/pt/LC_MESSAGES/pt.mo
	@rm -f $(DESTDIR)$(LOCALEDIR)/en/LC_MESSAGES/en.mo
	@rm -f $(DESTDIR)$(LOCALEDIR)/es/LC_MESSAGES/es.mo

# Regra para limpar os arquivos temporários de compilação locais
clean:
	rm -f src/*.o $(TARGET)

# Regra para resetar o banco de dados do usuário ativo
reset-db: clean
	@if [ -f "$(DB_FILE)" ]; then \
		echo "Removendo o banco de dados do usuário em $(DB_FILE)..."; \
		rm -f "$(DB_FILE)"; \
	fi

.PHONY: package

package: all translate
	@echo "Packaging version $(VERSION) into $(PKG_DIR)..."
	@rm -rf $(PKG_DIR)
	@mkdir -p $(PKG_DIR)/bin
	@mkdir -p $(PKG_DIR)/lib
	@mkdir -p $(PKG_DIR)/locale
	@mkdir -p $(PKG_DIR)/assets
	
	# 1. Copy the executable
	@cp $(TARGET) $(PKG_DIR)/bin/
	
	# 2. Copy the shared libraries
	@if [ -d "lib" ]; then cp -r lib/* $(PKG_DIR)/lib/ ; fi
	
	# 3. Copy the translations
	@cp -r locale/* $(PKG_DIR)/locale/ 2>/dev/null || true

	# 4. Copy the "Assets" (GARANTINDO QUE O SQL DO BANCO VAI JUNTO PARA O ASTRO.SH MOVER)
	@if [ -f ".env" ]; then cp .env $(PKG_DIR)/assets/ ; fi
	@if [ -f "$(DB_SQL)" ]; then cp $(DB_SQL) $(PKG_DIR)/assets/ ; fi
	@cp help_en.txt help_pt.txt $(PKG_DIR)/assets/ 2>/dev/null || true
	@cp topics_en.txt topics_pt.txt $(PKG_DIR)/assets/ 2>/dev/null || true
	@if [ -d "ephe" ]; then cp -r ephe $(PKG_DIR)/assets/ ; fi

	# 5. Copy the Launcher script
	@if [ -f "astro.sh" ]; then \
		cp astro.sh $(PKG_DIR)/; \
		chmod +x $(PKG_DIR)/astro.sh; \
	else \
		echo "ERROR: astro.sh not found! Package creation failed."; exit 1; \
	fi

	# 6. Create the compressed tarball
	tar -czf $(PACKAGE_NAME) -C $(PKG_DIR) .
	
	@echo "======================================================================="
	@echo " Package created with database blueprints: $(PACKAGE_NAME)"
	@echo "======================================================================="
