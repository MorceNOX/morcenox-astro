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

#define _XOPEN_SOURCE_EXTENDED 1
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ncursesw/curses.h>
#include <ctype.h>
#include <wchar.h>
#include <locale.h>

#include "var.h"
#include "arabic_parts.h"
#include "helper.h"
#include "draw-chart.h"
#include "planet_table.h"
#include "db-utils.h"


// Garante compatibilidade de tipos se o cabeçalho wchar não definir wint_t
#ifndef WEOF
typedef unsigned int wint_t;
#endif



// Função utilitária para rastrear a longitude de um objeto pelo ID (1 a 16)
double get_longitude_by_id(int id, int num_objects, ChartObject *obj) {
  
    if (id >= 1 && id <= 7) {
        return obj[id - 1].longitude;
    }
    
    for (int i = 0; i < num_objects; i++) {        
        if (obj[i].id == id) {
            return obj[i].longitude;
        }
    }
    return 0.0;
}

int load_and_calculate_arabic_parts(ChartObject *obj, int num_objects, double *cusps, ArabicPartCalculada *lista_resultado) {
    int qtd_partes = 0;
    sqlite3_stmt *stmt;
    
    // Query que filtra apenas as partes Universais (3) e as do Gênero atual do mapa (1 ou 2)
    // char query[256];
    // snprintf(query, sizeof(query), 
    // "SELECT name, diurnal_personal_point, diurnal_significator, diurnal_trigger, "
    // "nocturnal_personal_point, nocturnal_significator, nocturnal_trigger, description, link "
    // "FROM arabic_parts WHERE gender_id = 3 OR gender_id = %d;", GENDER);
    const char *query = 
        "SELECT "
            "ap.name, ap.description, ap.link, ap.link2, "
            "ap.diurnal_personal_point, ap.diurnal_significator, ap.diurnal_trigger, "
            "ap.nocturnal_personal_point, ap.nocturnal_significator, ap.nocturnal_trigger, "
            "obj_d_pp.symbol, obj_d_sig.symbol, obj_d_trig.symbol, "
            "obj_n_pp.symbol, obj_n_sig.symbol, obj_n_trig.symbol "
        "FROM arabic_parts AS ap "
        "LEFT JOIN objects AS obj_d_pp   ON ap.diurnal_personal_point = obj_d_pp.id "
        "LEFT JOIN objects AS obj_d_sig  ON ap.diurnal_significator = obj_d_sig.id "
        "LEFT JOIN objects AS obj_d_trig ON ap.diurnal_trigger = obj_d_trig.id "
        "LEFT JOIN objects AS obj_n_pp   ON ap.nocturnal_personal_point = obj_n_pp.id "
        "LEFT JOIN objects AS obj_n_sig  ON ap.nocturnal_significator = obj_n_sig.id "
        "LEFT JOIN objects AS obj_n_trig ON ap.nocturnal_trigger = obj_n_trig.id "
        "WHERE ap.gender_id = ?1 OR ap.gender_id = ?2;";

    int rc = sqlite3_prepare_v2(global_db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return 0; // Falha ao preparar consulta
    }

    sqlite3_bind_int(stmt, 1, 3);    // Associa o valor fixo 3 ao ?1
    sqlite3_bind_int(stmt, 2, GENDER);

    // Varre as linhas retornadas pelo SQLite
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ArabicPartCalculada *p = &lista_resultado[qtd_partes];
        
        // Coleta o nome e a descrição textuais
        strcpy(p->name, (const char *)sqlite3_column_text(stmt, 0));
        strcpy(p->description, (const char *)sqlite3_column_text(stmt, 1));
        strcpy(p->link, (const char *)sqlite3_column_text(stmt, 2));
        strcpy(p->link2, (const char *)sqlite3_column_text(stmt, 3));

        // Seleção dinâmica dos fatores (Personal, Significator, Trigger) baseada na SEITA global
        int id_personal, id_sig, id_trigger;
        //char pp[20], sig[20], trig[20];

        if (MAPA_DIURNO) {
            id_personal = sqlite3_column_int(stmt, 4);
            id_sig      = sqlite3_column_int(stmt, 5);
            id_trigger  = sqlite3_column_int(stmt, 6);
            strcpy(p->pp, (const char *)sqlite3_column_text(stmt, 10));
            strcpy(p->sig, (const char *)sqlite3_column_text(stmt, 11));
            strcpy(p->trig, (const char *)sqlite3_column_text(stmt, 12));
        } else {
            id_personal = sqlite3_column_int(stmt, 7);
            id_sig      = sqlite3_column_int(stmt, 8);
            id_trigger  = sqlite3_column_int(stmt, 9);
            strcpy(p->pp, (const char *)sqlite3_column_text(stmt, 13));
            strcpy(p->sig, (const char *)sqlite3_column_text(stmt, 14));
            strcpy(p->trig, (const char *)sqlite3_column_text(stmt, 15));
        }

        // Busca as longitudes físicas no seu array plots usando a função utilitária que criamos
        double lon_personal = get_longitude_by_id(id_personal, num_objects, obj);
        double lon_sig      = get_longitude_by_id(id_sig, num_objects, obj);
        double lon_trigger  = get_longitude_by_id(id_trigger, num_objects, obj);

        // Aritmética hermética: Parte = Personal + Significator - Trigger
        double total_lon = lon_personal + lon_sig - lon_trigger;

        // Normalização matemática do círculo zodiacal (0.0° a 360.0°)
        if (total_lon < 0.0) total_lon += 360.0;
        if (total_lon >= 360.0) total_lon = fmod(total_lon, 360.0);

        p->longitude = total_lon;

        // Calcula dinamicamente a casa da parte usando a rotina interna de domificação
        // Passamos as cúspides e a longitude para resgatar a string romana (ex: "IX")
        strcpy(p->house, (char *)get_house_roman(p->longitude, cusps));

        int lord = 0;
        int lord2 = 0;
        get_ruler_dom_by_sign_id((int)floor(p->longitude / 30.0) + 1, &lord);
        get_ruler_exalt_by_sign_id((int)floor(p->longitude / 30.0) + 1, &lord2, consider_modern_planets_rulling);
        const char *lord_str = obter_glifo_planeta_por_id(lord);
        const char *lord2_str = obter_glifo_planeta_por_id(lord2);

        if (lord2 == 0) {
            snprintf(p->lord, 8, "%s", lord_str);
        }
        else {
            snprintf(p->lord, 8, "%s %s", lord_str, lord2_str);
        }

        

        qtd_partes++;
        if (qtd_partes >= MAX_PARTS) break; // Trava de segurança do buffer
    }

    sqlite3_finalize(stmt);
    return qtd_partes; // Retorna o total de partes processadas de forma legítima
}

