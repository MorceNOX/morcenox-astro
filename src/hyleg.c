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
#include "almuten.h"
#include "hyleg.h"
#include "aspects.h"



// Tabela Tradicional de Anos dos Planetas (Anos Maiores, Médios e Menores)
// Índices baseados na escala 1 a 7 (0 descartado)
// Ordem: [1]=Sol, [2]=Lua, [3]=Mercúrio, [4]=Vênus, [5]=Marte, [6]=Júpiter, [7]=Saturno
const int anos_maiores[8] = {0, 120, 108, 76, 82, 66, 79, 57};
const int anos_medios[8]  = {0, 69,  66,  48, 45, 40, 45, 43};
const int anos_menores[8] = {0, 19,  25,  20, 8,  15, 12, 30};


// Regências clássicas dos signos (0=vazio, 1=Áries, 2=Touro...)
// Retorna o ID do planeta de 1 a 7
int obter_regente_tradicional(int id_signo) {
    switch (id_signo) {
        case 1:  case 8:  return 5; // Áries e Escorpião = Marte
        case 2:  case 7:  return 4; // Touro e Libra = Vênus
        case 3:  case 6:  return 3; // Gêmeos e Virgem = Mercúrio
        case 4:           return 2; // Câncer = Lua
        case 5:           return 1; // Leão = Sol
        case 9:  case 12: return 6; // Sagitário e Peixes = Júpiter
        case 10: case 11: return 7; // Capricórnio e Aquário = Saturno
        default: return 0;
    }
}


// Retorna true (1) se a casa for um lugar hylegíaco permitido (1, 9, 10, 11, 7)
bool is_lugar_hylegiaco(int casa) {
    return (casa == 1 || casa == 9 || casa == 10 || casa == 11 || casa == 7);
}



