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

#include "swephexp.h"
#include "sweph.h"
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
#include "directions.h"
#include "draw-chart.h"
#include "hyleg.h"
#include "arabic_parts.h"

#ifndef SE_KEEP_GREG_CAL
#define SE_KEEP_GREG_CAL 2 /* 0 = Juliano, 1 = Gregoriano, 2 = Misto automático */
#endif

#define OBLIQUIDADE 23.439291 // Obliqüidade média da Eclíptica em graus
#define NAIBOD_KEY  1.014646  // Chave de Naibod: graus equatoriais por ano de vida

#define DIRECT 0
#define CONVERSE 1


// Funções matemáticas utilitárias para conversão
static double para_radianos(double graus) { return graus * M_PI / 180.0; }
static double para_graus(double radianos) { return radianos * 180.0 / M_PI; }


static double get_obliquidade(double jd) {
    char serr[256];
    double xx[6];
    double eps;

    // 1. OBLIQUIDADE DO MAPA
    if (swe_calc_ut(jd, SE_ECL_NUT, SEFLG_SWIEPH, xx, serr) >= 0) {
        eps = xx[0]; 
    } else {
        eps = OBLIQUIDADE; 
    }

    return eps;
}


int obter_dias_do_mes(int mes, int ano) {
    // Para a imensa maioria dos meses, o valor é estático
    if (mes == 4 || mes == 6 || mes == 9 || mes == 11) return 30;
    if (mes != 2) return 31;

    // Se for fevereiro, precisamos checar se o ano específico foi bissexto naquela época da história.
    // Usamos o dia 29 de fevereiro fictício e tentamos validar na biblioteca usando a flag mista (2).
    int a = ano, m = mes, d = 29, h = 12, min = 0;
    double sec = 0.0, jd;
    char serr[256];

    // Se a biblioteca aceitar converter o dia 29 para Julian Day, significa que o ano é bissexto.
    if (swe_utc_to_jd(a, m, d, h, min, sec, SE_KEEP_GREG_CAL, &jd, serr) == OK) {
        return 29;
    }
    
    return 28;
}


// int obter_dias_do_mes(int mes, int ano) {
//     switch (mes) {
//         case 4: case 6: case 9: case 11:
//             return 30;
//         case 2:
//             // Verifica se o ano é bissexto
//             if ((ano % 4 == 0 && ano % 100 != 0) || (ano % 400 == 0)) {
//                 return 29;
//             }
//             return 28;
//         default:
//             return 31;
//     }
// }


// Calcula o Semi-Arco (Diurno ou Noturno) tridimensional de um corpo
double calcular_semi_arco(double dec_rad, double lat_geografica_rad, int acima_do_horizonte) {
    // Distância Ascensional: sen(AD) = tan(dec) * tan(lat_geografica)
    double val = tan(dec_rad) * tan(lat_geografica_rad);
    
    // Proteção contra latitudes extremas (círculo polar)
    if (val > 1.0) val = 1.0;
    if (val < -1.0) val = -1.0;
    
    double ad_rad = asin(val);
    double ad_graus = para_graus(ad_rad);
    
    if (acima_do_horizonte) {
        return 90.0 + ad_graus; // Semi-Arco Diurno (SAD)
    } else {
        return 90.0 - ad_graus; // Semi-Arco Noturno (SAN)
    }
}

// Calcula a Distância Meridiana Absoluta (em relação ao MC ou IC)
double calcular_distancia_meridiana(double ra, double ramc, int acima_do_horizonte) {
    double md = ra - ramc;
    if (md < 0) md += 360.0;
    if (md > 180.0) md = 360.0 - md;
    
    if (acima_do_horizonte) {
        // Distância ao Meio do Céu (MC)
        return md;
    } else {
        // Distância ao Fundo do Céu (IC = RAMC + 180)
        double md_ic = ra - (ramc + 180.0);
        while (md_ic < -180.0) md_ic += 360.0;
        while (md_ic > 180.0) md_ic -= 360.0;
        return fabs(md_ic);
    }
}







// Calcula a Ascensão Reta (RA) de forma protegida para planetas e pontos abstratos (Fortuna/SAN)
double calcular_ra(double longitude, double declinacao, double jd) {
    double dec_real = declinacao;

    // PROTEÇÃO CRÍTICA: Se a declinação for inválida (NAN) ou exatamente 0.0 (como em pontos abstratos),
    // nós calculamos a declinação astronômica exata que aquele grau do zodíaco possui na Eclíptica!
    if (isnan(declinacao) || declinacao == 0.0) {
        double lon_rad = para_radianos(longitude);
        double eps_rad = para_radianos(get_obliquidade(jd));
        // Fórmula clássica da declinação solar/eclíptica: sen(dec) = sen(lon) * sen(eps)
        dec_real = para_graus(asin(sin(lon_rad) * sin(eps_rad)));
    }

    double lon_rad = para_radianos(longitude);
    double dec_rad = para_radianos(dec_real);
    double eps_rad = para_radianos(get_obliquidade(jd));

    // Executa a fórmula da trigonometria esférica clássica com a declinação corrigida
    double ra_rad = atan2(sin(lon_rad) * cos(eps_rad) - tan(dec_rad) * sin(eps_rad), cos(lon_rad));
    double ra_graus = para_graus(ra_rad);
    
    if (ra_graus < 0) ra_graus += 360.0;
    return ra_graus;
}


