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


// Retorna o ID da Swiss Ephemeris baseado no nome do objeto do seu software
// Retorna -1 para pontos que não possuem nós orbitais (Ângulos, Fortuna, etc.)
int obter_swiss_ephemeris_id(const char *object) {
    if (strcmp(object, "☉") == 0) {
        return SE_SUN;
    }
    if (strcmp(object, "☽") == 0) {
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



// Função auxiliar para calcular a latitude dinâmica de qualquer planeta em uma longitude alvo
double calcular_latitude_dinamica_bianchini(double jd, const char *object, double lon_aspecto) {
    int se_id = obter_swiss_ephemeris_id(object);
    
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
    int32 método_nodo = SE_NODBIT_MEAN; // SE_NODBIT_MEAN = 1 (Nodos Médios, padrão astrológico)

    // Chamada oficial da Swiss Ephemeris com os 9 argumentos corretos e os tipos alinhados
    if (swe_nod_aps_ut(jd, se_id, flags_nodos, método_nodo, xnasc, xndsc, xperi, xaphel, serr) < 0) {
        return 0.0; // Fallback caso ocorra algum erro interno na biblioteca
    }

    // Índices oficiais da Swiss Ephemeris para os arrays de nodos/apsides:
    // [0] = Longitude
    // [1] = Latitude
    // [4] = Inclinação orbital em relação à eclíptica
    double lon_nodo = xnasc[0];     // Longitude do Nó Ascendente (Ω)
    double inclinacao = xnasc[4];   // Inclinação Orbital (i)

    // CORREÇÃO PARA O NÓ SUL: 
    if (strcmp(object, "☋") == 0) {
        lon_nodo = fmod(lon_nodo + 180.0, 360.0);
    }

    // Diferença angular entre a longitude do aspecto e o Nó Ascendente corrigido do planeta
    double dist_nodo_rad = para_radianos(lon_aspecto - lon_nodo);
    double inc_rad = para_radianos(inclinacao);

    // Fórmula de Bianchini: sin(lat) = sin(inc) * sin(λ_aspecto - Ω)
    double sin_lat_correto = sin(inc_rad) * sin(dist_nodo_rad);
    double lat_dinamica = para_graus(asin(sin_lat_correto));

    // Se for o Nó Sul físico do planeta, a latitude é invertida em relação ao plano norte
    if (strcmp(object, "☋") == 0) {
        lat_dinamica = -lat_dinamica;
    }

    return lat_dinamica;
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
int calcular_direcoes_zodiacais_geral(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, double jd, int sentido, Promissor *prom) {

    int qtd_direcoes = 0;
    int object_diff = show_modern_planets ? 0 : 3;

    //if (idx_alvo < 0 || idx_alvo >= NUM_OBJECTS) return 0;

    // Calcula a Ascensão Reta baseada na coordenada do ponto alvo escolhido
    double ra_significador = sig[idx_alvo].ra; //calcular_ra(plots[idx_alvo].longitude, plots[idx_alvo].declination, jd);

    double angulos_aspectos[] = {0.0, 60.0, 90.0, 120.0, 180.0};
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍"};

    //double epsilon = 0.000001;

    for (int p = 0; p < prom_id; p++) {
        for (int s = 0; s < 2; s++) {
            if ((p == idx_alvo && p < NUM_OBJECTS - object_diff - ((show_modern_planets)?5:4)) || prom[p].type == PROM_POINT || prom[p].type == PROM_ANGLE) continue; // Um ponto não direciona a si mesmo
            
            for (int a = 0; a < 5; a++) {

                if (prom[p].type == PROM_TERM && a > 0) break; // apenas conjunções para termos

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

                double lat_calculada = calcular_latitude_dinamica_bianchini(jd, prom[p].object, lon_aspecto);

                double xx[3];
                double xequat[3];

                xx[0] = lon_aspecto;   
                xx[1] = lat_calculada; 
                xx[2] = 1.0;           

                swe_cotrans(xx, xequat, -get_obliquidade(jd)); 

                double ra_aspecto = xequat[0];  // ÍNDICE CORRETO: [0] para Ascensão Reta
                //double dec_aspecto = xequat[1]; // Opcional: [1] para se precisar da Declinação dinâmica


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
                if (arco > 0.0 && arco <= MAX_AGE * 1.05) {
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];

                    d->sentido = s;
                    
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    
                    // Salva o nome e glifo do Significador Alvo atual
                    strcpy(d->significador_name, sig[idx_alvo].object_name);
                    strcpy(d->significador_glifo, sig[idx_alvo].object);
                    
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
                    d->tipo_direcao_id = DIRECAO_ZODIACAL;

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


// Retorna 1 se o planeta estiver ACIMA do horizonte, e 0 se estiver ABAIXO
int verificar_se_acima_horizonte(double ra_planeta, double dec_planeta_rad, double ramc, double lat_geo_rad) {
    // 1. Calcula a menor Distância Meridiana em relação ao MC (Superior) de forma circular segura
    double md_mc = fabs(ra_planeta - ramc);
    if (md_mc > 180.0) md_mc = 360.0 - md_mc;

    // 2. Calcula o Semi-Arco Diurno (SAD) exato via cossenos
    double tan_lat = tan(lat_geo_rad);
    double tan_dec = tan(dec_planeta_rad);
    double cos_sad = -tan_lat * tan_dec;

    // Proteção rigorosa contra regiões circumpolares (Sol da meia-noite / Noite polar)
    if (cos_sad >= 1.0)  return 0; // Nunca nasce (Sempre abaixo do horizonte)
    if (cos_sad <= -1.0) return 1; // Nunca se põe (Sempre acima do horizonte)

    double sad_graus = acos(cos_sad) * (180.0 / M_PI);

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




int calcular_direcoes_mundanas_geral(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promissor *prom) {
    int qtd_direcoes = 0;
    double lat_geo_rad = para_radianos(lat_geografica);

    //if (idx_alvo < 0 || idx_alvo >= NUM_OBJECTS) return 0;

    // 1. Dados tridimensionais REAIS do Significador (Alvo)
    double ra_sig = sig[idx_alvo].ra; //calcular_ra(plots[idx_alvo].longitude, plots[idx_alvo].declination, jd);
    double dec_sig_rad = para_radianos(sig[idx_alvo].declination);
    
    // Determinar se o significador está acima/abaixo do horizonte natal
    int sig_acima = verificar_se_acima_horizonte(ra_sig, dec_sig_rad, ramc, lat_geo_rad); 
    
    double sa_sig = __calcular_semi_arco(dec_sig_rad, lat_geo_rad, sig_acima);
    //double md_sig = __calcular_distancia_meridiana(ra_sig, ramc, sig_acima);
    //double cota_mundana_sig = md_sig / sa_sig;

    // Multiplicadores para os aspectos mundanos
    //double mult_aspectos[] = {0.0, 0.333333, 0.5, 0.666667, 1.0}; // Conjunção, Sextil, Quadratura, Trígono, Oposição
    double proporcao_aspecto[] = {0.0, 0.66666667, 1.0, 1.33333333, 2.0}; 
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍"};

    for (int p = 0; p < prom_id; p++) {
        if (prom[p].type == PROM_POINT || prom[p].type == PROM_ANGLE || prom[p].type == PROM_PART) continue;

        //if (prom[p].type == PROM_TERM) continue;
        
        // 2. Dados tridimensionais REAIS do Promissor
        double ra_prom = prom[p].ra; 
        double dec_prom_rad = para_radianos(prom[p].declination);
        
        // CORREÇÃO: Verificação astrométrica para o promissor também!
        int prom_acima = verificar_se_acima_horizonte(ra_prom, dec_prom_rad, ramc, lat_geo_rad);

        double sa_prom = __calcular_semi_arco(dec_prom_rad, lat_geo_rad, prom_acima);
        //double md_prom = __calcular_distancia_meridiana(ra_prom, ramc, prom_acima);

        for (int s = 0; s < 2; s++) { // 0 = Direta, 1 = Conversa
            
            // FILTRO CRÍTICO DE SENTIDO: Se o usuário filtrou por um sentido específico, pula o outro
            if (s == 0 && sentido == 1) continue; // Usuário quer apenas conversas (1), pula a direta (0)
            if (s == 1 && sentido == 0) continue; // Usuário quer apenas diretas (0), pula a conversa (1)

            // Multiplicador de distância em CASAS MUNDANAS completas
            // Conjunção=0, Sextil=2, Quadratura=3, Trígono=4, Oposição=6 casas de distância espacial
            //double casas_aspecto[] = {0.0, 2.0, 3.0, 4.0, 6.0};
            // Conjunção=0, Sextil=0.6666, Quadratura=1.0, Trígono=1.3333, Oposição=2.0
            //char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍"};

            for (int a = 0; a < 5; a++) {
                if (prom[p].type == PROM_TERM && a > 0) break; // apenas conjunções para termos

                double arco = 0.0;
                            
                // 1. Ângulos Horários com Sinal (Leste Negativo / Oeste Positivo)
                double md_sig_com_sinal = ra_sig - ramc;
                if (md_sig_com_sinal > 180.0)  md_sig_com_sinal -= 360.0;
                if (md_sig_com_sinal < -180.0) md_sig_com_sinal += 360.0;
                double cota_sig_orientada = md_sig_com_sinal / sa_sig;
            
                double md_prom_com_sinal = ra_prom - ramc;
                if (md_prom_com_sinal > 180.0)  md_prom_com_sinal -= 360.0;
                if (md_prom_com_sinal < -180.0) md_prom_com_sinal += 360.0;
            
                // 2. Mapeamento Real do Aspecto Mundano na Esfera (Avanço no sentido horário)
                // O aspecto desloca a posição do promissor somando frações do seu semi-arco
                double md_aspecto_prom = md_prom_com_sinal + (proporcao_aspecto[a] * sa_prom);
                
                // Normalização estrita do ângulo horário do aspecto (-180 a +180)
                if (md_aspecto_prom > 180.0)  md_aspecto_prom -= 360.0;
                if (md_aspecto_prom < -180.0) md_aspecto_prom += 360.0;
            
                // =========================================================================
                // 3. CÁLCULO DOS ARCOS DE DIREÇÃO MUNDANA
                // =========================================================================
                if (s == 0) { // === DIREÇÃO DIRETA ===
                    double md_destino = sa_prom * cota_sig_orientada;
                    arco = md_aspecto_prom - md_destino;
                } 
                else if (s == 1) { // === DIREÇÃO CONVERSA ===
                    // Na conversa, a cota proporcional do aspecto do promissor puxa o significador
                    double cota_aspecto_prom = md_aspecto_prom / sa_prom;
                    double md_destino = sa_sig * cota_aspecto_prom;
                    arco = md_destino - md_sig_com_sinal;
                }
            
                // =========================================================================
                // 4. NORMALIZAÇÃO FINAL DO MOVIMENTO PRIMÁRIO (0 a 360)
                // =========================================================================
                if (arco < 0.0) arco += 360.0;
                arco = fmod(arco, 360.0);
            
                // Filtra arcos de idade humana viável (0 a 150 anos)
                if (arco > 0.001 && arco <= MAX_AGE * 1.05) { // tolerânciazinha de borda
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];
                    
                    d->sentido = s;
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    strcpy(d->significador_name, sig[idx_alvo].object_name);
                    strcpy(d->significador_glifo, sig[idx_alvo].object);
                    d->promissor_type = prom[p].type;
                    
                    // 1. Calcula o arco e a idade do evento usando a SUA chave equatorial
                    d->arco_graus = arco;
                    d->idade_evento = arco / NAIBOD_KEY; // Usa 1.014646
            
                    // 2. Transforma a idade em dias de forma perfeitamente proporcional
                    // Naibod estabelece que 1 ano de idade = 1 Ano Tropical médio (365.242199 dias)
                    double dias_decorridos = d->idade_evento * 365.242199;
            
                    // 3. Calcula o Dia Juliano do evento somando ao JD natal
                    double jd_evento = jd + dias_decorridos;
            
                    // 4. Converte o Dia Juliano para data UTC (Swisseph gerencia calendários)
                    int ano_c, mes_c, dia_c, hora_c, min_c;
                    double sec_c;
                    swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);
            
                    // 5. Alimenta a estrutura com as datas corrigidas
                    d->ano_calendario = ano_c;
                    d->mes_calendario = mes_c;
                    d->dia_calendario = dia_c;
                                   
                    strcpy(d->tipo_direcao, _("Mundane"));
                    d->tipo_direcao_id = DIRECAO_MUNDANA;

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


void display_primary_directions(PlotObject *plots, Promissor *sig, AspectMatrix *matrix, PontosHylegiacos pontos, int regente_dia, int regente_hora, char *nome_anareta, char *nome_senhor_da_casa8, int tipo_h_natal, int idx_hyleg_natal, bool mapa_retorno, double jd, int tipo_san, PlanetDignities *dig, double ramc, double lat, Promissor *prom) {
       
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
    int idx_uranus = -1;
    int idx_neptune = -1;
    int idx_pluto = -1;
    int idx_north_node = -1;
    int idx_south_node = -1;
    int idx_dc = -1;
    int idx_ic = -1;

    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        if (sig[i].id == P_ASC - object_diff) idx_asc = i;
        if (sig[i].id == P_MC - object_diff)  idx_mc = i; 
        if (strcmp(sig[i].object_name, "SAN") == 0) idx_san = i; 
        if (strcmp(sig[i].object_name, _("Part of Fortune")) == 0) idx_fortuna = i; 
        if (sig[i].id == P_MERCURY) idx_mercury = i;
        if (sig[i].id == P_VENUS) idx_venus = i;
        if (sig[i].id == P_MARS) idx_mars = i;
        if (sig[i].id == P_JUPITER) idx_jupiter = i;
        if (sig[i].id == P_SATURN) idx_saturn = i;
        if (show_modern_planets) {
            if (sig[i].id == P_URANUS) idx_uranus = i;
            if (sig[i].id == P_NEPTUNE) idx_neptune = i;
            if (sig[i].id == P_PLUTO) idx_pluto = i;
        }
        if (sig[i].id == P_NORTH_NODE - object_diff) idx_north_node = i;
        if (sig[i].id == P_SOUTH_NODE - object_diff) idx_south_node = i;
        if (sig[i].id == P_DC - object_diff) idx_dc = i;
        if (sig[i].id == P_IC - object_diff) idx_ic = i; 
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
                if (tipo_h == H_FORTUNA && sig[i].id == P_FORTUNA - object_diff) { idx_hileg = i; break; }
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

    int max_linhas_exibicao = table_height - 13;
    WINDOW *scroll_pad = newpad(1200, table_width - 8); 

    // Desenha sombra e frame fixo de fundo
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0); 
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);
   
    wbkgd(table_win, COLOR_PAIR(13) | FLAGS);
    wbkgd(scroll_pad, COLOR_PAIR(13) | FLAGS); 

    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED, NULL);

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
            qtd_direcoes_zod = calcular_direcoes_zodiacais_geral(sig, idx_atual_calculo, cronograma_z, jd, sentido, prom);
        }
        if (tipo != 0) {   
            memset(cronograma_m, 0, sizeof(cronograma_m));
            qtd_direcoes_mun = calcular_direcoes_mundanas_geral(sig, idx_atual_calculo, cronograma_m, jd, ramc, lat, sentido, prom);
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
        qsort(cronograma, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade_tipo_termo);

        // obter glifo e nome do regente do termo natal do significador
        int regente_do_termo = get_term_ruler(sig[idx_atual_calculo].longitude);            
        char glifo_atual[10];
        char divisor_atual[30];
        snprintf(glifo_atual, sizeof(glifo_atual), "%s", planet_regent_symbols[regente_do_termo]);
        snprintf(divisor_atual, sizeof(divisor_atual), "%s", planet_regent_names[regente_do_termo]);

        for (int i = 0; i < qtd_direcoes; i++) {
            if (cronograma[i].promissor_type == PROM_TERM && 
                cronograma[i].tipo_direcao_id == DIRECAO_ZODIACAL &&
                cronograma[i].sentido == DIRECT
            ) {
                strcpy(divisor_atual, cronograma[i].promissor_name);
                int id_planeta = obter_id_planeta_por_nome(divisor_atual);
                strcpy(glifo_atual, obter_glifo_planeta_por_id(id_planeta));
            }

            strcpy(cronograma[i].divisor_name, divisor_atual);
            strcpy(cronograma[i].divisor_gliph, glifo_atual);
        }


        if (scroll_offset > qtd_direcoes * 2 - max_linhas_exibicao) {
            scroll_offset = qtd_direcoes * 2 - max_linhas_exibicao;
        }
        if (scroll_offset < 0) scroll_offset = 0;

        mvwprintw(table_win, 2, 4, _("Active Significator Target: "));
        wattron(table_win, A_BOLD | COLOR_PAIR(8));
        if (idx_atual_calculo != -1) {
            wprintw(table_win, "%s %s", sig[idx_atual_calculo].object, sig[idx_atual_calculo].object_name);
            if (seletor_alvo_atual == 0) wprintw(table_win, _(" [EMPHASIZED HYLEG]"));
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
        mvwprintw(table_win, 2, table_width - 32, _("Use [←/→] Signif. [↑/↓] Scroll"));
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
        mvwprintw(table_win, 5, col_arco + 4, _("Arc (Equat.)"));
        mvwprintw(table_win, 5, col_tipo + 4, _("Method"));
        mvwprintw(table_win, 5, col_sen + 4, _("Direction"));
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

                bool eh_termo = d->promissor_type == PROM_TERM;

                char texto_evento[100];
                snprintf(texto_evento, sizeof(texto_evento), " %s%s%s %s %s → %s ", 
                         eh_termo ? _("Term") : "",
                         eh_termo ? " " : "",
                         d->promissor_glifo, d->promissor_name,
                         (d->promissor_type == PROM_TERM)?"":d->aspecto_symbol,
                         d->significador_glifo);

                

                bool eh_aspecto_tenso = (strcmp(d->aspecto_symbol, "□") == 0 || strcmp(d->aspecto_symbol, "☍") == 0);
                bool eh_conjuncao = (strcmp(d->aspecto_symbol, "☌") == 0);

                bool eh_marte   = (strcmp(d->promissor_name, _("Mars")) == 0);
                bool eh_saturno = (strcmp(d->promissor_name, _("Saturn")) == 0);
                bool eh_nodo_sul = (strcmp(d->promissor_name, _("South Node")) == 0);
                bool eh_malefico_essencial = (eh_marte || eh_saturno || eh_nodo_sul);
                
                bool eh_anareta      = (strcmp(d->promissor_name, nome_anareta) == 0);
                bool eh_senhor_casa8 = (strcmp(d->promissor_name, nome_senhor_da_casa8) == 0);
                bool eh_anareta_ou_mortis = (eh_anareta || eh_senhor_casa8);

                bool eh_jupiter   = (strcmp(d->promissor_name, _("Jupiter")) == 0);
                bool eh_venus = (strcmp(d->promissor_name, _("Venus")) == 0);
                bool eh_nodo_norte = (strcmp(d->promissor_name, _("North Node")) == 0);

                bool eh_benefico_essencial = (eh_jupiter || eh_venus || eh_nodo_norte);

                int par_cor_ativo = COLOR_PAIR(13);
                int atributo_extra = A_NORMAL;

                if (eh_anareta_ou_mortis) {
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

                wattron(scroll_pad, par_cor_ativo | atributo_extra);

                mvwprintw(scroll_pad, row_pad, col_idade, "%8.4f y", d->idade_evento);
                mvwprintw(scroll_pad, row_pad, col_ano, "%4d.", d->ano_calendario);
                mvwprintw(scroll_pad, row_pad, col_mes, "%02d.", d->mes_calendario);
                mvwprintw(scroll_pad, row_pad, col_dia, "%02d", d->dia_calendario);
                mvwprintw(scroll_pad, row_pad, col_dir, "%s", texto_evento);
                mvwprintw(scroll_pad, row_pad, col_arco, "%05.2f°", d->arco_graus);
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
        mvwprintw(table_win, table_height - 7, 2, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        wattron(table_win, A_DIM | A_ITALIC);
        mvwprintw(table_win, table_height - 6, 4, _("Time Key: Naibod Rate (1 Year = 1.0146° of Equatorial Rotation). ε: Dynamic."));
        if (tipo == 0) {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Zodiacal with Real Latitude (Method Placidus)."));
        } else if (tipo == 1) {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Mundane proportional to Semi-Arcs."));
        } else {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Mixed Systems (Zodiacal w/ Latitude + Mundane proportional to Semi-Arcs)."));
        }
        wattroff(table_win, A_ITALIC);

        // Exibe um indicador visual de paginação se houver mais linhas abaixo ou acima
        if (linhas_reais_pad > max_linhas_exibicao) {
            mvwprintw(table_win, table_height - 3, 4, "%s %d-%d %s %d%s%s",
                _("[↑/↓] [PgUp/PgDn] Scroll (Showing"),
                scroll_offset / 2 + 1, 
                ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                _("of"),
                qtd_direcoes,
                _(") │ [←/→] Change Target"),
                _(" │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));
        } else {
            mvwprintw(table_win, table_height - 3, 4, _("Use [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));
        }
        wattroff(table_win, A_DIM);

        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));

        int flag = 0;
        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
        wattron(table_win, COLOR_PAIR(28) | flag);
        desenhar_scrollbar(table_win, scroll_offset, linhas_reais_pad, max_linhas_exibicao, 6);
        wattroff(table_win, COLOR_PAIR(28) | flag);

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
                    // 1. Descobre a coluna onde a barra é desenhada (usando a mesma lógica da sua função)
                    int col_scrollbar_absoluta = getbegx(table_win) + (getmaxx(table_win) - 2);

                    // 2. Verifica se o clique do mouse ocorreu exatamente na coluna da barra de rolagem
                    if (event.x == col_scrollbar_absoluta) {
                        
                        // 3. Descobre a linha clicada em relação ao início da janela 'table_win'
                        int linha_clique_janela = event.y - getbegy(table_win);
                        
                        // O seu offset_y passado na função foi 6. A área útil da barra começa na linha seguinte (7)
                        int offset_inicio_barra = 6 + 1; 
                        
                        // Calcula qual "degrau" da barra o usuário clicou (0 até max_linhas_exibicao - 1)
                        int linha_clique_barra = linha_clique_janela - offset_inicio_barra;

                        // 4. Verifica se o clique ocorreu dentro dos limites verticais da barra de rolagem
                        if (linha_clique_barra >= 0 && linha_clique_barra < max_linhas_exibicao) {
                            
                            // Calcula o limite máximo que o scroll_offset pode atingir
                            int max_scroll_y = (qtd_direcoes * 2) - max_linhas_exibicao;
                            if (max_scroll_y < 0) max_scroll_y = 0;

                            if (max_linhas_exibicao > 1 && max_scroll_y > 0) {
                                // Mapeia proporcionalmente a linha clicada para o novo offset de dados
                                int novo_offset = (linha_clique_barra * max_scroll_y) / (max_linhas_exibicao - 1);
                                
                                // Como o seu sistema avança de 2 em 2 linhas (par/ímpar devido aos dados),
                                // arredondamos para o número par mais próximo para não quebrar o layout da tabela
                                novo_offset = (novo_offset / 2) * 2;

                                // Garante que o valor respeite as barreiras de limite
                                if (novo_offset < 0) novo_offset = 0;
                                if (novo_offset > max_scroll_y) novo_offset = max_scroll_y;

                                scroll_offset = novo_offset;
                            }
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
}



int calcular_direcoes_zodiacais_partes(ArabicPartCalculada *parts, int qtd_partes, int idx_alvo, LinhaDirecao *lista_resultado, double jd, int sentido, Promissor *prom) {
    int qtd_direcoes = 0;

    if (idx_alvo < 0 || idx_alvo >= qtd_partes) return 0;

    // Calcula a Ascensão Reta baseada na coordenada do ponto alvo escolhido

    double dec_out, ra_out;
    calc_declination_ra_point(jd, parts[idx_alvo].longitude, &ra_out, &dec_out);
    double ra_significador = ra_out; //calcular_ra(parts[idx_alvo].longitude, NAN, jd);

    double angulos_aspectos[] = {0.0, 60.0, 90.0, 120.0, 180.0};
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍"};

    for (int p = 0; p < prom_id; p++) {
        if (prom[p].type == PROM_POINT || prom[p].type == PROM_ANGLE || prom[p].type == PROM_PART) continue;

        for (int s = 0; s < 2; s++) {
            for (int a = 0; a < 5; a++) {
                
                if (prom[p].type == PROM_TERM && a > 0) break; // apenas conjunções para termos

                double lon_aspecto;
                
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

                // Calcula a latitude dinâmica passando diretamente a string com o nome do objeto
                double lat_calculada = calcular_latitude_dinamica_bianchini(jd, prom[p].object, lon_aspecto);

                double xx[3];
                double xequat[3];

                xx[0] = lon_aspecto;   
                xx[1] = lat_calculada; 
                xx[2] = 1.0;           

                swe_cotrans(xx, xequat, -get_obliquidade(jd)); 

                double ra_aspecto = xequat[0];  // ÍNDICE CORRETO: [0] para Ascensão Reta
                //double dec_aspecto = xequat[1]; // Opcional: [1] para se precisar da Declinação dinâmica


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
                if (arco > 0.0 && arco <= MAX_AGE * 1.05) {
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
                    d->tipo_direcao_id = DIRECAO_ZODIACAL;

                    qtd_direcoes++;
                    if (qtd_direcoes >= 300) return qtd_direcoes;
                }
            }
        }
    }

    qsort(lista_resultado, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);

    return qtd_direcoes;
}




void display_primary_directions_parts(Promissor *prom, char *nome_anareta, char *nome_senhor_da_casa8, ChartObject *obj, int num_objects, double *cusps, double jd, double ramc, double lat) {

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

    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED, NULL);

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
            qtd_direcoes_zod = calcular_direcoes_zodiacais_partes(lista_partes, qtd_partes, idx_atual_calculo, cronograma_z, jd, sentido, prom);
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
        qsort(cronograma, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade_tipo_termo);

        // obter glifo e nome do regente do termo natal do significador
        int regente_do_termo = get_term_ruler(lista_partes[idx_atual_calculo].longitude);            
        char glifo_atual[10];
        char divisor_atual[30];
        snprintf(glifo_atual, sizeof(glifo_atual), "%s", planet_regent_symbols[regente_do_termo]);
        snprintf(divisor_atual, sizeof(divisor_atual), "%s", planet_regent_names[regente_do_termo]);

        for (int i = 0; i < qtd_direcoes; i++) {
            if (cronograma[i].promissor_type == PROM_TERM && 
                cronograma[i].tipo_direcao_id == DIRECAO_ZODIACAL &&
                cronograma[i].sentido == DIRECT
            ) {
                strcpy(divisor_atual, cronograma[i].promissor_name);
                int id_planeta = obter_id_planeta_por_nome(divisor_atual);
                strcpy(glifo_atual, obter_glifo_planeta_por_id(id_planeta));
            }

            strcpy(cronograma[i].divisor_name, divisor_atual);
            strcpy(cronograma[i].divisor_gliph, glifo_atual);
        }
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
        mvwprintw(table_win, 2, table_width - 32, _("Use [←/→] Signif. [↑/↓] Scroll"));
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
        mvwprintw(table_win, 5, col_arco + 4, _("Arc (Equat.)"));
        mvwprintw(table_win, 5, col_tipo + 4, _("Method"));
        mvwprintw(table_win, 5, col_sen + 4, _("Direction"));
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

                char texto_evento[100];
                snprintf(texto_evento, sizeof(texto_evento), " %s%s%s %s → %s ", 
                         eh_termo ? _("Term") : "",
                         eh_termo ? " " : "",
                         d->promissor_glifo, // d->promissor_name,
                         (d->promissor_type == PROM_TERM)?"":d->aspecto_symbol,
                         d->significador_glifo);


                bool eh_aspecto_tenso = (strcmp(d->aspecto_symbol, "□") == 0 || strcmp(d->aspecto_symbol, "☍") == 0);
                bool eh_conjuncao = (strcmp(d->aspecto_symbol, "☌") == 0);

                bool eh_marte   = (strcmp(d->promissor_name, _("Mars")) == 0);
                bool eh_saturno = (strcmp(d->promissor_name, _("Saturn")) == 0);
                bool eh_nodo_sul = (strcmp(d->promissor_name, _("South Node")) == 0);
                bool eh_malefico_essencial = (eh_marte || eh_saturno || eh_nodo_sul);
                
                bool eh_anareta      = (strcmp(d->promissor_name, nome_anareta) == 0);
                bool eh_senhor_casa8 = (strcmp(d->promissor_name, nome_senhor_da_casa8) == 0);
                bool eh_anareta_ou_mortis = (eh_anareta || eh_senhor_casa8);

                bool eh_jupiter   = (strcmp(d->promissor_name, _("Jupiter")) == 0);
                bool eh_venus = (strcmp(d->promissor_name, _("Venus")) == 0);
                bool eh_nodo_norte = (strcmp(d->promissor_name, _("North Node")) == 0);

                bool eh_benefico_essencial = (eh_jupiter || eh_venus || eh_nodo_norte);

                int par_cor_ativo = COLOR_PAIR(13);
                int atributo_extra = A_NORMAL;

                if (eh_anareta_ou_mortis) {
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
                
                wattron(scroll_pad, par_cor_ativo | atributo_extra);

                mvwprintw(scroll_pad, row_pad, col_idade, "%8.4f y", d->idade_evento);
                mvwprintw(scroll_pad, row_pad, col_ano, "%4d.", d->ano_calendario);
                mvwprintw(scroll_pad, row_pad, col_mes, "%02d.", d->mes_calendario);
                mvwprintw(scroll_pad, row_pad, col_dia, "%02d", d->dia_calendario);
                mvwprintw(scroll_pad, row_pad, col_dir, "%s", texto_evento);
                mvwprintw(scroll_pad, row_pad, col_arco, "%05.2f°", d->arco_graus);
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
        mvwprintw(table_win, table_height - 7, 2, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(13));

        wattron(table_win, A_DIM | A_ITALIC);
        mvwprintw(table_win, table_height - 6, 4, _("Time Key: Naibod Rate (1 Year = 1.0146° of Equatorial Rotation). ε: Dynamic."));
        if (tipo == 0) {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Zodiacal with Real Latitude (Method Placidus)."));
        } else if (tipo == 1) {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Mundane proportional to Semi-Arcs."));
        } else {
            mvwprintw(table_win, table_height - 5, 4, _("Aspects: Mixed Systems (Zodiacal w/ Latitude + Mundane proportional to Semi-Arcs)."));
        }
        wattroff(table_win, A_ITALIC);

        // Exibe um indicador visual de paginação se houver mais linhas abaixo ou acima
        if (linhas_reais_pad > max_linhas_exibicao) {
            mvwprintw(table_win, table_height - 3, 4, "%s %d-%d %s %d%s%s",
                _("[↑/↓] [PgUp/PgDn] Scroll (Showing"),
                scroll_offset / 2 + 1, 
                ((scroll_offset + max_linhas_exibicao) > qtd_direcoes * 2) ? qtd_direcoes : (scroll_offset / 2 + max_linhas_exibicao / 2),
                _("of"),
                qtd_direcoes,
                _(") │ [←/→] Change Target"),
                _(" │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));
        } else {
            mvwprintw(table_win, table_height - 3, 4, _("Use [←/→] Change Target │ [C] Conv [D] Dir [A] All │ [Z] Zod [M] Mund [B] Both"));
        }
        wattroff(table_win, A_DIM);

        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));

        int flag = 0;
        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
        wattron(table_win, COLOR_PAIR(28) | flag);
        desenhar_scrollbar(table_win, scroll_offset, qtd_direcoes * 2, max_linhas_exibicao, 6);
        wattroff(table_win, COLOR_PAIR(28) | flag);

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

            case KEY_MOUSE: {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    // 1. Descobre a coluna onde a barra é desenhada (usando a mesma lógica da sua função)
                    int col_scrollbar_absoluta = getbegx(table_win) + (getmaxx(table_win) - 2);

                    // 2. Verifica se o clique do mouse ocorreu exatamente na coluna da barra de rolagem
                    if (event.x == col_scrollbar_absoluta) {
                        
                        // 3. Descobre a linha clicada em relação ao início da janela 'table_win'
                        int linha_clique_janela = event.y - getbegy(table_win);
                        
                        // O seu offset_y passado na função foi 6. A área útil da barra começa na linha seguinte (7)
                        int offset_inicio_barra = 6 + 1; 
                        
                        // Calcula qual "degrau" da barra o usuário clicou (0 até max_linhas_exibicao - 1)
                        int linha_clique_barra = linha_clique_janela - offset_inicio_barra;

                        // 4. Verifica se o clique ocorreu dentro dos limites verticais da barra de rolagem
                        if (linha_clique_barra >= 0 && linha_clique_barra < max_linhas_exibicao) {
                            
                            // Calcula o limite máximo que o scroll_offset pode atingir
                            int max_scroll_y = (qtd_direcoes * 2) - max_linhas_exibicao;
                            if (max_scroll_y < 0) max_scroll_y = 0;

                            if (max_linhas_exibicao > 1 && max_scroll_y > 0) {
                                // Mapeia proporcionalmente a linha clicada para o novo offset de dados
                                int novo_offset = (linha_clique_barra * max_scroll_y) / (max_linhas_exibicao - 1);
                                
                                // Como o seu sistema avança de 2 em 2 linhas (par/ímpar devido aos dados),
                                // arredondamos para o número par mais próximo para não quebrar o layout da tabela
                                novo_offset = (novo_offset / 2) * 2;

                                // Garante que o valor respeite as barreiras de limite
                                if (novo_offset < 0) novo_offset = 0;
                                if (novo_offset > max_scroll_y) novo_offset = max_scroll_y;

                                scroll_offset = novo_offset;
                            }
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
}



int calcular_direcoes_mundanas_partes(ArabicPartCalculada *parts, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promissor *prom) {
    int qtd_direcoes = 0;
    double lat_geo_rad = para_radianos(lat_geografica);

    if (idx_alvo < 0 || idx_alvo >= NUM_OBJECTS) return 0;

    // 1. Dados tridimensionais REAIS do Significador (Alvo)
    double dec_out, ra_out;
    calc_declination_ra_point(jd, parts[idx_alvo].longitude, &ra_out, &dec_out);
    double ra_sig = ra_out;

    //double ra_sig = calcular_ra(parts[idx_alvo].longitude, NAN, jd);
    double dec_sig_rad = para_radianos(dec_out); //para_radianos(calc_declination_mathematical_point(jd, parts[idx_alvo].longitude));
    
    // Determinar se o significador está acima/abaixo do horizonte natal
    int sig_acima = verificar_se_acima_horizonte(ra_sig, dec_sig_rad, ramc, lat_geo_rad); 
    
    double sa_sig = __calcular_semi_arco(dec_sig_rad, lat_geo_rad, sig_acima);
    //double md_sig = __calcular_distancia_meridiana(ra_sig, ramc, sig_acima);
    //double cota_mundana_sig = md_sig / sa_sig;

    // Multiplicadores para os aspectos mundanos
    //double mult_aspectos[] = {0.0, 0.333333, 0.5, 0.666667, 1.0}; // Conjunção, Sextil, Quadratura, Trígono, Oposição
    double proporcao_aspecto[] = {0.0, 0.66666667, 1.0, 1.33333333, 2.0}; 
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍"};

    for (int p = 0; p < prom_id; p++) {
        if (prom[p].type == PROM_POINT || prom[p].type == PROM_ANGLE || prom[p].type == PROM_PART) continue;

        //if (prom[p].type == PROM_TERM) continue;
        
        // 2. Dados tridimensionais REAIS do Promissor
        double ra_prom = prom[p].ra; 
        double dec_prom_rad = para_radianos(prom[p].declination);
        
        // CORREÇÃO: Verificação astrométrica para o promissor também!
        int prom_acima = verificar_se_acima_horizonte(ra_prom, dec_prom_rad, ramc, lat_geo_rad);

        double sa_prom = __calcular_semi_arco(dec_prom_rad, lat_geo_rad, prom_acima);
        //double md_prom = __calcular_distancia_meridiana(ra_prom, ramc, prom_acima);
        for (int s = 0; s < 2; s++) { // 0 = Direta, 1 = Conversa
            
            // FILTRO CRÍTICO DE SENTIDO: Se o usuário filtrou por um sentido específico, pula o outro
            if (s == 0 && sentido == 1) continue; // Usuário quer apenas conversas (1), pula a direta (0)
            if (s == 1 && sentido == 0) continue; // Usuário quer apenas diretas (0), pula a conversa (1)

            // Multiplicador de distância em CASAS MUNDANAS completas
            // Conjunção=0, Sextil=2, Quadratura=3, Trígono=4, Oposição=6 casas de distância espacial
            //double casas_aspecto[] = {0.0, 2.0, 3.0, 4.0, 6.0}; 

            //char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍"};

            for (int a = 0; a < 5; a++) {
                if (prom[p].type == PROM_TERM && a > 0) break; // apenas conjunções para termos

                double arco = 0.0;
                            
                // 1. Ângulos Horários com Sinal (Leste Negativo / Oeste Positivo)
                double md_sig_com_sinal = ra_sig - ramc;
                if (md_sig_com_sinal > 180.0)  md_sig_com_sinal -= 360.0;
                if (md_sig_com_sinal < -180.0) md_sig_com_sinal += 360.0;
                double cota_sig_orientada = md_sig_com_sinal / sa_sig;
            
                double md_prom_com_sinal = ra_prom - ramc;
                if (md_prom_com_sinal > 180.0)  md_prom_com_sinal -= 360.0;
                if (md_prom_com_sinal < -180.0) md_prom_com_sinal += 360.0;
            
                // 2. Mapeamento Real do Aspecto Mundano na Esfera (Avanço no sentido horário)
                // O aspecto desloca a posição do promissor somando frações do seu semi-arco
                double md_aspecto_prom = md_prom_com_sinal + (proporcao_aspecto[a] * sa_prom);
                
                // Normalização estrita do ângulo horário do aspecto (-180 a +180)
                if (md_aspecto_prom > 180.0)  md_aspecto_prom -= 360.0;
                if (md_aspecto_prom < -180.0) md_aspecto_prom += 360.0;
            
                // =========================================================================
                // 3. CÁLCULO DOS ARCOS DE DIREÇÃO MUNDANA
                // =========================================================================
                if (s == 0) { // === DIREÇÃO DIRETA ===
                    double md_destino = sa_prom * cota_sig_orientada;
                    arco = md_aspecto_prom - md_destino;
                } 
                else if (s == 1) { // === DIREÇÃO CONVERSA ===
                    // Na conversa, a cota proporcional do aspecto do promissor puxa o significador
                    double cota_aspecto_prom = md_aspecto_prom / sa_prom;
                    double md_destino = sa_sig * cota_aspecto_prom;
                    arco = md_destino - md_sig_com_sinal;
                }
            
                // =========================================================================
                // 4. NORMALIZAÇÃO FINAL DO MOVIMENTO PRIMÁRIO (0 a 360)
                // =========================================================================
                if (arco < 0.0) arco += 360.0;
                arco = fmod(arco, 360.0);

                // Filtra arcos de idade humana viável (0 a 150 anos)
                if (arco > 0.001 && arco <= MAX_AGE * 1.05) { // tolerânciazinha de borda
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];
                    
                    d->sentido = s;
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    strcpy(d->significador_name, parts[idx_alvo].name);

                    char abreviacao[10];
                    get_part_abbreviation(parts[idx_alvo].name, abreviacao);
            
                    
                    strcpy(d->significador_glifo, abreviacao);
                    d->promissor_type = prom[p].type;
                    
                    // 1. Calcula o arco e a idade do evento usando a SUA chave equatorial
                    d->arco_graus = arco;
                    d->idade_evento = arco / NAIBOD_KEY; // Usa 1.014646
            
                    // 2. Transforma a idade em dias de forma perfeitamente proporcional
                    // Naibod estabelece que 1 ano de idade = 1 Ano Tropical médio (365.242199 dias)
                    double dias_decorridos = d->idade_evento * 365.242199;
            
                    // 3. Calcula o Dia Juliano do evento somando ao JD natal
                    double jd_evento = jd + dias_decorridos;
            
                    // 4. Converte o Dia Juliano para data UTC (Swisseph gerencia calendários)
                    int ano_c, mes_c, dia_c, hora_c, min_c;
                    double sec_c;
                    swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);
            
                    // 5. Alimenta a estrutura com as datas corrigidas
                    d->ano_calendario = ano_c;
                    d->mes_calendario = mes_c;
                    d->dia_calendario = dia_c;
                                   
                    strcpy(d->tipo_direcao, _("Mundane"));
                    d->tipo_direcao_id = DIRECAO_MUNDANA;

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