int get_hyleg(PontosHylegiacos pontos, PlotObject *plots, AspectMatrix *aspecto_matriz, int *id_planeta_almuten, int regente_dia, int regente_hora) {
    int casa_sol = 0, casa_lua = 0, casa_fortuna = 0;
    int object_diff = show_modern_planets ? 0 : 3;

    // 1. Extrai o número da casa física atual do Sol, Lua e Parte da Fortuna do array 'plots'
    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        if (plots[i].id == P_SOL) {
            casa_sol = romanToInt(plots[i].house);
        }
        if (plots[i].id == P_LUNA) {
            casa_lua = romanToInt(plots[i].house);
        }
        if (plots[i].id == P_FORTUNA - object_diff) {
            casa_fortuna = romanToInt(plots[i].house);
        }
    }


    // ────────────────────────────────────────────────────────────────────────
    // ABORDAGEM DIURNA (Prioridade: Sol -> Lua -> Almuten -> Asc)
    // ────────────────────────────────────────────────────────────────────────
    if (MAPA_DIURNO) {
        // Criterio 1: O Sol em lugar hylegíaco
        if (is_lugar_hylegiaco(casa_sol)) {
            int rulers[7];
            get_rulers_by_lon(plots[P_SOL].longitude, consider_modern_planets_rulling, &rulers[0], &rulers[1], &rulers[2], &rulers[3], &rulers[4], &rulers[5], &rulers[6]);

            for (int i = 0; i < 7; i++) {
                // se há aspecto com pelo menos um de seus regentes
                int id_ruler = (rulers[i] <= 10) ? rulers[i] - 1 : rulers[i] - 1 - object_diff;
                if (has_aspect(P_SOL, id_ruler, aspecto_matriz)) {
                    return H_SOL;
                }
            }  
        }
        // Criterio 2: A Lua em lugar hylegíaco
        if (is_lugar_hylegiaco(casa_lua)) {
            int rulers[7];
            get_rulers_by_lon(plots[P_LUNA].longitude, consider_modern_planets_rulling, &rulers[0], &rulers[1], &rulers[2], &rulers[3], &rulers[4], &rulers[5], &rulers[6]);

            for (int i = 0; i < 7; i++) {
                // se há aspecto com pelo menos um de seus regentes
                int id_ruler = (rulers[i] <= 10) ? rulers[i] - 1 : rulers[i] - 1 - object_diff;
                if (has_aspect(P_SOL, id_ruler, aspecto_matriz)) {
                    return H_LUNA;
                }
            }
        }
    } 
    // ────────────────────────────────────────────────────────────────────────
    // ABORDAGEM NOTURNA (Prioridade: Lua -> Sol -> Fortuna -> Almuten -> Asc)
    // ────────────────────────────────────────────────────────────────────────
    else {
        // Criterio 1: A Lua em lugar hylegíaco
        if (is_lugar_hylegiaco(casa_lua)) {
            int rulers[7];
            get_rulers_by_lon(plots[P_LUNA].longitude, consider_modern_planets_rulling, &rulers[0], &rulers[1], &rulers[2], &rulers[3], &rulers[4], &rulers[5], &rulers[6]);

            for (int i = 0; i < 7; i++) {
                // se há aspecto com pelo menos um de seus regentes
                int id_ruler = (rulers[i] <= 10) ? rulers[i] - 1 : rulers[i] - 1 - object_diff;
                if (has_aspect(P_SOL, id_ruler, aspecto_matriz)) {
                    return H_LUNA;
                }
            }
        }
        // Criterio 2: O Sol em lugar hylegíaco
        if (is_lugar_hylegiaco(casa_sol)) {
            int rulers[7];
            get_rulers_by_lon(plots[P_SOL].longitude, consider_modern_planets_rulling, &rulers[0], &rulers[1], &rulers[2], &rulers[3], &rulers[4], &rulers[5], &rulers[6]);

            for (int i = 0; i < 7; i++) {
                // se há aspecto com pelo menos um de seus regentes
                int id_ruler = (rulers[i] <= 10) ? rulers[i] - 1 : rulers[i] - 1 - object_diff;
                if (has_aspect(P_SOL, id_ruler, aspecto_matriz)) {
                    return H_SOL;
                }
            }
        }
        // Criterio 3: A Parte da Fortuna em lugar hylegíaco
        if (is_lugar_hylegiaco(casa_fortuna)) {
            int rulers[7];
            get_rulers_by_lon(plots[P_FORTUNA].longitude, consider_modern_planets_rulling, &rulers[0], &rulers[1], &rulers[2], &rulers[3], &rulers[4], &rulers[5], &rulers[6]);

            for (int i = 0; i < 7; i++) {
                // se há aspecto com pelo menos um de seus regentes
                int id_ruler = (rulers[i] <= 10) ? rulers[i] - 1 : rulers[i] - 1 - object_diff;
                if (has_aspect(P_SOL, id_ruler, aspecto_matriz)) {
                    return H_FORTUNA;
                }
            }  
        }
    }


    // testa o almuten da SAN

    int res_san[12] = {0};
    int qtd_san = get_almuten(plots[P_SAN - object_diff].longitude, res_san, aspecto_matriz, pontos, plots);
    

    if (qtd_san > 0) { 
        int luminar = (MAPA_DIURNO)?0:1;
        int tem_aspecto = 0;
        int id_candidato_san = -1;
        int casa_almuten_san = 0;   

        for (int q = 0; q < qtd_san; q++) {
            id_candidato_san = res_san[q]; // Pega o primeiro Almuten vencedor
            // Descobre em qual casa o planeta Almuten está posicionado fisicamente
            casa_almuten_san = 0;
            for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
                if (i + 1 == id_candidato_san) {
                    casa_almuten_san = romanToInt(plots[i].house);
                    break;
                }
            }
            // Se o Almuten da SAN estiver em uma casa vital, ele pode assumir o Hyleg
            if (is_lugar_hylegiaco(casa_almuten_san)) {
                
                // se tem aspecto com o luminar da seita assume o Hyleg
                if (has_aspect(luminar, id_candidato_san - 1, aspecto_matriz)) {
                    tem_aspecto = 1;
                    break;
                }
                
                if (tem_aspecto) {
                    *id_planeta_almuten = id_candidato_san; // Retorna por referência o ID do planeta (1 a 12)
                    return H_ALMUTEN_SAN;
                }
                
            }
        }
    }


    // ────────────────────────────────────────────────────────────────────────
    // CRITÉRIO DE RECURSO MEDIEVAL: Almuten dos Pontos Hylegíacos
    // Se Luminares/Fortuna/Almuten da SAN falharem, calcula-se o Almuten do Mapa que esteja em casa hylegíaca
    // ────────────────────────────────────────────────────────────────────────
    int res_figuris[12] = {0};
    int qtd = calcular_almuten_figuris(pontos, plots, aspecto_matriz, regente_dia, regente_hora, res_figuris);
    

    if (qtd > 0) { 
        int id_candidato = -1;
        int casa_almuten = 0;   

        for (int q = 0; q < qtd; q++) {
            id_candidato = res_figuris[q]; // Pega o primeiro Almuten vencedor
            // Descobre em qual casa o planeta Almuten está posicionado fisicamente
            casa_almuten = 0;
            for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
                if (i + 1 == id_candidato) {
                    casa_almuten = romanToInt(plots[i].house);
                    break;
                }
            }
            // Se o Almuten do mapa estiver em uma casa vital, ele assume o Hyleg
            if (is_lugar_hylegiaco(casa_almuten)) {                
                *id_planeta_almuten = id_candidato; // Retorna por referência o ID do planeta (1 a 12)
                return H_ALMUTEN;                
            }
        }
    }

    // ────────────────────────────────────────────────────────────────────────
    // ÚLTIMO RECURSO ABSOLUTO: O próprio grau do Ascendente (Sempre na Casa 1)
    // Se tudo falhar, o Ascendente é eleito o Hyleg por ser o doador do corpo físico
    // ────────────────────────────────────────────────────────────────────────
    return H_ASC;
}




