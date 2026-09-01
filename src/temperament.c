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
#include "arabic_parts.h"
#include "helper.h"
#include "draw-chart.h"
#include "planet_table.h"
#include "db-utils.h"
#include "temperament.h"
#include "helper.h"
#include "almuten.h"

const char *water_ascii[] = {
    "        ",
    " ▜▀▀▀▜▘ ",
    "  ▚ ▗▘  ",
    "   ▚▘   ",
    "        "
};

const char *earth_ascii[] = {
    "        ",
    " ▜▀▀▀▜▘ ",
    "  ▜▀▜▘  ",
    "   ▚▘   ",
    "        "
};

const char *air_ascii[] = {
    "        ",
    "   ▞▚   ",
    "  ▟▄▄▙  ",
    " ▟▄▄▄▄▙ ",
    "        "
};

const char *fire_ascii[] = {
    "        ",
    "   ▞▚   ",
    "  ▞  ▚  ",
    " ▟▄▄▄▄▙ ",
    "        "
};



const char **get_element_ascii_by_name(char *name) {

    if (strcmp(name, _("Fire")) == 0) return fire_ascii;
    if (strcmp(name, _("Water")) == 0) return water_ascii;
    if (strcmp(name, _("Air")) == 0) return air_ascii;
    if (strcmp(name, _("Earth")) == 0) return earth_ascii;

    return NULL;
}



// Função auxiliar para desenhar a barra gráfica baseada em porcentagem (0 a 100%)
void desenhar_barra_porcentagem(WINDOW *win, int row, int col, float porcentagem, int cor_par) {
    int max_barra_width = 40; // Mantém o mesmo tamanho padrão de 30 caracteres
    
    // Calcula o número de blocos proporcional a 100%
    int num_blocos = (int)((porcentagem * max_barra_width) / 100.0f);
    if (num_blocos > max_barra_width) num_blocos = max_barra_width;
    if (num_blocos < 0) num_blocos = 0;
    
    // Imprime o valor numérico formatado com uma casa decimal antes da barra
    mvwprintw(win, row, col, "[%5.1f%%] ", porcentagem);
    
    // Desenha os blocos preenchidos da barra
    wattron(win, COLOR_PAIR(cor_par) | A_DIM | A_UNDERLINE | FLAGS);
    for (int i = 0; i < num_blocos; i++) {
        wprintw(win, "█");
    }
    wattroff(win, COLOR_PAIR(cor_par) | A_DIM | FLAGS);
    
    // Desenha o fundo residual da barra (vazio)
    wattron(win, COLOR_PAIR(10) | A_DIM | FLAGS);
    for (int i = num_blocos; i < max_barra_width; i++) {
        wprintw(win, "░");
    }
    wattroff(win, COLOR_PAIR(10) | A_DIM | A_UNDERLINE);
}



// Função auxiliar para desenhar uma barra gráfica horizontal na interface
void desenhar_barra_temperamento(WINDOW *win, int row, int col, int valor, int total, int cor_par) {
    int max_barra_width = 30; // Tamanho máximo da barra em caracteres
    int num_blocos = 0;
    
    if (total > 0) {
        num_blocos = (valor * max_barra_width) / total;
    }
    
    // Imprime o valor numérico antes da barra
    mvwprintw(win, row, col, "[%02d] ", valor);
    
    // Liga a cor correspondente e desenha os blocos
    wattron(win, COLOR_PAIR(cor_par) | A_DIM | A_UNDERLINE | FLAGS);
    for (int i = 0; i < num_blocos; i++) {
        wprintw(win, "█");
    }
    // Fundo da barra
    wattroff(win, COLOR_PAIR(cor_par) | A_DIM | FLAGS);
    wattron(win, COLOR_PAIR(10) | A_DIM | FLAGS);
    for (int i = num_blocos; i < max_barra_width; i++) {
        wprintw(win, "░");
    }
    wattroff(win, COLOR_PAIR(10) | A_DIM | A_UNDERLINE | FLAGS);
}

