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
#include "var.h"
#include "helper.h"
#include "draw-chart.h"
#include "planet_table.h"
#include "db-utils.h"
#include "profections.h"



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
        case 4:           prof.id_senhor_do_ano = 2; strcpy(prof.glifo_senhor, "☽"); strcpy(prof.nome_senhor, _("Moon")); break;
        case 5:           prof.id_senhor_do_ano = 1; strcpy(prof.glifo_senhor, "☉"); strcpy(prof.nome_senhor, _("Sun")); break;
        case 9:  case 12: prof.id_senhor_do_ano = 6; strcpy(prof.glifo_senhor, "♃"); strcpy(prof.nome_senhor, _("Jupiter")); break;
        case 10: case 11: prof.id_senhor_do_ano = 7; strcpy(prof.glifo_senhor, "♄"); strcpy(prof.nome_senhor, _("Saturn")); break;
    }
    
    return prof;
}



void display_profections(PlotObject *plots, int anos_alcochoden, double *cusps, ChartObject *obj, int num_objects) {
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
    mvwprintw(table_win, 0, col_fechar + 1, "X");
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

    // Rodapé padrão
    mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
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
    } while (ch != 27 && ch != 'q');
    
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr);
    refresh();
}