// Função que retorna o nome descritivo do critério do Hyleg para exibição na UI
const char* obter_descricao_hileg(int tipo_hileg) {
    switch (tipo_hileg) {
        case H_SOL:     return _("Sun (Essential Luminary)");
        case H_LUNA:    return _("Luna (Essential Luminary)");
        case H_FORTUNA: return _("Part of Fortune (Mathematical Point)");
        case H_ASC:     return _("Ascendant Degree (Bodily Shield / Default)");
        case H_ALMUTEN: return _("Almuten Figuris (Chart Ruler)");
        case H_ALMUTEN_SAN: return _("Almuten of SAN (SAN's Ruler)");
        default:        return _("Unknown Criterion");
    }
}

int obter_anos_menores_por_nome(const char *object_name) {
    if (strcmp(object_name, "Sol") == 0 || strcmp(object_name, _("Sun")) == 0) return 19;
    if (strcmp(object_name, "Luna") == 0 || strcmp(object_name, _("Moon")) == 0) return 25;
    if (strcmp(object_name, _("Mercury")) == 0) return 20;
    if (strcmp(object_name, _("Venus")) == 0)   return 8;
    if (strcmp(object_name, _("Mars")) == 0)    return 15;
    if (strcmp(object_name, _("Jupiter")) == 0) return 12;
    if (strcmp(object_name, _("Saturn")) == 0)  return 30;
    return 0;
}





ResultadoAlcochoden calcular_alcochoden(int tipo_hileg, int idx_hileg_objeto, AspectMatrix *matrix, PlotObject *plots, PlanetDignities *dig, int regente_dia, int regente_hora, PontosHylegiacos pontos) {
    ResultadoAlcochoden resultado = {"None", "", 0, "None", 0}; // Inicializa casa como 0
    int object_diff = show_modern_planets ? 0 : 3;

    int candidatos[12] = {0};
    int qtd_candidatos = 0;

    // 1. Encontra o índice real do Hyleg no array plots
    int idx_hileg_grid = -1;
    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        if (tipo_hileg == H_SOL && plots[i].id == P_SOL) { idx_hileg_grid = i; break; }
        if (tipo_hileg == H_LUNA && plots[i].id == P_LUNA) { idx_hileg_grid = i; break; }
        if (tipo_hileg == H_ASC && plots[i].id == P_ASC - object_diff) { idx_hileg_grid = i; break; }
        if (tipo_hileg == H_FORTUNA && plots[i].id == P_FORTUNA - object_diff) { idx_hileg_grid = i; break; }
        if (tipo_hileg == H_ALMUTEN && (plots[i].id + 1) == idx_hileg_objeto) { idx_hileg_grid = i; break; }
        if (tipo_hileg == H_ALMUTEN_SAN && (plots[i].id + 1) == idx_hileg_objeto) { idx_hileg_grid = i; break; }
    }

    if (idx_hileg_grid == -1) return resultado;

    // 2. Procura planetas tradicionais (j de 0 a 6) que aspectam o Hileg
    for (int j = 0; j < 7; j++) {
        if (j == idx_hileg_grid) continue;

        AspectCell c1 = matrix->grid[j][idx_hileg_grid];
        AspectCell c2 = matrix->grid[idx_hileg_grid][j];

        if (c1.has_aspect || c2.has_aspect) {
            candidatos[qtd_candidatos] = j;
            qtd_candidatos++;
        }
    }

    int idx_vencedor_grid = -1;
    bool regra_tutor_medieval = false;

    if (qtd_candidatos == 0) {
        if (tipo_hileg == H_SOL || tipo_hileg == H_LUNA) {
            idx_vencedor_grid = idx_hileg_grid;
        } else {
            int res_figuris = 0; 
            calcular_almuten_figuris(pontos, plots, matrix, regente_dia, regente_hora, &res_figuris);
            
            for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
                if ((plots[i].id + 1) == res_figuris) {
                    idx_vencedor_grid = i;
                    break;
                }
            }
            regra_tutor_medieval = true;
        }
    } 
    else {
        int max_essencial = -999;
        for (int k = 0; k < qtd_candidatos; k++) {
            int idx_cand = candidatos[k];
            int id_planeta = plots[idx_cand].id + 1; 
            int idx_matriz = obter_indice_matriz_dig(id_planeta);
            
            if (idx_matriz != -1 && dig[idx_matriz].essential > max_essencial) {
                max_essencial = dig[idx_matriz].essential;
                idx_vencedor_grid = idx_cand;
            }
        }
    }

    if (idx_vencedor_grid == -1 || idx_vencedor_grid >= NUM_OBJECTS) return resultado;

    // Salva o nome e o glifo corretos baseados no vencedor
    strcpy(resultado.object_name, plots[idx_vencedor_grid].object_name);
    strcpy(resultado.glifo, plots[idx_vencedor_grid].object);

    int id_venc = plots[idx_vencedor_grid].id + 1; 
    
    // ATRIBUIÇÃO COMPACTA: Extrai a casa e armazena diretamente na struct de retorno
    resultado.casa_alcochoden = romanToInt(plots[idx_vencedor_grid].house);

    if (regra_tutor_medieval) {
        resultado.anos_concedidos = anos_menores[id_venc];
        strcpy(resultado.tipo_anos, _("Lesser (Proxy Rule)"));
    } else {
        if (resultado.casa_alcochoden == 1 || resultado.casa_alcochoden == 4 || resultado.casa_alcochoden == 7 || resultado.casa_alcochoden == 10) {
            resultado.anos_concedidos = anos_maiores[id_venc];
            strcpy(resultado.tipo_anos, _("Great"));
        } else if (resultado.casa_alcochoden == 2 || resultado.casa_alcochoden == 5 || resultado.casa_alcochoden == 8 || resultado.casa_alcochoden == 11) {
            resultado.anos_concedidos = anos_medios[id_venc];
            strcpy(resultado.tipo_anos, _("Medium"));
        } else {
            resultado.anos_concedidos = anos_menores[id_venc];
            strcpy(resultado.tipo_anos, _("Lesser"));
        }
    }

    // 4. PASSO DE ASPECTOS
    for (int j = 0; j < 7; j++) {
        if (j == idx_vencedor_grid) continue;

        AspectCell c1 = matrix->grid[j][idx_vencedor_grid];
        AspectCell c2 = matrix->grid[idx_vencedor_grid][j];

        if (c1.has_aspect || c2.has_aspect) {
            int anos_modificadores = obter_anos_menores_por_nome(plots[j].object_name);

            if (strcmp(plots[j].object_name, _("Jupiter")) == 0 || strcmp(plots[j].object_name, _("Venus")) == 0) {
                resultado.anos_concedidos += anos_modificadores;
            }
            else if (strcmp(plots[j].object_name, _("Saturn")) == 0 || strcmp(plots[j].object_name, _("Mars")) == 0) {
                resultado.anos_concedidos -= anos_modificadores;
            }
        }
    }

    if (resultado.anos_concedidos < anos_menores[id_venc]) {
        resultado.anos_concedidos = anos_menores[id_venc];
    }

    return resultado;
}





