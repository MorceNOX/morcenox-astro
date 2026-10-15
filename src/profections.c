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

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ncursesw/curses.h>
#include "swephexp.h"
#include "var.h"
#include "helper.h"
#include "draw-chart.h"
#include "planet_table.h"
#include "db-utils.h"
#include "profections.h"



int obter_regente_trad(int id_signo) {
    switch (id_signo) {
        case 0:  case 7:  return 5; // Áries e Escorpião = Marte
        case 1:  case 6:  return 4; // Touro e Libra = Vênus
        case 2:  case 5:  return 3; // Gêmeos e Virgem = Mercúrio
        case 3:           return 2; // Câncer = Lua
        case 4:           return 1; // Leão = Sol
        case 8:  case 11: return 6; // Sagitário e Peixes = Júpiter
        case 9:  case 10: return 7; // Capricórnio e Aquário = Saturno
        default: return 0;
    }
}


void gerar_tabela_profecoes_anuais(LinhaProfecao *tabela, int max_anos, double lon_asc_natal, double jd_nascimento) {
    int ano_natal, mes_n, dia_n, hora_n, min_n;
    double sec_n;
    
    // Converte o JD de nascimento para descobrir o ano inicial
    swe_jdut1_to_utc(jd_nascimento, 2, &ano_natal, &mes_n, &dia_n, &hora_n, &min_n, &sec_n);

    // Encontra o signo natal do Ascendente (0 = Áries, 11 = Peixes)
    int signo_asc_natal = (int)(lon_asc_natal / 30.0);

    for (int idade = 0; idade < max_anos; idade++) {
        LinhaProfecao *p = &tabela[idade];
        
        p->idade = idade;
        p->ano_calendario = ano_natal + idade; // No C puro: p->ano_calendario = ano_natal + idade;

        // 1. CÁLCULO DA CASA E DO SIGNO PROSPECTADO (Sign-by-Sign)
        // Ciclo de 12 casas (Idade 0 = Casa 1, Idade 12 = Casa 1...)
        p->casa_profectada = (idade % 12) + 1;
        p->signo_id = (signo_asc_natal + idade) % 12;

        // ACESSANDO SUAS FUNÇÕES DE SIGNOS:
        p->signo_nome  = get_sign_name(p->signo_id);
        p->signo_glifo = get_sign(p->signo_id);

        // 2. LONGITUDE EXATA DO ASCENDENTE PROFECTADO
        p->longitude_asc_prof = lon_asc_natal + (idade * 30.0);
        if (p->longitude_asc_prof >= 360.0) {
            p->longitude_asc_prof = fmod(p->longitude_asc_prof, 360.0);
        }

        // 3. IDENTIFICAÇÃO DO SENHOR DO ANO
        // ACESSANDO SUAS FUNÇÕES DE PLANETAS:
        p->senhor_ano_id    = obter_regente_trad(p->signo_id);
        p->senhor_ano_nome  = obter_nome_planeta_por_id(p->senhor_ano_id);
        p->senhor_ano_glifo = obter_glifo_planeta_por_id(p->senhor_ano_id);
    }
}



void exportar_profecoes_to_csv(LinhaProfecao *tabela_prof, int qtd_profecoes) {
    if (tabela_prof == NULL || qtd_profecoes <= 0) {
        show_alert_popup(_("Nothing to Print!"), _("No file created!"));
        return;
    }

    desativar_arrasto_mouse();

    char file_name[120];
    memset(file_name, 0, sizeof(file_name));

    set_file_name(file_name, sizeof(file_name));    
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/export/%s.csv", CONFIG_PATH, file_name);

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        show_alert_popup(_("Error attempting to save profections to file!"), file_path);
        ativar_arrasto_mouse();
        flushinp();
        return;
    }

    fprintf(file, _("Age;Year;Activated House;Sign Glyph;Sign Name;Degrees Profected Asc;Lord of Year Glyph;Lord of Year Name\n"));

    // 2. Varredura do Array Gravando as Linhas
    for (int i = 0; i < qtd_profecoes; i++) {
        const LinhaProfecao *p = &tabela_prof[i];

        fprintf(file, _("%d;%d;House %d;%s;%s;%.4f;%s;%s\n"),
            p->idade,
            p->ano_calendario,
            p->casa_profectada,
            p->signo_glifo,
            p->signo_nome,
            p->longitude_asc_prof,
            p->senhor_ano_glifo,
            p->senhor_ano_nome
        );
    }

    fclose(file);

    char file_csv[128];
    snprintf(file_csv, sizeof(file_csv), "%s.csv", file_name);
    show_alert_popup(_("Profections File:"), file_csv);

    ativar_arrasto_mouse();
    flushinp();
}