void display_arabic_parts(ChartObject *obj, double *cusps, int num_objects) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 28;
    int table_width = max_x - 10;
    int start_y = (max_y - table_height) / 2;
    int start_x = 5;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    keypad(table_win, TRUE);

    // Aloca o buffer para receber os dados calculados do banco
    ArabicPartCalculada lista[MAX_PARTS];
    memset(lista, 0, sizeof(lista));

    int qtd_partes = load_and_calculate_arabic_parts(obj, num_objects, cusps, lista);

    int seletor_linha_atual = 0; // NOVO: Controla qual índice de registro (0, 1, 2...) está selecionado
    int scroll_offset = 0;
    int max_linhas_exibicao = table_height - 10;
    
    WINDOW *scroll_pad = newpad(qtd_partes * 6 + 100, table_width - 8);
    wbkgd(scroll_pad, COLOR_PAIR(13) | FLAGS);

    wattron(shadow_win, COLOR_PAIR(9)); 
    box(shadow_win, 0, 0); 
    wattroff(shadow_win, COLOR_PAIR(9));
    wbkgd(shadow_win, COLOR_PAIR(13));
    wnoutrefresh(shadow_win);

    wbkgd(table_win, COLOR_PAIR(13) | FLAGS);

    int loop_interativo = 1;
    while (loop_interativo) {
        werase(table_win);
        werase(scroll_pad);        
        
        box(table_win, 0, 0); 
        
        wattron(table_win, A_BOLD);
        const char *title = _(" Arabic Parts & Hermetic Lots ");
        mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
        wattroff(table_win, A_BOLD);
        
        // Cabeçalho de Status Fixo
        mvwprintw(table_win, 2, 4, _("Current Chart Parameters: "));
        wattron(table_win, A_BOLD | COLOR_PAIR(15));
        wprintw(table_win, "%s", MAPA_DIURNO ? _("Diurnal Formulae") : _("Nocturnal Formulae"));
        wprintw(table_win, " | %s: %s", _("Gender"), (GENDER == 1) ? _("Masculine") : (GENDER == 2) ? _("Feminine") : _("Neuter"));
        wattroff(table_win, A_BOLD | COLOR_PAIR(15));

        //wattron(table_win, COLOR_PAIR(10) | A_DIM);
        cchar_t traco_horizontal;
        setcchar(&traco_horizontal, L"─", A_NORMAL, 0, NULL);
        mvwhline_set(table_win, 4, 2, &traco_horizontal, table_width - 5);

        int col_name = 1, col_pos = 33, col_house = 42, col_ruler = 49, col_link = 58, col_desc = 69;
        wattron(table_win, A_BOLD | COLOR_PAIR(13));
        mvwprintw(table_win, 5, col_name + 4, _("Part Name"));
        mvwprintw(table_win, 5, col_pos + 4, _("Position"));
        mvwprintw(table_win, 5, col_house + 4, _("House"));
        mvwprintw(table_win, 5, col_ruler + 4, _("Lords"));
        mvwprintw(table_win, 5, col_link + 4, _("Link"));
        mvwprintw(table_win, 5, col_desc + 4, _("Core Traditional Governance"));
        wattroff(table_win, A_BOLD | COLOR_PAIR(13));

        mvwhline_set(table_win, 6, 2, &traco_horizontal, table_width - 5);

        //wnoutrefresh(table_win);



        // --- PREENCHIMENTO DO PAD VIRTUAL ---
        int row_pad = 0;
        int max_desc_width = table_width - 80;
        
        // Array para guardar em qual linha física do PAD cada registro começou
        // Essencial para o cálculo matemático do scroll automático acompanhar o seletor
        int linha_inicio_registro[MAX_PARTS] = {0};
        
        int part_lines;

        if (qtd_partes == 0) {
            wattron(scroll_pad, A_DIM);
            mvwprintw(scroll_pad, row_pad, col_name, _("No Arabic Parts available for this chart configuration."));
            wattroff(scroll_pad, A_DIM);
        } else {
            for (int i = 0; i < qtd_partes; i++) {
                ArabicPartCalculada *p = &lista[i];
                linha_inicio_registro[i] = row_pad; // Salva o marco zero visual do lote i

                double total_graus = p->longitude;
                int sign_id = (int)floor(total_graus / 30.0);
                double de_graus_signo = fmod(total_graus, 30.0);
                int graus_inteiros = (int)floor(de_graus_signo);
                int minutos_inteiros = (int)round((de_graus_signo - graus_inteiros) * 60.0);
                if (minutos_inteiros == 60) { minutos_inteiros = 0; graus_inteiros++; }

                char coord_texto[32];
                snprintf(coord_texto, sizeof(coord_texto), "%2d°%s%02d'", graus_inteiros, get_sign(sign_id), minutos_inteiros);

                // NOVO: Se este registro for o selecionado atual, aplica o REVERSE na linha inteira!
                bool is_linha_focada = (i == seletor_linha_atual);
                bool is_major_lot = (strcmp(p->name, "Part of Fortune") == 0 || 
                                     strcmp(p->name, "Part of Spirit") == 0 || 
                                     strcmp(p->name, "Parte do Espírito") == 0 || 
                                     strcmp(p->name, "Parte da Fortuna") == 0 ||
                                     strcmp(p->name, "Lot of Fortune") == 0 || 
                                     strcmp(p->name, "Lot of Spirit") == 0 || 
                                     strcmp(p->name, "Lot do Espírito") == 0 || 
                                     strcmp(p->name, "Lot da Fortuna") == 0);

                if (is_linha_focada) {
                    wattron(table_win, A_BOLD);
                    mvwprintw(table_win, 2, col_desc + 1, _("|  Formula: "));
                    wattron(table_win, A_BOLD | COLOR_PAIR(37) | A_BOLD | A_REVERSE);
                    wprintw(table_win, "[ %s + %s - %s ]", p->pp, p->sig, p->trig);
                    wattroff(table_win, A_BOLD | COLOR_PAIR(37) | A_BOLD | A_REVERSE);
                }

                
                // Determina o tom da linha
                if (is_linha_focada) {
                    wattron(scroll_pad, COLOR_PAIR(37) | A_BOLD | A_REVERSE);
                    // Limpa e ilumina o fundo da primeira linha do registro
                    for(int x=0; x < table_width-8; x++) mvwprintw(scroll_pad, row_pad, x, " ");
                } else if (is_major_lot) {
                    wattron(scroll_pad, COLOR_PAIR(13) | A_BOLD); 
                } else {
                    wattron(scroll_pad, COLOR_PAIR(13));
                }

                

                mvwprintw(scroll_pad, row_pad, col_name, "%-26s", p->name);
                mvwprintw(scroll_pad, row_pad, col_pos, "%-16s", coord_texto);
                mvwprintw(scroll_pad, row_pad, col_house, " %-3s", p->house);
                
                wattron(scroll_pad, A_BOLD); 
                mvwprintw(scroll_pad, row_pad, col_ruler, " %s", p->lord);
                wattroff(scroll_pad, A_BOLD); 
                
                if (is_linha_focada) wattroff(scroll_pad, COLOR_PAIR(37) | A_BOLD | A_REVERSE);
                else if (is_major_lot) wattroff(scroll_pad, COLOR_PAIR(13) | A_BOLD);
                else wattroff(scroll_pad, COLOR_PAIR(13));

                if (is_linha_focada) wattron(scroll_pad, COLOR_PAIR(37) | A_BOLD | A_REVERSE);
                else wattron(scroll_pad, COLOR_PAIR(13) | A_BOLD);
                mvwprintw(scroll_pad, row_pad, col_link, "%-3s %-3s", p->link, p->link2);
                if (is_linha_focada) wattroff(scroll_pad, COLOR_PAIR(37) | A_BOLD | A_REVERSE);
                else wattroff(scroll_pad, COLOR_PAIR(13) | A_BOLD);

                // Word Wrap da descrição
                if (is_linha_focada) wattron(scroll_pad, COLOR_PAIR(37) | A_BOLD | A_REVERSE);
                else wattron(scroll_pad, A_DIM);
                
                char *texto_restante = p->description;

                part_lines = (int)ceil((double)get_visual_width(p->description) / max_desc_width);

                while (get_visual_width(texto_restante) > 0) {
                    char linha_buffer[256] = {0};

                    if (get_visual_width(texto_restante) <= max_desc_width) {
                        strcpy(linha_buffer, texto_restante);
                        mvwprintw(scroll_pad, row_pad, col_desc, "%s", linha_buffer);
                        texto_restante += get_visual_width(texto_restante);
                    } else {
                        int quebra_id = max_desc_width;
                        while (quebra_id > 0 && texto_restante[quebra_id] != ' ' && texto_restante[quebra_id] != '\0') {
                            quebra_id--;
                        }
                        if (quebra_id == 0) quebra_id = max_desc_width;

                        strncpy(linha_buffer, texto_restante, quebra_id);
                        linha_buffer[quebra_id] = '\0';

                        mvwprintw(scroll_pad, row_pad, col_desc, "%s", linha_buffer);
                        texto_restante += quebra_id;
                        if (*texto_restante == ' ') texto_restante++; 
                    }

                    if (get_visual_width(texto_restante) > 0) {
                        row_pad++;
                        if (is_linha_focada) {
                            for(int x=0; x < table_width-8; x++) mvwprintw(scroll_pad, row_pad, x, " ");
                        }
                    }
                }
                if (is_linha_focada) wattroff(scroll_pad, COLOR_PAIR(37) | A_BOLD | A_REVERSE);
                else wattroff(scroll_pad, A_DIM);

                row_pad += 2; 
            }
        }

        int total_linhas_virtuais_pad = row_pad;

        // NOVO: Algoritmo de Inteligência de Scroll Automático (O visor segue a barra de seleção)
        if (qtd_partes > 0) {
            int linha_foco_fisica = linha_inicio_registro[seletor_linha_atual];
            // Se a barra subiu além do topo visível, empurra o scroll para cima
            if (linha_foco_fisica < scroll_offset) {
                scroll_offset = linha_foco_fisica;
            }
            // Se a barra desceu além do fundo visível, empurra o scroll para baixo
            if (linha_foco_fisica >= (scroll_offset + max_linhas_exibicao - part_lines)) {
                scroll_offset = linha_foco_fisica - max_linhas_exibicao + part_lines + 2;
            }
        }

        if (scroll_offset > total_linhas_virtuais_pad - max_linhas_exibicao) scroll_offset = total_linhas_virtuais_pad - max_linhas_exibicao;
        if (scroll_offset < 0) scroll_offset = 0;

        // Rodapé Fixo
        mvwhline_set(table_win, table_height - 4, 2, &traco_horizontal, table_width - 5);

        wattron(table_win, A_DIM);
        mvwprintw(table_win, table_height - 3, 4, _("Use [↑/↓] Select | [e] Edit | [i] Include | [d] Delete | [x] Aspects."));
        wattroff(table_win, A_DIM);

        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
        
        wnoutrefresh(table_win);

        doupdate();

        prefresh(scroll_pad, scroll_offset, 0, start_y + 7, start_x + 4, start_y + 7 + max_linhas_exibicao - 2, start_x + table_width - 5);

        int ch = wgetch(table_win);
        switch (ch) {
            case KEY_DOWN: // Seta para Baixo: move a barra de seleção para o próximo lote
                if (seletor_linha_atual < qtd_partes - 1) {
                    seletor_linha_atual++;
                }
                break;
            case KEY_UP: // Seta para Cima: move a barra de seleção para o lote anterior
                if (seletor_linha_atual > 0) {
                    seletor_linha_atual--;
                }
                break;
            case 'i': case 'a': case 'I': case 'A': // Inclusão
                form_arabic_part(obj, num_objects, 0); 
                qtd_partes = load_and_calculate_arabic_parts(obj, num_objects, cusps, lista);
                break;
            case 'e': // Edição do lote selecionado!
            case 'E':
                if (qtd_partes > 0) {
                    // Captura dinamicamente o nome exato do lote que está com a barra iluminada!
                    int id_banco_alvo = obter_id_parte_por_nome(lista[seletor_linha_atual].name);
                    form_arabic_part(obj, num_objects, id_banco_alvo);
                    // Recarrega os dados atualizados pós-salvamento
                    qtd_partes = load_and_calculate_arabic_parts(obj, num_objects, cusps, lista);
                }
                break;
            case 'x': 
            case 'X': 
            case KEY_F(3):
                if (qtd_partes > 0) {
                    // Passamos os dados que JÁ ESTÃO mastigados na memória RAM!
                    display_part_aspects(obj, num_objects, lista, qtd_partes);
                    
                    // Ao fechar a matriz, força a janela anterior a redesenhar a borda
                    touchwin(shadow_win);
                    wnoutrefresh(shadow_win);
                    touchwin(table_win);
                    wnoutrefresh(table_win);

                    doupdate();
                }
                break;
            case 'd':
            case 'D': 
                if (qtd_partes > 0) {
                    int id_banco_alvo = obter_id_parte_por_nome(lista[seletor_linha_atual].name);
                    
                    // Abre o pop-up vermelho de confirmação passados os dados da RAM
                    deletar_parte_arabe_com_confirmacao(id_banco_alvo, lista[seletor_linha_atual].name);
                    
                    // Recarrega o buffer com os dados atualizados do SQLite
                    qtd_partes = load_and_calculate_arabic_parts(obj, num_objects, cusps, lista);
                    
                    // PROTEÇÃO DE MEMÓRIA: Se deletamos a última linha, recua o seletor para não travar
                    if (seletor_linha_atual >= qtd_partes && seletor_linha_atual > 0) {
                        seletor_linha_atual = qtd_partes - 1;
                    }
                    
                    // Ao fechar a matriz, força a janela anterior a redesenhar a borda
                    touchwin(shadow_win);
                    wnoutrefresh(shadow_win);
                    touchwin(table_win);
                    wnoutrefresh(table_win);

                    doupdate();
                }
                break;
            case 27:
            case 'q':
            case 'Q':
                loop_interativo = 0;
                break;
        }
    }
 // Fim do grande while (loop_interativo)

    delwin(scroll_pad); 
    delwin(shadow_win); 
    delwin(table_win); 
    touchwin(stdscr); 
    refresh();
}



