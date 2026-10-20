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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Funções matemáticas utilitárias para conversão
static double para_radianos(double graus) { return graus * M_PI / 180.0; }
static double para_graus(double radianos) { return radianos * 180.0 / M_PI; }




double get_obliquidade(double jd) {
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


/**
 * Converte um ponto da Eclíptica pura (Zodíaco) para coordenadas equatoriais
 * @param lon_aspecto Longitude zodiacal do aspecto calculado (0 a 360)
 * @param obliquidade Obliquidade verdadeira calculada para a data natal
 * @param out_ra Retorno da Ascensão Reta (RA) do ponto do aspecto
 * @param out_dec Retorno da Declinação (DEC) do ponto do aspecto
 */
void converter_ponto_zodiacal_para_equatorial(double lon_aspecto, double obliquidade, double *out_ra, double *out_dec) {
    double xin[3], xout[3];
    
    xin[0] = lon_aspecto; // Longitude Eclíptica (λ)
    xin[1] = 0.0;         // Latitude Eclíptica (β) = 0.0 para aspecto puro no zodíaco
    xin[2] = 1.0;         // Distância/Raio normalizado
    
    // Rotação de matriz tridimensional nativa da Swiss Ephemeris
    // Passa-se a obliquidade com sinal negativo para Eclíptica -> Equador
    swe_cotrans(xin, xout, -obliquidade);
    
    *out_ra = xout[0];
    *out_dec = xout[1];
}



double obter_ano_tropical_dinamico(double jd_natal) {
    // J2000.0 corresponde ao Dia Juliano 2451545.0
    // 36525.0 é o número de dias em um século juliano
    double T = (jd_natal - 2451545.0) / 36525.0;

    // Fórmula polinomial de Laskar para a variação do Ano Tropical
    double ano_tropical = 365.2421896698 - (0.00000615359 * T) - (0.000000000729 * T * T);

    return ano_tropical;
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









// DIREÇÕES MUNDANAS

/**
 * Calcula os Semi-Arcos Diurno e Noturno de um ponto celeste
 * @param latitude_geo Latitude do local de nascimento (-90.0 a 90.0, Sul é negativo)
 * @param declinacao Declinação do planeta/ponto (-90.0 a 90.0, Sul é negativo)
 */
SemiArcos calcular_semi_arcos(double latitude_geo, double declinacao) {
    SemiArcos resultado;
    
    // Converte para radianos para as funções do C
    double lat_rad = latitude_geo * DEG_TO_RAD;
    double dec_rad = declinacao * DEG_TO_RAD;
    
    // Calcula o argumento interno: tan(phi) * tan(delta)
    double argumento = tan(lat_rad) * tan(dec_rad);
    
    double ad = 0.0; // Diferença Ascensional
    
    // TRATAMENTO DE FRONTEIRA: Regiões polares ou corpos circumpolares
    if (argumento >= 1.0) {
        // O planeta nunca se põe (Dia Perpétuo para este corpo)
        resultado.sad = 180.0;
        resultado.san = 0.0;
        return resultado;
    } else if (argumento <= -1.0) {
        // O planeta nunca nasce (Noite Perpétua para este corpo)
        resultado.sad = 0.0;
        resultado.san = 180.0;
        return resultado;
    } else {
        // Caso padrão: Calcula a Diferença Ascensional em graus
        ad = asin(argumento) * RAD_TO_DEG;
    }
    
    // Aplicação das fórmulas clássicas
    resultado.sad = 90.0 + ad;
    resultado.san = 90.0 - ad;
    
    return resultado;
}




double calcular_posicao_mundana_topocentrica(double ra, double dec, double ramc, double lat_geo, double *out_md, double *out_hd) {
    // 1. Calcula a menor Distância Meridiana (MD) em relação ao Meio do Céu (Meridiano)
    double md_mc = fabs(ra - ramc);
    if (md_mc > 180.0) md_mc = 360.0 - md_mc;
    
    // Determina se está geometricamente acima ou abaixo do horizonte de forma topocêntrica
    // (Em termos de coordenadas equatoriais puras para o cálculo do Polo)
    double lat_rad = lat_geo * DEG_TO_RAD;
    double dec_rad = dec * DEG_TO_RAD;
    double md_rad  = md_mc * DEG_TO_RAD;
    
    // 2. EQUAÇÃO DO POLO TOPOCÊNTRICO (O coração do sistema Polich-Page)
    // tan(Polo) = tan(Latitude Geográfica) * sin(Distância Meridiana)
    double tan_polo = tan(lat_rad) * sin(md_rad);
    double polo_astro_rad = atan(tan_polo);
    
    // 3. Diferença Ascensional Topocêntrica (AD) do corpo sob o seu próprio polo
    double argumento_ad = tan(polo_astro_rad) * tan(dec_rad);
    double ad_topo = 0.0;
    if (argumento_ad < 1.0 && argumento_ad > -1.0) {
        ad_topo = asin(argumento_ad) * RAD_TO_DEG;
    } else {
        ad_topo = (argumento_ad >= 1.0) ? 90.0 : -90.0;
    }
    
    // No método Topocêntrico, a "Distância Horária" (hd) no equador equivalente 
    // é derivada da relação entre a Distância Meridiana e a Diferença Ascensional do Polo
    // Mapeando um comportamento análogo à triseção espacial.
    double hd_topo = (md_mc + ad_topo) / 90.0; 
    
    // Filtros de segurança para quadrantes
    if (hd_topo < 0.0) hd_topo = 0.0;
    
    *out_md = md_mc;
    *out_hd = hd_topo; // Passa a HD modificada pela curvatura topocêntrica
    
    // Identifica se o astro está no Hemisfério Oriental ou Ocidental
    double diff_ramc = ra - ramc;
    while (diff_ramc < -180.0) diff_ramc += 360.0;
    while (diff_ramc > 180.0)  diff_ramc -= 360.0;
    int e_oriental = (diff_ramc < 0.0) ? 1 : 0;
    
    // Determina se está acima ou abaixo testando se a Ascensão Reta cruza o horizonte oblíquo
    int esta_acima = (md_mc <= (90.0 + ad_topo)) ? 1 : 0;
    
    double pm = 0.0;
    if (esta_acima) {
        pm = e_oriental ? (270.0 + (hd_topo * 90.0)) : (270.0 - (hd_topo * 90.0));
    } else {
        // Para o hemisfério inferior, recalculamos a distância em relação ao Fundo do Céu
        double ra_ic = ramc + 180.0;
        if (ra_ic >= 360.0) ra_ic -= 360.0;
        double md_ic = fabs(ra - ra_ic);
        if (md_ic > 180.0) md_ic = 360.0 - md_ic;
        
        // Reconstrói a HD inferior topocêntrica
        double tan_polo_ic = tan(lat_rad) * sin(md_ic * DEG_TO_RAD);
        double polo_ic_rad = atan(tan_polo_ic);
        double ad_topo_ic = asin(tan(polo_ic_rad) * tan(dec_rad)) * RAD_TO_DEG;
        double hd_topo_ic = (md_ic + ad_topo_ic) / 90.0;
        
        *out_md = md_ic;
        *out_hd = hd_topo_ic;
        
        pm = e_oriental ? (0.0 + (hd_topo_ic * 90.0)) : (180.0 - (hd_topo_ic * 90.0));
    }
    
    pm = fmod(pm, 360.0);
    if (pm < 0.0) pm += 360.0;
    return pm;
}


double calcular_posicao_mundana_placidus(double ra, double ramc, SemiArcos sa, double *out_md, double *out_hd) {
    // Distância em relação ao Meio do Céu
    double md_mc = fabs(ra - ramc);
    if (md_mc > 180.0) md_mc = 360.0 - md_mc;
    
    double md = 0.0, hd = 0.0, sa_usado = 0.0;
    int esta_acima = (md_mc <= sa.sad) ? 1 : 0;
    
    if (esta_acima) {
        sa_usado = sa.sad;
        md = md_mc;
    } else {
        sa_usado = sa.san;
        double ra_ic = ramc + 180.0;
        if (ra_ic >= 360.0) ra_ic -= 360.0;
        md = fabs(ra - ra_ic);
        if (md > 180.0) md = 360.0 - md;
    }
    
    hd = (sa_usado > 1e-6) ? (md / sa_usado) : 0.0;
    *out_md = md;
    *out_hd = hd;
    
    // Identifica se está no Hemisfério Oriental ou Ocidental
    double diff_ramc = ra - ramc;
    while (diff_ramc < -180.0) diff_ramc += 360.0;
    while (diff_ramc > 180.0)  diff_ramc -= 360.0;
    int e_oriental = (diff_ramc < 0.0) ? 1 : 0;
    
    double pm = 0.0;
    if (esta_acima) {
        pm = e_oriental ? (270.0 + (hd * 90.0)) : (270.0 - (hd * 90.0));
    } else {
        pm = e_oriental ? (0.0 + (hd * 90.0)) : (180.0 - (hd * 90.0));
    }
    
    pm = fmod(pm, 360.0);
    if (pm < 0.0) pm += 360.0;
    return pm;
}



/**
 * Injeta as constantes mundanas absolutas com base no número da casa (1 a 12)
 * @param house_number Número da casa de 1 a 12 (1=Asc, 4=IC, 7=Dsc, 10=MC)
 * @param astro Ponteiro para a estrutura do seu objeto
 */
void injetar_constantes_mundanas_por_casa(int house_number, Promissor *astro) {
    // Valores padrão neutros para eixos fixos locais
    astro->sa.sad = 90.0;
    astro->sa.san = 90.0;

    switch (house_number) {
        case 1:  // Casa 1 / Ascendente (Horizonte Oriental)
            astro->pm = 0.0;   astro->hd = 1.0;       astro->md = 90.0; break;
        case 2:  // Casa 2
            astro->pm = 30.0;  astro->hd = 2.0 / 3.0; astro->md = 60.0; break;
        case 3:  // Casa 3
            astro->pm = 60.0;  astro->hd = 1.0 / 3.0; astro->md = 30.0; break;
        case 4:  // Casa 4 / Fundo do Céu (Meridiano Inferior)
            astro->pm = 90.0;  astro->hd = 0.0;       astro->md = 0.0;  break;
        case 5:  // Casa 5
            astro->pm = 120.0; astro->hd = 1.0 / 3.0; astro->md = 30.0; break;
        case 6:  // Casa 6
            astro->pm = 150.0; astro->hd = 2.0 / 3.0; astro->md = 60.0; break;
        case 7:  // Casa 7 / Descendente (Horizonte Ocidental)
            astro->pm = 180.0; astro->hd = 1.0;       astro->md = 90.0; break;
        case 8:  // Casa 8
            astro->pm = 210.0; astro->hd = 2.0 / 3.0; astro->md = 60.0; break;
        case 9:  // Casa 9
            astro->pm = 240.0; astro->hd = 1.0 / 3.0; astro->md = 30.0; break;
        case 10: // Casa 10 / Meio do Céu (Meridiano Superior)
            astro->pm = 270.0; astro->hd = 0.0;       astro->md = 0.0;  break;
        case 11: // Casa 11
            astro->pm = 300.0; astro->hd = 1.0 / 3.0; astro->md = 30.0; break;
        case 12: // Casa 12
            astro->pm = 330.0; astro->hd = 2.0 / 3.0; astro->md = 60.0; break;
        default:
            // Fallback de segurança para evitar comportamentos indefinidos
            astro->pm = 0.0;   astro->hd = 0.0;       astro->md = 0.0;  break;
    }
}





int processar_astro(double ramc, double lat_geo, Promissor *astro) {
    if (astro->type == PROM_ANGLE || astro->type == PROM_CUSP) {        
        if (astro->house >= 1 && astro->house <= 12) {
            injetar_constantes_mundanas_por_casa(astro->house, astro);
            return 0;
        }
    }

    astro->sa = calcular_semi_arcos(lat_geo, astro->declination);

    if (METODO_CALCULO_ATIVO == METODO_PLACIDUS_SEMI_ARCO) {
        astro->pm = calcular_posicao_mundana_placidus(astro->ra, ramc, astro->sa, &astro->md, &astro->hd);
    }
    else if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
        astro->pm = calcular_posicao_mundana_topocentrica(astro->ra, astro->declination, ramc, lat_geo, &astro->md, &astro->hd);
    }

    return 0;
}



void calcular_paralelos_mundanos_topocentrico(Promissor *sig, Promissor *prom, int s, double *arco_p, double *arco_cp) {
    if (s == 0) { // DIRETA
        // No Topocêntrico, o alvo equatorial é simplesmente a HD do Significador calculada na escala de 90 graus
        double md_alvo = sig->hd * 90.0;
        *arco_p = fabs(md_alvo - prom->md);
        
        // Contra-paralelo topocêntrico (mesma distância reflexa no equador)
        *arco_cp = fabs(md_alvo - prom->md); 
    } 
    else if (s == 1) { // CONVERSA
        double md_alvo = prom->hd * 90.0;
        *arco_p = fabs(md_alvo - sig->md);
        *arco_cp = fabs(md_alvo - sig->md);
    }
}




/**
 * Calcula o Arco de Paralelo e Contra-Paralelo Mundano (Direto e Converso)
 */
void calcular_paralelos_mundanos_direta(Promissor *sig, Promissor *prom, double *p_direto, double *cp_direto) {
    // --- 1. DIREÇÃO DIRETA (Promissor viaja até a proporção do Significador) ---
    // Regulado pelo Semi-Arco do Promissor
    *p_direto = fabs((sig->hd * prom->sa.sad) - prom->md);
    
    double sa_prom_oposto = (prom->md <= prom->sa.sad) ? prom->sa.san : prom->sa.sad;
    *cp_direto = fabs((sig->hd * sa_prom_oposto) - prom->md);    
}


void calcular_paralelos_mundanos_conversa(Promissor *sig, Promissor *prom, double *p_converso, double *cp_converso) {
    // --- 2. DIREÇÃO CONVERSA (Significador viaja até a proporção do Promissor) ---
    // Regulado pelo Semi-Arco do Significador
    *p_converso = fabs((prom->hd * sig->sa.sad) - sig->md);
    
    double sa_sig_oposto = (sig->md <= sig->sa.sad) ? sig->sa.san : sig->sa.sad;
    *cp_converso = fabs((prom->hd * sa_sig_oposto) - sig->md);    
}


// /**
//  * Calcula o Arco de Paralelo Rapto e Contra-Paralelo Rapto (Método de Regiomontanus/Placidus)
//  */
// void calcular_paralelos_rapto(Promissor *sig, Promissor *prom, double *arco_rapto, double *arco_contra_rapto) {
//     double sa_sig_uso = (sig->md <= sig->sa.sad) ? sig->sa.sad : sig->sa.san;
//     double sa_prom_uso = (prom->md <= prom->sa.sad) ? prom->sa.sad : prom->sa.san;
    
//     // Equação do Paralelo Rapto Puro
//     double numerador_rapto = (prom->md * sa_sig_uso) - (sig->md * sa_prom_uso);
//     double denominador = sa_sig_uso + sa_prom_uso;
    
//     *arco_rapto = (denominador > 1e-6) ? fabs(numerador_rapto / denominador) : 0.0;
    
//     // Equação do Contra-Paralelo Rapto
//     double numerador_contra = (prom->md * sa_sig_uso) + (sig->md * sa_prom_uso);
//     *arco_contra_rapto = (denominador > 1e-6) ? fabs(numerador_contra / denominador) : 0.0;
// }


void calcular_paralelos_rapto(Promissor *sig, Promissor *prom, double *arco_rapto, double *arco_contra_rapto) {
    
    if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
        // MATEMÁTICA TOPOCÊNTRICA PURA (Polich-Page)
        // O rapto ocorre na linha média exata das distâncias meridianas
        *arco_rapto = fabs(prom->md - sig->md) / 2.0;
        
        // O contra-rapto ocorre na soma das distâncias dividida por dois
        *arco_contra_rapto = (prom->md + sig->md) / 2.0;
    } 
    else if (METODO_CALCULO_ATIVO == METODO_PLACIDUS_SEMI_ARCO) {
        // MATEMÁTICA CLÁSSICA DE PLACIDUS (Código que você já tinha)
        double sa_sig_uso = (sig->md <= sig->sa.sad) ? sig->sa.sad : sig->sa.san;
        double sa_prom_uso = (prom->md <= prom->sa.sad) ? prom->sa.sad : prom->sa.san;
        
        double denominador = sa_sig_uso + sa_prom_uso;
        if (denominador > 1e-6) {
            *arco_rapto = fabs((prom->md * sa_sig_uso) - (sig->md * sa_prom_uso)) / denominador;
            *arco_contra_rapto = ((prom->md * sa_sig_uso) + (sig->md * sa_prom_uso)) / denominador;
        } else {
            *arco_rapto = 0.0;
            *arco_contra_rapto = 0.0;
        }
    }
}



int calc_mundane_directions(Promissor *sig, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promissor *promissors, int *mundane_aspects) {
    int qtd_direcoes = 0;

    processar_astro(ramc, lat_geografica, sig);

    char *simbolos_aspectos[] = {"☌", "⚹", "⚹",   "□",  "□",   "△",   "△",    "☍",    "∥",    "∦",   "R∥",  "R∦"};
    double desvios_mundanos[] = {0.0, 60.0, -60.0, 90.0, -90.0, 120.0, -120.0, 180.0,  999.9, 999.9, 999.9, 999.9};
    int total_aspects = 12;

    for (int p = 0; p < prom_id; p++) {
        Promissor *prom = &promissors[p];

        if (prom->type == PROM_POINT || 
            prom->type == PROM_ANGLE || 
            prom->type == PROM_PART  || 
            prom->type == PROM_TERM
        ) continue;

        if ((prom->type == PROM_ANTISCIUM || prom->type == PROM_CONTRANTISCIUM) && !ANT_PROM) continue;
        if ((prom->type == PROM_ANTISCIUM || prom->type == PROM_CONTRANTISCIUM) && strstr(prom->object, "🝴") != NULL) continue;

        processar_astro(ramc, lat_geografica, prom);

        for (int s = 0; s < 2; s++) {            
            if (s == 0 && sentido == 1) continue; 
            if (s == 1 && sentido == 0) continue; 

            for (int a = 0; a < total_aspects; a++) {
                if (!mundane_aspects[a]) continue;

                if ((sig->type == PROM_ANGLE || sig->type == PROM_CUSP) && (a > 0 && a < 7)) continue;

                double arco = 0.0;

                if (a < 8) {
                    if (s == 0) {
                        double pm_alvo_direto = sig->pm + desvios_mundanos[a];
                        pm_alvo_direto = fmod(pm_alvo_direto, 360.0);
                        if (pm_alvo_direto < 0.0) pm_alvo_direto += 360.0;
                        
                        double dist_direta = fabs(pm_alvo_direto - prom->pm);
                        if (dist_direta > 180.0) dist_direta = 360.0 - dist_direta;
                        
                        if (METODO_CALCULO_ATIVO == METODO_PLACIDUS_SEMI_ARCO) {
                            // Conversão para RA usando o Semi-Arco do Promissor (ele é o corpo que se move)
                            arco = dist_direta * (prom->sa.sad / 90.0);
                        }
                        else if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
                            arco = dist_direta;
                        }
                    }
                    else if (s == 1) {
                        double pm_alvo_converso = prom->pm + desvios_mundanos[a]; 
                        pm_alvo_converso = fmod(pm_alvo_converso, 360.0);
                        if (pm_alvo_converso < 0.0) pm_alvo_converso += 360.0;
                        
                        double dist_conversa = fabs(sig->pm - pm_alvo_converso);
                        if (dist_conversa > 180.0) dist_conversa = 360.0 - dist_conversa;
                        
                        if (METODO_CALCULO_ATIVO == METODO_PLACIDUS_SEMI_ARCO) {
                            // ATENÇÃO CONCEITUAL: Na direção conversa clássica de Placidus, como é o Significador 
                            // que se move em direção ao aspecto do Promissor, usamos o Semi-Arco do SIGNIFICADOR!
                            arco = dist_conversa * (sig->sa.sad / 90.0);
                        }
                        else if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
                            arco = dist_conversa;
                        }
                    }
                }
                else if (a == 8 || a == 9) {
                    if (s == 0) {
                        double p_direto = 0.0;
                        double cp_direto = 0.0;
                        if (METODO_CALCULO_ATIVO == METODO_PLACIDUS_SEMI_ARCO) {
                            calcular_paralelos_mundanos_direta(sig, prom, &p_direto, &cp_direto);
                        }
                        else if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
                            calcular_paralelos_mundanos_topocentrico(sig, prom, s, &p_direto, &cp_direto);
                        }
                        if (a == 8) arco = p_direto;
                        else        arco = cp_direto;
                    }
                    else if (s == 1) {
                        double p_converso = 0.0;
                        double cp_converso = 0.0;
                        if (METODO_CALCULO_ATIVO == METODO_PLACIDUS_SEMI_ARCO) {
                            calcular_paralelos_mundanos_conversa(sig, prom, &p_converso, &cp_converso);
                        }
                        else if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
                            calcular_paralelos_mundanos_topocentrico(sig, prom, s, &p_converso, &cp_converso);
                        }
                        if (a == 8) arco = p_converso;
                        else        arco = cp_converso;
                    }
                }
                else if (a == 10 || a == 11) {
                    // CONDICIONAL DE EXECUÇÃO ÚNICA:
                    // 1. Se o usuário quer AMBAS (sentido == 2), calcula apenas no loop Direto (s == 0) para não duplicar.
                    // 2. Se o usuário quer APENAS CONVERSAS (sentido == 1), calcula no loop Converso (s == 1), já que o s == 0 foi pulado.
                    int calcular_rapto_aqui = 0;
                    if (sentido == 2 && s == 0) calcular_rapto_aqui = 1;
                    if (sentido == 0 && s == 0) calcular_rapto_aqui = 1;
                    if (sentido == 1 && s == 1) calcular_rapto_aqui = 1;
                
                    if (calcular_rapto_aqui) {
                        double arco_rapto = 0.0;
                        double arco_contra_rapto = 0.0;
                        calcular_paralelos_rapto(sig, prom, &arco_rapto, &arco_contra_rapto);
                
                        if (a == 10) arco = arco_rapto;
                        else         arco = arco_contra_rapto;

                    } else {
                        // Ignora este aspecto nesta iteração do loop 's' para evitar duplicar no relatório
                        continue; 
                    }
                }

                if (arco < 0.0) arco += 360.0;
                arco = fmod(arco, 360.0);
      
           
                if (arco > 0.001 && arco <= MAX_AGE * 1.05) { 
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];

                    d->sig_sa = sig->sa;
                    d->sig_pm = sig->pm;
                    d->sig_hd = sig->hd;
                    
                    d->sentido = s;

                    d->promissor_id = p;
                    d->aspecto_id = a;
                    strcpy(d->promissor_name, prom->object_name);
                    strcpy(d->promissor_glifo, prom->object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    strcpy(d->significador_name, sig->object_name);
                    strcpy(d->significador_glifo, sig->object);
                    d->promissor_type = prom->type;
                    
                    d->arco_graus = arco;
                    
                    // 1. Obter a chave/fator do sistema usando suas macros estáticas
                    double CHAVE = get_time_key(TIME_KEY, jd, arco);
                    if (CHAVE <= 0.0) CHAVE = NAIBOD_KEY; 

                    // 2. CÁLCULO UNIFICADO DA IDADE DO EVENTO (Em anos decimais contínuos)
                    d->idade_evento = arco / CHAVE;
            
                    double dias_decorridos = 0.0;

                    // 3. FORK DE TRATAMENTO DE TEMPO COM PRECISÃO DE ACORDO COM CADA MACRO
                    if (TIME_KEY == TIME_KEY_NAIBOD) {
                        dias_decorridos = d->idade_evento * 365.256363004; // Inverso exato de NAIBOD_KEY
                    }
                    else if (TIME_KEY == TIME_KEY_CARDANO) {
                        dias_decorridos = d->idade_evento * 364.847162454; // Inverso exato de CARDANO_KEY
                    }
                    else if (TIME_KEY == TIME_KEY_PTOLEMY) {
                        dias_decorridos = d->idade_evento * 365.0;         // Ano Civil de Ptolomeu
                    }
                    else if (TIME_KEY == TIME_KEY_PLACIDUS) {
                        dias_decorridos = d->idade_evento * 365.25;        // Ano Juliano Eclesiástico
                    }
                    else if (TIME_KEY == TIME_KEY_KEPLER) {
                        d->idade_evento = arco * CHAVE;
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
                    else if (TIME_KEY == TIME_KEY_TRUE_SOLAR_ARC_RA) {
                        d->idade_evento = arco * CHAVE;
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
                    else {                                
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
            
                    // 4. Projeção Estrita no Dia Juliano e Conversão UTC via Swiss Ephemeris
                    double jd_evento = jd + dias_decorridos;
            
                    int ano_c, mes_c, dia_c, hora_c, min_c;
                    double sec_c;
                    swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);
            
                    d->ano_calendario = ano_c;
                    d->mes_calendario = mes_c;
                    d->dia_calendario = dia_c;
                                   
                    strcpy(d->tipo_direcao, _("Mundane"));
                    d->tipo_direcao_id = DIRECAO_MUNDANA;

                    qtd_direcoes++;
                    if (qtd_direcoes >= 600) goto fim_calculo;
                }
            }
        }
    }  