void exibir_janela_tabela_profecoes(LinhaProfecao *cronograma_prof, int qtd_profecoes, int idade) {

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 31;
    int table_width = max_x - 10;
    int start_y = (max_y - table_height) / 2;
    int start_x = 5;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    keypad(table_win, TRUE);
    
    int scroll_offset = 0;
    int loop_interativo = 1;

    int max_linhas_exibicao = (table_height / 2) * 2 - 14;
    WINDOW *scroll_pad = newpad(500, table_width - 8); 

    // Desenha sombra e frame fixo de fundo
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0); 
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);
   
    wbkgd(table_win, COLOR_PAIR(13) | FLAGS);
    wbkgd(scroll_pad, COLOR_PAIR(13) | FLAGS); 

    // 2. Desenha o botão [X] no canto superior direito
    int col_fechar = getmaxx(table_win) - 4; // Abre espaço para 3 caracteres: '[', 'X', ']'

    wattron(table_win, COLOR_PAIR(13)); // Cor padrão para os colchetes
    mvwprintw(table_win, 0, col_fechar, "[");
    mvwprintw(table_win, 0, col_fechar + 2, "]");
    wattroff(table_win, COLOR_PAIR(13));

    wattron(table_win, COLOR_PAIR(13) | A_BOLD); // Cor de destaque (ex: Vermelho) para o X
    mvwprintw(table_win, 0, col_fechar + 1, "✖");
    wattroff(table_win, COLOR_PAIR(13) | A_BOLD);
    wnoutrefresh(table_win);

    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(100);

    drag_and_drop_on();

    while (loop_interativo) {
        werase(table_win);
        werase(scroll_pad);

        box(table_win, 0, 0);
        wattron(table_win, A_BOLD);
        const char *title = _(" Profections ");
        mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 0, col_fechar, "[");
        mvwprintw(table_win, 0, col_fechar + 2, "]");
        wattroff(table_win, COLOR_PAIR(13));

        wattron(table_win, COLOR_PAIR(13) | A_BOLD);
        mvwprintw(table_win, 0, col_fechar + 1, "✖");
        wattroff(table_win, COLOR_PAIR(13) | A_BOLD);

        if (scroll_offset > qtd_profecoes * 2 - max_linhas_exibicao) {
            scroll_offset = qtd_profecoes * 2 - max_linhas_exibicao;
        }
        if (scroll_offset < 0) scroll_offset = 0;

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 4, 2, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        // 1. CALIBRAÇÃO DAS COLUNAS ADAPTADA PARA PROFECÇÕES
        int col_idade = 0, col_ano = 16, col_casa = 26, col_signo = 44, col_senhor = 80;

        wattron(table_win, A_BOLD | COLOR_PAIR(13));
        mvwprintw(table_win, 5, col_idade + 4, _("Age")); 
        mvwprintw(table_win, 5, col_ano + 4, _("Year"));
        mvwprintw(table_win, 5, col_casa + 4, _("Activated House"));
        mvwprintw(table_win, 5, col_signo + 4, _("Profected Sign")); 
        mvwprintw(table_win, 5, col_senhor + 4, _("Lord of the Year"));
        wattroff(table_win, A_BOLD | COLOR_PAIR(13));

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 6, 2, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        int row_pad = 0;
        int linhas_reais_pad = qtd_profecoes * 2; // Onde 'qtd_profecoes' é a extensão da tabela (ex: 100 anos)

        if (qtd_profecoes == 0) {
            wattron(scroll_pad, A_DIM);
            mvwprintw(scroll_pad, row_pad, col_casa, _("No profection data available for this chart."));
            wattroff(scroll_pad, A_DIM);
        } else {
            for (int i = 0; i < qtd_profecoes; i++) {
                // 'cronograma_prof' é o array de estruturas do tipo LinhaProfecao
                LinhaProfecao *p = &cronograma_prof[i]; 

                // Aplica destaque de cor se a linha corresponder à idade atual do nativo no ano vigente
                int par_cor_ativo = COLOR_PAIR(13);
                int atributo_extra = A_NORMAL;
                if (p->idade == idade) { // Substitua pelo ano dinâmico desejado para destacar o ano atual
                    par_cor_ativo = COLOR_PAIR(8); 
                    atributo_extra = A_BOLD;
                }

                wattron(scroll_pad, par_cor_ativo | atributo_extra);

                mvwprintw(scroll_pad, row_pad, col_idade, "%3d y", p->idade);
                mvwprintw(scroll_pad, row_pad, col_ano, "%4d", p->ano_calendario);
                
                // Exibe a casa de profecção anual tradicional (1 a 12)
                mvwprintw(scroll_pad, row_pad, col_casa, "%s %d", _("House"), p->casa_profectada);
                
                // Extração matemática de alta fidelidade para o formato astrológico
                double posicao_no_signo = fmod(p->longitude_asc_prof, 30.0);
                int grau_signo = (int)floor(posicao_no_signo);
                int minuto_signo = (int)floor((posicao_no_signo - grau_signo) * 60.0);

                // Renderização na UI: grau, glifo, minutos (nome por extenso)
                mvwprintw(scroll_pad, row_pad, col_signo, "%2d° %s %02d'  (%s)", 
                          grau_signo, 
                          p->signo_glifo, 
                          minuto_signo, 
                          p->signo_nome);

                // Exibe o Senhor do Ano (Glifo + Nome Traduzido) capturado das suas funções
                mvwprintw(scroll_pad, row_pad, col_senhor, "%s  %s", p->senhor_ano_glifo, p->senhor_ano_nome);

                wattroff(scroll_pad, par_cor_ativo | atributo_extra);

                wattron(scroll_pad, COLOR_PAIR(10) | A_DIM);
                mvwprintw(scroll_pad, row_pad + 1, 0, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
                wattroff(scroll_pad, COLOR_PAIR(10) | A_DIM);

                row_pad += 2;            
            }
        }

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, table_height - 9, 2, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));
        
        // 2. AJUSTE DOS CONTROLES DE RODAPÉ DA INTERFACE (UI)
        if (linhas_reais_pad > max_linhas_exibicao) {
            wattron(table_win, COLOR_PAIR(8) | A_BOLD);
            mvwprintw(table_win, table_height - 5, 4, "%s %d-%d %s %d",
                _("Showing"),
                scroll_offset / 2 + 1, 
                ((scroll_offset + max_linhas_exibicao) > qtd_profecoes * 2) ? qtd_profecoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                _("of"),
                qtd_profecoes                
            );
            wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
            mvwprintw(table_win, table_height - 3, 4, "%s", _("[↑/↓][PgUp/PgDn] Scroll"));  
                
        }
        
        // Rodapé descritivo focado em profecções tradicionais
        wattron(table_win, A_ITALIC);
        mvwprintw(table_win, table_height - 7, 4, _("Profection System: Sign-by-Sign (Traditional Wheel) ─ 1 Sign = 1 Year"));
        wattroff(table_win, A_ITALIC);

        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart ─ [P] Export to CSV"));

        int flag = 0;
        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
        wattron(table_win, COLOR_PAIR(28) | flag);
        desenhar_scrollbar(table_win, scroll_offset, linhas_reais_pad - 1, max_linhas_exibicao - 1, 6);
        wattroff(table_win, COLOR_PAIR(28) | flag);

        mvwaddstr(table_win, 6, table_width - 2, "▲");
        mvwaddstr(table_win, table_height - 9, table_width - 2, "▼");

        wnoutrefresh(table_win);

        int fim_y_recorte = start_y + 7 + max_linhas_exibicao - 2;
        if ((scroll_offset + max_linhas_exibicao) > linhas_reais_pad) {
            fim_y_recorte = start_y + 7 + (linhas_reais_pad - scroll_offset) - 1;
        }

        if (linhas_reais_pad > 0) {
            prefresh(scroll_pad, scroll_offset, 0, start_y + 7, start_x + 4, fim_y_recorte, start_x + table_width - 5);
        }
        doupdate();

        // 3. CAPTURA DO TECLADO SIMPLIFICADA PARA CONTROLES DE PROFEÇÃO
        int ch = wgetch(table_win);
        switch (ch) {
            case 'P':
            case 'p':
                exportar_profecoes_to_csv(cronograma_prof, qtd_profecoes);
                break;
            case KEY_DOWN:
                if (scroll_offset < (qtd_profecoes * 2 - max_linhas_exibicao)) {
                    scroll_offset += 2;
                }
                break;
            case KEY_UP:
                if (scroll_offset > 0) {
                    scroll_offset -= 2;
                }
                break;
            case KEY_NPAGE:
                if (scroll_offset < (qtd_profecoes * 2 - max_linhas_exibicao)) {
                    scroll_offset += max_linhas_exibicao;
                } else {
                    scroll_offset = qtd_profecoes * 2 - 1;
                }
                break;
            case KEY_PPAGE:
                if (scroll_offset >= 0) {
                    scroll_offset -= max_linhas_exibicao;
                    if (scroll_offset < 0) {
                        scroll_offset = 0;
                    }
                }
                break;
            case KEY_MOUSE: {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    int linha_clique_janela = event.y - getbegy(table_win);
                    int col_clique_janela = event.x - getbegx(table_win);
                    
                    int col_inicio_fechar = getmaxx(table_win) - 4;
                    int col_fim_fechar = col_inicio_fechar + 3; // '[X]'

                    if (linha_clique_janela == 0 && col_clique_janela >= col_inicio_fechar && col_clique_janela < col_fim_fechar) {
                        if (event.bstate & (BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED)) {
                            loop_interativo = 0;
                            break; 
                        }
                    }           
                    
                    int col_scrollbar_absoluta = getbegx(table_win) + (getmaxx(table_win) - 2);

                    if (event.x == col_scrollbar_absoluta) {
                        int linha_seta_cima = 6;
                        int linha_seta_baixo = 6 + max_linhas_exibicao; 
                        int offset_inicio_barra = 6 + 1; 
                        int altura_canal_barra = max_linhas_exibicao - 1;

                        // ADAPTAÇÃO: Teto de rolagem baseado em profecções
                        int max_scroll_y = (qtd_profecoes * 2) - max_linhas_exibicao;
                        if (max_scroll_y < 0) max_scroll_y = 0;

                        // ========================================================
                        // A: CLIQUE NA SETA PARA CIMA (▲)
                        // ========================================================
                        if (linha_clique_janela == linha_seta_cima) {
                            if (event.bstate & BUTTON1_PRESSED) {
                                nodelay(table_win, TRUE);
                                while (1) {
                                    if (scroll_offset > 1) {
                                        scroll_offset -= 2;
                                        
                                        desenhar_scrollbar(table_win, scroll_offset, linhas_reais_pad - 1, max_linhas_exibicao - 1, 6);
                                        if (linhas_reais_pad > max_linhas_exibicao) {
                                            wattron(table_win, COLOR_PAIR(8) | A_BOLD);
                                            mvwprintw(table_win, table_height - 5, 4, "%s %d-%d %s %d",
                                                _("Showing"),
                                                scroll_offset / 2 + 1, 
                                                ((scroll_offset + max_linhas_exibicao) > qtd_profecoes * 2) ? qtd_profecoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                                                _("of"),
                                                qtd_profecoes                
                                            );
                                            wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
                                            mvwprintw(table_win, table_height - 4, 4, "%s", _("[↑/↓][PgUp/PgDn] Scroll"));  
                                        }
                                        wnoutrefresh(table_win);
                                        prefresh(scroll_pad, scroll_offset, 0, start_y + 7, start_x + 4, fim_y_recorte, start_x + table_width - 5);
                                        doupdate();
                                    } else {
                                        scroll_offset = 0;
                                    }

                                    napms(125); 

                                    int ch_check = wgetch(table_win);
                                    if (ch_check == KEY_MOUSE) {
                                        MEVENT check_event;
                                        if (getmouse(&check_event) == OK && (check_event.bstate & BUTTON1_RELEASED)) {
                                            break;
                                        }
                                    }
                                }
                                nodelay(table_win, FALSE);
                            }
                        }
                        
                        // ========================================================
                        // B: CLIQUE NA SETA PARA BAIXO (▼)
                        // ========================================================
                        else if (linha_clique_janela == linha_seta_baixo) {
                            if (event.bstate & BUTTON1_PRESSED) {
                                nodelay(table_win, TRUE);
                                while (1) {
                                    if (scroll_offset < max_scroll_y - 1) {
                                        scroll_offset += 2;
                                        
                                        desenhar_scrollbar(table_win, scroll_offset, linhas_reais_pad - 1, max_linhas_exibicao - 1, 6);
                                        if (linhas_reais_pad > max_linhas_exibicao) {
                                            wattron(table_win, COLOR_PAIR(8) | A_BOLD);
                                            mvwprintw(table_win, table_height - 5, 4, "%s %d-%d %s %d",
                                                _("Showing"),
                                                scroll_offset / 2 + 1, 
                                                ((scroll_offset + max_linhas_exibicao) > qtd_profecoes * 2) ? qtd_profecoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                                                _("of"),
                                                qtd_profecoes                
                                            );
                                            wattroff(table_win, COLOR_PAIR(8) | A_BOLD);  
                                        }
                                        wnoutrefresh(table_win);
                                        prefresh(scroll_pad, scroll_offset, 0, start_y + 7, start_x + 4, fim_y_recorte, start_x + table_width - 5);
                                        doupdate();
                                    } else {
                                        scroll_offset = max_scroll_y;
                                    }

                                    napms(125);

                                    int ch_check = wgetch(table_win);
                                    if (ch_check == KEY_MOUSE) {
                                        MEVENT check_event;
                                        if (getmouse(&check_event) == OK && (check_event.bstate & BUTTON1_RELEASED)) {
                                            break;
                                        }
                                    }
                                }
                                nodelay(table_win, FALSE);
                            }
                        }
                        // ========================================================
                        // C: TRILHO DA BARRA DE ROLAGEM (CLIQUE E ARRASTO SEM SALTO)
                        // ========================================================
                        else if (event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED | REPORT_MOUSE_POSITION)) {
                            nodelay(table_win, TRUE);

                            // Calcula de forma reversa a posição visual exata do indicador do trilho
                            int linha_trilho_atual = 0;
                            if (max_scroll_y > 0 && (altura_canal_barra - 1) > 0) {
                                linha_trilho_atual = (scroll_offset * (altura_canal_barra - 1)) / max_scroll_y;
                            }
                            int linha_trilho_janela = linha_trilho_atual + offset_inicio_barra;

                            // Se clicou em cima do quadrado do trilho, ativa o arrasto relativo (sem saltos)
                            int modo_arrasto_trilho = (linha_clique_janela == linha_trilho_janela);
                            int linha_inicial_mouse = linha_clique_janela;
                            int scroll_offset_original = scroll_offset;

                            while (1) {
                                int novo_offset = scroll_offset;
                                int linha_clique_barra = linha_clique_janela - offset_inicio_barra;

                                if (modo_arrasto_trilho) {
                                    // Arrasto relativo baseado no movimento da mão do usuário
                                    int delta_mouse = linha_clique_janela - linha_inicial_mouse;
                                    if ((altura_canal_barra - 1) > 0 && max_scroll_y > 0) {
                                        int delta_scroll = (delta_mouse * max_scroll_y) / (altura_canal_barra - 1);
                                        novo_offset = scroll_offset_original + delta_scroll;
                                    }
                                } else {
                                    // Clique absoluto fora do trilho (teleporte rápido)
                                    if (linha_clique_barra >= 0 && linha_clique_barra < altura_canal_barra) {
                                        if ((altura_canal_barra - 1) > 0 && max_scroll_y > 0) {
                                            novo_offset = (linha_clique_barra * max_scroll_y) / (altura_canal_barra - 1);
                                        }
                                    }
                                }

                                // ARREDONDAMENTO PAR OBRIGATÓRIO: Garante que os pulos sejam sempre múltiplos de 2
                                novo_offset = (novo_offset / 2) * 2;

                                if (novo_offset < 0) novo_offset = 0;
                                if (novo_offset > max_scroll_y) novo_offset = max_scroll_y;

                                if (scroll_offset != novo_offset) {
                                    scroll_offset = novo_offset;
                                    
                                    desenhar_scrollbar(table_win, scroll_offset, linhas_reais_pad - 1, max_linhas_exibicao - 1, 6);
                                    if (linhas_reais_pad > max_linhas_exibicao) {
                                        wattron(table_win, COLOR_PAIR(8) | A_BOLD);
                                        mvwprintw(table_win, table_height - 5, 4, "%s %d-%d %s %d",
                                            _("Showing"),
                                            scroll_offset / 2 + 1, 
                                            ((scroll_offset + max_linhas_exibicao) > qtd_profecoes * 2) ? qtd_profecoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                                            _("of"),
                                            qtd_profecoes                
                                        );
                                        wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
                                                                                    
                                    }
                                    wnoutrefresh(table_win);
                                    prefresh(scroll_pad, scroll_offset, 0, start_y + 7, start_x + 4, fim_y_recorte, start_x + table_width - 5);
                                    doupdate();
                                }

                                // Se foi um clique rápido simples fora do trilho, quebra imediatamente
                                if ((event.bstate & BUTTON1_CLICKED) && !modo_arrasto_trilho) {
                                    break;
                                }

                                napms(10); // Rastreamento fluido do arrasto

                                int ch_check = wgetch(table_win);
                                if (ch_check == KEY_MOUSE) {
                                    MEVENT check_event;
                                    if (getmouse(&check_event) == OK) {
                                        if (check_event.bstate & BUTTON1_RELEASED) {
                                            break;
                                        }
                                        linha_clique_janela = check_event.y - getbegy(table_win);
                                    }
                                } else if (ch_check != ERR) {
                                    ungetch(ch_check);
                                    break;
                                }
                            }
                            nodelay(table_win, FALSE);
                        }
                    }
                }
                break;
            }    
    
            case 27:
            case 'q':
            case 'Q':
                loop_interativo = 0;
                break;
        }
    }
    
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr);
    refresh();

    drag_and_drop_off();
}



