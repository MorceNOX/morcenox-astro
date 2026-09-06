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




int obter_indice_matriz_dig(int id_almuten) {
    // Se for um dos 7 planetas tradicionais (IDs de 1 a 7), o índice na matriz dig é direto (ID - 1) -> 0 a 6
    if (id_almuten <= 7) {
        return id_almuten - 1;
    }

    if (show_modern_planets) {
        // Se os modernos estão ativos, o mapeamento continua linear direto:
        // ID 8, 9, 10 (Urano, Netuno, Plutão) -> índices 7, 8, 9
        // ID 11, 12 (Nodos) -> índices 10, 11
        return id_almuten - 1;
    } else {
        // Se os modernos NÃO estão ativos, Urano(8), Netuno(9) e Plutão(10) não existem na matriz dig.
        // O Nodo Norte (ID 11) vira índice 7. O Nodo Sul (ID 12) vira índice 8.
        if (id_almuten == 11) return 7;
        if (id_almuten == 12) return 8;
    }

    return -1; // Caso caia em Urano/Netuno/Plutão com a flag desativada
}



int get_dig_hyleg_points(int id_planeta, PontosHylegiacos pontos) {
    int total_dignidades = 0;
    
    // Vetor com as longitudes dos 5 pontos hylegíacos essenciais
    double longitudes[5] = {pontos.sol_lon, pontos.lua_lon, pontos.asc_lon, pontos.fortuna_lon, pontos.sizigia_lon};

    for (int p = 0; p < 5; p++) {
        double lon = longitudes[p];
        int sign = (int)floor(lon / 30.0) + 1;
        
        int n_ruler, n_exalted, n_exile, n_fall, r1, r2, r3;
        get_rulers_by_sign_id(sign, &n_ruler, &n_exalted, &n_exile, &n_fall, &r1, &r2, &r3);

        int term_ruler = get_term_ruler(lon);    
        int decan = get_decan(lon);
        int decan_ruler = get_decan_ruler(decan);    

        int n_term = converter_codigo_planeta(term_ruler);
        int n_dec = converter_codigo_planeta(decan_ruler);

        // Incrementa se o planeta possuir qualquer uma das 5 dignidades essenciais positivas neste ponto
        if (id_planeta == n_ruler) total_dignidades++;
        if (id_planeta == n_exalted) total_dignidades++;
        if (id_planeta == r1) total_dignidades++;
        if (id_planeta == r2) total_dignidades++;
        if (id_planeta == r3) total_dignidades++;
        if (id_planeta == n_term) total_dignidades++;
        if (id_planeta == n_dec) total_dignidades++;
    }

    return total_dignidades;
}


int get_governed_points_count(int id_planeta, PlotObject *plots) {
    int total_governança = 0;
    int object_diff = show_modern_planets ? 0 : 3;

    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        
        // Ignoramos apenas o Vertex
        // Planetas modernos serão considerados caso estejam sendo usados
        // if (i > 6 && plots[i].id != P_VERTEX - object_diff) {
        //     continue;
        // }

        int n_ruler, n_exalted, n_exile, n_fall, r1, r2, r3;
        int sign = (int)floor(plots[i].longitude / 30.0) + 1;
        
        get_rulers_by_sign_id(sign, &n_ruler, &n_exalted, &n_exile, &n_fall, &r1, &r2, &r3);

        // Se o planeta que estamos testando governar este ponto por Domicílio ou Exaltação, pontua!
        if (id_planeta == n_ruler) total_governança++;
        if (id_planeta == n_exalted) total_governança++;
    }

    return total_governança;
}



// 1. FUNÇÃO INDIVIDUAL DE PONTOS HYLEGÍACOS (Retorna o Almuten de uma Longitude Única)
int get_almuten(double longitude, int *resultados, AspectMatrix *aspecto_matriz, PontosHylegiacos pontos, PlotObject *plots) {
    int n_ruler, n_exalted, n_exile, n_fall, r1, r2, r3;
    int sign = (int)floor(longitude / 30.0) + 1;
    
    get_rulers_by_sign_id(sign, &n_ruler, &n_exalted, &n_exile, &n_fall, &r1, &r2, &r3);

    int term_ruler = get_term_ruler(longitude);    
    int decan = get_decan(longitude);
    int decan_ruler = get_decan_ruler(decan);    

    int n_term = converter_codigo_planeta(term_ruler);
    int n_dec = converter_codigo_planeta(decan_ruler);

    // Vetor de tamanho 13 para usar os índices de 1 a 12 diretamente de forma segura
    int table_local[13] = {0};

    // Almuten de ponto pontua positivo
    if (n_ruler >= 1 && n_ruler <= 12)     table_local[n_ruler] += 5;
    if (n_exalted >= 1 && n_exalted <= 12) table_local[n_exalted] += 4;

    if (r1 >= 1 && r1 <= 12) table_local[r1] += 3;  
    if (r2 >= 1 && r2 <= 12) table_local[r2] += 3;  
    if (r3 >= 1 && r3 <= 12) table_local[r3] += 3;  
    
    if (n_term >= 1 && n_term <= 12) table_local[n_term] += 2;
    if (n_dec >= 1 && n_dec <= 12)   table_local[n_dec] += 1;

    int max_value = -999;
    for (int i = 1; i <= 12; i++) {
        if (table_local[i] > max_value) max_value = table_local[i];
    }

    int candidatos[12] = {0};
    int qtd_candidatos = 0;
    for (int i = 1; i <= 12; i++) {
        if (table_local[i] == max_value) {
            candidatos[qtd_candidatos] = i;
            qtd_candidatos++;
        }
    }

    if (qtd_candidatos == 1) {
        resultados[0] = candidatos[0];
        return 1;
    }

    // --- CASCATA DE DESEMPATE DO PONTO ATUALIZADA (VERSÃO COMPLETA CLÁSSICA) ---
    int max_num_aspects = -999, max_dig_hyleg = -999, max_gov_points = -999;
    int qtd_vencedores = 0;

    int object_diff = show_modern_planets ? 0 : 3;

    for (int k = 0; k < qtd_candidatos; k++) {
        int id_planeta = candidatos[k];
        int idx = obter_indice_matriz_dig(id_planeta);
        if (idx == -1) continue;

        int num_aspects_atual = 0;
        
        // 1º Critério: Dignidades nos pontos Hylegíacos
        int dig_hyleg = get_dig_hyleg_points(id_planeta, pontos);
        
        // 2º Critério: Quantidade de pontos governados por Domicílio/Exaltação
        int gov_points = get_governed_points_count(id_planeta, plots);

        // 3º Critério: Quantidade total de aspectos em que se envolve
        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            if (idx != -1 && j != idx) {
                AspectCell c1 = aspecto_matriz->grid[j][idx];
                AspectCell c2 = aspecto_matriz->grid[idx][j];
    
                if (c1.has_aspect || c2.has_aspect) {
                    num_aspects_atual++;
                }
            }
        }

        bool substituir = false, empatar = false;
        if (k == 0) substituir = true;
        else {
            // 1º Filtro: Dignidades Hylegíacas
            if (dig_hyleg > max_dig_hyleg) substituir = true;
            else if (dig_hyleg == max_dig_hyleg) {
                
                // 2º Filtro: NOVO - Pontos Governados (Domicílio + Exaltação)
                if (gov_points > max_gov_points) substituir = true;
                else if (gov_points == max_gov_points) {
                    
                    // 3º Filtro: Quantidade total de aspectos
                    if (num_aspects_atual > max_num_aspects) substituir = true;
                    else if (num_aspects_atual == max_num_aspects) {
                        empatar = true;
                        
                    }
                }
            }
        }

        if (substituir) {
            max_dig_hyleg = dig_hyleg;
            max_gov_points = gov_points; // Salva o novo recorde de governança
            max_num_aspects = num_aspects_atual;
            
            resultados[0] = id_planeta;
            qtd_vencedores = 1;
        } else if (empatar) {
            resultados[qtd_vencedores] = id_planeta;
            qtd_vencedores++;
        }
    }
    return qtd_vencedores;
}



