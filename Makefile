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
VERSION = 1.53.8

# Variáveis de compilação (Precisão estrita e depuração ativadas)
CC       = gcc
CFLAGS   = -Iinclude
LDFLAGS  = 

UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M),x86_64)
    STRICT_MATH_FLAGS = -O0 -frounding-math -fexcess-precision=standard -ffloat-store -fno-associative-math -fno-fast-math
else
    STRICT_MATH_FLAGS = -O0 -frounding-math -fexcess-precision=standard -fno-associative-math -fno-fast-math
endif

override CFLAGS   = -Wall -Wextra $(STRICT_MATH_FLAGS) -Iinclude

SQLITE_SRC := vendor/sqlite/sqlite3.c
SQLITE_OBJ := vendor/sqlite/sqlite3.o

SWE_SRCS := $(wildcard vendor/swisseph/*.c)
SWE_OBJS := $(SWE_SRCS:.c=.o)

override CFLAGS += -Iinclude -Ivendor/sqlite -Ivendor/swisseph

LIBS     = -lm -lncursesw -lmenuw -licui18n -licuuc -licudata -lpthread -ldl

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
PREFIX   ?= /usr
BINDIR   := $(PREFIX)/bin
LOCALEDIR := $(PREFIX)/share/locale

DATADIR ?= $(PREFIX)/share/$(APP_NAME)
CFLAGS  += -DDATADIR=\"$(DATADIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\"

override CFLAGS += -Iinclude -D_DEFAULT_SOURCE -Wno-format-security \
                   -DVERSION=\"$(VERSION)\" -DAPPLICATION_NAME=\"$(APP_NAME)\" \
                   -DDATADIR=\"$(DATADIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\"

# Caminhos do Usuário (Configuração e Banco de Dados no XDG)
CONFIG_HOME := $(HOME)/.config
APP_CONFIG_DIR  := $(CONFIG_HOME)/$(APP_NAME)
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

# 1. Regra específica do SQLite (sem warnings)
vendor/sqlite/%.o: vendor/sqlite/%.c
	$(CC) -O2 -w -DSQLITE_ENABLE_ICU -Ivendor/sqlite -c $< -o $@

# 2. Regra específica da Swiss Ephemeris (sem warnings e compatível com GCC 14+)
vendor/swisseph/%.o: vendor/swisseph/%.c
	$(CC) $(STRICT_MATH_FLAGS) -w -std=gnu11 -Ivendor/swisseph -c $< -o $@

# 3. Regra normal para os arquivos do seu projeto (com -Wall -Wextra)
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@


# Regra para linkar o binário final
$(TARGET): $(OBJS) $(SQLITE_OBJ) $(SWE_OBJS)
	$(CC) $(OBJS) $(SQLITE_OBJ) $(SWE_OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)


# Regra para criar a estrutura de diretórios e injetar o banco no $HOME do usuário
setup-dir:
	@echo "Criando diretórios de configuração em $(APP_CONFIG_DIR)..."
	@mkdir -p $(CONFIG_HOME)
	@sleep 0.1
	@mkdir -p $(APP_CONFIG_DIR)
	@sleep 0.1
	@mkdir -p $(APP_CONFIG_DIR)/export
	@sleep 0.1
	@mkdir -p $(APP_CONFIG_DIR)/ephe
	@sleep 0.1 # Pequena pausa de segurança para o sistema operacional consolidar as pastas no disco
	@if [ -z "$$(ls -A $(APP_CONFIG_DIR)/ephe 2>/dev/null)" ]; then \
	    echo "Pasta ephe vazia. Copiando arquivos da Swiss Ephemeris..." ; \
	    cp -r ephe/* $(APP_CONFIG_DIR)/ephe/ ; \
	else \
	    echo "Arquivos da Swiss Ephemeris já existem em $(APP_CONFIG_DIR)/ephe/. Pulando cópia." ; \
	fi
	@sleep 0.1
	@mkdir -p $(APP_CONFIG_DIR)/database
	@if [ -z "$$(ls -A $(APP_CONFIG_DIR)/database 2>/dev/null)" ]; then \
	    echo "Pasta database vazia. Copiando arquivos de banco de dados da ZoneDetect..." ; \
	    cp -r database/* $(APP_CONFIG_DIR)/database/ ; \
	else \
	    echo "Arquivos da ZoneDetect já existem em $(APP_CONFIG_DIR)/database/. Pulando cópia." ; \
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
	xgettext --keyword=_  --keyword=N_ --language=C --from-code=UTF-8 --output=$(POT_FILE) $(SRCS)
	@echo "Mesclando novidades nos arquivos .po..."
	@if [ -f $(PO_DIR)/pt.po ]; then msgmerge --update $(PO_DIR)/pt.po $(POT_FILE); else msginit --no-translator --input=$(POT_FILE) --locale=pt --output=$(PO_DIR)/pt.po; fi
	@if [ -f $(PO_DIR)/en.po ]; then msgmerge --update $(PO_DIR)/en.po $(POT_FILE); else msginit --no-translator --input=$(POT_FILE) --locale=en --output=$(PO_DIR)/en.po; fi
	@if [ -f $(PO_DIR)/es.po ]; then msgmerge --update $(PO_DIR)/es.po $(POT_FILE); else msginit --no-translator --input=$(PO_DIR)/es.po --locale=es --output=$(PO_DIR)/es.po; fi

install: all
	@echo "Instalando o executável em $(DESTDIR)$(BINDIR)..."
	@install -d $(DESTDIR)$(BINDIR)
	@install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

	@echo "Instalando recursos e bases de dados em $(DESTDIR)$(DATADIR)..."
	@install -d $(DESTDIR)$(DATADIR)
	@if [ -f ".env" ]; then install -m 644 .env $(DESTDIR)$(DATADIR)/default.env; fi
	@if [ -f "$(DB_SQL)" ]; then install -m 644 $(DB_SQL) $(DESTDIR)$(DATADIR)/$(DB_SQL); fi

	@cp -f help_en.txt help_pt.txt topics_en.txt topics_pt.txt $(DESTDIR)$(DATADIR)/ 2>/dev/null || true

	@if [ -d "ephe" ]; then \
		cp -r ephe $(DESTDIR)$(DATADIR)/; \
	fi
	@if [ -d "database" ]; then \
		cp -r database $(DESTDIR)$(DATADIR)/; \
	fi
	@if [ -d "migrations" ]; then \
		cp -r migrations $(DESTDIR)$(DATADIR)/; \
	fi

	@echo "Instalando arquivos de tradução fixa do Gettext..."
	@for lang in pt en es; do \
		if [ -f locale/$$lang/LC_MESSAGES/$$lang.mo ]; then \
			install -d $(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES; \
			install -m 644 locale/$$lang/LC_MESSAGES/$$lang.mo $(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES/$$lang.mo; \
		fi; \
	done

	@install -d $(DESTDIR)$(PREFIX)/share/applications
	@install -m 644 packaging/morcenox-astro.desktop $(DESTDIR)$(PREFIX)/share/applications/morcenox-astro.desktop
	@install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps
	@install -m 644 packaging/morcenox-astro.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/morcenox-astro.png


# Regra de desinstalação limpa
uninstall:
	@echo "Removendo o executável de $(DESTDIR)$(BINDIR)..."
	@rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo "Removendo recursos de $(DESTDIR)$(DATADIR)..."
	@rm -rf $(DESTDIR)$(DATADIR)
	@echo "Removendo arquivos de tradução..."
	@rm -f $(DESTDIR)$(LOCALEDIR)/pt/LC_MESSAGES/pt.mo
	@rm -f $(DESTDIR)$(LOCALEDIR)/en/LC_MESSAGES/en.mo
	@rm -f $(DESTDIR)$(LOCALEDIR)/es/LC_MESSAGES/es.mo

	@rm -f $(DESTDIR)$(PREFIX)/share/applications/morcenox-astro.desktop
	@rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/morcenox-astro.png


# Regra para limpar os arquivos temporários de compilação locais
clean:
	rm -f src/*.o $(TARGET)

distclean: clean
	rm -f $(SQLITE_OBJ) $(SWE_OBJS)

# Regra para resetar o banco de dados do usuário ativo
reset-db: clean
	@if [ -f "$(DB_FILE)" ]; then \
		echo "Removendo o banco de dados do usuário em $(DB_FILE)..."; \
		rm -f "$(DB_FILE)"; \
	fi

.PHONY: package

package: all translate
	@echo "Packaging version $(VERSION) into $(PKG_DIR)..."
	@rm -rf $(PKG_DIR)/bin $(PKG_DIR)/locale $(PKG_DIR)/assets $(PKG_DIR)/astro.sh
	@mkdir -p $(PKG_DIR)/bin
	@mkdir -p $(PKG_DIR)/locale
	@mkdir -p $(PKG_DIR)/assets

	@cp $(TARGET) $(PKG_DIR)/bin/
	@cp -r locale/* $(PKG_DIR)/locale/ 2>/dev/null || true

	@if [ -f ".env" ]; then cp .env $(PKG_DIR)/assets/ ; fi
	@if [ -f "$(DB_SQL)" ]; then cp $(DB_SQL) $(PKG_DIR)/assets/ ; fi
	@cp help_en.txt help_pt.txt $(PKG_DIR)/assets/ 2>/dev/null || true
	@cp topics_en.txt topics_pt.txt $(PKG_DIR)/assets/ 2>/dev/null || true
	@if [ -d "ephe" ]; then cp -r ephe $(PKG_DIR)/assets/ ; fi
	@if [ -d "database" ]; then cp -r database $(PKG_DIR)/assets/ ; fi
	@if [ -d "migrations" ]; then cp -r migrations $(PKG_DIR)/assets/ ; fi

	@mkdir -p $(PKG_DIR)/assets/export 2>/dev/null || true

	@if [ -f "astro.sh" ]; then \
		cp astro.sh $(PKG_DIR)/; \
		chmod +x $(PKG_DIR)/astro.sh; \
	else \
		echo "ERROR: astro.sh not found! Package creation failed."; exit 1; \
	fi

	@if [ -f "packaging/morcenox-astro.png" ]; then cp packaging/morcenox-astro.png $(PKG_DIR)/assets/ ; fi
	@if [ -f "packaging/morcenox-astro.desktop" ]; then cp packaging/morcenox-astro.desktop $(PKG_DIR)/assets/ ; fi
	chmod +x $(PKG_DIR)/assets/morcenox-astro.desktop

	# Compacta diretamente dentro de $(PKG_DIR)/$(PACKAGE_NAME) e limpa os temporários
	tar -czf $(PKG_DIR)/$(PACKAGE_NAME) -C $(PKG_DIR) bin locale assets astro.sh
	@rm -rf $(PKG_DIR)/bin $(PKG_DIR)/locale $(PKG_DIR)/assets $(PKG_DIR)/astro.sh
	
	@echo "======================================================================="
	@echo " Package created: $(PKG_DIR)/$(PACKAGE_NAME)"
	@echo "======================================================================="


RPM_TMP_DIR  := build_rpm_tmp
SPEC_FILE    := packaging/$(APP_NAME).spec

.PHONY: rpm

rpm:
	@echo "======================================================================="
	@echo " Gerando pacote RPM isolado para $(APP_NAME) v$(VERSION)..."
	@echo "======================================================================="
	@rm -rf $(RPM_TMP_DIR)
	@mkdir -p $(RPM_TMP_DIR)/{BUILD,RPMS,SOURCES,SPECS,SRPMS} $(PKG_DIR)
	@tar --transform 's,^\.,$(APP_NAME)-$(VERSION),' \
	     -czf $(RPM_TMP_DIR)/SOURCES/$(APP_NAME)-$(VERSION).tar.gz \
	     --exclude=.git --exclude=$(PKG_DIR) --exclude=$(RPM_TMP_DIR) \
	     --exclude='*.o' --exclude=$(TARGET) .
	@rpmbuild --define "_topdir $(abspath $(RPM_TMP_DIR))" \
	          --define "_version $(VERSION)" \
	          -ba $(SPEC_FILE)
	@cp -f $(RPM_TMP_DIR)/RPMS/x86_64/$(APP_NAME)-$(VERSION)-*.rpm $(PKG_DIR)/
	@rm -rf $(RPM_TMP_DIR)
	@echo "======================================================================="
	@echo " RPM gerado com sucesso em: $(PKG_DIR)/"
	@ls -lh $(PKG_DIR)/$(APP_NAME)-$(VERSION)-*.rpm
	@echo "======================================================================="

DEB_TMP_DIR  := build_deb_tmp
ARCH_TMP_DIR := build_arch_tmp
PKGBUILD_TPL := packaging/PKGBUILD

.PHONY: deb arch release-all

# ==============================================================================
# GERADOR DE PACOTE .DEB (DEBIAN / UBUNTU) VIA PODMAN
# ==============================================================================
deb:
	@echo "======================================================================="
	@echo " Gerando pacote DEB para $(APP_NAME) v$(VERSION) via Podman..."
	@echo "======================================================================="
	@rm -rf $(DEB_TMP_DIR)
	@mkdir -p $(DEB_TMP_DIR)/src $(PKG_DIR)
	@tar --exclude=.git --exclude=$(PKG_DIR) --exclude=$(RPM_TMP_DIR) \
	     --exclude=$(DEB_TMP_DIR) --exclude=$(ARCH_TMP_DIR) \
	     --exclude='*.o' --exclude='vendor/*/*.o' --exclude=$(TARGET) -cf - . | tar -xf - -C $(DEB_TMP_DIR)/src
	@podman run --rm -v $(abspath $(DEB_TMP_DIR)):/build:Z -w /build/src docker.io/library/ubuntu:24.04 bash -c "\
		export DEBIAN_FRONTEND=noninteractive && \
		apt-get update -qq && \
		apt-get install -y -qq apt-utils gcc make gettext libncurses-dev libicu-dev dpkg-dev >/dev/null && \
		make PREFIX=/usr -j\$$(nproc) && \
		make install DESTDIR=/build/pkg PREFIX=/usr && \
		mkdir -p /build/pkg/DEBIAN debian && \
		printf 'Source: morcenox-astro\nSection: science\nPriority: optional\nMaintainer: MorceNOX <morcenox@example.com>\n\nPackage: morcenox-astro\nArchitecture: amd64\n' > debian/control && \
		DEPS=\$$(dpkg-shlibdeps -O /build/pkg/usr/bin/$(TARGET) | sed 's/shlibs:Depends=//') && \
		printf 'Package: morcenox-astro\nVersion: $(VERSION)\nSection: science\nPriority: optional\nArchitecture: amd64\nDepends: %s\nMaintainer: MorceNOX <morcenox@example.com>\nDescription: Traditional Astrology calculation engine and TUI\n Terminal-based Traditional Astrology suite written in C and ncurses,\n powered by Swiss Ephemeris, ZoneDetect, and SQLite.\n' \"\$$DEPS\" > /build/pkg/DEBIAN/control && \
		dpkg-deb --root-owner-group --build /build/pkg /build/morcenox-astro_$(VERSION)_amd64.deb && \
		rm -rf /build/pkg /build/src"
	@cp -f $(DEB_TMP_DIR)/morcenox-astro_$(VERSION)_amd64.deb $(PKG_DIR)/
	@rm -rf $(DEB_TMP_DIR)
	@echo "======================================================================="
	@echo " DEB gerado com sucesso em: $(PKG_DIR)/"
	@ls -lh $(PKG_DIR)/morcenox-astro_$(VERSION)_amd64.deb
	@echo "======================================================================="

# ==============================================================================
# GERADOR DE PACOTE ARCH LINUX (.PKG.TAR.ZST) VIA PODMAN
# ==============================================================================
arch:
	@echo "======================================================================="
	@echo " Gerando pacote Arch Linux para $(APP_NAME) v$(VERSION) via Podman..."
	@echo "======================================================================="
	@podman unshare rm -rf $(ARCH_TMP_DIR) 2>/dev/null || rm -rf $(ARCH_TMP_DIR)
	@mkdir -p $(ARCH_TMP_DIR) $(PKG_DIR)
	@tar --transform 's,^\.,$(APP_NAME)-$(VERSION),' \
	     -czf $(ARCH_TMP_DIR)/$(APP_NAME)-$(VERSION).tar.gz \
	     --exclude=.git --exclude=$(PKG_DIR) --exclude=$(RPM_TMP_DIR) \
	     --exclude=$(DEB_TMP_DIR) --exclude=$(ARCH_TMP_DIR) \
	     --exclude='*.o' --exclude='vendor/*/*.o' --exclude=$(TARGET) .
	@sed 's/^pkgver=.*/pkgver=$(VERSION)/' $(PKGBUILD_TPL) > $(ARCH_TMP_DIR)/PKGBUILD
	@podman run --rm -v $(abspath $(ARCH_TMP_DIR)):/build:Z -w /build docker.io/library/archlinux:latest bash -c "\
		pacman -Sy --noconfirm archlinux-keyring && \
		pacman -Syu --noconfirm --needed base-devel gettext ncurses icu && \
		ln -sfn /usr/include /usr/include/ncursesw && \
		useradd -m builder && \
		chown -R builder:builder /build && \
		su builder -c 'makepkg -f' && \
		chown -R 0:0 /build"
	@cp -f $(ARCH_TMP_DIR)/morcenox-astro-$(VERSION)-*.pkg.tar.zst $(PKG_DIR)/
	@podman unshare rm -rf $(ARCH_TMP_DIR) 2>/dev/null || rm -rf $(ARCH_TMP_DIR)
	@echo "======================================================================="
	@echo " Pacote Arch gerado com sucesso em: $(PKG_DIR)/"
	@ls -lh $(PKG_DIR)/morcenox-astro-$(VERSION)-*.pkg.tar.zst
	@echo "======================================================================="

release-all:
	@rm -rf $(PKG_DIR)
	@$(MAKE) package rpm deb arch
	@echo "======================================================================="
	@echo " TODOS OS PACOTES DA VERSAO $(VERSION) FORAM GERADOS EM $(PKG_DIR)/:"
	@ls -lh $(PKG_DIR)/*
	@echo "======================================================================="

.PHONY: package-arm64 rpm-arm64 deb-arm64 arch-arm64 release-arm64

# ==============================================================================
# 1. PACOTE PORTÁTIL .TAR.GZ (AARCH64) VIA PODMAN + QEMU
# ==============================================================================
package-arm64:
	@echo "======================================================================="
	@echo " Gerando pacote portátil TAR.GZ (aarch64) para $(APP_NAME) v$(VERSION)..."
	@echo "======================================================================="
	@rm -rf $(DEB_TMP_DIR)
	@mkdir -p $(DEB_TMP_DIR)/src $(PKG_DIR)
	@tar --exclude=.git --exclude=$(PKG_DIR) --exclude=$(RPM_TMP_DIR) \
	     --exclude=$(DEB_TMP_DIR) --exclude=$(ARCH_TMP_DIR) \
	     --exclude='*.o' --exclude='vendor/*/*.o' --exclude=$(TARGET) -cf - . | tar -xf - -C $(DEB_TMP_DIR)/src
	@podman run --rm --platform linux/arm64 -v $(abspath $(DEB_TMP_DIR)):/build:Z -w /build/src registry.fedoraproject.org/fedora:latest bash -c "\
		dnf install -y -q gcc make gettext ncurses-devel libicu-devel && \
		make all translate -j\$$(nproc) && \
		make package PACKAGE_NAME=$(APP_NAME)-v$(VERSION)-linux-aarch64.tar.gz && \
		cp -f $(PKG_DIR)/$(APP_NAME)-v$(VERSION)-linux-aarch64.tar.gz /build/"
	@cp -f $(DEB_TMP_DIR)/$(APP_NAME)-v$(VERSION)-linux-aarch64.tar.gz $(PKG_DIR)/
	@rm -rf $(DEB_TMP_DIR)
	@ls -lh $(PKG_DIR)/$(APP_NAME)-v$(VERSION)-linux-aarch64.tar.gz

# ==============================================================================
# 2. PACOTE .RPM (AARCH64) VIA PODMAN + QEMU
# ==============================================================================
rpm-arm64:
	@echo "======================================================================="
	@echo " Gerando pacote RPM (aarch64) para $(APP_NAME) v$(VERSION)..."
	@echo "======================================================================="
	@rm -rf $(RPM_TMP_DIR)
	@mkdir -p $(RPM_TMP_DIR)/{BUILD,RPMS,SOURCES,SPECS,SRPMS} $(PKG_DIR)
	@tar --transform 's,^\.,$(APP_NAME)-$(VERSION),' \
	     -czf $(RPM_TMP_DIR)/SOURCES/$(APP_NAME)-$(VERSION).tar.gz \
	     --exclude=.git --exclude=$(PKG_DIR) --exclude=$(RPM_TMP_DIR) \
	     --exclude=$(DEB_TMP_DIR) --exclude=$(ARCH_TMP_DIR) \
	     --exclude='*.o' --exclude='vendor/*/*.o' --exclude=$(TARGET) .
	@cp -f $(SPEC_FILE) $(RPM_TMP_DIR)/SPECS/
	@podman run --rm --platform linux/arm64 -v $(abspath $(RPM_TMP_DIR)):/rpmbuild:Z registry.fedoraproject.org/fedora:latest bash -c "\
		dnf install -y -q rpm-build gcc make gettext ncurses-devel libicu-devel && \
		rpmbuild --define '_topdir /rpmbuild' --define '_version $(VERSION)' -ba /rpmbuild/SPECS/$(APP_NAME).spec"
	@cp -f $(RPM_TMP_DIR)/RPMS/aarch64/$(APP_NAME)-$(VERSION)-*.aarch64.rpm $(PKG_DIR)/
	@rm -rf $(RPM_TMP_DIR)
	@ls -lh $(PKG_DIR)/$(APP_NAME)-$(VERSION)-*.aarch64.rpm

# ==============================================================================
# 3. PACOTE .DEB (ARM64) VIA PODMAN + QEMU
# ==============================================================================
deb-arm64:
	@echo "======================================================================="
	@echo " Gerando pacote DEB (arm64) para $(APP_NAME) v$(VERSION)..."
	@echo "======================================================================="
	@rm -rf $(DEB_TMP_DIR)
	@mkdir -p $(DEB_TMP_DIR)/src $(PKG_DIR)
	@tar --exclude=.git --exclude=$(PKG_DIR) --exclude=$(RPM_TMP_DIR) \
	     --exclude=$(DEB_TMP_DIR) --exclude=$(ARCH_TMP_DIR) \
	     --exclude='*.o' --exclude='vendor/*/*.o' --exclude=$(TARGET) -cf - . | tar -xf - -C $(DEB_TMP_DIR)/src
	@podman run --rm --platform linux/arm64 -v $(abspath $(DEB_TMP_DIR)):/build:Z -w /build/src docker.io/library/ubuntu:24.04 bash -c "\
		export DEBIAN_FRONTEND=noninteractive && \
		apt-get update -qq && \
		apt-get install -y -qq apt-utils gcc make gettext libncurses-dev libicu-dev dpkg-dev >/dev/null && \
		make PREFIX=/usr -j\$$(nproc) && \
		make install DESTDIR=/build/pkg PREFIX=/usr && \
		mkdir -p /build/pkg/DEBIAN debian && \
		printf 'Source: morcenox-astro\nSection: science\nPriority: optional\nMaintainer: MorceNOX <morcenox@example.com>\n\nPackage: morcenox-astro\nArchitecture: arm64\n' > debian/control && \
		DEPS=\$$(dpkg-shlibdeps -O /build/pkg/usr/bin/$(TARGET) | sed 's/shlibs:Depends=//') && \
		printf 'Package: morcenox-astro\nVersion: $(VERSION)\nSection: science\nPriority: optional\nArchitecture: arm64\nDepends: %s\nMaintainer: MorceNOX <morcenox@example.com>\nDescription: Traditional Astrology calculation engine and TUI\n Terminal-based Traditional Astrology suite written in C and ncurses,\n powered by Swiss Ephemeris, ZoneDetect, and SQLite.\n' \"\$$DEPS\" > /build/pkg/DEBIAN/control && \
		dpkg-deb --root-owner-group --build /build/pkg /build/morcenox-astro_$(VERSION)_arm64.deb && \
		rm -rf /build/pkg /build/src"
	@cp -f $(DEB_TMP_DIR)/morcenox-astro_$(VERSION)_arm64.deb $(PKG_DIR)/
	@rm -rf $(DEB_TMP_DIR)
	@ls -lh $(PKG_DIR)/morcenox-astro_$(VERSION)_arm64.deb

# ==============================================================================
# 4. PACOTE ARCH LINUX (AARCH64) VIA PODMAN + QEMU
# ==============================================================================
arch-arm64:
	@echo "======================================================================="
	@echo " Gerando pacote Arch Linux (aarch64) para $(APP_NAME) v$(VERSION)..."
	@echo "======================================================================="
	@podman unshare rm -rf $(ARCH_TMP_DIR) 2>/dev/null || rm -rf $(ARCH_TMP_DIR)
	@mkdir -p $(ARCH_TMP_DIR) $(PKG_DIR)
	@tar --transform 's,^\.,$(APP_NAME)-$(VERSION),' \
	     -czf $(ARCH_TMP_DIR)/$(APP_NAME)-$(VERSION).tar.gz \
	     --exclude=.git --exclude=$(PKG_DIR) --exclude=$(RPM_TMP_DIR) \
	     --exclude=$(DEB_TMP_DIR) --exclude=$(ARCH_TMP_DIR) \
	     --exclude='*.o' --exclude='vendor/*/*.o' --exclude=$(TARGET) .
	@sed 's/^pkgver=.*/pkgver=$(VERSION)/' $(PKGBUILD_TPL) > $(ARCH_TMP_DIR)/PKGBUILD
	@podman run --rm --privileged --platform linux/arm64 -v $(abspath $(ARCH_TMP_DIR)):/build:Z -w /build docker.io/menci/archlinuxarm:latest bash -c "\
		sed -i 's/^DownloadUser/#DownloadUser/' /etc/pacman.conf && \
		pacman -Sy --noconfirm --disable-sandbox archlinuxarm-keyring && \
		pacman -Syu --noconfirm --disable-sandbox --needed base-devel gettext ncurses icu && \
		ln -sfn /usr/include /usr/include/ncursesw && \
		useradd -m builder && \
		chown -R builder:builder /build && \
		su builder -c 'makepkg -f' && \
		chown -R 0:0 /build"
	@cp -f $(ARCH_TMP_DIR)/morcenox-astro-$(VERSION)-*-aarch64.pkg.tar.* $(PKG_DIR)/
	@podman unshare rm -rf $(ARCH_TMP_DIR) 2>/dev/null || rm -rf $(ARCH_TMP_DIR)
	@ls -lh $(PKG_DIR)/morcenox-astro-$(VERSION)-*-aarch64.pkg.tar.*

# Gera todos os pacotes ARM64 de uma só vez
release-arm64: package-arm64 rpm-arm64 deb-arm64 arch-arm64
	@echo "======================================================================="
	@echo " PACOTES ARM64 (AARCH64) DA VERSAO $(VERSION) PRONTOS EM $(PKG_DIR)/:"
	@ls -lh $(PKG_DIR)/*aarch64* $(PKG_DIR)/*arm64*
	@echo "======================================================================="
