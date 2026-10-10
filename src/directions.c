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



double obter_chave_kepler_ultra_rapida(double tjd_ut_natal, double arco_alvo) {
    double x2[6];
    char serr[256];
    int32 iflag = SEFLG_SPEED;

    // 1. Estimativa inicial rápida em dias (1 dia = 1 ano)
    double dias_estimados = arco_alvo / 0.98564733;

    // 2. Método de Newton-Raphson
    for (int i = 0; i < 3; i++) {
        // CORREÇÃO: Varre as efemérides no passo secundário correto (dias desde o natal)
        double tjd_ut_atual = tjd_ut_natal + dias_estimados;
        
        if (swe_calc_ut(tjd_ut_atual, SE_SUN, iflag, x2, serr) < 0) {
            break; 
        }

        double velocidade_sol = x2[3]; 
        
        // Na lógica de Kepler: Arco = Velocidade do Dia * Tempo decorrido (em anos)
        // Como mapeamos 1 dia = 1 ano para o trânsito, o tempo decorrido é dias_estimados
        double arco_estimado = velocidade_sol * dias_estimados;
        double erro = arco_estimado - arco_alvo;

        dias_estimados -= erro / velocidade_sol;
    }

    if (dias_estimados < 1e-6) {
        swe_calc_ut(tjd_ut_natal, SE_SUN, iflag, x2, serr);
        return x2[3]; 
    }

    // Retorna a chave calibrada para fechar perfeitamente com a equação do motor pai
    double idade_anos_reais = dias_estimados;
    return arco_alvo / idade_anos_reais;
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



int calcular_direcoes_zodiacais_topocentrico(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, 
                                            double jd, int sentido_filtro, Promissor *prom, int total_promissores,
                                            double ramc, double lat_geografica, int *qtd_direcoes, bool is_part) {
    
    int object_diff = show_modern_planets ? 0 : 3;
    double angulos_aspectos[] = {0.0, 60.0, -60.0, 90.0, -90.0, 120.0, -120.0, 180.0, 999.9, 999.9};
    char *simbolos_aspectos[] = {"☌", "⚹", "⚹", "□", "□", "△", "△", "☍", "∥", "∦"};
    
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
            if (s == 0 && sentido_filtro == 1) continue;
            if (s == 1 && sentido_filtro == 0) continue;
                        
            for (int a = 0; a < 10; a++) {
                if (prom[p].type == PROM_TERM && a > 0) break;

                double arco = 0.0;
                int eh_paralelo = (a == 8 || a == 9);

                if (eh_paralelo) {
                    if (s == 1) continue; // Trava original do seu código para paralelos

                    double dec_alvo_paralelo = (a == 8) ? dec_sig : -dec_sig;
                    double obl = get_obliquidade(jd);

                    // =========================================================
                    // 1. EXTRAINDO AS COORDENADAS EQUATORIAIS NATAIS REAIS DO PROMISSOR
                    // =========================================================
                    double xx_prom[3], xequat_prom[3];
                    xx_prom[0] = prom[p].longitude;
                    xx_prom[1] = calcular_latitude_dinamica_bianchini(jd, prom[p].object, prom[p].longitude);
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
                                    strcmp(lista_resultado[m].promissor_name, prom[p].object_name) == 0 &&
                                    strcmp(lista_resultado[m].aspecto_symbol, simbolos_aspectos[a]) == 0 &&
                                    fabs(lista_resultado[m].arco_graus - arco) < 0.005) {
                                    ja_salvo = 1; break;
                                }
                            }
                            if (ja_salvo) continue;

                            // Preenchimento mantendo seus ponteiros originais intactos para o color-coding
                            LinhaDirecao *d = &lista_resultado[*qtd_direcoes];
                            d->sentido = s;

                            d->promissor_id = p;
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
                            else {                                
                                dias_decorridos = d->idade_evento * 365.242199;
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

                double lat_calculada = calcular_latitude_dinamica_bianchini(jd, prom[p].object, lon_aspecto);

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
                            strcmp(lista_resultado[m].promissor_name, prom[p].object_name) == 0 &&
                            strcmp(lista_resultado[m].aspecto_symbol, simbolos_aspectos[a]) == 0 &&
                            fabs(lista_resultado[m].arco_graus - arco) < 0.005) {
                            ja_salvo = 1; break;
                        }
                    }
                    if (ja_salvo) continue;

                    LinhaDirecao *d = &lista_resultado[*qtd_direcoes];
                    d->sentido = s;

                    d->promissor_id = p;
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
                    else {                                
                        dias_decorridos = d->idade_evento * 365.242199;
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

    // if (*qtd_direcoes > 1) {
    //     int i_valido = 0;

    //     for (int i_atual = 1; i_atual < *qtd_direcoes; i_atual++) {
    //         LinhaDirecao *d0 = &lista_resultado[i_valido];
    //         LinhaDirecao *d1 = &lista_resultado[i_atual];

    //         bool mesmo_promissor = (d0->promissor_id == d1->promissor_id);
    //         bool mesmo_sentido = (d0->sentido == d1->sentido);
            
    //         double delta_arco = fabs(d1->arco_graus - d0->arco_graus);

    //         bool arco_identico = (delta_arco < DELTA_ARCO);

    //         if (mesmo_promissor && mesmo_sentido && arco_identico) {
    //             continue; 
    //         }

    //         i_valido++;
    //         if (i_valido != i_atual) {
    //             lista_resultado[i_valido] = *d1;
    //         }
    //     }

    //     *qtd_direcoes = i_valido + 1;
    // }
    return *qtd_direcoes;
}







// Calcula o cronograma de direções zodiacais para QUALQUER ponto escolhido
int calcular_direcoes_zodiacais_geral(Promissor *sig, int idx_alvo, LinhaDirecao *lista_resultado, double jd, int sentido, Promissor *prom, bool is_part) {

    int qtd_direcoes = 0;
    int object_diff = show_modern_planets ? 0 : 3;

    double ra_significador = sig[idx_alvo].ra; //calcular_ra(plots[idx_alvo].longitude, plots[idx_alvo].declination, jd);
    double dec_significador = sig[idx_alvo].declination; // Declinação natal do alvo

    double angulos_aspectos[] = {0.0, 60.0, -60.0, 90.0, -90.0, 120.0, -120.0, 180.0, 999.9, 999.9};
    char *simbolos_aspectos[] = {"☌", "⚹", "⚹", "□", "□", "△", "△", "☍", "∥", "∦"};


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
            for (int a = 0; a < 10; a++) {

                if (prom[p].type == PROM_TERM && a > 0) break; // apenas conjunções para termos

                double arco = 0.0;
                int eh_paralelo = (a == 8 || a == 9);

                if (eh_paralelo) {
                    double dec_alvo_paralelo = (a == 8) ? dec_significador : -dec_significador;

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
                                        
                    double arcos_paralelo[2] = {arco1, arco2};

                    for (int k = 0; k < 2; k++) {
                        arco = arcos_paralelo[k];

                        // Filtra arcos de idade humana viável (0 a 150 anos)
                        if (arco > 0.001 && arco <= MAX_AGE * 1.05) {
                            LinhaDirecao *d = &lista_resultado[qtd_direcoes];
                            d->sentido = s;
                            
                            d->promissor_id = p;
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
                            else {                                
                                dias_decorridos = d->idade_evento * 365.242199;
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
                if (arco > 0.001 && arco <= MAX_AGE * 1.05) {
                    LinhaDirecao *d = &lista_resultado[qtd_direcoes];

                    d->sentido = s;
                    
                    d->promissor_id = p;
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
                    else {                                
                        dias_decorridos = d->idade_evento * 365.242199;
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

    // if (qtd_direcoes > 1) {
    //     int i_valido = 0;

    //     for (int i_atual = 1; i_atual < qtd_direcoes; i_atual++) {
    //         LinhaDirecao *d0 = &lista_resultado[i_valido];
    //         LinhaDirecao *d1 = &lista_resultado[i_atual];

    //         bool mesmo_promissor = (d0->promissor_id == d1->promissor_id);
    //         bool mesmo_sentido = (d0->sentido == d1->sentido);
            
    //         double delta_arco = fabs(d1->arco_graus - d0->arco_graus);

    //         bool arco_identico = (delta_arco < DELTA_ARCO);

    //         if (mesmo_promissor && mesmo_sentido && arco_identico) {
    //             continue; 
    //         }

    //         i_valido++;
    //         if (i_valido != i_atual) {
    //             lista_resultado[i_valido] = *d1;
    //         }
    //     }

    //     qtd_direcoes = i_valido + 1;
    // }
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
        case TIME_KEY_NAIBOD:                   return NAIBOD_KEY;
        case TIME_KEY_CARDANO:                  return CARDANO_KEY;
        case TIME_KEY_PTOLEMY:                  return PTOLEMY_KEY;
        case TIME_KEY_PLACIDUS:                 return PLACIDUS_KEY;
        case TIME_KEY_TRUE_SOLAR_ARC_LONGITUDE: return obter_chave_arco_solar_ultra_rapida(jd, arco); //obter_chave_arco_solar(jd, arco);           
        case TIME_KEY_KEPLER:                   return obter_chave_kepler_ultra_rapida(jd, arco); //obter_chave_kepler(jd, arco);
        case TIME_KEY_TRUE_SOLAR_ARC_RA:        return encontrar_chave_por_arco_solar_ra_FAST(jd, arco);
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



double calcular_cota_universal(double posicao_domal_swe) {
    int casa_base = (int)posicao_domal_swe;                // Ex: 11
    double fracao = posicao_domal_swe - casa_base;         // Ex: 0.5
    
    // Mapeia linearmente as transições de cúspides do espaço local
    double cota_cuspide_atual = obter_cota_fixa_casa_placidus(casa_base);
    
    int proxima_casa = (casa_base == 12) ? 1 : casa_base + 1;
    double cota_proxima_cuspide = obter_cota_fixa_casa_placidus(proxima_casa);
    
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
    double cota_atual = obter_cota_fixa_casa_placidus(casa_base);
    
    int proxima_casa = (casa_base == 12) ? 1 : casa_base + 1;
    double cota_proxima = obter_cota_fixa_casa_placidus(proxima_casa);

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
            cota_sig_orientada = obter_cota_fixa_casa_placidus(num_casa);
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
                    cota_espacial_fixa = obter_cota_fixa_casa_placidus(num_casa);                    
                }
                else if (sig[idx_alvo].type == PROM_CUSP) {
                    int num_casa = sig[idx_alvo].house;
                    if (num_casa == 1 || num_casa == 4 || num_casa == 7 || num_casa == 10) {
                        eh_angulo_angular = 1;
                        cota_espacial_fixa = obter_cota_fixa_casa_placidus(num_casa);
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
                    // MOTOR PADRÃO: PLACIDUS SEMI-ARCO (Seu Código Original Corrigido)
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
                    else {                                
                        dias_decorridos = d->idade_evento * 365.242199;
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
            LinhaDirecao *d0 = &lista_resultado[i_valido];
            LinhaDirecao *d1 = &lista_resultado[i_atual];

            bool mesmo_promissor = (d0->promissor_id == d1->promissor_id);
            bool mesmo_sentido = (d0->sentido == d1->sentido);
            
            double delta_arco = fabs(d1->arco_graus - d0->arco_graus);

            bool arco_identico = (delta_arco < DELTA_ARCO);

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


    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED, NULL);
    mouseinterval(100);

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

        if (tipo != 1) {
            memset(cronograma_z, 0, sizeof(cronograma_z));
            if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
                qtd_direcoes_zod = calcular_direcoes_zodiacais_topocentrico(sig, idx_atual_calculo, cronograma_z, jd, sentido, prom, prom_id, ramc, lat, &qtd_direcoes_zod, PART_DIRECTIONS);
            }
            else {
                qtd_direcoes_zod = calcular_direcoes_zodiacais_geral(sig, idx_atual_calculo, cronograma_z, jd, sentido, prom, PART_DIRECTIONS);
            }
        }
        if (tipo != 0) {   
            memset(cronograma_m, 0, sizeof(cronograma_m));            
            qtd_direcoes_mun = calcular_direcoes_mundanas_geral(sig, idx_atual_calculo, cronograma_m, jd, ramc, lat, sentido, prom, mundane_aspects);            
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
        mvwprintw(table_win, table_height - 3, 4, _("Mundane Aspects: [1] (☌ ⚹ □ △ ☍ ∥ ∦ R∥ R∦)    │ [2] (☌ △ ☍ ∥ ∦ R∥ R∦)"));
        wattroff(table_win, A_DIM);

        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart ─ [P] Print to File"));

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
        free(cronograma);
    }
    
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr);
    refresh();
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

    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED, NULL);
    mouseinterval(100);

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

        if (tipo != 1) {
            memset(cronograma_z, 0, sizeof(cronograma_z));
            if (METODO_CALCULO_ATIVO == METODO_TOPOCENTRICO) {
                qtd_direcoes_zod = calcular_direcoes_zodiacais_topocentrico(sig, idx_atual_calculo, cronograma_z, jd, sentido, prom, prom_id, ramc, lat, &qtd_direcoes_zod, PART_DIRECTIONS);
            }
            else {
                qtd_direcoes_zod = calcular_direcoes_zodiacais_geral(sig, idx_atual_calculo, cronograma_z, jd, sentido, prom, PART_DIRECTIONS);
            }
        }
        if (tipo != 0) {   
            memset(cronograma_m, 0, sizeof(cronograma_m));            
            qtd_direcoes_mun = calcular_direcoes_mundanas_geral(sig, idx_atual_calculo, cronograma_m, jd, ramc, lat, sentido, prom, mundane_aspects);            
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
        mvwprintw(table_win, table_height - 3, 4, _("Mundane Aspects: [1] (☌ ⚹ □ △ ☍ ∥ ∦ R∥ R∦)    │ [2] (☌ △ ☍ ∥ ∦ R∥ R∦)"));
        wattroff(table_win, A_DIM);

        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart ─ [P] Print to File"));

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
            case KEY_RIGHT:
                seletor_alvo_atual = (seletor_alvo_atual + 1) % (qtd_partes);
                scroll_offset = 0;
                break;
            case KEY_LEFT:
                seletor_alvo_atual = (seletor_alvo_atual - 1 + qtd_partes) % (qtd_partes);
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
        free(cronograma);
    }

    free(sig);
    
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

    double angulos_aspectos[] = {0.0, 60.0, -60.0, 90.0, -90.0, 120.0, -120.0, 180.0, 999.9, 999.9};
    char *simbolos_aspectos[] = {"☌", "⚹", "⚹", "□", "□", "△", "△", "☍", "∥", "∦"};

    for (int p = 0; p < prom_id; p++) {
        if (prom[p].type == PROM_POINT || prom[p].type == PROM_ANGLE || prom[p].type == PROM_PART) continue;
        if ((prom[p].type == PROM_ANTISCIUM || prom[p].type == PROM_CONTRANTISCIUM) && !ANT_PROM) continue;

        for (int s = 0; s < 2; s++) {

            //if (prom[p].type == PROM_TERM && s == 1) continue;

            for (int a = 0; a < 10; a++) {
                
                if (prom[p].type == PROM_TERM && a > 0) break; // apenas conjunções para termos

                double arco = 0.0;
                int eh_paralelo = (a == 8 || a == 9);

                if (eh_paralelo) {
                    if (s == 1) continue; // Evita duplicidade de sentido para paralelos

                    // Declinação que o PROMISSOR precisa alcançar
                    double dec_alvo_paralelo = (a == 8) ? dec_significador : -dec_significador;

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
                            
                            d->promissor_id = p;
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
                            else {                                
                                dias_decorridos = d->idade_evento * 365.242199;
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
                    
                    d->promissor_id = p;
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
                    else {                                
                        dias_decorridos = d->idade_evento * 365.242199;
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
                    if (qtd_direcoes >= 600) return qtd_direcoes;
                }
            }
        }
    }
fim_calculo:
    qsort(lista_resultado, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);
    return qtd_direcoes;
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

        fprintf(file, "%8.4f;%02d/%02d/%04d;%s;%s;%s;%s;%s;%8.4f%s;%s;%s;%s;%s\n",
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
                       "°",
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






int calcular_direcoes_mundanas_partes(ArabicPartCalculada *parts, int idx_alvo, LinhaDirecao *lista_resultado, double jd, double ramc, double lat_geografica, int sentido, Promissor *prom) {
    int qtd_direcoes = 0;
    double lat_geo_rad = para_radianos(lat_geografica);

    //if (idx_alvo < 0 || idx_alvo >= NUM_OBJECTS) return 0;

    double dec_out, ra_out;
    calc_declination_ra_point(jd, parts[idx_alvo].longitude, &ra_out, &dec_out);
    double ra_sig = ra_out;

    double dec_sig_rad = para_radianos(dec_out); //para_radianos(calc_declination_mathematical_point(jd, parts[idx_alvo].longitude));
    
    int sig_acima = verificar_se_acima_horizonte(ra_sig, dec_sig_rad, ramc, lat_geo_rad); 
    
    double sa_sig = __calcular_semi_arco(dec_sig_rad, lat_geo_rad, sig_acima);
    
    
    double proporcao_aspecto[] = {0.0, 0.66666667, -0.66666667, 1.0, -1.0, 1.33333333, -1.33333333, 2.0, 999.9, 999.9}; 
    char *simbolos_aspectos[] = {"☌", "⚹", "⚹", "□", "□", "△", "△", "☍", "∥", "∦"};

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

            for (int a = 0; a < 10; a++) {
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
                if (a == 8 || a == 9) {
                    // Alvo geométrico: mesma cota (paralelo) ou cota invertida (contraparalelo)
                    double cota_alvo_mundo = (a == 8) ? cota_sig_orientada : -cota_sig_orientada;

                    if (s == 0) { // Direção Direta
                        // O promissor se move até atingir a proporção mundana do significador
                        double md_destino = sa_prom * cota_alvo_mundo;
                        arco = md_prom_com_sinal - md_destino;
                    } 
                    else if (s == 1) { // Direção Conversa
                        // O significador se move até atingir a proporção mundana do promissor
                        double cota_prom_natal = md_prom_com_sinal / sa_prom;
                        // Ajusta o sinal para o espelhamento converso do contraparalelo
                        if (a == 9) cota_prom_natal = -cota_prom_natal; 
                        
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

                    d->promissor_id = p;
                    strcpy(d->promissor_name, prom[p].object_name);
                    strcpy(d->promissor_glifo, prom[p].object);
                    strcpy(d->aspecto_symbol, simbolos_aspectos[a]);
                    strcpy(d->significador_name, parts[idx_alvo].name);

                    char abreviacao[10];
                    get_part_abbreviation(parts[idx_alvo].name, abreviacao);
            
                    
                    strcpy(d->significador_glifo, abreviacao);
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
                    else {                                
                        dias_decorridos = d->idade_evento * 365.242199;
                    }
            
                    // 4. Projeção Estrita no Dia Juliano e Conversão UTC via Swiss Ephemeris
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
                    if (qtd_direcoes >= 600) goto fim_calculo;
                }
            }

        }
    }

fim_calculo:
    qsort(lista_resultado, qtd_direcoes, sizeof(LinhaDirecao), comparar_directions_por_idade);
    return qtd_direcoes;
}