// 2. FUNÇÃO AUXILIAR PARA O ALMUTEN FIGURIS (Acumula APENAS Dignidades Positivas)
void acumular_dignidades_figuris(double longitude, int *tabela_figuris) {
    int n_ruler, n_exalted, n_exile, n_fall, r1, r2, r3;
    int sign = (int)floor(longitude / 30.0) + 1;
    
    get_rulers_by_sign_id(sign, &n_ruler, &n_exalted, &n_exile, &n_fall, &r1, &r2, &r3);

    int term_ruler = get_term_ruler(longitude);    
    int decan = get_decan(longitude);
    int decan_ruler = get_decan_ruler(decan);    

    int n_term = converter_codigo_planeta(term_ruler);
    int n_dec = converter_codigo_planeta(decan_ruler);

    // IMPORTANTE: Almuten Figuris acumula apenas as 5 dignidades essenciais positivas (sem subtrair exílio/queda)
    if (n_ruler >= 1 && n_ruler <= 12)     tabela_figuris[n_ruler] += 5;
    if (n_exalted >= 1 && n_exalted <= 12) tabela_figuris[n_exalted] += 4;
    
    if (r1 >= 1 && r1 <= 12) tabela_figuris[r1] += 3;
    if (r2 >= 1 && r2 <= 12) tabela_figuris[r2] += 3;
    if (r3 >= 1 && r3 <= 12) tabela_figuris[r3] += 3;

    if (n_term >= 1 && n_term <= 12)       tabela_figuris[n_term] += 2;
    if (n_dec >= 1 && n_dec <= 12)         tabela_figuris[n_dec] += 1;
}


int get_almuten_multiplo(double *longitudes, int qtd_longitudes, int *resultados, AspectMatrix *aspecto_matriz, PontosHylegiacos pontos, PlotObject *plots) {
    // Vetor mestre para acumular as pontuações de todas as longitudes enviadas
    int table_global[13] = {0};

    // 1. Varre todas as longitudes e acumula as dignidades essenciais na mesma tabela
    for (int i = 0; i < qtd_longitudes; i++) {
        acumular_dignidades_figuris(longitudes[i], table_global);
    }

    // 2. Encontra a maior pontuação essencial atingida no total combinado
    int max_value = -999;
    for (int i = 1; i <= 12; i++) {
        if (table_global[i] > max_value) max_value = table_global[i];
    }

    // 3. Isola os candidatos que atingiram o topo essencial
    int candidatos[12] = {0};
    int qtd_candidatos = 0;
    for (int i = 1; i <= 12; i++) {
        if (table_global[i] == max_value) {
            candidatos[qtd_candidatos] = i;
            qtd_candidatos++;
        }
    }

    // Se houver apenas 1 vencedor isolado nas essenciais somadas, retorna direto
    if (qtd_candidatos == 1) {
        resultados[0] = candidatos[0];
        return 1;
    }

    // --- 4. CASCATA DE DESEMPATE ORIGINAL APLICADA AO TOTAL ---
    int max_num_aspects = -999, max_dig_hyleg = -999, max_gov_points = -999;
    int qtd_vencedores = 0;

    int object_diff = show_modern_planets ? 0 : 3;

    for (int k = 0; k < qtd_candidatos; k++) {
        int id_planeta = candidatos[k];
        int idx = obter_indice_matriz_dig(id_planeta);
        if (idx == -1) continue;

        int num_aspects_atual = 0;
        int dig_hyleg = get_dig_hyleg_points(id_planeta, pontos);
        int gov_points = get_governed_points_count(id_planeta, plots);

        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            if (idx != -1 && j != idx) {
                AspectCell c1 = aspecto_matriz->grid[j][idx];
                AspectCell c2 = aspecto_matriz->grid[idx][j];
                if (c1.has_aspect || c2.has_aspect) {
                    num_aspects_atual++;
                }
            }
        }

        bool substituir = false, empatar = false;
        if (k == 0) substituir = true;
        else {
            if (dig_hyleg > max_dig_hyleg) substituir = true;
            else if (dig_hyleg == max_dig_hyleg) {
                if (gov_points > max_gov_points) substituir = true;
                else if (gov_points == max_gov_points) {
                    if (num_aspects_atual > max_num_aspects) substituir = true;
                    else if (num_aspects_atual == max_num_aspects) {
                        empatar = true;
                    
                    }
                }
            }
        }

        if (substituir) {
            max_dig_hyleg = dig_hyleg;
            max_gov_points = gov_points; 
            max_num_aspects = num_aspects_atual;
            
            resultados[0] = id_planeta;
            qtd_vencedores = 1;
        } else if (empatar) {
            resultados[qtd_vencedores] = id_planeta;
            qtd_vencedores++;
        }
    }
    return qtd_vencedores;
}


// Função auxiliar para retornar a pontuação clássica de força baseada na Casa Astrológica (1 a 12)
int obter_pontos_por_casa(int casa) {
    switch (casa) {
        case 1:  case 10: return 12; // Ângulos primordiais
        case 4:  case 7:  case 11: return 10; // Outros ângulos e a melhor sucedente
        case 2:  case 5:  return 8;  // Casas sucedentes normais
        case 9:           return 6;  // Cadente favorável (Deus)
        case 3:           return 5;  // Cadente moderada (Deusa)
        case 6:  case 8:  return 4;  // Casas maléficas
        case 12:          return 3;  // A pior casa cadente (Gênio Mau)
        default:          return 0;
    }
}



