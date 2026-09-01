/*
 * MorceNOX-ASTRO™
 * Copyright (C) 2026 Amilcar Antonio Mesquita Rizk amilcar.rizk@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://gnu.org>.
 */

#define _XOPEN_SOURCE
#define NCURSES_WIDECHAR 1
#include "swephexp.h"
#include <ncurses.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <string.h>
#include <wchar.h>
#include <time.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <menu.h>
#include <ctype.h>
#include "var.h"
#include "helper.h"
#include "planet_table.h"
#include "directions.h"



const char *str_dow(int dow) {
    switch(dow) {
        case 0: return _("Sunday");
        case 1: return _("Monday");
        case 2: return _("Tuesday");
        case 3: return _("Wednesday");
        case 4: return _("Thursday");
        case 5: return _("Friday");
        case 6: return _("Saturday");
    }
    return "";
}

int get_int_greater_if_found(int *array, int count, int n, int distance) {
    int found = 0;    
    do {
        found = 0;
        for (int i = 0; i < count; i++) {
            if (n == array[i]) {
                found = 1;
                break;
            }
        }

        if (found) {
            n += distance;
        }
    } while (found);

    return n;
    
}

int get_int_lesser_if_found(int *array, int count, int n, int distance) {
    int found = 0;    
    do {
        found = 0;
        for (int i = 0; i < count; i++) {
            if (n == array[i]) {
                found = 1;
                break;
            }
        }

        if (found) {
            n -= distance;
        }
    } while (found);

    return n;
    
}

// Para comparar AuxOrdenacao
int comparar_distantes(const void *a, const void *b) {
    double dist_a = ((AuxOrdenacao*)a)->dist_relativa;
    double dist_b = ((AuxOrdenacao*)b)->dist_relativa;
    return (dist_a > dist_b) - (dist_a < dist_b);
}


int comparar_doubles(const void *a, const void *b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    
    if (da < db) return -1;
    if (da > db) return 1;
    return 0; // Se forem iguais
}


int comparar_directions_por_idade(const void *a, const void *b) {
    // 1. Converte os ponteiros void para ponteiros da sua struct
    const LinhaDirecao *objA = (const LinhaDirecao *)a;
    const LinhaDirecao *objB = (const LinhaDirecao *)b;

    // 2. Compara os valores double evitando erros de precisão no retorno int
    if (objA->idade_evento < objB->idade_evento) return -1;
    if (objA->idade_evento > objB->idade_evento) return 1;
    return 0; // Caso sejam iguais
}


int comparar_plots_por_id(const void *a, const void *b) {
    // 1. Converte os ponteiros void para ponteiros da sua struct
    const PlotObject *objA = (const PlotObject *)a;
    const PlotObject *objB = (const PlotObject *)b;

    // 2. Compara os valores double evitando erros de precisão no retorno int
    if (objA->id < objB->id) return -1;
    if (objA->id > objB->id) return 1;
    return 0; // Caso sejam iguais
}

int comparar_plots_por_longitude(const void *a, const void *b) {
    // 1. Converte os ponteiros void para ponteiros da sua struct
    const PlotObject *objA = (const PlotObject *)a;
    const PlotObject *objB = (const PlotObject *)b;

    // 2. Compara os valores double evitando erros de precisão no retorno int
    if (objA->longitude < objB->longitude) return -1;
    if (objA->longitude > objB->longitude) return 1;
    return 0; // Caso sejam iguais
}

int comparar_zodiac_por_longitude(const void *a, const void *b) {
    // 1. Converte os ponteiros void para ponteiros da sua struct
    const ZodiacLongitude *objA = (const ZodiacLongitude *)a;
    const ZodiacLongitude *objB = (const ZodiacLongitude *)b;

    // 2. Compara os valores double evitando erros de precisão no retorno int
    if (objA->longitude < objB->longitude) return -1;
    if (objA->longitude > objB->longitude) return 1;
    return 0; // Caso sejam iguais
}





/**
 * Função auxiliar para testar se o caminho de efemérides atual contém arquivos válidos.
 * Tenta calcular o Sol usando alta precisão. Se falhar, o caminho é inválido.
 */