void display_temperament(PlotObject *plots, AspectMatrix *aspecto_matrix, int fase_lunar, int estacao, int week_day, int planetary_hour) {
    // 1. EXTRAÇÃO DE COORDENADAS E IDs INICIAIS
    double sol_lon = 0, lua_lon = 0, asc_lon = 0, fortuna_lon, san_lon;
    int id_regente_asc = 0; // Usado mais abaixo na função que encontra o regente clássico do ASC
    int id_regente_lua = 0;

    int object_diff = show_modern_planets ? 0 : 3;
    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        if (plots[i].id == P_SOL) {
            sol_lon = plots[i].longitude; 
        }
        if (plots[i].id == P_LUNA) {
            lua_lon = plots[i].longitude;
        }
        if (plots[i].id == P_ASC - object_diff) {
            asc_lon = plots[i].longitude;
        }
        if (plots[i].id == P_FORTUNA - object_diff) {
            fortuna_lon = plots[i].longitude;
        }
        if (plots[i].id == P_SAN - object_diff) {
            san_lon = plots[i].longitude;
        }
    }

    // 2. CÁLCULO INTEGRADO DOS ALMUTENS
    // Prepara a struct de pontos hylegíacos para gerar os almutens
    PontosHylegiacos pontos_hyl;
    pontos_hyl.sol_lon = sol_lon;
    pontos_hyl.lua_lon = lua_lon;
    pontos_hyl.asc_lon = asc_lon;
    pontos_hyl.fortuna_lon = fortuna_lon; 
    pontos_hyl.sizigia_lon = san_lon;

    //int res_almuten_lua[12] = {0};
    //int qtd_alm_lua = get_almuten(lua_lon, res_almuten_lua, aspecto_matrix, pontos_hyl, plots);

    int res_figuris[12] = {0};
    int qtd_alm_fig = calcular_almuten_figuris(pontos_hyl, plots, aspecto_matrix, converter_codigo_planeta(get_hour_regent(week_day - 1, (MAPA_DIURNO)?0:12)), converter_codigo_planeta(get_hour_regent(week_day - 1, planetary_hour - 1)), res_figuris);

    // 3. PROCESSAMENTO DOS SCORES DO TEMPERAMENTO VIA SQLITE
    ScoreTemperament score = {0, 0, 0, 0};
    PrimitiveProperties prop;

    // Ponto 1: Signo do Ascendente (Ajuste +1)
    int signo_asc = (int)floor(asc_lon / 30.0) + 1;
    if (get_sign_properties(signo_asc, &prop)) {
        // Se for positivo pende para o Calor, se for negativo pende para o Frio
        if (prop.temperature > 0) score.total_quente += prop.temperature; 
        else if (prop.temperature < 0) score.total_frio += abs(prop.temperature);
        
        // Se for positivo pende para a Umidade, se for negativo pende para a Secura
        if (prop.moisture > 0) score.total_umido += prop.moisture; 
        else if (prop.moisture < 0) score.total_seco += abs(prop.moisture);
    }

    int n_exalted, n_exile, n_fall, n_tri1, n_tri2, n_tri3;
    get_rulers_by_sign_id(signo_asc, &id_regente_asc, &n_exalted, &n_exile, &n_fall, &n_tri1, &n_tri2, &n_tri3);

    int lon_ruler_asc = plots[id_regente_asc - 1].longitude;
    int sign_ruler_asc = (int)floor(lon_ruler_asc / 30) + 1;

    // Ponto 2: Signo do Planeta Regente do Ascendente
    if (get_sign_properties(sign_ruler_asc, &prop)) {
        if (prop.temperature > 0) score.total_quente += prop.temperature; 
        else if (prop.temperature < 0) score.total_frio += abs(prop.temperature);
        
        if (prop.moisture > 0) score.total_umido += prop.moisture; 
        else if (prop.moisture < 0) score.total_seco += abs(prop.moisture);
    }

    // Ponto 3: Planetas na Casa 1
    int planeta_na_casa1[13] = {0};
    
    for (int i = 0; i < 12 - object_diff; i++) {
        if (romanToInt(plots[i].house) == 1) {
            int id_atual = 0; // Variável para armazenar o ID calculado e marcar no controle

            if (i < 7) {
                id_atual = plots[i].id + 1; // Calcula o ID na escala 1-12

                // Registra o ID no array de controle
                if (id_atual >= 1 && id_atual <= 12) {
                    planeta_na_casa1[id_atual] = 1;
                }

                // se o plabeta for o Sol, pega as propriedades da estação do ano
                if (id_atual == 1) {
                    if (estacao == 1) { score.total_quente += 1; score.total_umido += 1; } // Primavera
                    else if (estacao == 2) { score.total_quente += 1; score.total_seco += 1;  } // Verão
                    else if (estacao == 3) { score.total_frio += 1;   score.total_seco += 1;  } // Outono
                    else if (estacao == 4) { score.total_frio += 1;   score.total_umido += 1; } // Inverno
                }
                else if (get_planet_properties(id_atual, &prop)) {
                    if (prop.temperature > 0) score.total_quente += prop.temperature; 
                    else if (prop.temperature < 0) score.total_frio += abs(prop.temperature);
                    
                    if (prop.moisture > 0) score.total_umido += prop.moisture; 
                    else if (prop.moisture < 0) score.total_seco += abs(prop.moisture);
                }
            }
            else {
                id_atual = plots[i].id + 1 + object_diff; // Calcula o ID pulando os modernos se necessário

                // Registra o ID no array de controle
                if (id_atual >= 1 && id_atual <= 12) {
                    planeta_na_casa1[id_atual] = 1;
                }

                if (get_planet_properties(id_atual, &prop)) {
                    if (prop.temperature > 0) score.total_quente += prop.temperature; 
                    else if (prop.temperature < 0) score.total_frio += abs(prop.temperature);
                    
                    if (prop.moisture > 0) score.total_umido += prop.moisture; 
                    else if (prop.moisture < 0) score.total_seco += abs(prop.moisture);
                }
            }
        }
    }



    // --- ADICIONAL 3: ASPECTOS NA LUA E NO ASCENDENTE (PLANETAS ±1 | NODOS APENAS ☌) ---
    int idx_lua = -1;
    int idx_asc = -1;

    // 1. Descobre os índices internos da Lua e do Ascendente no array plots/grid
    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        if (plots[i].id == P_LUNA) {
            idx_lua = i;
        }
        if (plots[i].id == P_ASC - object_diff) {
            idx_asc = i;
        }
    }

    // 2. Processa os aspectos (Varrendo as duas direções da matriz para cada objeto)

    // Ponto 4: Planetas em conjunção com a Lua e signo dos planetas que aspectam a Lua
    for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
        
        int id_planeta_aspectante = j + 1;
        if (j >= 7) {
            id_planeta_aspectante = j + 1 + object_diff;    
        }
        
        if (id_planeta_aspectante < 1 || id_planeta_aspectante > 12) {
            continue; 
        }

        // --- ASPECTOS NA LUA ---
        if (idx_lua != -1 && j != idx_lua) {
            AspectCell celula_lua_1 = aspecto_matrix->grid[j][idx_lua];
            AspectCell celula_lua_2 = aspecto_matrix->grid[idx_lua][j];

            if (celula_lua_1.has_aspect || celula_lua_2.has_aspect) {
                bool aspecto_valido = false;
                bool is_conjunction = false;

                // NOVA REGRA: Se for Nodo (11 ou 12), aceita estritamente apenas CONJUNÇÃO (☌)
                if (id_planeta_aspectante >= 11) {
                    if (strstr(celula_lua_1.symbol, "☌") != NULL || strstr(celula_lua_2.symbol, "☌") != NULL) {
                        aspecto_valido = true;
                        is_conjunction = true;
                    }
                } 
                // Se for planeta normal (1 a 10), aceita todos os aspectos maiores
                else {
                    if (strstr(celula_lua_1.symbol, "☌") != NULL || strstr(celula_lua_2.symbol, "☌") != NULL ||
                        strstr(celula_lua_1.symbol, "☍") != NULL || strstr(celula_lua_2.symbol, "☍") != NULL ||
                        strstr(celula_lua_1.symbol, "□") != NULL || strstr(celula_lua_2.symbol, "□") != NULL ||
                        strstr(celula_lua_1.symbol, "△") != NULL || strstr(celula_lua_2.symbol, "△") != NULL ||
                        strstr(celula_lua_1.symbol, "⚹") != NULL || strstr(celula_lua_2.symbol, "⚹") != NULL) 
                    {
                        aspecto_valido = true;
                        if (strstr(celula_lua_1.symbol, "☌") != NULL || strstr(celula_lua_2.symbol, "☌") != NULL) {
                            is_conjunction = true;
                        }
                    }
                }

                if (aspecto_valido) {
                    PrimitiveProperties prop_asp;
                    if (is_conjunction) {
                        if (get_planet_properties(id_planeta_aspectante, &prop_asp)) {
                            if (prop_asp.temperature > 0)      score.total_quente += 1;
                            else if (prop_asp.temperature < 0) score.total_frio += 1;

                            if (prop_asp.moisture > 0)         score.total_umido += 1;
                            else if (prop_asp.moisture < 0)    score.total_seco += 1;
                        }
                    }
                    else {
                        int lon_ruler_aspectante = plots[id_planeta_aspectante - 1].longitude;
                        int sign_ruler_aspectante = (int)floor(lon_ruler_aspectante / 30) + 1;
                        if (get_sign_properties(sign_ruler_aspectante, &prop_asp)) {
                            if (prop_asp.temperature > 0)      score.total_quente += 1;
                            else if (prop_asp.temperature < 0) score.total_frio += 1;

                            if (prop_asp.moisture > 0)         score.total_umido += 1;
                            else if (prop_asp.moisture < 0)    score.total_seco += 1;
                        }

                    }
                }
            }
        }

        // Ponto 5: Planetas em conjunção com o Ascendente e signo dos planetas que aspectam o Ascendente
        // --- ASPECTOS NO ASCENDENTE ---
        if (idx_asc != -1 && j != idx_asc) {

            if (id_planeta_aspectante >= 1 && id_planeta_aspectante <= 12) {
                continue; // Pula para o próximo planeta do laço 'j'
            }

            AspectCell celula_asc_1 = aspecto_matrix->grid[j][idx_asc];
            AspectCell celula_asc_2 = aspecto_matrix->grid[idx_asc][j];

            if (celula_asc_1.has_aspect || celula_asc_2.has_aspect) {
                bool aspecto_valido = false;
                bool is_conjunction = false;

                // NOVA REGRA: Mesma restrição de conjunção estrita para os Nodos no Ascendente
                if (id_planeta_aspectante >= 11) {
                    if (strstr(celula_asc_1.symbol, "☌") != NULL || strstr(celula_asc_2.symbol, "☌") != NULL) {
                        aspecto_valido = true;
                        is_conjunction = true;
                    }
                } 
                else {
                    if (strstr(celula_asc_1.symbol, "☌") != NULL || strstr(celula_asc_2.symbol, "☌") != NULL ||
                        strstr(celula_asc_1.symbol, "☍") != NULL || strstr(celula_asc_2.symbol, "☍") != NULL ||
                        strstr(celula_asc_1.symbol, "□") != NULL || strstr(celula_asc_2.symbol, "□") != NULL ||
                        strstr(celula_asc_1.symbol, "△") != NULL || strstr(celula_asc_2.symbol, "△") != NULL ||
                        strstr(celula_asc_1.symbol, "⚹") != NULL || strstr(celula_asc_2.symbol, "⚹") != NULL) 
                    {
                        aspecto_valido = true;
                        if (strstr(celula_asc_1.symbol, "☌") != NULL || strstr(celula_asc_2.symbol, "☌") != NULL) {
                            is_conjunction = true;
                        }
                    }
                }

                if (aspecto_valido) {
                    PrimitiveProperties prop_asp;

                    // se o planeta for o Sol, se for conjunção e se ele não estiver na casa 1, pega a estação do ano
                    if (id_planeta_aspectante == 1 && is_conjunction && !planeta_na_casa1[id_planeta_aspectante]) {

                        if (estacao == 1) { score.total_quente += 1; score.total_umido += 1; } // Primavera
                        else if (estacao == 2) { score.total_quente += 1; score.total_seco += 1;  } // Verão
                        else if (estacao == 3) { score.total_frio += 1;   score.total_seco += 1;  } // Outono
                        else if (estacao == 4) { score.total_frio += 1;   score.total_umido += 1; } // Inverno
                    }
                    // se for conjunção, se não for o Sol e se planeta não estiver na casa 1, toma-se suas propriedades
                    else if (is_conjunction && !planeta_na_casa1[id_planeta_aspectante]) {

                        if (get_planet_properties(id_planeta_aspectante, &prop_asp)) {
                            if (prop_asp.temperature > 0)      score.total_quente += 1;
                            else if (prop_asp.temperature < 0) score.total_frio += 1;

                            if (prop_asp.moisture > 0)         score.total_umido += 1;
                            else if (prop_asp.moisture < 0)    score.total_seco += 1;
                        }
                    }
                    // se planeta, inclusive o sol, não estiver na casa 1 e se for qualquer outro aspecto, pega o signo
                    else if (!planeta_na_casa1[id_planeta_aspectante]) {

                        int lon_ruler_aspectante = plots[id_planeta_aspectante - 1].longitude;
                        int sign_ruler_aspectante = (int)floor(lon_ruler_aspectante / 30) + 1;
                        if (get_sign_properties(sign_ruler_aspectante, &prop_asp)) {
                            if (prop_asp.temperature > 0)      score.total_quente += 1;
                            else if (prop_asp.temperature < 0) score.total_frio += 1;

                            if (prop_asp.moisture > 0)         score.total_umido += 1;
                            else if (prop_asp.moisture < 0)    score.total_seco += 1;
                        }

                    }
                }
            }
        }
    }
    

    // Ponto 6: Signo da Lua
    int signo_lua = (int)floor(lua_lon / 30.0) + 1;
    if (get_sign_properties(signo_lua, &prop)) {
        if (prop.temperature > 0) score.total_quente += prop.temperature; 
        else if (prop.temperature < 0) score.total_frio += abs(prop.temperature);
        
        if (prop.moisture > 0) score.total_umido += prop.moisture; 
        else if (prop.moisture < 0) score.total_seco += abs(prop.moisture);
    }

    // Ponto 7: Signo do Regente da Lua

    get_rulers_by_sign_id(signo_lua, &id_regente_lua, &n_exalted, &n_exile, &n_fall, &n_tri1, &n_tri2, &n_tri3);
    int lon_ruler_lua = plots[id_regente_lua - 1].longitude;
    int sign_ruler_lua = (int)floor(lon_ruler_lua / 30) + 1;

    // for (int i = 0; i < qtd_alm_lua; i++) {
    //     int lon_ruler_lua = plots[res_almuten_lua[i] - 1].longitude;
    //     int sign_ruler_lua = (int)floor(lon_ruler_lua / 30) + 1;

    if (get_sign_properties(sign_ruler_lua, &prop)) {
        if (prop.temperature > 0) score.total_quente += prop.temperature; 
        else if (prop.temperature < 0) score.total_frio += abs(prop.temperature);
        
        if (prop.moisture > 0) score.total_umido += prop.moisture; 
        else if (prop.moisture < 0) score.total_seco += abs(prop.moisture);
    }
    //}

    // Ponto 8: Almuten Figuris e seu signo se as propriedades não forem iguais
    for (int i = 0; i < qtd_alm_fig; i++) {
        PrimitiveProperties prop_alm;
        if (get_planet_properties(res_figuris[i], &prop_alm)) {
            if (prop_alm.temperature > 0) score.total_quente += prop_alm.temperature; 
            else if (prop_alm.temperature < 0) score.total_frio += abs(prop_alm.temperature);
            
            if (prop_alm.moisture > 0) score.total_umido += prop_alm.moisture; 
            else if (prop_alm.moisture < 0) score.total_seco += abs(prop_alm.moisture);
        }

        int lon_ruler_alm = plots[res_figuris[i] - 1].longitude;
        int sign_ruler_alm = (int)floor(lon_ruler_alm / 30) + 1;

        if (get_sign_properties(sign_ruler_alm, &prop)) {
            if (prop_alm.moisture != prop.moisture || prop_alm.temperature != prop.temperature) {

                if (prop.temperature > 0) score.total_quente += prop.temperature; 
                else if (prop.temperature < 0) score.total_frio += abs(prop.temperature);
                
                if (prop.moisture > 0) score.total_umido += prop.moisture; 
                else if (prop.moisture < 0) score.total_seco += abs(prop.moisture);
            }
        }
    }

    // Ponto 9: Fase Lunar (Mantém peso +1 para manter o equilíbrio com os planetas)
    if (fase_lunar == 1) { score.total_quente += 1; score.total_umido += 1; } // Nova
    else if (fase_lunar == 2) { score.total_quente += 1; score.total_seco += 1;  } // Crescente
    else if (fase_lunar == 3) { score.total_frio += 1;   score.total_seco += 1;  } // Cheia
    else if (fase_lunar == 4) { score.total_frio += 1;   score.total_umido += 1; } // Minguante

    // Ponto 10: Estação do Ano (Mantém peso +1)
    if (estacao == 1) { score.total_quente += 1; score.total_umido += 1; } // Primavera
    else if (estacao == 2) { score.total_quente += 1; score.total_seco += 1;  } // Verão
    else if (estacao == 3) { score.total_frio += 1;   score.total_seco += 1;  } // Outono
    else if (estacao == 4) { score.total_frio += 1;   score.total_umido += 1; } // Inverno

    // Totais para cálculo proporcional das barras (A matemática continua perfeitamente igual aqui)
    int total_eixo_temp = score.total_quente + score.total_frio;
    int total_eixo_moist = score.total_umido + score.total_seco;

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 30;
    int table_width = max_x - 10;
    int start_y = (max_y - table_height) / 2;
    int start_x = 5;
    
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
    const char *title = _("Natal Temperament Analysis");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
    wattroff(table_win, A_BOLD);

    // DESENHO DAS BARRAS GRÁFICAS
    int col_bars = 16;
    mvwprintw(table_win, 3, 4, _("Hot:"));
    desenhar_barra_temperamento(table_win, 3, col_bars, score.total_quente, total_eixo_temp, 11); // Vermelho/Laranja

    mvwprintw(table_win, 5, 4, _("Cold:  "));
    desenhar_barra_temperamento(table_win, 5, col_bars, score.total_frio, total_eixo_temp, 8);   // Azul

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 7, 4, "────────────────────────────────────────────────────────────────────────────────────");
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    mvwprintw(table_win, 9, 4, _("Moist: "));
    desenhar_barra_temperamento(table_win, 9, col_bars, score.total_umido, total_eixo_moist, 12);  // Ciano/Verde

    mvwprintw(table_win, 11, 4, _("Dry:  "));
    desenhar_barra_temperamento(table_win, 11, col_bars, score.total_seco, total_eixo_moist, 25);  // Amarelo

    wattron(table_win, COLOR_PAIR(13));
    mvwprintw(table_win, 13, 4, "────────────────────────────────────────────────────────────────────────────────────");
    wattroff(table_win, COLOR_PAIR(13));

    // --- CÁLCULO CIENTÍFICO DAS PORCENTAGENS DOS 4 TEMPERAMENTOS ---
    float peso_sanguineo  = (float)score.total_quente * score.total_umido;
    float peso_colerico   = (float)score.total_quente * score.total_seco;
    float peso_fleumatico  = (float)score.total_frio   * score.total_umido;
    float peso_melancolico = (float)score.total_frio   * score.total_seco;

    float soma_total_pesos = peso_sanguineo + peso_colerico + peso_fleumatico + peso_melancolico;
    if (soma_total_pesos == 0) {
        soma_total_pesos = 1.0f;
    }

    float pct_sanguineo  = (peso_sanguineo  / soma_total_pesos) * 100.0f;
    float pct_colerico   = (peso_colerico   / soma_total_pesos) * 100.0f;
    float pct_fleumatico  = (peso_fleumatico  / soma_total_pesos) * 100.0f;
    float pct_melancolico = (peso_melancolico / soma_total_pesos) * 100.0f;

    
    // Preenche o array com os dados e formatações exatas que você definiu
    ItemTemperamento lista[4] = {
        {SANGUINEO,   "     Sanguine (🜁 Air):", pct_sanguineo, 12},
        {COLERICO,    "    Choleric (🜂 Fire):", pct_colerico, 11},
        {FLEUMATICO,  " Phlegmatic (🜄 Water):", pct_fleumatico, 8},
        {MELANCOLICO, "Melancholic (🜃 Earth):", pct_melancolico, 7}
    };

    snprintf(lista[0].label, 30, "%s", _("     Sanguine (🜁 Air):"));
    snprintf(lista[1].label, 30, "%s", _("    Choleric (🜂 Fire):"));
    snprintf(lista[2].label, 30, "%s", _(" Phlegmatic (🜄 Water):"));
    snprintf(lista[3].label, 30, "%s", _("Melancholic (🜃 Earth):"));

    // Ordenação decrescente simples (Bubble Sort)
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (lista[j].porcentagem > lista[i].porcentagem) {
                ItemTemperamento temp = lista[i];
                lista[i] = lista[j];
                lista[j] = temp;
            }
        }
    }

    // Como o array está ordenado do maior para o menor, o índice 0 SEMPRE tem a maior porcentagem
    float maior_porcentagem = lista[0].porcentagem;

    // --- EXIBIÇÃO DAS PORCENTAGENS NA TELA ---
    int col_bars_pct = 29; // Mantém o recuo horizontal perfeitamente alinhado
    int max_barra_width = 40; // Tamanho da barra vindo da função de desenho

    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 14, 4, _("Temperament Distribution (Ranked):"));
    wattroff(table_win, A_BOLD);

    // Renderiza as linhas 16, 18, 20 e 22 de forma ordenada
    for (int i = 0; i < 4; i++) {
        int linha_atual = 16 + (i * 2);
        
        // Imprime o texto correspondente do temperamento

        wattron(table_win, A_ITALIC);
        mvwprintw(table_win, linha_atual, 6, "%s %5.1f%%", lista[i].label, lista[i].porcentagem);
        wattroff(table_win, A_ITALIC);
        
        // Desenha a barra gráfica com o par de cores correto associado ao elemento
        desenhar_barra_porcentagem(table_win, linha_atual, col_bars_pct, lista[i].porcentagem, lista[i].cor_par);

        // --- LÓGICA DO TROFÉU 🏆 ---
        // Se a porcentagem atual for igual à maior de todas, coloca o troféu após a barra
        // O cálculo da coluna pula: recuo inicial (29) + tamanho do texto numérico "[100.0%] " (9) + largura máxima da barra (40) + 1 espaço de folga
        if (lista[i].porcentagem == maior_porcentagem && maior_porcentagem > 0.0f) {
            wattron(table_win, COLOR_PAIR(11) | A_BOLD); // Usa a cor Amarela/Laranja para o troféu reluzir
            mvwprintw(table_win, linha_atual, col_bars_pct + 9 + max_barra_width + 1, _("🏆 Winner"));
            wattroff(table_win, COLOR_PAIR(11) | A_BOLD);
        }
    }


    // --- DETERMINAÇÃO FINAL DO HUMOR DOMINANTE ---
    int eixo_calor = score.total_quente - score.total_frio;
    int eixo_umidade = score.total_umido - score.total_seco;

    wattron(table_win, COLOR_PAIR(13));
    mvwprintw(table_win, 24, 4, "────────────────────────────────────────────────────────────────────────────────────");
    wattroff(table_win, COLOR_PAIR(13));
    
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 25, 4, _("Dominant Temperament: "));
    // O restante do seu switch case segue igual abaixo...
   
    char element1[10] = " ";
    char element2[10] = " ";
    char element3[10] = " ";
    char element4[10] = " ";

    // 1. CASO DE EMPATE NO CALOR (Eixo Calor == 0)
    if (eixo_calor == 0) {
        if (eixo_umidade > 0) {
            wattron(table_win, COLOR_PAIR(15));
            wprintw(table_win, _("SANGUINE-PHLEGMATIC MIX (Balanced Temp)"));

            snprintf(element1, 10, _("Air"));
            snprintf(element2, 10, _("Water"));
        } else if (eixo_umidade < 0) {
            wattron(table_win, COLOR_PAIR(21));
            wprintw(table_win, _("CHOLERIC-MELANCHOLIC MIX (Balanced Temp)"));

            snprintf(element1, 10, _("Fire"));
            snprintf(element2, 10, _("Earth"));
        } else {
            wattron(table_win, COLOR_PAIR(13));
            wprintw(table_win, _("ABSOLUTE BALANCE (Quadrapartite)"));

            snprintf(element1, 10, _("Fire"));
            snprintf(element2, 10, _("Water"));
            snprintf(element3, 10, _("Air"));
            snprintf(element4, 10, _("Earth"));
        }
    }
    // 2. CASO DE EMPATE NA UMIDADE (Eixo Umidade == 0)
    else if (eixo_umidade == 0) {
        if (eixo_calor > 0) {
            wattron(table_win, COLOR_PAIR(11)); 
            wprintw(table_win, _("SANGUINE-CHOLERIC MIX (Hot / Balanced Moisture)"));

            snprintf(element1, 10, _("Air"));
            snprintf(element2, 10, _("Fire"));
        } else {
            wattron(table_win, COLOR_PAIR(30) | A_REVERSE); 
            wprintw(table_win, _("PHLEGMATIC-MELANCHOLIC MIX (Cold / Balanced Moisture)"));

            snprintf(element1, 10, _("Water"));
            snprintf(element2, 10, _("Earth"));
        }
    }
    // 3. CASOS PURA E ESTREITAMENTE CONFIGURADOS (Sem empates nos eixos)
    else {
        if (eixo_calor > 0 && eixo_umidade > 0) {
            wattron(table_win, COLOR_PAIR(12) | A_REVERSE); 
            wprintw(table_win, _("SANGUINE (Hot & Wet - 🜁 Air Element)"));
            snprintf(element1, 10, _("Air"));
        } else if (eixo_calor > 0 && eixo_umidade < 0) {
            wattron(table_win, COLOR_PAIR(11)); 
            wprintw(table_win, _("CHOLERIC (Hot & Dry - 🜂 Fire Element)"));
            snprintf(element1, 10, _("Fire"));
        } else if (eixo_calor < 0 && eixo_umidade > 0) {
            wattron(table_win, COLOR_PAIR(8)); 
            wprintw(table_win, _("PHLEGMATIC (Cold & Wet - 🜄 Water Element)"));
            snprintf(element1, 10, _("Water"));
        } else if (eixo_calor < 0 && eixo_umidade < 0) { 
            wattron(table_win, COLOR_PAIR(30)); 
            wprintw(table_win, _("MELANCHOLIC (Cold & Dry - 🜃 Earth Element)"));
            snprintf(element1, 10, _("Earth"));
        }
    }

    
    
    char **element_ascii1 = (char **)get_element_ascii_by_name(element1);
    char **element_ascii2 = (char **)get_element_ascii_by_name(element2);
    char **element_ascii3 = (char **)get_element_ascii_by_name(element3);
    char **element_ascii4 = (char **)get_element_ascii_by_name(element4);

    if (element_ascii1) {
        for (int i = 0; i < 5; i++) {
            mvwprintw(table_win, 16 + i, 92, "%s", element_ascii1[i]);
        }
    }
    if (element_ascii2) {
        for (int i = 0; i < 5; i++) {
            mvwprintw(table_win, 16 + i, 100, "%s", element_ascii2[i]);
        }
    }
    if (element_ascii3) {
        for (int i = 0; i < 5; i++) {
            mvwprintw(table_win, 16 + i, 108, "%s", element_ascii3[i]);
        }
    }
    if (element_ascii4) {
        for (int i = 0; i < 5; i++) {
            mvwprintw(table_win, 16 + i, 116, "%s", element_ascii4[i]);
        }
    }
    
    // Desliga todos os atributos e cores de uma só vez de forma segura no ncurses
    wattroff(table_win, A_BOLD | COLOR_PAIR(30) | COLOR_PAIR(11) | COLOR_PAIR(8) | COLOR_PAIR(12) | COLOR_PAIR(13) | COLOR_PAIR(21) | A_REVERSE);

    /* ATUALIZADO: Mensagem amigável avisando da nova funcionalidade */
    mvwprintw(table_win, table_height - 1, 2, _("Press [i] for Full Text Interpretation | ESC/q to Return"));
    wnoutrefresh(table_win);

    doupdate();

    keypad(table_win, TRUE);
    nodelay(table_win, FALSE);
    
    int ch;
    do {
        ch = wgetch(table_win);
        
        /* GATILHO: Se pressionar 'i', abre o relatório corrido */
        if (ch == 'i' || ch == 'I') {
            abrir_janela_interpretacao_temperamento(score, lista, eixo_calor, eixo_umidade);
            
            /* Ao fechar o relatório, redesenha a janela do painel para limpar resíduos */
            touchwin(shadow_win);
            wnoutrefresh(shadow_win);
            touchwin(table_win);
            wnoutrefresh(table_win);

            doupdate();
        }

    } while (ch != 27 && ch != 'q' && ch != 'Q');
    
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr);
    refresh();
}