DadosProfeccao calcular_profeccao_anual(double asc_longitude, int idade_atual) {
    DadosProfeccao prof = {0, 0, 0, "", ""};
    
    // Descobre o Signo do Ascendente Natal (Base 1 a 12 usando sua fórmula floor + 1)
    int signo_asc_natal = (int)floor(asc_longitude / 30.0) + 1;
    
    // A Casa ativada avança 1 por ano a partir da Casa 1 (Modular 12)
    prof.casa_ativada = (idade_atual % 12) + 1;
    
    // O Signo ativado avança 1 por ano a partir do Signo do ASC Natal
    prof.signo_ativado = (signo_asc_natal - 1 + idade_atual) % 12 + 1;
    
    // Determinar o Senhor do Ano (Lord of the Year) baseado nas regências clássicas de 1 a 7
    switch (prof.signo_ativado) {
        case 1:  case 8:  prof.id_senhor_do_ano = 5; strcpy(prof.glifo_senhor, "♂"); strcpy(prof.nome_senhor, _("Mars")); break;
        case 2:  case 7:  prof.id_senhor_do_ano = 4; strcpy(prof.glifo_senhor, "♀"); strcpy(prof.nome_senhor, _("Venus")); break;
        case 3:  case 6:  prof.id_senhor_do_ano = 3; strcpy(prof.glifo_senhor, "☿"); strcpy(prof.nome_senhor, _("Mercury")); break;
        case 4:           prof.id_senhor_do_ano = 2; strcpy(prof.glifo_senhor, "☾"); strcpy(prof.nome_senhor, _("Moon")); break;
        case 5:           prof.id_senhor_do_ano = 1; strcpy(prof.glifo_senhor, "☉"); strcpy(prof.nome_senhor, _("Sun")); break;
        case 9:  case 12: prof.id_senhor_do_ano = 6; strcpy(prof.glifo_senhor, "♃"); strcpy(prof.nome_senhor, _("Jupiter")); break;
        case 10: case 11: prof.id_senhor_do_ano = 7; strcpy(prof.glifo_senhor, "♄"); strcpy(prof.nome_senhor, _("Saturn")); break;
    }
    
    return prof;
}