int testar_caminho_efemerides() {
    double x2[6];
    char err_msg[256];
    // Dia Juliano arbitrário para o teste (J2000.0)
    double jd_teste = 2451545.0; 
    
    // Força a busca pelo arquivo físico de alta precisão (SEFLG_SWIEPH)
    if (swe_calc_ut(jd_teste, SE_SUN, SEFLG_SWIEPH, x2, err_msg) == ERR) {
        return 0; // Falhou: Arquivos ausentes ou corrompidos no caminho definido
    }
    return 1; // Sucesso: Caminho funcional
}

int inicializar_swiss_ephemeris() {
    char caminho_completo[1024];
    char *env_path = getenv("SE_EPHE_PATH");

    // --- TENTATIVA 1: Variável de Ambiente SE_EPHE_PATH ---
    if (env_path != NULL && strlen(env_path) > 0) {
        swe_set_ephe_path(env_path);
        
        if (testar_caminho_efemerides()) {
            fprintf(stderr, "Swiss Ephemeris inicializada via SE_EPHE_PATH: %s\n", env_path);
            return 1; 
        }
        fprintf(stderr, "Aviso: SE_EPHE_PATH definida, mas os arquivos de efemérides são inválidos.\n");
    }

    // --- TENTATIVA 2: Fallback para CONFIG_PATH/ephe ---
    // Monta o caminho dinamicamente combinando a sua global CONFIG_PATH com a pasta 'ephe'
    snprintf(caminho_completo, sizeof(caminho_completo), "%s/ephe", CONFIG_PATH);
    
    swe_set_ephe_path(caminho_completo);
    if (testar_caminho_efemerides()) {
        fprintf(stderr, "Swiss Ephemeris inicializada via CONFIG_PATH: %s\n", caminho_completo);
        return 1;
    }

    // --- TENTATIVA 3: Último recurso (Modelo de Moshier integrado) ---
    // Se nenhum arquivo físico (.se1) foi achado, avisamos o usuário mas NÃO paramos a aplicação.
    // O modelo analítico de Moshier roda sem arquivos externos e mantém o programa vivo.
    fprintf(stderr, "Alerta: Arquivos físicos de efemérides não encontrados. Usando modelo analítico de Moshier (precisão reduzida).\n");
    
    // Passamos uma string vazia para forçar o Moshier nativo da biblioteca
    swe_set_ephe_path(""); 
    return 1; 
}


int verificar_arquivos_efemerides() {
    double julian_day = 2461215.0; 
    int32 ipl = SE_SUN;            
    int32 iflag = SEFLG_SWIEPH;    
    double xx[6]; // Array obrigatório de 6 posições para evitar crash
    char serr[256];

    // Executa o cálculo teste
    int32 return_flags = swe_calc_ut(julian_day, ipl, iflag, xx, serr);

    // Cenário 1: Erro crítico na biblioteca (data impossível, etc)
    if (return_flags < 0) {
        return 0; 
    }

    // Cenário 2: O arquivo .se1 NÃO foi encontrado e a lib usou Moshier de fallback
    if (return_flags & SEFLG_MOSEPH) {
        return 0; // Alta precisão falhou!
    } 
    
    // Cenário 3: O bit SEFLG_SWIEPH se manteve ativo (arquivos físicos carregados com sucesso)
    if (return_flags & SEFLG_SWIEPH) {
        return 1; // Alta precisão confirmada!
    }

    return 0;
}
    

int get_visual_width(const char *str) {
    if (str == NULL) return 0;
    size_t len = mbstowcs(NULL, str, 0);
    if (len == (size_t)-1) return strlen(str); 

    wchar_t *wstr = malloc((len + 1) * sizeof(wchar_t));
    mbstowcs(wstr, str, len + 1);

    int width = 0;
    for (size_t i = 0; i < len; i++) {
        int w = wcwidth(wstr[i]);
        if (w > 0) width += w;
    }
    free(wstr);
    return width;
}

unsigned short get_terminal_width(void) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        return ws.ws_col;
    } else if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0) {
        return ws.ws_col;
    } else if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) == 0) {
        return ws.ws_col;
    }

    return 80;
}

unsigned short get_terminal_height(void) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        return ws.ws_row;
    } else if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0) {
        return ws.ws_row;
    } else if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) == 0) {
        return ws.ws_row;
    }

    return 80;
}


