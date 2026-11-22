# MorceNOX™ ASTRO

<img width="866" height="509" alt="MorceNOX-ASTRO-logo" src="MorceNOX-Astro.png" />

<p align="right">
  <a href="README.md"><img src="https://flagcdn.com/w40/gb.png" width="25" alt="English"></a>
  <a href="README-pt.md"><img src="https://flagcdn.com/w40/br.png" width="25" alt="Português"></a>
</p>

**Seu companheiro astrológico avançado, baseado em terminal.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: Terminal](https://img.shields.io/badge/Platform-Terminal-lightgrey.svg)](https://en.wikipedia.org/wiki/Terminal)
[![GitHub Release](https://img.shields.io/github/v/release/MorceNOX/morcenox-astro?logo=github&label=Release)](https://github.com/MorceNOX/morcenox-astro/releases)
[![Build & Publish Universal Linux Packages](https://github.com/MorceNOX/morcenox-astro/actions/workflows/release.yml/badge.svg)](https://github.com/MorceNOX/morcenox-astro/actions/workflows/release.yml)
[![Copr build status](https://copr.fedorainfracloud.org/coprs/morcenox/morcenox-astro/package/MorceNOX-Astro/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/morcenox/morcenox-astro/package/MorceNOX-Astro/)

Um aplicativo de Astrologia Tradicional rodando no seu terminal! Calcule seu mapa natal ou visualize rapidamente o mapa para o momento atual. Verifique a tabela de horas planetárias para escolher o melhor momento para uma atividade, explore sua análise de temperamento e mente, navegue por eventos passados e futuros em uma tabela de Direções Primárias, e muito mais!

**MorceNOX™ ASTRO** é um motor astrológico de alto desempenho, visualmente impactante e preciso, projetado para o ambiente de terminal. Construído em C com a Swiss Ephemeris, ele fornece aos astrólogos uma ferramenta poderosa para calcular e visualizar mapas clássicos complexos, desde o Mapa Natal e de Retorno Solar até as técnicas avançadas de regência temporal (Time-Lord) como Firdária e Direções Primárias, acompanhados de cálculos exatos para Sizígia Pré-Natal, Partes Árabes e Estrelas Fixas.

Possui uma interface rica em arte ASCII, animação celestial em tempo real e capacidade total offline.

<hr>
<img width="2048" height="1233" alt="01-chart-wheel-pres-pt" src="https://github.com/user-attachments/assets/7652045b-9baf-4068-ae31-edcb7a2c5653" />
<hr>

## 🌌 Principais Recursos

### 🛠 Mapas e Cálculos Avançados
* **Suporte a Múltiplos Mapas:** Construa mapas Natais (Radix), de Retorno Solar (Anuais) e o mapa do momento atual com animação em tempo real .
* **Análise de Temperamento:** Um cálculo preciso do temperamento do nativo, exibindo um gráfico com os temperamentos predominantes e sua interpretação.
* **Força dos Planetas:** Calcule o **Almuten Figuris** e a **Força dos Planetas**, de acordo com as dignidades *Essenciais* e *Acidentais*. 
* **Motor de Retorno Solar:** Um sofisticado sistema de "Confrontação de Radix" que examina o mapa anual contra o mapa natal através de sete pontos de verificação distintos (Almuten, Trânsitos de Casas, Co-alinhamento de Regentes, Ascendente, etc.). Também verifica os trânsitos dos planetas contra as posições natais (radicais), incluindo as Partes Árabes e a **Ativação das Estrelas Fixas**, bem como as posições/status das estrelas no mapa radical.
* **Técnicas de Regência Temporal (Time-Lord):** Módulos integrados para **Firdária**, **Profeções Anuais** e **Cronocratas Vitais** (Hyleg e Alcochoden).
* **Direções Primárias:** Cálculo preciso de direções Diretas e Conversas para planetas, ângulos e Partes Árabes usando várias chaves de tempo e três métodos de cálculo, tanto para as direções *zodíacais com latitude* e *mundanas*, incluindo direções para Partes Árabes e significadores dirigidos às Estrelas Fixas.
* **Partes Árabes:** Crie, edite e gerencie a sua coleção personalizada de Partes Árabes, com cálculo automático de aspectos.
* **Anareta & Ameaças Vitais:** Identifique potenciais riscos físicos e ameaças vitais baseadas no regente da 8ª casa e no planeta Anareta.
* **Estrelas Fixas:** Selecione, calcule, visualize os **Aspectos**, **Parans**, **Fases Heliacais** e a **Ativação** das **Estrelas Fixas** no Mapa de Retorno Solar, com a opção de incluí-las como promissoras nas Direções Primárias;
* e muito mais!

### 📐 Precisão Matemática
* **Profundidade Astrológica:** Inclui análise do Almuten Figuris e dos Pontos Hylegíacos, pontuação da força dos planetas, conforme as Dignidades Essenciais e Acidentais, e análise do Temperamento e da Mente.
* **Sistemas de Casas:** Suporta uma ampla gama de sistemas, incluindo Signos Inteiros (Whole Signs), Campanus, Regiomontanus, Placidus, Koch e Topocêntrico.
* **Horas Planetárias:** Cálculo completo das horas planetárias para dia e noite, sincronizado com o nascer e pôr do sol.

### 🖥 Interface de Terminal Interativa
* **Excelência Visual:** Rodas de mapa em arte ASCII de alta resolução com cores vibrantes e elementos codificados por cores (Fogo, Terra, Ar, Água), cores individuais para cada planeta, etc.
* **Interação Dinâmica:** Zoom in/out, pan (movimentação) do mapa e alternância de sobreposições como Decanatos, Termos e Limites de Casa.
* **Modo de Animação:** Visualize e controle o movimento passado e futuro dos corpos celestes ajustando a velocidade da animação.
* **Capacidade Total Offline:** Operação 100% offline. Nenhum dado é enviado ou recebido da internet.
* **Suporte ao Mouse:** Clique duas vezes em uma opção dos menus para executá-la! A Roda do Mapa e todas as janelas são clicáveis. Você pode mover o mapa arrastando e soltando. O clique duplo permite zoom; todos os botões, barras de rolagem e janelas são interativos.
* **Ajuda Extensa:** Você pode ler em uma janela apropriada sobre qualquer parte do programa apenas selecionando a opção *Ajuda* e escolhendo um tópico.

### 🌎 Suporte Multilíngue
* O MorceNOX™ ASTRO está disponível inteiramente em **Inglês** e **Português do Brasil** na mesma versão. Basta selecionar o idioma no menu de Configurações (Settings).

## 🛠 Stack Tecnológica

* **Core:** C
* **Efemérides:** [Swiss Ephemeris](https://github.com/aloistr/swisseph) (para precisão astronômica).
* **UI/UX:** `ncurses` (para a interface baseada em terminal).
* **Banco de Dados:** [SQLite](https://www.sqlite.org/index.html) (para armazenar mapas, configurações e um enorme banco de dados de cidades).
* **Fonte de Dados:** Versão aprimorada do `countries-states-cities-database` (aprox. 153 mil cidades).
* **Internacionalização:** `libicu` (suporte Unicode/ICU) e `ZoneDetect` (https://github.com/BertoldVdb/ZoneDetect).

## 🚀 Primeiros Passos

Existem três maneiras de instalar e rodar o **MorceNOX™ ASTRO** no Linux.

### 🐧 Opção 1: Pacotes Nativos Linux (Recomendado)

#### Fedora / RHEL / CentOS — Via Fedora COPR (Atualizações Automáticas)
Para usuários Fedora, recomendamos habilitar nosso repositório oficial [Fedora COpr](https://copr.fedorainfracloud.org/coprs/morcenox/morcenox-astro/) (`x86_64` e `aarch64`) e instalar o programa diretamente via `dnf` para receber atualizações automáticas:

```bash
sudo dnf copr enable morcenox/morcenox-astro
sudo dnf install MorceNOX-Astro
```

---

#### Instalação Manual de Pacote (`.rpm`, `.deb`, `.pkg.tar.zst`)
Alternativamente, pacotes pré-compilados estão disponíveis para as principais famílias de distribuições Linux. Tanto a **Swiss Ephemeris** quanto o **SQLite3** (com suporte a ICU) estão incorporados estaticamente no binário, e todas as bibliotecas de tempo de execução necessárias (`ncurses` e `libicu`) são resolvidas automaticamente pelo seu gerenciador de pacotes.

1. Vá para a página de [Releases](https://github.com/MorceNOX/morcenox-astro/releases) e baixe o pacote para sua distribuição.
2. Instale usando o gerenciador de pacotes do seu sistema:

##### Fedora / RHEL / CentOS (`.rpm`)
```bash
sudo dnf install ./MorceNOX-Astro-X.Y.Z-1.*.x86_64.rpm
```
ou
```bash
sudo dnf install ./MorceNOX-Astro-X.Y.Z-1.*.aarch64.rpm
```

##### Debian / Ubuntu / Linux Mint / Pop!_OS (`.deb`)
```bash
sudo apt update
sudo apt install ./morcenox-astro_X.Y.Z_amd64.deb
```
ou
```bash
sudo apt update
sudo apt install ./morcenox-astro_X.Y.Z_arm64.deb
```

##### Arch Linux / Manjaro / EndeavourOS (`.pkg.tar.zst`)
```bash
sudo pacman -U ./morcenox-astro-X.Y.Z-1-x86_64.pkg.tar.zst
```
ou
```bash
sudo pacman -U ./morcenox-astro-X.Y.Z-1-aarch64.pkg.tar.xz
```


3. Inicie o aplicativo de qualquer terminal:
```bash
astro
```
> **Nota:** Na primeira execução, o `astro` cria automaticamente `$HOME/.config/MorceNOX-Astro`, configura seus arquivos de configuração e inicializa o banco de dados SQLite de cidades.

---

### 🌟 Opção 2: Versão Portátil

Use esta opção se preferir rodar o aplicativo a partir de uma pasta autocontida, sem instalação no sistema (requer as bibliotecas de tempo de execução `ncurses` e `libicu` no host).

1. Vá para a página de [Releases](https://github.com/MorceNOX/morcenox-astro/releases).
2. Baixe o arquivo mais recente `MorceNOX-Astro-vX.Y.Z-linux.tar.gz`.
3. Extraia para sua pasta preferida:
   ```bash
   tar -xzvf MorceNOX-Astro-vX.Y.Z-linux.tar.gz
   ```
4. Execute o launcher:
   ```bash
   cd MorceNOX-Astro-vX.Y.Z-linux
   ./astro.sh
   ```

---

### 🛠 Opção 3: Compilando do Código-Fonte

Use esta opção se desejar customizar o código ou compilar para uma configuração Linux personalizada.

#### Pré-requisitos:
* **Fedora / RHEL:**
  ```bash
  sudo dnf install gcc make gettext ncurses-devel libicu-devel
  ```
* **Ubuntu / Debian:**
  ```bash
  sudo apt-get install build-essential gettext libncurses-dev libicu-dev
  ```
* **Arch Linux:**
  ```bash
  sudo pacman -S base-devel gettext ncurses icu
  ```

#### Passos de Compilação e Instalação:
```bash
# Clone o repositório
git clone https://github.com/MorceNOX/morcenox-astro.git
cd morcenox-astro

# Compile o motor e as traduções
make -j\$(nproc)

# Instale no sistema (prefixo padrão: /usr/local)
sudo make install
```

Depois, basta rodar o aplicativo digitando: `astro`

---

### 🖥️ Integração com Desktop

Embora o **MorceNOX™ ASTRO** seja um aplicativo de terminal (`ncurses`), ele inclui integração completa com Freedesktop (`.desktop`) e ícones de alta resolução:

* **Pacotes de Sistema (`.rpm`, `.deb`, Arch):** Instala automaticamente o atalho do aplicativo e o ícone em `/usr/share/applications` e `/usr/share/icons`, tornando o **MorceNOX Astro** imediatamente pesquisável no menu de aplicativos do seu ambiente de desktop (GNOME, KDE, XFCE, etc.).
* **Versão Portátil (`.tar.gz`):** Na primeira execução de `./astro.sh`, o launcher registra automaticamente uma entrada de desktop e um ícone local dentro de `~/.local/share/applications` apontando para sua pasta extraída — sem a necessidade de privilégios de `root`.


## ⌨️ Uso & Navegação

O MorceNOX™ Astro foi projetado para velocidade, precisão de cálculo e para ser um aplicativo astrológico visualmente atraente e fácil de usar. A maioria das ações requer apenas uma ou duas teclas, ou um clique/duplo clique.

* **Menu Principal:** Use as `Teclas de Seta` para navegar e `Enter` para selecionar, ou `clique duas vezes` em uma opção.
* **Janela do Mapa:**
    * `[M]` - Abrir Menu de Ações.
    * `[A]` - Alternar Animação.
    * `[+]` / `[-]` - Zoom In / Zoom Out.
    * `[D]` - Exibir Decanatos.
    * `[B]` - Exibir Términos (Bounds).
    * `[H]` - Exibir Limites de Casa.
    * `[←↓↑→] ou arrastar e soltar` - Pan (Movimentação).
    * `[R]` - Resetar Zoom e Pan.
* **Sub-módulos:** Acessados via teclas de Função, Numéricas e Letras (ex: `F1` para Dados, `F3` para Aspectos, `F12` para Firdaria, `3` para Direções Primárias), e muito mais. Selecione a opção `Help` para ver todas as teclas de atalho disponíveis e ler sobre qualquer parte do aplicativo.

## Screenshots

Aqui estão algumas capturas de tela. Você pode ter uma ideia do que esperar deste aplicativo. E há muitos outros módulos para explorar!

### Menu Principal
<hr>

<img width="2048" height="1233" alt="00-main-menu-pt" src="https://github.com/user-attachments/assets/a32fd8ea-11b0-417b-9a6e-f8dd8922cbf8" />

<hr>

### A Roda do Mapa (Chart Wheel)
<hr>

<img width="2048" height="1233" alt="01-chart-wheel-pt" src="https://github.com/user-attachments/assets/cbc8815a-3943-434b-b646-295078dcbbb6" />

<img width="2048" height="1233" alt="01-chart-zoom-pt" src="https://github.com/user-attachments/assets/65f7682d-0548-4dcb-971f-c7203f8eede8" />

<img width="2048" height="1233" alt="01-chart-zoom-house-div-pt" src="https://github.com/user-attachments/assets/cddcb583-53d2-4362-ab7a-cd1c3eb0d81e" />

<img width="2048" height="1233" alt="01-chart-menu-actions-pt" src="https://github.com/user-attachments/assets/bd80d6a2-12d7-415e-8318-7ca53cdff2de" />

<hr>

### Tabelas de Aspectos
<hr>

<img width="2048" height="1233" alt="10-aspects-pt" src="https://github.com/user-attachments/assets/403c4fc0-c66f-42df-a45a-5a27953d05b0" />

<img width="2048" height="1233" alt="10-aspects-parallel-pt" src="https://github.com/user-attachments/assets/174f1c03-d35d-4fd4-b7b7-a980fca85d07" />

<img width="2048" height="1233" alt="09-houses-aspects-pt" src="https://github.com/user-attachments/assets/b17f7725-326c-46eb-a8e5-8f707447b9f8" />

<img width="2048" height="1233" alt="05-stars-aspects-conjunctions-pt" src="https://github.com/user-attachments/assets/0dbd3de5-c021-41a5-a871-2e1e9d209902" />

<hr>

### Direções Primárias
<hr>

<img width="2048" height="1233" alt="02-directions-pt" src="https://github.com/user-attachments/assets/65e60e2d-7a35-4aaa-b322-79849e1c85bc" />

<hr>

### Firdária
<hr>

<img width="2048" height="1233" alt="04-firdaria-pt" src="https://github.com/user-attachments/assets/4a2cc27b-9889-40da-b313-d357384e812e" />

<hr>

### Gráfico de Perfil Energético
<hr>

<img width="2048" height="1233" alt="06-energy-pt" src="https://github.com/user-attachments/assets/24ea3cc2-6193-47db-925d-4f4d0cc94f1d" />

<hr>

### Gráfico de Temperamento
<hr>

<img width="2048" height="1233" alt="08-temperament-pt" src="https://github.com/user-attachments/assets/91b33811-14a7-472b-8211-ac981f6b21e1" />

<hr>

### Interpretação de Temperamento
<hr>

<img width="2048" height="1233" alt="08-temperament-interpretation-pt" src="https://github.com/user-attachments/assets/d72724da-0bfc-4304-ada3-fc40a606d646" />

<hr>

### Retorno Solar
<hr>

<img width="2048" height="1233" alt="03-solar-return-chart-pt" src="https://github.com/user-attachments/assets/d2532083-0994-4ab5-9165-bea4209f0782" />

<hr>

### Confrontação de Radix no Retorno Solar
<hr>

<img width="2048" height="1233" alt="03-solar-return-confront-pt" src="https://github.com/user-attachments/assets/53e7c252-8341-4f8d-b337-1c3c2cffe36e" />

<hr>

### Projeção de Trânsito no Retorno Solar
<hr>

<img width="2048" height="1233" alt="03-solar-return-transit-pt" src="https://github.com/user-attachments/assets/2bd6e89d-a3e7-4e9d-9d63-db87201e9bcd" />

<hr>

### Horas Planetárias
<hr>

<img width="2048" height="1233" alt="07-planetary-hours-pt" src="https://github.com/user-attachments/assets/dce8ed4c-d869-496e-aa0d-1be4bdd66894" />

<hr>

## Licenças


### 📜 Licenças de Bibliotecas e Recursos de Terceiros




#### • Swiss Ephemeris

  https://github.com/aloistr/swisseph

  GNU Affero General Public License (AGPL)

  <https://www.gnu.org/licenses/agpl-3.0.ca.html>




#### • countries-states-cities-database

  https://github.com/dr5hn/countries-states-cities-database

  Licença ODbL-1.0

  <https://opendatacommons.org/licenses/odbl/1.0/>




#### • SQLite

  httpshttps://sqlite.org/

  Domínio Público




#### • ncurses

  https://invisible-island.net/ncurses/

  Licença NCURSES

  <https://invisible-island.net/ncurses/ncurses-license.html>




#### • Biblioteca ICU (libicu)

  https://icu.unicode.org/

  (https://github.com/unicode-org/icu/blob/main/LICENSE)




#### • ZoneDetect

  https://github.com/BertoldVdb/ZoneDetect

  <https://github.com/BertoldVdb/ZoneDetect#BSD-3-Clause-1-ov-file>

  Copyright (c) 2018-2019, Bertold Van den Bergh

  (vandenbergh@bertold.org) Todos os direitos reservados.

<hr>



### Licença

Este programa é um software livre. Você pode redistribuí-lo e/ou modificá-lo sob os termos da **Licença Pública Geral GNU (GPL)** conforme publicada pela Free Software Foundation.

> Tudo isso está sendo feito para colocar em prática o que aprendi sobre astrologia e os cálculos astronômicos associados. Portanto, é possível que alguém encontre algo incorreto ou incomum. Nesse caso, eu agradeceria qualquer ajuda ou sugestão para tornar este aplicativo ainda melhor.

## 👤 Autor

**Amilcar Antonio Mesquita Rizk**
*Copyright © 2026*

---
*Aviso Legal: Este software é fornecido "como está", sem garantia de qualquer tipo. Use para pesquisa astrológica e prática profissional por sua conta e risco.*