void display_profections(PlotObject *plots, int anos_alcochoden, double *cusps, ChartObject *obj, int num_objects, double jd) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int idade_padrao = obter_idade_padrao_mapa();

    int idade = selecionar_idade_visual(idade_padrao); // Abre o seletor visual e sugere 40 inicialmente
    if (idade == -1) {
        touchwin(stdscr); // Se o usuário deu ESC, limpa e retorna
        refresh();
        return;
    }

    // ────────────────────────────────────────────────────────────────────────
    // JANELA 2: EXIBIÇÃO DO RELATÓRIO DE PROFEÇÃO ANUAL
    // ────────────────────────────────────────────────────────────────────────
    int table_height = 30;
    int table_width = max_x - 10;
    int start_y = (max_y - table_height) / 2;
    int start_x = (max_x - table_width) / 2;;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13) | FLAGS);
    
    wattron(table_win, A_BOLD);
    const char *title = _("Annual Profections Panel");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);

    // 2. Desenha o botão [X] no canto superior direito
    int col_fechar = getmaxx(table_win) - 4; // Abre espaço para 3 caracteres: '[', 'X', ']'

    wattron(table_win, COLOR_PAIR(13)); // Cor padrão para os colchetes
    mvwprintw(table_win, 0, col_fechar, "[");
    mvwprintw(table_win, 0, col_fechar + 2, "]");
    wattroff(table_win, COLOR_PAIR(13));

    wattron(table_win, COLOR_PAIR(13) | A_BOLD); // Cor de destaque (ex: Vermelho) para o X
    mvwprintw(table_win, 0, col_fechar + 1, "✖");
    wattroff(table_win, COLOR_PAIR(13) | A_BOLD);

    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED, NULL);
    mouseinterval(100);


    // Encontra a longitude do Ascendente Natal varrendo o array plots
    double asc_lon = 0;
    int object_diff = show_modern_planets ? 0 : 3;
    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        if (plots[i].id == P_ASC - object_diff) {
            asc_lon = plots[i].longitude;
            break;
        }
    }

    // Executa a nossa engine matemática
    DadosProfeccao prof = calcular_profeccao_anual(asc_lon, idade);

    // --- IMPRESSÃO DOS RESULTADOS NA TELA ---
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 2, 4, "Target Age Analyzed: ");
    wattron(table_win, A_BOLD | COLOR_PAIR(15));
    wprintw(table_win, "%d years old", idade);
    wattroff(table_win, A_BOLD | COLOR_PAIR(15));

    // CRÍTICO: Checa se a idade atual coincide com o ciclo de pulsação do seu Alcochoden!
    if (anos_alcochoden > 0 && (idade % anos_alcochoden == 0)) {
        wattron(table_win, COLOR_PAIR(11) | A_BOLD | A_BLINK);
        wprintw(table_win, _("  [CRITICAL ALCOCHODEN RETURN YEAR!]"));
        wattroff(table_win, COLOR_PAIR(11) | A_BOLD | A_BLINK);
    }

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 4, 2, "─────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    // Exibe a Casa e o Signo Natal Ativados
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 5, 4, _("ACTIVATED NATAL HOUSE: "));
    wprintw(table_win, "%s %d", _("House"), prof.casa_ativada);
    wattroff(table_win, A_BOLD);
    wprintw(table_win, _(" (Focusing your natal experiences into this specific area of life)"));

    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 7, 4, _("PROFECTED ZODIAC SIGN: "));
    wattron(table_win, A_BOLD | COLOR_PAIR(8));
    wprintw(table_win, "%s", get_sign(prof.signo_ativado - 1)); // get_sign usa base 0 (0=Áries)
    wattroff(table_win, A_BOLD | COLOR_PAIR(8));

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 9, 2, "─────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    // Exibe o Senhor do Ano (Lord of the Year)
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 11, 4, _("LORD OF THE YEAR (Chronocrator): "));
    wattron(table_win, A_BOLD | COLOR_PAIR(11));
    wprintw(table_win, " %s ", prof.glifo_senhor);
    wattroff(table_win, A_BOLD | COLOR_PAIR(11));
    wprintw(table_win, " (%s - %s)", prof.nome_senhor, _("This planet rules your entire year and all transits to it"));

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 13, 2, "─────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    // --- BLOCO INFERIOR: PLANETAS NATAIS QUE FORAM ACORDADOS NESSA CASA ---
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 15, 4, _("NATAL PLANETS WOKEN UP IN THIS HOUSE:"));
    wattroff(table_win, A_BOLD);
    int row_p = 17;
    int planetas_encontrados = 0;

    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        // Ignora pontos extras vazios e checa se a casa do planeta bate com a casa acordada
        if (romanToInt(plots[i].house) == prof.casa_ativada && i < P_ASC - object_diff) { 
            if (i < 10 - object_diff) {
                mvwprintw(table_win, row_p, 6, "• %s ", _("Planet"));
            }
            else {
                mvwprintw(table_win, row_p, 6, "• ");
            }
            wattron(table_win, COLOR_PAIR(8) | A_BOLD);
            wprintw(table_win, "%s (%s) ", plots[i].object, plots[i].object_name); // Imprime o glifo direto (.object)
            wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
            wprintw(table_win, "%s", _("is natally placed here and is now triggered.")); // Imprime o nome (.name)
            
            row_p++;
            planetas_encontrados++;
        }
    }

    if (planetas_encontrados == 0) {
        wattron(table_win, A_DIM);
        mvwprintw(table_win, 17, 6, "%s %d. %s", _("No natal planets placed in House"), prof.casa_ativada, _("The Lord of the Year acts alone."));
        wattroff(table_win, A_DIM);
    }

    ArabicPartCalculada lista[MAX_PARTS];
    memset(lista, 0, sizeof(lista));

    int qtd_partes = load_and_calculate_arabic_parts(obj, num_objects, cusps, lista);
    
    int partes_encontradas = 0;

    for (int i = 0; i < qtd_partes; i++) {
        if (romanToInt(lista[i].house) == prof.casa_ativada) { 
            mvwprintw(table_win, row_p, 6, "• %s ", _("Part"));
            
            wattron(table_win, COLOR_PAIR(8) | A_BOLD);
            wprintw(table_win, "%s ", lista[i].name);
            wattroff(table_win, COLOR_PAIR(8) | A_BOLD);

            wprintw(table_win, "%s", _("is placed here in your natal chart and is now triggered.")); // Imprime o nome (.name)
                        
            row_p++;
            partes_encontradas++;
        }
    }

    if (partes_encontradas == 0) {
        wattron(table_win, A_DIM);
        mvwprintw(table_win, row_p, 6, "%s %d.", _("No natal parts placed in House"), prof.casa_ativada);
        wattroff(table_win, A_DIM);
    }

    mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart ─ F2 to Profections Table"));
    wnoutrefresh(table_win);

    doupdate();

    keypad(table_win, TRUE);
    nodelay(table_win, FALSE);
    int ch;
    do {
        ch = wgetch(table_win);
        if (ch == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                // Coordenadas do clique convertidas para o plano local da janela
                int linha_clique_janela = event.y - getbegy(table_win);
                int col_clique_janela = event.x - getbegx(table_win);
                
                // Define matematicamente a caixa de clique do botão fechar
                int col_inicio_fechar = getmaxx(table_win) - 4;
                int col_fim_fechar = col_inicio_fechar + 3; // Abrange '[X]'

                // ========================================================
                // NOVO ROTEAMENTO: O clique acertou o botão [X]?
                // ========================================================
                if (linha_clique_janela == 0 && col_clique_janela >= col_inicio_fechar && col_clique_janela < col_fim_fechar) {
                    if (event.bstate & (BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED)) {
                        break;
                    }
                }                                
            }

        }
        else if (ch == KEY_F(2)) {
            LinhaProfecao tabela[100];
            gerar_tabela_profecoes_anuais(tabela, 121, cusps[1], jd);
            exibir_janela_tabela_profecoes(tabela, 121, idade);
        }
    } while (ch != 27 && ch != 'q');
    
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr);
    refresh();
}