void display_life_givers(PontosHylegiacos pontos, PlanetDignities *dig, PlotObject *plots, AspectMatrix *matrix, int week_day, int planetary_hour) {
    // 1. DIMENSIONAMENTO DA JANELA POP-UP
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 22; // Altura compacta perfeita para o bloco cronocrático
    int table_width = max_x - 10;
    int start_y = (max_y - table_height) / 2;
    int start_x = 5;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13));

    // Título Centralizado da Interface
    wattron(table_win, A_BOLD);
    const char *title = _("Vital Chronocrators: Life Givers");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
    wattroff(table_win, A_BOLD);

    
    int regente_dia = converter_codigo_planeta(get_hour_regent(week_day - 1, (MAPA_DIURNO)?0:12));
    int regente_hora = converter_codigo_planeta(get_hour_regent(week_day - 1, planetary_hour - 1));

    
    // 3. EXECUÇÃO INTEGRADA DOS CÁLCULOS EM SEGUNDO PLANO
    int id_almuten_ref = 0;
    int object_diff = show_modern_planets ? 0 : 3;
    
    // Recupera o Hileg calculado pelo sistema para passar as coordenadas de aspectos
    int tipo_h = get_hyleg(pontos, plots, matrix, &id_almuten_ref, regente_dia, regente_hora);
    int idx_hileg_grid = -1;
    
    if (tipo_h == H_SOL) idx_hileg_grid = 0;
    else if (tipo_h == H_LUNA) idx_hileg_grid = 1;
    else if (tipo_h == H_ALMUTEN) idx_hileg_grid = id_almuten_ref - 1;
    else if (tipo_h == H_ALMUTEN_SAN) idx_hileg_grid = id_almuten_ref - 1;
    else {
        for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
            if (tipo_h == H_ASC && plots[i].id == P_ASC - object_diff) { idx_hileg_grid = i; break; }
            if (tipo_h == H_FORTUNA && plots[i].id == P_FORTUNA - object_diff) { idx_hileg_grid = i; break; }
        }
    }
    // Roda a rotina de busca de aspectos do Alcochoden que estruturamos
    ResultadoAlcochoden alco = calcular_alcochoden(tipo_h, idx_hileg_grid + 1, matrix, plots, dig, regente_dia, regente_hora, pontos);

    // 4. RENDERIZAÇÃO VISUAL DOS BLOCOS DE TEXTO
    
    // Linha 1: Seita do Mapa Natal
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 2, 4, _("Sect of the Chart: "));
    wattron(table_win, A_BOLD);
    if (MAPA_DIURNO) {
        wattron(table_win, COLOR_PAIR(15) | A_BOLD | A_REVERSE); // Amarelo para o Dia
        wprintw(table_win, _("DIURNAL"));
        wattroff(table_win, COLOR_PAIR(15) | A_BOLD | A_REVERSE);
    } else {
        wattron(table_win, COLOR_PAIR(8) | A_REVERSE); // Azul para a Noite
        wprintw(table_win, _("NOCTURNAL"));
        wattroff(table_win, COLOR_PAIR(8) | A_REVERSE);
    }
    wattroff(table_win, A_BOLD);

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 4, 2, "───────────────────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    // Linha 2: Exibição do Hileg Eleito
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 5, 4, _("HYLEG (Giver of Life): "));
    wattron(table_win, COLOR_PAIR(8) | A_BOLD);
    
    if (tipo_h == H_SOL) {
        wprintw(table_win, " %s ", obter_glifo_planeta_por_id(1));
    } else if (tipo_h == H_LUNA) {
        wprintw(table_win, " %s ", obter_glifo_planeta_por_id(2));
    } else if (tipo_h == H_ALMUTEN) {
        wprintw(table_win, " %s ", obter_glifo_planeta_por_id(id_almuten_ref));
    } else if (tipo_h == H_FORTUNA) {
        wprintw(table_win, " 🝴 ");
    } else {
        wprintw(table_win, " ASC ");
    }
    wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
    
    wattron(table_win, A_DIM);
    wprintw(table_win, " -> %s: %s", _("Selected via"), obter_descricao_hileg(tipo_h));
    wattroff(table_win, A_DIM);

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 7, 2, "───────────────────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    // Linha 3: Exibição do Alcochoden e dos Anos Resultantes na sua Janela
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 8, 4, _("ALCOCHODEN (Giver of Years): "));
    wattroff(table_win, A_BOLD);
    
    if (strcmp(alco.object_name, "None") != 0) {
        wattron(table_win, COLOR_PAIR(17) | A_BOLD);
        
        // IMPRESSÃO DIRETA: Imprime o glifo Unicode estocado de forma 100% segura
        wprintw(table_win, " %s ", alco.glifo);
        
        wattroff(table_win, COLOR_PAIR(17) | A_BOLD);

        // Dentro da sua display_life_givers, onde exibe o resultado final de anos:
        wattron(table_win, A_BOLD);
        mvwprintw(table_win, 10, 4, _("Calculated Lifespan Measure: "));
        wattron(table_win, COLOR_PAIR(15) | A_BOLD | A_UNDERLINE);
        wprintw(table_win, "%d %s", alco.anos_concedidos, _("Years"));
        wattroff(table_win, COLOR_PAIR(15) | A_BOLD | A_UNDERLINE);
    
        // Se o valor estiver travado em 8 por falta de aspectos, avisa o usuário!
        if (alco.anos_concedidos == 8 && strcmp(alco.tipo_anos, _("Lesser (Proxy Rule)")) == 0) {
            wattron(table_win, COLOR_PAIR(11) | A_BOLD);
            wprintw(table_win, _(" [FERAL/UNASPECTED PLANET]"));
            wattroff(table_win, COLOR_PAIR(11) | A_BOLD);
        }
    
        // CORREÇÃO: Consome a casa real do Alcochoden direto da struct de retorno
        wattron(table_win, A_DIM);
        wprintw(table_win, " %s. %s %d", alco.tipo_anos, _("Years distributed via physical Position in House"), alco.casa_alcochoden);
        wattroff(table_win, A_DIM);

        char planet_hyleg[30];

        if (tipo_h == H_SOL) {
            snprintf(planet_hyleg, 30, "%s", obter_glifo_planeta_por_id(1));
        } else if (tipo_h == H_LUNA) {
            snprintf(planet_hyleg, 30, "%s", obter_glifo_planeta_por_id(2));
        } else if (tipo_h == H_ALMUTEN) {
            snprintf(planet_hyleg, 30, "%s", obter_glifo_planeta_por_id(id_almuten_ref));
        } else if (tipo_h == H_FORTUNA) {
            snprintf(planet_hyleg, 30, "%s", "🝴");
        } else {
            snprintf(planet_hyleg, 30, "ASC");
        }

        wattron(table_win, A_BOLD);
        mvwprintw(table_win, 1, 105, "%s", "Hyleg:");
        if (strcmp(planet_hyleg, "ASC") != 0) {
            const char **ascii_art = get_planet_ascii_by_gliph(planet_hyleg);
            
            wattron(table_win, COLOR_PAIR(8));
            mvwprintw(table_win, 2, 105, "%s", ascii_art[0]);
            mvwprintw(table_win, 3, 105, "%s", ascii_art[1]);
            mvwprintw(table_win, 4, 105, "%s", ascii_art[2]);
            mvwprintw(table_win, 5, 105, "%s", ascii_art[3]);
            mvwprintw(table_win, 6, 105, "%s", ascii_art[4]);
            mvwprintw(table_win, 7, 105, "%s", ascii_art[5]);
        }
        else {
            wattron(table_win, COLOR_PAIR(8));
            mvwprintw(table_win, 4, 105, "%s", "ASC");
        }
        wattroff(table_win, COLOR_PAIR(8));

        mvwprintw(table_win, 9, 105, "%s", "Alcochoden:");
        const char **ascii_art1 = get_planet_ascii_by_gliph(alco.glifo);

        wattron(table_win, COLOR_PAIR(8));
        mvwprintw(table_win, 10, 105, "%s", ascii_art1[0]);
        mvwprintw(table_win, 11, 105, "%s", ascii_art1[1]);
        mvwprintw(table_win, 12, 105, "%s", ascii_art1[2]);
        mvwprintw(table_win, 13, 105, "%s", ascii_art1[3]);
        mvwprintw(table_win, 14, 105, "%s", ascii_art1[4]);
        mvwprintw(table_win, 15, 105, "%s", ascii_art1[5]);

        wattroff(table_win, COLOR_PAIR(8) | A_BOLD);

    }
    

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 12, 2, "───────────────────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    // Rodapé de Notas Astrológicas Tradicionais
    wattron(table_win, A_DIM);
    mvwprintw(table_win, 14, 4, _("Note 1: Cadent houses (3, 6, 9, 12) restrict the Alcochoden to its Lesser Years."));
    mvwprintw(table_win, 15, 4, _("Benefic or malefic aspects dynamically modify this base vitality score."));
    mvwprintw(table_win, 17, 4, _("Note 2: An unaspected Alcochoden provides only its raw lesser years as a base."));
    mvwprintw(table_win, 18, 4, _("Traditional doctrine shifts these into major 10-year cycles if the native survives infancy."));
    wattroff(table_win, A_DIM);

    // Instruções de Encerramento Padrão
    mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
    wrefresh(table_win);

    keypad(table_win, TRUE);
    nodelay(table_win, FALSE);
    
    int ch;
    do {
        ch = wgetch(table_win);
    } while (ch != 27 && ch != 'q');
    
    // Descarte seguro limpando memória da janela e tocando o stdscr para restaurar o fundo
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr);
    refresh();
}


