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
    double tjd_ut_atual = tjd_ut_natal + (idade_anos * 365.242199);
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
    double sol_alvo = normalize360(sol_natal + arco_alvo);

    // 2. Estimativa inicial rápida (Usa Naibod como ponto de partida)
    double dias_estimados = arco_alvo / NAIBOD_KEY;

    // 3. Método de Newton-Raphson (Apenas 3 passos são necessários!)
    for (int i = 0; i < 3; i++) {
        swe_calc_ut(tjd_ut_natal + dias_estimados, SE_SUN, iflag, x2, serr);
        double sol_estimado = x2[0];
        double velocidade_sol = x2[3]; // Velocidade em graus/dia

        double erro = normalize360(sol_estimado - sol_alvo);
        if (erro > 180.0) erro -= 360.0;

        // Ajusta a estimativa usando a derivada (velocidade real do Sol naquele dia)
        dias_estimados -= erro / velocidade_sol;
    }

    if (dias_estimados < 1e-6) {
        swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x2, serr);
        return x2[3];
    }

    // Retorna a chave perfeitamente calibrada para a escala de tempo "1 ano = 1 dia"
    return arco_alvo / dias_estimados;
}




// Função auxiliar interna para simular o arco gerado pela velocidade do Sol
static double calcular_arco_kepler_interno(double tjd_ut_natal, double idade_anos) {
    double x2[6];
    char serr[256];
    int32 iflag = SEFLG_SPEED;
    
    double tjd_ut_atual = tjd_ut_natal + (idade_anos * 365.242199);
    swe_calc_ut(tjd_ut_atual, SE_SUN, iflag, x2, serr);
    
    return x2[3] * idade_anos; // Velocidade instantânea do dia * anos acumulados
}

/**
 * Retorna o valor CHAVE dinâmico para a lógica de Kepler.
 * Uso no seu código: double chave = obter_chave_kepler(tjd_natal, arco);
 *                    double idade = arco / chave;
 */
double obter_chave_kepler(double tjd_ut_natal, double arco_alvo) {
    double limite_inferior = 0.0;
    double limite_superior = 120.0;
    double idade_estimada = 0.0;

    // Busca interna da idade correspondente ao arco sob as leis de Kepler
    for (int i = 0; i < 50; i++) {
        idade_estimada = (limite_inferior + limite_superior) / 2.0;
        
        double arco_estimado = calcular_arco_kepler_interno(tjd_ut_natal, idade_estimada);
        
        if (fabs(arco_estimado - arco_alvo) < 1e-8) break;

        if (arco_estimado > arco_alvo) limite_superior = idade_estimada;
        else limite_inferior = idade_estimada;
    }

    // Evita divisão por zero para arcos nulos
    if (idade_estimada < 1e-6) {
        double x2[6];
        char serr[256];
        swe_calc_ut(tjd_ut_natal, SE_SUN, SEFLG_SPEED, x2, serr);
        return x2[3];
    }

    // Retorna a Chave Equivalente para fechar com a sua equação matemática
    return arco_alvo / idade_estimada;
}



#include "swephexp.h"
#include <stdio.h>
#include <math.h>

/**
 * Retorna o valor CHAVE dinâmico para a lógica de Kepler com altíssima velocidade.
 * Uso no seu código: double chave = obter_chave_kepler_ultra_rapida(tjd_natal, arco);
 *                    double idade = arco / chave;
 */
double obter_chave_kepler_ultra_rapida(double tjd_ut_natal, double arco_alvo) {
    double x2[6];
    char serr[256];
    int32 iflag = SEFLG_SPEED;

    // 1. Estimativa inicial rápida usando uma média padrão (ex: Naibod)
    // Isso nos joga muito perto da idade real antes de começar
    double idade_estimada = arco_alvo / NAIBOD_KEY;

    // 2. Método de Newton-Raphson (Apenas 3 passos encontram a precisão máxima)
    for (int i = 0; i < 3; i++) {
        // Encontra a data do trânsito na idade estimada
        double tjd_ut_atual = tjd_ut_natal + (idade_estimada * 365.242199);
        
        if (swe_calc_ut(tjd_ut_atual, SE_SUN, iflag, x2, serr) < 0) {
            // Se falhar, aborta para evitar loop infinito
            break; 
        }

        double velocidade_sol = x2[3]; // x2[3] contém a velocidade diária em graus/dia
        
        // Na lógica de Kepler: Arco = Velocidade * Idade
        double arco_estimado = velocidade_sol * idade_estimada;
        double erro = arco_estimado - arco_alvo;

        // Ajusta a estimativa dividindo o erro pela derivada aproximada (velocidade)
        idade_estimada -= erro / velocidade_sol;
    }

    // 3. Proteção contra divisão por zero para arcos nulos ou recém-nascidos
    if (idade_estimada < 1e-6) {
        swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x2, serr);
        return x2[3]; // Retorna a velocidade do dia do nascimento
    }

    // Retorna a Chave Equivalente exata para fechar com a sua equação matemática:
    // Idade = Arco / Chave -> Chave = Arco / Idade
    return arco_alvo / idade_estimada;
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