/**
 * Handles: Visual width calculation, padding math, and ncurses drawing.
 * @param win: The window to draw on
 * @param y: Vertical position
 * @param x: Horizontal starting position
 * @param available_width: The width of the area we are centering within
 * @param text: The string to print
 * @param attr: The ncurses attribute (color/bold/etc)
 */
void draw_centered_text(WINDOW *win, int y, int x, int available_width, const char *text, int attr) {
    if (!text) return;

    int text_w = get_visual_width(text);
    
    // If text is wider than the area, we just print it normally starting at x
    if (text_w >= available_width) {
        wattron(win, attr);
        mvwprintw(win, y, x, "%s", text);
        wattroff(win, attr);
        //wrefresh(win);
        return;
    }

    int padding_left = (available_width - text_w) / 2;
    int padding_right = available_width - text_w - padding_left;

    wattron(win, attr);
    // We use %*s to handle the left padding and then the right padding
    mvwprintw(win, y, x, "%*s%s%*s", padding_left, "", text, padding_right, "");
    wattroff(win, attr);
    //wrefresh(win);
    
}



int find_first_item_with_letter_offset(const char **items, int count, int start_index, char letter, int offset) {
    // O texto real do tópico começa após "Index XX: " (9 caracteres do padrão + 1 do espaço opcional)

    // Busca a partir do start_index em diante
    for (int i = start_index; i < count; i++) {
        if (items[i] && (int)strlen(items[i]) > offset) {
            // Se houver um espaço após o ':', avançamos para o caractere 10
            int inicio_real = (items[i][offset] == ' ') ? offset + 1 : offset;
            
            if (tolower(items[i][inicio_real]) == tolower(letter)) {
                return i;
            }
        }
    }
    
    // Se não encontrou até o fim, busca a partir do início do array
    for (int i = 0; i < start_index; i++) {
        if (items[i] && (int)strlen(items[i]) > offset) {
            // Mesma verificação de espaço para garantir precisão
            int inicio_real = (items[i][offset] == ' ') ? offset + 1 : offset;
            
            if (tolower(items[i][inicio_real]) == tolower(letter)) {
                return i;
            }
        }
    }
    
    return start_index; // Retorna o índice atual se nenhuma correspondência for encontrada
}


int find_first_item_with_letter(const char **items, int count, int start_index, char letter) {
    // Search from start_index onwards
    for (int i = start_index; i < count; i++) {
        if (items[i] && items[i][0] && 
            (tolower(items[i][0]) == tolower(letter))) {
            return i;
        }
    }
    // If not found from start_index, search from beginning
    for (int i = 0; i < start_index; i++) {
        if (items[i] && items[i][0] && 
            (tolower(items[i][0]) == tolower(letter))) {
            return i;
        }
    }
    return start_index; // Return current index if no match found
}


int calculate_max_display_items(int menu_height) {
    // Subtract space for border, title, and any padding
    return menu_height - 4;  // 4 = 1 border + 1 title + 1 padding + 1 padding
}






void draw_scrolled_menu(WINDOW *win, const char **items, int count, int selected, int start_index, int max_items) {
    // Clear the window content
    werase(win);
    
    // Draw border
    wattron(win, COLOR_PAIR(2) | A_DIM);
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(2) | A_DIM);
    
    // Draw title
    mvwprintw(win, 0, 2, "Select Item");
    
    // Draw items with scrolling
    int display_count = (count < max_items) ? count : max_items;
    for (int i = 0; i < display_count; i++) {
        int item_index = i + start_index;
        if (item_index < count) {
            int attr = (i == selected) ? (COLOR_PAIR(3) | A_REVERSE | A_BOLD) : COLOR_PAIR(2);
            wattron(win, attr);
            mvwprintw(win, i + 2, 2, "%s", items[item_index]);
            wattroff(win, attr);
        }
    }
    
    wrefresh(win);
}

char* load_file_content(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory for file content
    char *content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    // Read file content
    size_t bytes_read = fread(content, 1, file_size, file);
    content[bytes_read] = '\0';
    
    fclose(file);
    return content;
}