ResultadoAnareta calcular_anareta(int idx_hileg_grid, AspectMatrix *matrix, PlotObject *plots, PlanetDignities *dig, int signo_casa8) {
    ResultadoAnareta resultado = {"None", "-", 0, 0, "No imminent vital threats found"};
    snprintf(resultado.regra_eleicao, 80, "%s", _("No imminent vital threats found"));

    // Array para armazenar o score de ameaça calculado para cada um dos 7 planetas tradicionais
    // Índices de 1 a 7 (0 descartado)
    int pesos_ameaca[8] = {0}; 

    int id_regente_c8 = obter_regente_tradicional(signo_casa8);

    // ────────────────────────────────────────────────────────────────────────
    // PASSO 1: CALCULAR O PESO DE AMEAÇA DE CADA PLANETA TRADICIONAL
    // ────────────────────────────────────────────────────────────────────────
    for (int j = 0; j < 7; j++) {
        int id_p = j + 1; // Transforma o índice 0-6 do plots no ID do banco 1-7
        int casa_fisica = romanToInt(plots[j].house);
        int idx_matriz_dig = obter_indice_matriz_dig(id_p);

        // A) Posição Corporal em Casas Críticas
        if (casa_fisica == 8) {
            pesos_ameaca[id_p] += 100; // Prioridade máxima: Casa da Morte
        }
        if (casa_fisica == 4) {
            pesos_ameaca[id_p] += 80;  // Segunda prioridade: Fim das Coisas
        }

        // B) Regência da Casa 8
        if (id_p == id_regente_c8) {
            pesos_ameaca[id_p] += 60;  // Dono da casa da morte ganha forte relevância
        }

        // C) Aspectos Hostis (Quadraturas e Oposições) enviados ao Hileg
        if (idx_hileg_grid != -1 && j != idx_hileg_grid) {
            AspectCell c1 = matrix->grid[j][idx_hileg_grid];
            AspectCell c2 = matrix->grid[idx_hileg_grid][j];

            if (c1.has_aspect || c2.has_aspect) {
                // Checa se o aspecto é Quadratura (□) ou Oposição (☍)
                if (strstr(c1.symbol, "□") != NULL || strstr(c2.symbol, "□") != NULL ||
                    strstr(c1.symbol, "☍") != NULL || strstr(c2.symbol, "☍") != NULL) 
                {
                    // Apenas Maléficos naturais (Marte/Saturno) ganham peso por aspecto hostil ao Hileg
                    if (id_p == 5 || id_p == 7) {
                        pesos_ameaca[id_p] += 50;
                    }
                }
            }
        }

        // D) Aflições Acidentais Graves (Vindas da matriz de dignidades atualizada)
        if (idx_matriz_dig != -1) {
            // Se o seu motor marcou o planeta como Combusto
            if (dig[idx_matriz_dig].row.combust == 1) {
                pesos_ameaca[id_p] += 30; // Pior aflição acidental
            }
            // Se o planeta está em Exílio ou Queda (Essential negativo ou checagem de flag)
            //if (dig[idx_matriz_dig].major_dig < 0) {
            if (dig[idx_matriz_dig].essential < 0) {
                pesos_ameaca[id_p] += 20; 
            }
            // Se o planeta está Retrógrado (Checa o texto da sua string de velocidade ou velocidade < 0)
            if (plots[j].speed < 0 || strstr(plots[j].retrograde, "℞") != NULL) {
                pesos_ameaca[id_p] += 15;
            }
        }
    }

    // ────────────────────────────────────────────────────────────────────────
    // PASSO 2: ELEGER O VENCEDOR (O MAIOR SCORE)
    // ────────────────────────────────────────────────────────────────────────
    int max_peso = 0;
    int id_eleito = 0;

    for (int id = 1; id <= 7; id++) {
        // Na busca tradicional, o Anareta precisa ter um score mínimo para agir (ao menos reger a 8 ou estar na 4/8)
        if (pesos_ameaca[id] > max_peso) {
            max_peso = pesos_ameaca[id];
            id_eleito = id;
        }
    }

    // Se o maior peso for zero, significa que o mapa é extremamente equilibrado e não há Anareta ativo por aflição
    if (id_eleito == 0) {
        return resultado;
    }

    // ────────────────────────────────────────────────────────────────────────
    // PASSO 3: COPIAR OS DADOS DO VENCEDOR PARA O RELATÓRIO
    // ────────────────────────────────────────────────────────────────────────
    int idx_plots_vencedor = id_eleito - 1;
    
    resultado.id_anareta = id_eleito;
    resultado.score_ameaca = max_peso;
    strcpy(resultado.name, plots[idx_plots_vencedor].object_name);
    strcpy(resultado.glifo, plots[idx_plots_vencedor].object);

    // Determina textualmente qual foi o gatilho principal da eleição para documentar na UI
    int casa_final = romanToInt(plots[idx_plots_vencedor].house);
    if (casa_final == 8) {
        strcpy(resultado.regra_eleicao, _("Elected via: Corporeal Presence in the 8th House (Precedence)"));
    } else if (casa_final == 4) {
        strcpy(resultado.regra_eleicao, _("Elected via: Corporeal Presence in the 4th House (End of Life)"));
    } else if (id_eleito == id_regente_c8) {
        strcpy(resultado.regra_eleicao, _("Elected via: Lord of the 8th House Cusp"));
    } else {
        strcpy(resultado.regra_eleicao, _("Elected via: Hostile Aspect (Square/Opposition) to Hyleg"));
    }

    return resultado;
}