fim_calculo:
    qsort(lista_resultado, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);

    if (qtd_direcoes > 1) {
        int i_valido = 0;

        for (int i_atual = 1; i_atual < qtd_direcoes; i_atual++) {
            // Mudança sutil: Comparamos o atual com o último elemento que aceitamos como válido
            LinhaDirecao *d0 = &lista_resultado[i_valido];
            LinhaDirecao *d1 = &lista_resultado[i_atual];

            bool mesmo_promissor = (d0->promissor_id == d1->promissor_id);
            bool mesmo_sentido   = (d0->sentido == d1->sentido);
            
            double delta_arco = fabs(d1->arco_graus - d0->arco_graus);
            bool arco_identico = (delta_arco < 0.0001); // Use um epsilon pequeno explícito (ex: 0.0001)

            // Se o mesmo planeta, no mesmo sentido (Direto/Converso), atingir o mesmo arco geométrico...
            if (mesmo_promissor && mesmo_sentido && arco_identico) {
                continue; // Ignora o d1 (duplicado)
            }

            // Se não for duplicado, movemos o ponteiro de gravação
            i_valido++;
            if (i_valido != i_atual) {
                lista_resultado[i_valido] = *d1;
            }
        }

        qtd_direcoes = i_valido + 1;
    }
    return qtd_direcoes;

}


// FIM - DIREÇÕES MUNDANAS


// DIREÇÕES ZODIACAIS

// Estrutura interna temporária para gerenciar as coordenadas do ponto do aspecto no loop
typedef struct {
    double ra;
    double dec;
    double sad;
    double san;
    double md;
    double hd;
} CoordenadasPontoAspecto;

/**
 * Calcula as distâncias locais e semi-arcos dinâmicos para um ponto equatorial do aspecto
 * @param ra_aspecto RA calculada para o ponto do aspecto
 * @param dec_aspecto Declinação calculada para o ponto do aspecto
 * @param ramc RAMC natal do mapa
 * @param lat_geo Latitude geográfica do nascimento
 * @param out_coord Ponteiro para preencher os dados dinâmicos do aspecto
 */
void calcular_coordenadas_locais_aspecto(double ra_aspecto, double dec_aspecto, double ramc, double lat_geo, CoordenadasPontoAspecto *out_coord) {
    out_coord->ra = ra_aspecto;
    out_coord->dec = dec_aspecto;
    
    // 1. Cálculo dos Semi-Arcos dinâmicos do aspecto
    double lat_rad = lat_geo * (M_PI / 180.0);
    double dec_rad = dec_aspecto * (M_PI / 180.0);
    double argumento = tan(lat_rad) * tan(dec_rad);
    double ad = 0.0;
    
    if (argumento >= 1.0) {
        out_coord->sad = 180.0; out_coord->san = 0.0;
    } else if (argumento <= -1.0) {
        out_coord->sad = 0.0;  out_coord->san = 180.0;
    } else {
        ad = asin(argumento) * (180.0 / M_PI);
        out_coord->sad = 90.0 + ad;
        out_coord->san = 90.0 - ad;
    }
    
    // 2. Distância Meridiana (MD) do ponto do aspecto em relação ao Meio do Céu (RAMC)
    double md_mc = fabs(ra_aspecto - ramc);
    if (md_mc > 180.0) md_mc = 360.0 - md_mc;
    
    // 3. Critério de Placidus para o Ponto do Aspecto
    int esta_acima = (md_mc <= out_coord->sad) ? 1 : 0;
    double sa_uso = esta_acima ? out_coord->sad : out_coord->san;
    
    if (esta_acima) {
        out_coord->md = md_mc;
    } else {
        double ra_ic = ramc + 180.0;
        if (ra_ic >= 360.0) ra_ic -= 360.0;
        
        out_coord->md = fabs(ra_aspecto - ra_ic);
        if (out_coord->md > 180.0) out_coord->md = 360.0 - out_coord->md;
    }
    
    // 4. Distância Horária (HD) proporcional do ponto do aspecto
    out_coord->hd = (sa_uso > 1e-6) ? (out_coord->md / sa_uso) : 0.0;
}



/**
 * Calcula o Polo de um astro de acordo com o método escolhido
 */
double calcular_polo_astro(Promissor *astro, double lat_geo) {
    double lat_rad = lat_geo * DEG_TO_RAD;
    
    if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
        // No Topocêntrico, o Polo depende exclusivamente da Latitude e da MD do astro
        double tan_polo = tan(lat_rad) * sin(astro->md * DEG_TO_RAD);
        return atan(tan_polo) * RAD_TO_DEG;

    } else if (METODO_CALCULO_ATIVO == METODO_PLACIDUS_SEMI_ARCO) {
        // Em Placidus, o Polo (Regiomontanus-Placidus approximation) usa a Proporção Horária (HD)
        // sin(Polo) = sin(Latitude) * sin(HD * 90)
        double sin_polo = sin(lat_rad) * sin(astro->hd * 90.0 * DEG_TO_RAD);
        return asin(sin_polo) * RAD_TO_DEG;
    }

    return 0.0;
}

/**
 * Calcula o Arco de Direção Zodiacal para Paralelo e Contra-Paralelo
 * @param s Sentido: 0 = Direta (Promissor move-se), 1 = Conversa (Significador move-se)
 */
void calcular_paralelos_zodiacais(Promissor *sig, Promissor *prom, double lat_geo, int s, double *arco_p, double *arco_cp) {
    // Determina quem é o alvo (parado) e quem se move
    Promissor *alvo = (s == 0) ? sig : prom;
    Promissor *movel = (s == 0) ? prom : sig;
    
    // O Polo que regula a rotação equatorial deve ser o do planeta que se move
    double polo_movel = calcular_polo_astro(movel, lat_geo);
    double polo_rad = polo_movel * DEG_TO_RAD;
    
    // Coleta a declinação do alvo natal que queremos atingir
    double dec_alvo_p  = alvo->declination;
    double dec_alvo_cp = -alvo->declination; // Sinal invertido para Contra-Paralelo
    
    // Para encontrar o arco, calculamos a Ascensão Obíqua (AO) do alvo sob o Polo do móvel
    // AO = RA - ad_polo -> onde sin(ad_polo) = tan(polo) * tan(dec)
    double sin_ad_p = tan(polo_rad) * tan(dec_alvo_p * DEG_TO_RAD);
    double sin_ad_cp = tan(polo_rad) * tan(dec_alvo_cp * DEG_TO_RAD);
    
    double ad_p = (sin_ad_p < 1.0 && sin_ad_p > -1.0) ? asin(sin_ad_p) * RAD_TO_DEG : 0.0;
    double ad_cp = (sin_ad_cp < 1.0 && sin_ad_cp > -1.0) ? asin(sin_ad_cp) * RAD_TO_DEG : 0.0;
    
    // Coleta a RA do alvo natal
    double ao_alvo_p  = alvo->ra - ad_p;
    double ao_alvo_cp = alvo->ra - ad_cp;
    
    // Coleta a AO natal do planeta móvel sob seu próprio polo
    double sin_ad_movel = tan(polo_rad) * tan(movel->declination * DEG_TO_RAD);
    double ad_movel = (sin_ad_movel < 1.0 && sin_ad_movel > -1.0) ? asin(sin_ad_movel) * RAD_TO_DEG : 0.0;
    double ao_movel = movel->ra - ad_movel;
    
    // Arco final é a diferença das Ascensões Obíquas sob o mesmo plano polar
    *arco_p  = ao_alvo_p - ao_movel;
    *arco_cp = ao_alvo_cp - ao_movel;
    
    // Normalizações circulares de segurança (0 a 360)
    while (*arco_p < 0.0)  {
        *arco_p += 360.0; 
        *arco_p = fmod(*arco_p, 360.0);
    }
    while (*arco_cp < 0.0) {
        *arco_cp += 360.0; 
        *arco_cp = fmod(*arco_cp, 360.0);
    }
}


/**
 * Calcula o arco de direção zodiacal de círculo com base no método escolhido
 */
double obter_arco_zodiacal_de_circulo(CoordenadasPontoAspecto coord_ponto, Promissor *movel) {
    if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
        // No Topocêntrico Zodiacal (Polich-Page), o arco é a diferença de Ascensão Reta 
        // sob o horizonte do Polo do aspecto equivalente. No zodíaco puro, mapeia-se:
        double arco = coord_ponto.ra - movel->ra;
        while (arco < 0.0) arco += 360.0;
        return fmod(arco, 360.0);

    } else if (METODO_CALCULO_ATIVO == METODO_PLACIDUS_SEMI_ARCO) {
        // Em Placidus Zodiacal clássico, o movimento iguala as Proporções Horárias (HD)
        // Arco = | (HD_movel_natal * SA_ponto_aspecto) - MD_ponto_aspecto |
        // Precisamos checar se o ponto do aspecto está acima ou abaixo para usar o SA correto
        double sa_ponto = (coord_ponto.md <= coord_ponto.sad) ? coord_ponto.sad : coord_ponto.san;
        
        double arco = fabs((movel->hd * sa_ponto) - coord_ponto.md);
        return arco;
    }

    return 0.0;
}


/**
 * Motor Principal de Direções Primárias Zodiacais (Eclípticas)
 * Suporta os métodos Placidus e Topocêntrico com remoção de duplicidades pós-qsort.
 */
int calc_zodiacal_directions(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promissor *promissors, bool is_part) {
    int qtd_direcoes = 0;
    double obliquidade = get_obliquidade(jd);

    int object_diff = show_modern_planets ? 0 : 3;

    char *simbolos_aspectos[] = {"☌", "⚹", "⚹",   "□",  "□",   "△",   "△",    "☍",    "∥",    "∦"};
    double desvios_zodiacos[] = {0.0, 60.0, -60.0, 90.0, -90.0, 120.0, -120.0, 180.0,  999.9,  999.9};
    int total_aspects = 10; // Reduzido para 10 (excluindo os dois Paralelos-Rapto)

    bool sig_is_fortune = strcmp(sig->object, "FOR") == 0;

    for (int p = 0; p < prom_id; p++) {
        Promissor *prom = &promissors[p];

        if ((prom->type == PROM_ANTISCIUM || prom->type == PROM_CONTRANTISCIUM) && !ANT_PROM) continue;
        else if (prom->type == PROM_POINT || prom->type == PROM_ANGLE) continue;
        else if (!is_part) {
            if ((p == idx_alvo && p < NUM_OBJECTS - object_diff - (show_modern_planets ? 5 : 4))) continue;
        }
        else {
            if (sig_is_fortune && (p == P_FORTUNA - object_diff)) continue;
        }

        for (int s = 0; s < 2; s++) {
            if (s == 0 && sentido == 1) continue;
            if (s == 1 && sentido == 0) continue;
            
            for (int a = 0; a < total_aspects; a++) {

                if (prom->type == PROM_TERM && a > 0) break;

                double arco = 0.0;

                if (a < 8) {
                    double lon_base = 0.0;
                    Promissor *movel = NULL;
                
                    if (s == 0) {
                        // --- DIREÇÃO DIRETA ---
                        if (prom->type == PROM_TERM) {
                            // EXCEÇÃO PARA TERMOS DIRETOS:
                            // O alvo parado passa a ser a fronteira inicial do Termo.
                            // O corpo móvel que viaja até ele será o próprio Significador.
                            lon_base = prom->longitude;
                            movel = sig; 
                        } else {
                            // Lógica padrão para planetas normais na Direta
                            lon_base = sig->longitude;
                            movel = prom;
                        }
                    } else {
                        // --- DIREÇÃO CONVERSA ---
                        if (prom->type == PROM_TERM) {
                            // O alvo parado é a fronteira final do Termo (o que você já corrigiu)
                            lon_base = prom->longitude_fim;
                            movel = sig;
                        } else {
                            // Lógica padrão para planetas normais na Conversa
                            lon_base = prom->longitude;
                            movel = sig;
                        }
                    }
                
                    // Projeta o aspecto (para Termos, desvios_zodiacos[0] = 0.0, mantendo a lon_base pura)
                    double lon_aspecto_alvo = lon_base + desvios_zodiacos[a];
                    lon_aspecto_alvo = fmod(lon_aspecto_alvo, 360.0);
                    if (lon_aspecto_alvo < 0.0) lon_aspecto_alvo += 360.0;
                
                    // Converte o ponto alvo (fronteira do termo) em coordenadas equatoriais
                    double ra_ponto = 0.0, dec_ponto = 0.0;
                    converter_ponto_zodiacal_para_equatorial(lon_aspecto_alvo, obliquidade, &ra_ponto, &dec_ponto);
                
                    // Calcula os semi-arcos dinâmicos e flutuantes da fronteira do termo específica
                    CoordenadasPontoAspecto coord_ponto;
                    calcular_coordenadas_locais_aspecto(ra_ponto, dec_ponto, ramc, lat_geografica, &coord_ponto);
                
                    // Agora a função calculará o arco usando a HD real do planeta movel contra o SA flutuante do Termo
                    arco = obter_arco_zodiacal_de_circulo(coord_ponto, movel);
                }

                if (arco < 0.0) arco += 360.0;
                arco = fmod(arco, 360.0);      
           
                if (arco > 0.001 && arco <= MAX_AGE * 1.05) { 
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];
                    
                    d->sentido = s;

                    d->promissor_id = p;
                    d->aspecto_id = a;
                    strcpy(d->promissor_name, prom->object_name);
                    strcpy(d->promissor_glifo, prom->object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    strcpy(d->significador_name, sig->object_name);
                    strcpy(d->significador_glifo, sig->object);
                    d->promissor_type = prom->type;
                    
                    d->arco_graus = arco;
                    
                    double CHAVE = get_time_key(TIME_KEY, jd, arco);
                    if (CHAVE <= 0.0) CHAVE = NAIBOD_KEY; 

                    d->idade_evento = arco / CHAVE;
            
                    double dias_decorridos = 0.0;

                    if (TIME_KEY == TIME_KEY_NAIBOD) {
                        dias_decorridos = d->idade_evento * 365.256363004; // Inverso exato de NAIBOD_KEY
                    }
                    else if (TIME_KEY == TIME_KEY_CARDANO) {
                        dias_decorridos = d->idade_evento * 364.847162454; // Inverso exato de CARDANO_KEY
                    }
                    else if (TIME_KEY == TIME_KEY_PTOLEMY) {
                        dias_decorridos = d->idade_evento * 365.0;         // Ano Civil de Ptolomeu
                    }
                    else if (TIME_KEY == TIME_KEY_PLACIDUS) {
                        dias_decorridos = d->idade_evento * 365.25;        // Ano Juliano Eclesiástico
                    }
                    else if (TIME_KEY == TIME_KEY_KEPLER) {
                        d->idade_evento = arco * CHAVE;
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
                    else if (TIME_KEY == TIME_KEY_TRUE_SOLAR_ARC_RA) {
                        d->idade_evento = arco * CHAVE;
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
                    else {                                
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
            
                    // 4. Projeção Estrita no Dia Juliano e Conversão UTC via Swiss Ephemeris
                    double jd_evento = jd + dias_decorridos;
            
                    int ano_c, mes_c, dia_c, hora_c, min_c;
                    double sec_c;
                    swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);
            
                    d->ano_calendario = ano_c;
                    d->mes_calendario = mes_c;
                    d->dia_calendario = dia_c;
                                   
                    strcpy(d->tipo_direcao, _("Zodiacal"));
                    d->tipo_direcao_id = DIRECAO_ZODIACAL;

                    qtd_direcoes++;
                    if (qtd_direcoes >= 600) goto fim_calculo;
                }
            } 
        }
    }                    

fim_calculo:
    qsort(lista_resultado, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);

    if (qtd_direcoes > 1) {
        int i_valido = 0;

        for (int i_atual = 1; i_atual < qtd_direcoes; i_atual++) {
            LinhaDirecao *d0 = &lista_resultado[i_valido];
            LinhaDirecao *d1 = &lista_resultado[i_atual];

            bool mesmo_promissor = (d0->promissor_id == d1->promissor_id);
            bool mesmo_sentido   = (d0->sentido == d1->sentido);
            
            double delta_arco = fabs(d1->arco_graus - d0->arco_graus);
            bool arco_identico = (delta_arco < 0.0001);
            if (mesmo_promissor && mesmo_sentido && arco_identico) {
                continue;
            }

            i_valido++;
            if (i_valido != i_atual) {
                lista_resultado[i_valido] = *d1;
            }
        }

        qtd_direcoes = i_valido + 1;
    }
    return qtd_direcoes;
}


















/**
 * Calcula a Distância Meridiana (MD) e determina automaticamente 
 * se o planeta opera no Semi-Arco Diurno ou Noturno.
 * @param ra_planeta Ascensão Reta do planeta
 * @param ramc Ascensão Reta do Meio do Céu
 * @param sa Estrutura de Semi-Arcos calculada previamente
 * @param out_semi_arco_usado Ponteiro para retornar qual SA foi usado (SAD ou SAN)
 * @return A Distância Meridiana (MD) exata entre 0° e 180°
 */
double calcular_md_e_definir_semi_arco(double ra_planeta, double ramc, SemiArcos sa, double *out_semi_arco_usado) {
    // 1. Calcula a Distância em relação ao Meio do Céu (Meridiano Superior)
    double md_mc = fabs(ra_planeta - ramc);
    if (md_mc > 180.0) {
        md_mc = 360.0 - md_mc;
    }
    
    // 2. Se a distância até o MC for menor ou igual a 90 graus + a Diferença Ascensional, 
    // ou se geometricamente o planeta estiver acima do horizonte:
    // O teste definitivo clássico: se md_mc <= Semi-Arco Diurno, o planeta está ACIMA do horizonte.
    if (md_mc <= sa.sad) {
        *out_semi_arco_usado = sa.sad; // Planeta está acima (Casas 7 a 12)
        return md_mc;                  // MD é a distância até o MC
    } else {
        // Planeta está abaixo (Casas 1 a 6)
        *out_semi_arco_usado = sa.san; // Usa Semi-Arco Noturno
        
        // Calcula a distância até o Fundo do Céu (Meridiano Inferior)
        double ra_ic = ramc + 180.0;
        if (ra_ic >= 360.0) ra_ic -= 360.0;
        
        double md_ic = fabs(ra_planeta - ra_ic);
        if (md_ic > 180.0) {
            md_ic = 360.0 - md_ic;
        }
        return md_ic;
    }
}



double calcular_md_com_sinal_e_definir_semi_arco(double ra_planeta, double ramc, SemiArcos sa, double *out_semi_arco_usado) {
    // 1. Calcula a Distância em relação ao Meio do Céu (Meridiano Superior)
    double md_mc = fabs(ra_planeta - ramc);
    if (md_mc > 180.0) {
        md_mc = 360.0 - md_mc;
    }
    
    // 2. Se a distância até o MC for menor ou igual a 90 graus + a Diferença Ascensional, 
    // ou se geometricamente o planeta estiver acima do horizonte:
    // O teste definitivo clássico: se md_mc <= Semi-Arco Diurno, o planeta está ACIMA do horizonte.
    if (md_mc <= sa.sad) {
        *out_semi_arco_usado = sa.sad; // Planeta está acima (Casas 7 a 12)
        md_mc = ra_planeta - ramc;                  // MD é a distância até o MC
        if (md_mc > 180.0)  md_mc -= 360.0;
        if (md_mc < -180.0) md_mc += 360.0;

        return md_mc;
    } else {
        // Planeta está abaixo (Casas 1 a 6)
        *out_semi_arco_usado = sa.san; // Usa Semi-Arco Noturno
        
        // Calcula a distância até o Fundo do Céu (Meridiano Inferior)
        double ra_ic = ramc + 180.0;
        if (ra_ic >= 360.0) ra_ic -= 360.0;
        
        double md_ic = ra_planeta - ra_ic;
        if (md_ic > 180.0)  md_ic -= 360.0;
        if (md_ic < -180.0) md_ic += 360.0;
        return md_ic;
    }
}



double calcular_md_com_sinal(double ra_planeta, double ramc, SemiArcos sa) {
    // 1. Calcula a Distância em relação ao Meio do Céu (Meridiano Superior)
    double md_mc = fabs(ra_planeta - ramc);
    if (md_mc > 180.0) {
        md_mc = 360.0 - md_mc;
    }
    
    // 2. Se a distância até o MC for menor ou igual a 90 graus + a Diferença Ascensional, 
    // ou se geometricamente o planeta estiver acima do horizonte:
    // O teste definitivo clássico: se md_mc <= Semi-Arco Diurno, o planeta está ACIMA do horizonte.
    if (md_mc <= sa.sad) {
        md_mc = ra_planeta - ramc;                  // MD é a distância até o MC
        if (md_mc > 180.0)  md_mc -= 360.0;
        if (md_mc < -180.0) md_mc += 360.0;

        return md_mc;
    } else {
        // Planeta está abaixo (Casas 1 a 6)
       
        // Calcula a distância até o Fundo do Céu (Meridiano Inferior)
        double ra_ic = ramc + 180.0;
        if (ra_ic >= 360.0) ra_ic -= 360.0;
        
        double md_ic = ra_planeta - ra_ic;
        if (md_ic > 180.0)  md_ic -= 360.0;
        if (md_ic < -180.0) md_ic += 360.0;
        return md_ic;
    }
}

/**
 * Calcula a Distância Horária (HD) proporcional para o método de Placidus
 * @return Valor proporcional entre 0.0 (Meridiano) e 1.0 (Horizonte)
 */
double calcular_distancia_horaria(double md, double semi_arco_usado) {
    if (semi_arco_usado < 1e-6) return 0.0; // Proteção contra divisão por zero em zonas polares
    return md / semi_arco_usado;
}


/**
 * Calcula a Ascensão Obíqua (OA) ou Descensão Obíqua (OD) de um corpo celeste
 * @param ra Ascensão Reta do astro
 * @param dec Declinação do astro
 * @param latitude_geo Latitude geográfica do local (-90.0 a 90.0)
 * @param calcular_od Se for 1 calcula OD (ocaso), se for 0 calcula OA (nascer)
 * @return O ângulo equatorial resultante normalizado entre 0° e 360°
 */
double calcular_ao_do(double ra, double dec, double latitude_geo, int calcular_od) {
    double lat_rad = latitude_geo * DEG_TO_RAD;
    double dec_rad = dec * DEG_TO_RAD;
    
    double argumento = tan(lat_rad) * tan(dec_rad);
    double ad = 0.0;
    
    // Tratamento de segurança para corpos circumpolares
    if (argumento < 1.0 && argumento > -1.0) {
        ad = asin(argumento) * RAD_TO_DEG;
    } else {
        ad = (argumento >= 1.0) ? 90.0 : -90.0;
    }
    
    double resultado;
    if (calcular_od) {
        resultado = ra + ad; // Descensão Obíqua
    } else {
        resultado = ra - ad; // Ascensão Obíqua
    }
    
    // Normalização estrita entre 0 e 360 graus
    resultado = fmod(resultado, 360.0);
    if (resultado < 0.0) resultado += 360.0;
    
    return resultado;
}


// /**
//  * Calcula a Posição Mundana de Placidus (em graus de casa: 0° a 360°)
//  * @param md Distância Meridiana calculada pela função anterior
//  * @param hd Distância Horária (md / semi_arco_usado)
//  * @param ra_planeta Ascensão Reta do planeta
//  * @param ramc Ascensão Reta do Meio do Céu
//  * @param sa_diurno Semi-Arco Diurno do planeta
//  * @return Coordenada Mundana exata mapeada no espaço das casas (0° a 360°)
//  */
// double calcular_posicao_mundana_placidus(double md, double hd, double ra_planeta, double ramc, double sa_diurno) {
//     double pos_mundana = 0.0;
    
//     // Determina o quadrante exato usando a diferença circular bruta para saber se o planeta
//     // está no hemisfério oriental (casas 10,11,12,1,2,3) ou ocidental (casas 4,5,6,7,8,9)
//     double diff_ramc = ra_planeta - ramc;
//     while (diff_ramc < -180.0) diff_ramc += 360.0;
//     while (diff_ramc > 180.0)  diff_ramc -= 360.0;
//     int e_oriental = (diff_ramc < 0.0) ? 1 : 0;

//     // Verifica se o planeta está acima ou abaixo com base na Distância Meridiana e Semi-Arco Diurno
//     int esta_acima = (md <= sa_diurno) ? 1 : 0;

//     if (esta_acima) {
//         if (e_oriental) {
//             // Quadrante 4: Do Meio do Céu (270°) ao Ascendente (360°) -> Casas 10, 11, 12
//             pos_mundana = 270.0 + (hd * 90.0);
//         } else {
//             // Quadrante 3: Do Descendente (180°) ao Meio do Céu (270°) -> Casas 7, 8, 9
//             pos_mundana = 270.0 - (hd * 90.0);
//         }
//     } else {
//         if (e_oriental) {
//             // Quadrante 1: Do Ascendente (0°) ao Fundo do Céu (90°) -> Casas 1, 2, 3
//             pos_mundana = 0.0 + (hd * 90.0);
//         } else {
//             // Quadrante 2: Do Fundo do Céu (90°) ao Descendente (180°) -> Casas 4, 5, 6
//             pos_mundana = 180.0 - (hd * 90.0);
//         }
//     }

//     // Normalização de segurança
//     pos_mundana = fmod(pos_mundana, 360.0);
//     if (pos_mundana < 0.0) pos_mundana += 360.0;

//     return pos_mundana;
// }








// Retorna o ID da Swiss Ephemeris baseado no nome do objeto do seu software
// Retorna -1 para pontos que não possuem nós orbitais (Ângulos, Fortuna, etc.)
int obter_swiss_ephemeris_id(const char *object) {
    if (strcmp(object, "☉") == 0) {
        return SE_SUN;
    }
    if (strcmp(object, "☾") == 0) {
        return SE_MOON;
    }
    if (strcmp(object, "☿") == 0) {
        return SE_MERCURY;
    }
    if (strcmp(object, "♀") == 0) {
        return SE_VENUS;
    }
    if (strcmp(object, "♂") == 0) {
        return SE_MARS;
    }
    if (strcmp(object, "♃") == 0) {
        return SE_JUPITER;
    }
    if (strcmp(object, "♄") == 0) {
        return SE_SATURN;
    }
    if (strcmp(object, "♅") == 0) {
        return SE_URANUS;
    }
    if (strcmp(object, "♆") == 0) {
        return SE_NEPTUNE;
    }
    if (strcmp(object, "⯓") == 0) {
        return SE_PLUTO;
    }
    if (strcmp(object, "☊") == 0) {
        return SE_TRUE_NODE; 
    }
    if (strcmp(object, "☋") == 0) {
        return SE_TRUE_NODE; // O Nó Sul usa os mesmos parâmetros orbitais do Nó Norte (inversão tratada na fórmula)
    }

    // Retorna -1 para Asc, Desc, MC, IC, Vertex, Fortuna, Termos, etc.
    return -1; 
}


int obter_swiss_ephemeris_id_por_nome(const char *object) {
    if (strcmp(object, _("Sol")) == 0) {
        return SE_SUN;
    }
    if (strcmp(object, _("Luna")) == 0) {
        return SE_MOON;
    }
    if (strcmp(object, _("Mercury")) == 0) {
        return SE_MERCURY;
    }
    if (strcmp(object, _("Venus")) == 0) {
        return SE_VENUS;
    }
    if (strcmp(object, _("Mars")) == 0) {
        return SE_MARS;
    }
    if (strcmp(object, _("Jupiter")) == 0) {
        return SE_JUPITER;
    }
    if (strcmp(object, _("Saturn")) == 0) {
        return SE_SATURN;
    }
    if (strcmp(object, _("Uranus")) == 0) {
        return SE_URANUS;
    }
    if (strcmp(object, _("Neptune")) == 0) {
        return SE_NEPTUNE;
    }
    if (strcmp(object, _("Pluto")) == 0) {
        return SE_PLUTO;
    }
    if (strcmp(object, _("North Node")) == 0) {
        return SE_TRUE_NODE; 
    }
    if (strcmp(object, _("South Node")) == 0) {
        return SE_TRUE_NODE; // O Nó Sul usa os mesmos parâmetros orbitais do Nó Norte (inversão tratada na fórmula)
    }

    // Retorna -1 para Asc, Desc, MC, IC, Vertex, Fortuna, Termos, etc.
    return -1; 
}







// Função auxiliar para calcular a latitude dinâmica de qualquer planeta em uma longitude alvo
double calcular_latitude_dinamica_bianchini(double jd, const char *object_name, double lon_aspecto, int object_type) {
    if (object_type == PROM_ANGLE ||
        object_type == PROM_CUSP ||
        object_type == PROM_PART ||
        object_type == PROM_POINT ||
        object_type == PROM_TERM)
    {
        return 0.0;
    }

    int se_id = obter_swiss_ephemeris_id_por_nome(object_name);
    
    // Se for Sol, ponto abstrato, ângulo ou termo sem planeta físico (-1), latitude é 0.0
    if (se_id == -1 || se_id == SE_SUN) {
        return 0.0;
    }

    // De acordo com a documentação da libswe, xnasc e xndsc precisam ser arrays de double com tamanho 6.
    // xaphel e xperi também recebem os dados de apogeu/perigeu e precisam ter tamanho 6.
    double xnasc[6]; 
    double xndsc[6];
    double xaphel[6];
    double xperi[6];
    char serr[256];
    
    // O sinalizador de flags e o método precisam ser int32
    int32 flags_nodos = SEFLG_TOPOCTR;
    int32 metodo_nodo = SE_NODBIT_MEAN; // SE_NODBIT_MEAN = 1 (Nodos Médios, padrão astrológico)

    // Chamada oficial da Swiss Ephemeris com os 9 argumentos corretos e os tipos alinhados
    if (swe_nod_aps_ut(jd, se_id, flags_nodos, metodo_nodo, xnasc, xndsc, xperi, xaphel, serr) < 0) {
        return 0.0; // Fallback caso ocorra algum erro interno na biblioteca
    }

    // Índices oficiais da Swiss Ephemeris para os arrays de nodos/apsides:
    // [0] = Longitude
    // [1] = Latitude
    // [4] = Inclinação orbital em relação à eclíptica
    double lon_nodo = xnasc[0];     // Longitude do Nó Ascendente (Ω)
    double inclinacao = xnasc[4];   // Inclinação Orbital (i)

    // CORREÇÃO PARA O NÓ SUL: 
    if (strcmp(object_name, _("South Node")) == 0) {
        lon_nodo = fmod(lon_nodo + 180.0, 360.0);
    }

    // Diferença angular entre a longitude do aspecto e o Nó Ascendente corrigido do planeta
    double dist_nodo_rad = para_radianos(lon_aspecto - lon_nodo);
    double inc_rad = para_radianos(inclinacao);

    // Fórmula de Bianchini: sin(lat) = sin(inc) * sin(λ_aspecto - Ω)
    double sin_lat_correto = sin(inc_rad) * sin(dist_nodo_rad);
    double lat_dinamica = para_graus(asin(sin_lat_correto));

    // Se for o Nó Sul físico do planeta, a latitude é invertida em relação ao plano norte
    if (strcmp(object_name, _("South Node")) == 0) {
        lat_dinamica = -lat_dinamica;
    }

    if (object_type == PROM_CONTRANTISCIUM) {
        lat_dinamica = -lat_dinamica;
    }

    return lat_dinamica;
}






double descobrir_idade_por_arco_solar(double tjd_ut_natal, double arco_alvo) {
    double x2[6];
    char serr[256];
    double sol_natal, sol_alvo_progredido;
    int32 iflag = SEFLG_SPEED;

    // 1. Descobre a posição do Sol Natal
    swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x2, serr);
    sol_natal = x2[0];

    // 2. Define o alvo que o Sol precisa atingir no céu pós-natal
    sol_alvo_progredido = normalize360(sol_natal + arco_alvo);

    // 3. Método Numérico (Bisseção) para achar em qual dia isso acontece
    // Assumimos um limite de idade humana (ex: 0 a 120 anos = 0 a 120 dias pós-natal)
    double limite_inferior_dias = 0.0;
    double limite_superior_dias = 120.0; 
    double dias_estimados = 0.0;
    
    for (int i = 0; i < 50; i++) { // 50 iterações garantem precisão absurda
        dias_estimados = (limite_inferior_dias + limite_superior_dias) / 2.0;
        
        swe_calc_ut(tjd_ut_natal + dias_estimados, SE_SUN, iflag, x2, serr);
        double sol_estimado = x2[0];
        
        // Ajusta a distância angular considerando a virada do zodíaco
        double diferenca = normalize360(sol_estimado - sol_alvo_progredido);
        if (diferenca > 180.0) diferenca -= 360.0;

        if (fabs(diferenca) < 1e-8) break; // Convergiu com precisão máxima

        if (diferenca > 0) {
            limite_superior_dias = dias_estimados;
        } else {
            limite_inferior_dias = dias_estimados;
        }
    }

    // Como 1 dia pós-natal = 1 ano de idade:
    double idade_anos = dias_estimados; 
    return idade_anos;
}



double calcular_arco_kepler_para_idade(double tjd_ut_natal, double idade_anos) {
    double x2[6];
    char serr[256];
    int32 iflag = SEFLG_SPEED;
    
    // Calcula a velocidade do Sol no ano/data do trânsito alvo
    double tjd_ut_atual = tjd_ut_natal + (idade_anos * obter_ano_tropical_dinamico(tjd_ut_natal));
    swe_calc_ut(tjd_ut_atual, SE_SUN, iflag, x2, serr);
    
    double velocidade_do_sol = x2[3]; // Índice 3 contém a velocidade em graus/dia
    
    // O arco acumulado sob a lógica de Kepler para essa idade específica
    return velocidade_do_sol * idade_anos;
}

double descobrir_idade_por_arco_kepler(double tjd_ut_natal, double arco_alvo) {
    double limite_inferior_anos = 0.0;
    double limite_superior_anos = 120.0; // limite de idade
    double idade_estimada = 0.0;

    for (int i = 0; i < 50; i++) {
        idade_estimada = (limite_inferior_anos + limite_superior_anos) / 2.0;
        
        double arco_estimado = calcular_arco_kepler_para_idade(tjd_ut_natal, idade_estimada);
        
        if (fabs(arco_estimado - arco_alvo) < 1e-8) break;

        if (arco_estimado > arco_alvo) {
            limite_superior_anos = idade_estimada;
        } else {
            limite_inferior_anos = idade_estimada;
        }
    }

    return idade_estimada;
}




/**
 * Retorna o valor CHAVE dinâmico para o Arco Solar Verdadeiro.
 * Uso no seu código: double chave = obter_chave_arco_solar(tjd_natal, arco);
 *                    double idade = arco / chave;
 */
double obter_chave_arco_solar(double tjd_ut_natal, double arco_alvo) {
    double x2[6];
    char serr[256];
    double sol_natal, sol_alvo_progredido;
    int32 iflag = SEFLG_SPEED;

    // 1. Descobre a posição do Sol Natal
    swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x2, serr);
    sol_natal = x2[0];

    // 2. Define a longitude alvo que o Sol precisa atingir
    sol_alvo_progredido = normalize360(sol_natal + arco_alvo);

    // 3. Busca interna da idade (em dias ephemeris) que gera este arco
    double limite_inferior = 0.0;
    double limite_superior = 120.0; // Limite de 120 anos/dias
    double dias_estimados = 0.0;
    
    for (int i = 0; i < 50; i++) {
        dias_estimados = (limite_inferior + limite_superior) / 2.0;
        
        swe_calc_ut(tjd_ut_natal + dias_estimados, SE_SUN, iflag, x2, serr);
        double sol_estimado = x2[0];
        
        double diferenca = normalize360(sol_estimado - sol_alvo_progredido);
        if (diferenca > 180.0) diferenca -= 360.0;

        if (fabs(diferenca) < 1e-8) break;

        if (diferenca > 0) limite_superior = dias_estimados;
        else limite_inferior = dias_estimados;
    }

    // Evita divisão por zero caso o arco seja nulo
    if (dias_estimados < 1e-6) {
        // Retorna a velocidade do Sol no exato instante do nascimento como chave inicial
        swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x2, serr);
        return x2[3]; // Índice 3 é a velocidade diária em graus/dia
    }

    // 4. Retorna a Chave Equivalente: Arco dividido pela Idade (dias_estimados)
    // Como Idade = Arco / Chave, logo Chave = Arco / Idade
    return arco_alvo / dias_estimados;
}