// Calcula o cronograma de direções zodiacais para QUALQUER ponto escolhido
int calcular_direcoes_zodiacais_geral(PlotObject *plots, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double *latitudes, int sentido, Promissor *prom) {
    (void)latitudes;

    int qtd_direcoes = 0;
    //int object_diff = show_modern_planets ? 0 : 3;

    if (idx_alvo < 0 || idx_alvo >= NUM_OBJECTS) return 0;

    // Calcula a Ascensão Reta baseada na coordenada do ponto alvo escolhido
    double ra_significador = calcular_ra(plots[idx_alvo].longitude, plots[idx_alvo].declination, jd);

    double angulos_aspectos[] = {0.0, 60.0, 90.0, 120.0, 180.0};
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍"};

    //double epsilon = 0.000001;

    // Varre os 7 planetas tradicionais como Promissores (agentes de movimento)
    for (int p = 0; p < 81; p++) {
        for (int s = 0; s < 2; s++) {
            if (p == idx_alvo) continue; // Um ponto não direciona a si mesmo
            
            for (int a = 0; a < 5; a++) {

                if (prom[p].type == PROM_TERM && a > 0) break; // apenas conjunções para termos

                // Dentro do loop de aspectos (for a = 0; a < 5; a++)

                double lon_aspecto = fmod(prom[p].longitude + angulos_aspectos[a], 360.0);

                // 1. Pegamos a LATITUDE natal do planeta promissor (armazenada no objeto plot)
                double lat_natal_promissor = prom[p].latitude; // Certifique-se de carregar a latitude real aqui

                // 2. Convertemos as coordenadas eclípticas (Longitude do Aspecto + Latitude Natal) para Equatoriais
                double xx[6];
                double xequat[6];

                xx[0] = lon_aspecto;          // Longitude do aspecto
                xx[1] = lat_natal_promissor;  // Latitude real que o planeta possui na sua órbita
                xx[2] = 1.0;                  // Distância (pode ser 1.0 para este cálculo)

                // A função da Swiss Ephemeris faz a trigonometria esférica exata para nós
                swe_cotrans(xx, xequat, -get_obliquidade(jd)); // O sinal negativo converte de eclíptica para equatorial

                double ra_aspecto = xequat[0];  // Ascensão Reta tridimensional exata do aspecto
                //double dec_aspecto = xequat[1]; // Declinação tridimensional exata do aspecto

                // Agora o ra_aspecto já saiu pronto e perfeitamente simétrico ao significador!

                double arco = 0.0;

                if (s == 0 && sentido != 1) {
                    arco = ra_aspecto - ra_significador;
                }
                else if (s == 1 && sentido != 0) {
                    arco = ra_significador - ra_aspecto;
                }

                if (arco < 0) {
                    arco += 360.0;
                }

                // Filtra arcos de idade humana viável (0 a 150 anos)
                if (arco > 0.0 && arco <= MAX_AGE) {
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];

                    d->sentido = s;
                    
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    
                    // Salva o nome e glifo do Significador Alvo atual
                    strcpy(d->significador_name, plots[idx_alvo].object_name);
                    strcpy(d->significador_glifo, plots[idx_alvo].object);
                    
                    d->promissor_type = prom[p].type;

                    // 1. Calcula o arco e a idade do evento normalmente
                    d->arco_graus = arco;
                    d->idade_evento = arco / NAIBOD_KEY; // Baseado em #define NAIBOD_KEY 1.014646

                    // 2. Transforma a idade em dias exatos (Ano trópico astronômico médio)
                    // Ano trópico médio = 365.242199 dias. 
                    double dias_decorridos = d->idade_evento * 365.242199;

                    // 3. Calcula o Dia Juliano exato em que o evento ocorre
                    // 'jd' é o Dia Juliano UT do momento do nascimento passado para a função
                    double jd_evento = jd + dias_decorridos;

                    // 4. Devolve o Dia Juliano direto para o calendário misto histórico da Swiss Ephemeris
                    int ano_c, mes_c, dia_c, hora_c, min_c;
                    double sec_c;
                    //char err_msg[256];

                    // Usa o valor 2 (SE_KEEP_GREG_CAL fictício) para transição automática Juliano/Gregoriano de 1582
                    swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);

                    // 5. Alimenta a sua estrutura LinhaDirecao com a precisão mecânica da biblioteca
                    d->ano_calendario = ano_c;
                    d->mes_calendario = mes_c;
                    d->dia_calendario = dia_c;

                                    
                    strcpy(d->tipo_direcao, "Zodiacal");

                    qtd_direcoes++;
                    if (qtd_direcoes >= 300) goto fim_calculo;
                }
            }
        }
    }

fim_calculo:
    qsort(lista_resultado, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);
    return qtd_direcoes;
}


int calcular_direcoes_mundanas_geral(PlotObject *plots, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promissor *prom) {
    int qtd_direcoes = 0;
    double lat_geo_rad = para_radianos(lat_geografica);

    if (idx_alvo < 0 || idx_alvo >= NUM_OBJECTS) return 0;

    // 1. Dados tridimensionais REAIS do Significador (Alvo)
    double ra_sig = calcular_ra(plots[idx_alvo].longitude, plots[idx_alvo].declination, jd);
    double dec_sig_rad = para_radianos(plots[idx_alvo].declination);
    
    // Determinar se o significador está acima/abaixo do horizonte natal
    int sig_acima = (romanToInt(plots[idx_alvo].house) >= 7 && romanToInt(plots[idx_alvo].house) <= 12); 
    
    double sa_sig = calcular_semi_arco(dec_sig_rad, lat_geo_rad, sig_acima);
    double md_sig = calcular_distancia_meridiana(ra_sig, ramc, sig_acima);
    double cota_mundana_sig = md_sig / sa_sig;

    // Multiplicadores para os aspectos mundanos
    double mult_aspectos[] = {0.0, 0.333333, 0.5, 0.666667, 1.0}; // Conjunção, Sextil, Quadratura, Trígono, Oposição
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍"};

    for (int p = 0; p < 81; p++) {
        //if (p == idx_alvo) continue;
        if (prom[p].type == PROM_TERM) continue;

        // 2. Dados tridimensionais REAIS do Promissor
        double ra_prom = calcular_ra(prom[p].longitude, prom[p].declination, jd);
        double dec_prom_rad = para_radianos(prom[p].declination);
        int prom_acima = ((prom[p].house) >= 7 && (prom[p].house) <= 12);

        double sa_prom = calcular_semi_arco(dec_prom_rad, lat_geo_rad, prom_acima);
        double md_prom = calcular_distancia_meridiana(ra_prom, ramc, prom_acima);

        for (int s = 0; s < 2; s++) { // 0 = Direta, 1 = Conversa
            
            // FILTRO CRÍTICO DE SENTIDO: Se o usuário filtrou por um sentido específico, pula o outro
            if (s == 0 && sentido == 1) continue; // Usuário quer apenas conversas (1), pula a direta (0)
            if (s == 1 && sentido == 0) continue; // Usuário quer apenas diretas (0), pula a conversa (1)

            for (int a = 0; a < 5; a++) {

                double md_destino = 0.0;
                double arco = 0.0;
                
                // Calcula a distância meridiana onde o aspecto do promissor se projeta no espaço
                double md_prom_aspecto = md_prom + (sa_prom * mult_aspectos[a]);
                
                if (s == 0) { // Direta
                    // O aspecto do Promissor se move até a cota proporcional do Significador
                    md_destino = sa_prom * cota_mundana_sig;
                    arco = md_prom_aspecto - md_destino;   
                }
                else if (s == 1) { // Conversa
                    // O Significador se move até a cota proporcional do aspecto do Promissor
                    md_destino = sa_sig * (md_prom_aspecto / sa_prom);
                    arco = md_destino - md_sig;                               
                }

                // Correção de rotação circular esférica
                if (arco < 0) arco += 360.0; 

                // Filtra arcos de idade humana viável (0 a 150 anos)
                if (arco > 0.0 && arco <= MAX_AGE) {
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];

                    d->sentido = s; // Salva 0 para direta ou 1 para conversa
                    
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    
                    strcpy(d->significador_name, plots[idx_alvo].object_name);
                    strcpy(d->significador_glifo, plots[idx_alvo].object);

                    d->promissor_type = prom[p].type;
                    
                    // 1. Calcula o arco e a idade do evento normalmente
                    d->arco_graus = arco;
                    d->idade_evento = arco / NAIBOD_KEY; 

                    // 2. Transforma a idade em dias exatos
                    double dias_decorridos = d->idade_evento * 365.242199;

                    // 3. Calcula o Dia Juliano do evento
                    double jd_evento = jd + dias_decorridos;

                    // 4. Converte o Dia Juliano para data do calendário (Gregoriano/Juliano automático)
                    int ano_c, mes_c, dia_c, hora_c, min_c;
                    double sec_c;
                    swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);

                    // 5. Alimenta a estrutura com as datas
                    d->ano_calendario = ano_c;
                    d->mes_calendario = mes_c;
                    d->dia_calendario = dia_c;
                                    
                    strcpy(d->tipo_direcao, _("Mundane")); // CORREÇÃO: Identifica corretamente como Mundana

                    qtd_direcoes++;
                    if (qtd_direcoes >= 300) goto fim_calculo; // Sai de forma limpa se estourar o limite
                }
            }
        }
    }