// Busca numérica robusta baseada no objeto do promissor e sua curva de Bianchini
double encontrar_longitude_promissor_por_dec(double dec_alvo, double jd, char* obj_prom, double limite_inf, double limite_sup) {
    double lon_min = limite_inf;
    double lon_max = limite_sup;
    double lon_mid = (lon_min + lon_max) / 2.0;
    double obl = -get_obliquidade(jd);
    int max_iter = 40;

    for (int i = 0; i < max_iter; i++) {
        lon_mid = (lon_min + lon_max) / 2.0;
        double lat = calcular_latitude_dinamica_bianchini(jd, obj_prom, lon_mid);
        
        double xx[3] = {lon_mid, lat, 1.0};
        double xequat[3];
        swe_cotrans(xx, xequat, obl);
        
        double dec_calc = xequat[1]; // Declinação calculada no modelo equatorial

        if (fabs(dec_calc - dec_alvo) < 0.00001) break;

        // Como a curva com latitude varia, testamos a derivada local para ajustar os limites
        double lat_ajuste = calcular_latitude_dinamica_bianchini(jd, obj_prom, lon_mid + 0.01);
        double xx_ajuste[3] = {lon_mid + 0.01, lat_ajuste, 1.0};
        double xequat_ajuste[3];
        swe_cotrans(xx_ajuste, xequat_ajuste, obl);
        
        int ascendente = (xequat_ajuste[1] > dec_calc);

        if ((dec_calc < dec_alvo && ascendente) || (dec_calc > dec_alvo && !ascendente)) {
            lon_min = lon_mid;
        } else {
            lon_max = lon_mid;
        }
    }
    return lon_mid;
}