void display_arabic_parts_solar_natal_confrontation(ChartObject *obj, double *cusps, int num_objects, double *cusps_natal, ChartObject *obj_natal) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 28;
    int table_width = max_x - 10;
    int start_y = (max_y - table_height) / 2;
    int start_x = 5;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    keypad(table_win, TRUE);

    // Aloca o buffer para receber os dados calculados do banco
    ArabicPartCalculada lista[MAX_PARTS];
    memset(lista, 0, sizeof(lista));

    int qtd_partes = load_and_calculate_arabic_parts(obj, num_objects, cusps, lista);

    ArabicPartCalculada parts_natal_house[MAX_PARTS];
    memset(parts_natal_house, 0, sizeof(parts_natal_house));

    qtd_partes = load_and_calculate_arabic_parts(obj, num_objects, cusps_natal, parts_natal_house);

    int seletor_linha_atual = 0; // NOVO: Controla qual índice de registro (0, 1, 2...) está selecionado
    int scroll_offset = 0;
    int max_linhas_exibicao = table_height - 10;
    
    WINDOW *scroll_pad = newpad(qtd_partes * 6 + 100, table_width - 8);
    wbkgd(scroll_pad, COLOR_PAIR(13) | FLAGS);

    wattron(shadow_win, COLOR_PAIR(9)); 
    box(shadow_win, 0, 0); 
    wattroff(shadow_win, COLOR_PAIR(9));
    wbkgd(shadow_win, COLOR_PAIR(13));
    wnoutrefresh(shadow_win);

    wbkgd(table_win, COLOR_PAIR(13) | FLAGS);

    int loop_interativo = 1;
    while (loop_interativo) {
        werase(table_win);
        werase(scroll_pad);        
        
        box(table_win, 0, 0); 
        //wbkgd(table_win, COLOR_PAIR(13) | FLAGS);
        wattron(table_win, A_BOLD);
        const char *title = _(" Arabic Parts - Solar Revolution Radix Confrontation ");
        mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
        wattroff(table_win, A_BOLD);
        
        // Cabeçalho de Status Fixo
        mvwprintw(table_win, 2, 4, _("Current Chart Parameters: "));
        wattron(table_win, A_BOLD | COLOR_PAIR(15));
        wprintw(table_win, "%s", MAPA_DIURNO ? _("Diurnal Formulae") : _("Nocturnal Formulae"));
        wprintw(table_win, " | %s: %s", _("Gender"), (GENDER == 1) ? _("Masculine") : (GENDER == 2) ? _("Feminine") : _("Neuter"));
        wattroff(table_win, A_BOLD | COLOR_PAIR(15));

        //wattron(table_win, COLOR_PAIR(10) | A_DIM);
        cchar_t traco_horizontal;
        setcchar(&traco_horizontal, L"─", A_NORMAL, 0, NULL);
        mvwhline_set(table_win, 4, 2, &traco_horizontal, table_width - 5);

        int col_name = 1, col_pos = 33, col_house = 42, col_ruler = 49, col_link = 58, col_natal = 69; //, col_natal_ruler = 82;
        wattron(table_win, A_BOLD | COLOR_PAIR(13));
        mvwprintw(table_win, 5, col_name + 4, _("Part Name"));
        mvwprintw(table_win, 5, col_pos + 4, _("Position"));
        mvwprintw(table_win, 5, col_house + 4, _("House"));
        mvwprintw(table_win, 5, col_ruler + 4, _("Lords"));
        mvwprintw(table_win, 5, col_link + 4, _("Link"));
        mvwprintw(table_win, 5, col_natal + 4, _("Radical House"));
        wattroff(table_win, A_BOLD | COLOR_PAIR(13));

        mvwhline_set(table_win, 6, 2, &traco_horizontal, table_width - 5);

        // --- PREENCHIMENTO DO PAD VIRTUAL ---
        int row_pad = 0;
        
        // Array para guardar em qual linha física do PAD cada registro começou
        // Essencial para o cálculo matemático do scroll automático acompanhar o seletor
        int linha_inicio_registro[MAX_PARTS] = {0};
        
        //int part_lines;

        if (qtd_partes == 0) {
            wattron(scroll_pad, A_DIM);
            mvwprintw(scroll_pad, row_pad, col_name, _("No Arabic Parts available for this chart configuration."));
            wattroff(scroll_pad, A_DIM);
        } else {
            for (int i = 0; i < qtd_partes; i++) {
                ArabicPartCalculada *p = &lista[i];
                ArabicPartCalculada *np = &parts_natal_house[i];
                linha_inicio_registro[i] = row_pad; // Salva o marco zero visual do lote i

                double total_graus = p->longitude;
                int sign_id = (int)floor(total_graus / 30.0);
                double de_graus_signo = fmod(total_graus, 30.0);
                int graus_inteiros = (int)floor(de_graus_signo);
                int minutos_inteiros = (int)round((de_graus_signo - graus_inteiros) * 60.0);
                if (minutos_inteiros == 60) { minutos_inteiros = 0; graus_inteiros++; }

                char coord_texto[32];
                snprintf(coord_texto, sizeof(coord_texto), "%02d°%s%02d'", graus_inteiros, get_sign(sign_id), minutos_inteiros);

                // NOVO: Se este registro for o selecionado atual, aplica o REVERSE na linha inteira!
                bool is_linha_focada = (i == seletor_linha_atual);
                bool is_major_lot = (strcmp(p->name, "Part of Fortune") == 0 || 
                                     strcmp(p->name, "Part of Spirit") == 0 || 
                                     strcmp(p->name, "Parte do Espírito") == 0 || 
                                     strcmp(p->name, "Parte da Fortuna") == 0 ||
                                     strcmp(p->name, "Lot of Fortune") == 0 || 
                                     strcmp(p->name, "Lot of Spirit") == 0 || 
                                     strcmp(p->name, "Lot do Espírito") == 0 || 
                                     strcmp(p->name, "Lot da Fortuna") == 0);

                if (is_linha_focada) {
                    wattron(table_win, A_BOLD);
                    mvwprintw(table_win, 2, col_natal + 1, _("|  Formula: "));
                    wattron(table_win, A_BOLD | COLOR_PAIR(37) | A_BOLD | A_REVERSE);
                    wprintw(table_win, "[ %s + %s - %s ]", p->pp, p->sig, p->trig);
                    wattroff(table_win, A_BOLD | COLOR_PAIR(37) | A_BOLD | A_REVERSE);
                }
                
                // Determina o tom da linha
                if (is_linha_focada) {
                    wattron(scroll_pad, COLOR_PAIR(37) | A_BOLD | A_REVERSE);
                    // Limpa e ilumina o fundo da primeira linha do registro
                    for(int x=0; x < table_width-8; x++) mvwprintw(scroll_pad, row_pad, x, " ");
                } else if (is_major_lot) {
                    wattron(scroll_pad, COLOR_PAIR(13) | A_BOLD); 
                } else {
                    wattron(scroll_pad, COLOR_PAIR(13));
                }

                mvwprintw(scroll_pad, row_pad, col_name, "%-26s", p->name);
                mvwprintw(scroll_pad, row_pad, col_pos, "%-16s", coord_texto);
                mvwprintw(scroll_pad, row_pad, col_house, " %-3s", p->house);
                mvwprintw(scroll_pad, row_pad, col_ruler, " %s", p->lord);
                mvwprintw(scroll_pad, row_pad, col_link, "%-3s %-3s", p->link, p->link2);
                mvwprintw(scroll_pad, row_pad, col_natal, "%-3s", np->house);
                
                wattroff(scroll_pad, COLOR_PAIR(37) | COLOR_PAIR(13) | A_BOLD | A_REVERSE);

                row_pad += 2; 
            }
        }

        int total_linhas_virtuais_pad = row_pad;

        if (qtd_partes > 0) {
            int linha_foco_fisica = linha_inicio_registro[seletor_linha_atual];
            // Se a barra subiu além do topo visível, empurra o scroll para cima
            if (linha_foco_fisica < scroll_offset) {
                scroll_offset = linha_foco_fisica;
            }
            // Se a barra desceu além do fundo visível, empurra o scroll para baixo
            if (linha_foco_fisica >= (scroll_offset + max_linhas_exibicao)) {
                scroll_offset = linha_foco_fisica - max_linhas_exibicao + 2;
            }
        }

        if (scroll_offset > total_linhas_virtuais_pad - max_linhas_exibicao) scroll_offset = total_linhas_virtuais_pad - max_linhas_exibicao;
        if (scroll_offset < 0) scroll_offset = 0;

        // Rodapé Fixo
        mvwhline_set(table_win, table_height - 4, 2, &traco_horizontal, table_width - 5);

        wattron(table_win, A_DIM);
        mvwprintw(table_win, table_height - 3, 4, _("Use [↑/↓] Select | [x] Aspects with Radical Planets."));
        wattroff(table_win, A_DIM);

        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
        
        wnoutrefresh(table_win);

        doupdate();

        prefresh(scroll_pad, scroll_offset, 0, start_y + 7, start_x + 4, start_y + 7 + max_linhas_exibicao - 2, start_x + table_width - 5);

        int ch = wgetch(table_win);
        switch (ch) {
            case KEY_DOWN: // Seta para Baixo: move a barra de seleção para o próximo lote
                if (seletor_linha_atual < qtd_partes - 1) {
                    seletor_linha_atual++;
                }
                break;
            case KEY_UP: // Seta para Cima: move a barra de seleção para o lote anterior
                if (seletor_linha_atual > 0) {
                    seletor_linha_atual--;
                }
                break;
            case 'x': 
            case KEY_F(3):
                if (qtd_partes > 0) {
                    //int object_diff = show_modern_planets ? 0 : 3;
                    display_part_aspects(obj_natal, num_objects, lista, qtd_partes);
                    
                    touchwin(shadow_win);
                    wnoutrefresh(shadow_win);
                    touchwin(table_win);
                    wnoutrefresh(table_win);

                    doupdate();
                }
                break;
            case 27:
            case 'q':
            case 'Q':
                loop_interativo = 0;
                break;
        }
    }

    delwin(scroll_pad); delwin(shadow_win); delwin(table_win); touchwin(stdscr); refresh();
}