// Add this helper function to split content into lines
char** split_lines(char* content, int* line_count) {
    if (!content) {
        *line_count = 0;
        return NULL;
    }
    
    // Count newlines to estimate number of lines
    int count = 1; // At least one line
    for (int i = 0; content[i]; i++) {
        if (content[i] == '\n') {
            count++;
        }
    }
    
    char** lines = malloc((count + 1) * sizeof(char*)); // +1 for NULL terminator
    if (!lines) {
        *line_count = 0;
        return NULL;
    }
    
    char* line_start = content;
    int line_index = 0;
    
    for (int i = 0; content[i]; i++) {
        if (content[i] == '\n') {
            content[i] = '\0'; // Null terminate current line
            lines[line_index] = strdup(line_start);
            line_index++;
            line_start = content + i + 1; // Next line starts after newline
        }
    }
    
    // Handle last line (no trailing newline)
    if (line_start[0] != '\0') {
        lines[line_index] = strdup(line_start);
        line_index++;
    }
    
    lines[line_index] = NULL; // NULL terminator
    *line_count = line_index;
    
    return lines;
}


char** split_lines_wrap(char* content, int* line_count, int max_width) {
    if (!content) {
        *line_count = 0;
        return NULL;
    }

    int contador = 0;
    char **lines = NULL;
    
    char *ptr = content;
    char buffer[4096]; // Buffer generoso para capturar cada linha original do arquivo

    while (*ptr) {
        // Isola uma linha física do arquivo original (até achar '\n' ou '\0')
        int i = 0;
        while (*ptr && *ptr != '\n' && i < (int)sizeof(buffer) - 1) {
            buffer[i++] = *ptr++;
        }
        buffer[i] = '\0';
        
        if (*ptr == '\n') ptr++; // Pula o caractere de nova linha original

        // --- PROTEÇÃO DO ASCII ART ---
        // Se a linha original já cabe perfeitamente na janela (menor ou igual a max_width),
        // nós salvamos ela INTEGRALMENTE, sem rodar nenhuma lógica de espaço ou quebra.
        if ((int)get_visual_width(buffer) <= max_width) {
            char **temp = realloc(lines, (contador + 1) * sizeof(char*));
            if (!temp) break;
            lines = temp;
            lines[contador++] = strdup(buffer);
            continue; // Avança direto para a próxima linha do arquivo
        }

        // --- CÓDIGO DE WRAPPING (RODA APENAS PARA PARÁGRAFOS EXCEPCIONALMENTE LONGOS) ---
        char *texto_restante = buffer;
        while (get_visual_width(texto_restante) > 0) {
            if (get_visual_width(texto_restante) < max_width) {
                char **temp = realloc(lines, (contador + 1) * sizeof(char*));
                if (!temp) break;
                lines = temp;
                lines[contador++] = strdup(texto_restante);
                break;
            }

            // Procura o último espaço em branco antes do limite da janela
            int ponto_quebra = max_width - 1;
            while (ponto_quebra > 0 && texto_restante[ponto_quebra] != ' ') {
                ponto_quebra--;
            }

            // Se for uma palavra gigante sem espaços, força a quebra no limite máximo
            if (ponto_quebra == 0) {
                ponto_quebra = max_width - 1;
            }

            char **temp = realloc(lines, (contador + 1) * sizeof(char*));
            if (!temp) break;
            lines = temp;

            lines[contador] = malloc(ponto_quebra + 1);
            if (lines[contador]) {
                strncpy(lines[contador], texto_restante, ponto_quebra);
                lines[contador][ponto_quebra] = '\0';
                contador++;
            }

            texto_restante += ponto_quebra;
            if (*texto_restante == ' ') {
                texto_restante++;
            }
        }
    }

    // Adiciona o terminador nulo para o seu laço de cleanup
    char **temp = realloc(lines, (contador + 1) * sizeof(char*));
    if (temp) {
        lines = temp;
        lines[contador] = NULL;
    }
    
    *line_count = contador;
    return lines;
}


int print_split_lines(WINDOW *win, const char *text, int max_width) {
    char **lines;
    int num_lines = 0;

    lines = split_lines_wrap((char *)text, &num_lines, max_width);

    for (int i = 0; i < num_lines; i++) {
        wprintw(win, lines[i]);
        wprintw(win, "\n");
    }
    wprintw(win, "\n");

    return num_lines + 1;

}