int calcular_almuten_figuris(PontosHylegiacos pontos, PlotObject *plots, AspectMatrix *aspecto_matriz, int regente_dia, int regente_hora, int *resultado_figuris) {
    // Tabela global do Almuten Figuris (índices de 1 a 12 de forma segura)
    int tabela_figuris[13] = {0};

    // 1. Acumula os pesos de dignidades essenciais nos 5 lugares hylegíacos
    acumular_dignidades_figuris(pontos.sol_lon, tabela_figuris);
    acumular_dignidades_figuris(pontos.lua_lon, tabela_figuris);
    acumular_dignidades_figuris(pontos.asc_lon, tabela_figuris);
    acumular_dignidades_figuris(pontos.fortuna_lon, tabela_figuris);
    acumular_dignidades_figuris(pontos.sizigia_lon, tabela_figuris);

    // 2. NOVO: Adiciona os pontos do Regente do Dia (+7) e Regente da Hora (+6)
    tabela_figuris[regente_dia] += 7;
    tabela_figuris[regente_hora] += 6;

    // 3. NOVO: Adiciona a pontuação clássica pela posição física nas Casas Astrológicas
    int object_diff = show_modern_planets ? 0 : 3;
    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        // Ignora os planetas modernos ou secundários se a flag mandar pular
        if ((show_modern_planets && (i >= 14 && i <= 17)) || 
            (!show_modern_planets && (i >= 13 && i <= 14))) {
            continue;
        }

        // Descobre o ID (1 a 12) do objeto atual sendo varrido na lista 'plots'
        int id_planeta = plots[i].id + 1;
        if (i > 6) {
            id_planeta = plots[i].id + object_diff + 1;
        } 

        if (id_planeta >= 1 && id_planeta <= 12) {
            int num_casa = 0;

            num_casa = romanToInt(plots[i].house);
                        
            if (num_casa >= 1 && num_casa <= 12) {
                tabela_figuris[id_planeta] += obter_pontos_por_casa(num_casa);
            }
        }
    }

    // 4. Encontra a maior pontuação absoluta na tabela compendiada do Figuris
    int max_pontos = -999;
    for (int i = 1; i <= 12; i++) {
        if (tabela_figuris[i] > max_pontos) {
            max_pontos = tabela_figuris[i];
        }
    }

    // 5. Coleta os candidatos que empataram no topo
    int candidatos[12] = {0};
    int qtd_candidatos = 0;
    for (int i = 1; i <= 12; i++) {
        if (tabela_figuris[i] == max_pontos) {
            candidatos[qtd_candidatos] = i;
            qtd_candidatos++;
        }
    }

    // Se não houver empate, temos o Almuten Figuris definitivo de forma direta
    if (qtd_candidatos == 1) {
        resultado_figuris[0] = candidatos[0];
        return 1;
    }

    // --- CASCATA DE DESEMPATE DO PONTO ATUALIZADA (VERSÃO COMPLETA CLÁSSICA) ---
    int max_num_aspects = -999, max_dig_hyleg = -999, max_gov_points = -999;
    int qtd_vencedores = 0;

    for (int k = 0; k < qtd_candidatos; k++) {
        int id_planeta = candidatos[k];
        int idx = obter_indice_matriz_dig(id_planeta);
        if (idx == -1) continue;

        int num_aspects_atual = 0;
        
        // 1º Critério: Dignidades nos pontos Hylegíacos
        int dig_hyleg = get_dig_hyleg_points(id_planeta, pontos);
        
        // 2º Critério: Quantidade de pontos governados por Domicílio/Exaltação
        int gov_points = get_governed_points_count(id_planeta, plots);

        // 3º Critério: Quantidade total de aspectos em que se envolve
        for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
            if (idx != -1 && j != idx) {
                AspectCell c1 = aspecto_matriz->grid[j][idx];
                AspectCell c2 = aspecto_matriz->grid[idx][j];
    
                if (c1.has_aspect || c2.has_aspect) {
                    num_aspects_atual++;
                }
            }
        }

        bool substituir = false, empatar = false;
        if (k == 0) substituir = true;
        else {
            // 1º Filtro: Dignidades Hylegíacas
            if (dig_hyleg > max_dig_hyleg) substituir = true;
            else if (dig_hyleg == max_dig_hyleg) {
                
                // 2º Filtro: NOVO - Pontos Governados (Domicílio + Exaltação)
                if (gov_points > max_gov_points) substituir = true;
                else if (gov_points == max_gov_points) {
                    
                    // 3º Filtro: Quantidade total de aspectos
                    if (num_aspects_atual > max_num_aspects) substituir = true;
                    else if (num_aspects_atual == max_num_aspects) {
                        empatar = true;
                        
                    }
                }
            }
        }

        if (substituir) {
            max_dig_hyleg = dig_hyleg;
            max_gov_points = gov_points; // Salva o novo recorde de governança
            max_num_aspects = num_aspects_atual;
            
            
            resultado_figuris[0] = id_planeta;
            qtd_vencedores = 1;
        } else if (empatar) {
            resultado_figuris[qtd_vencedores] = id_planeta;
            qtd_vencedores++;
        }
    }
    return qtd_vencedores;
}