int obter_id_parte_por_nome(const char *name_lote) {
    sqlite3_stmt *stmt;
    int id_retorno = 0;
    const char *sql = "SELECT id FROM arabic_parts WHERE name = ?;";
    
    if (sqlite3_prepare_v2(global_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        
        sqlite3_bind_text(stmt, 1, name_lote, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) { 
            id_retorno = sqlite3_column_int(stmt, 0); 
        }
        sqlite3_finalize(stmt);
    }
    return id_retorno;
}




// Função utilitária que varre o array obj e retorna o nome textual baseado no ID
static const char *get_dynamic_id_label(int id, ChartObject *obj, int num_objects) {
    for (int i = 0; i < num_objects; i++) {
        if (obj[i].id == id) {
            // Mude para object_name para visualizar o nome do objeto e não o símbolo
            return obj[i].object; // Retorna o nome em inglês ex: "Sol Exaltation", "Lord2"
        }
    }
    return "Unknown";
}

// Rotaciona para o próximo ou anterior ID válido varrendo linearmente o vetor obj
static int rotate_dynamic_id(int id_atual, int direcao, ChartObject *obj, int num_objects) {
    int idx_atual = -1;
    
    // Localiza a posição do ID atual dentro do array
    for (int i = 0; i < num_objects; i++) {
        if (obj[i].id == id_atual) {
            idx_atual = i;
            break;
        }
    }
    
    // Se o ID não for encontrado por segurança, começa do início
    if (idx_atual == -1) idx_atual = 0;
    
    // Rotaciona de forma circular e modular dentro do tamanho real de objetos (num_objects)
    idx_atual = (idx_atual + direcao + num_objects) % num_objects;
    
    return obj[idx_atual].id;
}




static int campo_texto_amigavel_avancado(WINDOW *win, int y, int x, char *buffer, int max_len) {
    char backup[512];
    strcpy(backup, buffer); // Mantém o backup seguro para o caso de ESC

    // len agora rastreia o número TOTAL DE BYTES da string na memória
    int len = strlen(buffer); 
    
    // cursor_idx também deve operar baseado em BYTES para podermos navegar com precisão
    int cursor_idx = len; 
    
    // Mudança crucial: ch vira wint_t para aceitar teclas de controle e códigos wchar (acentos)
    wint_t ch; 

    // Descobre os limites físicos reais da janela atual
    int win_max_y, win_max_x;
    getmaxyx(win, win_max_y, win_max_x);
    (void)win_max_y;

    // Calcula a largura visual útil do campo para nunca estourar a borda direita
    int largura_campo_visivel = win_max_x - x - 3;
    if (largura_campo_visivel > max_len) {
        largura_campo_visivel = max_len;
    }

    // Controle de rolagem (offset) do texto (operando em bytes)
    int offset = 0;

    // Ativa o cursor piscante e garante escuta total do teclado
    curs_set(1);
    keypad(win, TRUE);

    int res;

    while (1) {
        // AJUSTE DINÂMICO DO SCROLL
        if (cursor_idx - offset >= largura_campo_visivel) {
            offset = cursor_idx - largura_campo_visivel + 1;
        }
        if (cursor_idx < offset) {
            offset = cursor_idx;
        }

        wattron(win, COLOR_PAIR(1) | A_REVERSE);
        
        // 1. Limpa a área visual do campo
        mvwprintw(win, y, x, " ");
        for (int i = 0; i < largura_campo_visivel; i++) {
            wprintw(win, " ");
        }
        wprintw(win, " ");

        // 2. Extrai e exibe apenas a fatia visível do buffer
        char buffer_visivel[512] = {0};
        if (offset < len) {
            strncpy(buffer_visivel, &buffer[offset], largura_campo_visivel);
        }
        
        mvwprintw(win, y, x + 1, "%s", buffer_visivel);
        wattroff(win, COLOR_PAIR(1) | A_REVERSE);

        // Move o cursor físico compensando o offset do scroll
        int passos_visuais = 0;
        if (cursor_idx > offset) {
            // Cria uma string temporária para a fatia entre o offset e o cursor
            int tamanho_fatia_bytes = cursor_idx - offset;
            char fatia_cursor[512] = {0};
            
            // Limita para não estourar o tamanho do buffer temporário
            if (tamanho_fatia_bytes > 511) tamanho_fatia_bytes = 511;

            strncpy(fatia_cursor, &buffer[offset], tamanho_fatia_bytes);
            fatia_cursor[tamanho_fatia_bytes] = '\0';

            passos_visuais = get_visual_width(fatia_cursor);
        }

        // Move o cursor físico somando os espaços visuais corretos
        wmove(win, y, x + 1 + passos_visuais);
        wrefresh(win);


        // Captura ch tanto como tecla especial (KEY_CODE_YES) quanto caractere largo (acentos)
        res = wget_wch(win, &ch);

        if (res != KEY_CODE_YES && ch == 10) { // ENTER
            break;
        }

        if (res != KEY_CODE_YES && ch == 27) { // ESC
            strcpy(buffer, backup);
            break;
        }

        // Se capturou uma tecla de movimentação/controle especial
        if (res == KEY_CODE_YES) {
            switch (ch) {
                case KEY_LEFT:
                    if (cursor_idx > 0) {
                        // Anda para trás tratando UTF-8 de forma segura
                        cursor_idx--;
                        while (cursor_idx > 0 && (buffer[cursor_idx] & 0xC0) == 0x80) {
                            cursor_idx--;
                        }
                    }
                    break;

                case KEY_RIGHT:
                    if (cursor_idx < len) {
                        // Avança para frente tratando UTF-8 de forma segura
                        cursor_idx++;
                        while (cursor_idx < len && (buffer[cursor_idx] & 0xC0) == 0x80) {
                            cursor_idx++;
                        }
                    }
                    break;

                case KEY_BACKSPACE:
                    if (cursor_idx > 0) {
                        // Descobre o tamanho do caractere UTF-8 anterior
                        int start = cursor_idx - 1;
                        while (start > 0 && (buffer[start] & 0xC0) == 0x80) {
                            start--;
                        }
                        int char_bytes = cursor_idx - start;

                        // Remove os bytes correspondentes da memória
                        for (int i = start; i < len - char_bytes; i++) {
                            buffer[i] = buffer[i + char_bytes];
                        }
                        len -= char_bytes;
                        cursor_idx = start;
                        buffer[len] = '\0';
                    }
                    break;

                case KEY_DC: // DELETE
                    if (cursor_idx < len) {
                        // Descobre o tamanho do caractere UTF-8 atual sob o cursor
                        int end = cursor_idx + 1;
                        while (end < len && (buffer[end] & 0xC0) == 0x80) {
                            end++;
                        }
                        int char_bytes = end - cursor_idx;

                        // Remove os bytes correspondentes
                        for (int i = cursor_idx; i < len - char_bytes; i++) {
                            buffer[i] = buffer[i + char_bytes];
                        }
                        len -= char_bytes;
                        buffer[len] = '\0';
                    }
                    break;
            }
        } 
        // Se capturou um caractere normal ou caractere acentuado
        else {
            char mb_char[MB_CUR_MAX];
            // Converte o caractere largo (wchar) para sua representação em bytes UTF-8
            int mb_len = wctomb(mb_char, (wchar_t)ch);

            // Se for um caractere imprimível válido e houver espaço no buffer
            if (mb_len > 0 && ch >= 32 && (len + mb_len) < max_len) {
                // Abre espaço no meio da string movendo os bytes subsequentes para frente
                for (int i = len + mb_len - 1; i >= cursor_idx + mb_len; i--) {
                    buffer[i] = buffer[i - mb_len];
                }
                
                // Insere todos os bytes do novo caractere (seja 1 byte para ASCII ou 2 para acento)
                for (int i = 0; i < mb_len; i++) {
                    buffer[cursor_idx + i] = mb_char[i];
                }

                len += mb_len;
                cursor_idx += mb_len;
                buffer[len] = '\0';
            }
        }
    }

    curs_set(0); // Oculta o cursor piscante ao sair
    return (res != KEY_CODE_YES && ch != 27);
}




// Vetor contendo todas as opções válidas de Links do sistema (Total: 24 opções)
static const char *OPCOES_LINK[] = {
    "-",                                                                     // 0: Nenhum
    "☉", "☽", "☿", "♀", "♂", "♃", "♄", "♅", "♆", "⯓", "☊", "☋",             // 1-12: Planetas e Nodos
    "AC", "II", "III", "IC", "V", "VI", "DC", "VIII", "IX", "MC", "XI", "XII" // 13-24: Casas Romanas
};
static const int TOTAL_OPCOES_LINK = 25;

// Função auxiliar para descobrir o índice atual da string do Link e rotacioná-la
static int rotacionar_indice_link(const char *link_atual, int direcao) {
    int idx_atual = 0; // Se não achar, assume o "-"
    
    for (int i = 0; i < TOTAL_OPCOES_LINK; i++) {
        if (strcmp(OPCOES_LINK[i], link_atual) == 0) {
            idx_atual = i;
            break;
        }
    }
    
    // Rotaciona de forma circular e modular dentro do vetor de opções
    idx_atual = (idx_atual + direcao + TOTAL_OPCOES_LINK) % TOTAL_OPCOES_LINK;
    return idx_atual;
}



void form_arabic_part(ChartObject *obj, int num_objects, int part_id_edicao) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int w_height = 24;
    int w_width = max_x - 14;

    int start_y = (max_y - w_height) / 2;
    int start_x = (max_x - w_width) / 2;
    
    WINDOW *win = newwin(w_height, w_width, start_y, start_x);
    WINDOW *shadow = newwin(w_height, w_width, start_y + 1, start_x + 1);

    keypad(win, TRUE);
    //curs_set(0);

    // Estados iniciais do formulário (Valores padrões)
    char f_name[32];
    snprintf(f_name, 32, "%s", _("New Custom Part"));
    int f_gender = 3; 
    int f_d_personal = 15, f_d_sig = 1, f_d_trigger = 2; 
    int f_n_personal = 15, f_n_sig = 2, f_n_trigger = 1; 
    char f_link[8] = "-"; // NOVO: Inicializa o link vazio
    char f_link2[8] = "-";
    char f_desc[5121];
    snprintf(f_desc, 512, "%s", _("User defined hermetic lot formula."));

    // Se for Edição, puxa os dados do SQLite incluindo a nova coluna link
    if (part_id_edicao > 0) {
        sqlite3 *db;
        sqlite3_stmt *stmt;
        int rc;

        db = open_database();
        
        const char *sql = "SELECT name, gender_id, diurnal_personal_point, diurnal_significator, diurnal_trigger, "
                          "nocturnal_personal_point, nocturnal_significator, nocturnal_trigger, description, link, link2 "
                          "FROM arabic_parts WHERE id = ?;";

        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            return;
        }
    
        sqlite3_bind_int(stmt, 1, part_id_edicao);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            strcpy(f_name, (const char *)sqlite3_column_text(stmt, 0));
            f_gender     = sqlite3_column_int(stmt, 1);
            f_d_personal = sqlite3_column_int(stmt, 2);
            f_d_sig      = sqlite3_column_int(stmt, 3);
            f_d_trigger  = sqlite3_column_int(stmt, 4);
            f_n_personal = sqlite3_column_int(stmt, 5);
            f_n_sig      = sqlite3_column_int(stmt, 6);
            f_n_trigger  = sqlite3_column_int(stmt, 7);
            strcpy(f_desc, (const char *)sqlite3_column_text(stmt, 8));
            strcpy(f_link, (const char *)sqlite3_column_text(stmt, 9)); // NOVO
            strcpy(f_link2, (const char *)sqlite3_column_text(stmt, 10));
        }
        sqlite3_finalize(stmt);        
    }

    // Expandido para 8 campos: 0=Name, 1=Gender, 2=Diurnal, 3=Nocturnal, 4=Desc, 5=Link, 6=Link2, 7=Save
    int campo_atual = 0; 
    int sub_campo = 0;   
    int ch;
    int loop = 1;

    werase(shadow); 
    wattron(shadow, COLOR_PAIR(9)); 
    box(shadow, 0, 0); 
    wattroff(shadow, COLOR_PAIR(9)); 
    wnoutrefresh(shadow);

    while (loop) {        
        werase(win); wbkgd(win, COLOR_PAIR(13) | FLAGS); box(win, 0, 0);

        wattron(win, A_BOLD);
        mvwprintw(win, 0, (w_width - 28) / 2, part_id_edicao > 0 ? _(" Edit Arabic Part Formula ") : _(" New Arabic Part Formula "));

        // 1. Part Name
        wattron(win, A_BOLD);
        mvwprintw(win, 2, 4, _("Part Name: "));
        wattroff(win, A_BOLD);

        if (campo_atual == 0) wattron(win, COLOR_PAIR(28) | A_REVERSE);
        mvwprintw(win, 2, 17, " %-32s ", f_name);
        wattroff(win, campo_atual == 0 ? (COLOR_PAIR(28) | A_REVERSE) : 0);

        // 2. Gender Scope
        wattron(win, A_BOLD);
        mvwprintw(win, 4, 4, _("Gender Scope: "));
        wattroff(win, A_BOLD);

        if (campo_atual == 1) wattron(win, COLOR_PAIR(28) | A_REVERSE);
        mvwprintw(win, 4, 22, " <%s> ", f_gender == 1 ? _("Masculine") : f_gender == 2 ? _("Feminine") : _("Neuter/Universal"));
        wattroff(win, campo_atual == 1 ? (COLOR_PAIR(28) | A_REVERSE) : 0);

        // 3. Diurnal Formula
        wattron(win, A_BOLD);
        mvwprintw(win, 6, 4, _("Diurnal Formula (Personal Point + Significator - Trigger):"));
        wattroff(win, A_BOLD);

        mvwprintw(win, 7, 6, "[");
        for(int s = 0; s < 3; s++) {
            if (campo_atual == 2 && sub_campo == s) wattron(win, COLOR_PAIR(15) | A_REVERSE | A_BOLD);
            if (s == 0) wprintw(win, " %s ", get_dynamic_id_label(f_d_personal, obj, num_objects));
            if (s == 1) wprintw(win, " + %s ", get_dynamic_id_label(f_d_sig, obj, num_objects));
            if (s == 2) wprintw(win, " - %s ", get_dynamic_id_label(f_d_trigger, obj, num_objects));
            wattroff(win, (campo_atual == 2 && sub_campo == s) ? (COLOR_PAIR(15) | A_REVERSE | A_BOLD) : 0);
        }
        wprintw(win, " ]");

        // 4. Nocturnal Formula
        wattron(win, A_BOLD);
        mvwprintw(win, 9, 4, _("Nocturnal Formula (Personal Point + Significatpr - Trigger):"));
        wattroff(win, A_BOLD);

        mvwprintw(win, 10, 6, "[");
        for(int s = 0; s < 3; s++) {
            if (campo_atual == 3 && sub_campo == s) wattron(win, COLOR_PAIR(15) | A_REVERSE | A_BOLD);
            if (s == 0) wprintw(win, " %s ", get_dynamic_id_label(f_n_personal, obj, num_objects));
            if (s == 1) wprintw(win, " + %s ", get_dynamic_id_label(f_n_sig, obj, num_objects));
            if (s == 2) wprintw(win, " - %s ", get_dynamic_id_label(f_n_trigger, obj, num_objects));
            wattroff(win, (campo_atual == 3 && sub_campo == s) ? (COLOR_PAIR(15) | A_REVERSE | A_BOLD) : 0);
        }
        wprintw(win, " ]");

        // 5. NOVO CAMPO: Link Tradicional (Selecionável por Setas)
        wattron(win, A_BOLD);
        mvwprintw(win, 12, 4, _("Traditional Links: "));
        wattroff(win, A_BOLD);

        if (campo_atual == 4) wattron(win, COLOR_PAIR(15) | A_REVERSE | A_BOLD); // Amarelo destaque para seleção
        mvwprintw(win, 12, 22, " < %s > ", f_link);
        wattroff(win, campo_atual == 4 ? (COLOR_PAIR(15) | A_REVERSE | A_BOLD) : 0);

        if (campo_atual == 5) wattron(win, COLOR_PAIR(15) | A_REVERSE | A_BOLD); // Amarelo destaque para seleção
        mvwprintw(win, 13, 22, " < %s > ", f_link2);
        wattroff(win, campo_atual == 5 ? (COLOR_PAIR(15) | A_REVERSE | A_BOLD) : 0);

        // 6. Description Textual (Empurrado para a linha 14)
        wattron(win, A_BOLD);
        mvwprintw(win, 15, 4, _("Description: "));
        wattroff(win, A_BOLD);

        if (campo_atual == 6) wattron(win, COLOR_PAIR(28) | A_REVERSE);
        
        // 1. Coleta dinamicamente os limites de largura da janela atual
        int w_max_y, w_max_x;
        getmaxyx(win, w_max_y, w_max_x);

        (void)w_max_y; 

        // 2. Calcula o espaço horizontal disponível a partir da coluna 4
        // Descontamos 2 caracteres para proteger a moldura da janela
        int largura_util = w_max_x - 4 - 2;
        if (largura_util > 120) largura_util = 120; // Limite máximo do seu campo

        // 3. Limpa a área imprimindo os espaços em branco controlados pelo limite real
        mvwprintw(win, 16, 4, " ");
        for (int i = 0; i < largura_util; i++) {
            wprintw(win, " ");
        }
        wprintw(win, " ");

        // 4. Cria um buffer temporário para extrair apenas a fatia visível do texto
        char f_desc_visivel[125] = {0};
        strncpy(f_desc_visivel, f_desc, largura_util);

        // 5. Imprime o texto truncado com segurança dentro da área reservada
        mvwprintw(win, 16, 5, "%s", f_desc_visivel);

        if (campo_atual == 6) wattroff(win, COLOR_PAIR(28) | A_REVERSE);


        // 7. Botão de Gravação (Empurrado para a linha 17)
        if (campo_atual == 7) wattron(win, COLOR_PAIR(11) | A_REVERSE);
        else wattron(win, COLOR_PAIR(11));
        mvwprintw(win, 19, (w_width - 16) / 2, _(" [ SAVE FORMULA ] "));
        wattroff(win, COLOR_PAIR(11) | A_REVERSE);

        wattron(win, A_DIM);
        mvwprintw(win, 21, 2, _("Use [↑/↓] Vertical Fields | [←/→] Adjust Value | [TAB] Horizontal Fields | [ENTER] Edit Text / Save"));
        wattroff(win, A_DIM);

        wnoutrefresh(win);

        doupdate();

        ch = wgetch(win);

        switch (ch) {
            case KEY_UP:
                campo_atual = (campo_atual - 1 + 8) % 8; // Módulo 7 campos
                sub_campo = 0;
                break;
            case KEY_DOWN:
                campo_atual = (campo_atual + 1) % 8;
                sub_campo = 0;
                break;
            case KEY_RIGHT:
                if (campo_atual == 1) { f_gender = (f_gender % 3) + 1; }
                if (campo_atual == 2) {
                    if (sub_campo == 0) f_d_personal = rotate_dynamic_id(f_d_personal, 1, obj, num_objects);
                    if (sub_campo == 1) f_d_sig      = rotate_dynamic_id(f_d_sig, 1, obj, num_objects);
                    if (sub_campo == 2) f_d_trigger  = rotate_dynamic_id(f_d_trigger, 1, obj, num_objects);
                }
                if (campo_atual == 3) {
                    if (sub_campo == 0) f_n_personal = rotate_dynamic_id(f_n_personal, 1, obj, num_objects);
                    if (sub_campo == 1) f_n_sig      = rotate_dynamic_id(f_n_sig, 1, obj, num_objects);
                    if (sub_campo == 2) f_n_trigger  = rotate_dynamic_id(f_n_trigger, 1, obj, num_objects);
                }
                if (campo_atual == 4) { // NOVO: Roda os glifos/casas para a DIREITA
                    int next_idx = rotacionar_indice_link(f_link, 1);
                    strcpy(f_link, OPCOES_LINK[next_idx]);
                }
                if (campo_atual == 5) { // NOVO: Roda os glifos/casas para a DIREITA
                    int next_idx2 = rotacionar_indice_link(f_link2, 1);
                    strcpy(f_link2, OPCOES_LINK[next_idx2]);
                }
                break;
            case KEY_LEFT:
                if (campo_atual == 1) { f_gender = (f_gender - 2 + 3) % 3 + 1; }
                if (campo_atual == 2) {
                    if (sub_campo == 0) f_d_personal = rotate_dynamic_id(f_d_personal, -1, obj, num_objects);
                    if (sub_campo == 1) f_d_sig      = rotate_dynamic_id(f_d_sig, -1, obj, num_objects);
                    if (sub_campo == 2) f_d_trigger  = rotate_dynamic_id(f_d_trigger, -1, obj, num_objects);
                }
                if (campo_atual == 3) {
                    if (sub_campo == 0) f_n_personal = rotate_dynamic_id(f_n_personal, -1, obj, num_objects);
                    if (sub_campo == 1) f_n_sig      = rotate_dynamic_id(f_n_sig, -1, obj, num_objects);
                    if (sub_campo == 2) f_n_trigger  = rotate_dynamic_id(f_n_trigger, -1, obj, num_objects);
                }
                if (campo_atual == 4) { // NOVO: Roda os glifos/casas para a ESQUERDA
                    int prev_idx = rotacionar_indice_link(f_link, -1);
                    strcpy(f_link, OPCOES_LINK[prev_idx]);
                }
                if (campo_atual == 5) { // NOVO: Roda os glifos/casas para a ESQUERDA
                    int prev_idx2 = rotacionar_indice_link(f_link2, -1);
                    strcpy(f_link2, OPCOES_LINK[prev_idx2]);
                }
                break;
            case '\t': 
                if (campo_atual == 2 || campo_atual == 3) {
                    sub_campo = (sub_campo + 1) % 3;
                }
                break;
            case 10: // ENTER
                if (campo_atual == 0) { // Campo Part Name: mvwprintw na coordenada (2, 17)
                    // Passamos x = 17 exatamente para sincronizar com mvwprintw(win, 2, 17, ...)
                    campo_texto_amigavel_avancado(win, 2, 17, f_name, 32);
                }
                if (campo_atual == 6) { // Campo Description: mvwprintw na coordenada (15, 4)
                    // Passamos x = 17 exatamente para sincronizar com mvwprintw(win, 14, 17, ...)
                    campo_texto_amigavel_avancado(win, 16, 4, f_desc, 512);
                }
                if (campo_atual == 7) { // SAVE FORMULA (Sua rotina SQLite global idêntica...)

                    sqlite3_stmt *stmt = NULL;
                    const char *sql_query = NULL;

                    // 1. Definimos a estrutura da query com os marcadores '?' no lugar dos valores
                    if (part_id_edicao > 0) {
                        sql_query = "UPDATE arabic_parts SET name=?, gender_id=?, "
                                    "diurnal_personal_point=?, diurnal_significator=?, diurnal_trigger=?, "
                                    "nocturnal_personal_point=?, nocturnal_significator=?, nocturnal_trigger=?, "
                                    "description=?, link=?, link2=? WHERE id=?;";
                    } else {
                        sql_query = "INSERT INTO arabic_parts (name, gender_id, diurnal_personal_point, "
                                    "diurnal_significator, diurnal_trigger, nocturnal_personal_point, "
                                    "nocturnal_significator, nocturnal_trigger, description, link, link2) "
                                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
                    }

                    // 2. Compila a query no SQLite
                    int rc = sqlite3_prepare_v2(global_db, sql_query, -1, &stmt, NULL);
                    
                    if (rc == SQLITE_OK) {
                        // 3. Vincula os parâmetros (a contagem dos índices começa em 1)
                        
                        // TEXTOS: SQLITE_TRANSIENT faz o SQLite criar uma cópia interna segura da string
                        sqlite3_bind_text(stmt, 1, f_name, -1, SQLITE_TRANSIENT);
                        
                        // INTEIROS
                        sqlite3_bind_int(stmt, 2, f_gender);
                        sqlite3_bind_int(stmt, 3, f_d_personal);
                        sqlite3_bind_int(stmt, 4, f_d_sig);
                        sqlite3_bind_int(stmt, 5, f_d_trigger);
                        sqlite3_bind_int(stmt, 6, f_n_personal);
                        sqlite3_bind_int(stmt, 7, f_n_sig);
                        sqlite3_bind_int(stmt, 8, f_n_trigger);
                        
                        // TEXTOS
                        sqlite3_bind_text(stmt, 9, f_desc, -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(stmt, 10, f_link, -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(stmt, 11, f_link2, -1, SQLITE_TRANSIENT);
                        
                        // Se for UPDATE, vincula o ID no 11º ponto de interrogação
                        if (part_id_edicao > 0) {
                            sqlite3_bind_int(stmt, 12, part_id_edicao);
                        }

                        // 4. Executa a query
                        rc = sqlite3_step(stmt);
                        if (rc != SQLITE_DONE) {
                            // Opcional: Tratar erro de execução aqui se rc não for SQLITE_DONE
                            fprintf(stderr, "Erro ao executar: %s\n", sqlite3_errmsg(global_db));
                            sqlite3_finalize(stmt);
                            delwin(shadow); 
                            delwin(win);
                            return;
                        }
                    } else {
                        // Opcional: Tratar erro de compilação da query aqui
                        fprintf(stderr, "Erro ao preparar: %s\n", sqlite3_errmsg(global_db));
                        delwin(shadow); 
                        delwin(win);
                        return;
                    }

                    sqlite3_finalize(stmt);

                    loop = 0;
 
                }
                break;

            case 27: // ESC
                loop = 0;
                break;
        }
    }
        
    delwin(shadow); 
    delwin(win);
}


void calcular_aspectos_partes(ChartObject *obj, int num_objects, ArabicPartCalculada *lista, int qtd_partes, AspectPartMatrix *m_part) {
    memset(m_part, 0, sizeof(AspectPartMatrix));

    // 1. CARREGAMENTO DOS ORBES DINÂMICOS DO SISTEMA
    // Criamos buffers locais para receber os dados da sua função get_planet_orbis
    int planet_ids[12] = {0};
    double planet_orbis[12] = {0.0};
    char planet_symbols[12][10];
    memset(planet_symbols, 0, sizeof(planet_symbols));

    // Mapeia os 12 IDs que correspondem aos planetas type==1 (Sol=1 até Nodo Sul=12)
    int total_ids_mapeados = 0;
    for (int p = 0; p < num_objects; p++) {
        if (obj[p].type == 1 && total_ids_mapeados < 12) {
            planet_ids[total_ids_mapeados] = obj[p].id;
            total_ids_mapeados++;
        }
    }

    // Dispara a sua função nativa para povoar o vetor planet_orbis com os valores do sistema
    get_planet_orbis(planet_ids, planet_orbis, planet_symbols, total_ids_mapeados);

    // Definição dos ângulos geométricos maiores
    double angulos_aspectos[] = {0.0, 60.0, 90.0, 120.0, 180.0};
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍"};

    // 2. CRUZAMENTO GEOMÉTRICO (Planetas vs Partes)
    int idx_planeta_valido = 0;

    for (int p = 0; p < num_objects; p++) {
        if (obj[p].type != OBJ_PLANET) continue; 
        
        // BLOQUEIO MATEMÁTICO: Impede que Urano, Netuno e Plutão ocupem espaço nas linhas da matriz
        if (!show_modern_planets && (obj[p].id >= 8 && obj[p].id <= 10)) {
            continue;
        }
        
        if (idx_planeta_valido >= 12) break; 

        double orbe_maximo = planet_orbis[idx_planeta_valido];
        if (orbe_maximo <= 0.0) {
            orbe_maximo = (obj[p].id <= 7) ? 5.0 : 3.0;
        }

        for (int i = 0; i < qtd_partes; i++) {
            double lon_planeta = obj[p].longitude;
            double lon_parte   = lista[i].longitude;

            double diff = fabs(lon_planeta - lon_parte);
            if (diff > 180.0) diff = 360.0 - diff;

            // Varre os 5 aspectos maiores aplicando o orbe dinâmico do planeta
            for (int a = 0; a < 5; a++) {
                double target_angulo = angulos_aspectos[a];
                
                // ====================
                // APLICA O MEIO-ORBE!
                // ====================
                if (fabs(diff - target_angulo) <= (orbe_maximo + 1.0) / 2.0 ) {
                    PartAspectCell *cell = &m_part->grid[idx_planeta_valido][i];
                    cell->has_aspect = true;
                    strcpy(cell->symbol, simbolos_aspectos[a]);
                    
                    // Acopla a paleta de cores padronizada da sua aplicação
                    if (a == 2 || a == 4) cell->color_pair = 11;      // Vermelho para □ e ☍
                    else if (a == 0)      cell->color_pair = 7;       // Magenta para Conjunção
                    else                  cell->color_pair = 8;       // Azul para △ e ⚹
                    break; 
                }
            }
        }
        idx_planeta_valido++;
    }
}


/**
 * Função auxiliar que copia exatamente 3 LETRAS reais de 'origem' para 'destino',
 * convertendo-as para maiúsculo (incluindo a-z, ç e acentos em UTF-8).
 */
void copy_upper3_utf8(const char *origem, char *destino) {
    int i = 0; // Índice na string de origem
    int d = 0; // Índice na string de destino (bytes gravados)
    int letras_copiadas = 0;

    // Queremos extrair exatamente 3 letras visíveis
    while (origem[i] != '\0' && letras_copiadas < 3) {
        
        // 1. Trata caracteres acentuados comuns ou 'ç' em UTF-8 (Começam com 0xC3)
        if ((unsigned char)origem[i] == 0xC3) {
            unsigned char proximo_byte = (unsigned char)origem[i + 1];

            destino[d] = 0xC3; // Mantém o byte indicador do UTF-8

            // Se o próximo byte for uma letra minúscula acentuada/ç
            if (proximo_byte >= 0xA0 && proximo_byte <= 0xBF) {
                destino[d + 1] = proximo_byte - 0x20; // Converte para maiúscula
            } else {
                destino[d + 1] = proximo_byte; // Já era maiúscula ou símbolo
            }

            i += 2; // Avança 2 bytes na origem
            d += 2; // Avança 2 bytes no destino
            letras_copiadas++;
        }
        // 2. Trata alfabeto padrão ASCII minúsculo (a-z)
        else if (origem[i] >= 'a' && origem[i] <= 'z') {
            destino[d] = origem[i] - 32;
            i++;
            d++;
            letras_copiadas++;
        }
        // 3. Trata qualquer outro caractere de 1 byte (números, espaços, já maiúsculas)
        else {
            destino[d] = origem[i];
            i++;
            d++;
            letras_copiadas++;
        }
    }

    destino[d] = '\0'; // Finaliza a string da abreviação corretamente
}

void get_part_abbreviation(char *name, char *abreviacao) {

    if (strncmp(name, "Part of ", 8) == 0) {
        copy_upper3_utf8(&name[8], abreviacao);
    }
    else if (strncmp(name, "Lot of ", 7) == 0) {
        copy_upper3_utf8(&name[7], abreviacao);
    }
    else if (strncmp(name, "Lote do ", 8) == 0) { 
        copy_upper3_utf8(&name[8], abreviacao);
    }
    else if (strncmp(name, "Lote da ", 8) == 0) { 
        copy_upper3_utf8(&name[8], abreviacao);
    }    
    else if (strncmp(name, "Lot do ", 7) == 0) { 
        copy_upper3_utf8(&name[7], abreviacao);
    }
    else if (strncmp(name, "Lot da ", 7) == 0) { 
        copy_upper3_utf8(&name[7], abreviacao);
    }
    else if (strncmp(name, "Lote dos ", 9) == 0) { 
        copy_upper3_utf8(&name[9], abreviacao);
    }
    else if (strncmp(name, "Lote das ", 9) == 0) { 
        copy_upper3_utf8(&name[9], abreviacao);
    }    
    else if (strncmp(name, "Lot dos ", 8) == 0) { 
        copy_upper3_utf8(&name[8], abreviacao);
    }
    else if (strncmp(name, "Lot das ", 8) == 0) { 
        copy_upper3_utf8(&name[8], abreviacao);
    }
    else if (strncmp(name, "Pars ", 5) == 0) {
        copy_upper3_utf8(&name[5], abreviacao);
    }
    else if (strncmp(name, "Parte do ", 9) == 0) {
        copy_upper3_utf8(&name[9], abreviacao); 
    }
    else if (strncmp(name, "Parte da ", 9) == 0) {
        copy_upper3_utf8(&name[9], abreviacao); 
    }    
    else if (strncmp(name, "Parte dos ", 10) == 0) {
        copy_upper3_utf8(&name[10], abreviacao); 
    }
    else if (strncmp(name, "Parte das ", 10) == 0) {
        copy_upper3_utf8(&name[10], abreviacao); 
    }
    else {
        copy_upper3_utf8(&name[0], abreviacao);
    }
}




void display_part_aspects(ChartObject *obj, int num_objects, ArabicPartCalculada *lista, int qtd_partes) {
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int table_height = 28;
    int table_width = max_x - 4;
    int start_y = (max_y - table_height) / 2;
    int start_x = 2;

    WINDOW *aspects_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *aspects_shadow = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(aspects_shadow);
    wattron(aspects_shadow, COLOR_PAIR(9)); 
    box(aspects_shadow, 0, 0); 
    wattroff(aspects_shadow, COLOR_PAIR(9));
    wnoutrefresh(aspects_shadow);

    box(aspects_win, 0, 0);
    wbkgd(aspects_win, COLOR_PAIR(6) | FLAGS);
    wnoutrefresh(aspects_win);

    doupdate();

    keypad(aspects_win, TRUE);
    nodelay(aspects_win, FALSE);

    // --- VARIÁVEIS DE CONTROLE DA PAGINAÇÃO E SCROLL ---
    int pagina_offset = 0;
    int max_partes_tela = (table_width - 64) / 6;
    
    int row_offset = 0; // Deslocamento vertical inicial
    // Cada planeta ocupa 2 linhas físicas na tabela. Descontamos 5 linhas de cabeçalho/rodapé.
    int max_linhas_tela = (table_height - 10) / 2; 
    
    bool deve_fazer_wipe = true;

    wattron(aspects_win, A_DIM);
    mvwprintw(aspects_win, 2, table_width - 35, _("[←/→] More Parts [↑/↓] Scroll"));
    wattroff(aspects_win, A_DIM);

    while (1) {
        werase(aspects_win);
        box(aspects_win, 0, 0);
        
        wattron(aspects_win, A_BOLD);
        const char *title = _(" Arabic Parts Aspect Matrix Grid ");
        mvwprintw(aspects_win, 0, (table_width - get_visual_width(title)) / 2, title);
        wattroff(aspects_win, A_BOLD);

        wattron(aspects_win, A_DIM);
        mvwprintw(aspects_win, 2, table_width - 35, _("[←/→] More Parts [↑/↓] Scroll"));
        wattroff(aspects_win, A_DIM);

        int partes_nesta_pagina = qtd_partes - pagina_offset;
        if (partes_nesta_pagina > max_partes_tela) {
            partes_nesta_pagina = max_partes_tela;
        }

        AspectPartMatrix m_part;
        calcular_aspectos_partes(obj, num_objects, &lista[pagina_offset], partes_nesta_pagina, &m_part);

        // Rastreia e mapeia quais índices de 'obj' são planetas válidos para paginação vertical
        int mapeamento_planetas[255];
        int total_planetas_validos = 0;
        for (int p = 0; p < num_objects; p++) {
            if (obj[p].type == 1) {
                if (!show_modern_planets && (obj[p].id >= 8 && obj[p].id <= 10)) {
                    continue;
                }
                mapeamento_planetas[total_planetas_validos] = p;
                total_planetas_validos++;
            }
        }

        // Limita a quantidade de linhas que serão renderizadas de fato nesta tela
        int linhas_nesta_tela = total_planetas_validos - row_offset;
        if (linhas_nesta_tela > max_linhas_tela) {
            linhas_nesta_tela = max_linhas_tela;
        }

        // 1. ANTES DO WIPE: Desenha o Cabeçalho lateral baseado no row_offset
        for (int r = 0; r < linhas_nesta_tela; r++) {
            int p_idx = mapeamento_planetas[row_offset + r];
            wattron(aspects_win, A_BOLD);
            mvwprintw(aspects_win, 5 + 2 * r, 8, "%-10s", obj[p_idx].object);
            wattroff(aspects_win, A_BOLD);
        }

        // Renderização por colunas (Efeito Wipe)
        for (int j = 0; j < partes_nesta_pagina; j++) {
            
            // A) Desenha a abreviação do cabeçalho superior desta coluna específica
            char abreviacao[6];
            get_part_abbreviation(lista[pagina_offset + j].name, abreviacao);
            wattron(aspects_win, A_BOLD | COLOR_PAIR(13));
            mvwprintw(aspects_win, 2, 14 + 6 * j, "%s", abreviacao); // Movido para X=14 para alinhar com o grid
            wattroff(aspects_win, A_BOLD | COLOR_PAIR(13));

            int degree = (int)fmod(lista[pagina_offset + j].longitude, 30);
            const char *sign_str = get_sign((int)(lista[pagina_offset + j].longitude / 30));
            char text[10];
            snprintf(text, 10, "%d%s", degree, sign_str); 

            mvwprintw(aspects_win, 3, 14 + 6 * j, text);

            // B) Desenha o Grid Estrutural desta coluna (Linhas horizontais e verticais)
            for (int i = 0; i < (linhas_nesta_tela * 2); i++) {
                mvwprintw(aspects_win, 5 + i, 12 + 6 * j, "│");
                if (j == partes_nesta_pagina - 1) {
                    mvwprintw(aspects_win, 5 + i, 12 + 6 * (j + 1), "│");
                }
            }

            for (int i = 0; i < linhas_nesta_tela + 1; i++) {
                mvwprintw(aspects_win, 4 + 2 * i, 12 + 6 * j, (i == 0) ? "┼─────┼" : "┼─────┼");
            }
            
            // C) Preenche os dados astrológicos baseados no deslocamento vertical
            for (int r = 0; r < linhas_nesta_tela; r++) {
                int idx_p_matriz = row_offset + r; // Posição real do planeta na matriz calculada
                int p_idx = mapeamento_planetas[idx_p_matriz]; // Posição real no objeto astronômico

                PartAspectCell cell = m_part.grid[idx_p_matriz][j];
                int x_pos = 14 + 6 * j;
                int y_pos_simbolo = 5 + 2 * r;
                int y_pos_angulo  = 6 + 2 * r;

                if (cell.has_aspect) {
                    wattron(aspects_win, COLOR_PAIR(cell.color_pair) | A_DIM);
                    mvwprintw(aspects_win, y_pos_simbolo, x_pos + 1, "%s", cell.symbol);
                    wattroff(aspects_win, COLOR_PAIR(cell.color_pair) | A_DIM);

                    wattron(aspects_win, COLOR_PAIR(10) | A_DIM);
                    double diff = fabs(obj[p_idx].longitude - lista[pagina_offset + j].longitude);
                    if (diff > 180.0) diff = 360.0 - diff;
                    
                    double angulos_base[] = {0.0, 60.0, 90.0, 120.0, 180.0};
                    double orbe_real = 0.0;
                    double menor_distancia = 999.0;

                    for (int a = 0; a < 5; a++) {
                        double dist_ao_aspecto = fabs(diff - angulos_base[a]);
                        if (dist_ao_aspecto < menor_distancia) {
                            menor_distancia = dist_ao_aspecto;
                            orbe_real = dist_ao_aspecto;
                        }
                    }

                    char ag_txt[8];
                    snprintf(ag_txt, sizeof(ag_txt), "%3.1f", orbe_real);
                    mvwprintw(aspects_win, y_pos_angulo, x_pos + 1, "%s", ag_txt);
                    wattroff(aspects_win, COLOR_PAIR(10) | A_DIM);
                } else {
                    wattron(aspects_win, COLOR_PAIR(10) | A_DIM);
                    mvwprintw(aspects_win, y_pos_simbolo, x_pos - 1, "░░░░░");
                    wattroff(aspects_win, COLOR_PAIR(10) | A_DIM);
                }
            }

            if (deve_fazer_wipe) {
                wrefresh(aspects_win);
                napms(15);
            }
        }        
        
        deve_fazer_wipe = false; 

        // --- RODAPÉ DINÂMICO ADAPTADO PARA SCROLL VERTICAL ---
        int ate_qual = pagina_offset + partes_nesta_pagina;
        mvwprintw(aspects_win, table_height - 3, 6, _("(*) Numbers = angular difference in degrees"));
        mvwprintw(aspects_win, table_height - 1, 2, 
            "%s (%d-%d/%d) - [ ↑/↓ ] %s (%d/%d)", 
            _("ESC: Exit - [ ←/→ ] Parts"), pagina_offset + 1, ate_qual, qtd_partes, _("Scroll Planets"), row_offset + linhas_nesta_tela, total_planetas_validos);

        wnoutrefresh(aspects_win);
        doupdate();

        int ch = wgetch(aspects_win);

        if (ch == 27 || ch == 'q') { // ESC
            break;
        }

        // CONTROLE VERTICAL: Scroll de linhas (Planetas)
        if (ch == KEY_DOWN) {
            if (row_offset + max_linhas_tela < total_planetas_validos) {
                row_offset++;
                // Opcional: mude para true se quiser que o wipe aconteça no scroll vertical também
                deve_fazer_wipe = false; 
            }
        }
        else if (ch == KEY_UP) {
            if (row_offset > 0) {
                row_offset--;
                deve_fazer_wipe = false;
            }
        }
        // CONTROLE HORIZONTAL: Paginação de colunas (Partes)
        else if (ch == KEY_RIGHT) {
            if (pagina_offset + max_partes_tela < qtd_partes) {
                pagina_offset += max_partes_tela;
                deve_fazer_wipe = true;
            }
        } 
        else if (ch == KEY_LEFT) {
            if (pagina_offset - max_partes_tela >= 0) {
                pagina_offset -= max_partes_tela;
                deve_fazer_wipe = true;
            } else if (pagina_offset != 0) {
                pagina_offset = 0;
                deve_fazer_wipe = true;
            }
        }
        else if (qtd_partes <= max_partes_tela && total_planetas_validos <= max_linhas_tela) {
            // Se tudo couber perfeitamente na tela atual, qualquer outra tecla fecha a janela
            break;
        }
    }
    
    delwin(aspects_shadow);
    delwin(aspects_win);
    touchwin(stdscr); 
    refresh();
}
        
    
void deletar_parte_arabe_com_confirmacao(int id_banco_alvo, const char *nome_parte) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int w_height = 8, w_width = 54;
    int start_y = (max_y - w_height) / 2;
    int start_x = (max_x - w_width) / 2;

    WINDOW *conf_win = newwin(w_height, w_width, start_y, start_x);
    WINDOW *conf_shadow = newwin(w_height, w_width, start_y + 1, start_x + 1);
    
    nodelay(conf_win, FALSE);
    keypad(conf_win, TRUE);
    //curs_set(0);

    // Desenha a sombra do pop-up
    werase(conf_shadow);
    wattron(conf_shadow, COLOR_PAIR(24)); 
    box(conf_shadow, 0, 0); 
    wattroff(conf_shadow, COLOR_PAIR(24));
    wnoutrefresh(conf_shadow);

    int botao_focado = 0; // 0 = Confirmar (Yes), 1 = Cancelar (Cancel)
    int confirmado = 0;
    int ch;

    wbkgd(conf_win, COLOR_PAIR(26) | FLAGS);

    while (1) {
        // Redesenha a estrutura fixa da caixinha de alerta a cada ciclo
        werase(conf_win);
        wattron(conf_win, COLOR_PAIR(27) | A_BOLD);
        box(conf_win, 0, 0);
        const char *title = _(" CONFIRM DELETE ");  
        mvwprintw(conf_win, 0, (w_width - get_visual_width(title)) / 2, title);
        wattroff(conf_win, COLOR_PAIR(27) | A_BOLD);
        
        mvwprintw(conf_win, 2, 4, _("Are you sure you want to permanently delete:"));
        mvwprintw(conf_win, 3, 4, "\"%.46s\" ?", nome_parte);
        
        // Desenha dinamicamente o Botão YES com base no foco
        int attr_yes = (botao_focado == 0) ? (COLOR_PAIR(36) | A_REVERSE | A_BOLD) : COLOR_PAIR(23);
        wattron(conf_win, attr_yes);
        mvwprintw(conf_win, 5, 10, "   YES   ");
        wattroff(conf_win, attr_yes);

        // Desenha dinamicamente o Botão CANCEL com base no foco
        int attr_cancel = (botao_focado == 1) ? (COLOR_PAIR(36) | A_REVERSE | A_BOLD) : COLOR_PAIR(23);
        wattron(conf_win, attr_cancel);
        mvwprintw(conf_win, 5, 32, "  CANCEL  ");
        wattroff(conf_win, attr_cancel);
        
        wnoutrefresh(conf_win);

        doupdate();

        ch = wgetch(conf_win);

        if (ch == KEY_LEFT || ch == KEY_RIGHT) {
            // Alterna o foco de forma circular entre os dois botões
            botao_focado = 1 - botao_focado;
        } 
        else if (ch == 10) { // ENTER
            // Executa a ação do botão selecionado
            confirmado = (botao_focado == 0) ? 1 : 0;
            break;
        } 
        else if (ch == 'y' || ch == 'Y') { // Atalho direto para aceitar
            confirmado = 1;
            break;
        } 
        else if (ch == 'n' || ch == 'N' || ch == 27) { // Atalhos diretos para cancelar
            confirmado = 0;
            break;
        }
    }

    if (confirmado) {
        sqlite3 *db;
        sqlite3_stmt *stmt;
        int rc;
        
        db = open_database();
        if (!db) {
            fprintf(stderr, "Failed to open database in deletar_parte_arabe_com_confirmacao\n");
            delwin(conf_shadow);
            delwin(conf_win);
            return;
        }

        const char *sql_delete = "DELETE FROM arabic_parts WHERE id = ?;";
        rc = sqlite3_prepare_v2(db, sql_delete, -1, &stmt, NULL);
        
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare delete statement: %s\n", sqlite3_errmsg(db));            
            delwin(conf_shadow);
            delwin(conf_win);
            sqlite3_finalize(stmt);

            show_alert_popup(_("Failed deleting arabic part!"), nome_parte);

            return;
        }
        
        sqlite3_bind_int(stmt, 1, id_banco_alvo);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed deleting arabic part: %s\n", sqlite3_errmsg(db));
            delwin(conf_shadow);
            delwin(conf_win);
            sqlite3_finalize(stmt);

            show_alert_popup(_("Failed deleting arabic part!"), nome_parte);

            return;
        }

        show_alert_popup(_("Arabic Part deleted successfully!"), nome_parte);
    }
    
    delwin(conf_shadow);
    delwin(conf_win);
}