/**
 * Imprime uma string no ncurses quebrando-a em quantas linhas forem necessárias.
 * A quebra respeita caracteres UTF-8 e procura sempre o último espaço disponível dentro do limite.
 *
 * @param win               A janela (WINDOW*) do ncurses.
 * @param linha_inicial     A linha (Y) onde o texto começará a ser impresso.
 * @param coluna_inicial    A coluna (X) onde todas as linhas geradas se alinharão.
 * @param max_colunas_linha O limite visual de colunas para cada linha.
 * @param texto             A string original em formato UTF-8 (char*).
 * @return int              O número total de linhas que foram utilizadas para imprimir o texto.
 */
int print_text_multiline(WINDOW *win, int linha_inicial, int coluna_inicial, int max_colunas_linha, const char *texto) {
    if (texto == NULL || win == NULL) return 0;

    // Se o texto inteiro couber em uma única linha, imprime direto e retorna 1 linha
    if (get_visual_width(texto) <= max_colunas_linha) {
        mvwprintw(win, linha_inicial, coluna_inicial, "%s", texto);
        return 1;
    }

    // Converte a string UTF-8 para caracteres largos (wchar_t)
    size_t len_chars = mbstowcs(NULL, texto, 0);
    if (len_chars == (size_t)-1) {
        // Fallback de segurança se o locale falhar: imprime tudo na linha inicial
        mvwprintw(win, linha_inicial, coluna_inicial, "%s", texto);
        return 1;
    }

    wchar_t *w_texto = malloc((len_chars + 1) * sizeof(wchar_t));
    mbstowcs(w_texto, texto, len_chars + 1);

    size_t indice_atual = 0;
    int linhas_impressas = 0;

    // Laço principal que consome a string até o fim
    while (indice_atual < len_chars) {
        // Ignora espaços em branco no início de uma nova linha (evita desalinhamentos)
        while (indice_atual < len_chars && w_texto[indice_atual] == L' ') {
            indice_atual++;
        }

        // Se a string acabou após remover os espaços, encerra o laço
        if (indice_atual >= len_chars) {
            break;
        }

        int colunas_acumuladas = 0;
        size_t ultimo_espaco = 0;
        size_t i = indice_atual;

        // Mede o tamanho dos próximos caracteres para descobrir onde quebrar a linha atual
        while (i < len_chars && colunas_acumuladas < max_colunas_linha) {
            int largura_char = wcwidth(w_texto[i]);
            if (largura_char < 0) largura_char = 0; // Ignora caracteres de controle

            // Se o caractere estourar o limite de colunas da linha atual, interrompe a busca
            if (colunas_acumuladas + largura_char > max_colunas_linha) {
                break;
            }

            colunas_acumuladas += largura_char;

            // Registra a posição do último espaço encontrado nesta linha
            if (w_texto[i] == L' ') {
                ultimo_espaco = i;
            }
            i++;
        }

        size_t indice_quebra = i;

        // Se não chegamos ao fim da string inteira, precisamos aplicar a lógica do espaço
        if (i < len_chars) {
            // Se encontramos um espaço dentro do limite da linha, quebramos nele
            if (ultimo_espaco > indice_atual) {
                indice_quebra = ultimo_espaco;
            } 
            // Caso crítico: uma palavra gigante sem nenhum espaço que supera 'max_colunas_linha'.
            // Forçamos a quebra no limite exato de colunas para não entrar em loop infinito.
            else if (indice_quebra == indice_atual) {
                indice_quebra = indice_atual + 1;
            }
        }

        // Isola temporariamente a fatia de texto correspondente à linha atual
        wchar_t caractere_salvo = w_texto[indice_quebra];
        w_texto[indice_quebra] = L'\0';

        // Move o cursor e imprime a linha atualizada na tela
        wmove(win, linha_inicial + linhas_impressas, coluna_inicial);
        waddwstr(win, &w_texto[indice_atual]);
        linhas_impressas++;

        // Restaura o caractere e avança o ponteiro de leitura para a próxima linha
        w_texto[indice_quebra] = caractere_salvo;
        indice_atual = indice_quebra;
    }

    free(w_texto);
    return linhas_impressas;
}