/**
 * Retorna a Chave do Arco Solar Verdadeiro IMEDIATAMENTE (Sem Loops / Performance Ultra Rápida)
 * Erro máximo aproximado: menor que 0.001 dias (alguns minutos de tempo real).
 */
double obter_chave_arco_solar_ultra_fast(double tjd_ut_natal, double arco_alvo) {
    double x2[6];
    char serr[256];
    int32 iflag = SEFLG_SPEED;

    // 1. Faz APENAS UMA chamada para pegar a posição e velocidade do Sol Natal
    if (swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x2, serr) < 0) {
        return NAIBOD_KEY; // Fallback para Naibod caso falhe
    }
    
    double sol_natal = x2[0];
    double velocidade_natal = x2[3]; // Velocidade diária real do Sol no nascimento

    // 2. Modelo Kepleriano de movimento médio do Sol
    // O Sol corre mais rápido no Periélio (3 de Janeiro) ~ 283° de longitude tropical
    double longitude_perielio = 283.0; 
    double anomalia_media_natal = (sol_natal - longitude_perielio) * (PI / 180.0);

    // Amplitude da variação da velocidade do Sol devido à excentricidade da órbita da Terra
    // Velocidade Max ≈ 1.019°/dia, Min ≈ 0.953°/dia. Amplitude da oscilação ≈ 0.033
    double amplitude_oscilacao = 0.0334; 

    // 3. Estimativa analítica direta da idade baseada na geometria orbital (Equação de Kepler invertida)
    // Em vez de chutar 50 vezes, calculamos diretamente onde o Sol estará baseado na velocidade atual
    double idade_estimada_dias = arco_alvo / velocidade_natal;
    
    // Ajuste de perturbação de primeira ordem (corrige a aceleração/desaceleração do Sol no período)
    double ajuste_kepler = (amplitude_oscilacao / 2.0) * sin(anomalia_media_natal) * (arco_alvo / 0.9856);
    idade_estimada_dias += ajuste_kepler;

    // Evita divisão por zero para arcos nulos
    if (idade_estimada_dias < 1e-6) {
        return velocidade_natal;
    }

    // 4. Retorna a chave equivalente para manter seu código funcionando por divisão
    return arco_alvo / idade_estimada_dias;
}

double obter_chave_arco_solar_ultra_rapida(double tjd_ut_natal, double arco_alvo) {
    double x2[6];
    char serr[256];
    int32 iflag = SEFLG_SPEED;

    // 1. Posição natal do Sol
    swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x2, serr);
    double sol_natal = x2[0];
    double sol_alvo = sol_natal + arco_alvo;
    if (sol_alvo >= 360.0) sol_alvo -= 360.0;

    // 2. Estimativa inicial rápida (Usa Naibod como ponto de partida)
    double dias_estimados = arco_alvo / 0.98564733;

    // 3. Método de Newton-Raphson (Apenas 3 passos são necessários!)
    for (int i = 0; i < 3; i++) {
        swe_calc_ut(tjd_ut_natal + dias_estimados, SE_SUN, iflag, x2, serr);
        double sol_estimado = x2[0];
        double velocidade_sol = x2[3]; // Velocidade em graus/dia

        double erro = sol_estimado - sol_alvo;
        if (erro > 180.0)  erro -= 360.0;
        if (erro < -180.0) erro += 360.0;

        // Ajusta a estimativa usando a derivada exata
        dias_estimados -= erro / velocidade_sol;
    }

    if (dias_estimados < 1e-6) {
        swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x2, serr);
        return x2[3];
    }

    // CORREÇÃO DE ESCALA: Neutraliza a multiplicação por 365.242199 que o motor pai fará,
    // garantindo que os dias_decorridos finais batam exatamente com dias_estimados!
    double idade_anos_reais = dias_estimados; 
    return arco_alvo / idade_anos_reais;
}

// 1. FUNÇÃO INTERNA CORRIGIDA (Calcula o arco de RA real para um JD Progredido específico)
double calcular_arco_solar_real_ra_FAST(double ra_sol_natal, double jd_progredido, int *err_code) {
    double xx_prog[6], xequat_prog[3], xx_eq_prog[3];
    char err_msg[256];
    
    // Calcula a posição do Sol no JD progredido real
    if (swe_calc_ut(jd_progredido, SE_SUN, 0, xx_prog, err_msg) < 0) {
        *err_code = -2;
        return 0.0;
    }
    
    // CORREÇÃO: Obter a obliquidade dinâmica correta para a data progredida
    double obl_dinamica = get_obliquidade(jd_progredido); 
    
    xx_eq_prog[0] = xx_prog[0];
    xx_eq_prog[1] = xx_prog[1];
    xx_eq_prog[2] = 1.0;
    
    swe_cotrans(xx_eq_prog, xequat_prog, -obl_dinamica); 
    
    double arco_solar_ra = xequat_prog[0] - ra_sol_natal;
    if (arco_solar_ra < 0.0)  arco_solar_ra += 360.0;
    if (arco_solar_ra > 360.0) arco_solar_ra = fmod(arco_solar_ra, 360.0);
    
    return arco_solar_ra;
}

// 2. FUNÇÃO DE BUSCA ATUALIZADA (Retorna o fator de conversão perfeito)
double encontrar_chave_por_arco_solar_ra_FAST(double jd_natal, double arco_direcao) {
    double xx_natal[6], xequat_natal[3], xx_eq_natal[3];
    char err_msg[256];
    int err_code;
    
    // Calcula Sol Natal
    if (swe_calc_ut(jd_natal, SE_SUN, 0, xx_natal, err_msg) < 0) {
        return NAIBOD_KEY;
    }
    
    double obl_natal = get_obliquidade(jd_natal);
    xx_eq_natal[0] = xx_natal[0];
    xx_eq_natal[1] = xx_natal[1];
    xx_eq_natal[2] = 1.0;
    
    swe_cotrans(xx_eq_natal, xequat_natal, -obl_natal);
    double ra_sol_natal = xequat_natal[0];

    // BUSCA DE NEWTON-RAPHSON BASEADA EM DIAS JULIANOS REAIS
    // Chave inicial estimada baseada em Naibod: 1 ano de vida = 1 dia de progressão solar
    double idade_estimada_anos = arco_direcao / 0.98564733; 
    double jd_progredido_estimado = jd_natal + idade_estimada_anos; 
    
    double erro = 1.0;
    int iteracoes = 0;
    const double velocidade_media_sol_por_dia = 0.98564733; // Velocidade em graus por dia de progressão

    while (fabs(erro) > 0.000001 && iteracoes < 10) {
        // Passa o JD progredido real estimado para o cálculo astronômico
        double arco_solar_calculado = calcular_arco_solar_real_ra_FAST(ra_sol_natal, jd_progredido_estimado, &err_code);
        
        erro = arco_solar_calculado - arco_direcao;
        
        // O ajuste corrige DIRETAMENTE os dias do Dia Juliano de progressão [1]
        jd_progredido_estimado -= (erro / velocidade_media_sol_por_dia); 
        iteracoes++;
    }
    
    // Descobre quantos dias de progressão real se passaram
    double dias_de_progressao = jd_progredido_estimado - jd_natal;
    
    if (dias_de_progressao > 0.0) {
        // CORREÇÃO DA FRAÇÃO DA CHAVE:
        // Como o seu motor principal faz: d->idade_evento = arco / CHAVE;
        // E depois faz: dias_decorridos = d->idade_evento * 365.242199;
        // Nós devolvemos a CHAVE perfeitamente calibrada para neutralizar o ano tropical do seu motor pai!
        double idade_anos_reais = dias_de_progressao; 
        return arco_direcao / idade_anos_reais;
    }
    
    return NAIBOD_KEY;
}



// Calcula o Arco Solar Real em Ascensão Reta para uma idade específica
double calcular_arco_solar_real_ra(double jd_natal, double idade_anos, int *err_code) {
    double xx_natal[6], xx_prog[6];
    char err_msg[256];
    
    // 1. Calcula a posição do Sol no momento exato do nascimento (Eclíptica)
    if (swe_calc_ut(jd_natal, SE_SUN, 0, xx_natal, err_msg) < 0) {
        *err_code = -1;
        return 0.0;
    }
    double lon_sol_natal = xx_natal[0];
    double lat_sol_natal = xx_natal[1];
    
    // 2. Transforma as coordenadas natas do Sol para o plano Equatorial para pegar a AR Natal
    double xx_eq_natal[3] = {lon_sol_natal, lat_sol_natal, 1.0};
    double xequat_natal[3];
    // Pegamos a obliquidade real do momento do nascimento para manter a precisão mecânica
    double obl_natal = 23.4392911; // Ideal obter dinamicamente com swe_calc_ut para o ponto epsilon se quiser
    swe_cotrans(xx_eq_natal, xequat_natal, -obl_natal);
    double ra_sol_natal = xequat_natal[0];
    
    // 3. Aplica o princípio de 1 dia = 1 ano para achar a data progredida
    double jd_progredido = jd_natal + idade_anos;
    
    // 4. Calcula a posição do Sol na data progredida
    if (swe_calc_ut(jd_progredido, SE_SUN, 0, xx_prog, err_msg) < 0) {
        *err_code = -2;
        return 0.0;
    }
    double lon_sol_prog = xx_prog[0];
    double lat_sol_prog = xx_prog[1];
    
    // 5. Transforma a posição progredida para o plano Equatorial
    double xx_eq_prog[3] = {lon_sol_prog, lat_sol_prog, 1.0};
    double xequat_prog[3];
    swe_cotrans(xx_eq_prog, xequat_prog, -obl_natal); // Mantém a obliquidade radix para projeção tradicional
    double ra_sol_prog = xequat_prog[0];
    
    // 6. O ARCO SOLAR EM ASCENSÃO RETA é a diferença direta das RAs
    double arco_solar_ra = ra_sol_prog - ra_sol_natal;
    
    // Normalização estrita do círculo
    if (arco_solar_ra < 0.0) arco_solar_ra += 360.0;
    
    return arco_solar_ra;
}

double encontrar_chave_por_arco_solar_ra(double jd_natal, double arco_direcao) {
    double idade_estimada = arco_direcao; // Palpite inicial (1° = 1 ano)
    double erro = 1.0;
    int iteracoes = 0;
    int err_code;
    
    while (fabs(erro) > 0.00001 && iteracoes < 20) {
        // Chama a função astronômica que criamos anteriormente
        double arco_solar_calculado = calcular_arco_solar_real_ra(jd_natal, idade_estimada, &err_code);
        
        erro = arco_solar_calculado - arco_direcao;
        idade_estimada -= erro; // Ajusta o palpite
        iteracoes++;
    }
    
    // Para encaixar no cálculo padrão atual: d->idade_evento = arco / CHAVE;
    // Retornamos um "fator equivalente" para que a divisão dê a idade_estimada correta.
    if (idade_estimada > 0.0) {
        return arco_direcao / idade_estimada;
    }
    return NAIBOD_KEY;
}



double encontrar_chave_por_arco_solar_ra_OTIMIZADA(double jd_natal, double arco_direcao) {
    double x_natal[6], x_atual[6];
    char err_msg[256];
    
    // Força a Swiss Ephemeris a calcular diretamente em coordenadas equatoriais
    // O índice [0] vira RA e o índice [3] vira a velocidade exata em RA/dia
    int32 iflag = SEFLG_SPEED | SEFLG_EQUATORIAL;
    
    // 1. Calcula Sol Natal em RA diretamente
    if (swe_calc_ut(jd_natal, SE_SUN, iflag, x_natal, err_msg) < 0) {
        return NAIBOD_KEY;
    }
    double ra_sol_natal = x_natal[0];
    double vel_ra_natal = x_natal[3]; 

    // 2. Estimativa inicial (1 ano = 1 dia)
    double idade_estimada_anos = arco_direcao / vel_ra_natal; 
    double jd_progredido_estimado = jd_natal + idade_estimada_anos; 
    
    double erro = 1.0;
    int iteracoes = 0;

    // Busca pura de Newton-Raphson com derivada dinâmica
    while (fabs(erro) > 0.000001 && iteracoes < 8) {
        if (swe_calc_ut(jd_progredido_estimado, SE_SUN, iflag, x_atual, err_msg) < 0) {
            break;
        }
        
        double ra_atual = x_atual[0];
        double vel_ra_atual = x_atual[3]; // f'(x) exata e dinâmica do dia progredido

        // Trata a diferença angular minimizando saltos de 360 graus
        double arco_solar_calculado = ra_atual - ra_sol_natal;
        while (arco_solar_calculado < -180.0) arco_solar_calculado += 360.0;
        while (arco_solar_calculado > 180.0)  arco_solar_calculado -= 360.0;
        
        erro = arco_solar_calculado - arco_direcao;
        
        // Newton-Raphson verdadeiro: usa a derivada exata do ponto atual
        jd_progredido_estimado -= (erro / vel_ra_atual); 
        iteracoes++;
    }
    
    double dias_de_progressao = jd_progredido_estimado - jd_natal;
    
    if (dias_de_progressao > 0.0) {
        // Devolve o fator calibrado que mata o ano tropical do seu motor principal
        return arco_direcao / dias_de_progressao;
    }
    
    return NAIBOD_KEY;
}






// // Função auxiliar interna para simular o arco gerado pela velocidade do Sol
// static double calcular_arco_kepler_interno(double tjd_ut_natal, double idade_anos) {
//     double x2[6];
//     char serr[256];
//     int32 iflag = SEFLG_SPEED;
    
//     double tjd_ut_atual = tjd_ut_natal + (idade_anos * obter_ano_tropical_dinamico(tjd_ut_natal));
//     swe_calc_ut(tjd_ut_atual, SE_SUN, iflag, x2, serr);
    
//     return x2[3] * idade_anos; // Velocidade instantânea do dia * anos acumulados
// }

// /**
//  * Retorna o valor CHAVE dinâmico para a lógica de Kepler.
//  * Uso no seu código: double chave = obter_chave_kepler(tjd_natal, arco);
//  *                    double idade = arco / chave;
//  */
// double obter_chave_kepler(double tjd_ut_natal, double arco_alvo) {
//     double limite_inferior = 0.0;
//     double limite_superior = 120.0;
//     double idade_estimada = 0.0;