void display_almutens(PontosHylegiacos pontos, PlotObject *plots, AspectMatrix *aspecto_matriz, int week_day, int planetary_hour, bool mapa_retorno) {
    int regente_dia = converter_codigo_planeta(get_hour_regent(week_day - 1, (MAPA_DIURNO)?0:12));
    int regente_hora = converter_codigo_planeta(get_hour_regent(week_day - 1, planetary_hour - 1));

    // Calcular as dimensões da nova janela
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 24; 
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = 2;
    
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
    const char *title = _("Hylegiacal & Figuris Almutens");
    // Adiciona o Título da Tela
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);

    // Cabeçalho da tabela
    mvwprintw(table_win, 1, 2, _("   Hylegiacal Point       Longitude       Calculated Almuten                             Almuten Figuris"));
    wattroff(table_win, A_BOLD);

    // Arrays para receber as respostas dos cálculos
    int qtd;
    int row = 3;

    // Definição e processamento estático dos 5 pontos hylegíacos
    const char *nomes_pontos[5] = {_("Sol "), _("Luna"), _("Ascendant"), _("Part of Fortune"), _("Previous Syzygy")};
    double lons_pontos[5] = {pontos.sol_lon, pontos.lua_lon, pontos.asc_lon, pontos.fortuna_lon, pontos.sizigia_lon};

    for (int i = 0; i < 5; i++) {
        wattron(table_win, COLOR_PAIR(10) | A_DIM);
        mvwprintw(table_win, row - 1, 2, "────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(10) | A_DIM);

        // Processa graus e minutos da longitude do ponto hylegíaco
        double sign_remainder = fmod(lons_pontos[i], 30.0);
        int deg = (int)sign_remainder;        
        double min = (int)((sign_remainder - deg) * 60.0);
        const char *sign_str = get_sign((int)(lons_pontos[i] / 30));
    
        int res_almuten[12] = {0};
        qtd = get_almuten(lons_pontos[i], res_almuten, aspecto_matriz, pontos, plots);
    
        char almuten_str[50] = "";
        if (qtd == 1) {
            snprintf(almuten_str, sizeof(almuten_str), "%s", obter_glifo_planeta_por_id(res_almuten[0]));
        } else {
            snprintf(almuten_str, sizeof(almuten_str), "%s/%s (%s)", 
                     obter_glifo_planeta_por_id(res_almuten[0]), obter_glifo_planeta_por_id(res_almuten[1]), _("Tie"));
        }
    

        // Desenha a linha na janela
        mvwprintw(table_win, row, 5, "%-25s %02d° %s %02.0f'      ", nomes_pontos[i], deg, sign_str, min);
        
        // Destaca o planeta vencedor aplicando o Glifo Unicode em Negrito e com a Cor Azul (Par 8)
        wattron(table_win, COLOR_PAIR(8) | A_BOLD);
        wprintw(table_win, "%s", almuten_str);
        wattroff(table_win, COLOR_PAIR(8) | A_BOLD);

        row += 2;
    }
    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, row - 1, 2, "────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    wattron(table_win, A_BOLD);
    mvwprintw(table_win, row + 1, 5, "%s %s(%d)/%s(%d): ", _("Planetary"), (MAPA_DIURNO)?_("Day"):_("Night"), week_day, _("Hour"), planetary_hour);
    wattroff(table_win, A_BOLD);

    wattron(table_win, COLOR_PAIR(8) | A_BOLD);
    wprintw(table_win, "%s / %s", planet_regent_symbols[get_hour_regent(week_day - 1, (MAPA_DIURNO)?0:12)], planet_regent_symbols[get_hour_regent(week_day - 1, planetary_hour - 1)]);
    wattroff(table_win, COLOR_PAIR(8) | A_BOLD);

    // ────────────────────────────────────────────────────────────────────────────────
    // SEÇÃO INFERIOR: Exibição Destacada do Almuten Figuris
    // ────────────────────────────────────────────────────────────────────────────────
    wattron(table_win, COLOR_PAIR(6));
    mvwprintw(table_win, row + 3, 2, "────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(6));

    int res_figuris[12];
    
    int qtd_figuris = calcular_almuten_figuris(pontos, plots, aspecto_matriz, regente_dia, regente_hora, res_figuris);

    wattron(table_win, A_BOLD);
    mvwprintw(table_win, row + 4, 5, _("ALMUTEN FIGURIS (Lord of the Chart): "));
    wattroff(table_win, A_BOLD);

    // Exibe o Glifo Unicode do Almuten Figuris com destaque sublinhado
    if (qtd_figuris == 1) {
        wattron(table_win, COLOR_PAIR(8) | A_BOLD);
        wprintw(table_win, " %s ", obter_glifo_planeta_por_id(res_figuris[0]));
        wattroff(table_win, COLOR_PAIR(8) | A_BOLD);

        const char **ascii_art = get_planet_ascii(res_figuris[0]);
        
        wattron(table_win, COLOR_PAIR(7) | A_BOLD);
        mvwprintw(table_win, 2, 95, "%s", ascii_art[0]);
        mvwprintw(table_win, 3, 95, "%s", ascii_art[1]);
        mvwprintw(table_win, 4, 95, "%s", ascii_art[2]);
        mvwprintw(table_win, 5, 95, "%s", ascii_art[3]);
        mvwprintw(table_win, 6, 95, "%s", ascii_art[4]);
        mvwprintw(table_win, 7, 95, "%s", ascii_art[5]);

        mvwprintw(table_win, 8, 92, "%s", obter_nome_planeta_por_id(res_figuris[0]));
    } else {
        wattron(table_win, COLOR_PAIR(11) | A_BOLD);
        wprintw(table_win, " %s & %s (Tie) ", obter_glifo_planeta_por_id(res_figuris[0]), obter_glifo_planeta_por_id(res_figuris[1]));
        wattroff(table_win, COLOR_PAIR(11) | A_BOLD);

        const char **ascii_art1 = get_planet_ascii(res_figuris[0]);
        const char **ascii_art2 = get_planet_ascii(res_figuris[1]);
        
        wattron(table_win, COLOR_PAIR(7) | A_BOLD);
        mvwprintw(table_win, 2, 90, "%s  %s", ascii_art1[0], ascii_art2[0]);
        mvwprintw(table_win, 3, 90, "%s  %s", ascii_art1[1], ascii_art2[1]);
        mvwprintw(table_win, 4, 90, "%s  %s", ascii_art1[2], ascii_art2[2]);
        mvwprintw(table_win, 5, 90, "%s  %s", ascii_art1[3], ascii_art2[3]);
        mvwprintw(table_win, 6, 90, "%s  %s", ascii_art1[4], ascii_art2[4]);
        mvwprintw(table_win, 7, 90, "%s  %s", ascii_art1[5], ascii_art2[5]);

        mvwprintw(table_win, 8, 90, "%s & %s", obter_nome_planeta_por_id(res_figuris[0]), obter_nome_planeta_por_id(res_figuris[1]));
    }
    wattroff(table_win, COLOR_PAIR(7) | A_BOLD);

    // Instruções de encerramento da janela
    mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart - [i] to open interpretation window"));
    
    wnoutrefresh(table_win);

    doupdate();

    keypad(table_win, TRUE);
    nodelay(table_win, FALSE);
    
    int ch;
    do {
        ch = wgetch(table_win);
        
        /* GATILHO: Se pressionar 'i', abre o relatório corrido */
        if (ch == 'i' || ch == 'I') {
            if (!mapa_retorno) {
                abrir_janela_interpretacao_almuten(res_figuris,  qtd);
            } 
            else {
                abrir_janela_interpretacao_almuten_revolucao(res_figuris, qtd);
            }
            
            /* Ao fechar o relatório, redesenha a janela do painel para limpar resíduos */
            
            touchwin(stdscr);
            refresh();
            touchwin(shadow_win);
            wnoutrefresh(shadow_win);
            touchwin(table_win);
            wnoutrefresh(table_win);

            doupdate();
        }

    } while (ch != 27 && ch != 'q' && ch != 'Q');
    
    // Liberação e descarte da janela (restaurando o conteúdo de fundo)
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr); 
    refresh();
}