void abrir_janela_interpretacao_temperamento(ScoreTemperament score, ItemTemperamento *lista, int eixo_calor, int eixo_umidade) {
    int p_max_y, p_max_x;
    getmaxyx(stdscr, p_max_y, p_max_x); 

    // 1. DIMENSIONAMENTO RESPONSIVO DA JANELA
    int i_height = p_max_y - 6;
    if (i_height > 24) i_height = 24; 
    int i_width = p_max_x - 12;
    if (i_width > 102) i_width = 102;   

    int i_start_y = (p_max_y - i_height) / 2;
    int i_start_x = (p_max_x - i_width) / 2;

    // 2. CRIAÇÃO E RENDERIZAÇÃO DA JANELA DE SOMBRA (FUNDO)
    WINDOW *shadow_win = newwin(i_height, i_width, i_start_y + 1, i_start_x + 1);
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9)); // Par de cor preta/escura para a sombra
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);

    // 3. CRIAÇÃO DA MOLDURA PRINCIPAL
    WINDOW *border_win = newwin(i_height, i_width, i_start_y, i_start_x);
    wbkgd(border_win, COLOR_PAIR(13) | FLAGS);
    box(border_win, 0, 0);
    
    wattron(border_win, A_BOLD);
    const char *title = _(" Temperament Detailed Analysis ");
    mvwprintw(border_win, 0, (i_width - get_visual_width(title)) / 2, title);
    wattroff(border_win, A_BOLD);
    
    mvwprintw(border_win, i_height - 1, (i_width - 44) / 2, _(" [↓↑|JK: Scroll | Q|ESC: Return] "));
    wnoutrefresh(border_win);

    doupdate();

    // 4. CRIAÇÃO DA PAD INTERNA COM MAIS ESPAÇO HORIZONTAL
    int pad_lines = 150; // Aumentado para suportar os novos espaços em branco
    int pad_cols = i_width - 6; // Margem lateral ligeiramente maior para o texto respirar
    WINDOW *pad = newpad(pad_lines, pad_cols);
    wbkgd(pad, COLOR_PAIR(13) | FLAGS);
    keypad(pad, TRUE);
    idlok(pad, TRUE);
    scrollok(pad, TRUE);

    // 5. ESCRITA DOS TEXTOS NA PAD (COM ESPAÇAMENTO E MARGENS REFORÇADAS)
    wprintw(pad, "\n"); 

    wattron(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, _("  1. RAW SCORE SUMMARY\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wprintw(pad, "    %s: %d  |  %s: %d  |  %s: %d  |  %s: %d\n\n", 
            _("Hot"), score.total_quente, 
            _("Cold"), score.total_frio, 
            _("Moist"), score.total_umido, 
            _("Dry"), score.total_seco);
    wprintw(pad, "    %s: %d  |  %s: %d\n\n\n", _("Heat Axis"), eixo_calor, _("Moisture Axis"), eixo_umidade);
    wattron(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, _("  2. STRUCTURAL TEMPERAMENT DYNAMICS\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    
    // 1. CASO DE EMPATE NO CALOR (Eixo Calor == 0)
    if (eixo_calor == 0) {
        if (eixo_umidade > 0) {
            wattron(pad, A_BOLD | COLOR_PAIR(15));
            wprintw(pad, _("    SANGUINE-PHLEGMATIC MIX (Balanced Temp)\n\n"));
            wattroff(pad, A_BOLD | COLOR_PAIR(15));
            wprintw(pad, _("    Your vital energy functions in perfect thermal equilibrium, blending \n"
                         "Air and Water.\n"
                         "    This grants a deeply flexible, empathetic, and highly social persona.\n"
                         "    The talkative mind flows smoothly with natural emotional sensitivity, \n"
                         "offering extreme social adaptability but risking mild emotional \n"
                         "scattering.\n\n\n"));
        } else if (eixo_umidade < 0) {
            wattron(pad, A_BOLD | COLOR_PAIR(21));
            wprintw(pad, _("    CHOLERIC-MELANCHOLIC MIX (Balanced Temp)\n\n"));
            wattroff(pad, A_BOLD | COLOR_PAIR(21));
            wprintw(pad, _("    Your active forces exist in a state of controlled thermal poise, \n"
                         "blending Fire and Earth.\n"
                         "    This creates an unstoppable achiever rooted in deep structural \n"
                         "efficiency and discipline.\n"
                         "    You possess immense willpower, though you must guard against chronic \n"
                         "internal stress, rigid perfectionism, and deep stubbornness.\n\n\n"));
        } else {
            wattron(pad, A_BOLD | COLOR_PAIR(13));
            wprintw(pad, _("    ABSOLUTE BALANCE (Quadrapartite Temperament)\n\n"));
            wattroff(pad, A_BOLD | COLOR_PAIR(13));
            wprintw(pad, _("    Your chart reflects the rare, ideal geometric balance of all four \n"
                         "humors.\n"
                         "    Fire, Water, Air, and Earth converge into perfect cosmic proportion. \n"
                         "You possess the courage of the Choleric, the social grace of the Sanguine, \n"
                         "the empathy of the Phlegmatic, and the objective realism of the Melancholic, \n"
                         "activated exactly when needed.\n\n\n"));
        }
    }
    // 2. CASO DE EMPATE NA UMIDADE (Eixo Umidade == 0)
    else if (eixo_umidade == 0) {
        if (eixo_calor > 0) {
            wattron(pad, A_BOLD | COLOR_PAIR(11));
            wprintw(pad, _("    SANGUINE-CHOLERIC MIX (Hot / Balanced Moisture)\n\n"));
            wattroff(pad, A_BOLD | COLOR_PAIR(11));
            wprintw(pad, _("    An expressive, warm, and highly action-oriented signature blending \n"
                         "Air and Fire.\n"
                         "    Your dynamic energy acts as a strong catalyst in social or \n"
                         "professional domains.\n"
                         "    The intellectual curiosity of Air merges with the raw ambition \n"
                         "of Fire, fueling powerful leadership qualities, with a risk of occasional \n"
                         "impatience.\n\n\n"));
        } else {
            wattron(pad, A_BOLD | COLOR_PAIR(30) | A_REVERSE);
            wprintw(pad, _("    PHLEGMATIC-MELANCHOLIC MIX (Cold / Balanced Moisture)\n\n"));
            wattroff(pad, A_BOLD | COLOR_PAIR(30) | A_REVERSE);
            wprintw(pad, _("    A calm, deeply introspective, and self-protective constitution \n"
                         "blending Water and Earth.\n"
                         "    Your operating engine is deliberate, highly structured, and \n"
                         "cautious.\n"
                         "    The emotional depths of Water anchor onto the realistic stability \n"
                         "of Earth, yielding an exceptionally reliable, analytical individual \n"
                         "who moves at a careful pace.\n\n\n"));
        }
    }
    // 3. CASOS PURA E ESTREITAMENTE CONFIGURADOS (Sem empates nos eixos)
    else {
        if (eixo_calor > 0 && eixo_umidade > 0) {
            wattron(pad, A_BOLD | COLOR_PAIR(12) | A_REVERSE);
            wprintw(pad, _("    SANGUINE (Hot & Wet - Air Element)\n\n"));
            wattroff(pad, A_BOLD | COLOR_PAIR(12) | A_REVERSE);
            wprintw(pad, _("    Your psychological engine revolves around communication, expansion, \n"
                         "and mental fluidity.\n"
                         "    You absorb external impressions instantly and possess an adaptable, \n"
                         "expressive mind that thrives on curiosity and social exchange. Guard \n"
                         "against scattering focus.\n\n\n"));
        } else if (eixo_calor > 0 && eixo_umidade < 0) {
            wattron(pad, A_BOLD | COLOR_PAIR(11));
            wprintw(pad, _("    CHOLERIC (Hot & Dry - Fire Element)\n\n"));
            wattroff(pad, A_BOLD | COLOR_PAIR(11));
            wprintw(pad, _("    Your core essence is driven by swift, active, and result-oriented \n"
                         "single-minded impulses.\n"
                         "    You possess an innate leadership spark, extreme courage when \n"
                         "confronting adversity, and unyielding passion. Your chief challenge \n"
                         "is mastering reactive impatience.\n\n\n"));
        } else if (eixo_calor < 0 && eixo_umidade > 0) {
            wattron(pad, A_BOLD | COLOR_PAIR(8));
            wprintw(pad, _("    PHLEGMATIC (Cold & Wet - Water Element)\n\n"));
            wattroff(pad, A_BOLD | COLOR_PAIR(8));
            wprintw(pad, _("    You embody a stable, welcoming, and peacekeeping emotional \n"
                         "constitution.\n"
                         "    You operate through deep empathy, automatic diplomatic mediation, \n"
                         "and a steady, reliable operational rhythm. Watch out for behavioral \n"
                         "inertia or stagnation.\n\n\n"));
        } else if (eixo_calor < 0 && eixo_umidade < 0) {
            wattron(pad, A_BOLD | COLOR_PAIR(30));
            wprintw(pad, _("    MELANCHOLIC (Cold & Dry - Earth Element)\n\n"));
            wattroff(pad, A_BOLD | COLOR_PAIR(30));
            wprintw(pad, _("    Your nature is built on structural caution, real-world pragmatism, \n"
                         "and profound reflection.\n"
                         "    An exceptional asset for deep analysis, long-term organization, and \n"
                         "concrete security  building. Beware a natural tendency toward brooding \n"
                         "or social withdrawal.\n\n\n"));
        }
    }


    /* =========================================================================
       NOVO: IDENTIFICAÇÃO DINÂMICA DE INFLUENCIAS SECUNDÁRIAS (70% do Líder)
       ========================================================================= */
    
    float proporcao_minima = TEMPERAMENT_RANK_PROPORTION;
    // O corte é calculado com base no primeiro colocado da lista (o de maior porcentagem)
    float corte_significativo = lista[0].porcentagem * proporcao_minima;
    
    int exibiu_conector = 0;

    // Varre a lista a partir do segundo colocado (índice 1) até o quarto (índice 3)
    for (int i = 1; i < 4; i++) {
        
        // Regra de Ouro: O elemento precisa estar acima do corte de 70%,
        // mas DEVE ter uma porcentagem menor que o líder. Se for estritamente igual,
        // significa que os eixos já trataram isso nativamente como um Empate lá em cima.
        if (lista[i].porcentagem >= corte_significativo && lista[i].porcentagem < lista[0].porcentagem) {
        
            // Se for o primeiro coadjuvante significativo encontrado, imprime a transição
            if (!exibiu_conector) {
                wattron(pad, A_BOLD);
                wprintw(pad, _("    SECONDARY INFLUENCES & NUANCES\n\n"));
                wattroff(pad, A_BOLD);
                wprintw(pad, _("    Although your core template is defined above, your psychiatric map\n"
                             "    shows other active forces adding layers to your behavior:\n\n"));
                exibiu_conector = 1;
            }

            // Renderiza o modificador específico baseado no ID de cor do temperamento
            switch (lista[i].id) {
                case SANGUINEO: // Sanguine
                    wattron(pad, COLOR_PAIR(12) | A_REVERSE);
                    wprintw(pad, _("    ✦ Sanguine Influence: "));
                    wattroff(pad, COLOR_PAIR(12) | A_REVERSE);
                    wprintw(pad, _("Adds an overlay of communicative ease, adaptability,\n"
                                 "      and cognitive curiosity. This softens any rigid boundaries or\n"
                                 "      stagnation imposed by your dominant humor.\n\n"));
                    break;

                case COLERICO: // Choleric
                    wattron(pad, COLOR_PAIR(11));
                    wprintw(pad, _("    ✦ Choleric Influence: "));
                    wattroff(pad, COLOR_PAIR(11));
                    wprintw(pad, _("Injects raw drive, sharp focus on execution, and a\n"
                                 "      natural urgency to fix problems. It sharpens a passive baseline\n"
                                 "      into a highly proactive mindset.\n\n"));
                    break;

                case FLEUMATICO: // Phlegmatic
                    wattron(pad, COLOR_PAIR(8));
                    wprintw(pad, _("    ✦ Phlegmatic Influence: "));
                    wattroff(pad, COLOR_PAIR(8));
                    wprintw(pad, _("Acts as a natural thermal and emotional ballast, introducing\n"
                                 "      diplomatic mediation, structural patience, and a steady pace that\n"
                                 "      protects you from chronic stress.\n\n"));
                    break;

                case MELANCOLICO: // Melancholic
                    wattron(pad, COLOR_PAIR(7));
                    wprintw(pad, _("    ✦ Melancholic Influence: "));
                    wattroff(pad, COLOR_PAIR(7));
                    wprintw(pad, _("Confers analytical depth, long-term organization skills,\n"
                                 "      and a realistic sense of caution. It ensures your impulses are\n"
                                 "      grounded in real-world feasibility.\n\n"));
                    break;
            }
        }
    }

    wprintw(pad, "\n\n");
    
    /* --- CAMADA 3: PORCENTAGENS DO RANKING (AUXILIAR) --- */
    wattron(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, _("  3. RANKED DISTRIBUTION DATA\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    for (int i = 0; i < 4; i++) {
        wprintw(pad, "    %s %d: %s %5.1f%%\n", _("Rank"), i + 1, lista[i].label, lista[i].porcentagem);
    }
    wprintw(pad, "\n\n");

    wattron(pad, A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wprintw(pad, _("  [NARRATIVE END] - Press 'Q' or ESC to return to the graphs.\n"));
    wattroff(pad, A_DIM);

    // 6. LOOP DE INTERAÇÃO E REDESENHO CONSTANTE DA PAD
    int pad_line_pos = 0;
    int max_scroll_y = 65; 
    int ch;

    prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);

    while ((ch = wgetch(pad)) != 27 && ch != 'q' && ch != 'Q') {
        switch (ch) {
            case KEY_UP:
            case 'k':
            case 'K':
                if (pad_line_pos > 0) pad_line_pos--;
                break;
            case KEY_DOWN:
            case 'j':
            case 'J':
                if (pad_line_pos < max_scroll_y) pad_line_pos++;
                break;
        }
        prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);
    }

    // 7. DESTRUIÇÃO E LIMPEZA
    delwin(pad);
    delwin(border_win);
    delwin(shadow_win);
}