void display_anareta(PlotObject *plots, AspectMatrix *matrix, PlanetDignities *dig, PontosHylegiacos pontos, int signo_casa8, int week_day, int planetary_hour) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Dimensões ideais para o painel de ameaças
    int table_height = 20;
    int table_width = max_x - 10;
    int start_y = (max_y - table_height) / 2;
    int start_x = 5;

    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    // Inicialização e limpeza das janelas (sombra e fundo)
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9)); box(shadow_win, 0, 0); wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13));

    wattron(table_win, A_BOLD);
    const char *title = _(" Vital Threats: The Anareta ");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);


    int regente_dia = converter_codigo_planeta(get_hour_regent(week_day - 1, (MAPA_DIURNO)?0:12));
    int regente_hora = converter_codigo_planeta(get_hour_regent(week_day - 1, planetary_hour - 1));




    // 1. EXECUÇÃO DO MOTOR MATEMÁTICO EM SEGUNDO PLANO
    int id_almuten_ref = 0;
    int object_diff = show_modern_planets ? 0 : 3;
    
    // Recupera o Hileg calculado pelo sistema para passar as coordenadas de aspectos
    int tipo_h = get_hyleg(pontos, plots, matrix, &id_almuten_ref, regente_dia, regente_hora);
    int idx_hileg_grid = -1;
    
    if (tipo_h == H_SOL) idx_hileg_grid = 0;
    else if (tipo_h == H_LUNA) idx_hileg_grid = 1;
    else if (tipo_h == H_ALMUTEN) idx_hileg_grid = id_almuten_ref - 1;
    else if (tipo_h == H_ALMUTEN_SAN) idx_hileg_grid = id_almuten_ref - 1;
    else {
        for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
            if (tipo_h == H_ASC && plots[i].id == P_ASC - object_diff) { idx_hileg_grid = i; break; }
            if (tipo_h == H_FORTUNA && plots[i].id == P_FORTUNA - object_diff) { idx_hileg_grid = i; break; }
        }
    }

    // Roda a nossa engine aritmética de pesos
    ResultadoAnareta anar = calcular_anareta(idx_hileg_grid, matrix, plots, dig, signo_casa8);
    int id_regente_c8 = obter_regente_tradicional(signo_casa8);

    // 2. RENDERIZAÇÃO DO CABEÇALHO DA INTERFACE CORRIGIDA
    mvwprintw(table_win, 2, 4, _("Target Hyleg:      "));
    wattron(table_win, A_BOLD | COLOR_PAIR(8));
    
    if (tipo_h == H_SOL) wprintw(table_win, _("☉ Sun"));
    else if (tipo_h == H_LUNA) wprintw(table_win, _("☽ Moon"));
    else if (tipo_h == H_ASC) wprintw(table_win, _("ASC (Ascendant Degree)"));
    else if (tipo_h == H_FORTUNA) wprintw(table_win, _("🝴 Part of Fortune"));
    else if (tipo_h == H_ALMUTEN) {
        // CORREÇÃO: Varre o array plots para encontrar o planeta Almuten correspondente
        int idx_almuten_plots = -1;
        for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
            // Se plots.id começa em 0 (0=Sol, 1=Lua...), somamos 1 para bater com a base 1-12
            if ((plots[i].id + 1) == id_almuten_ref) {
                idx_almuten_plots = i;
                break;
            }
        }
        
        if (idx_almuten_plots != -1) {
            // Imprime o glifo real e o nome do planeta (ex: ♀ Venus (Almuten Figuris))
            wprintw(table_win, "%s %s (%s)", 
                    plots[idx_almuten_plots].object, 
                    plots[idx_almuten_plots].object_name,
                    _("Almuten Figuris Protector"));
        } else {
            wprintw(table_win, _("Almuten Figuris Protector"));
        }
    }
    wattroff(table_win, A_BOLD | COLOR_PAIR(8));

    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 3, 4, _("8th House Sign & Ruler:   "));
    wattron(table_win, A_BOLD | COLOR_PAIR(21));
    wprintw(table_win, "%s (%s %s)", get_sign(signo_casa8 - 1), plots[id_regente_c8 - 1].object, plots[id_regente_c8 - 1].object_name);
    wattroff(table_win, A_BOLD | COLOR_PAIR(21));

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 5, 2, "────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    // 3. EXIBIÇÃO DO VEREDITO DO PLANETA DESTRUIDOR (THE CHOSEN ANARETA)
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 7, 4, _("THE CHOSEN ANARETA (Interfector): "));
    
    if (anar.id_anareta > 0) {
        // Alerta visual vermelho para o planeta hostil eleito
        wattron(table_win, COLOR_PAIR(11) | A_BOLD); 
        wprintw(table_win, " %s ", anar.glifo);
        wattroff(table_win, COLOR_PAIR(11) | A_BOLD);
        wprintw(table_win, "%s", anar.name);

        wattron(table_win, A_BOLD);
        mvwprintw(table_win, 9, 6, _("Calculated Threat Score: "));
        wattron(table_win, COLOR_PAIR(11) | A_BOLD | A_UNDERLINE);
        wprintw(table_win, "%d %s", anar.score_ameaca, _("Points"));
        wattroff(table_win, COLOR_PAIR(11) | A_BOLD | A_UNDERLINE);

        wattron(table_win, A_DIM);
        mvwprintw(table_win, 11, 6, "%s", anar.regra_eleicao);
        wattroff(table_win, A_DIM);

        wattron(table_win, COLOR_PAIR(11) | A_BOLD); 
        mvwprintw(table_win, 1, 102, "%s", "Anareta:");
        const char **ascii_art = get_planet_ascii_by_gliph(anar.glifo);

        mvwprintw(table_win, 2, 105, "%s", ascii_art[0]);
        mvwprintw(table_win, 3, 105, "%s", ascii_art[1]);
        mvwprintw(table_win, 4, 105, "%s", ascii_art[2]);
        mvwprintw(table_win, 5, 105, "%s", ascii_art[3]);
        mvwprintw(table_win, 6, 105, "%s", ascii_art[4]);
        mvwprintw(table_win, 7, 105, "%s", ascii_art[5]);

        mvwprintw(table_win, 9, 99, "%s", "Dominus Mortis:");
        const char **ascii_art1 = get_planet_ascii(id_regente_c8);

        mvwprintw(table_win, 10, 105, "%s", ascii_art1[0]);
        mvwprintw(table_win, 11, 105, "%s", ascii_art1[1]);
        mvwprintw(table_win, 12, 105, "%s", ascii_art1[2]);
        mvwprintw(table_win, 13, 105, "%s", ascii_art1[3]);
        mvwprintw(table_win, 14, 105, "%s", ascii_art1[4]);
        mvwprintw(table_win, 15, 105, "%s", ascii_art1[5]);

        wattroff(table_win, COLOR_PAIR(11) | A_BOLD); 
    } else {
        wattron(table_win, COLOR_PAIR(12) | A_BOLD | A_REVERSE); // Verde para indicar mapa seguro
        wprintw(table_win, "None Active");
        wattroff(table_win, COLOR_PAIR(12) | A_BOLD | A_REVERSE);
        wattron(table_win, A_BOLD);
        mvwprintw(table_win, 9, 6, _("Calculated Threat Score: "));
        wattroff(table_win, A_BOLD);
        wprintw(table_win, _("0 Points"));
        mvwprintw(table_win, 11, 6, _("No severe planetary afflictions targeting the vital points."));
    }

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 13, 2, "────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    // Notas de Literatura Astrológica Tradicional (Lilly / Ptolomeu)
    wattron(table_win, A_DIM);
    mvwprintw(table_win, 15, 4, _("Note: The Anareta represents the geometric source of physical or metabolic risk."));
    mvwprintw(table_win, 16, 4, _("Its activation requires explicit conjunctions or hostile primary directions."));
    wattroff(table_win, A_DIM);

    // Comando de encerramento da Janela Pop-up
    mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
    wrefresh(table_win);

    keypad(table_win, TRUE);
    nodelay(table_win, FALSE);
    int ch;
    do {
        ch = wgetch(table_win);
    } while (ch != 27 && ch != 'q');
    
    // Destruição das janelas da memória e restauração do mapa de fundo
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr);
    refresh();
}