fim_calculo:
    qsort(lista_resultado, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);
    return qtd_direcoes;
}


void display_primary_directions(PlotObject *plots, AspectMatrix *matrix, PontosHylegiacos pontos, int regente_dia, int regente_hora, char *nome_anareta, char *nome_senhor_da_casa8, int tipo_h_natal, int idx_hyleg_natal, bool mapa_retorno, double jd, double *latitudes, int tipo_san, PlanetDignities *dig, double ramc, double lat, Promissor *prom) {
       
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 29;
    int table_width = max_x - 10;
    int start_y = (max_y - table_height) / 2;
    int start_x = 5;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    keypad(table_win, TRUE);

    // ────────────────────────────────────────────────────────────────────────
    // MAPEAMENTO DA LISTA DE CRONOCRATORES ALVOS (Os 6 Significadores)
    // ────────────────────────────────────────────────────────────────────────
    int id_almuten_ref = 0;
    int object_diff = show_modern_planets ? 0 : 3;
    int tipo_h = -1; 
    
    if (!mapa_retorno) {
        tipo_h = get_hyleg(pontos, plots, matrix, &id_almuten_ref, regente_dia, regente_hora, tipo_san, dig);
    }
    else {
        tipo_h = tipo_h_natal;
    }

    int idx_hileg = -1;
    int idx_sol = 0;   
    int idx_lua = 1;   
    int idx_asc = -1;
    int idx_mc = -1;
    int idx_san = -1;
    int idx_fortuna = -1;
    int idx_mercury = -1;
    int idx_venus = -1;
    int idx_mars = -1;
    int idx_jupiter = -1;
    int idx_saturn = -1;
    int idx_dc = -1;
    int idx_ic = -1;

    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        if (plots[i].id == P_ASC - object_diff) idx_asc = i;
        if (plots[i].id == P_MC - object_diff)  idx_mc = i; 
        if (strcmp(plots[i].object_name, "SAN") == 0) idx_san = i; 
        if (strcmp(plots[i].object_name, _("Part of Fortune")) == 0) idx_fortuna = i; 
        if (plots[i].id == P_MERCURY) idx_mercury = i;
        if (plots[i].id == P_VENUS) idx_venus = i;
        if (plots[i].id == P_MARS) idx_mars = i;
        if (plots[i].id == P_JUPITER) idx_jupiter = i;
        if (plots[i].id == P_SATURN) idx_saturn = i;
        if (plots[i].id == P_DC - object_diff) idx_dc = i;
        if (plots[i].id == P_IC - object_diff)  idx_ic = i; 
    }

    if (!mapa_retorno) {
        if (tipo_h == H_SOL) idx_hileg = 0;
        else if (tipo_h == H_LUNA) idx_hileg = 1;
        else if (tipo_h == H_SAN) idx_hileg = P_SAN - object_diff;
        else if (tipo_h == H_ALMUTEN) idx_hileg = id_almuten_ref - 1;
        else if (tipo_h == H_ALMUTEN_HYL) idx_hileg = id_almuten_ref - 1;
        else {
            for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
                if (tipo_h == H_ASC && plots[i].id == P_ASC - object_diff) { idx_hileg = i; break; }
                if (tipo_h == H_FORTUNA && plots[i].id == P_FORTUNA - object_diff) { idx_hileg = i; break; }
            }
        }
    }
    else {
        idx_hileg = idx_hyleg_natal;
    }

    int indices_significadores[14];
    indices_significadores[0] = idx_hileg;
    indices_significadores[1] = idx_sol;
    indices_significadores[2] = idx_lua;
    indices_significadores[3] = idx_asc;
    indices_significadores[4] = idx_mc;
    indices_significadores[5] = idx_san;
    indices_significadores[6] = idx_fortuna;
    indices_significadores[7] = idx_mercury;
    indices_significadores[8] = idx_venus;
    indices_significadores[9] = idx_mars;
    indices_significadores[10] = idx_jupiter;
    indices_significadores[11] = idx_saturn;
    indices_significadores[12] = idx_dc;
    indices_significadores[13] = idx_ic;

    int seletor_alvo_atual = 0; 
    int scroll_offset = 0;
    int loop_interativo = 1;

    int max_linhas_exibicao = table_height - 13;
    WINDOW *scroll_pad = newpad(1200, table_width - 8); 

    // Desenha sombra e frame fixo de fundo
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0); 
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);
   
    wbkgd(table_win, COLOR_PAIR(13) | FLAGS);
    wbkgd(scroll_pad, COLOR_PAIR(13) | FLAGS); 

    int sentido = 2;
    int tipo = 2;

    while (loop_interativo) {
        werase(table_win);
        werase(scroll_pad);

        box(table_win, 0, 0);
        wattron(table_win, A_BOLD);
        const char *title = _(" Primary Directions ");
        mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);

        int idx_atual_calculo = indices_significadores[seletor_alvo_atual];

        
        int qtd_direcoes_zod = 0;
        int qtd_direcoes_mun = 0;
        
        LinhaDirecao cronograma_z[300];
        LinhaDirecao cronograma_m[300];
        
        if (tipo != 1) {
            memset(cronograma_z, 0, sizeof(cronograma_z));
            qtd_direcoes_zod = calcular_direcoes_zodiacais_geral(plots, idx_atual_calculo, cronograma_z, jd, latitudes, sentido, prom);
        }
        if (tipo != 0) {   
            memset(cronograma_m, 0, sizeof(cronograma_m));
            qtd_direcoes_mun = calcular_direcoes_mundanas_geral(plots, idx_atual_calculo, cronograma_m, jd, ramc, lat, sentido, prom);
        }
        int qtd_direcoes = qtd_direcoes_zod + qtd_direcoes_mun;

        LinhaDirecao cronograma[qtd_direcoes];
        memset(cronograma, 0, sizeof(cronograma));

        int index = 0;
        for (int i = 0; i < qtd_direcoes_zod; i++) {
            cronograma[index] = cronograma_z[i];
            index++;
        }
        for (int i = 0; i < qtd_direcoes_mun; i++) {
            cronograma[index] = cronograma_m[i];
            index++;
        }
        qsort(cronograma, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);





        if (scroll_offset > qtd_direcoes * 2 - max_linhas_exibicao) {
            scroll_offset = qtd_direcoes * 2 - max_linhas_exibicao;
        }
        if (scroll_offset < 0) scroll_offset = 0;

        mvwprintw(table_win, 2, 4, _("Active Significator Target: "));
        wattron(table_win, A_BOLD | COLOR_PAIR(8));
        if (idx_atual_calculo != -1) {
            wprintw(table_win, "%s %s", plots[idx_atual_calculo].object, plots[idx_atual_calculo].object_name);
            if (seletor_alvo_atual == 0) wprintw(table_win, _(" [EMPHASIZED HYLEG]"));
        } else {
            wprintw(table_win, _("Point not calculated in this chart"));
        }
        wattroff(table_win, A_BOLD | COLOR_PAIR(8));

        wattron(table_win, A_DIM);
        mvwprintw(table_win, 2, table_width - 32, _("Use [←/→] Signif. [↑/↓] Scroll"));
        wattroff(table_win, A_DIM);

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 4, 2, "────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        int col_idade = 0, col_ano = 16, col_mes = 21, col_dia = 24, col_dir = 33, col_arco = 67, col_tipo = 81, col_sen = 91;

        wattron(table_win, A_BOLD | COLOR_PAIR(13));
        mvwprintw(table_win, 5, col_idade + 4, _("Age")); 
        mvwprintw(table_win, 5, col_ano + 4, _("Year"));
        mvwprintw(table_win, 5, col_mes + 3, _(" Mo"));
        mvwprintw(table_win, 5, col_dia + 4, _("Day"));
        mvwprintw(table_win, 5, col_dir + 4, _("Directional Event")); 
        mvwprintw(table_win, 5, col_arco + 4, _("Arc (Equat.)"));
        mvwprintw(table_win, 5, col_tipo + 4, _("Method"));
        mvwprintw(table_win, 5, col_sen + 4, _("Direction"));
        wattroff(table_win, A_BOLD | COLOR_PAIR(13));

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 6, 2, "────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        int row_pad = 0;
        int linhas_reais_pad = qtd_direcoes * 2;

        if (qtd_direcoes == 0 || idx_atual_calculo == -1) {
            wattron(scroll_pad, A_DIM);
            mvwprintw(scroll_pad, row_pad, col_dir, _("No directional contacts available for this specific point."));
            wattroff(scroll_pad, A_DIM);
        } else {
            for (int i = 0; i < qtd_direcoes; i++) {
                LinhaDirecao *d = &cronograma[i];

                char texto_evento[100];
                snprintf(texto_evento, sizeof(texto_evento), "%s %s → %s %s", 
                d->promissor_glifo, // d->promissor_name,
                (d->promissor_type == PROM_TERM)?"":d->aspecto_symbol,
                d->significador_glifo, 
                d->significador_name);

                bool eh_aspecto_tenso = (strcmp(d->aspecto_symbol, "□") == 0 || strcmp(d->aspecto_symbol, "☍") == 0);
                bool eh_conjuncao = (strcmp(d->aspecto_symbol, "☌") == 0);

                bool eh_marte   = (strcmp(d->promissor_name, _("Mars")) == 0);
                bool eh_saturno = (strcmp(d->promissor_name, _("Saturn")) == 0);
                bool eh_malefico_essencial = (eh_marte || eh_saturno);
                
                bool eh_anareta      = (strcmp(d->promissor_name, nome_anareta) == 0);
                bool eh_senhor_casa8 = (strcmp(d->promissor_name, nome_senhor_da_casa8) == 0);
                bool eh_anareta_ou_mortis = (eh_anareta || eh_senhor_casa8);

                bool eh_jupiter   = (strcmp(d->promissor_name, _("Jupiter")) == 0);
                bool eh_venus = (strcmp(d->promissor_name, _("Venus")) == 0);
                bool eh_benefico_essencial = (eh_jupiter || eh_venus);

                int par_cor_ativo = COLOR_PAIR(13);
                int atributo_extra = A_NORMAL;

                if (eh_anareta_ou_mortis) {
                    if (eh_aspecto_tenso || strcmp(d->aspecto_symbol, "☌") == 0) {
                        par_cor_ativo = COLOR_PAIR(11);
                        atributo_extra = A_BOLD | A_REVERSE;
                    } else {
                        par_cor_ativo = COLOR_PAIR(11); 
                        atributo_extra = A_BOLD;
                    }
                }
                else if (eh_malefico_essencial && (eh_aspecto_tenso || eh_conjuncao)) {
                    par_cor_ativo = COLOR_PAIR(11); 
                    atributo_extra = A_NORMAL;
                }
                else if (!eh_malefico_essencial && eh_aspecto_tenso) {
                    par_cor_ativo = COLOR_PAIR(25);
                    atributo_extra = A_ITALIC | A_REVERSE;
                }
                else if (eh_benefico_essencial) {
                    par_cor_ativo = COLOR_PAIR(8);
                    atributo_extra = A_ITALIC | A_BOLD;      
                }
                else if (strcmp(d->aspecto_symbol, "☌") == 0) {
                    par_cor_ativo = COLOR_PAIR(7);
                    atributo_extra = A_BOLD;
                }

                wattron(scroll_pad, par_cor_ativo | atributo_extra);

                mvwprintw(scroll_pad, row_pad, col_idade, "%8.4f y", d->idade_evento);
                mvwprintw(scroll_pad, row_pad, col_ano, "%4d.", d->ano_calendario);
                mvwprintw(scroll_pad, row_pad, col_mes, "%02d.", d->mes_calendario);
                mvwprintw(scroll_pad, row_pad, col_dia, "%02d", d->dia_calendario);
                mvwprintw(scroll_pad, row_pad, col_dir, "%s", texto_evento);
                mvwprintw(scroll_pad, row_pad, col_arco, "%05.2f°", d->arco_graus);
                mvwprintw(scroll_pad, row_pad, col_tipo, "%s", d->tipo_direcao);
                mvwprintw(scroll_pad, row_pad, col_sen, "%s", (d->sentido == 0 ? _("Direct") : _("Converse")));

                wattroff(scroll_pad, par_cor_ativo | atributo_extra);

                wattron(scroll_pad, COLOR_PAIR(10) | A_DIM);
                mvwprintw(scroll_pad, row_pad + 1, 0, "────────────────────────────────────────────────────────────────────────────────────────────────────"); 
                wattroff(scroll_pad, COLOR_PAIR(10) | A_DIM);

                row_pad += 2;            
            }
        }

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, table_height - 7, 2, "────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        wattron(table_win, A_DIM);
        mvwprintw(table_win, table_height - 6, 4, _("Time Key: Naibod Rate (1° of Equatorial Rotation = 1.0146 Years). ε: Dynamic."));
        // CORREÇÃO: Legenda dinâmica baseada no tipo ativo
        if (tipo == 0) {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Zodiacal with Real Latitude (Method Placidus)."));
        } else if (tipo == 1) {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Mundane proportional to Semi-Arcs."));
        } else {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Mixed Systems (Zodiacal w/ Latitude + Mundane proportional to Semi-Arcs)."));
        }

        // Exibe um indicador visual de paginação se houver mais linhas abaixo ou acima
        if (linhas_reais_pad > max_linhas_exibicao) {
            mvwprintw(table_win, table_height - 3, 4, "%s %d-%d %s %d%s%s",
                _("[↑/↓] [PgUp/PgDn] Scroll (Showing"),
                scroll_offset / 2 + 1, 
                ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                _("of"),
                qtd_direcoes,
                _(") | [←/→] Change Target"),
                _(" | [C] Conv [D] Dir [A] All | [Z] Zod [M] Mund [B] Both"));
        } else {
            mvwprintw(table_win, table_height - 3, 4, _("Use [←/→] Change Target | [C] Conv [D] Dir [A] All | [Z] Zod [M] Mund [B] Both"));
        }
        wattroff(table_win, A_DIM);

        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));

        
        wnoutrefresh(table_win);

        doupdate();

        int fim_y_recorte = start_y + 7 + max_linhas_exibicao - 2;
        if ((scroll_offset + max_linhas_exibicao) > linhas_reais_pad) {
            fim_y_recorte = start_y + 7 + (linhas_reais_pad - scroll_offset) - 1;
        }

        if (linhas_reais_pad > 0) {
            prefresh(scroll_pad, scroll_offset, 0, start_y + 7, start_x + 4, fim_y_recorte, start_x + table_width - 5);
        }
        int ch = wgetch(table_win);
        switch (ch) {
            case 'C':
            case 'c':
                sentido = 1;
                break;
            case 'd':
            case 'D':
                sentido = 0;
                break;
            case 'a':
            case 'A':
                sentido = 2;
                break;
            case 'Z':
            case 'z':
                tipo = 0;
                break;
            case 'm':
            case 'M':
                tipo = 1;
                break;
            case 'b':
            case 'B':
                tipo = 2;
                break;
            case KEY_RIGHT:
                seletor_alvo_atual = (seletor_alvo_atual + 1) % 14;
                scroll_offset = 0;
                break;
            case KEY_LEFT:
                seletor_alvo_atual = (seletor_alvo_atual - 1 + 14) % 14;
                scroll_offset = 0;
                break;
            case KEY_DOWN:
                if (scroll_offset < (qtd_direcoes * 2 - max_linhas_exibicao)) {
                    scroll_offset += 2;
                }
                break;
            case KEY_UP:
                if (scroll_offset > 0) {
                    scroll_offset -= 2;
                }
                break;
            case KEY_NPAGE:
                if (scroll_offset < (qtd_direcoes * 2 - max_linhas_exibicao)) {
                    scroll_offset += max_linhas_exibicao;
                }
                else {
                    scroll_offset = qtd_direcoes * 2 - 1;
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
}



int calcular_direcoes_zodiacais_partes(ArabicPartCalculada *parts, int qtd_partes, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double *latitudes, int sentido, Promissor *prom) {
    (void) latitudes;
    int qtd_direcoes = 0;

    if (idx_alvo < 0 || idx_alvo >= qtd_partes) return 0;

    // Calcula a Ascensão Reta baseada na coordenada do ponto alvo escolhido
    double ra_significador = calcular_ra(parts[idx_alvo].longitude, NAN, jd);

    double angulos_aspectos[] = {0.0, 60.0, 90.0, 120.0, 180.0};
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍"};

    // Varre os 7 planetas tradicionais como Promissores (agentes de movimento)
    for (int p = 0; p < 7; p++) {
        for (int s = 0; s < 2; s++) {
            for (int a = 0; a < 5; a++) {
                
                if (prom[p].type == PROM_TERM && a > 0) break; // apenas conjunções para termos

                double lon_aspecto = fmod(prom[p].longitude + angulos_aspectos[a], 360.0);

                // 1. Pegamos a LATITUDE natal do planeta promissor (armazenada no seu objeto plot)
                double lat_natal_promissor = prom[p].latitude; // Certifique-se de carregar a latitude real aqui

                // 2. Convertemos as coordenadas eclípticas (Longitude do Aspecto + Latitude Natal) para Equatoriais
                double xx[6];
                double xequat[6];

                xx[0] = lon_aspecto;          // Longitude do aspecto
                xx[1] = lat_natal_promissor;  // Latitude real que o planeta possui na sua órbita
                xx[2] = 1.0;                  // Distância (pode ser 1.0 para este cálculo)

                // A função da Swiss Ephemeris faz a trigonometria esférica exata para nós
                swe_cotrans(xx, xequat, -get_obliquidade(jd)); // O sinal negativo converte de eclíptica para equatorial

                double ra_aspecto = xequat[0];  // Ascensão Reta tridimensional exata do aspecto
                //double dec_aspecto = xequat[1]; // Declinação tridimensional exata do aspecto

                double arco = 0.0;

                if (s == 0 && sentido != 1) {
                    arco = ra_aspecto - ra_significador;
                }
                else if (s == 1 && sentido != 0) {
                    arco = ra_significador - ra_aspecto;
                }

                if (arco < 0) {
                    arco += 360.0;
                }

                // Filtra arcos de idade humana viável (0 a 150 anos)
                if (arco > 0.0 && arco <= MAX_AGE) {
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];

                    d->sentido = s;
                    
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    
                    // Salva o nome e glifo do Significador Alvo atual
                    strcpy(d->significador_name, parts[idx_alvo].name);

                    char abreviacao[10];
                    get_part_abbreviation(parts[idx_alvo].name, abreviacao);
            
                    
                    strcpy(d->significador_glifo, abreviacao);

                    d->promissor_type = prom[p].type;
                    
                    // 1. Calcula o arco e a idade do evento normalmente
                    d->arco_graus = arco;
                    d->idade_evento = arco / NAIBOD_KEY; // Baseado em #define NAIBOD_KEY 1.014646

                    // 2. Transforma a idade em dias exatos (Ano trópico astronômico médio)
                    // Ano trópico médio = 365.242199 dias. 
                    double dias_decorridos = d->idade_evento * 365.242199;

                    // 3. Calcula o Dia Juliano exato em que o evento ocorre
                    // 'jd' é o Dia Juliano UT do momento do nascimento que passado para a função
                    double jd_evento = jd + dias_decorridos;

                    // 4. Devolve o Dia Juliano direto para o calendário misto histórico da Swiss Ephemeris
                    int ano_c, mes_c, dia_c, hora_c, min_c;
                    double sec_c;
                    //char err_msg[256];

                    // Usa o valor 2 (SE_KEEP_GREG_CAL fictício) para transição automática Juliano/Gregoriano de 1582
                    swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);

                    // 5. Alimenta a sua estrutura LinhaDirecao com a precisão mecânica da biblioteca
                    d->ano_calendario = ano_c;
                    d->mes_calendario = mes_c;
                    d->dia_calendario = dia_c;

                    
                    strcpy(d->tipo_direcao, "Zodiacal");

                    qtd_direcoes++;
                    if (qtd_direcoes >= 300) return qtd_direcoes;
                }
            }
        }
    }

    qsort(lista_resultado, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);

    return qtd_direcoes;
}