//     // Busca interna da idade correspondente ao arco sob as leis de Kepler
//     for (int i = 0; i < 50; i++) {
//         idade_estimada = (limite_inferior + limite_superior) / 2.0;
        
//         double arco_estimado = calcular_arco_kepler_interno(tjd_ut_natal, idade_estimada);
        
//         if (fabs(arco_estimado - arco_alvo) < 1e-8) break;

//         if (arco_estimado > arco_alvo) limite_superior = idade_estimada;
//         else limite_inferior = idade_estimada;
//     }

//     // Evita divisão por zero para arcos nulos
//     if (idade_estimada < 1e-6) {
//         double x2[6];
//         char serr[256];
//         swe_calc_ut(tjd_ut_natal, SE_SUN, SEFLG_SPEED, x2, serr);
//         return x2[3];
//     }

//     // Retorna a Chave Equivalente para fechar com a sua equação matemática
//     return arco_alvo / idade_estimada;
// }



// double obter_chave_kepler_ultra_rapida(double tjd_ut_natal, double arco_alvo) {
//     double x2[6];
//     char serr[256];
//     int32 iflag = SEFLG_SPEED;

//     // 1. Estimativa inicial rápida em dias (1 dia = 1 ano)
//     double dias_estimados = arco_alvo / 0.98564733;

//     // 2. Método de Newton-Raphson
//     for (int i = 0; i < 3; i++) {
//         // CORREÇÃO: Varre as efemérides no passo secundário correto (dias desde o natal)
//         double tjd_ut_atual = tjd_ut_natal + dias_estimados;
        
//         if (swe_calc_ut(tjd_ut_atual, SE_SUN, iflag, x2, serr) < 0) {
//             break; 
//         }

//         double velocidade_sol = x2[3]; 
        
//         // Na lógica de Kepler: Arco = Velocidade do Dia * Tempo decorrido (em anos)
//         // Como mapeamos 1 dia = 1 ano para o trânsito, o tempo decorrido é dias_estimados
//         double arco_estimado = velocidade_sol * dias_estimados;
//         double erro = arco_estimado - arco_alvo;

//         dias_estimados -= erro / velocidade_sol;
//     }

//     if (dias_estimados < 1e-6) {
//         swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x2, serr);
//         return x2[3]; 
//     }

//     // Retorna a chave calibrada para fechar perfeitamente com a equação do motor pai
//     double idade_anos_reais = dias_estimados;
//     return arco_alvo / idade_anos_reais;
// }




// Função auxiliar para tratar a passagem cíclica pelo ponto zero da Ascensão Reta (360° -> 0°)
static double normalizar_diferenca_ra(double atual, double natal) {
    double diff = atual - natal;
    while (diff < -180.0) diff += 360.0;
    while (diff > 180.0)  diff -= 360.0;
    return diff;
}

/**
 * Calcula a Chave Dinâmica de Placidus (True Solar Arc em Ascensão Reta)
 * @param tjd_ut_natal Dia Juliano do nascimento (Tempo Universal)
 * @param arco_alvo O arco equatorial calculado pelo seu motor de direções primárias
 * @return O fator de conversão (Chave) em Graus por Ano
 */
double obter_chave_placidus_ra(double tjd_ut_natal, double arco_alvo) {
    double x_natal[6], x_atual[6];
    char serr[256];
    
    // ATENÇÃO: Adicionamos SEFLG_EQUATORIAL para que o índice [0] seja RA e [3] seja a velocidade em RA
    int32 iflag = SEFLG_SPEED | SEFLG_EQUATORIAL;

    // 1. Obter a Ascensão Reta (RA) e a velocidade em RA do Sol Natal
    if (swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x_natal, serr) < 0) {
        return 0.98564733; // Fallback caso a efeméride falhe
    }
    double ra_natal = x_natal[0];
    double vel_ra_natal = x_natal[3]; // Velocidade de variação da RA por dia

    // 2. Estimativa inicial do tempo decorrido em dias (1 dia = 1 ano)
    double dias_estimados = arco_alvo / vel_ra_natal;

    // 3. Método de Newton-Raphson para convergência rápida
    for (int i = 0; i < 4; i++) {
        double tjd_ut_atual = tjd_ut_natal + dias_estimados;
        
        if (swe_calc_ut(tjd_ut_atual, SE_SUN, iflag, x_atual, serr) < 0) {
            break; 
        }

        double ra_atual = x_atual[0];
        double vel_ra_atual = x_atual[3]; // Derivada instantânea da RA

        // O arco estimado em RA é a diferença real de posição equatorial
        double arco_estimado = normalizar_diferenca_ra(ra_atual, ra_natal);
        
        double erro = arco_estimado - arco_alvo;

        // Ajuste numérico usando a velocidade atualizada em RA
        dias_estimados -= erro / vel_ra_atual;
    }

    // Proteção para arcos nulos ou negativos
    if (dias_estimados < 1e-6) {
        return vel_ra_natal; 
    }

    // Retorna a chave de Placidus calibrada (Graus por Ano)
    return arco_alvo / dias_estimados;
}








// Função auxiliar para normalizar diferenças de ângulos (evita problemas em 0° de Áries)
double normalizar_diferenca_graus(double atual, double natal) {
    double diff = atual - natal;
    while (diff < -180.0) diff += 360.0;
    while (diff > 180.0)  diff -= 360.0;
    return diff;
}

double obter_chave_kepler(double tjd_ut_natal, double arco_alvo) {
    double x_natal[6], x_atual[6];
    char serr[256];
    int32 iflag = SEFLG_SPEED;

    // 1. Obter a posição (Longitude) e velocidade iniciais do Sol Natal
    if (swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x_natal, serr) < 0) {
        return 0.98564733; // Fallback caso a efeméride falhe
    }
    double lon_natal = x_natal[0];
    double vel_natal = x_natal[3];

    // 2. Estimativa inicial do tempo (1 dia na efeméride = 1 ano de vida)
    double dias_estimados = arco_alvo / vel_natal;

    // 3. Método de Newton-Raphson (3 a 4 iterações bastam para precisão cirúrgica)
    for (int i = 0; i < 4; i++) {
        double tjd_ut_atual = tjd_ut_natal + dias_estimados;
        
        if (swe_calc_ut(tjd_ut_atual, SE_SUN, iflag, x_atual, serr) < 0) {
            break; 
        }

        double lon_atual = x_atual[0];
        double vel_atual = x_atual[3]; // A velocidade atual funciona como a derivada exata da posição

        // CORREÇÃO: O arco estimado é a diferença real de posição na eclíptica
        double arco_estimado = normalizar_diferenca_graus(lon_atual, lon_natal);
        
        double erro = arco_estimado - arco_alvo;

        // Newton-Raphson: x_novo = x_atual - f(x)/f'(x)
        // Onde f(x) é o erro e f'(x) é a velocidade instantânea do Sol
        dias_estimados -= erro / vel_atual;
    }

    // Se o arco for zero ou negativo (borda extrema de segurança)
    if (dias_estimados < 1e-6) {
        return vel_natal; 
    }

    // Retorna a chave dinâmica ajustada (Graus por Ano)
    // Multiplicando qualquer arco do seu motor por esta chave, você terá a idade exata em anos
    return arco_alvo / dias_estimados;
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




// // Busca numérica robusta baseada no objeto do promissor e sua curva de Bianchini
// double encontrar_longitude_promissor_por_dec(double dec_alvo, double jd, char* obj_prom, double limite_inf, double limite_sup) {
//     double lon_min = limite_inf;
//     double lon_max = limite_sup;
//     double lon_mid = (lon_min + lon_max) / 2.0;
//     double obl = -get_obliquidade(jd);
//     int max_iter = 40;

//     for (int i = 0; i < max_iter; i++) {
//         lon_mid = (lon_min + lon_max) / 2.0;
//         double lat = calcular_latitude_dinamica_bianchini(jd, obj_prom, lon_mid, );
        
//         double xx[3] = {lon_mid, lat, 1.0};
//         double xequat[3];
//         swe_cotrans(xx, xequat, obl);
        
//         double dec_calc = xequat[1]; // Declinação calculada no modelo equatorial

//         if (fabs(dec_calc - dec_alvo) < 0.00001) break;

//         // Como a curva com latitude varia, testamos a derivada local para ajustar os limites
//         double lat_ajuste = calcular_latitude_dinamica_bianchini(jd, obj_prom, lon_mid + 0.01);
//         double xx_ajuste[3] = {lon_mid + 0.01, lat_ajuste, 1.0};
//         double xequat_ajuste[3];
//         swe_cotrans(xx_ajuste, xequat_ajuste, obl);
        
//         int ascendente = (xequat_ajuste[1] > dec_calc);

//         if ((dec_calc < dec_alvo && ascendente) || (dec_calc > dec_alvo && !ascendente)) {
//             lon_min = lon_mid;
//         } else {
//             lon_max = lon_mid;
//         }
//     }
//     return lon_mid;
// }