/**
 * Imprime uma string no ncurses no fluxo natural do cursor, quebrando-a
 * inteligentemente com base no limite de colunas e expandindo o pad se necessário.
 */
int imprimir_texto_fluxo(WINDOW *win, int coluna_inicial, int max_colunas_linha, const char *texto) {
    if (texto == NULL || win == NULL) return 0;

    int max_y_janela = getmaxy(win);
    int max_x_janela = getmaxx(win);
    int y_cursor = getcury(win);

    // EXPANSÃO DINÂMICA: Se o cursor estiver perto do fim do pad (últimas 2 linhas ou além),
    // expande o pad adicionando mais 50 linhas dinamicamente para o texto caber.
    if (y_cursor >= max_y_janela - 2) {
        max_y_janela += 50; 
        wresize(win, max_y_janela, max_x_janela);
    }

    size_t tamanho_bytes = strlen(texto);
    if (tamanho_bytes == 0) return 0;

    size_t len_chars = mbstowcs(NULL, texto, tamanho_bytes);
    if (len_chars == (size_t)-1) {
        // Fallback de segurança contra UTF-8 truncado
        len_chars = mbstowcs(NULL, texto, tamanho_bytes - 1);
        if (len_chars == (size_t)-1) {
            waddstr(win, texto);
            return 1;
        }
    }

    wchar_t *w_texto = malloc((len_chars + 1) * sizeof(wchar_t));
    if (w_texto == NULL) return 0;
    
    mbstowcs(w_texto, texto, len_chars);
    w_texto[len_chars] = L'\0';

    size_t indice_atual = 0;
    int linhas_impressas = 0;

    while (indice_atual < len_chars) {
        // Garante que o pad se expanda mesmo se o texto crescer muito dentro deste laço
        if (getcury(win) >= max_y_janela - 2) {
            max_y_janela += 50;
            wresize(win, max_y_janela, max_x_janela);
        }

        if (linhas_impressas > 0 && w_texto[indice_atual] == L' ') {
            if (indice_atual > 0 && w_texto[indice_atual - 1] != L'\n') {
                while (indice_atual < len_chars && w_texto[indice_atual] == L' ') {
                    indice_atual++;
                }
            }
        }

        if (indice_atual >= len_chars) break;

        int x_atual = getcurx(win);
        int limite_desta_linha = max_colunas_linha - x_atual;

        if (limite_desta_linha <= 5) { 
            waddch(win, '\n');
            wmove(win, getcury(win), coluna_inicial);
            x_atual = coluna_inicial;
            limite_desta_linha = max_colunas_linha - x_atual;
            linhas_impressas++;
        }

        int colunas_acumuladas = 0;
        size_t ultimo_espaco = 0;
        size_t i = indice_atual;
        int encontrou_newline_manual = 0;

        while (i < len_chars && colunas_acumuladas < limite_desta_linha) {
            if (w_texto[i] == L'\n') {
                encontrou_newline_manual = 1;
                i++; 
                break;
            }

            int largura_char = wcwidth(w_texto[i]);
            if (largura_char < 0) largura_char = 0;

            if (colunas_acumuladas + largura_char > limite_desta_linha) {
                break;
            }

            colunas_acumuladas += largura_char;

            if (w_texto[i] == L' ') {
                ultimo_espaco = i;
            }
            i++;
        }

        size_t indice_quebra = i;

        if (!encontrou_newline_manual && i < len_chars) {
            if (ultimo_espaco > indice_atual) {
                indice_quebra = ultimo_espaco;
            } 
            else if (indice_quebra == indice_atual) {
                indice_quebra = indice_atual + 1;
            }
        }

        wchar_t caractere_salvo = w_texto[indice_quebra];
        w_texto[indice_quebra] = L'\0';

        waddwstr(win, &w_texto[indice_atual]);

        if (!encontrou_newline_manual && indice_quebra < len_chars) {
            waddch(win, '\n');
            wmove(win, getcury(win), coluna_inicial);
        }
        
        linhas_impressas++;

        w_texto[indice_quebra] = caractere_salvo;
        indice_atual = indice_quebra;
    }

    free(w_texto);
    return linhas_impressas;
}