void display_primary_directions_parts(Promissor *prom, char *nome_anareta, char *nome_senhor_da_casa8, ChartObject *obj, int num_objects, double *cusps, double jd, double *latitudes, double ramc, double lat) {
    (void) latitudes;

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 29;
    int table_width = max_x - 10;
    int start_y = (max_y - table_height) / 2;
    int start_x = 5;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    keypad(table_win, TRUE); // Habilita o teclado para capturar as 4 setas

    ArabicPartCalculada lista_partes[MAX_PARTS] = {0};

    int qtd_partes = load_and_calculate_arabic_parts(obj, num_objects, cusps, lista_partes);

    int indices_significadores[qtd_partes];

    for (int i = 0; i < qtd_partes; i++) {
        indices_significadores[i] = i;
    }

    int seletor_alvo_atual = 0; 
    int scroll_offset = 0; // Controla qual linha virtual será a primeira a aparecer na tela
    int loop_interativo = 1;

    // ────────────────────────────────────────────────────────────────────────
    // CRIAÇÃO DO PAD VIRTUAL DE ROLAGEM
    // ────────────────────────────────────────────────────────────────────────
    // Criamos um espaço de 180 linhas de altura (cabe qualquer volume de direções)
    int max_linhas_exibicao = table_height - 13; // Espaço físico real na janela para os dados
    WINDOW *scroll_pad = newpad(300, table_width - 8); 

    // Desenha sombra e frame fixo de fundo
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0); 
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);

    wbkgd(table_win, COLOR_PAIR(13) | FLAGS);
    wbkgd(scroll_pad, COLOR_PAIR(13) | FLAGS); 

    int sentido = 2;
    int tipo = 2;

    while (loop_interativo) {
        // Limpa todas as estruturas gráficas antes de recalcular
        werase(table_win);
        werase(scroll_pad);

        box(table_win, 0, 0);
        
        wattron(table_win, A_BOLD);
        const char *title = _(" Primary Directions to Arabic Parts ");
        mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);

        int idx_atual_calculo = indices_significadores[seletor_alvo_atual];

        int qtd_direcoes_zod = 0;
        int qtd_direcoes_mun = 0;
        
        LinhaDirecao cronograma_z[300];
        LinhaDirecao cronograma_m[300];
        
        if (tipo != 1) {
            memset(cronograma_z, 0, sizeof(cronograma_z));
            qtd_direcoes_zod = calcular_direcoes_zodiacais_partes(lista_partes, qtd_partes, idx_atual_calculo, cronograma_z, jd, latitudes, sentido, prom);
        }
        if (tipo != 0) {   
            memset(cronograma_m, 0, sizeof(cronograma_m));
            qtd_direcoes_mun = calcular_direcoes_mundanas_partes(lista_partes, idx_atual_calculo, cronograma_m, jd, ramc, lat, sentido, prom);
        }
        int qtd_direcoes = qtd_direcoes_zod + qtd_direcoes_mun;

        LinhaDirecao cronograma[qtd_direcoes];
        memset(cronograma, 0, sizeof(cronograma));

        int index = 0;
        for (int i = 0; i < qtd_direcoes_zod; i++) {
            cronograma[index] = cronograma_z[i];
            index++;
        }
        for (int i = 0; i < qtd_direcoes_mun; i++) {
            cronograma[index] = cronograma_m[i];
            index++;
        }
        qsort(cronograma, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);

        // Garante que o scroll não vá para o vazio se trocarmos para um planeta com menos direções
        if (scroll_offset > qtd_direcoes * 2 - max_linhas_exibicao) {
            scroll_offset = qtd_direcoes * 2 - max_linhas_exibicao;
        }
        if (scroll_offset < 0) scroll_offset = 0;

        // --- RENDERIZAÇÃO DO CABEÇALHO FIXO ---
        mvwprintw(table_win, 2, 4, _("Active Significator Target: "));
        wattron(table_win, A_BOLD | COLOR_PAIR(8));
        if (idx_atual_calculo != -1) {

            char abreviacao[4];
            get_part_abbreviation(lista_partes[idx_atual_calculo].name, abreviacao);
        

            wprintw(table_win, "%s - %s", abreviacao, lista_partes[idx_atual_calculo].name);
        } else {
            wprintw(table_win, _("Point not calculated in this chart"));
        }
        wattroff(table_win, A_BOLD | COLOR_PAIR(8));

        wattron(table_win, A_DIM);
        mvwprintw(table_win, 2, table_width - 32, _("Use [←/→] Signif. [↑/↓] Scroll"));
        wattroff(table_win, A_DIM);

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 4, 2, "────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        // Colunas Alinhadas Fixas (Mapeadas a partir de 0 para casar com as coordenadas do Pad)
        int col_idade = 0, col_ano = 16, col_mes = 21, col_dia = 24, col_dir = 33, col_arco = 67, col_tipo = 81, col_sen = 91;

        wattron(table_win, A_BOLD | COLOR_PAIR(13));
        mvwprintw(table_win, 5, col_idade + 4, _("Age")); 
        mvwprintw(table_win, 5, col_ano + 4, _("Year"));
        mvwprintw(table_win, 5, col_mes + 3, _(" Mo"));
        mvwprintw(table_win, 5, col_dia + 4, _("Day"));
        mvwprintw(table_win, 5, col_dir + 4, _("Directional Event")); 
        mvwprintw(table_win, 5, col_arco + 4, _("Arc (Equat.)"));
        mvwprintw(table_win, 5, col_tipo + 4, _("Method"));
        mvwprintw(table_win, 5, col_sen + 4, _("Direction"));
        wattroff(table_win, A_BOLD | COLOR_PAIR(13));

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 6, 2, "────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        // --- RENDERIZAÇÃO DAS LINHAS DENTRO DO PAD VIRTUAL ---
        int row_pad = 0; // O Pad começa na linha virtual 0 e vai empilhando tudo
        int linhas_reais_pad = qtd_direcoes * 2;

        if (qtd_direcoes == 0 || idx_atual_calculo == -1) {
            wattron(scroll_pad, A_DIM);
            mvwprintw(scroll_pad, row_pad, col_dir, _("No directional contacts available for this specific point."));
            wattroff(scroll_pad, A_DIM);
        } else {
            for (int i = 0; i < qtd_direcoes; i++) {
                LinhaDirecao *d = &cronograma[i];

                char texto_evento[100];
                snprintf(texto_evento, sizeof(texto_evento), "%s %s → %s %s", 
                d->promissor_glifo, // d->promissor_name,
                (d->promissor_type == PROM_TERM)?"":d->aspecto_symbol,
                d->significador_glifo, 
                d->significador_name);


                // --- MOTOR DE DECISÃO DE CORES POR STRINGS (TRADICIONAL) ---
                bool eh_aspecto_tenso = (strcmp(d->aspecto_symbol, "□") == 0 || strcmp(d->aspecto_symbol, "☍") == 0);
                bool eh_conjuncao = (strcmp(d->aspecto_symbol, "☌") == 0);

                // Checagem dos Maléficos Essenciais por nome textual completo
                bool eh_marte   = (strcmp(d->promissor_name, _("Mars")) == 0);
                bool eh_saturno = (strcmp(d->promissor_name, _("Saturn")) == 0);
                bool eh_malefico_essencial = (eh_marte || eh_saturno);
                
                // Checagem funcional dinâmica do Anareta e do Senhor da Casa 8 por texto
                bool eh_anareta      = (strcmp(d->promissor_name, nome_anareta) == 0);
                bool eh_senhor_casa8 = (strcmp(d->promissor_name, nome_senhor_da_casa8) == 0);
                bool eh_anareta_ou_mortis = (eh_anareta || eh_senhor_casa8);

                // Checagem dos Benéficos Essenciais por nome textual completo
                bool eh_jupiter   = (strcmp(d->promissor_name, _("Jupiter")) == 0);
                bool eh_venus = (strcmp(d->promissor_name, _("Venus")) == 0);
                bool eh_benefico_essencial = (eh_jupiter || eh_venus);

                int par_cor_ativo = COLOR_PAIR(13); // Cor neutra padrão (Ciano/Verde)
                int atributo_extra = A_NORMAL;

                if (eh_anareta_ou_mortis) {
                    if (eh_aspecto_tenso || strcmp(d->aspecto_symbol, "☌") == 0) {
                        /* CRITICAL ALARM: O cortador da vida ou Senhor da 8 atuando por aspecto tenso ou conjunção */
                        par_cor_ativo = COLOR_PAIR(11);    // Vermelho de Alerta
                        atributo_extra = A_BOLD | A_REVERSE; // Destaca em negrito e pisca na tela
                    } else {
                        /* Aspecto harmônico (trígono/sextil) do Anareta: Desafios pesados, mas com proteção */
                        par_cor_ativo = COLOR_PAIR(11); 
                        atributo_extra = A_BOLD;
                    }
                }
                else if (eh_malefico_essencial && (eh_aspecto_tenso || eh_conjuncao)) {
                    /* Maléfico essencial comum em aspecto tenso: Dificuldade operacional drástica no período */
                    par_cor_ativo = COLOR_PAIR(11); 
                    atributo_extra = A_NORMAL;
                }
                else if (!eh_malefico_essencial && eh_aspecto_tenso) {
                    /* Benéfico (Júpiter/Vênus) ou Mercúrio em quadratura: Desafio produtivo, NÃO perigo fatal */
                    par_cor_ativo = COLOR_PAIR(25); // Par de cor 14 (Amarelo/Aviso)
                    atributo_extra = A_ITALIC | A_REVERSE;      // Distingue visualmente das crises físicas
                }
                else if (eh_benefico_essencial) {
                    /* Benéfico (Júpiter/Vênus) */
                    par_cor_ativo = COLOR_PAIR(8); // Par de cor 8 (Azul/Bom)
                    atributo_extra = A_ITALIC | A_BOLD;      // Distingue visualmente das crises físicas
                }
                else if (strcmp(d->aspecto_symbol, "☌") == 0) {
                    /* Conjunções comuns normais */
                    par_cor_ativo = COLOR_PAIR(7);
                    atributo_extra = A_BOLD;
                }
                
                wattron(scroll_pad, par_cor_ativo | atributo_extra);

                mvwprintw(scroll_pad, row_pad, col_idade, "%8.4f y", d->idade_evento);
                mvwprintw(scroll_pad, row_pad, col_ano, "%4d.", d->ano_calendario);
                mvwprintw(scroll_pad, row_pad, col_mes, "%02d.", d->mes_calendario);
                mvwprintw(scroll_pad, row_pad, col_dia, "%02d", d->dia_calendario);
                mvwprintw(scroll_pad, row_pad, col_dir, "%s", texto_evento);
                mvwprintw(scroll_pad, row_pad, col_arco, "%05.2f°", d->arco_graus);
                mvwprintw(scroll_pad, row_pad, col_tipo, "%s", d->tipo_direcao);
                mvwprintw(scroll_pad, row_pad, col_sen, "%s", (d->sentido == 0 ? _("Direct") : _("Converse")));

                wattroff(scroll_pad, par_cor_ativo | atributo_extra);

                wattron(scroll_pad, COLOR_PAIR(10) | A_DIM);
                mvwprintw(scroll_pad, row_pad + 1, 0, "────────────────────────────────────────────────────────────────────────────────────────────────────"); 
                wattroff(scroll_pad, COLOR_PAIR(10) | A_DIM);


                row_pad += 2;
            }
        }

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, table_height - 7, 2, "────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        wattron(table_win, A_DIM);
        mvwprintw(table_win, table_height - 6, 4, _("Time Key: Naibod Rate (1° of Equatorial Rotation = 1.0146 Years). ε: Dynamic."));
        // CORREÇÃO: Legenda dinâmica baseada no tipo ativo
        if (tipo == 0) {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Zodiacal with Real Latitude (Method Placidus)."));
        } else if (tipo == 1) {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Mundane proportional to Semi-Arcs."));
        } else {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Mixed Systems (Zodiacal w/ Latitude + Mundane proportional to Semi-Arcs)."));
        }

        // Exibe um indicador visual de paginação se houver mais linhas abaixo ou acima
        if (linhas_reais_pad > max_linhas_exibicao) {
            mvwprintw(table_win, table_height - 3, 4, "%s %d-%d %s %d%s%s",
                _("[↑/↓] [PgUp/PgDn] Scroll (Showing"),
                scroll_offset / 2 + 1, 
                ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                _("of"),
                qtd_direcoes,
                _(") | [←/→] Change Target"),
                _(" | [C] Conv [D] Dir [A] All | [Z] Zod [M] Mund [B] Both"));
        } else {
            mvwprintw(table_win, table_height - 3, 4, _("Use [←/→] Change Target | [C] Conv [D] Dir [A] All | [Z] Zod [M] Mund [B] Both"));
        }
        wattroff(table_win, A_DIM);

        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));

        
        wnoutrefresh(table_win);

        doupdate();

        int fim_y_recorte = start_y + 7 + max_linhas_exibicao - 2;
        if ((scroll_offset + max_linhas_exibicao) > linhas_reais_pad) {
            fim_y_recorte = start_y + 7 + (linhas_reais_pad - scroll_offset) - 1;
        }

        if (linhas_reais_pad > 0) {
            prefresh(scroll_pad, scroll_offset, 0, start_y + 7, start_x + 4, fim_y_recorte, start_x + table_width - 5);
        }
        int ch = wgetch(table_win);
        switch (ch) {
            case 'C':
            case 'c':
                sentido = 1;
                break;
            case 'd':
            case 'D':
                sentido = 0;
                break;
            case 'a':
            case 'A':
                sentido = 2;
                break;
            case 'Z':
            case 'z':
                tipo = 0;
                break;
            case 'm':
            case 'M':
                tipo = 1;
                break;
            case 'b':
            case 'B':
                tipo = 2;
                break;
            case KEY_RIGHT:
                seletor_alvo_atual = (seletor_alvo_atual + 1) % 14;
                scroll_offset = 0;
                break;
            case KEY_LEFT:
                seletor_alvo_atual = (seletor_alvo_atual - 1 + 14) % 14;
                scroll_offset = 0;
                break;
            case KEY_DOWN:
                if (scroll_offset < (qtd_direcoes * 2 - max_linhas_exibicao)) {
                    scroll_offset += 2;
                }
                break;
            case KEY_UP:
                if (scroll_offset > 0) {
                    scroll_offset -= 2;
                }
                break;
            case KEY_NPAGE:
                if (scroll_offset < (qtd_direcoes * 2 - max_linhas_exibicao)) {
                    scroll_offset += max_linhas_exibicao;
                }
                else {
                    scroll_offset = qtd_direcoes * 2 - 1;
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
}



int calcular_direcoes_mundanas_partes(ArabicPartCalculada *parts, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promissor *prom) {
    int qtd_direcoes = 0;
    double lat_geo_rad = para_radianos(lat_geografica);

    if (idx_alvo < 0 || idx_alvo >= NUM_OBJECTS) return 0;

    // 1. Dados tridimensionais REAIS do Significador (Alvo)
    double ra_sig = calcular_ra(parts[idx_alvo].longitude, NAN, jd);
    double dec_sig_rad = para_radianos(calc_declination_mathematical_point(jd, parts[idx_alvo].longitude));
    
    // Determinar se o significador está acima/abaixo do horizonte natal
    int sig_acima = (romanToInt(parts[idx_alvo].house) >= 7 && romanToInt(parts[idx_alvo].house) <= 12); 
    
    double sa_sig = calcular_semi_arco(dec_sig_rad, lat_geo_rad, sig_acima);
    double md_sig = calcular_distancia_meridiana(ra_sig, ramc, sig_acima);
    double cota_mundana_sig = md_sig / sa_sig;

    // Multiplicadores para os aspectos mundanos
    double mult_aspectos[] = {0.0, 0.333333, 0.5, 0.666667, 1.0}; // Conjunção, Sextil, Quadratura, Trígono, Oposição
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍"};

    for (int p = 0; p < 81; p++) {
        //if (p == idx_alvo) continue;
        if (prom[p].type == PROM_TERM) continue;

        // 2. Dados tridimensionais REAIS do Promissor
        double ra_prom = calcular_ra(prom[p].longitude, prom[p].declination, jd);
        double dec_prom_rad = para_radianos(prom[p].declination);
        int prom_acima = ((prom[p].house) >= 7 && (prom[p].house) <= 12);

        double sa_prom = calcular_semi_arco(dec_prom_rad, lat_geo_rad, prom_acima);
        double md_prom = calcular_distancia_meridiana(ra_prom, ramc, prom_acima);

        for (int s = 0; s < 2; s++) { // 0 = Direta, 1 = Conversa
            
            // FILTRO CRÍTICO DE SENTIDO: Se o usuário filtrou por um sentido específico, pula o outro
            if (s == 0 && sentido == 1) continue; // Usuário quer apenas conversas (1), pula a direta (0)
            if (s == 1 && sentido == 0) continue; // Usuário quer apenas diretas (0), pula a conversa (1)

            for (int a = 0; a < 5; a++) {

                double md_destino = 0.0;
                double arco = 0.0;
                
                // Calcula a distância meridiana onde o aspecto do promissor se projeta no espaço
                double md_prom_aspecto = md_prom + (sa_prom * mult_aspectos[a]);
                
                if (s == 0) { // Direta
                    // O aspecto do Promissor se move até a cota proporcional do Significador
                    md_destino = sa_prom * cota_mundana_sig;
                    arco = md_prom_aspecto - md_destino;   
                }
                else if (s == 1) { // Conversa
                    // O Significador se move até a cota proporcional do aspecto do Promissor
                    md_destino = sa_sig * (md_prom_aspecto / sa_prom);
                    arco = md_destino - md_sig;                               
                }

                // Correção de rotação circular esférica
                if (arco < 0) arco += 360.0; 

                // Filtra arcos de idade humana viável (0 a 150 anos)
                if (arco > 0.0 && arco <= MAX_AGE) {
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];

                    d->sentido = s; // Salva 0 para direta ou 1 para conversa
                    
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    
                    strcpy(d->significador_name, parts[idx_alvo].name);

                    char abreviacao[10];
                    get_part_abbreviation(parts[idx_alvo].name, abreviacao);
            
                    
                    strcpy(d->significador_glifo, abreviacao);

                    d->promissor_type = prom[p].type;
                    
                    // 1. Calcula o arco e a idade do evento normalmente
                    d->arco_graus = arco;
                    d->idade_evento = arco / NAIBOD_KEY; 

                    // 2. Transforma a idade em dias exatos
                    double dias_decorridos = d->idade_evento * 365.242199;

                    // 3. Calcula o Dia Juliano do evento
                    double jd_evento = jd + dias_decorridos;

                    // 4. Converte o Dia Juliano para data do calendário (Gregoriano/Juliano automático)
                    int ano_c, mes_c, dia_c, hora_c, min_c;
                    double sec_c;
                    swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);

                    // 5. Alimenta a estrutura com as datas
                    d->ano_calendario = ano_c;
                    d->mes_calendario = mes_c;
                    d->dia_calendario = dia_c;
                                    
                    strcpy(d->tipo_direcao, _("Mundane")); // CORREÇÃO: Identifica corretamente como Mundana

                    qtd_direcoes++;
                    if (qtd_direcoes >= 300) goto fim_calculo; // Sai de forma limpa se estourar o limite
                }
            }
        }
    }

fim_calculo:
    qsort(lista_resultado, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);
    return qtd_direcoes;
}