// Calcula o cronograma de direções zodiacais para QUALQUER ponto escolhido
int calcular_direcoes_zodiacais_geral(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, double jd, int sentido, Promissor *prom) {

    int qtd_direcoes = 0;
    int object_diff = show_modern_planets ? 0 : 3;

    //if (idx_alvo < 0 || idx_alvo >= NUM_OBJECTS) return 0;

    // Calcula a Ascensão Reta baseada na coordenada do ponto alvo escolhido
    double ra_significador = sig[idx_alvo].ra; //calcular_ra(plots[idx_alvo].longitude, plots[idx_alvo].declination, jd);
    double dec_significador = sig[idx_alvo].declination; // Declinação natal do alvo

    double angulos_aspectos[] = {0.0, 60.0, 90.0, 120.0, 180.0, 999.9, 999.9};
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍", "∥", "∦"};

    //double epsilon = 0.000001;

    for (int p = 0; p < prom_id; p++) {

        if ((prom[p].type == PROM_ANTISCIUM || prom[p].type == PROM_CONTRANTISCIUM) && !ANT_PROM) continue;

        for (int s = 0; s < 2; s++) {
            if ((p == idx_alvo && p < NUM_OBJECTS - object_diff - ((show_modern_planets)?5:4)) || prom[p].type == PROM_POINT || prom[p].type == PROM_ANGLE) continue; // Um ponto não direciona a si mesmo
            if (prom[p].type == PROM_TERM && s == 1) continue;

            for (int a = 0; a < 7; a++) {

                if (prom[p].type == PROM_TERM && a > 0) break; // apenas conjunções para termos

                // --- DENTRO DO LOOP PRINCIPAL DOS ASPECTOS (a < 7) ---

                double arco = 0.0;
                int eh_paralelo = (a == 5 || a == 6);

                if (eh_paralelo) {
                    if (s == 1) continue; // Evita duplicidade de sentido para paralelos

                    // Declinação que o PROMISSOR precisa alcançar
                    double dec_alvo_paralelo = (a == 5) ? dec_significador : -dec_significador;

                    // Pega as coordenadas equatoriais NATAIS do promissor
                    // Precisamos saber a AR natal do promissor para descobrir quanto ele precisa andar
                    double xx_prom[3], xequat_prom[3];
                    xx_prom[0] = prom[p].longitude;
                    xx_prom[1] = calcular_latitude_dinamica_bianchini(jd, prom[p].object, prom[p].longitude);
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

                    // Escolhemos qual dos dois arcos processar nesta iteração. 
                    // Para processar ambos no seu motor sem quebrar o loop, usamos uma técnica simples:
                    // Na primeira iteração passamos o arco1, se quiser mapear o segundo ponto, podemos rodar um mini sub-loop.
                    // Vamos usar um loop de 2 iterações para garantir que os dois pontos entrem no cronograma!
                    
                    double arcos_paralelo[2] = {arco1, arco2};

                    for (int k = 0; k < 2; k++) {
                        arco = arcos_paralelo[k];

                        // Filtra arcos de idade humana viável (0 a 150 anos)
                        if (arco > 0.0 && arco <= MAX_AGE * 1.05) {
                            LinhaDirecao *d = &lista_resultado[qtd_direcoes];
                            d->sentido = s;
                            
                            strcpy(d->promissor_name, prom[p].object_name);
                            strcpy(d->promissor_glifo, prom[p].object);
                            strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                            strcpy(d->significador_name, sig[idx_alvo].object_name);
                            strcpy(d->significador_glifo, sig[idx_alvo].object);
                            d->promissor_type = prom[p].type;
                            d->arco_graus = arco;

                            double CHAVE = get_time_key(TIME_KEY, jd, arco);
                            d->idade_evento = arco / CHAVE;

                            double dias_decorridos = d->idade_evento * 365.242199;
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
                            if (qtd_direcoes >= 300) goto fim_calculo;
                        }
                    }
                    continue; // Pula o resto do loop padrão de aspectos longitudinais para não duplicar dados!
                }

                // --- LOGICA ORIGINAL PARA OS OUTROS 5 ASPECTOS (a < 5) ---
                // (Mantenha o seu cálculo original de lon_aspecto, swe_cotrans e cálculo de arco aqui)


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

                    double CHAVE = get_time_key(TIME_KEY, jd, arco);

                    d->idade_evento = arco / CHAVE; // Baseado em #define NAIBOD_KEY 1.014646

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


double get_time_key(int key, double jd, double arco) {
    switch(key) {
        case TIME_KEY_NAIBOD:         return NAIBOD_KEY;
        case TIME_KEY_CARDANO:         return CARDANO_KEY;
        case TIME_KEY_PTOLEMY:        return PTOLEMY_KEY;
        case TIME_KEY_PLACIDUS:       return PLACIDUS_KEY;
        case TIME_KEY_TRUE_SOLAR_ARC: return obter_chave_arco_solar_ultra_rapida(jd, arco); //obter_chave_arco_solar(jd, arco);           
        case TIME_KEY_KEPLER:         return obter_chave_kepler_ultra_rapida(jd, arco); //obter_chave_kepler(jd, arco);
        default: return NAIBOD_KEY;
    }
}


double get_key(int key) {
    switch(key) {
        case TIME_KEY_NAIBOD:         return NAIBOD_KEY;
        case TIME_KEY_CARDANO:         return CARDANO_KEY;
        case TIME_KEY_PTOLEMY:        return PTOLEMY_KEY;
        case TIME_KEY_PLACIDUS:       return PLACIDUS_KEY;
        case TIME_KEY_TRUE_SOLAR_ARC: return -1.0;           
        case TIME_KEY_KEPLER:         return -1.0;
        default: return NAIBOD_KEY;
    }
}


const char* get_key_name(int key) {
    switch(key) {
        case TIME_KEY_NAIBOD:         return "Naibod";
        case TIME_KEY_CARDANO:         return _("Cardano");
        case TIME_KEY_PTOLEMY:        return _("Ptolemy");
        case TIME_KEY_PLACIDUS:       return "Placidus";
        case TIME_KEY_TRUE_SOLAR_ARC: return _("True Solar Arc");           
        case TIME_KEY_KEPLER:         return "Kepler";
        default: return "Naibod";
    }
}


double obter_cota_fixa_casa_placidus(int numero_casa) {
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



int calcular_direcoes_mundanas_geral(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promissor *prom) {
    int qtd_direcoes = 0;
    double lat_geo_rad = para_radianos(lat_geografica);

    // 1. Dados tridimensionais REAIS do Significador (Alvo)
    double ra_sig = sig[idx_alvo].ra;
    double dec_sig_rad = para_radianos(sig[idx_alvo].declination);
    
    int sig_acima = verificar_se_acima_horizonte(ra_sig, dec_sig_rad, ramc, lat_geo_rad); 
    
    double sa_sig = __calcular_semi_arco(dec_sig_rad, lat_geo_rad, sig_acima);
    
    double proporcao_aspecto[] = {0.0, 0.66666667, 1.0, 1.33333333, 2.0, 999.9, 999.9}; 
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍", "∥", "∦"};

    for (int p = 0; p < prom_id; p++) {
        if (prom[p].type == PROM_POINT || 
            prom[p].type == PROM_ANGLE || 
            prom[p].type == PROM_PART  || 
            prom[p].type == PROM_TERM
        ) continue;

        if ((prom[p].type == PROM_ANTISCIUM || prom[p].type == PROM_CONTRANTISCIUM) && !ANT_PROM) continue;
       
        // 2. Dados tridimensionais REAIS do Promissor
        double ra_prom = prom[p].ra; 
        double dec_prom_rad = para_radianos(prom[p].declination);
        
        int prom_acima = verificar_se_acima_horizonte(ra_prom, dec_prom_rad, ramc, lat_geo_rad);

        double sa_prom = __calcular_semi_arco(dec_prom_rad, lat_geo_rad, prom_acima);

        for (int s = 0; s < 2; s++) { // 0 = Direta, 1 = Conversa            
            if (s == 0 && sentido == 1) continue; 
            if (s == 1 && sentido == 0) continue; 

            for (int a = 0; a < 7; a++) {
                if (prom[p].type == PROM_TERM && a > 0) break;

                double arco = 0.0;
                            
                // 1. Ângulos Horários Iniciais (Cálculos que você já faz)
                double md_sig_com_sinal = ra_sig - ramc;
                if (md_sig_com_sinal > 180.0)  md_sig_com_sinal -= 360.0;
                if (md_sig_com_sinal < -180.0) md_sig_com_sinal += 360.0;
                double cota_sig_orientada = md_sig_com_sinal / sa_sig;
            
                double md_prom_com_sinal = ra_prom - ramc;
                if (md_prom_com_sinal > 180.0)  md_prom_com_sinal -= 360.0;
                if (md_prom_com_sinal < -180.0) md_prom_com_sinal += 360.0;
            
                // 2. DECLARAÇÃO ANTECIPADA: Calcule a md_aspecto_prom aqui para servir a ambos os blocos!
                double md_aspecto_prom = md_prom_com_sinal;
                if (a < 5) { // Apenas para os aspectos longitudinais clássicos
                    md_aspecto_prom = md_prom_com_sinal + (proporcao_aspecto[a] * sa_prom);
                }
                
                // Normalização circular segura da md_aspecto_prom
                if (md_aspecto_prom > 180.0)  md_aspecto_prom -= 360.0;
                if (md_aspecto_prom < -180.0) md_aspecto_prom += 360.0;
            

                // Nova variável para identificar se o alvo atual comporta-se como um ângulo fixo no espaço local
                int eh_angulo_angular = 0;
                double cota_espacial_fixa = 0.0;
            
                if (sig[idx_alvo].type == PROM_ANGLE) {
                    if (strcmp(sig[idx_alvo].object, "AC") == 0) {
                        eh_angulo_angular = 1;
                        cota_espacial_fixa = -1.0; // Horizonte Leste exato
                    }
                    else if (strcmp(sig[idx_alvo].object, "MC") == 0) {
                        eh_angulo_angular = 1;
                        cota_espacial_fixa = 0.0;  // Meridiano exato
                    }
                    else if (strcmp(sig[idx_alvo].object, "DC") == 0) {
                        eh_angulo_angular = 1;
                        cota_espacial_fixa = 1.0;  // Horizonte Oeste exato
                    }
                    else if (strcmp(sig[idx_alvo].object, "IC") == 0) {
                        eh_angulo_angular = 1;
                        cota_espacial_fixa = 0.0;  // Meridiano exato
                    }
                }
                // Se o significador for a estrutura da Cúspide
                else if (sig[idx_alvo].type == PROM_CUSP) {
                    int num_casa = sig[idx_alvo].house;
                    // Permitimos que as Casas Angulares (1, 4, 7, 10) também entrem no bloco matemático de cotas fixas
                    if (num_casa == 1 || num_casa == 4 || num_casa == 7 || num_casa == 10) {
                        eh_angulo_angular = 1;
                        cota_espacial_fixa = obter_cota_fixa_casa_placidus(num_casa);
                    }
                }
            
                // ====================================================
                // INJEÇÃO NO MOTOR DE CÁLCULO DO ARCO
                // ====================================================
                if (eh_angulo_angular) {
                    // Ambos agora rodam por aqui! Sem ruídos de ponto flutuante das coordenadas equatoriais natais.
                    if (s == 0) { // === DIREÇÃO DIRETA ===
                        double md_destino = sa_prom * cota_espacial_fixa;
                        arco = md_aspecto_prom - md_destino;
                    } 
                    else if (s == 1) { // === DIREÇÃO CONVERSA ===
                        double cota_aspecto_prom = md_aspecto_prom / sa_prom;
                        double md_destino = sa_sig * cota_aspecto_prom; 
                        arco = md_destino - md_sig_com_sinal;
                    }
                }
                else if (sig[idx_alvo].type == PROM_CUSP) {
                    // Casas intermediárias (2, 3, 5, 6, 8, 9, 11, 12)
                    if (a > 0 && a < 5) continue; // Trava os aspectos intermediários longitudinais apenas nelas
            
                    int num_casa = sig[idx_alvo].house; 
                    double cota_casa = obter_cota_fixa_casa_placidus(num_casa);
            
                    if (s == 0) { 
                        double md_destino = sa_prom * cota_casa;
                        arco = md_aspecto_prom - md_destino;
                    } 
                    else if (s == 1) { 
                        double cota_aspecto_prom = md_aspecto_prom / sa_prom;
                        double md_destino = sa_sig * cota_aspecto_prom; 
                        arco = md_destino - md_sig_com_sinal;
                    }
                }
                else {
                
                    // === TRATAMENTO PARA PLANETAS (bloco com paralelos e aspectos normais) ===
                    if (a == 5 || a == 6) {
                        double cota_alvo_mundo = (a == 5) ? cota_sig_orientada : -cota_sig_orientada;
            
                        if (s == 0) { 
                            double md_destino = sa_prom * cota_alvo_mundo;
                            arco = md_prom_com_sinal - md_destino;
                        } 
                        else if (s == 1) { 
                            double cota_prom_natal = md_prom_com_sinal / sa_prom;
                            if (a == 6) cota_prom_natal = -cota_prom_natal; 
                            double md_destino = sa_sig * cota_prom_natal;
                            arco = md_destino - md_sig_com_sinal;
                        }
                    }
                    else { 
                        if (s == 0) { 
                            double md_destino = sa_prom * cota_sig_orientada;
                            arco = md_aspecto_prom - md_destino;
                        } 
                        else if (s == 1) { 
                            double cota_aspecto_prom = md_aspecto_prom / sa_prom;
                            double md_destino = sa_sig * cota_aspecto_prom;
                            arco = md_destino - md_sig_com_sinal;
                        }
                    }
                }
                      
                // Normalização comum do arco resultante
                if (arco < 0.0) arco += 360.0;
                arco = fmod(arco, 360.0);
            
                if (arco > 0.001 && arco <= MAX_AGE * 1.05) { 
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];
                    
                    d->sentido = s;
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    strcpy(d->significador_name, sig[idx_alvo].object_name);
                    strcpy(d->significador_glifo, sig[idx_alvo].object);
                    d->promissor_type = prom[p].type;
                    
                    d->arco_graus = arco;
                    double CHAVE = get_time_key(TIME_KEY, jd, arco);
                    d->idade_evento = arco / CHAVE;
            
                    double dias_decorridos = d->idade_evento * 365.242199;
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

    int max_linhas_exibicao = (table_height / 2) * 2 - 12;
    WINDOW *scroll_pad = newpad(1200, table_width - 8); 

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


    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED, NULL);
    mouseinterval(100);

    int sentido = 2;
    int tipo = 2;

    while (loop_interativo) {
        werase(table_win);
        werase(scroll_pad);

        box(table_win, 0, 0);
        wattron(table_win, A_BOLD);
        const char *title = _(" Primary Directions ");
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
        
        LinhaDirecao cronograma_z[300];
        LinhaDirecao cronograma_m[300];
        
        memset(cronograma_z, 0, sizeof(cronograma_z));
        qtd_direcoes_zod = calcular_direcoes_zodiacais_geral(sig, idx_atual_calculo, cronograma_z, jd, 2, prom);

        if (tipo != 0) {   
            memset(cronograma_m, 0, sizeof(cronograma_m));
            qtd_direcoes_mun = calcular_direcoes_mundanas_geral(sig, idx_atual_calculo, cronograma_m, jd, ramc, lat, 2, prom);
        }
        int qtd_direcoes = qtd_direcoes_zod + qtd_direcoes_mun;

        LinhaDirecao cronograma_a[qtd_direcoes];
        memset(cronograma_a, 0, sizeof(cronograma_a));

        int index = 0;
        for (int i = 0; i < qtd_direcoes_zod; i++) {
            cronograma_a[index] = cronograma_z[i];
            index++;
        }
        for (int i = 0; i < qtd_direcoes_mun; i++) {
            cronograma_a[index] = cronograma_m[i];
            index++;
        }
        qsort(cronograma_a, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade_tipo_termo);

        // obter glifo e nome do regente do termo natal do significador
        int regente_do_termo = get_term_ruler(sig[idx_atual_calculo].longitude);            
        char glifo_atual[10];
        char divisor_atual[30];
        snprintf(glifo_atual, sizeof(glifo_atual), "%s", planet_regent_symbols[regente_do_termo]);
        snprintf(divisor_atual, sizeof(divisor_atual), "%s", planet_regent_names[regente_do_termo]);

        for (int i = 0; i < qtd_direcoes; i++) {
            if (cronograma_a[i].promissor_type == PROM_TERM && 
                cronograma_a[i].tipo_direcao_id == DIRECAO_ZODIACAL &&
                cronograma_a[i].sentido == DIRECT
            ) {
                strcpy(divisor_atual, cronograma_a[i].promissor_name);
                int id_planeta = obter_id_planeta_por_nome(divisor_atual);
                strcpy(glifo_atual, obter_glifo_planeta_por_id(id_planeta));
            }

            strcpy(cronograma_a[i].divisor_name, divisor_atual);
            strcpy(cronograma_a[i].divisor_gliph, glifo_atual);
        }

        LinhaDirecao cronograma[qtd_direcoes];
        memset(cronograma, 0, sizeof(cronograma));

        index = 0;
        if (tipo == 1 && sentido == 2) {
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].tipo_direcao_id == 0) {
                    continue;
                }
                cronograma[index] = cronograma_a[i];
                index++;
            }
            qtd_direcoes = qtd_direcoes - qtd_direcoes_zod;
        }
        else if (tipo == 1 && sentido == 0) {
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].tipo_direcao_id == 0 || cronograma_a[i].sentido == 1) {
                    continue;
                }
                cronograma[index] = cronograma_a[i];
                index++;
            }
            qtd_direcoes = index;
        }
        else if (tipo == 1 && sentido == 1) {
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].tipo_direcao_id == 0 || cronograma_a[i].sentido == 0) {
                    continue;
                }
                
                cronograma[index] = cronograma_a[i];
                index++;
            }
            qtd_direcoes = index;
        }
        else if (sentido == 1) {
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].sentido == 0) {
                    continue;
                }
                cronograma[index] = cronograma_a[i];
                index++;
            }
            qtd_direcoes = index;
        }
        else if (sentido == 0) {
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].sentido == 1) {
                    continue;
                }
                cronograma[index] = cronograma_a[i];
                index++;
            }
            qtd_direcoes = index;
        }
        else {
            for (int i = 0; i < qtd_direcoes; i++) {
                cronograma[i] = cronograma_a[i];
            }
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

                

                bool eh_aspecto_tenso = (strcmp(d->aspecto_symbol, "□") == 0 || strcmp(d->aspecto_symbol, "☍") == 0 || strcmp(d->aspecto_symbol, "∦") == 0);
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
        if (TIME_KEY < 5) {
            mvwprintw(table_win, table_height - 6, 4, _("Time Key: %s Rate (1° of Equatorial Rotation = %6.4f Years). ε: Dynamic."), get_key_name(TIME_KEY), 1.0 / get_key(TIME_KEY));
        }
        else {
            mvwprintw(table_win, table_height - 6, 4, _("Time Key: %s Rate (Dynamic). ε: Dynamic."), get_key_name(TIME_KEY));
        }
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
        desenhar_scrollbar(table_win, scroll_offset, linhas_reais_pad - 1, max_linhas_exibicao - 1, 6);
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
                        if (linha_clique_barra >= 0 && linha_clique_barra < max_linhas_exibicao - 1) {
                            
                            // Calcula o limite máximo que o scroll_offset pode atingir
                            int max_scroll_y = (qtd_direcoes * 2) - max_linhas_exibicao;
                            if (max_scroll_y < 0) max_scroll_y = 0;

                            if (max_linhas_exibicao > 1 && max_scroll_y > 0) {
                                // Mapeia proporcionalmente a linha clicada para o novo offset de dados
                                int novo_offset = (linha_clique_barra * max_scroll_y) / (max_linhas_exibicao - 2);
                                
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
    double dec_significador = dec_out; // Declinação natal do alvo

    double angulos_aspectos[] = {0.0, 60.0, 90.0, 120.0, 180.0, 999.9, 999.9};
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍", "∥", "∦"};
    
    for (int p = 0; p < prom_id; p++) {
        if (prom[p].type == PROM_POINT || prom[p].type == PROM_ANGLE || prom[p].type == PROM_PART) continue;
        if ((prom[p].type == PROM_ANTISCIUM || prom[p].type == PROM_CONTRANTISCIUM) && !ANT_PROM) continue;

        for (int s = 0; s < 2; s++) {

            if (prom[p].type == PROM_TERM && s == 1) continue;

            for (int a = 0; a < 7; a++) {
                
                if (prom[p].type == PROM_TERM && a > 0) break; // apenas conjunções para termos

                double arco = 0.0;
                int eh_paralelo = (a == 5 || a == 6);

                if (eh_paralelo) {
                    if (s == 1) continue; // Evita duplicidade de sentido para paralelos

                    // Declinação que o PROMISSOR precisa alcançar
                    double dec_alvo_paralelo = (a == 5) ? dec_significador : -dec_significador;

                    // Pega as coordenadas equatoriais NATAIS do promissor
                    // Precisamos saber a AR natal do promissor para descobrir quanto ele precisa andar
                    double xx_prom[3], xequat_prom[3];
                    xx_prom[0] = prom[p].longitude;
                    xx_prom[1] = calcular_latitude_dinamica_bianchini(jd, prom[p].object, prom[p].longitude);
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

                    // Escolhemos qual dos dois arcos processar nesta iteração. 
                    // Para processar ambos no seu motor sem quebrar o loop, usamos uma técnica simples:
                    // Na primeira iteração passamos o arco1, se quiser mapear o segundo ponto, podemos rodar um mini sub-loop.
                    // Vamos usar um loop de 2 iterações para garantir que os dois pontos entrem no cronograma!
                    
                    double arcos_paralelo[2] = {arco1, arco2};

                    for (int k = 0; k < 2; k++) {
                        arco = arcos_paralelo[k];

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
                            d->arco_graus = arco;

                            double CHAVE = get_time_key(TIME_KEY, jd, arco);
                            d->idade_evento = arco / CHAVE;

                            double dias_decorridos = d->idade_evento * 365.242199;
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
                            if (qtd_direcoes >= 300) goto fim_calculo;
                        }
                    }
                    continue; // Pula o resto do loop padrão de aspectos longitudinais para não duplicar dados!
                }

                // --- LOGICA ORIGINAL PARA OS OUTROS 5 ASPECTOS (a < 5) ---
                // (Mantenha o seu cálculo original de lon_aspecto, swe_cotrans e cálculo de arco aqui)


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

                    double CHAVE = get_time_key(TIME_KEY, jd, arco);
                    d->idade_evento = arco / CHAVE;

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
fim_calculo:
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
    int max_linhas_exibicao = (table_height / 2) * 2 - 12; // Espaço físico real na janela para os dados
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


    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED, NULL);
    mouseinterval(100);

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
        
        LinhaDirecao cronograma_z[300];
        LinhaDirecao cronograma_m[300];
        
        memset(cronograma_z, 0, sizeof(cronograma_z));
        qtd_direcoes_zod = calcular_direcoes_zodiacais_partes(lista_partes, qtd_partes, idx_atual_calculo, cronograma_z, jd, 2, prom);
        
        if (tipo != 0) {   
            memset(cronograma_m, 0, sizeof(cronograma_m));
            qtd_direcoes_mun = calcular_direcoes_mundanas_partes(lista_partes, idx_atual_calculo, cronograma_m, jd, ramc, lat, 2, prom);
        }
        int qtd_direcoes = qtd_direcoes_zod + qtd_direcoes_mun;

        LinhaDirecao cronograma_a[qtd_direcoes];
        memset(cronograma_a, 0, sizeof(cronograma_a));

        int index = 0;
        for (int i = 0; i < qtd_direcoes_zod; i++) {
            cronograma_a[index] = cronograma_z[i];
            index++;
        }
        for (int i = 0; i < qtd_direcoes_mun; i++) {
            cronograma_a[index] = cronograma_m[i];
            index++;
        }
        qsort(cronograma_a, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade_tipo_termo);

        // obter glifo e nome do regente do termo natal do significador
        int regente_do_termo = get_term_ruler(lista_partes[idx_atual_calculo].longitude);            
        char glifo_atual[10];
        char divisor_atual[30];
        snprintf(glifo_atual, sizeof(glifo_atual), "%s", planet_regent_symbols[regente_do_termo]);
        snprintf(divisor_atual, sizeof(divisor_atual), "%s", planet_regent_names[regente_do_termo]);

        for (int i = 0; i < qtd_direcoes; i++) {
            if (cronograma_a[i].promissor_type == PROM_TERM && 
                cronograma_a[i].tipo_direcao_id == DIRECAO_ZODIACAL &&
                cronograma_a[i].sentido == DIRECT
            ) {
                strcpy(divisor_atual, cronograma_a[i].promissor_name);
                int id_planeta = obter_id_planeta_por_nome(divisor_atual);
                strcpy(glifo_atual, obter_glifo_planeta_por_id(id_planeta));
            }

            strcpy(cronograma_a[i].divisor_name, divisor_atual);
            strcpy(cronograma_a[i].divisor_gliph, glifo_atual);
        }

        
        LinhaDirecao cronograma[qtd_direcoes];
        memset(cronograma, 0, sizeof(cronograma));

        index = 0;
        if (tipo == 1 && sentido == 2) {
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].tipo_direcao_id == 0) {
                    continue;
                }
                cronograma[index] = cronograma_a[i];
                index++;
            }
            qtd_direcoes = qtd_direcoes - qtd_direcoes_zod;
        }
        else if (tipo == 1 && sentido == 0) {
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].tipo_direcao_id == 0 || cronograma_a[i].sentido == 1) {
                    continue;
                }
                cronograma[index] = cronograma_a[i];
                index++;
            }
            qtd_direcoes = index;
        }
        else if (tipo == 1 && sentido == 1) {
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].tipo_direcao_id == 0 || cronograma_a[i].sentido == 0) {
                    continue;
                }
                
                cronograma[index] = cronograma_a[i];
                index++;
            }
            qtd_direcoes = index;
        }
        else if (sentido == 1) {
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].sentido == 0) {
                    continue;
                }
                cronograma[index] = cronograma_a[i];
                index++;
            }
            qtd_direcoes = index;
        }
        else if (sentido == 0) {
            for (int i = 0; i < qtd_direcoes; i++) {
                if (cronograma_a[i].sentido == 1) {
                    continue;
                }
                cronograma[index] = cronograma_a[i];
                index++;
            }
            qtd_direcoes = index;
        }
        else {
            for (int i = 0; i < qtd_direcoes; i++) {
                cronograma[i] = cronograma_a[i];
            }
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


                bool eh_aspecto_tenso = (strcmp(d->aspecto_symbol, "□") == 0 || strcmp(d->aspecto_symbol, "☍") == 0 || strcmp(d->aspecto_symbol, "∦") == 0);
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
        if (TIME_KEY < 5) {
            mvwprintw(table_win, table_height - 6, 4, _("Time Key: %s Rate (1° of Equatorial Rotation = %6.4f Years). ε: Dynamic."), get_key_name(TIME_KEY), 1.0 / get_key(TIME_KEY));
        }
        else {
            mvwprintw(table_win, table_height - 6, 4, _("Time Key: %s Rate (Dynamic). ε: Dynamic."), get_key_name(TIME_KEY));
        }
        
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
        desenhar_scrollbar(table_win, scroll_offset, qtd_direcoes * 2 - 1, max_linhas_exibicao - 1, 6);
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
                        if (linha_clique_barra >= 0 && linha_clique_barra < max_linhas_exibicao - 1) {
                            
                            // Calcula o limite máximo que o scroll_offset pode atingir
                            int max_scroll_y = (qtd_direcoes * 2) - max_linhas_exibicao;
                            if (max_scroll_y < 0) max_scroll_y = 0;

                            if (max_linhas_exibicao > 1 && max_scroll_y > 0) {
                                // Mapeia proporcionalmente a linha clicada para o novo offset de dados
                                int novo_offset = (linha_clique_barra * max_scroll_y) / (max_linhas_exibicao - 2);
                                
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

    double dec_out, ra_out;
    calc_declination_ra_point(jd, parts[idx_alvo].longitude, &ra_out, &dec_out);
    double ra_sig = ra_out;

    double dec_sig_rad = para_radianos(dec_out); //para_radianos(calc_declination_mathematical_point(jd, parts[idx_alvo].longitude));
    
    int sig_acima = verificar_se_acima_horizonte(ra_sig, dec_sig_rad, ramc, lat_geo_rad); 
    
    double sa_sig = __calcular_semi_arco(dec_sig_rad, lat_geo_rad, sig_acima);
    
    
    double proporcao_aspecto[] = {0.0, 0.66666667, 1.0, 1.33333333, 2.0, 999.9, 999.9}; 
    char *simbolos_aspectos[] = {"☌", "⚹", "□", "△", "☍", "∥", "∦"};

    for (int p = 0; p < prom_id; p++) {
        if (prom[p].type == PROM_POINT || 
            prom[p].type == PROM_ANGLE || 
            prom[p].type == PROM_PART || 
            prom[p].type == PROM_TERM
        ) continue;

        if ((prom[p].type == PROM_ANTISCIUM || prom[p].type == PROM_CONTRANTISCIUM) && !ANT_PROM) continue;

        // 2. Dados tridimensionais REAIS do Promissor
        double ra_prom = prom[p].ra; 
        double dec_prom_rad = para_radianos(prom[p].declination);
        
        // CORREÇÃO: Verificação astrométrica para o promissor também!
        int prom_acima = verificar_se_acima_horizonte(ra_prom, dec_prom_rad, ramc, lat_geo_rad);

        double sa_prom = __calcular_semi_arco(dec_prom_rad, lat_geo_rad, prom_acima);
        //double md_prom = __calcular_distancia_meridiana(ra_prom, ramc, prom_acima);
        for (int s = 0; s < 2; s++) { // 0 = Direta, 1 = Conversa
            
            if (s == 0 && sentido == 1) continue;
            if (s == 1 && sentido == 0) continue;

            for (int a = 0; a < 7; a++) {
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
            
                // --- TRATAMENTO DOS PARALELOS E CONTRAPARALELOS MUNDANOS ---
                if (a == 5 || a == 6) {
                    // Alvo geométrico: mesma cota (paralelo) ou cota invertida (contraparalelo)
                    double cota_alvo_mundo = (a == 5) ? cota_sig_orientada : -cota_sig_orientada;

                    if (s == 0) { // Direção Direta
                        // O promissor se move até atingir a proporção mundana do significador
                        double md_destino = sa_prom * cota_alvo_mundo;
                        arco = md_prom_com_sinal - md_destino;
                    } 
                    else if (s == 1) { // Direção Conversa
                        // O significador se move até atingir a proporção mundana do promissor
                        double cota_prom_natal = md_prom_com_sinal / sa_prom;
                        // Ajusta o sinal para o espelhamento converso do contraparalelo
                        if (a == 6) cota_prom_natal = -cota_prom_natal; 
                        
                        double md_destino = sa_sig * cota_prom_natal;
                        arco = md_destino - md_sig_com_sinal;
                    }
                }
                // --- TRATAMENTO DOS ASPECTOS LONGITUDINAIS CLÁSSICOS (0 a 4) ---
                else {
                    double md_aspecto_prom = md_prom_com_sinal + (proporcao_aspecto[a] * sa_prom);
                    
                    if (md_aspecto_prom > 180.0)  md_aspecto_prom -= 360.0;
                    if (md_aspecto_prom < -180.0) md_aspecto_prom += 360.0;
                
                    if (s == 0) { 
                        double md_destino = sa_prom * cota_sig_orientada;
                        arco = md_aspecto_prom - md_destino;
                    } 
                    else if (s == 1) { 
                        double cota_aspecto_prom = md_aspecto_prom / sa_prom;
                        double md_destino = sa_sig * cota_aspecto_prom;
                        arco = md_destino - md_sig_com_sinal;
                    }
                }
                                       
                if (arco < 0.0) arco += 360.0;
                arco = fmod(arco, 360.0);

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
                    
                    d->arco_graus = arco;

                    double CHAVE = get_time_key(TIME_KEY, jd, arco);
                    d->idade_evento = arco / CHAVE;
            
                    double dias_decorridos = d->idade_evento * 365.242199;
            
                    double jd_evento = jd + dias_decorridos;
            
                    // 4. Converte o Dia Juliano para data UTC (Swisseph gerencia calendários)
                    int ano_c, mes_c, dia_c, hora_c, min_c;
                    double sec_c;
                    swe_jdut1_to_utc(jd_evento, 2, &ano_c, &mes_c, &dia_c, &hora_c, &min_c, &sec_c);
            
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