int calcular_direcoes_zodiacais_topocentrico(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, 
                                            double jd, int sentido, Promissor *prom, int total_promissores,
                                            double ramc, double lat_geografica, int *qtd_direcoes, bool is_part) {
    
    int object_diff = show_modern_planets ? 0 : 3;
    double angulos_aspectos[] = {0.0, 60.0, -60.0, 90.0, -90.0, 120.0, -120.0, 180.0, 999.9, 999.9};
    char *simbolos_aspectos[] = {"☌", "⚹", "⚹", "□", "□", "△", "△", "☍", "∥", "∦"};
    int total_aspects = 10;

    // 1. Extrair os dados equatoriais e a Posição Horária (PH) do Significador Fixo
    double ra_sig = sig[idx_alvo].ra;
    double dec_sig = sig[idx_alvo].declination;
    double dec_sig_rad = dec_sig * M_PI / 180.0;
    double lat_geo_rad = lat_geografica * M_PI / 180.0;
    
    double ad_sig_geo = asin(tan(dec_sig_rad) * tan(lat_geo_rad));
    double sad_sig = 90.0 + (ad_sig_geo * 180.0 / M_PI);
    
    double diff_sig = ra_sig - ramc;
    while (diff_sig > 180.0)  diff_sig -= 360.0;
    while (diff_sig < -180.0) diff_sig += 360.0;
    double dm_sig = fabs(diff_sig);
    
    // PH do Significador
    double ph_sig = (dm_sig <= sad_sig) ? (dm_sig / sad_sig) : ((180.0 - dm_sig) / (180.0 - sad_sig));
    
    // 2. Fixar o Polo Topocêntrico do Significador
    double tan_polo_sig = tan(lat_geo_rad) * ph_sig;

    
    bool sig_is_fortune = strcmp(sig[idx_alvo].object, "FOR") == 0;


    for (int p = 0; p < total_promissores; p++) {
        if ((prom[p].type == PROM_ANTISCIUM || prom[p].type == PROM_CONTRANTISCIUM) && !ANT_PROM) continue;
        else if (prom[p].type == PROM_POINT || prom[p].type == PROM_ANGLE) continue;
        else if (!is_part) {
            if ((p == idx_alvo && p < NUM_OBJECTS - object_diff - (show_modern_planets ? 5 : 4))) continue;
        }
        else {
            if (sig_is_fortune && (p == P_FORTUNA - object_diff)) continue;
        }
        
        for (int s = 0; s < 2; s++) {
            if (s == 0 && sentido == 1) continue;
            if (s == 1 && sentido == 0) continue;
                        
            for (int a = 0; a < total_aspects; a++) {
                if (prom[p].type == PROM_TERM && a > 0) break;

                double arco = 0.0;
                int eh_paralelo = (a == 8 || a == 9);

                if (eh_paralelo) {
                    
                    double dec_alvo_paralelo = (a == 8) ? dec_sig : -dec_sig;
                    double obl = get_obliquidade(jd);

                    // =========================================================
                    // 1. EXTRAINDO AS COORDENADAS EQUATORIAIS NATAIS REAIS DO PROMISSOR
                    // =========================================================
                    double xx_prom[3], xequat_prom[3];
                    xx_prom[0] = prom[p].longitude;
                    xx_prom[1] = calcular_latitude_dinamica_bianchini(jd, prom[p].object_name, prom[p].longitude, prom[p].type);
                    xx_prom[2] = 1.0;
                    swe_cotrans(xx_prom, xequat_prom, -obl);
                    
                    // REINTRODUÇÃO DAS SUAS VARIÁVEIS CRÍTICAS
                    double ra_natal_prom  = xequat_prom[0]; // RA Natal real do planeta promissor
                    double dec_natal_prom = xequat_prom[1]; // DEC Natal real do planeta promissor

                    // 2. Descobrimos onde a Eclíptica pura intercepta a declinação alvo do Significador
                    double sin_lon = sin(dec_alvo_paralelo * M_PI / 180.0) / sin(obl * M_PI / 180.0);
                    if (fabs(sin_lon) > 1.0) continue; // Declinação impossível de alcançar
                    
                    double lon_raiz1 = asin(sin_lon) * 180.0 / M_PI;
                    if (lon_raiz1 < 0) lon_raiz1 += 360.0;
                    double lon_raiz2 = fmod(180.0 - lon_raiz1 + 360.0, 360.0);

                    double xx_alvo[3], xequat_alvo[3];
                    double lon_cortes[2] = {lon_raiz1, lon_raiz2};

                    for (int k = 0; k < 2; k++) {
                        xx_alvo[0] = lon_cortes[k]; 
                        xx_alvo[1] = 0.0; // Paralelo zodiacal puro intercepta a linha da eclíptica
                        xx_alvo[2] = 1.0;
                        swe_cotrans(xx_alvo, xequat_alvo, -obl);
                        
                        double ra_aspecto  = xequat_alvo[0];
                        double dec_aspecto = dec_alvo_paralelo;

                        // =========================================================
                        // 3. MOTOR TOPOCÊNTRICO AMARRADO À POSIÇÃO NATAL DO PROMISSOR
                        // =========================================================
                        // Calculamos as Distâncias Ascensionais (AD) sob o mesmo Polo do Significador.
                        // Repare o acoplamento: 'dec_natal_prom' agora calibra a AD do Significador Natal!
                        double sin_ad_sig_polo  = tan(dec_natal_prom * M_PI / 180.0) * tan_polo_sig;
                        double sin_ad_prom_polo = tan(dec_aspecto * M_PI / 180.0) * tan_polo_sig;
                        
                        if (fabs(sin_ad_sig_polo) > 1.0)  sin_ad_sig_polo  = (sin_ad_sig_polo > 0)  ? 1.0 : -1.0;
                        if (fabs(sin_ad_prom_polo) > 1.0) sin_ad_prom_polo = (sin_ad_prom_polo > 0) ? 1.0 : -1.0;

                        // As Ascensões Oblíquas agora usam a 'ra_natal_prom' como o ponto de partida real!
                        double oa_sig_ficticio = ra_natal_prom - (asin(sin_ad_sig_polo) * 180.0 / M_PI);
                        double oa_prom         = ra_aspecto    - (asin(sin_ad_prom_polo) * 180.0 / M_PI);

                        // O arco mede a separação exata que diferencia cada planeta
                        arco = (s == 0) ? (oa_prom - oa_sig_ficticio) : (oa_sig_ficticio - oa_prom);
                        
                        if (arco < 0) arco += 360.0;
                        arco = fmod(arco, 360.0);
                        if (arco > 180.0) arco = 360.0 - arco;

                        if (arco > 0.001 && arco <= MAX_AGE * 1.05) {
                            // Filtro contra ecos internos do próprio planeta
                            int ja_salvo = 0;
                            for (int m = 0; m < *qtd_direcoes; m++) {
                                if (lista_resultado[m].sentido == s &&
                                    lista_resultado[m].promissor_id == prom[p].id &&
                                    lista_resultado[m].aspecto_id == a &&
                                    fabs(lista_resultado[m].arco_graus - arco) < 0.001) {
                                    ja_salvo = 1; break;
                                }
                            }
                            if (ja_salvo) continue;

                            // Preenchimento mantendo seus ponteiros originais intactos para o color-coding
                            LinhaDirecao *d = &lista_resultado[*qtd_direcoes];
                            d->sentido = s;

                            d->promissor_id = p;
                            d->aspecto_id = a;
                            strcpy(d->promissor_name, prom[p].object_name);
                            strcpy(d->promissor_glifo, prom[p].object);
                            strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                            strcpy(d->significador_name, sig[idx_alvo].object_name);
                            strcpy(d->significador_glifo, sig[idx_alvo].object);
                            d->promissor_type = prom[p].type;
                            d->arco_graus = arco;

                            // 1. Obter a chave/fator do sistema usando suas macros estáticas
                            double CHAVE = get_time_key(TIME_KEY, jd, arco);
                            if (CHAVE <= 0.0) CHAVE = NAIBOD_KEY; 

                            // 2. CÁLCULO UNIFICADO DA IDADE DO EVENTO (Em anos decimais contínuos)
                            d->idade_evento = arco / CHAVE;
                    
                            double dias_decorridos = 0.0;

                            // 3. FORK DE TRATAMENTO DE TEMPO COM PRECISÃO DE ACORDO COM CADA MACRO
                            if (TIME_KEY == TIME_KEY_NAIBOD) {
                                dias_decorridos = d->idade_evento * 365.256363004; // Inverso exato de NAIBOD_KEY
                            }
                            else if (TIME_KEY == TIME_KEY_CARDANO) {
                                dias_decorridos = d->idade_evento * 364.847162454; // Inverso exato de CARDANO_KEY
                            }
                            else if (TIME_KEY == TIME_KEY_PTOLEMY) {
                                dias_decorridos = d->idade_evento * 365.0;         // Ano Civil de Ptolomeu
                            }
                            else if (TIME_KEY == TIME_KEY_PLACIDUS) {
                                dias_decorridos = d->idade_evento * 365.25;        // Ano Juliano Eclesiástico
                            }
                            else if (TIME_KEY == TIME_KEY_KEPLER) {
                                d->idade_evento = arco * CHAVE;
                                double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                                dias_decorridos = d->idade_evento * ano_tropical_epoca;
                            }
                            else if (TIME_KEY == TIME_KEY_TRUE_SOLAR_ARC_RA) {
                                d->idade_evento = arco * CHAVE;
                                double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                                dias_decorridos = d->idade_evento * ano_tropical_epoca;
                            }
                            else {                                
                                double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                                dias_decorridos = d->idade_evento * ano_tropical_epoca;
                            }
                    
                            // 4. Projeção Estrita no Dia Juliano e Conversão UTC via Swiss Ephemeris
                            double jd_evento = jd + dias_decorridos;
                            
                            int ano_c, mes_c, dia_c, hora_c, min_c;
                            double sec_c;
                            swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);

                            d->ano_calendario = ano_c; d->mes_calendario = mes_c; d->dia_calendario = dia_c;
                            strcpy(d->tipo_direcao, "Zodiacal");
                            d->tipo_direcao_id = DIRECAO_ZODIACAL;

                            (*qtd_direcoes)++;
                            if (*qtd_direcoes >= 600) goto fim_calculo;
                        }
                    }
                    continue; // Pula o resto do loop de aspectos longitudinais para este planeta
                }



                // --- OUTROS 8 ASPECTOS LONGITUDINAIS TOPOCÊNTRICOS ---
                double lon_aspecto;
                if (s == 0) {
                    lon_aspecto = fmod(prom[p].longitude + angulos_aspectos[a], 360.0);
                } else if (prom[p].type == PROM_TERM && s == 1) {
                    lon_aspecto = fmod(prom[p].longitude_fim - angulos_aspectos[a], 360.0);
                } else {
                    lon_aspecto = fmod(prom[p].longitude - angulos_aspectos[a], 360.0);
                }
                lon_aspecto = fmod(lon_aspecto, 360.0);
                if (lon_aspecto < 0.0) lon_aspecto += 360.0;

                double lat_calculada = calcular_latitude_dinamica_bianchini(jd, prom[p].object_name, lon_aspecto, prom[p].type);

                double xx[3], xequat[3];
                xx[0] = lon_aspecto; xx[1] = lat_calculada; xx[2] = 1.0;
                swe_cotrans(xx, xequat, -get_obliquidade(jd));
                double ra_aspecto = xequat[0];
                double dec_aspecto = xequat[1];

                // Ascensão Oblíqua sob o Polo Topocêntrico do Significador
                double sin_ad_sig_polo  = tan(dec_sig_rad) * tan_polo_sig;
                double sin_ad_prom_polo = tan(dec_aspecto * M_PI / 180.0) * tan_polo_sig;
                if (fabs(sin_ad_sig_polo) > 1.0)  sin_ad_sig_polo  = (sin_ad_sig_polo > 0)  ? 1.0 : -1.0;
                if (fabs(sin_ad_prom_polo) > 1.0) sin_ad_prom_polo = (sin_ad_prom_polo > 0) ? 1.0 : -1.0;

                double oa_sig  = ra_sig - (asin(sin_ad_sig_polo) * 180.0 / M_PI);
                double oa_prom = ra_aspecto - (asin(sin_ad_prom_polo) * 180.0 / M_PI);

                arco = (s == 0) ? (oa_prom - oa_sig) : (oa_sig - oa_prom);
                
                if (arco < 0) arco += 360.0;
                arco = fmod(arco, 360.0);
                if (arco > 180.0) arco = 360.0 - arco;

                if (arco > 0.001 && arco <= MAX_AGE * 1.05) {
                    int ja_salvo = 0;
                    for (int m = 0; m < *qtd_direcoes; m++) {
                        if (lista_resultado[m].sentido == s &&
                            lista_resultado[m].promissor_id == prom[p].id &&
                            lista_resultado[m].aspecto_id == a &&
                            fabs(lista_resultado[m].arco_graus - arco) < 0.001) {
                            ja_salvo = 1; break;
                        }
                    }
                    if (ja_salvo) continue;

                    LinhaDirecao *d = &lista_resultado[*qtd_direcoes];
                    d->sentido = s;

                    d->promissor_id = p;
                    d->aspecto_id = a;
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    strcpy(d->significador_name, sig[idx_alvo].object_name);
                    strcpy(d->significador_glifo, sig[idx_alvo].object);
                    d->promissor_type = prom[p].type;
                    d->arco_graus = arco;

                    // 1. Obter a chave/fator do sistema usando suas macros estáticas
                    double CHAVE = get_time_key(TIME_KEY, jd, arco);
                    if (CHAVE <= 0.0) CHAVE = NAIBOD_KEY; 

                    // 2. CÁLCULO UNIFICADO DA IDADE DO EVENTO (Em anos decimais contínuos)
                    d->idade_evento = arco / CHAVE;
            
                    double dias_decorridos = 0.0;

                    // 3. FORK DE TRATAMENTO DE TEMPO COM PRECISÃO DE ACORDO COM CADA MACRO
                    if (TIME_KEY == TIME_KEY_NAIBOD) {
                        dias_decorridos = d->idade_evento * 365.256363004; // Inverso exato de NAIBOD_KEY
                    }
                    else if (TIME_KEY == TIME_KEY_CARDANO) {
                        dias_decorridos = d->idade_evento * 364.847162454; // Inverso exato de CARDANO_KEY
                    }
                    else if (TIME_KEY == TIME_KEY_PTOLEMY) {
                        dias_decorridos = d->idade_evento * 365.0;         // Ano Civil de Ptolomeu
                    }
                    else if (TIME_KEY == TIME_KEY_PLACIDUS) {
                        dias_decorridos = d->idade_evento * 365.25;        // Ano Juliano Eclesiástico
                    }
                    else if (TIME_KEY == TIME_KEY_KEPLER) {
                        d->idade_evento = arco * CHAVE;
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
                    else if (TIME_KEY == TIME_KEY_TRUE_SOLAR_ARC_RA) {
                        d->idade_evento = arco * CHAVE;
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
                    else {                                
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
            
                    // 4. Projeção Estrita no Dia Juliano e Conversão UTC via Swiss Ephemeris
                    double jd_evento = jd + dias_decorridos;

                    int ano_c, mes_c, dia_c, hora_c, min_c;
                    double sec_c;
                    swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);

                    d->ano_calendario = ano_c; d->mes_calendario = mes_c; d->dia_calendario = dia_c;
                    strcpy(d->tipo_direcao, "Zodiacal");
                    d->tipo_direcao_id = DIRECAO_ZODIACAL;
                    
                    (*qtd_direcoes)++;
                    if (*qtd_direcoes >= 600) goto fim_calculo;
                }
            }
        }
    }

fim_calculo:
    qsort(lista_resultado, *qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);

    if (*qtd_direcoes > 1) {
        int i_valido = 0;

        for (int i_atual = 1; i_atual < *qtd_direcoes; i_atual++) {
            // Mudança sutil: Comparamos o atual com o último elemento que aceitamos como válido
            LinhaDirecao *d0 = &lista_resultado[i_valido];
            LinhaDirecao *d1 = &lista_resultado[i_atual];

            bool mesmo_promissor = (d0->promissor_id == d1->promissor_id);
            bool mesmo_sentido   = (d0->sentido == d1->sentido);
            
            double delta_arco = fabs(d1->arco_graus - d0->arco_graus);
            bool arco_identico = (delta_arco < 0.0001); // Use um epsilon pequeno explícito (ex: 0.0001)

            // Se o mesmo planeta, no mesmo sentido (Direto/Converso), atingir o mesmo arco geométrico...
            if (mesmo_promissor && mesmo_sentido && arco_identico) {
                continue; // Ignora o d1 (duplicado)
            }

            // Se não for duplicado, movemos o ponteiro de gravação
            i_valido++;
            if (i_valido != i_atual) {
                lista_resultado[i_valido] = *d1;
            }
        }

        *qtd_direcoes = i_valido + 1;
    }
    return *qtd_direcoes;
}







// Calcula o cronograma de direções zodiacais para QUALQUER ponto escolhido
int calcular_direcoes_zodiacais_geral(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, double jd, int sentido, Promissor *prom, bool is_part) {

    int qtd_direcoes = 0;
    int object_diff = show_modern_planets ? 0 : 3;

    double ra_significador = sig[idx_alvo].ra;
    double dec_significador = sig[idx_alvo].declination; // Declinação natal do alvo

    double angulos_aspectos[] = {0.0, 60.0, -60.0, 90.0, -90.0, 120.0, -120.0, 180.0, 999.9, 999.9};
    char *simbolos_aspectos[] = {"☌", "⚹", "⚹", "□", "□", "△", "△", "☍", "∥", "∦"};
    int total_aspects = 10;

    bool sig_is_fortune = strcmp(sig[idx_alvo].object, "FOR") == 0;



    for (int p = 0; p < prom_id; p++) {
        if ((prom[p].type == PROM_ANTISCIUM || prom[p].type == PROM_CONTRANTISCIUM) && !ANT_PROM) continue;
        else if (prom[p].type == PROM_POINT || prom[p].type == PROM_ANGLE) continue;
        else if (!is_part) {
            if ((p == idx_alvo && p < NUM_OBJECTS - object_diff - (show_modern_planets ? 5 : 4))) continue;
        }
        else {
            if (sig_is_fortune && (p == P_FORTUNA - object_diff)) continue;
        }

        for (int s = 0; s < 2; s++) {
            if (s == 0 && sentido == 1) continue;
            if (s == 1 && sentido == 0) continue;

            for (int a = 0; a < total_aspects; a++) {

                if (prom[p].type == PROM_TERM && a > 0) break; // apenas conjunções para termos

                double arco = 0.0;
                int eh_paralelo = (a == 8 || a == 9);

                if (eh_paralelo) {
                    double dec_alvo_paralelo = (a == 8) ? dec_significador : -dec_significador;

                    // Pega as coordenadas equatoriais NATAIS do promissor
                    // Precisamos saber a AR natal do promissor para descobrir quanto ele precisa andar
                    double xx_prom[3], xequat_prom[3];
                    xx_prom[0] = prom[p].longitude;
                    xx_prom[1] = calcular_latitude_dinamica_bianchini(jd, prom[p].object_name, prom[p].longitude, prom[p].type);
                    xx_prom[2] = 1.0;
                    swe_cotrans(xx_prom, xequat_prom, -get_obliquidade(jd));
                    double ra_natal_prom = xequat_prom[0];

                    // Agora, descobrimos em quais longitudes do zodíaco essa declinação alvo existe.
                    // Como a declinação é simétrica, existem dois pontos na eclíptica pura:
                    double obl = get_obliquidade(jd);
                    double sin_lon = sin(dec_alvo_paralelo * M_PI / 180.0) / sin(obl * M_PI / 180.0);
                    
                    if (fabs(sin_lon) > 1.0) continue; // Declinação impossível de alcançar na eclíptica
                    
                    double lon_raiz1 = asin(sin_lon) * 180.0 / M_PI;
                    if (lon_raiz1 < 0) lon_raiz1 += 360.0;
                    double lon_raiz2 = fmod(180.0 - lon_raiz1 + 360.0, 360.0);

                    // Convertemos essas duas longitudes alvo para Ascensão Reta (AR)
                    double xx_alvo[3], xequat_alvo[3];
                    
                    // Ponto Geométrico 1
                    xx_alvo[0] = lon_raiz1; xx_alvo[1] = 0.0; xx_alvo[2] = 1.0;
                    swe_cotrans(xx_alvo, xequat_alvo, -obl);
                    double ra_alvo1 = xequat_alvo[0];

                    // Ponto Geométrico 2
                    xx_alvo[0] = lon_raiz2; xx_alvo[1] = 0.0; xx_alvo[2] = 1.0;
                    swe_cotrans(xx_alvo, xequat_alvo, -obl);
                    double ra_alvo2 = xequat_alvo[0];

                    // O ARCO é a distância que o PROMISSOR precisa andar de sua AR natal até a AR alvo!
                    // Aqui está a mágica: agora o cálculo usa a 'ra_natal_prom', diferenciando cada planeta!
                    double arco1 = ra_alvo1 - ra_natal_prom;
                    double arco2 = ra_alvo2 - ra_natal_prom;

                    if (arco1 < 0) arco1 += 360.0;
                    if (arco2 < 0) arco2 += 360.0;
                                        
                    double arcos_paralelo[2] = {arco1, arco2};

                    for (int k = 0; k < 2; k++) {
                        arco = arcos_paralelo[k];

                        // Filtra arcos de idade humana viável (0 a 150 anos)
                        if (arco > 0.001 && arco <= MAX_AGE * 1.05) {
                            LinhaDirecao *d = &lista_resultado[qtd_direcoes];
                            d->sentido = s;
                            
                            d->promissor_id = p;
                            d->aspecto_id = a;
                            strcpy(d->promissor_name, prom[p].object_name);
                            strcpy(d->promissor_glifo, prom[p].object);
                            strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                            strcpy(d->significador_name, sig[idx_alvo].object_name);
                            strcpy(d->significador_glifo, sig[idx_alvo].object);
                            d->promissor_type = prom[p].type;
                            d->arco_graus = arco;

                            // 1. Obter a chave/fator do sistema usando suas macros estáticas
                            double CHAVE = get_time_key(TIME_KEY, jd, arco);
                            if (CHAVE <= 0.0) CHAVE = NAIBOD_KEY; 

                            // 2. CÁLCULO UNIFICADO DA IDADE DO EVENTO (Em anos decimais contínuos)
                            d->idade_evento = arco / CHAVE;
                    
                            double dias_decorridos = 0.0;

                            // 3. FORK DE TRATAMENTO DE TEMPO COM PRECISÃO DE ACORDO COM CADA MACRO
                            if (TIME_KEY == TIME_KEY_NAIBOD) {
                                dias_decorridos = d->idade_evento * 365.256363004; // Inverso exato de NAIBOD_KEY
                            }
                            else if (TIME_KEY == TIME_KEY_CARDANO) {
                                dias_decorridos = d->idade_evento * 364.847162454; // Inverso exato de CARDANO_KEY
                            }
                            else if (TIME_KEY == TIME_KEY_PTOLEMY) {
                                dias_decorridos = d->idade_evento * 365.0;         // Ano Civil de Ptolomeu
                            }
                            else if (TIME_KEY == TIME_KEY_PLACIDUS) {
                                dias_decorridos = d->idade_evento * 365.25;        // Ano Juliano Eclesiástico
                            }
                            else if (TIME_KEY == TIME_KEY_KEPLER) {
                                d->idade_evento = arco * CHAVE;
                                double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                                dias_decorridos = d->idade_evento * ano_tropical_epoca;
                            }
                            else if (TIME_KEY == TIME_KEY_TRUE_SOLAR_ARC_RA) {
                                d->idade_evento = arco * CHAVE;
                                double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                                dias_decorridos = d->idade_evento * ano_tropical_epoca;
                            }
                            else {                                
                                double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                                dias_decorridos = d->idade_evento * ano_tropical_epoca;
                            }
                    
                            // 4. Projeção Estrita no Dia Juliano e Conversão UTC via Swiss Ephemeris
                            double jd_evento = jd + dias_decorridos;
        
                            int ano_c, mes_c, dia_c, hora_c, min_c;
                            double sec_c;
                            swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);

                            d->ano_calendario = ano_c;
                            d->mes_calendario = mes_c;
                            d->dia_calendario = dia_c;
                                            
                            strcpy(d->tipo_direcao, "Zodiacal");
                            d->tipo_direcao_id = DIRECAO_ZODIACAL;

                            qtd_direcoes++;
                            if (qtd_direcoes >= 600) goto fim_calculo;
                        }
                    }
                    continue; // Pula o resto do loop padrão de aspectos longitudinais para não duplicar dados!
                }

                // --- LOGICA ORIGINAL PARA OS OUTROS 5 ASPECTOS (a < 5) ---

                double lon_aspecto; // = fmod(prom[p].longitude + angulos_aspectos[a], 360.0);
                
                if (s == 0) {
                    lon_aspecto = fmod(prom[p].longitude + angulos_aspectos[a], 360.0);
                } 
                else if (prom[p].type == PROM_TERM && s == 1) {
                    lon_aspecto = fmod(prom[p].longitude_fim - angulos_aspectos[a], 360.0);
                }
                else {
                    lon_aspecto = fmod(prom[p].longitude - angulos_aspectos[a], 360.0);
                }

                // Normalização estrita da longitude alvo (0 a 360)
                lon_aspecto = fmod(lon_aspecto, 360.0);
                if (lon_aspecto < 0.0) lon_aspecto += 360.0;

                double lat_calculada = calcular_latitude_dinamica_bianchini(jd, prom[p].object_name, lon_aspecto, prom[p].type);

                double xx[3];
                double xequat[3];

                xx[0] = lon_aspecto;   
                xx[1] = lat_calculada; 
                xx[2] = 1.0;           

                swe_cotrans(xx, xequat, -get_obliquidade(jd)); 

                double ra_aspecto = xequat[0];  // ÍNDICE CORRETO: [0] para Ascensão Reta
                //double dec_aspecto = xequat[1]; // Opcional: [1] para se precisar da Declinação dinâmica


                // Agora o ra_aspecto já saiu pronto e perfeitamente simétrico ao significador!

                //double arco = 0.0;

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
                if (arco > 0.001 && arco <= MAX_AGE * 1.05) {
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];

                    d->sentido = s;
                    
                    d->promissor_id = p;
                    d->aspecto_id = a;
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    
                    // Salva o nome e glifo do Significador Alvo atual
                    strcpy(d->significador_name, sig[idx_alvo].object_name);
                    strcpy(d->significador_glifo, sig[idx_alvo].object);
                    
                    d->promissor_type = prom[p].type;

                    // 1. Calcula o arco e a idade do evento normalmente
                    d->arco_graus = arco;

                    // 1. Obter a chave/fator do sistema usando suas macros estáticas
                    double CHAVE = get_time_key(TIME_KEY, jd, arco);
                    if (CHAVE <= 0.0) CHAVE = NAIBOD_KEY; 

                    // 2. CÁLCULO UNIFICADO DA IDADE DO EVENTO (Em anos decimais contínuos)
                    d->idade_evento = arco / CHAVE;
            
                    double dias_decorridos = 0.0;

                    // 3. FORK DE TRATAMENTO DE TEMPO COM PRECISÃO DE ACORDO COM CADA MACRO
                    if (TIME_KEY == TIME_KEY_NAIBOD) {
                        dias_decorridos = d->idade_evento * 365.256363004; // Inverso exato de NAIBOD_KEY
                    }
                    else if (TIME_KEY == TIME_KEY_CARDANO) {
                        dias_decorridos = d->idade_evento * 364.847162454; // Inverso exato de CARDANO_KEY
                    }
                    else if (TIME_KEY == TIME_KEY_PTOLEMY) {
                        dias_decorridos = d->idade_evento * 365.0;         // Ano Civil de Ptolomeu
                    }
                    else if (TIME_KEY == TIME_KEY_PLACIDUS) {
                        dias_decorridos = d->idade_evento * 365.25;        // Ano Juliano Eclesiástico
                    }
                    else if (TIME_KEY == TIME_KEY_KEPLER) {
                        d->idade_evento = arco * CHAVE;
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
                    else if (TIME_KEY == TIME_KEY_TRUE_SOLAR_ARC_RA) {
                        d->idade_evento = arco * CHAVE;
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
                    else {                                
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
            
                    // 4. Projeção Estrita no Dia Juliano e Conversão UTC via Swiss Ephemeris
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
                    d->tipo_direcao_id = DIRECAO_ZODIACAL;

                    qtd_direcoes++;
                    if (qtd_direcoes >= 600) goto fim_calculo;
                }
            }
        }
    }

fim_calculo:
    qsort(lista_resultado, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);

    if (qtd_direcoes > 1) {
        int i_valido = 0;

        for (int i_atual = 1; i_atual < qtd_direcoes; i_atual++) {
            // Mudança sutil: Comparamos o atual com o último elemento que aceitamos como válido
            LinhaDirecao *d0 = &lista_resultado[i_valido];
            LinhaDirecao *d1 = &lista_resultado[i_atual];

            bool mesmo_promissor = (d0->promissor_id == d1->promissor_id);
            bool mesmo_sentido   = (d0->sentido == d1->sentido);
            
            double delta_arco = fabs(d1->arco_graus - d0->arco_graus);
            bool arco_identico = (delta_arco < 0.0001); // Use um epsilon pequeno explícito (ex: 0.0001)

            // Se o mesmo planeta, no mesmo sentido (Direto/Converso), atingir o mesmo arco geométrico...
            if (mesmo_promissor && mesmo_sentido && arco_identico) {
                continue; // Ignora o d1 (duplicado)
            }

            // Se não for duplicado, movemos o ponteiro de gravação
            i_valido++;
            if (i_valido != i_atual) {
                lista_resultado[i_valido] = *d1;
            }
        }

        qtd_direcoes = i_valido + 1;
    }
    return qtd_direcoes;
}



// int verificar_se_acima_horizonte(double ra_planeta, double dec_planeta_rad, double ramc, double lat_geo_rad) {
//     // 1. Calcula a menor Distância Meridiana em relação ao MC (Superior) de forma circular segura
//     double md_mc = fabs(ra_planeta - ramc);
//     if (md_mc > 180.0) md_mc = 360.0 - md_mc;

//     // 2. Calcula o Semi-Arco Diurno (SAD) exato via cossenos
//     double tan_lat = tan(lat_geo_rad);
//     double tan_dec = tan(dec_planeta_rad);
//     double cos_sad = -tan_lat * tan_dec;

//     // Proteção rigorosa contra regiões circumpolares (Sol da meia-noite / Noite polar)
//     if (cos_sad >= 1.0)  return 0; // Nunca nasce (Sempre abaixo do horizonte)
//     if (cos_sad <= -1.0) return 1; // Nunca se põe (Sempre acima do horizonte)

//     double sad_graus = acos(cos_sad) * (180.0 / M_PI);

//     // Se a distância ao Meio do Céu for menor ou igual ao Semi-Arco Diurno, está acima
//     return (md_mc <= sad_graus) ? 1 : 0;
// }

// Retorna 1 se o planeta estiver ACIMA do horizonte, e 0 se estiver ABAIXO
int verificar_se_acima_horizonte(double ra_planeta, double dec_planeta_rad, double ramc, double lat_geo_rad) {
    // 1. Calcula a menor Distância Meridiana em relação ao MC (Superior)
    double md_mc = ra_planeta - ramc;
    if (md_mc > 180.0)  md_mc -= 360.0;
    if (md_mc < -180.0) md_mc += 360.0;
    md_mc = fabs(md_mc);

    // 2. Calcula o Semi-Arco Diurno (SAD) exato via cossenos
    double tan_lat = tan(lat_geo_rad);
    double tan_dec = tan(dec_planeta_rad);
    double cos_sad = -tan_lat * tan_dec;

    // Proteção rigorosa contra regiões circumpolares (Sol da meia-noite / Noite polar)
    if (cos_sad >= 1.0)  return 0; // Nunca nasce (Sempre abaixo)
    if (cos_sad <= -1.0) return 1; // Nunca se põe (Sempre acima)

    double sad_graus = acos(cos_sad) * (180.0 / M_PI);

    // CORREÇÃO CRÍTICA: Tolerância de ponto flutuante (1 milionésimo de grau)
    // Evita que erros de arredondamento joguem o Descendente para baixo e o Ascendente para cima
    double epsilon = 1e-6;

    if (fabs(md_mc - sad_graus) < epsilon) {
        // O ponto está EXATAMENTE sobre o horizonte (Borda do Ascendente/Descendente).
        // Na astrologia tradicional e matemática de Placidus, os pontos de horizonte 
        // são padronizados como início do quadrante diurno (Acima) ou noturno (Abaixo).
        // Adotaremos 1 (Acima) para ambos para manter a simetria de espelhamento do motor.
        return 1; 
    }

    // Se a distância ao Meio do Céu for menor ou igual ao Semi-Arco Diurno, está acima
    return (md_mc <= sad_graus) ? 1 : 0;
}


double __calcular_semi_arco(double dec_rad, double lat_rad, int esta_acima) {
    // Fórmula astronômica esférica estrita para Diferença Ascensional (DA)
    double sin_DA = tan(lat_rad) * tan(dec_rad);

    // Proteção contra casos circumpolares extremos
    if (sin_DA >= 1.0) sin_DA = 1.0;
    if (sin_DA <= -1.0) sin_DA = -1.0;

    // Trabalhamos com o módulo da Diferença Ascensional para evitar confusão de sinais do quadrante
    double DA_graus = fabs(asin(sin_DA) * (180.0 / M_PI));
    double semi_arco = 0.0;

    // Regra de Sinais de Placidus: Se Lat e Dec têm o mesmo sinal, o arco diurno é maior que 90°
    // Em C, (lat_rad * dec_rad >= 0) checa se os sinais são iguais (ambos + ou ambos -)
    int mesmo_hemisferio = (lat_rad * dec_rad >= 0);

    if (esta_acima) {
        // Semi-Arco Diurno
        semi_arco = mesmo_hemisferio ? (90.0 + DA_graus) : (90.0 - DA_graus);
    } else {
        // Semi-Arco Noturno
        semi_arco = mesmo_hemisferio ? (90.0 - DA_graus) : (90.0 + DA_graus);
    }

    // Blindagem de ponto flutuante
    if (semi_arco <= 0.0) semi_arco = 90.0;
    
    return semi_arco;
}

double __calcular_distancia_meridiana(double ra_planeta, double ramc, int esta_acima) {
    double md = 0.0;

    if (esta_acima) {
        // Distância em relação ao Meio do Céu (RAMC)
        md = fabs(ra_planeta - ramc);
    } else {
        // Distância em relação ao Fundo do Céu (RAIC = RAMC + 180)
        double ra_ic = ramc + 180.0;
        if (ra_ic >= 360.0) ra_ic -= 360.0;
        md = fabs(ra_planeta - ra_ic);
    }

    // Correção estrita de descontinuidade de arco menor circular (0 a 180)
    if (md > 180.0) {
        md = 360.0 - md;
    }

    return md;
}


double get_time_key(int key, double jd, double arco) {
    switch(key) {
        case TIME_KEY_NAIBOD:                   return NAIBOD_KEY;
        case TIME_KEY_CARDANO:                  return CARDANO_KEY;
        case TIME_KEY_PTOLEMY:                  return PTOLEMY_KEY;
        case TIME_KEY_PLACIDUS:                 return PLACIDUS_KEY;
        case TIME_KEY_TRUE_SOLAR_ARC_LONGITUDE: return obter_chave_arco_solar_ultra_rapida(jd, arco); //obter_chave_arco_solar(jd, arco);           
        case TIME_KEY_KEPLER:                   return obter_chave_kepler(jd, arco); //obter_chave_kepler_ultra_rapida(jd, arco); //obter_chave_kepler(jd, arco);
        case TIME_KEY_TRUE_SOLAR_ARC_RA:        return obter_chave_placidus_ra(jd, arco); //encontrar_chave_por_arco_solar_ra_FAST(jd, arco);
        default:                                return NAIBOD_KEY;
    }
}


double get_key(int key) {
    switch(key) {
        case TIME_KEY_NAIBOD:                   return NAIBOD_KEY;
        case TIME_KEY_CARDANO:                  return CARDANO_KEY;
        case TIME_KEY_PTOLEMY:                  return PTOLEMY_KEY;
        case TIME_KEY_PLACIDUS:                 return PLACIDUS_KEY;
        case TIME_KEY_TRUE_SOLAR_ARC_LONGITUDE: return -1.0;           
        case TIME_KEY_KEPLER:                   return -1.0;
        case TIME_KEY_TRUE_SOLAR_ARC_RA:        return -1.0;
        default: return NAIBOD_KEY;
    }
}


const char* get_key_name(int key) {
    switch(key) {
        case TIME_KEY_NAIBOD:                   return "Naibod";
        case TIME_KEY_CARDANO:                  return _("Cardano");
        case TIME_KEY_PTOLEMY:                  return _("Ptolemy");
        case TIME_KEY_PLACIDUS:                 return "Placidus";
        case TIME_KEY_TRUE_SOLAR_ARC_LONGITUDE: return _("True Solar Ecliptical Arc");           
        case TIME_KEY_KEPLER:                   return "Kepler";
        case TIME_KEY_TRUE_SOLAR_ARC_RA:        return _("True Solar Equatorial Arc");
        default: return "Naibod";
    }
}



double obter_proporcao_polar_casa(int numero_casa) {
    switch(numero_casa) {
        case 10: return  0.0;         // Meio do Céu (Cresta do Meridiano)
        case 11: return -0.33333333;  // Leste, Acima (1/3 do SAD)
        case 12: return -0.66666667;  // Leste, Acima (2/3 do SAD)
        case 1:  return -1.0;         // Ascendente (Horizonte Leste) - Pode usar -1.0 ou 1.0 dependendo do hemisfério do SA
        case 2:  return -0.66666667;  // Leste, Abaixo (2/3 do SAN)
        case 3:  return -0.33333333;  // Leste, Abaixo (1/3 do SAN)
        case 4:  return  0.0;         // Fundo do Céu (IC)
        case 5:  return  0.33333333;  // Oeste, Abaixo (1/3 do SAN)
        case 6:  return  0.66666667;  // Oeste, Abaixo (2/3 do SAN)
        case 7:  return  1.0;         // Descendente (Horizonte Oeste)
        case 8:  return  0.66666667;  // Oeste, Acima (2/3 do SAD)
        case 9:  return  0.33333333;  // Oeste, Acima (1/3 do SAD)
        default: return  0.0;
    }
}



double calcular_cota_universal(double posicao_domal_swe) {
    int casa_base = (int)posicao_domal_swe;                // Ex: 11
    double fracao = posicao_domal_swe - casa_base;         // Ex: 0.5
    
    // Mapeia linearmente as transições de cúspides do espaço local
    double cota_cuspide_atual = obter_proporcao_polar_casa(casa_base);
    
    int proxima_casa = (casa_base == 12) ? 1 : casa_base + 1;
    double cota_proxima_cuspide = obter_proporcao_polar_casa(proxima_casa);
    
    // Interpolação que amarra a cota com o sistema de casas em uso!
    return cota_cuspide_atual + (fracao * (cota_proxima_cuspide - cota_cuspide_atual));
}


double calcular_cota_dinamica_sistema(double house_pos) {
    // Tratamento estrito de bordas para evitar overflow circular
    if (house_pos < 1.0) house_pos += 12.0;
    if (house_pos >= 13.0) house_pos -= 12.0;

    int casa_base = (int)house_pos;                  // Ex: 11
    double fracao = house_pos - (double)casa_base;   // Ex: 0.5

    // Busca as cotas espaciais das duas cúspides que delimitam o planeta
    double cota_atual = obter_proporcao_polar_casa(casa_base);
    
    int proxima_casa = (casa_base == 12) ? 1 : casa_base + 1;
    double cota_proxima = obter_proporcao_polar_casa(proxima_casa);

    // Tratamento matemático especial para a transição crítica do Horizonte (Casa 1 para Casa 2)
    // No nosso mapa de cotas, a Casa 1 vale -1.0 e a Casa 2 vale -0.666667.
    // Porém, o Horizonte Oeste (Casa 7 para Casa 8) passa de 1.0 para 0.666667.
    // A interpolação linear simples resolve perfeitamente a variação contínua dos quadrantes:
    double cota_interpolada = cota_atual + (fracao * (cota_proxima - cota_atual));

    return cota_interpolada;
}


// NOVA FUNÇÃO: Conversão Zodiacal COMPLETA (Com Latitude Eclíptica β)
void converter_zodiacal_com_latitude(double longitude, double latitude_ecliptica, double *ra, double *dec) {
    double lam = para_radianos(longitude);          // λ
    double bet = para_radianos(latitude_ecliptica); // β
    double eps = para_radianos(OBLIQUIDADE);         // ε
    
    // 1. Cálculo da Declinação (δ)
    double sin_dec = sin(bet) * cos(eps) + cos(bet) * sin(eps) * sin(lam);
    double dec_rad = asin(sin_dec);
    *dec = para_graus(dec_rad);
    
    // 2. Cálculo da Ascensão Reta (RA)
    double num = sin(lam) * cos(eps) - tan(bet) * sin(eps);
    double den = cos(lam);
    
    double ra_rad = atan2(num, den);
    double ra_graus = para_graus(ra_rad);
    
    // Normalização no círculo de 0 a 360 graus
    if (ra_graus < 0) ra_graus += 360.0;
    *ra = ra_graus;
}



double calcular_arco_mundano_topocentrico_interno(double ra_sig, double dec_sig_rad, double cota_sig, 
                                                 double ra_prom, double dec_prom_rad, double cota_prom_natal,
                                                 double offset_proporcao, int a, int s, double ramc, double lat_geografica) {
    double lat_geo_rad = para_radianos(lat_geografica);
    double cota_alvo_sig = cota_sig;
    double cota_alvo_prom = cota_prom_natal;

    (void)ramc;

    // 1. FILTRO TOPOLÓGICO: Validação geométrica de quadrantes para Rapt-Parallel e Contra-Parallel
    if (a == 10 || a == 11) {
        // Sinal da cota representa o lado em relação ao meridiano (Leste < 0, Oeste > 0)
        int mesmos_lados_meridiano = ((cota_sig >= 0 && cota_prom_natal >= 0) || (cota_sig < 0 && cota_prom_natal < 0));

        if (a == 10 && !mesmos_lados_meridiano) {
            return -1.0; // Aborta Rapt Parallel em lados opostos
        }
        if (a == 11 && mesmos_lados_meridiano) {
            return -1.0; // Aborta Rapt Contra-Parallel no mesmo lado
        }

        // DECISÃO DE EIXO TOPOCÊNTRICO: 
        // As cotas vão de 0.0 (Meridiano) a 1.0/-1.0 (Horizonte).
        int focado_no_horizonte = (fabs(cota_sig) + fabs(cota_prom_natal) > 1.0) ? 1 : 0;

        // Variável auxiliar para evitar divisões por zero em planetas muito próximos do meridiano
        double soma_cotas = fabs(cota_sig) + fabs(cota_prom_natal);
        if (soma_cotas <= 0.0001) soma_cotas = 1.0;

        if (!focado_no_horizonte) {
            // --- EIXO DO MERIDIANO (Cota Proporcional Dinâmica) ---
            double cota_proporcional = (fabs(cota_sig) * cota_prom_natal + fabs(cota_prom_natal) * cota_sig) / soma_cotas;

            if (a == 10) {
                if (s == 0) cota_alvo_prom = cota_proporcional;
                else        cota_alvo_sig  = cota_proporcional;
            } 
            else if (a == 11) {
                if (s == 0) cota_alvo_prom = -cota_proporcional;
                else        cota_alvo_sig  = -cota_proporcional;
            }
        } 
        else {
            // --- EIXO DO HORIZONTE (Cota Complementar Proporcional) ---
            // 1. Encontra a distância proporcional que falta para cada um tocar o horizonte (1.0)
            double dist_horizonte_sig  = 1.0 - fabs(cota_sig);
            double dist_horizonte_prom = 1.0 - fabs(cota_prom_natal);
            double soma_dist_horizonte = dist_horizonte_sig + dist_horizonte_prom;
            if (soma_dist_horizonte <= 0.0001) soma_dist_horizonte = 1.0;

            // 2. Calcula o ponto médio complementar ponderado no horizonte
            double dist_proporcional_encontro = (dist_horizonte_sig * dist_horizonte_prom + dist_horizonte_prom * dist_horizonte_sig) / soma_dist_horizonte;

            if (a == 10) {
                if (s == 0) {
                    double sinal = (cota_sig >= 0) ? 1.0 : -1.0;
                    cota_alvo_prom = sinal * (1.0 - dist_proporcional_encontro);
                } else {
                    double sinal = (cota_prom_natal >= 0) ? 1.0 : -1.0;
                    cota_alvo_sig  = sinal * (1.0 - dist_proporcional_encontro);
                }
            } 
            else if (a == 11) {
                if (s == 0) {
                    double sinal = (cota_sig >= 0) ? -1.0 : 1.0; // Inverte hemisfério
                    cota_alvo_prom = sinal * (1.0 - dist_proporcional_encontro);
                } else {
                    double sinal = (cota_prom_natal >= 0) ? -1.0 : 1.0;
                    cota_alvo_sig  = sinal * (1.0 - dist_proporcional_encontro);
                }
            }
        }


    } 
    // Tratamento de Aspectos e Declinações Mundanas Padrão
    else if (a == 8 || a == 9) { 
        if (s == 0) cota_alvo_sig = (a == 8) ? cota_sig : -cota_sig;
        else        cota_alvo_prom = (a == 8) ? cota_prom_natal : -cota_prom_natal;
    } 
    else if (a < 8) { 
        if (s == 0) cota_alvo_sig = cota_sig + offset_proporcao;
        else        cota_alvo_prom = cota_prom_natal + offset_proporcao;
    }
    
    // 2. Determinação dos Polos Topocêntricos Contínuos (Fórmula de Tangente)
    double tan_polo_sig  = tan(lat_geo_rad) * fabs(cota_alvo_sig);
    double tan_polo_prom = tan(lat_geo_rad) * fabs(cota_alvo_prom);

    // 3. Cálculos de Distância Ascensional (AD) sob os respectivos Polos Topocêntricos
    double sin_ad_sig  = tan(dec_sig_rad) * tan_polo_sig;
    double sin_ad_prom = tan(dec_prom_rad) * tan_polo_sig; 
    if (s == 1) {
        sin_ad_sig  = tan(dec_sig_rad) * tan_polo_prom;    
        sin_ad_prom = tan(dec_prom_rad) * tan_polo_prom;
    }

    // Travas de segurança para limites esféricos fora da curva
    if (fabs(sin_ad_sig) > 1.0)  sin_ad_sig  = (sin_ad_sig > 0)  ? 1.0 : -1.0;
    if (fabs(sin_ad_prom) > 1.0) sin_ad_prom = (sin_ad_prom > 0) ? 1.0 : -1.0;

    double ad_sig_graus  = asin(sin_ad_sig) * 180.0 / M_PI;
    double ad_prom_graus = asin(sin_ad_prom) * 180.0 / M_PI;

    // 4. Aplicação da regra de sinais Leste/Oeste para gerar Ascensões Oblíquas (OA = RA - AD)
    double oa_sig  = ra_sig  - (ad_sig_graus  * (cota_alvo_sig < 0  ? 1.0 : -1.0));
    double oa_prom = ra_prom - (ad_prom_graus * (cota_alvo_prom < 0 ? 1.0 : -1.0));

    // 5. O Arco Topocêntrico final
    double arco_topo = 0.0;
    if (s == 0) {
        arco_topo = oa_prom - oa_sig;
    } else {
        arco_topo = oa_sig - oa_prom;
    }

    // Normalização matemática do arco resultante entre -180 e 180
    if (arco_topo > 180.0)  arco_topo -= 360.0;
    if (arco_topo < -180.0) arco_topo += 360.0;
    arco_topo = fabs(arco_topo);

    return arco_topo;
}





int calcular_direcoes_mundanas_geral(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promissor *prom, int *mundane_aspects) {
    int qtd_direcoes = 0;
    double lat_geo_rad = para_radianos(lat_geografica);

    // 1. Dados tridimensionais REAIS do Significador (Alvo)
    double ra_sig = sig[idx_alvo].ra;
    double dec_sig_rad = para_radianos(sig[idx_alvo].declination);
    
    int sig_acima = verificar_se_acima_horizonte(ra_sig, dec_sig_rad, ramc, lat_geo_rad); 
    
    double sa_sig = __calcular_semi_arco(dec_sig_rad, lat_geo_rad, sig_acima);
    
    double proporcao_aspecto[] = {0.0, 0.66666667, -0.66666667, 1.0, -1.0, 1.33333333, -1.33333333, 2.0, 999.9, 999.9, 999.9, 999.9}; 
    char *simbolos_aspectos[] = {"☌", "⚹", "⚹", "□", "□", "△", "△", "☍", "∥", "∦", "R∥", "R∦"};

    for (int p = 0; p < prom_id; p++) {
        if (prom[p].type == PROM_POINT || 
            prom[p].type == PROM_ANGLE || 
            prom[p].type == PROM_PART  || 
            prom[p].type == PROM_TERM
        ) continue;

        if ((prom[p].type == PROM_ANTISCIUM || prom[p].type == PROM_CONTRANTISCIUM) && !ANT_PROM) continue;
        if ((prom[p].type == PROM_ANTISCIUM || prom[p].type == PROM_CONTRANTISCIUM) && strstr(prom[p].object, "🝴") != NULL) continue;
       
        // 2. Dados tridimensionais REAIS do Promissor
        double ra_prom = prom[p].ra; 
        double dec_prom_rad = para_radianos(prom[p].declination);
        int prom_acima = verificar_se_acima_horizonte(ra_prom, dec_prom_rad, ramc, lat_geo_rad);
        double sa_prom = __calcular_semi_arco(dec_prom_rad, lat_geo_rad, prom_acima);

        // ====================================================================
        // EXTRAÇÃO DINÂMICA DE COTAS VIA SISTEMA DE CASAS ATIVO (Topocêntrico/Placidus)
        // ====================================================================
        double cota_sig_orientada;
        if (sig[idx_alvo].type == PROM_CUSP) {
            int num_casa = sig[idx_alvo].house;
            cota_sig_orientada = obter_proporcao_polar_casa(num_casa);
        } else {
            cota_sig_orientada = calcular_cota_dinamica_sistema(sig[idx_alvo].house_pos);
        }

        double cota_prom_natal_dinamica = calcular_cota_dinamica_sistema(prom[p].house_pos);

        for (int s = 0; s < 2; s++) {            
            if (s == 0 && sentido == 1) continue; 
            if (s == 1 && sentido == 0) continue; 

            for (int a = 0; a < 12; a++) {
                if (!mundane_aspects[a]) continue;
                if (prom[p].type == PROM_TERM && a > 0) break;

                double arco = 0.0;
                            
                // Ângulos Horários Iniciais com Sinal
                double md_sig_com_sinal = ra_sig - ramc;
                if (md_sig_com_sinal > 180.0)  md_sig_com_sinal -= 360.0;
                if (md_sig_com_sinal < -180.0) md_sig_com_sinal += 360.0;
            
                double md_prom_com_sinal = ra_prom - ramc;
                if (md_prom_com_sinal > 180.0)  md_prom_com_sinal -= 360.0;
                if (md_prom_com_sinal < -180.0) md_prom_com_sinal += 360.0;
            
                // Projeção do Aspecto Longitudinal
                double md_aspecto_prom = md_prom_com_sinal;
                if (a < 8) {
                    md_aspecto_prom = md_prom_com_sinal + (proporcao_aspecto[a] * sa_prom);
                }
                if (md_aspecto_prom > 180.0)  md_aspecto_prom -= 360.0;
                if (md_aspecto_prom < -180.0) md_aspecto_prom += 360.0;
            
                int eh_angulo_angular = 0;
                double cota_espacial_fixa = 0.0;
            
                if (sig[idx_alvo].type == PROM_ANGLE) {
                    int num_casa = sig[idx_alvo].house;
                    eh_angulo_angular = 1;
                    cota_espacial_fixa = obter_proporcao_polar_casa(num_casa);                    
                }
                else if (sig[idx_alvo].type == PROM_CUSP) {
                    int num_casa = sig[idx_alvo].house;
                    if (num_casa == 1 || num_casa == 4 || num_casa == 7 || num_casa == 10) {
                        eh_angulo_angular = 1;
                        cota_espacial_fixa = obter_proporcao_polar_casa(num_casa);
                    }
                }
            
                // FORK DE ALGORITMO: PLACIDUS VS TOPOCÊNTRICO
                if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
                    double cota_sig_final = eh_angulo_angular ? cota_espacial_fixa : cota_sig_orientada;
                    
                    // Chamada da função interna atualizada
                    arco = calcular_arco_mundano_topocentrico_interno(ra_sig, dec_sig_rad, cota_sig_final,
                                                                     ra_prom, dec_prom_rad, cota_prom_natal_dinamica,
                                                                     proporcao_aspecto[a], a, s, ramc, lat_geografica);
                    
                    // VALIDAÇÃO DE ERRO TOPOCÊNTRICO: Aborta se o filtro de quadrante barrou o aspecto (-1.0)
                    if (arco < 0.0) {
                        continue;
                    }
                } 
                else {
                    // MOTOR PADRÃO: PLACIDUS SEMI-ARCO (Código Original Corrigido)
                    if (eh_angulo_angular) {
                        if (s == 0) { 
                            double md_destino = sa_prom * cota_espacial_fixa;
                            arco = md_aspecto_prom - md_destino;
                        } 
                        else if (s == 1) { 
                            double cota_aspecto_prom = md_aspecto_prom / sa_prom;
                            double md_destino = sa_sig * cota_aspecto_prom; 
                            arco = md_destino - md_sig_com_sinal;
                        }
                    }              
                    else {
                        if (sig[idx_alvo].type == PROM_CUSP && a > 0 && a < 8) continue;    
                        
                        // 1. PARALELOS E CONTRA-PARALELOS MUNDANOS SIMPLES (Apenas um se move)
                        if (a == 8 || a == 9) { 
                            if (s == 0) { 
                                double cota_alvo_mundo = (a == 8) ? cota_sig_orientada : -cota_sig_orientada;
                                double md_destino = sa_prom * cota_alvo_mundo; 
                                arco = md_prom_com_sinal - md_destino; 
                            } 
                            else if (s == 1) { 
                                double cota_alvo_conversa = (a == 8) ? cota_prom_natal_dinamica : -cota_prom_natal_dinamica; 
                                double md_destino = sa_sig * cota_alvo_conversa; 
                                arco = md_destino - md_sig_com_sinal; 
                            } 
                        } 
                        // 2. ASPECTOS LONGITUDINAIS CLÁSSICOS (Trígono, Quadratura, etc.)
                        else if (a < 8) { 
                            if (s == 0) { 
                                double md_destino = sa_prom * cota_sig_orientada; 
                                arco = md_aspecto_prom - md_destino; 
                            } else if (s == 1) { 
                                double cota_aspecto_prom = md_aspecto_prom / sa_prom; 
                                double md_destino = sa_sig * cota_aspecto_prom; 
                                arco = md_destino - md_sig_com_sinal; 
                            } 
                        }
                        // 3. RAPT PARALLEL E RAPT CONTRA-PARALELO (Ambos se movem - MOVIMENTO DUPLO)
                        else if (a == 10 || a == 11) {
                            double md_sig = md_sig_com_sinal;
                            double md_prom = md_prom_com_sinal;
                            
                            double delta_ra = ra_prom - ra_sig;
                            if (delta_ra > 180.0)  delta_ra -= 360.0;
                            if (delta_ra < -180.0) delta_ra += 360.0;
                            delta_ra = fabs(delta_ra);

                            int mesmos_lados_meridiano = ((md_sig >= 0 && md_prom >= 0) || (md_sig < 0 && md_prom < 0));

                            if (a == 10 && !mesmos_lados_meridiano) continue;
                            if (a == 11 && mesmos_lados_meridiano)  continue;

                            double sa_sig_efetivo = sa_sig;
                            double sa_prom_efetivo = sa_prom;

                            double md_sig_abs = fabs(md_sig);
                            double md_prom_abs = fabs(md_prom);
                            double hd_sig_abs = fabs(sa_sig_efetivo - md_sig_abs);
                            double hd_prom_abs = fabs(sa_prom_efetivo - md_prom_abs);

                            int focado_no_horizonte = ((hd_sig_abs + hd_prom_abs) < (md_sig_abs + md_prom_abs)) ? 1 : 0;

                            if (!focado_no_horizonte) {
                                if (s == 0) { // Direta: calcula o arco baseado na proporção do movimento duplo
                                    double dist_proporcional_prom = (sa_prom_efetivo * delta_ra) / (sa_sig_efetivo + sa_prom_efetivo);
                                    arco = md_prom_abs - dist_proporcional_prom;
                                } else { // Conversa
                                    double dist_proporcional_sig = (sa_sig_efetivo * delta_ra) / (sa_sig_efetivo + sa_prom_efetivo);
                                    arco = md_sig_abs - dist_proporcional_sig;
                                }
                            } else {
                                if (s == 0) {
                                    double dist_proporcional_prom = (sa_prom_efetivo * delta_ra) / (sa_sig_efetivo + sa_prom_efetivo);
                                    arco = hd_prom_abs - dist_proporcional_prom;
                                } else {
                                    double dist_proporcional_sig = (sa_sig_efetivo * delta_ra) / (sa_sig_efetivo + sa_prom_efetivo);
                                    arco = hd_sig_abs - dist_proporcional_sig;
                                }
                            }

                        }
                    }
                }

                                 
                if (arco < 0.0) arco += 360.0;
                arco = fmod(arco, 360.0);
      
           
                if (arco > 0.001 && arco <= MAX_AGE * 1.05) { 
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];
                    
                    d->sentido = s;

                    d->promissor_id = p;
                    d->aspecto_id = a;
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    strcpy(d->significador_name, sig[idx_alvo].object_name);
                    strcpy(d->significador_glifo, sig[idx_alvo].object);
                    d->promissor_type = prom[p].type;
                    
                    d->arco_graus = arco;
                    
                    // 1. Obter a chave/fator do sistema usando suas macros estáticas
                    double CHAVE = get_time_key(TIME_KEY, jd, arco);
                    if (CHAVE <= 0.0) CHAVE = NAIBOD_KEY; 

                    // 2. CÁLCULO UNIFICADO DA IDADE DO EVENTO (Em anos decimais contínuos)
                    d->idade_evento = arco / CHAVE;
            
                    double dias_decorridos = 0.0;

                    // 3. FORK DE TRATAMENTO DE TEMPO COM PRECISÃO DE ACORDO COM CADA MACRO
                    if (TIME_KEY == TIME_KEY_NAIBOD) {
                        dias_decorridos = d->idade_evento * 365.256363004; // Inverso exato de NAIBOD_KEY
                    }
                    else if (TIME_KEY == TIME_KEY_CARDANO) {
                        dias_decorridos = d->idade_evento * 364.847162454; // Inverso exato de CARDANO_KEY
                    }
                    else if (TIME_KEY == TIME_KEY_PTOLEMY) {
                        dias_decorridos = d->idade_evento * 365.0;         // Ano Civil de Ptolomeu
                    }
                    else if (TIME_KEY == TIME_KEY_PLACIDUS) {
                        dias_decorridos = d->idade_evento * 365.25;        // Ano Juliano Eclesiástico
                    }
                    else if (TIME_KEY == TIME_KEY_KEPLER) {
                        d->idade_evento = arco * CHAVE;
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
                    else if (TIME_KEY == TIME_KEY_TRUE_SOLAR_ARC_RA) {
                        d->idade_evento = arco * CHAVE;
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
                    else {                                
                        double ano_tropical_epoca = obter_ano_tropical_dinamico(jd);                        
                        dias_decorridos = d->idade_evento * ano_tropical_epoca;
                    }
            
                    // 4. Projeção Estrita no Dia Juliano e Conversão UTC via Swiss Ephemeris
                    double jd_evento = jd + dias_decorridos;
            
                    int ano_c, mes_c, dia_c, hora_c, min_c;
                    double sec_c;
                    swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);
            
                    d->ano_calendario = ano_c;
                    d->mes_calendario = mes_c;
                    d->dia_calendario = dia_c;
                                   
                    strcpy(d->tipo_direcao, _("Mundane"));
                    d->tipo_direcao_id = DIRECAO_MUNDANA;

                    qtd_direcoes++;
                    if (qtd_direcoes >= 600) goto fim_calculo;
                }
            } 
        }
    }                    

fim_calculo:
    qsort(lista_resultado, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);

    if (qtd_direcoes > 1) {
        int i_valido = 0;

        for (int i_atual = 1; i_atual < qtd_direcoes; i_atual++) {
            // Mudança sutil: Comparamos o atual com o último elemento que aceitamos como válido
            LinhaDirecao *d0 = &lista_resultado[i_valido];
            LinhaDirecao *d1 = &lista_resultado[i_atual];

            bool mesmo_promissor = (d0->promissor_id == d1->promissor_id);
            bool mesmo_sentido   = (d0->sentido == d1->sentido);
            
            double delta_arco = fabs(d1->arco_graus - d0->arco_graus);
            bool arco_identico = (delta_arco < 0.0001); // Use um epsilon pequeno explícito (ex: 0.0001)

            // Se o mesmo planeta, no mesmo sentido (Direto/Converso), atingir o mesmo arco geométrico...
            if (mesmo_promissor && mesmo_sentido && arco_identico) {
                continue; // Ignora o d1 (duplicado)
            }

            // Se não for duplicado, movemos o ponteiro de gravação
            i_valido++;
            if (i_valido != i_atual) {
                lista_resultado[i_valido] = *d1;
            }
        }

        qtd_direcoes = i_valido + 1;
    }

    return qtd_direcoes;

}


void display_primary_directions(PlotObject *plots, Promissor *sig, AspectMatrix *matrix, PontosHylegiacos pontos, int regente_dia, int regente_hora, char *nome_anareta, char *nome_senhor_da_casa8, int tipo_h_natal, int idx_hyleg_natal, bool mapa_retorno, double jd, int tipo_san, PlanetDignities *dig, double ramc, double lat, Promissor *prom) {
       
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 31;
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
    int idx_uranus = -1;
    int idx_neptune = -1;
    int idx_pluto = -1;
    int idx_north_node = -1;
    int idx_south_node = -1;
    int idx_dc = -1;
    int idx_ic = -1;

    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        if (sig[i].id == P_ASC - object_diff) idx_asc = i;
        else if (sig[i].id == P_MC - object_diff)  idx_mc = i; 
        else if (strcmp(sig[i].object_name, "SAN") == 0) idx_san = i; 
        else if (strcmp(sig[i].object_name, _("Part of Fortune")) == 0) idx_fortuna = i; 
        else if (sig[i].id == P_MERCURY) idx_mercury = i;
        else if (sig[i].id == P_VENUS) idx_venus = i;
        else if (sig[i].id == P_MARS) idx_mars = i;
        else if (sig[i].id == P_JUPITER) idx_jupiter = i;
        else if (sig[i].id == P_SATURN) idx_saturn = i;
        else if (show_modern_planets) {
            if (sig[i].id == P_URANUS) idx_uranus = i;
            else if (sig[i].id == P_NEPTUNE) idx_neptune = i;
            else if (sig[i].id == P_PLUTO) idx_pluto = i;
        }
        else if (sig[i].id == P_NORTH_NODE - object_diff) idx_north_node = i;
        else if (sig[i].id == P_SOUTH_NODE - object_diff) idx_south_node = i;
        else if (sig[i].id == P_DC - object_diff) idx_dc = i;
        else if (sig[i].id == P_IC - object_diff) idx_ic = i; 
    }

    if (!mapa_retorno) {
        if (tipo_h == H_SOL) idx_hileg = 0;
        else if (tipo_h == H_LUNA) idx_hileg = 1;
        else if (tipo_h == H_SAN) idx_hileg = P_SAN - object_diff;
        else if (tipo_h == H_ALMUTEN) idx_hileg = id_almuten_ref - 1;
        else if (tipo_h == H_ALMUTEN_HYL) idx_hileg = id_almuten_ref - 1;
        else {
            for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
                if (tipo_h == H_ASC && sig[i].id == P_ASC - object_diff) { idx_hileg = i; break; }
                else if (tipo_h == H_FORTUNA && sig[i].id == P_FORTUNA - object_diff) { idx_hileg = i; break; }
            }
        }
    }
    else {
        idx_hileg = idx_hyleg_natal;
    }

    int indices_significadores[TOTAL_SIGNIFICADORES];
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
    if (show_modern_planets) {
        indices_significadores[12] = idx_uranus;
        indices_significadores[13] = idx_neptune;
        indices_significadores[14] = idx_pluto;
        indices_significadores[15] = idx_north_node;
        indices_significadores[16] = idx_south_node;
        indices_significadores[17] = idx_dc;
        indices_significadores[18] = idx_ic;
    } else {
        indices_significadores[12] = idx_north_node;
        indices_significadores[13] = idx_south_node;
        indices_significadores[14] = idx_dc;
        indices_significadores[15] = idx_ic;
    }
    


    for (int i = 1; i <= 12; i++) {
        if (show_modern_planets) {
            indices_significadores[18 + i] = idx_ic + i;
        } else {
            indices_significadores[15 + i] = idx_ic + i;
        }
    }



    int seletor_alvo_atual = 0; 
    int scroll_offset = 0;
    int loop_interativo = 1;

    int max_linhas_exibicao = (table_height / 2) * 2 - 14;
    WINDOW *scroll_pad = newpad(2400, table_width - 8); 

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

    int sentido = 2;
    int tipo = 2;

    bool PART_DIRECTIONS = false;

    int mundane_aspects[12] = {1,1,1,1,1,1,1,1,1,1,1,1};

    while (loop_interativo) {
        werase(table_win);
        werase(scroll_pad);

        box(table_win, 0, 0);
        wattron(table_win, A_BOLD);
        const char *title = _(" Primary Directions ");
        mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 0, col_fechar, "[");
        mvwprintw(table_win, 0, col_fechar + 2, "]");
        wattroff(table_win, COLOR_PAIR(13));

        wattron(table_win, COLOR_PAIR(13) | A_BOLD);
        mvwprintw(table_win, 0, col_fechar + 1, "✖");
        wattroff(table_win, COLOR_PAIR(13) | A_BOLD);

        int idx_atual_calculo = indices_significadores[seletor_alvo_atual];

        
        int qtd_direcoes_zod = 0;
        int qtd_direcoes_mun = 0;
        int qtd_direcoes_asc = 0;
        
        LinhaDirecao cronograma_z[600];
        LinhaDirecao cronograma_m[600];
        LinhaDirecao cronograma_asc[600];

        memset(cronograma_asc, 0, sizeof(cronograma_asc));
        if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
            qtd_direcoes_asc = calcular_direcoes_zodiacais_topocentrico(sig, indices_significadores[3], cronograma_asc, jd, 2, prom, prom_id, ramc, lat, &qtd_direcoes_asc, PART_DIRECTIONS);
        }
        else {
            qtd_direcoes_asc = calcular_direcoes_zodiacais_geral(sig, indices_significadores[3], cronograma_asc, jd, 2, prom, PART_DIRECTIONS);
        }
        //qtd_direcoes_asc = calc_zodiacal_directions(&sig[indices_significadores[3]], indices_significadores[3], cronograma_asc, jd, ramc, lat, 2, prom, PART_DIRECTIONS);


        if (tipo != 1) {
            memset(cronograma_z, 0, sizeof(cronograma_z));
            if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
                qtd_direcoes_zod = calcular_direcoes_zodiacais_topocentrico(sig, idx_atual_calculo, cronograma_z, jd, sentido, prom, prom_id, ramc, lat, &qtd_direcoes_zod, PART_DIRECTIONS);
            }
            else {
                qtd_direcoes_zod = calcular_direcoes_zodiacais_geral(sig, idx_atual_calculo, cronograma_z, jd, sentido, prom, PART_DIRECTIONS);
            }
            //qtd_direcoes_zod = calc_zodiacal_directions(&sig[idx_atual_calculo], idx_atual_calculo, cronograma_z, jd, ramc, lat, sentido, prom, PART_DIRECTIONS);

        }
        if (tipo != 0) {   
            memset(cronograma_m, 0, sizeof(cronograma_m));            
            qtd_direcoes_mun = calc_mundane_directions(&sig[idx_atual_calculo], cronograma_m, jd, ramc, lat, sentido, prom, mundane_aspects);            
            //qtd_direcoes_mun = calcular_direcoes_mundanas_geral(sig, idx_atual_calculo, cronograma_m, jd, ramc, lat, sentido, prom, mundane_aspects);            
        }
        
        int qtd_direcoes_real = qtd_direcoes_zod + qtd_direcoes_mun; // guarda para depois
        int qtd_direcoes = qtd_direcoes_real;
        int qtd_direcoes_calculo = qtd_direcoes + qtd_direcoes_asc;


        LinhaDirecao *cronograma_a = (LinhaDirecao *)calloc(qtd_direcoes_calculo, sizeof(LinhaDirecao));

        

        int index = 0;
        if (cronograma_a) {    
            for (int i = 0; i < qtd_direcoes_asc; i++) {
                if (cronograma_asc[i].promissor_type == PROM_TERM) {
                    cronograma_a[index] = cronograma_asc[i];
                        
                    snprintf(cronograma_a[index].significador_glifo, 10, "%c", '0');
                    index++;
                }            
            }
            
            if (qtd_direcoes_zod > 0) {
                memcpy(&cronograma_a[index], cronograma_z, qtd_direcoes_zod * sizeof(LinhaDirecao));
                index += qtd_direcoes_zod;
            }
            if (qtd_direcoes_mun > 0) {
                memcpy(&cronograma_a[index], cronograma_m, qtd_direcoes_mun * sizeof(LinhaDirecao));
                index += qtd_direcoes_mun;
            }
                        
        }    
        qsort(cronograma_a, index, sizeof(LinhaDirecao), comparar_directions_por_idade_tipo_termo);

        // obter glifo e nome do regente do termo natal do ascendente //significador
        int regente_do_termo = get_term_ruler(sig[indices_significadores[3]].longitude);            
        char glifo_atual[10];
        char divisor_atual[30];
        snprintf(glifo_atual, sizeof(glifo_atual), "%s", planet_regent_symbols[regente_do_termo]);
        snprintf(divisor_atual, sizeof(divisor_atual), "%s", planet_regent_names[regente_do_termo]);

        qtd_direcoes = index;

        for (int i = 0; i < qtd_direcoes; i++) {
            if (cronograma_a[i].promissor_type == PROM_TERM && 
                cronograma_a[i].significador_glifo[0] == '0'
            ) {
                strcpy(divisor_atual, cronograma_a[i].promissor_name);
                int id_planeta = obter_id_planeta_por_nome(divisor_atual);
                strcpy(glifo_atual, obter_glifo_planeta_por_id(id_planeta));
            }

            strcpy(cronograma_a[i].divisor_name, divisor_atual);
            strcpy(cronograma_a[i].divisor_gliph, glifo_atual);
        }



        LinhaDirecao *cronograma = (LinhaDirecao *)calloc(qtd_direcoes_zod + qtd_direcoes_mun, sizeof(LinhaDirecao));

        index = 0;
        if (cronograma) {            
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].significador_glifo[0] == '0') {
                    continue;
                }
                
                cronograma[index] = cronograma_a[i];
                index++;
            }
        }
        qtd_direcoes = index;






        // index = 0;
        // if (tipo == 1 && sentido == 2) {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         if (cronograma_a[i].tipo_direcao_id == 0) {
        //             continue;
        //         }
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }
        // else if (tipo == 1 && sentido == 0) {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         if (cronograma_a[i].tipo_direcao_id == 0 || cronograma_a[i].sentido == 1) {
        //             continue;
        //         }                
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }
        // else if (tipo == 1 && sentido == 1) {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         if (cronograma_a[i].tipo_direcao_id == 0 || cronograma_a[i].sentido == 0) {
        //             continue;
        //         }                
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }
        // else if (sentido == 1) {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         if (cronograma_a[i].sentido == 0) {
        //             continue;
        //         }
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }
        // else if (sentido == 0) {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         if (cronograma_a[i].sentido == 1) {
        //             continue;
        //         }
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }
        // else {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }



        free(cronograma_a);




        // if (qtd_direcoes != qtd_direcoes_real) {
        //     show_alert_popup("Qtde de direções não bate!", "");
        // }

        if (scroll_offset > qtd_direcoes * 2 - max_linhas_exibicao) {
            scroll_offset = qtd_direcoes * 2 - max_linhas_exibicao;
        }
        if (scroll_offset < 0) scroll_offset = 0;

        wattron(table_win, A_BOLD);
        mvwprintw(table_win, 2, 4, _("Active Significator Target: "));
        wattroff(table_win, A_BOLD);

        wattron(table_win, A_BOLD | COLOR_PAIR(8));
        if (idx_atual_calculo != -1) {
            wprintw(table_win, "%s %s", sig[idx_atual_calculo].object, sig[idx_atual_calculo].object_name);
            if (seletor_alvo_atual == 0) wprintw(table_win, _(" [EMPHASIZED HYLEG]"));
        } else {
            wprintw(table_win, _("Point not calculated in this chart"));
        }
        wattroff(table_win, A_BOLD | COLOR_PAIR(8));

        wattron(table_win, A_ITALIC | A_BOLD);
        wprintw(table_win, _(" │ Directions: "));
        wattroff(table_win, A_BOLD);

        wattron(table_win, A_BOLD | COLOR_PAIR(7));
        if (tipo == 0) {
            wprintw(table_win, "Zod");            
        }
        else if (tipo == 1) {
            wprintw(table_win, "Mund");            
        }
        else if (tipo == 2) {
            wprintw(table_win, "Zod & Mund");            
        }
        wprintw(table_win, " │ ");

        if (sentido == 0) {
            wprintw(table_win, "Dir");            
        }
        else if (sentido == 1) {
            wprintw(table_win, "Conv");            
        }
        else if (sentido == 2) {
            wprintw(table_win, "Dir & Conv");            
        }
        wattroff(table_win, A_BOLD | A_ITALIC | COLOR_PAIR(7));


        wattron(table_win, A_DIM);
        mvwprintw(table_win, 2, table_width - 33, _("Use [←/→] Signif. [↑/↓] Scroll"));               
        wattroff(table_win, A_DIM);

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 4, 2, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        int col_idade = 0, col_ano = 16, col_mes = 21, col_dia = 24, col_dir = 33, col_arco = 67, col_tipo = 81, col_sen = 91, col_div = 102;

        wattron(table_win, A_BOLD | COLOR_PAIR(13));
        mvwprintw(table_win, 5, col_idade + 4, _("Age")); 
        mvwprintw(table_win, 5, col_ano + 4, _("Year"));
        mvwprintw(table_win, 5, col_mes + 3, _(" Mo"));
        mvwprintw(table_win, 5, col_dia + 4, _("Day"));
        mvwprintw(table_win, 5, col_dir + 4, _("Directional Event")); 
        mvwprintw(table_win, 5, col_arco + 4, _("Arc"));
        mvwprintw(table_win, 5, col_tipo + 4, _("Sphere"));
        mvwprintw(table_win, 5, col_sen + 4, _("Motion"));
        mvwprintw(table_win, 5, col_div + 4, _("Divisor"));
        wattroff(table_win, A_BOLD | COLOR_PAIR(13));

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 6, 2, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
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

                // DEBUG
                // wattron(table_win, A_DIM);
                // mvwprintw(table_win, 1, table_width - 20, " HD: %10.6f", d->sig_hd);
                // mvwprintw(table_win, 2, table_width - 20, " PM: %10.6f", d->sig_pm);
                // mvwprintw(table_win, 3, table_width - 20, "SAD: %10.6f", d->sig_sa.sad);        
                // wattroff(table_win, A_DIM);

                bool eh_termo = d->promissor_type == PROM_TERM;
                bool eh_antiscia = d->promissor_type == PROM_ANTISCIUM || d->promissor_type == PROM_CONTRANTISCIUM;

                char texto_evento[100];
                snprintf(texto_evento, sizeof(texto_evento), " %s%s%s %s %s → %s ", 
                         eh_termo ? _("Term") : "",
                         eh_termo ? " " : "",
                         d->promissor_glifo, d->promissor_name,
                         (d->promissor_type == PROM_TERM)?"":d->aspecto_symbol,
                         d->significador_glifo
                );              

                bool eh_aspecto_tenso = (strcmp(d->aspecto_symbol, "□") == 0 || strcmp(d->aspecto_symbol, "☍") == 0 || strcmp(d->aspecto_symbol, "∦") == 0 || strcmp(d->aspecto_symbol, "R∦") == 0);
                bool eh_conjuncao = (strcmp(d->aspecto_symbol, "☌") == 0);
                
                bool eh_marte   = (strcmp(d->promissor_name, _("Mars")) == 0);
                bool eh_saturno = (strcmp(d->promissor_name, _("Saturn")) == 0);
                bool eh_nodo_sul = (strcmp(d->promissor_name, _("South Node")) == 0);
                bool eh_malefico_essencial = (eh_marte || eh_saturno || eh_nodo_sul);
                
                bool eh_anareta      = (strcmp(d->promissor_name, nome_anareta) == 0);
                bool eh_senhor_casa8 = (strcmp(d->promissor_name, nome_senhor_da_casa8) == 0);
                //bool eh_anareta_ou_mortis = (eh_anareta || eh_senhor_casa8);

                bool eh_jupiter   = (strcmp(d->promissor_name, _("Jupiter")) == 0);
                bool eh_venus = (strcmp(d->promissor_name, _("Venus")) == 0);
                bool eh_nodo_norte = (strcmp(d->promissor_name, _("North Node")) == 0);

                bool eh_benefico_essencial = (eh_jupiter || eh_venus || eh_nodo_norte);

                int par_cor_ativo = COLOR_PAIR(13);
                int atributo_extra = A_NORMAL;

                if (eh_anareta) {
                    if (eh_aspecto_tenso || strcmp(d->aspecto_symbol, "☌") == 0) {
                        par_cor_ativo = COLOR_PAIR(36);
                        atributo_extra |= (A_REVERSE | A_BOLD);
                    } else {
                        par_cor_ativo = COLOR_PAIR(11); 
                        //atributo_extra = A_BOLD;
                    }
                }
                else if (eh_malefico_essencial && (eh_aspecto_tenso || eh_conjuncao)) {
                    par_cor_ativo = COLOR_PAIR(11); 
                    atributo_extra |= A_BOLD;
                }
                else if (eh_senhor_casa8 && eh_aspecto_tenso) {
                    par_cor_ativo = COLOR_PAIR(11); 
                    atributo_extra |= A_BOLD;
                }
                else if (!eh_malefico_essencial && eh_aspecto_tenso) {
                    par_cor_ativo = COLOR_PAIR(25);
                    atributo_extra |= A_REVERSE;
                }
                else if (eh_benefico_essencial) {
                    par_cor_ativo = COLOR_PAIR(12);
                    atributo_extra = A_DIM;      
                }
                else if (strcmp(d->aspecto_symbol, "☌") == 0) {
                    par_cor_ativo = COLOR_PAIR(7);
                    atributo_extra |= A_BOLD;
                }
                else if (!eh_aspecto_tenso) {
                    par_cor_ativo = COLOR_PAIR(8);
                    atributo_extra |= A_NORMAL;
                }

                if (eh_termo) {
                   atributo_extra |= A_UNDERLINE;
                }
                else if (eh_antiscia) {
                    atributo_extra |= A_DIM | A_ITALIC;
                } 


                wattron(scroll_pad, par_cor_ativo | atributo_extra);

                mvwprintw(scroll_pad, row_pad, col_idade, "%8.4f y", d->idade_evento);
                mvwprintw(scroll_pad, row_pad, col_ano, "%4d.", d->ano_calendario);
                mvwprintw(scroll_pad, row_pad, col_mes, "%02d.", d->mes_calendario);
                mvwprintw(scroll_pad, row_pad, col_dia, "%02d", d->dia_calendario);
                mvwprintw(scroll_pad, row_pad, col_dir, "%s", texto_evento);
                mvwprintw(scroll_pad, row_pad, col_arco, "%8.4f°", d->arco_graus);
                mvwprintw(scroll_pad, row_pad, col_tipo, "%s", d->tipo_direcao);
                mvwprintw(scroll_pad, row_pad, col_sen, "%s", (d->sentido == 0 ? _("Direct") : _("Converse")));
                mvwprintw(scroll_pad, row_pad, col_div, "%s %s", d->divisor_gliph, d->divisor_name);

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

        wattron(table_win, A_DIM | A_ITALIC);
        if (TIME_KEY < 5) {
            mvwprintw(table_win, table_height - 8, 4, _("Time Key: %s (1° of Equatorial Rotation = %6.4f Years). ε: Dynamic."), get_key_name(TIME_KEY), 1.0 / get_key(TIME_KEY));
        }
        else {
            mvwprintw(table_win, table_height - 8, 4, _("Time Key: %s (Dynamic). ε: Dynamic."), get_key_name(TIME_KEY));
        }
        
        
        if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
            if (tipo == 0) {
                mvwprintw(table_win, table_height - 7, 4, _("Topocentric Method: Zodiacal (Oblique Ascensions under the Pole)."));
            } else if (tipo == 1) {
                mvwprintw(table_win, table_height - 7, 4, _("Topocentric Method: Mundane (Continuous Local Poles)."));
            } else {
                mvwprintw(table_win, table_height - 7, 4, _("Topocentric Method: Zodiacal (Oblique Ascensions under the Pole) + Mundane (Continuous Local Poles)."));
            }
        } else {
            if (tipo == 0) {
                mvwprintw(table_win, table_height - 7, 4, _("Placidus Method: Zodiacal (Ecliptic Projection w/ Lat. Bianchini Method)."));
            } else if (tipo == 1) {
                mvwprintw(table_win, table_height - 7, 4, _("Placidus Method: Mundane (Proportional Semi-Arcs In Mundo)."));
            } else {
                mvwprintw(table_win, table_height - 7, 4, _("Placidus Method: Zodiacal (Ecliptic Projection w/ Lat.) + Mundane (Proportional Semi-Arcs)."));
            }
        }
        
        wattroff(table_win, A_ITALIC);

        if (linhas_reais_pad > max_linhas_exibicao) {
            wattron(table_win, COLOR_PAIR(8) | A_BOLD);
            mvwprintw(table_win, table_height - 5, 4, "%s %d-%d %s %d",
                _("Showing"),
                scroll_offset / 2 + 1, 
                ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                _("of"),
                qtd_direcoes                
            );
            wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
            mvwprintw(table_win, table_height - 4, 4, "%s", _("[↑/↓][PgUp/PgDn] Scroll │ [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));  
                
        } else {
            mvwprintw(table_win, table_height - 4, 4, _("Use [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));
        }
        mvwprintw(table_win, table_height - 3, 4, _("Mundane Aspects: [1] (☌ ⚹ □ △ ☍ ∥ ∦ R∥ R∦)    │ [2] (☌ △ ☍ ∥ ∦ R∥ R∦)    │ [3] (☌ □ △ ☍ ∥ ∦ R∥ R∦) │ [4] (☌ ⚹ □ △ ☍ )"));
        wattroff(table_win, A_DIM);

        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart ─ [P] Print to File"));

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
            case 'P':
            case 'p':
                print_directions_to_csv(cronograma, qtd_direcoes, METODO_CALCULO_ATIVO, TIME_KEY);
                break;
            case '1':
                for (int i = 0; i < 12; i++) mundane_aspects[i] = 1;
                break;
            case '2':
                for (int i = 0; i < 12; i++) {
                    if (i > 0 && i < 5) {
                        mundane_aspects[i] = 0; 
                    } else {
                        mundane_aspects[i] = 1;
                    }
                }
                break;
            case '3':
                for (int i = 0; i < 12; i++) {
                    if (i > 0 && i < 3) {
                        mundane_aspects[i] = 0; 
                    } else {
                        mundane_aspects[i] = 1;
                    }
                }
                break;
            case '4':
                for (int i = 0; i < 12; i++) {
                    if (i > 7) {
                        mundane_aspects[i] = 0; 
                    } else {
                        mundane_aspects[i] = 1;
                    }
                }
                break;
            case KEY_RIGHT:
                seletor_alvo_atual = (seletor_alvo_atual + 1) % (TOTAL_SIGNIFICADORES - object_diff);
                scroll_offset = 0;
                break;
            case KEY_LEFT:
                seletor_alvo_atual = (seletor_alvo_atual - 1 + TOTAL_SIGNIFICADORES - object_diff) % (TOTAL_SIGNIFICADORES - object_diff);
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
            case KEY_MOUSE: {
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
                            loop_interativo = 0;
                            break; // Sai do switch do mouse e fecha a janela
                        }
                    }           
                    
                    // 1. Descobre a coluna onde a barra é desenhada
                    int col_scrollbar_absoluta = getbegx(table_win) + (getmaxx(table_win) - 2);

                    // 2. Verifica se o clique do mouse ocorreu exatamente na coluna da barra de rolagem
                    if (event.x == col_scrollbar_absoluta) {
                        
                        // O seu offset_y passado na função foi 6. As setas estão posicionadas nas extremidades.
                        // Assumindo estrutura padrão: Seta Cima na linha 6, Canal da Barra da 7 até max, Seta Baixo no fim.
                        int linha_seta_cima = 6;
                        int linha_seta_baixo = 6 + max_linhas_exibicao; // Ajuste se a sua seta de baixo estiver em outra linha
                        int offset_inicio_barra = 6 + 1; 
                        int altura_canal_barra = max_linhas_exibicao - 1;

                        int max_scroll_y = (qtd_direcoes * 2) - max_linhas_exibicao;
                        if (max_scroll_y < 0) max_scroll_y = 0;

                        // ========================================================
                        // A: CLIQUE NA SETA PARA CIMA (▲) - ROLAGEM DE 2 EM 2
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
                                                ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                                                _("of"),
                                                qtd_direcoes                
                                            );
                                            wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
                                            mvwprintw(table_win, table_height - 4, 4, "%s", _("[↑/↓][PgUp/PgDn] Scroll │ [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));  
                                                
                                        }
                                        wnoutrefresh(table_win);
                                        prefresh(scroll_pad, scroll_offset, 0, start_y + 7, start_x + 4, fim_y_recorte, start_x + table_width - 5);
                                        doupdate();
                                    } else {
                                        scroll_offset = 0;
                                    }

                                    napms(125); // Confortável para as setas

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
                        // B: CLIQUE NA SETA PARA BAIXO (▼) - ROLAGEM DE 2 EM 2
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
                                                ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                                                _("of"),
                                                qtd_direcoes                
                                            );
                                            wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
                                            mvwprintw(table_win, table_height - 4, 4, "%s", _("[↑/↓][PgUp/PgDn] Scroll │ [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));  
                                                
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
                                            ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                                            _("of"),
                                            qtd_direcoes                
                                        );
                                        wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
                                        mvwprintw(table_win, table_height - 4, 4, "%s", _("[↑/↓][PgUp/PgDn] Scroll │ [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));  
                                            
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
        free(cronograma);
    }
    
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr);
    refresh();

    drag_and_drop_off();
}


void display_primary_directions_parts(Promissor *prom, char *nome_anareta, char *nome_senhor_da_casa8, ChartObject *obj, int num_objects, double *cusps, double jd, double ramc, double lat) {

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 31;
    int table_width = max_x - 10;
    int start_y = (max_y - table_height) / 2;
    int start_x = 5;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    keypad(table_win, TRUE); // Habilita o teclado para capturar as 4 setas

    ArabicPartCalculada lista_partes[MAX_PARTS] = {0};

    int qtd_partes = load_and_calculate_arabic_parts(obj, num_objects, cusps, lista_partes);

    int indices_significadores[qtd_partes];
    int idx_fortuna = 0;

    for (int i = 0; i < qtd_partes; i++) {
        if (strstr(lista_partes[i].name, "Fortune") != NULL    ||
            strstr(lista_partes[i].name, "Fortuna") != NULL    ||
            strstr(lista_partes[i].name, _("Fortune")) != NULL ||
            strstr(lista_partes[i].name, _("Part of Fortune")) != NULL ||
            strstr(lista_partes[i].name, "Pars Fortunae") != NULL ||
            strstr(lista_partes[i].name, _("Lot of Fortune")) != NULL ||
            strstr(lista_partes[i].name, "Lot of Fortune") != NULL ||
            strstr(lista_partes[i].name, "Lot da Fortuna") != NULL
        ) {
            idx_fortuna = i;
        }
        indices_significadores[i] = i;
    }

    Promissor *sig = (Promissor *)calloc(qtd_partes, sizeof(Promissor));


    char house_system_pd = HOUSE_SYSTEM;
    if (HOUSE_SYSTEM == 'E' || HOUSE_SYSTEM == 'W' || HOUSE_SYSTEM == 'M') {
        if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
            house_system_pd = 'T';
        }
        else {
            house_system_pd = 'P';
        }
    }

    for (int i = 0; i < qtd_partes; i++) {
        char abrev[10];
        get_part_abbreviation(lista_partes[i].name, abrev);
        
        snprintf(sig[i].object, 10, "%s", abrev);
        snprintf(sig[i].object_name, 64, "%s", lista_partes[i].name);
        sig[i].id = i;

        sig[i].longitude = lista_partes[i].longitude;            
        sig[i].latitude = 0.0; // Rigorosamente 0.0 na eclíptica
        
        double xx_in[3], xx_out[3];
        xx_in[0] = sig[i].longitude;
        xx_in[1] = sig[i].latitude;
        xx_in[2] = 1.0;

        double true_obliquity = get_obliquidade(jd);

        swe_cotrans(xx_in, xx_out, -true_obliquity);
        sig[i].declination = xx_out[1];
        sig[i].ra = xx_out[0];
        
        sig[i].house = get_house(sig[i].longitude, cusps);
        sig[i].type = PROM_PART;

        double x2[6];
        char serr[256];
        x2[0] = sig[i].longitude;
        x2[1] = sig[i].latitude;
        sig[i].house_pos = swe_house_pos(ramc, lat, true_obliquity, house_system_pd, x2, serr);        
    }


    int seletor_alvo_atual = 0; 
    int scroll_offset = 0; // Controla qual linha virtual será a primeira a aparecer na tela
    int loop_interativo = 1;

    // ────────────────────────────────────────────────────────────────────────
    // CRIAÇÃO DO PAD VIRTUAL DE ROLAGEM
    // ────────────────────────────────────────────────────────────────────────
    // Criamos um espaço de 180 linhas de altura (cabe qualquer volume de direções)
    int max_linhas_exibicao = (table_height / 2) * 2 - 14; // Espaço físico real na janela para os dados
    WINDOW *scroll_pad = newpad(2400, table_width - 8); 

    // Desenha sombra e frame fixo de fundo
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0); 
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);

    wbkgd(table_win, COLOR_PAIR(13) | FLAGS);
    wbkgd(scroll_pad, COLOR_PAIR(13) | FLAGS); 

    int sentido = 2;
    int tipo = 2;

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

    bool PART_DIRECTIONS = true;

    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(100);

    drag_and_drop_on();

    int mundane_aspects[12] = {1,1,1,1,1,1,1,1,1,1,1,1};

    while (loop_interativo) {
        // Limpa todas as estruturas gráficas antes de recalcular
        werase(table_win);
        werase(scroll_pad);

        box(table_win, 0, 0);
        
        wattron(table_win, A_BOLD);
        const char *title = _(" Primary Directions to Arabic Parts ");
        mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);

        wattron(table_win, COLOR_PAIR(13)); // Cor padrão para os colchetes
        mvwprintw(table_win, 0, col_fechar, "[");
        mvwprintw(table_win, 0, col_fechar + 2, "]");
        wattroff(table_win, COLOR_PAIR(13));

        wattron(table_win, COLOR_PAIR(13) | A_BOLD); // Cor de destaque (ex: Vermelho) para o X
        mvwprintw(table_win, 0, col_fechar + 1, "✖");
        wattroff(table_win, COLOR_PAIR(13) | A_BOLD);

        int idx_atual_calculo = indices_significadores[seletor_alvo_atual];

        int qtd_direcoes_zod = 0;
        int qtd_direcoes_mun = 0;
        int qtd_direcoes_for = 0;
        
        LinhaDirecao cronograma_z[600];
        LinhaDirecao cronograma_m[600];
        LinhaDirecao cronograma_for[600];

        memset(cronograma_for, 0, sizeof(cronograma_for));
        if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
            qtd_direcoes_for = calcular_direcoes_zodiacais_topocentrico(sig, indices_significadores[idx_fortuna], cronograma_for, jd, 2, prom, prom_id, ramc, lat, &qtd_direcoes_for, PART_DIRECTIONS);
        }
        else {
            qtd_direcoes_for = calcular_direcoes_zodiacais_geral(sig, indices_significadores[idx_fortuna], cronograma_for, jd, 2, prom, PART_DIRECTIONS);
        }
        //qtd_direcoes_for = calc_zodiacal_directions(&sig[indices_significadores[idx_fortuna]], indices_significadores[idx_fortuna], cronograma_for, jd, ramc, lat, 2, prom, PART_DIRECTIONS);


        if (tipo != 1) {
            memset(cronograma_z, 0, sizeof(cronograma_z));
            if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
                qtd_direcoes_zod = calcular_direcoes_zodiacais_topocentrico(sig, idx_atual_calculo, cronograma_z, jd, sentido, prom, prom_id, ramc, lat, &qtd_direcoes_zod, PART_DIRECTIONS);
            }
            else {
                qtd_direcoes_zod = calcular_direcoes_zodiacais_geral(sig, idx_atual_calculo, cronograma_z, jd, sentido, prom, PART_DIRECTIONS);
            }
            //qtd_direcoes_zod = calc_zodiacal_directions(&sig[idx_atual_calculo], idx_atual_calculo, cronograma_z, jd, ramc, lat, sentido, prom, PART_DIRECTIONS);

        }
        if (tipo != 0) {   
            memset(cronograma_m, 0, sizeof(cronograma_m));            
            qtd_direcoes_mun = calc_mundane_directions(&sig[idx_atual_calculo], cronograma_m, jd, ramc, lat, sentido, prom, mundane_aspects);            
            //qtd_direcoes_mun = calcular_direcoes_mundanas_geral(sig, idx_atual_calculo, cronograma_m, jd, ramc, lat, sentido, prom, mundane_aspects);            
        }

        // qtd_direcoes_for = calcular_direcoes_zodiacais_partes(lista_partes, qtd_partes, indices_significadores[idx_fortuna], cronograma_for, jd, 2, prom);

        // if (tipo != 1) {
        //     memset(cronograma_z, 0, sizeof(cronograma_z));
        //     qtd_direcoes_zod = calcular_direcoes_zodiacais_partes(lista_partes, qtd_partes, idx_atual_calculo, cronograma_z, jd, sentido, prom);
        // }
        // if (tipo != 0) {   
        //     memset(cronograma_m, 0, sizeof(cronograma_m));
        //     qtd_direcoes_mun = calcular_direcoes_mundanas_partes(lista_partes, idx_atual_calculo, cronograma_m, jd, ramc, lat, sentido, prom);
        // }
        int qtd_direcoes_real = qtd_direcoes_zod + qtd_direcoes_mun; // guarda para depois
        int qtd_direcoes = qtd_direcoes_real;
        int qtd_direcoes_calculo = qtd_direcoes + qtd_direcoes_for;




        LinhaDirecao *cronograma_a = (LinhaDirecao *)calloc(qtd_direcoes_calculo, sizeof(LinhaDirecao));

        


        int index = 0;
        if (cronograma_a) {
            for (int i = 0; i < qtd_direcoes_for; i++) {
                if (cronograma_for[i].promissor_type == PROM_TERM) {
                    cronograma_a[index] = cronograma_for[i];
                        
                    snprintf(cronograma_a[index].significador_glifo, 10, "%c", '0');
                    index++;
                }            
            }
            if (qtd_direcoes_zod > 0) {
                memcpy(&cronograma_a[index], cronograma_z, qtd_direcoes_zod * sizeof(LinhaDirecao));
                index += qtd_direcoes_zod;
            }
            if (qtd_direcoes_mun > 0) {
                memcpy(&cronograma_a[index], cronograma_m, qtd_direcoes_mun * sizeof(LinhaDirecao));
                index += qtd_direcoes_mun;
            }
            
        }
        qsort(cronograma_a, index, sizeof(LinhaDirecao), comparar_directions_por_idade_tipo_termo);

        // obter glifo e nome do regente do termo natal do significador
        //int regente_do_termo = get_term_ruler(lista_partes[indices_significadores[idx_fortuna]].longitude);            
        int regente_do_termo = get_term_ruler(sig[indices_significadores[idx_fortuna]].longitude);   
        char glifo_atual[10];
        char divisor_atual[30];
        snprintf(glifo_atual, sizeof(glifo_atual), "%s", planet_regent_symbols[regente_do_termo]);
        snprintf(divisor_atual, sizeof(divisor_atual), "%s", planet_regent_names[regente_do_termo]);

        qtd_direcoes = index;

        for (int i = 0; i < qtd_direcoes; i++) {
            if (cronograma_a[i].promissor_type == PROM_TERM && 
                cronograma_a[i].significador_glifo[0] == '0'
            ) {
                strcpy(divisor_atual, cronograma_a[i].promissor_name);
                int id_planeta = obter_id_planeta_por_nome(divisor_atual);
                strcpy(glifo_atual, obter_glifo_planeta_por_id(id_planeta));
            }

            strcpy(cronograma_a[i].divisor_name, divisor_atual);
            strcpy(cronograma_a[i].divisor_gliph, glifo_atual);
        }




        LinhaDirecao *cronograma = (LinhaDirecao *)calloc(qtd_direcoes_zod + qtd_direcoes_mun, sizeof(LinhaDirecao));




        index = 0;
        if (cronograma) {            
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].significador_glifo[0] == '0') {
                    continue;
                }
                
                cronograma[index] = cronograma_a[i];
                index++;
            }
        }
        qtd_direcoes = index;




        // index = 0;
        // if (tipo == 1 && sentido == 2) {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         if (cronograma_a[i].tipo_direcao_id == 0) {
        //             continue;
        //         }
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }
        // else if (tipo == 1 && sentido == 0) {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         if (cronograma_a[i].tipo_direcao_id == 0 || cronograma_a[i].sentido == 1) {
        //             continue;
        //         }                
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }
        // else if (tipo == 1 && sentido == 1) {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         if (cronograma_a[i].tipo_direcao_id == 0 || cronograma_a[i].sentido == 0) {
        //             continue;
        //         }                
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }
        // else if (sentido == 1) {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         if (cronograma_a[i].sentido == 0) {
        //             continue;
        //         }
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }
        // else if (sentido == 0) {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         if (cronograma_a[i].sentido == 1) {
        //             continue;
        //         }
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }
        // else {
        //     for (int i = 0; i < qtd_direcoes; i++) {
        //         if (cronograma_a[i].significador_glifo[0] == '0') {
        //             continue;
        //         }
        //         cronograma[index] = cronograma_a[i];
        //         index++;
        //     }
        //     qtd_direcoes = index;
        // }
        
        
        
        free(cronograma_a);




        // if (qtd_direcoes != qtd_direcoes_real) {
        //     show_alert_popup("Qtde de direções não bate!", "");
        // }

        // Garante que o scroll não vá para o vazio se trocarmos para um planeta com menos direções
        if (scroll_offset > qtd_direcoes * 2 - max_linhas_exibicao) {
            scroll_offset = qtd_direcoes * 2 - max_linhas_exibicao;
        }
        if (scroll_offset < 0) scroll_offset = 0;

        // --- RENDERIZAÇÃO DO CABEÇALHO FIXO ---
        mvwprintw(table_win, 2, 4, _("Active Significator Target: "));
        wattron(table_win, A_BOLD | COLOR_PAIR(8));
        // if (idx_atual_calculo != -1) {

        //     char abreviacao[4];
        //     get_part_abbreviation(lista_partes[idx_atual_calculo].name, abreviacao);
        

        //     wprintw(table_win, "%s - %s", abreviacao, lista_partes[idx_atual_calculo].name);
        // } else {
        //     wprintw(table_win, _("Point not calculated in this chart"));
        // }

        if (idx_atual_calculo != -1) {
            wprintw(table_win, "%s %s", sig[idx_atual_calculo].object, sig[idx_atual_calculo].object_name);
        } else {
            wprintw(table_win, _("Point not calculated in this chart"));
        }
        wattroff(table_win, A_BOLD | COLOR_PAIR(8));

        wattron(table_win, A_ITALIC);
        wprintw(table_win, _(" │ Directions: "));
        wattron(table_win, A_BOLD | COLOR_PAIR(7));
        if (tipo == 0) {
            wprintw(table_win, "Zod");            
        }
        else if (tipo == 1) {
            wprintw(table_win, "Mund");            
        }
        else if (tipo == 2) {
            wprintw(table_win, "Zod & Mund");            
        }
        wprintw(table_win, " │ ");

        if (sentido == 0) {
            wprintw(table_win, "Dir");            
        }
        else if (sentido == 1) {
            wprintw(table_win, "Conv");            
        }
        else if (sentido == 2) {
            wprintw(table_win, "Dir & Conv");            
        }
        wattroff(table_win, A_BOLD | A_ITALIC | COLOR_PAIR(7));

        wattron(table_win, A_DIM);
        mvwprintw(table_win, 2, table_width - 33, _("Use [←/→] Signif. [↑/↓] Scroll"));
        wattroff(table_win, A_DIM);

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 4, 2, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        // Colunas Alinhadas Fixas (Mapeadas a partir de 0 para casar com as coordenadas do Pad)
        int col_idade = 0, col_ano = 16, col_mes = 21, col_dia = 24, col_dir = 33, col_arco = 67, col_tipo = 81, col_sen = 91, col_div = 102;

        wattron(table_win, A_BOLD | COLOR_PAIR(13));
        mvwprintw(table_win, 5, col_idade + 4, _("Age")); 
        mvwprintw(table_win, 5, col_ano + 4, _("Year"));
        mvwprintw(table_win, 5, col_mes + 3, _(" Mo"));
        mvwprintw(table_win, 5, col_dia + 4, _("Day"));
        mvwprintw(table_win, 5, col_dir + 4, _("Directional Event")); 
        mvwprintw(table_win, 5, col_arco + 4, _("Arc"));
        mvwprintw(table_win, 5, col_tipo + 4, _("Sphere"));
        mvwprintw(table_win, 5, col_sen + 4, _("Motion"));
        mvwprintw(table_win, 5, col_div + 4, _("Divisor"));
        wattroff(table_win, A_BOLD | COLOR_PAIR(13));

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 6, 2, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
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

                bool eh_termo = d->promissor_type == PROM_TERM;
                bool eh_antiscia = d->promissor_type == PROM_ANTISCIUM || d->promissor_type == PROM_CONTRANTISCIUM;

                char texto_evento[100];
                snprintf(texto_evento, sizeof(texto_evento), " %s%s%s %s %s → %s ", 
                         eh_termo ? _("Term") : "",
                         eh_termo ? " " : "",
                         d->promissor_glifo, d->promissor_name,
                         (d->promissor_type == PROM_TERM)?"":d->aspecto_symbol,
                         d->significador_glifo
                );

                bool eh_aspecto_tenso = (strcmp(d->aspecto_symbol, "□") == 0 || strcmp(d->aspecto_symbol, "☍") == 0 || strcmp(d->aspecto_symbol, "∦") == 0 || strcmp(d->aspecto_symbol, "R∦") == 0);
                bool eh_conjuncao = (strcmp(d->aspecto_symbol, "☌") == 0);
                
                bool eh_marte   = (strcmp(d->promissor_name, _("Mars")) == 0);
                bool eh_saturno = (strcmp(d->promissor_name, _("Saturn")) == 0);
                bool eh_nodo_sul = (strcmp(d->promissor_name, _("South Node")) == 0);
                bool eh_malefico_essencial = (eh_marte || eh_saturno || eh_nodo_sul);
                
                bool eh_anareta      = (strcmp(d->promissor_name, nome_anareta) == 0);
                bool eh_senhor_casa8 = (strcmp(d->promissor_name, nome_senhor_da_casa8) == 0);
                //bool eh_anareta_ou_mortis = (eh_anareta || eh_senhor_casa8);

                bool eh_jupiter   = (strcmp(d->promissor_name, _("Jupiter")) == 0);
                bool eh_venus = (strcmp(d->promissor_name, _("Venus")) == 0);
                bool eh_nodo_norte = (strcmp(d->promissor_name, _("North Node")) == 0);

                bool eh_benefico_essencial = (eh_jupiter || eh_venus || eh_nodo_norte);

                int par_cor_ativo = COLOR_PAIR(13);
                int atributo_extra = A_NORMAL;

                if (eh_anareta) {
                    if (eh_aspecto_tenso || strcmp(d->aspecto_symbol, "☌") == 0) {
                        par_cor_ativo = COLOR_PAIR(36);
                        atributo_extra |= (A_REVERSE | A_BOLD);
                    } else {
                        par_cor_ativo = COLOR_PAIR(11); 
                        //atributo_extra = A_BOLD;
                    }
                }
                else if (eh_malefico_essencial && (eh_aspecto_tenso || eh_conjuncao)) {
                    par_cor_ativo = COLOR_PAIR(11); 
                    atributo_extra |= A_BOLD;
                }
                else if (eh_senhor_casa8 && eh_aspecto_tenso) {
                    par_cor_ativo = COLOR_PAIR(11); 
                    atributo_extra |= A_BOLD;
                }
                else if (!eh_malefico_essencial && eh_aspecto_tenso) {
                    par_cor_ativo = COLOR_PAIR(25);
                    atributo_extra |= A_REVERSE;
                }
                else if (eh_benefico_essencial) {
                    par_cor_ativo = COLOR_PAIR(12);
                    atributo_extra = A_DIM;      
                }
                else if (strcmp(d->aspecto_symbol, "☌") == 0) {
                    par_cor_ativo = COLOR_PAIR(7);
                    atributo_extra |= A_BOLD;
                }
                else if (!eh_aspecto_tenso) {
                    par_cor_ativo = COLOR_PAIR(8);
                    atributo_extra |= A_NORMAL;
                }

                if (eh_termo) {
                    atributo_extra |= A_UNDERLINE;
                 }
                 else if (eh_antiscia) {
                     atributo_extra |= A_DIM | A_ITALIC;
                 }

                wattron(scroll_pad, par_cor_ativo | atributo_extra);

                mvwprintw(scroll_pad, row_pad, col_idade, "%8.4f y", d->idade_evento);
                mvwprintw(scroll_pad, row_pad, col_ano, "%4d.", d->ano_calendario);
                mvwprintw(scroll_pad, row_pad, col_mes, "%02d.", d->mes_calendario);
                mvwprintw(scroll_pad, row_pad, col_dia, "%02d", d->dia_calendario);
                mvwprintw(scroll_pad, row_pad, col_dir, "%s", texto_evento);
                mvwprintw(scroll_pad, row_pad, col_arco, "%8.4f°", d->arco_graus);
                mvwprintw(scroll_pad, row_pad, col_tipo, "%s", d->tipo_direcao);
                mvwprintw(scroll_pad, row_pad, col_sen, "%s", (d->sentido == 0 ? _("Direct") : _("Converse")));
                mvwprintw(scroll_pad, row_pad, col_div, "%s %s", d->divisor_gliph, d->divisor_name);

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

        wattron(table_win, A_DIM | A_ITALIC);
        if (TIME_KEY < 5) {
            mvwprintw(table_win, table_height - 8, 4, _("Time Key: %s (1° of Equatorial Rotation = %6.4f Years). ε: Dynamic."), get_key_name(TIME_KEY), 1.0 / get_key(TIME_KEY));
        }
        else {
            mvwprintw(table_win, table_height - 8, 4, _("Time Key: %s (Dynamic). ε: Dynamic."), get_key_name(TIME_KEY));
        }
        
        
        if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
            if (tipo == 0) {
                mvwprintw(table_win, table_height - 7, 4, _("Topocentric Method: Zodiacal (Oblique Ascensions under the Pole)."));
            } else if (tipo == 1) {
                mvwprintw(table_win, table_height - 7, 4, _("Topocentric Method: Mundane (Continuous Local Poles)."));
            } else {
                mvwprintw(table_win, table_height - 7, 4, _("Topocentric Method: Zodiacal (Oblique Ascensions under the Pole) + Mundane (Continuous Local Poles)."));
            }
        } else {
            if (tipo == 0) {
                mvwprintw(table_win, table_height - 7, 4, _("Placidus Method: Zodiacal (Ecliptic Projection w/ Lat. Bianchini Method)."));
            } else if (tipo == 1) {
                mvwprintw(table_win, table_height - 7, 4, _("Placidus Method: Mundane (Proportional Semi-Arcs In Mundo)."));
            } else {
                mvwprintw(table_win, table_height - 7, 4, _("Placidus Method: Zodiacal (Ecliptic Projection w/ Lat.) + Mundane (Proportional Semi-Arcs)."));
            }
        }
        
        wattroff(table_win, A_ITALIC);

        if (linhas_reais_pad > max_linhas_exibicao) {
            wattron(table_win, COLOR_PAIR(8) | A_BOLD);
            mvwprintw(table_win, table_height - 5, 4, "%s %d-%d %s %d",
                _("Showing"),
                scroll_offset / 2 + 1, 
                ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                _("of"),
                qtd_direcoes                
            );
            wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
            mvwprintw(table_win, table_height - 4, 4, "%s", _("[↑/↓][PgUp/PgDn] Scroll │ [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));  
                
        } else {
            mvwprintw(table_win, table_height - 4, 4, _("Use [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));
        }
        mvwprintw(table_win, table_height - 3, 4, _("Mundane Aspects: [1] (☌ ⚹ □ △ ☍ ∥ ∦ R∥ R∦)    │ [2] (☌ △ ☍ ∥ ∦ R∥ R∦)    │ [3] (☌ □ △ ☍ ∥ ∦ R∥ R∦) │ [4] (☌ ⚹ □ △ ☍ )"));
        wattroff(table_win, A_DIM);

        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart ─ [P] Print to File"));

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
            case 'P':
            case 'p':
                print_directions_to_csv(cronograma, qtd_direcoes, METODO_CALCULO_ATIVO, TIME_KEY);
                break;
            case '1':
                for (int i = 0; i < 12; i++) mundane_aspects[i] = 1;
                break;
            case '2':
                for (int i = 0; i < 12; i++) {
                    if (i > 0 && i < 5) {
                        mundane_aspects[i] = 0; 
                    } else {
                        mundane_aspects[i] = 1;
                    }
                }
                break;
            case '3':
                for (int i = 0; i < 12; i++) {
                    if (i > 0 && i < 3) {
                        mundane_aspects[i] = 0; 
                    } else {
                        mundane_aspects[i] = 1;
                    }
                }
                break;
            case '4':
                for (int i = 0; i < 12; i++) {
                    if (i > 7) {
                        mundane_aspects[i] = 0; 
                    } else {
                        mundane_aspects[i] = 1;
                    }
                }
                break;
            case KEY_RIGHT:
                seletor_alvo_atual = (seletor_alvo_atual + 1) % qtd_partes;
                scroll_offset = 0;
                break;
            case KEY_LEFT:
                seletor_alvo_atual = (seletor_alvo_atual - 1 + qtd_partes) % qtd_partes;
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
            case KEY_MOUSE: {
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
                            loop_interativo = 0;
                            break; // Sai do switch do mouse e fecha a janela
                        }
                    }           
                    
                    // 1. Descobre a coluna onde a barra é desenhada
                    int col_scrollbar_absoluta = getbegx(table_win) + (getmaxx(table_win) - 2);

                    // 2. Verifica se o clique do mouse ocorreu exatamente na coluna da barra de rolagem
                    if (event.x == col_scrollbar_absoluta) {
                        
                        // O seu offset_y passado na função foi 6. As setas estão posicionadas nas extremidades.
                        // Assumindo estrutura padrão: Seta Cima na linha 6, Canal da Barra da 7 até max, Seta Baixo no fim.
                        int linha_seta_cima = 6;
                        int linha_seta_baixo = 6 + max_linhas_exibicao; // Ajuste se a sua seta de baixo estiver em outra linha
                        int offset_inicio_barra = 6 + 1; 
                        int altura_canal_barra = max_linhas_exibicao - 1;

                        int max_scroll_y = (qtd_direcoes * 2) - max_linhas_exibicao;
                        if (max_scroll_y < 0) max_scroll_y = 0;

                        // ========================================================
                        // A: CLIQUE NA SETA PARA CIMA (▲) - ROLAGEM DE 2 EM 2
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
                                                ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                                                _("of"),
                                                qtd_direcoes                
                                            );
                                            wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
                                            mvwprintw(table_win, table_height - 4, 4, "%s", _("[↑/↓][PgUp/PgDn] Scroll │ [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));  
                                                
                                        }
                                        wnoutrefresh(table_win);
                                        prefresh(scroll_pad, scroll_offset, 0, start_y + 7, start_x + 4, fim_y_recorte, start_x + table_width - 5);
                                        doupdate();
                                    } else {
                                        scroll_offset = 0;
                                    }

                                    napms(125); // Confortável para as setas

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
                        // B: CLIQUE NA SETA PARA BAIXO (▼) - ROLAGEM DE 2 EM 2
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
                                                ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                                                _("of"),
                                                qtd_direcoes                
                                            );
                                            wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
                                            mvwprintw(table_win, table_height - 4, 4, "%s", _("[↑/↓][PgUp/PgDn] Scroll │ [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));  
                                                
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
                                            ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                                            _("of"),
                                            qtd_direcoes                
                                        );
                                        wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
                                        mvwprintw(table_win, table_height - 4, 4, "%s", _("[↑/↓][PgUp/PgDn] Scroll │ [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));  
                                            
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
        free(cronograma);
    }
    
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr);
    refresh();

    drag_and_drop_off();
}



void print_directions_to_csv(LinhaDirecao *dir, int qtd_direcoes, MetodoDirecaoGlobal method, int time_key) {
    
    desativar_arrasto_mouse();

    char file_name[120];
    memset(file_name, 0, sizeof(file_name));

    set_file_name(file_name, sizeof(file_name));    
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/export/%s.csv", CONFIG_PATH, file_name);

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        show_alert_popup(_("Error attempting to save directions to file!"), file_path);
        ativar_arrasto_mouse();
        flushinp();
        return;
    }

    fprintf(file, _("Age;Date;Promissor Glyph;Promissor Name;Event;Significator Glyph;Significator Name;Arc;Sphere;Motion;Divisor Glyph;Divisor Name\n"));

    for (int i = 0; i < qtd_direcoes; i++) {
        LinhaDirecao *d = &dir[i];

        fprintf(file, "%8.4f;%02d/%02d/%04d;%s;%s;%s;%s;%s;%8.4f;%s;%s;%s;%s\n",
                       d->idade_evento,
                       d->dia_calendario,
                       d->mes_calendario,
                       d->ano_calendario,
                       d->promissor_glifo,
                       d->promissor_name,
                       d->aspecto_symbol,
                       d->significador_glifo,
                       d->significador_name,
                       d->arco_graus,
                       (d->tipo_direcao_id == 0 ? "Z" : "M"),
                       (d->sentido == 0 ? "D" : "C"),
                       d->divisor_gliph,
                       d->divisor_name
        );
    }

    fclose(file);
    char file_csv[128];
    snprintf(file_csv, sizeof(file_csv), "%s.csv", file_name);
    show_alert_popup(_("Directions exported to file:"), file_csv);

    char sig_glyph[10];
    char sig_name[64];

    snprintf(sig_glyph, 10, "%s", dir[1].significador_glifo);
    snprintf(sig_name, 64, "%s", dir[1].significador_name);

    char info_path[512];
    snprintf(info_path, sizeof(info_path), "%s/export/%s.info", CONFIG_PATH, file_name);

    FILE *info = fopen(info_path, "w");
    if (info == NULL) {
        show_alert_popup(_("Error attempting to save directions metadata to file!"), info_path);
        ativar_arrasto_mouse();
        flushinp();
        return;
    }

    fprintf(info, "%s: %s\n", _("Primary Directions Calculation Method"), (method == METODO_TOPOCENTRICO ? _("Tpocentric Method") : _("Placidus Proportional Semi-Arcs Method")));
    fprintf(info, "%s: %s\n", _("Time Key"), get_key_name(time_key));
    fprintf(info, "%s: %s - %s\n", _("Active Significator Target"), sig_glyph, sig_name);
    fprintf(info, "%s: %s\n", _("csv File:"), file_csv);
    fprintf(info, "%s: %d\n", _("Total Directions:"), qtd_direcoes);
    fclose(info);

    char file_info[128];
    snprintf(file_info, sizeof(file_info), "%s.info", file_name);
    show_alert_popup(_("Directions metadata exported to file:"), file_info);


    ativar_arrasto_mouse();
    flushinp();
}