void abrir_janela_interpretacao_almuten(int res_almuten[12], int qtd_vencedores) {
    int p_max_y, p_max_x;
    getmaxyx(stdscr, p_max_y, p_max_x); 

    // 1. DIMENSIONAMENTO RESPONSIVO DA JANELA
    int i_height = p_max_y - 6;
    if (i_height > 26) i_height = 26; 
    int i_width = p_max_x - 12;
    if (i_width > 102) i_width = 102;   

    int i_start_y = (p_max_y - i_height) / 2;
    int i_start_x = (p_max_x - i_width) / 2;

    // 2. CRIAÇÃO E RENDERIZAÇÃO DA JANELA DE SOMBRA (FUNDO)
    WINDOW *shadow_win = newwin(i_height, i_width, i_start_y + 1, i_start_x + 1);
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9)); 
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);

    // 3. CRIAÇÃO DA MOLDURA PRINCIPAL
    WINDOW *border_win = newwin(i_height, i_width, i_start_y, i_start_x);
    wbkgd(border_win, COLOR_PAIR(13) | FLAGS);
    box(border_win, 0, 0);
    
    wattron(border_win, A_BOLD);
    const char *title = _(" Almuten Figuris Spiritual Guide ");
    mvwprintw(border_win, 0, (i_width - get_visual_width(title)) / 2, title);
    wattroff(border_win, A_BOLD);
    
    mvwprintw(border_win, i_height - 1, (i_width - 44) / 2, _(" [↓↑|JK: Scroll | Q|ESC: Return to Chart] "));
    wnoutrefresh(border_win);

    doupdate();

    // 4. CRIAÇÃO DA PAD INTERNA PARA SCROLL
    int pad_lines = 180; // Espaço vertical estendido para casos de múltiplos planetas
    int pad_cols = i_width - 6; 
    WINDOW *pad = newpad(pad_lines, pad_cols);
    wbkgd(pad, COLOR_PAIR(13) | FLAGS);
    keypad(pad, TRUE);
    idlok(pad, TRUE);
    scrollok(pad, TRUE);

    char str[512];

    wprintw(pad, "\n");

    int line_count = 1;

    // --- PREÂMBULO ESPIRITUAL ---
    wattron(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, _(" THE LORD OF THE CHART (ALMUTEN FIGURIS) \n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    
    line_count += 2;

    snprintf(str, 512, _("The Almuten Figuris acts as the supreme ruler of your natal chart, "
                 "representing the evolutionary helmsman of your soul. While the Ascendant "
                 "governs the physical vessel, the Almuten dictates the highest spiritual "
                 "purpose, latent talents, and the path to inner mastery.\n\n"));

    line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);
    
    // TRATAMENTO DE CO-REGÊNCIA (CASOS DE EMPATE)
    if (qtd_vencedores > 1) {
        wattron(pad, A_BOLD | COLOR_PAIR(11));
        wprintw(pad, _("CO-REGENCY DETECTED: Spiritual Conjunction\n\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(11));
        
        line_count += 2;
        
        snprintf(str, 512,  _("Your chart presents a rare cosmic occurrence: a shared throne. Multiple "
                     "archaic planetary forces balance each other perfectly, demanding that you "
                     "integrate both streams of consciousness to achieve your destiny.\n\n"));
    
        line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

    }

    // 5. LOOP DE RENDERIZAÇÃO DOS PLANETAS VENCEDORES
    for (int p = 0; p < qtd_vencedores; p++) {
        int id_planeta = res_almuten[p];
        const char *glifo = obter_glifo_planeta_por_id(id_planeta);

        wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
        
        line_count++;

        wattron(pad, A_BOLD | COLOR_PAIR(32) | A_REVERSE);
        wprintw(pad, " %s  %s ", glifo, _("ARCHETYPAL RULER"));
        
        // Define o título nominal baseado no ID astronômico da Swiss Ephemeris
        if (id_planeta == 1) wprintw(pad, _("(THE SUN - ☉)\n"));
        else if (id_planeta == 2) wprintw(pad, _("(THE MOON - ☽)\n"));
        else if (id_planeta == 3) wprintw(pad, _("(MERCURY - ☿)\n"));
        else if (id_planeta == 4) wprintw(pad, _("(VENUS - ♀)\n"));
        else if (id_planeta == 5) wprintw(pad, _("(MARS - ♂)\n"));
        else if (id_planeta == 6) wprintw(pad, _("(JUPITER - ♃)\n"));
        else if (id_planeta == 7) wprintw(pad, _("(SATURN - ♄)\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(32) | A_REVERSE);
        wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n\n");

        line_count += 3;

        // IMPRESSÃO DOS TEXTOS DE ACORDO COM O ID DO PLANETA
        if (id_planeta == 1) { // SOL
            wattron(pad, A_BOLD); wprintw(pad, _("Spiritual Essence:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("The soul seeks the expression of pure individuality, truth, and integrity. "
                         "There is an intrinsic calling to become a source of light, clarity, and "
                         "centrality for yourself and others.\n\n"));
            
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); wprintw(pad, _("Character Impact:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;
            
            snprintf(str, 512,  _("Confers natural nobility, personal magnetism, generosity, and a sharp "
                         "sense of destiny or mission. The native leads through dignified actions.\n\n"));
            
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); wprintw(pad, _("Evolutionary Hurdles:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512,  _("Avoid the trap of excessive pride, egocentrism, vanity, or a neurotic "
                         "need for constant external validation and applause.\n\n\n"));

            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

        }
        else if (id_planeta == 2) { // LUA
            wattron(pad, A_BOLD); wprintw(pad, _("Spiritual Essence:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("The spiritual path is realized through nurturing, cosmic alignment, and a "
                         "deep connection to the emotional tides of the world. The soul protects.\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); wprintw(pad, _("Character Impact:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("Grants extreme empathy, razor-sharp intuition, a bond with ancestry, "
                         "and a rich imagination. Highly adept at reading environmental atmospheres.\n\n"));
            
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); wprintw(pad, _("Evolutionary Hurdles:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("Mastering unstable mood swings, hypersensitivity, over-attachment to the "
                         "past, and a natural tendency to slide into passive dependency.\n\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

        }
        else if (id_planeta == 3) { // MERCÚRIO
            wattron(pad, A_BOLD); wprintw(pad, _("Spiritual Essence:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("Evolution occurs via the intellect, the translation of reality, and the "
                         "unraveling of the laws that weave and connect all structural data.\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); wprintw(pad, _("Character Impact:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("Brings a brilliant mind, precise adaptability, and an elite talent for "
                         "the written or spoken word. An eternal learner and master problem solver.\n\n"));

            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); wprintw(pad, _("Evolutionary Hurdles:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("Guarding against nervous system exhaustion, oscillating opinions, "
                         "intellectual duplicity, and a tendency to rationalize true emotions.\n\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

        }
        else if (id_planeta == 4) { // VÊNUS
            wattron(pad, A_BOLD); wprintw(pad, _("Spiritual Essence:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("The soul seeks beauty, balance, absolute justice, love, and the sacred "
                         "reconciliation of opposites. Growth mirrors through deep relationships.\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); wprintw(pad, _("Character Impact:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("Confers authentic charm, innate diplomacy, refined aesthetic taste, and "
                         "magnetic attraction. You attract assets, art, and peace with comfort.\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); wprintw(pad, _("Evolutionary Hurdles:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("Evading superficiality, self-indulgence, a chronic fear of confrontation "
                         "(which breeds false diplomacy), and codependency.\n\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

        }
        else if (id_planeta == 5) { // MARTE
            wattron(pad, A_BOLD); wprintw(pad, _("Spiritual Essence:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("The soul refines itself through friction, righteous combat, courage, "
                         "and severing stagnant attachments. Vital force demands protection of the weak.\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); wprintw(pad, _("Character Impact:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("Grants unyielding determination, executive focus, wild independence, and "
                         "pioneering drives. The native actively thrives under extreme pressure.\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); wprintw(pad, _("Evolutionary Hurdles:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("Channelling destructive wrath, chronic impatience, verbal aggression, "
                         "and the base impulse to act tyrannically or purely out of self-interest.\n\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

        }
        else if (id_planeta == 6) { // JÚPITER
            wattron(pad, A_BOLD); wprintw(pad, _("Spiritual Essence:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("The journey is bound to higher consciousness expansion, unwavering faith "
                         "in natural law, philosophy, and magnifying the spirit of benevolence.\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); wprintw(pad, _("Character Impact:\n\n")); wattroff(pad, A_BOLD);
            line_count += 2;

            snprintf(str, 512, _("Brings infectious optimism, a broad structural worldview, accidental luck, "
                         "and deep ethical codes. Acts as a natural, comforting counselor.\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);


            wattron(pad, A_BOLD); 
            wprintw(pad, _("Evolutionary Hurdles:\n\n")); 
            line_count += 2;
            wattroff(pad, A_BOLD);
            
            snprintf(str, 512, _("Controlling the slope toward excess (dogmatism, over-spending), naive "
                        "optimism that miscalculates physical danger, and intellectual arrogance.\n\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

        }
        else if (id_planeta == 7) { // SATURNO
            wattron(pad, A_BOLD); 
            wprintw(pad, _("Spiritual Essence:\n\n")); 
            line_count += 2;
            wattroff(pad, A_BOLD);
            
            snprintf(str, 512, _("The soul elected absolute mastery via self-discipline, heavy responsibility, "
                        "the laws of time, hard boundaries, and wisdom etched directly into stone.\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

    
            wattron(pad, A_BOLD); 
            wprintw(pad, _("Character Impact:\n\n")); 
            line_count += 2;
            wattroff(pad, A_BOLD);
            
            snprintf(str, 512, _("Confers exceptional sobriety, titanic resilience, strategic patience, and "
                        "pragmatism. Built to harvest true success and authority in maturity.\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

    
            wattron(pad, A_BOLD); 
            wprintw(pad, _("Evolutionary Hurdles:\n\n")); 
            line_count += 2;
            wattroff(pad, A_BOLD);
            
            snprintf(str, 512, _("Avoiding chronic pessimism, paralyzing melancholy, fear of material failure, "
                        "mental rigidity, and crushing yourself under unearned guilt.\n\n\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

        }
    }
    
    wattron(pad, A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wprintw(pad, _("  [NARRATIVE END] - Press 'Q' or ESC to return to the Almuten panel.\n"));
    wattroff(pad, A_DIM);
    
    line_count += 3;

    // 6. LOOP DE INTERAÇÃO E SCROLL DA PAD
    int pad_line_pos = 0;
    //int max_scroll_y = (qtd_vencedores > 1) ? 90 : 45; // Adapta a profundidade de scroll
    int ch;
    
   
    // Altura visível real onde o texto do pad aparece na tela
    int visible_height = (i_start_y + i_height - 2) - (i_start_y + 1) + 1;
    
    // A altura física da scrollbar deve bater com o espaço vertical interno da border_win
    int scrollbar_height = i_height - 2; 

    // Habilita as setas do teclado na janela de borda para o wgetch capturar corretamente
    keypad(border_win, TRUE);

    while (1) {
        // --- 1. CÁLCULO E DESENHO DA SCROLLBAR ---
        if (line_count > visible_height) {
            // Posição proporcional baseada em qual linha estamos (pad_line_pos) 
            // sobre o total que pode ser rolado (line_count - visible_height)
            int max_scroll = line_count - visible_height;
            int scrollbar_pos = (pad_line_pos * (scrollbar_height - 1)) / max_scroll;
            
            for (int i = 0; i < scrollbar_height; i++) {
                if (i == scrollbar_pos) {
                    mvwaddch(border_win, 1 + i, i_width - 2, ACS_BLOCK); // Indicador
                } else {
                    mvwaddch(border_win, 1 + i, i_width - 2, ACS_VLINE); // Linha guia de fundo
                }
            }
        }

        // --- 2. ENVIAR JANELAS PARA O BUFFER (Ordem correta de renderização) ---
        wnoutrefresh(border_win); 
        // prefresh envia os dados do pad diretamente para a tela virtual
        prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 3, i_start_x + i_width - 4);
        doupdate(); // Executa a pintura unificada na tela física

        // --- 3. CAPTURA DE INPUT (Na border_win, não no pad) ---
        ch = wgetch(border_win);
        if (ch == 27 || ch == 'q' || ch == 'Q') {
            break;
        }

        // --- 4. TRATAMENTO DA ROLAGEM ---
        switch (ch) {
            case KEY_UP: case 'k': case 'K': 
                if (pad_line_pos > 0) pad_line_pos--; 
                break;
            case KEY_DOWN: case 'j': case 'J': 
                // Não permite rolar além da última página de texto visível
                if (pad_line_pos < (line_count - visible_height)) pad_line_pos++; 
                break;
        }
    }
    
    // 7. LIMPEZA COMPLETA DA MEMÓRIA
    delwin(pad);
    delwin(border_win);
    delwin(shadow_win);

}



void abrir_janela_interpretacao_almuten_revolucao(int res_almuten[12], int qtd_vencedores) {
    int p_max_y, p_max_x;
    getmaxyx(stdscr, p_max_y, p_max_x); 

    // 1. DIMENSIONAMENTO RESPONSIVO DA JANELA
    int i_height = p_max_y - 6;
    if (i_height > 26) i_height = 26; 
    int i_width = p_max_x - 12;
    if (i_width > 102) i_width = 102;   

    int i_start_y = (p_max_y - i_height) / 2;
    int i_start_x = (p_max_x - i_width) / 2;

    // 2. CRIAÇÃO E RENDERIZAÇÃO DA JANELA DE SOMBRA (FUNDO)
    WINDOW *shadow_win = newwin(i_height, i_width, i_start_y + 1, i_start_x + 1);
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9)); 
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);

    // 3. CRIAÇÃO DA MOLDURA PRINCIPAL
    WINDOW *border_win = newwin(i_height, i_width, i_start_y, i_start_x);
    wbkgd(border_win, COLOR_PAIR(13) | FLAGS);
    box(border_win, 0, 0);
    
    wattron(border_win, A_BOLD);
    const char *title = _(" Solar Return Almuten: Lord of the Year ");
    mvwprintw(border_win, 0, (i_width - get_visual_width(title)) / 2, title);
    wattroff(border_win, A_BOLD);
    
    mvwprintw(border_win, i_height - 1, (i_width - 44) / 2, _(" [↓↑|JK: Scroll | Q|ESC: Return to Chart] "));
    wnoutrefresh(border_win);

    doupdate();

    // 4. CRIAÇÃO DA PAD INTERNA PARA SCROLL
    int pad_lines = 180; 
    int pad_cols = i_width - 6; 
    WINDOW *pad = newpad(pad_lines, pad_cols);
    wbkgd(pad, COLOR_PAIR(13) | FLAGS);
    keypad(pad, TRUE);
    idlok(pad, TRUE);
    scrollok(pad, TRUE);

    wprintw(pad, "\n"); 
    int line_count = 1;

    // --- PREÂMBULO DA REVOLUÇÃO SOLAR ---
    wattron(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, _(" THE LORD OF THE YEAR (ALMUTEN OF SOLAR RETURN) \n"));
    line_count++;

    wattroff(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    line_count++;

    line_count += print_split_lines(pad, _("Unlike your Natal Almuten, which rules over your entire lifetime and soul blueprint, "
                 "the Almuten of the Solar Return operates as the temporal manager of your current year. "
                 "It indicates where the cosmic focus will manifest most intensely, driving events, "
                 "circumstances, and the psychological tone of these specific 12 months.\n\n"), MAX_LINE_WIDTH);


    if (qtd_vencedores > 1) {
        wattron(pad, A_BOLD | COLOR_PAIR(11));
        wprintw(pad, _("CO-REGENCY DETECTED: Shared Annual Focus\n\n"));
        line_count += 2;

        wattroff(pad, A_BOLD | COLOR_PAIR(11));
        line_count += print_split_lines(pad, _("Two or more planetary archetypes share equal operational strength this year. "
                     "Your experiences will be double-sided, forcing you to balance both planetary themes "
                     "simultaneously to navigate the year's challenges effectively.\n\n"), MAX_LINE_WIDTH);
    }

    // 5. LOOP DE RENDERIZAÇÃO DOS PLANETAS VENCEDORES DO ANO
    for (int p = 0; p < qtd_vencedores; p++) {
        int id_planeta = res_almuten[p];
        const char *glifo = obter_glifo_planeta_por_id(id_planeta);

        wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
        line_count++;

        wattron(pad, A_BOLD | COLOR_PAIR(32) | A_REVERSE);
        wprintw(pad, " %s  %s ", glifo, _(" ANNUAL OPERATIONAL RULER "));
        
        if (id_planeta == 1) wprintw(pad, _("(THE SUN - ☉) \n"));
        else if (id_planeta == 2) wprintw(pad, _("(THE MOON - ☽) \n"));
        else if (id_planeta == 3) wprintw(pad, _("(MERCURY - ☿) \n"));
        else if (id_planeta == 4) wprintw(pad, _("(VENUS - ♀) \n"));
        else if (id_planeta == 5) wprintw(pad, _("(MARS - ♂) \n"));
        else if (id_planeta == 6) wprintw(pad, _("(JUPITER - ♃) \n"));
        else if (id_planeta == 7) wprintw(pad, _("(SATURN - ♄) \n"));
        line_count++;

        wattroff(pad, A_BOLD | COLOR_PAIR(32) | A_REVERSE);
        wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n\n");
        line_count += 2;

        if (id_planeta == 1) { // SOL
            wattron(pad, A_BOLD); 
            wprintw(pad, _("The Focus of the Year:\n\n"));
            line_count += 3;

            wattroff(pad, A_BOLD);
            line_count += print_split_lines(pad, _("This is a year of high visibility, core awakening, and personal sovereignty. "
                         "Circumstances will push you to step into the center stage of your own life, "
                         "demanding absolute clarity of purpose and professional recognition.\n\n"), MAX_LINE_WIDTH);
            wattron(pad, A_BOLD); 
            wprintw(pad, _("Practical Manifestations:\n\n"));
            line_count += 3; 
            wattroff(pad, A_BOLD);
            line_count += print_split_lines(pad, _("Expect opportunities for career promotion, leadership roles, and a surge in "
                         "vital energy. Important encounters with authoritative figures are highly likely.\n\n"), MAX_LINE_WIDTH);
            wattron(pad, A_BOLD); 
            wprintw(pad, _("Annual Warnings:\n\n"));
            line_count += 3;

            wattroff(pad, A_BOLD);
            line_count += print_split_lines(pad, _("Guard heavily against egotism, prideful power struggles, and burning out due to "
                         "a neurotic need for constant public approval and validation.\n\n\n"), MAX_LINE_WIDTH);
        }
        else if (id_planeta == 2) { // LUA
            wattron(pad, A_BOLD); wprintw(pad, _("The Focus of the Year:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("A highly personal, internal, and emotional 12-month chapter. The focus shifts "
                         "entirely toward foundations, family, domestic stability, and inner security.\n\n"), MAX_LINE_WIDTH);
            wattron(pad, A_BOLD); wprintw(pad, _("Practical Manifestations:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("Strong indicators of real estate adjustments, changes in the household or family "
                         "dynamics, and events that will test and restructure your emotional resilience.\n\n"), MAX_LINE_WIDTH);
            wattron(pad, A_BOLD); wprintw(pad, _("Annual Warnings:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("Beware of volatile mood swings, hypersensitivity to external criticism, and the "
                         "tendency to retreat into defensive, nostalgic isolation when friction arises.\n\n\n"), MAX_LINE_WIDTH);
        }
        else if (id_planeta == 3) { // MERCÚRIO
            wattron(pad, A_BOLD); wprintw(pad, _("The Focus of the Year:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("A year driven by intense mental activity, intellectual output, and networking. "
                         "Your analytical skills and communication channels will be highly accelerated.\n\n"), MAX_LINE_WIDTH);
            wattron(pad, A_BOLD); wprintw(pad, _("Practical Manifestations:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("Excellent periods for signing contracts, negotiating business deals, academic "
                         "pursuits, writing, and multiple short-distance travels or structural moves.\n\n"), MAX_LINE_WIDTH);
            wattron(pad, A_BOLD); wprintw(pad, _("Annual Warnings:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("High risk of nervous system exhaustion, acute mental anxiety, split focus, and "
                         "the danger of over-rationalizing critical decisions that require emotional depth.\n\n\n"), MAX_LINE_WIDTH);
        }
        else if (id_planeta == 4) { // VÊNUS
            wattron(pad, A_BOLD); wprintw(pad, _("The Focus of the Year:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("A year designated for harmonic consolidation, financial focus, and relationship "
                         "evaluations. The core theme is finding value, alignment, and social peace.\n\n"), MAX_LINE_WIDTH);
            wattron(pad, A_BOLD); wprintw(pad, _("Practical Manifestations:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("Favorable timelines for artistic endeavors, income enhancement, entering strategic "
                         "alliances, and experiencing a profound growth in your active romantic life.\n\n"), MAX_LINE_WIDTH);
            wattron(pad, A_BOLD); wprintw(pad, _("Annual Warnings:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("Watch out for financial extravagance, lazy self-indulgence, and avoiding necessary "
                         "confrontations out of a codependent desire to keep up superficial appearances.\n\n\n"), MAX_LINE_WIDTH);
        }
        else if (id_planeta == 5) { // MARTE
            wattron(pad, A_BOLD); wprintw(pad, _("The Focus of the Year:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("A dynamic, highly friction-based, and action-heavy period. This year demands "
                         "intense physical courage, independent initiatives, and cutting away dead weight.\n\n"), MAX_LINE_WIDTH);
            wattron(pad, A_BOLD); wprintw(pad, _("Practical Manifestations:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("Overcoming major obstacles through raw effort, launch of independent enterprises, "
                         "but also a strong indicator of physical expenditures and competitive standoffs.\n\n"), MAX_LINE_WIDTH);
            wattron(pad, A_BOLD); wprintw(pad, _("Annual Warnings:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("Prone to acute accidents due to rash haste, explosive outbursts of anger, "
                         "unnecessary litigations, and burnouts caused by operating in perpetual survival mode.\n\n\n"), MAX_LINE_WIDTH);
        }
        else if (id_planeta == 6) { // JÚPITER
            wattron(pad, A_BOLD); wprintw(pad, _("The Focus of the Year:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("The year of expansion, philosophical growth, and providential opportunities. "
                         "A benevolent energy wraps around your efforts, granting a protective buffer.\n\n"), MAX_LINE_WIDTH);
            wattron(pad, A_BOLD); wprintw(pad, _("Practical Manifestations:\n\n")); wattroff(pad, A_BOLD);
            line_count += 3;

            line_count += print_split_lines(pad, _("Financial expansion, legal resolutions rolling in your favor, publishing success, "
                         "and meaningful long-distance journeys that broaden your entire existential worldview.\n\n"), MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); 
            wprintw(pad, _("Annual Warnings:\n\n"));
            line_count += 3;

            wattroff(pad, A_BOLD);
            line_count += print_split_lines(pad, _("Severe risk of over-expansion and arrogance. Blind, naive optimism that ignores "
                            "material safety nets, leading to over-leveraging and ideological dogmatism.\n\n\n"), MAX_LINE_WIDTH);
        }
        else if (id_planeta == 7) { // SATURNO
            wattron(pad, A_BOLD); 
            wprintw(pad, _("The Focus of the Year:\n\n"));
            line_count += 3;

            wattroff(pad, A_BOLD);
            line_count += print_split_lines(pad, _("A sobering year of evaluation, heavy duty, consolidation, and setting limits. "
                           "Time demands that you build structure and account for real-world responsibilities.\n\n"), MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); 
            wprintw(pad, _("Practical Manifestations:\n\n"));
            line_count += 3;

            wattroff(pad, A_BOLD);
            line_count += print_split_lines(pad, _("Hard, grinding work that yields long-term security, settling deep debts, dealing "
                           "with administrative inheritances, and professional solidifications under pressure.\n\n"), MAX_LINE_WIDTH);

            wattron(pad, A_BOLD); 
            wprintw(pad, _("Annual Warnings:\n\n"));
            line_count += 3;

            wattroff(pad, A_BOLD);
            line_count += print_split_lines(pad, _("Prone to chronic fatigue, paralyzing fear of failure, bouts of deep melancholy, "
                           "and feeling crushed under structural delays or burdens that are not yours to carry.\n\n\n"), MAX_LINE_WIDTH);
        }
    }

    wattron(pad, A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wprintw(pad, _("  [NARRATIVE END] - Press 'Q' or ESC to return to the Solar Return panel.\n"));
    wattroff(pad, A_DIM);

    line_count += 3;

    // 6. LOOP DE INTERAÇÃO E SCROLL DA PAD
    int pad_line_pos = 0;
    //int max_scroll_y = (qtd_vencedores > 1) ? 90 : 45; // Adapta a profundidade de scroll
    int ch;
    
   
    // Altura visível real onde o texto do pad aparece na tela
    int visible_height = (i_start_y + i_height - 2) - (i_start_y + 1) + 1;
    
    // A altura física da scrollbar deve bater com o espaço vertical interno da border_win
    int scrollbar_height = i_height - 2; 

    // Habilita as setas do teclado na janela de borda para o wgetch capturar corretamente
    keypad(border_win, TRUE);

    while (1) {
        // --- 1. CÁLCULO E DESENHO DA SCROLLBAR ---
        if (line_count > visible_height) {
            // Posição proporcional baseada em qual linha estamos (pad_line_pos) 
            // sobre o total que pode ser rolado (line_count - visible_height)
            int max_scroll = line_count - visible_height;
            int scrollbar_pos = (pad_line_pos * (scrollbar_height - 1)) / max_scroll;
            
            for (int i = 0; i < scrollbar_height; i++) {
                if (i == scrollbar_pos) {
                    mvwaddch(border_win, 1 + i, i_width - 2, ACS_BLOCK); // Indicador
                } else {
                    mvwaddch(border_win, 1 + i, i_width - 2, ACS_VLINE); // Linha guia de fundo
                }
            }
        }

        // --- 2. ENVIAR JANELAS PARA O BUFFER (Ordem correta de renderização) ---
        wnoutrefresh(border_win); 
        // prefresh envia os dados do pad diretamente para a tela virtual
        prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 3, i_start_x + i_width - 4);
        doupdate(); // Executa a pintura unificada na tela física

        // --- 3. CAPTURA DE INPUT (Na border_win, não no pad) ---
        ch = wgetch(border_win);
        if (ch == 27 || ch == 'q' || ch == 'Q') {
            break;
        }

        // --- 4. TRATAMENTO DA ROLAGEM ---
        switch (ch) {
            case KEY_UP: case 'k': case 'K': 
                if (pad_line_pos > 0) pad_line_pos--; 
                break;
            case KEY_DOWN: case 'j': case 'J': 
                // Não permite rolar além da última página de texto visível
                if (pad_line_pos < (line_count - visible_height)) pad_line_pos++; 
                break;
        }
    }

    // 7. LIMPEZA COMPLETA DA MEMÓRIA
    delwin(pad);
    delwin(border_win);
    delwin(shadow_win);
}
