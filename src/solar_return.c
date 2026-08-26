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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "swephexp.h" // Cabeçalho da Swiss Ephemeris
#include <string.h>
#include <ncursesw/curses.h>
#include "var.h"
#include "helper.h"
#include "draw-chart.h"
#include "planet_table.h"
#include "db-utils.h"
#include "aspects.h"
#include "fuso-horario.h"
#include "solar_return.h"
#include "firdaria.h"
#include "profections.h"
#include "hyleg.h"


struct tm obter_tempo_local_revolucao(double jd_revolucao_ut, double fuso_horario_destino) {
    int ano, mes, dia, horas, minutos;
    double segundos_decimal;
    struct tm t_local;

    /* 1. Ajusta o Julian Day UTC para o Horário Local do destino
          Exemplo: Se o fuso for -3.0 (Brasília), subtrai 3 horas do JD.
          Se o fuso tiver horário de verão (+1h), passe -2.0 ao invés de -3.0 */
    double jd_local = jd_revolucao_ut + (fuso_horario_destino / 24.0);

    /* 2. Extrai os dados civis a partir do JD já localizado */
    swe_jdut1_to_utc(jd_local, SE_GREG_CAL, &ano, &mes, &dia, &horas, &minutos, &segundos_decimal);

    /* 3. Arredondamento e preenchimento da struct tm */
    int segundos = (int)(segundos_decimal + 0.5);

    t_local.tm_year  = ano - 1900;
    t_local.tm_mon   = mes - 1;
    t_local.tm_mday  = dia;
    t_local.tm_hour  = horas;
    t_local.tm_min   = minutos;
    t_local.tm_sec   = segundos;
    t_local.tm_isdst = -1; 

    /* 4. Normaliza os dias da semana/ano sem alterar o fuso */
    mktime(&t_local); 

    return t_local;
}


double calcular_julian_day_retorno_solar(double jd_nascimento, int idade_selecionada, double sol_natal_exibido) {
    int ano, mes, dia;
    double hora;
    
    // CORREÇÃO: Declarado como array de strings para receber a mensagem de erro da SE
    char err_msg[256]; 
    
    // Flags padrões do sistema
    int32 iflag = SEFLG_SPEED | SEFLG_SWIEPH; 

    // Descobre a data civil de nascimento em UTC
    swe_revjul(jd_nascimento, SE_GREG_CAL, &ano, &mes, &dia, &hora);
    
    // Projeta o ano alvo da Revolução Solar
    int ano_rev = ano + idade_selecionada;

    // Cria a estimativa inicial no ano alvo
    double jd_estimado_ut = swe_julday(ano_rev, mes, dia, hora, SE_GREG_CAL);

    // Chamada oficial usando o ponteiro do array corrigido
    double jd_revolucao_ut = 0.0;
    if ((jd_revolucao_ut = swe_solcross_ut(sol_natal_exibido, jd_estimado_ut, iflag, err_msg)) < 0) {
        // Fallback de segurança caso a função da biblioteca falhe
        return jd_estimado_ut;
    }

    return jd_revolucao_ut; 
}



// /**
//  * Calcula o Dia Juliano exato do retorno solar (Revolução Solar) para uma determinada idade.
//  * 
//  * @param jd_nascimento      O Dia Juliano (UT) do momento do nascimento.
//  * @param idade_selecionada  A idade para a qual deseja-se calcular o retorno.
//  * @return                   O Dia Juliano (UT) do momento exato da Revolução Solar.
//  */
// double calc_julian_day_retorno_solar(double jd_nascimento, int idade_selecionada) {
//     char err_msg[256];
//     double x2[6];
//     int32_t flags = SEFLG_SPEED; // Necessário para obter a velocidade do Sol

//     // 1. Obter a longitude solar exata do nascimento
//     if (swe_calc_ut(jd_nascimento, SE_SUN, flags, x2, err_msg) == ERR) {
//         fprintf(stderr, "Erro ao calcular Sol do nascimento: %s\n", err_msg);
//         return -1.0; // Retorna erro caso falhe
//     }
//     double lon_nascimento = x2[0];

//     // 2. Estimar o Dia Juliano alvo aproximado (Ano Trópico médio ~365.242199 dias)
//     double jd_calculado = jd_nascimento + (idade_selecionada * 365.242199);

//     // 3. Refinar o momento usando o método de Newton-Raphson
//     const double TOLERANCIA = 1e-7; // Alta precisão (fração de segundos de arco)
//     const int MAX_ITERACOES = 15;
//     int iteracoes = 0;
//     double erro = 1.0;

//     while (fabs(erro) > TOLERANCIA && iteracoes < MAX_ITERACOES) {
//         // Calcula a posição e a velocidade do Sol para o dia estimado atual
//         if (swe_calc_ut(jd_calculado, SE_SUN, flags, x2, err_msg) == ERR) {
//             fprintf(stderr, "Erro na iteração %d: %s\n", iteracoes, err_msg);
//             return -1.0;
//         }

//         double lon_atual = x2[0];
//         double velocidade_atual = x2[3]; // x2[3] contém a velocidade em graus por dia

//         // Calcula a diferença angular (erro)
//         erro = lon_atual - lon_nascimento;

//         // Normaliza a diferença para o intervalo [-180, 180] para lidar com a virada de 360° para 0°
//         erro = fmod(erro, 360.0);
//         if (erro > 180.0) erro -= 360.0;
//         if (erro < -180.0) erro += 360.0;

//         // Ajusta o Dia Juliano: tempo = erro_angular / velocidade_angular
//         jd_calculado -= erro / velocidade_atual;

//         iteracoes++;
//     }

//     return jd_calculado;
// }

/**
 * Calcula o Dia Juliano exato do retorno solar (Revolução Solar) para qualquer período histórico.
 * Funciona perfeitamente para séculos passados, corrigindo variações temporais.
 * 
 * @param jd_nascimento      O Dia Juliano (UT) do momento do nascimento.
 * @param idade_selecionada  A idade para a qual deseja-se calcular o retorno.
 * @return                   O Dia Juliano (UT) do momento exato da Revolução Solar.
 */
double calc_julian_day_retorno_solar(double jd_nascimento, int idade_selecionada) {
    char err_msg[256];
    double x2[6];
    
    // SEFLG_SPEED: Obrigatório para velocidade.
    // SEFLG_SWIEPH: Tenta usar os arquivos de alta precisão da AstroDienst se disponíveis.
    int32_t flags = SEFLG_SPEED | SEFLG_SWIEPH; 

    // 1. Obter a longitude solar exata do nascimento
    if (swe_calc_ut(jd_nascimento, SE_SUN, flags, x2, err_msg) == ERR) {
        // Se falhar por falta de arquivos físicos de efemérides do século passado,
        // tenta rodar com o modelo analítico de Moshier (nativo da biblioteca).
        flags = SEFLG_SPEED | SEFLG_MOSEPH;
        if (swe_calc_ut(jd_nascimento, SE_SUN, flags, x2, err_msg) == ERR) {
            fprintf(stderr, "Erro crítico ao calcular Sol do nascimento: %s\n", err_msg);
            return -1.0;
        }
    }
    double lon_nascimento = x2[0];

    // 2. Estimar o Dia Juliano alvo aproximado
    // Em mapas muito antigos, a duração do ano trópico desvia levemente.
    double jd_calculado = jd_nascimento + (idade_selecionada * 365.242199);

    // --- SALVAGUARDA HISTÓRICA ---
    // Verifica se a estimativa inicial não caiu no mês/ano errado devido a desvios históricos
    // Fazemos um pré-ajuste grosseiro se o erro inicial for maior que 5 graus.
    if (swe_calc_ut(jd_calculado, SE_SUN, flags, x2, err_msg) != ERR) {
        double erro_grosseiro = x2[0] - lon_nascimento;
        erro_grosseiro = fmod(erro_grosseiro, 360.0);
        if (erro_grosseiro > 180.0)  erro_grosseiro -= 360.0;
        if (erro_grosseiro < -180.0) erro_grosseiro += 360.0;
        
        // Se a estimativa estiver desalinhada por mais de 5 graus (um erro de ~5 dias),
        // ajustamos linearmente antes de entrar no refinamento fino.
        if (fabs(erro_grosseiro) > 5.0) {
            jd_calculado -= erro_grosseiro; // Ajuste inicial em dias aproximados
        }
    }

    // 3. Refinar o momento usando o método de Newton-Raphson (Alta Precisão)
    const double TOLERANCIA = 1e-7; // Equivalente a milissegundos de tempo
    const int MAX_ITERACOES = 15;
    int iteracoes = 0;
    double erro = 1.0;

    while (fabs(erro) > TOLERANCIA && iteracoes < MAX_ITERACOES) {
        if (swe_calc_ut(jd_calculado, SE_SUN, flags, x2, err_msg) == ERR) {
            fprintf(stderr, "Erro na iteração histórica %d: %s\n", iteracoes, err_msg);
            return -1.0;
        }

        double lon_atual = x2[0];
        double velocidade_atual = x2[3]; // Velocidade diária real do Sol naquele século/dia

        // Calcula a diferença angular
        erro = lon_atual - lon_nascimento;

        // Normalização cíclica de 360 graus
        erro = fmod(erro, 360.0);
        if (erro > 180.0)  erro -= 360.0;
        if (erro < -180.0) erro += 360.0;

        // Aplica correção no Dia Juliano
        jd_calculado -= erro / velocidade_atual;

        iteracoes++;
    }

    return jd_calculado;
}


/* Certifique-se de passar jd_natal e jd_revolucao para a assinatura da função */
void processar_confronto_natal_revolucao(
    int id_almuten_rev,               
    double longitude_almuten_rev,     
    double latitude_almuten_rev,      
    int dignidade_natal,           
    double lat_natal,                 
    double armc_natal,                
    int id_senhor_profeccao,          
    int id_senhor_firdaria,           
    int id_senhor_subfirdaria,
    double asc_revolucao,
    int *strength_planets,
    double jd_natal,                  /* ADICIONADO: JD real do Nascimento */
    double jd_revolucao_ut,
    double armc_rev,
    double lat_rev,
    double asc_natal)           /* ADICIONADO: JD real da Revolução Solar */
{
    char serr[256];
    double xx[6];
    double eps_natal;
    double eps_rev;

    // 1. OBLIQUIDADE DO NASCIMENTO (Para o Almuten)
    if (swe_calc_ut(jd_natal, SE_ECL_NUT, SEFLG_SWIEPH, xx, serr) >= 0) {
        eps_natal = xx[0]; 
    } else {
        eps_natal = 23.439291; 
    }

    // 2. OBLIQUIDADE DA REVOLUÇÃO (Para o Ascendente do Ano)
    if (swe_calc_ut(jd_revolucao_ut, SE_ECL_NUT, SEFLG_SWIEPH, xx, serr) >= 0) {
        eps_rev = xx[0]; 
    } else {
        eps_rev = 23.439291; 
    }

    /* 3. PROJEÇÃO DO ALMUTEN NAS CASAS NATAIS
          Trocado 'P' pela sua variável global HOUSE_SYSTEM */
    double xpin_almuten[2];
    xpin_almuten[0] = longitude_almuten_rev;
    xpin_almuten[1] = latitude_almuten_rev;

    double casa_alm_dec = swe_house_pos(armc_natal, lat_natal, eps_natal, (int)HOUSE_SYSTEM, xpin_almuten, serr);
    int casa_natal_transitada = (int)floor(casa_alm_dec);
    if (casa_natal_transitada < 1 || casa_natal_transitada > 12) casa_natal_transitada = 1;

    /* 4. PROJEÇÃO DO ASCENDENTE DA REVOLUÇÃO NAS CASAS NATAIS
          Agora usando o eps_rev correto e a global HOUSE_SYSTEM */
    double xpin_asc[2];
    xpin_asc[0] = asc_revolucao; 
    xpin_asc[1] = 0.0;           

    double casa_asc_dec = swe_house_pos(armc_natal, lat_natal, eps_rev, (int)HOUSE_SYSTEM, xpin_asc, serr);
    int casa_natal_do_asc = (int)floor(casa_asc_dec);
    if (casa_natal_do_asc < 1 || casa_natal_do_asc > 12) casa_natal_do_asc = 1;

    double xpin_asc_nat[2];
    xpin_asc_nat[0] = asc_natal; 
    xpin_asc_nat[1] = 0.0;
    double casa_rev_asc_dec = swe_house_pos(armc_rev, lat_rev, eps_natal, (int)HOUSE_SYSTEM, xpin_asc_nat, serr);
    int casa_rev_do_asc_natal = (int)floor(casa_rev_asc_dec);
    if (casa_rev_do_asc_natal < 1 || casa_rev_do_asc_natal > 12) casa_rev_do_asc_natal = 1;

    int pontuacao_dignidade_natal = dignidade_natal;
    int aproveitamento_almuten = strength_planets[id_almuten_rev - 1];

    // DISPARA A JANELA VISUAL COM OS INTEIROS CALCULADOS SOB PRECISÃO MÁXIMA
    abrir_janela_confronto_natal_revolucao(
        id_almuten_rev, 
        pontuacao_dignidade_natal, 
        casa_natal_transitada, 
        id_senhor_profeccao,
        id_senhor_firdaria,
        id_senhor_subfirdaria,
        casa_natal_do_asc,
        aproveitamento_almuten,
        casa_rev_do_asc_natal
    );
}



void disparar_revolucao_solar(double julian_day, char *chart_name, double *cusps_natal, bool mapa_diurno, double lat_natal, double armc, PlanetDignities *dig, char *nome_anareta_natal, char *nome_s8_natal, int tipo_h_natal, int idx_hyleg_natal, double *longitudes_natal, int *strength_planets) {
    
    double idade_padrao = obter_idade_padrao_mapa_double();
    double idade_escolhida = selecionar_idade_visual_fracionada(idade_padrao);
    if (idade_escolhida < 0) return; // Usuário cancelou

    // 2. Cria variáveis LOCAIS para armazenar o local do Retorno Solar
    char cidade_retorno[100];
    char country_retorno[100];
    char state_retorno[100];
    char tz_iana_retorno[100];
    double tz_offset_retorno = 0.0;

    double lat = 0.0, lon = 0.0, elev = 0.0;
    
    
    if (!show_confirm_yesno(CITY, _("Keep the same city"))) {    
        if (!load_city_coordinates(cidade_retorno, country_retorno, state_retorno, tz_iana_retorno, &tz_offset_retorno, &lat, &lon, &elev)) {
            return;
        }
    }
    else {

        get_coordinates(CITY, COUNTRY, STATE, &lat, &lon, &elev, &tz_offset_retorno, tz_iana_retorno);

        snprintf(cidade_retorno, 100, "%s", CITY);
        snprintf(country_retorno, 100, "%s", COUNTRY);
        snprintf(state_retorno, 100, "%s", STATE);
    }
    
    

    /* Roda o motor atualizado passando o double estável */
    RelatorioFirdaria fird = processar_dados_firdaria(idade_escolhida, mapa_diurno);


    // Obtem os dados da Firdária e Profecção
    //RelatorioFirdaria fird = processar_dados_firdaria(idade_escolhida, mapa_diurno);
    DadosProfeccao prof = calcular_profeccao_anual(cusps_natal[1], (int)floor(idade_escolhida));


    // 3. Calcula o Julian Day exato do Retorno Solar (usando a função astronômica)
    double jd_natal = julian_day;
    


    // 1. Calcula o Julian Day do Retorno Solar
    
    double jd_retorno = calc_julian_day_retorno_solar(jd_natal, (int)floor(idade_escolhida));

    // 2. Gera a struct tm compatível
    struct tm tempo_retorno = julian_day_para_struct_tm(jd_retorno);
    
    struct tm tempo_local = obter_tempo_local_revolucao(jd_retorno, tz_offset_retorno);

   
    // 3. Monta a string do nome do gráfico (Ex: "Retorno Solar 2026")
    char novo_nome_chart[128];
    snprintf(novo_nome_chart, sizeof(novo_nome_chart), _("%s - Solar Return (%d)"), chart_name, tempo_retorno.tm_year + 1900);


    double dst_offset = 0.0;
    dst_offset = (double)obter_segundos_dst_na_data(tz_iana_retorno, tempo_retorno.tm_year + 1900, tempo_retorno.tm_mon + 1, tempo_retorno.tm_mday, tempo_retorno.tm_hour, tempo_retorno.tm_min) / 3600; 

    tz_offset_retorno += dst_offset;
    // 4. Dispara a sua função chart de forma recursiva passando os dados locais do retorno
    // O seu parâmetro 'chart_name' receberá o 'novo_nome_chart'
    chart(
        &tempo_local, 
        lat, 
        lon, 
        elev, 
        tz_offset_retorno, 
        cidade_retorno, 
        country_retorno, 
        false,
        0, 
        novo_nome_chart, 
        HOUSE_SYSTEM, 
        GENDER, 
        DARK_MODE,
        true,
        prof.id_senhor_do_ano,
        fird.id_major,
        fird.id_sub,
        armc,
        lat_natal,
        dig,
        nome_anareta_natal,
        nome_s8_natal,
        tipo_h_natal,
        idx_hyleg_natal,
        longitudes_natal,
        julian_day,
        strength_planets,
        cusps_natal[1],
        cusps_natal
    );

}



void abrir_janela_confronto_natal_revolucao(
    int id_almuten_rev, 
    int pontuacao_dignidade_natal, 
    int casa_natal_transitada, 
    int id_senhor_profeccao, 
    int id_senhor_firdaria, 
    int id_senhor_subfirdaria,
    int casa_natal_do_asc,
    int aproveitamento_almuten,
    int casa_rev_do_asc_natal) /* RECEBE O INTEIRO JÁ PRONTO */
{
    int p_max_y, p_max_x;
    getmaxyx(stdscr, p_max_y, p_max_x); 

    // 1. DIMENSIONAMENTO RESPONSIVO
    int i_height = p_max_y - 6;
    if (i_height > 26) i_height = 26; 
    int i_width = p_max_x - 12;
    if (i_width > 104) i_width = 104;   

    int i_start_y = (p_max_y - i_height) / 2;
    int i_start_x = (p_max_x - i_width) / 2;

    // 2. RENDERIZAÇÃO DA SOMBRA (FUNDO)
    WINDOW *shadow_win = newwin(i_height, i_width, i_start_y + 1, i_start_x + 1);
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9)); 
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    // 3. MOLDURA PRINCIPAL
    WINDOW *border_win = newwin(i_height, i_width, i_start_y, i_start_x);
    wbkgd(border_win, COLOR_PAIR(13));
    box(border_win, 0, 0);
    
    wattron(border_win, A_BOLD);
    const char *title = _(" Radix Confrontation: Solar Return Integration ");
    mvwprintw(border_win, 0, (i_width - get_visual_width(title)) / 2, title);
    wattroff(border_win, A_BOLD);
    
    mvwprintw(border_win, i_height - 1, (i_width - 44) / 2, _(" [↓↑|JK: Scroll | Q|ESC: Return to Chart] "));
    wrefresh(border_win);

    // 4. PAD INTERNA PARA SCROLL
    int pad_lines = 120; // Aumentado para comportar o texto da Firdária confortavelmente
    int pad_cols = i_width - 6; 
    WINDOW *pad = newpad(pad_lines, pad_cols);
    wbkgd(pad, COLOR_PAIR(13));
    keypad(pad, TRUE);
    
    char str_text[512];

    wprintw(pad, "\n\n"); 

    // --- CABEÇALHO DO CONFRONTO ---
    wattron(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, _("  THE GOLDEN RADIX RULE: CONFRONTING THE LORD OF THE YEAR\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    snprintf(str_text, 256, "%s", _("    According to Hellenistic and Medieval tradition, the Solar Return Almuten "
                 "cannot be interpreted in a vacuum. Its promises are deeply filtered by its "
                 "fundamental condition in your Birth Chart (Radix) and its structural "
                 "cross-transits.\n\n"));

    imprimir_texto_fluxo(pad, 4, 80, str_text);
    wprintw(pad, "\n\n");

    const char *nomes_planetas[] = {"", _("SUN ☉"), _("MOON ☽"), _("MERCURY ☿"), _("VENUS ♀"), _("MARS ♂"), _("JUPITER ♃"), _("SATURN ♄")};
    
    
    // --- VERIFICAÇÃO 1: FILTRO DE APROVEITAMENTO UNIVERSAL ESPELHADO ---
    wattron(pad, COLOR_PAIR(10) | A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattroff(pad, COLOR_PAIR(10) | A_DIM);

    wattron(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    wprintw(pad, _("  [CHECK I] RADIX DIGNITY & STRUCTURAL EFFICIENCY FILTER\n\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    
    /* Calcula os pontos ponderados reais apenas para a string informativa do texto */
    double weights[50];
    get_weights(weights, show_modern_planets);
    int pontos_finais_exibicao = (int)ceil(((double)aproveitamento_almuten * weights[id_almuten_rev]) / 10.0);

    wprintw(pad, _("    The Lord of the Year is the %s. In your Natal Chart, its base dignity score is: %d.\n"
                    "    Its relative cosmic efficiency is: %d%% (Resulting in %d Net Strength Points).\n\n"), 
            nomes_planetas[id_almuten_rev], pontuacao_dignidade_natal, aproveitamento_almuten, pontos_finais_exibicao);
    
    wprintw(pad, _("    Structural Efficiency Verdict:\n\n"));

    // Julgamento por porcentagem pura e justa: Mercúrio com 83% fica verde!
    if (aproveitamento_almuten >= 65) {
        wattron(pad, A_BOLD | A_REVERSE | COLOR_PAIR(12)); // Excelente / Verde
        wprintw(pad, _("    • HIGH OPERATIONAL CAPACITY (EXCELLENT CHAPTER):\n\n"));
        wattroff(pad, A_BOLD | A_REVERSE | COLOR_PAIR(12));
        snprintf(str_text, 512, _("       This planet commands the year with magnificent backing from your birth chart.\n"
                        "       Because its cosmic efficiency is highly abundant (%d%%), it acts as an honored\n"
                        "and powerful executive. The promises of this Solar Return will manifest with clarity, "
                        "bringing structural progress, sudden expansion, and minimal friction.\n\n\n"), aproveitamento_almuten);
    } 
    else if (aproveitamento_almuten >= 35) {
        wattron(pad, A_BOLD | COLOR_PAIR(8)); // Moderado / Azul
        wprintw(pad, _("    • MODERATE OPERATIONAL CAPACITY (BALANCED CHAPTER):\n\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(8));
        snprintf(str_text, 512, _("       This planet holds average, stable ground in your baseline blueprint (%d%%).\n"
                        "       It possesses the standard authority to execute its functions, but will demand steady "
                        "discipline and continuous focus from you. Events will unfold normally, tracking your "
                        "real-world daily effort without extraordinary windfalls or sudden structural collapses.\n\n\n"), aproveitamento_almuten);
    } 
    else {
        wattron(pad, A_BOLD | COLOR_PAIR(11)); // Crítico / Vermelho
        wprintw(pad, _("    • CRITICAL CAPACITY DRAIN (MUTED OR IMPEDED CHAPTER):\n\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(11));
        snprintf(str_text, 512, _("       WARNING: The Lord of the Year operates under extreme systemic debility (%d%%).\n"
                        "       Even though it governs the time stream of this anniversary, it lacks the raw vital "
                        "resources to fulfill its promises easily. The sectors it triggers this year will demand "
                        "intense adjustments, manifesting through chronic delays, heavy exhaustion, "
                        "administrative blocks, or the feeling of working against a locked door.\n\n\n"), aproveitamento_almuten);
    }

    imprimir_texto_fluxo(pad, 4, 80, str_text);
    wprintw(pad, "\n");
    wprintw(pad, "\n");

    // --- VERIFICAÇÃO 2: A POSIÇÃO POR CASA RADICAL ---
    wattron(pad, COLOR_PAIR(10) | A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattroff(pad, COLOR_PAIR(10) | A_DIM);

    wattron(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    wprintw(pad, _("  [CHECK II] RADIX HOUSE TRANSIT\n\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    wprintw(pad, _("    The Solar Return Almuten is currently transiting through your NATAL HOUSE %d.\n\n"), casa_natal_transitada);
    
    wattron(pad, A_BOLD );
    wprintw(pad, _("    Interpretation:\n\n"));
    wattroff(pad, A_BOLD );
    if (casa_natal_transitada == 1) {
        snprintf(str_text, 512, _("    The lens focuses strictly on your physical body, personal vitality, "
                     "and identity.\n"
                     "    A year to actively reinvent yourself and take direct command of your "
                     "path.\n\n\n"));
    } else if (casa_natal_transitada == 2) {
        snprintf(str_text, 512, _("    The core theme will revolve entirely around your personal resources, "
                     "finances, and material possessions. Events will force a heavy evaluation "
                     "of security and income.\n\n\n"));
    } else if (casa_natal_transitada == 6 || casa_natal_transitada == 8 || casa_natal_transitada == 12) {
        snprintf(str_text, 512, _("    The planet activates an obscure house of your radix. Focus shifts toward "
                     "heavy adjustments, health maintenance, administrative debts, or psychological "
                     "shedding and deep spiritual isolation.\n\n\n"));
    } else if (casa_natal_transitada == 10) {
        snprintf(str_text, 512, _("    The cosmic spotlight hits your professional destiny, career elevation, "
                     "and social standing. Major events will directly reshape your public reputation "
                     "and authority.\n\n\n"));
    } else {
        snprintf(str_text, 512, _("    This alignment directly activates the social, relational, and communicative "
                     "sectors of your radix, driving daily encounters and local environment shifting "
                     "over the next 12 months.\n\n\n"));
    }

    imprimir_texto_fluxo(pad, 4, 80, str_text);
    wprintw(pad, "\n");
    wprintw(pad, "\n");

    // --- VERIFICAÇÃO 3: CONDIÇÃO DO SENHOR DA PROFECÇÃO ---
    wattron(pad, COLOR_PAIR(10) | A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattroff(pad, COLOR_PAIR(10) | A_DIM);

    wattron(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    wprintw(pad, _("  [CHECK III] THE TIMELORD CO-ALIGNMENT\n\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    wprintw(pad, _("    The current Profection Lord of the Year is: %s.\n"
                 "    The current Solar Return Almuten is: %s.\n\n"), 
            nomes_planetas[id_senhor_profeccao], nomes_planetas[id_almuten_rev]);
    
    wattron(pad, A_BOLD );
    wprintw(pad, _("    Interpretation:\n\n"));
    wattroff(pad, A_BOLD );
    if (id_almuten_rev == id_senhor_profeccao) {
        wattron(pad, A_BOLD | COLOR_PAIR(11));
        wprintw(pad, _("    CRITICAL YEAR CRITERIA MATCH: FATAL EVENTS AHEAD.\n\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(11));
        snprintf(str_text, 512, _("    The Lord of the Return is the EXACT same planet ruling your profection "
                     "time stream!\n"
                     "    In traditional astrology, this synchronization indicates a highly "
                     "turning-point year.\n"
                     "    The planet gains double cosmic authorization. The events scheduled under "
                     "its watch are unavoidable, highly prominent, and will actively reshape your "
                     "life history.\n\n\n"));
    } else {
        snprintf(str_text, 512, _("    Standard Alignment. The Return Lord and the Profection Lord are operating "
                     "under distinct frequencies. This distributes your energy evenly, allowing you to "
                     "manage professional matters and internal shifts along separate, parallel tracks "
                     "without overwhelming intensity.\n\n\n"));
    }

    imprimir_texto_fluxo(pad, 4, 80, str_text);
    wprintw(pad, "\n");
    wprintw(pad, "\n");


    // --- VERIFICAÇÃO 4: O ALINHAMENTO CRONOCRÁTICO DAS FIRDÁRIAS ---
    wattron(pad, COLOR_PAIR(10) | A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattroff(pad, COLOR_PAIR(10) | A_DIM);

    wattron(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    wprintw(pad, _("  [CHECK IV] THE FIRDARIA CHRONOCRATOR ALIGNMENT\n\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    wprintw(pad, _("    Current Firdaria Master Ruler: %s\n"
                 "    Current Firdaria Sub-Ruler: %s\n"
                 "    Solar Return Almuten (Lord of Year): %s\n\n"), 
            nomes_planetas[id_senhor_firdaria], 
            nomes_planetas[id_senhor_subfirdaria],
            nomes_planetas[id_almuten_rev]);
    
    wattron(pad, A_BOLD );
    wprintw(pad, _("    Interpretation:\n"));
    wattroff(pad, A_BOLD );
    if (id_almuten_rev == id_senhor_firdaria) {
        wattron(pad, A_BOLD | COLOR_PAIR(12)); 
        snprintf(str_text, 512, _("    MAJOR CHRONOCRATOR ALIGNMENT DETECTED.\n\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(12));
        snprintf(str_text, 512, _("The Lord of the Year is also the supreme ruler of your current Firdaria cycle! "
                     "In traditional astrology, this means the planet has total systemic harmony. The events "
                     "it promises this year are backed by the macro-cyclical trend of your life, bringing "
                     "profound, lasting developments that perfectly fulfill your current life chapter.\n\n\n"));
    } 
    else if (id_almuten_rev == id_senhor_subfirdaria) {
        wattron(pad, A_BOLD | COLOR_PAIR(1)); 
        snprintf(str_text, 512, _("    SUB-FIRDARIA ALIGNMENT DETECTED.\n\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(1));
        snprintf(str_text, 512, _("The Lord of the Year coordinates directly with your current Firdaria sub-period. "
                     "This indicates that the events of the next 12 months will act as the perfect trigger "
                     "to release the potential promised by the current sub-ruler in your birth chart. "
                     "Expect a highly focused, active year regarding this planet's themes.\n\n\n"));
    } 
    else {
        snprintf(str_text, 512, _("Parallel Current. The Lord of the Year operates on a distinct energetic line from "
                     "the active Firdaria rulers. This implies that while the Firdaria manages long-term "
                     "background developments in your life, the Almuten of the Return will bring immediate, "
                     "practical tasks and events that keep you busy on a day-to-day level without disrupting "
                     "the macro-cycle.\n\n\n"));
    }

    imprimir_texto_fluxo(pad, 4, 80, str_text);
    wprintw(pad, "\n");
    wprintw(pad, "\n");

    // --- VERIFICAÇÃO 5: PROJEÇÃO DO ASCENDENTE DA REVOLUÇÃO ---
    wattron(pad, COLOR_PAIR(10) | A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattroff(pad, COLOR_PAIR(10) | A_DIM);

    wattron(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    wprintw(pad, _("  [CHECK V] SOLAR RETURN ASCENDANT PROJECTION\n\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    
    /* Agora a impressão é direta, limpa e imune a falhas de compilação */
    wprintw(pad, _("    The Solar Return Ascendant falls into your NATAL HOUSE %d.\n\n"), casa_natal_do_asc);
    wattron(pad, A_BOLD );
    wprintw(pad, _("    Interpretation:\n\n"));
    wattroff(pad, A_BOLD );
    
    if (casa_natal_do_asc == 1) {
        snprintf(str_text, 512, _("    A year of absolute self-empowerment. Your personal choices, physical vitality, "
                     "and individual projects take absolute priority over external demands.\n\n\n"));
    } else if (casa_natal_do_asc == 4) {
        snprintf(str_text, 512, _("    Focus on foundations, family, and home life. Events will deeply affect your "
                     "domestic environment, property management, or ancestral roots.\n\n\n"));
    } else if (casa_natal_do_asc == 7) {
        snprintf(str_text, 512, _("    The year centers on partnerships, contracts, and legal matters. Relationships "
                     "(both romantic and professional) will face testing and restructuring.\n\n\n"));
    } else if (casa_natal_do_asc == 10) {
        snprintf(str_text, 512, _("    A powerful professional window. Your career, public standing, and long-term "
                     "ambitions are directly activated, pushing you into positions of authority.\n\n\n"));
    } else {
        snprintf(str_text, 512, _("    This house activation indicates that your immediate daily affairs, social circles, "
                     "or resource management will absorb your primary energetic focus this year.\n\n\n"));
    }

    imprimir_texto_fluxo(pad, 4, 80, str_text);
    wprintw(pad, "\n");
    wprintw(pad, "\n");

    wattron(pad, COLOR_PAIR(10) | A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattroff(pad, COLOR_PAIR(10) | A_DIM);

    wattron(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    wprintw(pad, _("  [CHECK VI] NATAL ASCENDANT PROJECTION IN SOLAR RETURN\n\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    
    /* Substitua 'casa_sr_do_natal' pela sua variável de cálculo */
    wprintw(pad, _("    Your Natal Ascendant falls into your SOLAR RETURN HOUSE %d.\n\n"), casa_rev_do_asc_natal);
    wattron(pad, A_BOLD);
    wprintw(pad, _("    Interpretation:\n\n"));
    wattroff(pad, A_BOLD);

    wprintw(pad, "\n");
    
    if (casa_rev_do_asc_natal == 1) {
        snprintf(str_text, 512, _("    Total alignment. Your core identity acts with absolute clarity and autonomy."
                     "The year allows you to express your true self without masks or friction.\n\n\n"));
    } else if (casa_rev_do_asc_natal == 2) {
        snprintf(str_text, 512, _("    Your vital energy is heavily directed toward financial security and values."
                     "You will feel a deep, personal need to consolidate resources and self-worth.\n\n\n"));
    } else if (casa_rev_do_asc_natal == 3) {
        snprintf(str_text, 512, _("    Your mind and immediate expression are highlighted. Communication, short travels, "
                     "and local networks will demand your active intellectual engagement.\n\n\n"));
    } else if (casa_rev_do_asc_natal == 4) {
        snprintf(str_text, 512, _("    An internal, reflective year. You are drawn to privacy, emotional security, "
                     "and resolving matters tied to family, real estate, or your private world.\n\n\n"));
    } else if (casa_rev_do_asc_natal == 5) {
        snprintf(str_text, 512, _("    A highly expressive and creative period. Your vital drive seeks joy, speculation, "
                     "romance, or projects involving children and individual self-expression.\n\n\n"));
    } else if (casa_rev_do_asc_natal == 6) {
        snprintf(str_text, 512, _("    A year requiring duty, physical adjustment, or service. You will need to focus "
                     "heavily on your physical health, daily routines, or workplace obligations.\n\n\n"));
    } else if (casa_rev_do_asc_natal == 7) {
        snprintf(str_text, 512, _("    Your identity is mirrored through others. You will find yourself adapting you r"
                     "personal goals to fit the needs of significant partners or legal agreements.\n\n\n"));
    } else if (casa_rev_do_asc_natal == 8) {
        snprintf(str_text, 512, _("    A period of psychological crisis or deep transformation. You are dealing with "
                     "shared resources, debts, or letting go of old attachments to allow rebirth.\n\n\n"));
    } else if (casa_rev_do_asc_natal == 9) {
        snprintf(str_text, 512, _("    Expansion of consciousness. Your personal focus shifts toward higher learning, "
                     "long-distance travel, astrology, philosophy, or defining your belief system.\n\n\n"));
    } else if (casa_rev_do_asc_natal == 10) {
        snprintf(str_text, 512, _("    High public visibility. Your personal actions are on display for the world to see, "
                     "strongly impacting your career reputation and societal standing this year.\n\n\n"));
    } else if (casa_rev_do_asc_natal == 11) {
        snprintf(str_text, 512, _("    A year of alliances and aspirations. Your focus is placed on collective projects, "
                     "friendships, benefactors, and the realization of long-held hopes.\n\n\n"));
    } else if (casa_rev_do_asc_natal == 12) {
        snprintf(str_text, 512, _("    A period of relative isolation or spiritual retreat. You operate behind the scenes, "
                     "dealing with hidden matters, subconscious patterns, or karmic endings.\n\n\n"));
    } else {
        snprintf(str_text, 512, _("    Invalid house data calculated for the projection.\n\n\n"));
    }

    imprimir_texto_fluxo(pad, 4, 80, str_text);


    const char* msg_cenario_externo[13] = {
        "", // Índice 0 não usado
        _("The world demands immediate, independent action from you, forcing individual projects to the forefront"), // Casa 1
        _("Financial matters, cash flow, and material security will be the central stage where events play out"),  // Casa 2
        _("Circumstances will force heavy communication, frequent local travel, or immediate family/peer interactions"), // Casa 3
        _("External events shift drastically toward your domestic life, home, foundations, or family property"), // Casa 4
        _("The environment opens up opportunities for creativity, speculation, children, or romance"), // Casa 5
        _("The year brings heavy focus on labor obligations, health management, or addressing daily friction"), // Casa 6
        _("Critical events will manifest through contracts, open confrontations, or important partnerships"), // Casa 7
        _("Circumstances push you to deal with joint resources, legacy matters, deep psychological shifts, or debts"), // Casa 8
        _("The outer world calls you toward long journeys, academic pursuits, legal matters, or philosophical changes"), // Casa 9
        _("A massive spotlight is placed on your professional arena, public status, and career obligations"), // Casa 10
        _("Events will actively involve your social circles, networks, benefactors, or long-term alliances"), // Casa 11
        _("Circumstances will force you into the background, dealing with hidden matters, institutions, or isolation") // Casa 12
    };
    
    const char* msg_atitude_interna[13] = {
        "", // Índice 0 não usado
        _("you will face this with total autonomy, absolute vitality, hand clear personal alignment."), // Casa 1
        _("your primary focus will be protecting your resources, asking 'what is this worth to me?'."), // Casa 2
        _("your mind will operate at high speed, analyzing, learning, and sharing information constantly."), // Casa 3
        _("you will respond by seeking emotional security, privacy, and anchoring yourself in your roots."), // Casa 4
        _("your vital drive will demand joy, dramatic self-expression, and a desire to take risks."), // Casa 5
        _("you will feel a strong call to service, requiring patience, physical discipline, and duty."), // Casa 6
        _("you will constantly seek the mirror of the 'Other', adapting your identity to keep relationships balanced."), // Casa 7
        _("you will experience an internal crisis or intense urge to transform and let go of what is dead."), // Casa 8
        _("your consciousness will expand, looking at the big picture through faith, study, or exploration."), // Casa 9
        _("your ego will drive you to be seen, aiming for authority, leadership, and public recognition."), // Casa 10
        _("your energy will be poured into collective hopes, relying heavily on support from friends."), // Casa 11
        _("you will feel internally drained or reflective, operating best from behind the scenes.") // Casa 12
    };

    wattron(pad, COLOR_PAIR(10) | A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattroff(pad, COLOR_PAIR(10) | A_DIM);

    wattron(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
    wprintw(pad, _("  [CHECK VII] SYNTHESIS: THE CROSS PROJECTION\n\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);

    // Validação de segurança para os índices do vetor (evita falha de segmentação)
    if (casa_natal_do_asc >= 1 && casa_natal_do_asc <= 12 && 
        casa_rev_do_asc_natal >= 1 && casa_rev_do_asc_natal <= 12) {

        wprintw(pad, "\n");

        wattron(pad, A_BOLD);
        wprintw(pad, _("    Astrological Synthesis for your Year:\n\n"));
        wattroff(pad, A_BOLD);

        wprintw(pad, "\n");

        /* 
           Aqui acontece a mágica: O código junta a frase da circunstância externa (onde a energia bate)
           com a frase da atitude interna (como você reage).
        */

        char str_text1[512];


        snprintf(str_text1, 512, _("    %s; however, %s\n\n"), 
                msg_cenario_externo[casa_natal_do_asc], 
                msg_atitude_interna[casa_rev_do_asc_natal]);

        imprimir_texto_fluxo(pad, 4, 80, str_text1);
        wprintw(pad, "\n");



        // Uma breve conclusão tradicional dependendo se as duas posições combinam ou conflitam
        wattron(pad, A_BOLD);
        wprintw(pad, _("    Context: "));
        wprintw(pad, "\n");
        wprintw(pad, "\n");
        wattroff(pad, A_BOLD);

        char str_text2[200];

        if (casa_natal_do_asc == casa_rev_do_asc_natal) {
            snprintf(str_text2, 200, _("    An intensely focused year. Your environment and your psychological drive"
                         "    are perfectly aligned, accelerating results with very little inner friction."));
        } else if ((casa_natal_do_asc == 10 && casa_rev_do_asc_natal == 12) || (casa_natal_do_asc == 12 && casa_rev_do_asc_natal == 10)) {
            snprintf(str_text2, 200, _("    A polarizing year. The tension between public obligations and the absolute"
                         "    need for private withdrawal will require strict boundaries to avoid burnout."));
        } else {
            snprintf(str_text2, 200, _("    A year of multi-layered experiences. You will need to learn how to balance the"
                         "    concrete external demands of House %d with your inner needs in House %d."), 
                         casa_natal_do_asc, casa_rev_do_asc_natal);
        }
        imprimir_texto_fluxo(pad, 4, 80, str_text2);


    } else {
        wprintw(pad, _("    Error: Invalid house calculation data for synthesis mapping.\n\n\n"));

    }

    wprintw(pad, "\n");
    wprintw(pad, "\n");




    wattron(pad, A_DIM);
    wattron(pad, A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wprintw(pad, _("  [CONFRONTATION END] - Press 'Q' or ESC to return to the Solar Return interface.\n"));
    wattroff(pad, A_DIM);

    // 6. LOOP DE INTERAÇÃO DO SCROLL
    int pad_line_pos = 0;
    //int max_scroll_y = 120; // Aumentado para permitir a leitura do Check IV até o final
    int ch;

    prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);

    while ((ch = wgetch(pad)) != 27 && ch != 'q' && ch != 'Q') {
        switch (ch) {
            case KEY_UP:
            case 'k':
            case 'K':
                if (pad_line_pos > 0) {
                    pad_line_pos--;
                }
                break;
                
            case KEY_DOWN:
            case 'j':
            case 'J': {
                // 1. Descobre a altura total atualizada do pad na memória (incluindo os wresize)
                int total_linhas_pad = getmaxy(pad);
                
                // 2. Calcula o tamanho da "janela" física na tela onde o texto é renderizado
                int altura_janela_tela = (i_start_y + i_height - 2) - (i_start_y + 1) + 1;
                
                // 3. O limite dinâmico é a diferença entre o tamanho do pergaminho e a janela de exibição
                int limite_dinamico_scroll = total_linhas_pad - altura_janela_tela;
                if (limite_dinamico_scroll < 0) {
                    limite_dinamico_scroll = 0;
                }

                // 4. Se o texto que você imprimiu não preencheu o pad todo, o cursor final
                // do ncurses nos diz onde a última linha real foi escrita.
                int y_cursor_atual = getcury(pad);
                int ultima_linha_com_texto = (y_cursor_atual < total_linhas_pad) ? y_cursor_atual : total_linhas_pad;
                
                int limite_real_texto = ultima_linha_com_texto - altura_janela_tela + 1;
                if (limite_real_texto < 0) limite_real_texto = 0;
                
                // Escolhe o menor limite seguro para não rolar uma tela em branco infinita
                int max_scroll_atual = (limite_real_texto < limite_dinamico_scroll) ? limite_real_texto : limite_dinamico_scroll;

                if (pad_line_pos < max_scroll_atual) {
                    pad_line_pos++;
                }
                break;
            }
        }
        prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);
    }


    // 7. LIMPEZA DA MEMÓRIA
    delwin(pad);
    delwin(border_win);
    delwin(shadow_win);
}


void get_hyleg_data(int tipo_h_natal, int idx_hyleg_natal, double *longitudes_natal, double *lon_hyleg_radix_real, char *nome_hyleg_texto, char *glifo_hyleg_texto) {
    *lon_hyleg_radix_real = longitudes_natal[idx_hyleg_natal];

    // Mapeamento de nomes e glifos do Hyleg Natal baseado no ID tradicional dele (0 a 6)
    const char *nomes_tradicionais[] = {_("Sun"), _("Moon"), _("Mercury"), _("Venus"), _("Mars"), _("Jupiter"), _("Saturn")};
    const char *glifos_tradicionais[] = {"☉", "☽", "☿", "♀", "♂", "♃", "♄"};
        
    if (idx_hyleg_natal >= 0 && idx_hyleg_natal <= 6) {
        strcpy(nome_hyleg_texto, nomes_tradicionais[idx_hyleg_natal]);
        strcpy(glifo_hyleg_texto, glifos_tradicionais[idx_hyleg_natal]);
    } else {
        if (tipo_h_natal == H_ASC) {
            strcpy(nome_hyleg_texto, "Ascendant");
            strcpy(glifo_hyleg_texto, "AC");
        }
        else if (tipo_h_natal == H_FORTUNA) {
            strcpy(nome_hyleg_texto, "Part of Fortune");
            strcpy(glifo_hyleg_texto, "🝴");
        }
    }
}


void get_natal_houses_rev_planets(double jd_natal, double *rev_longitudes, double *rev_latitudes, double armc_natal, double lat_natal, char house_system, int *casas_planetas_natal_proj) {
    char serr[256];
    double xx[6];
    double eps_natal;

    // 2. CALCULAR A OBLIQUIDADE DA ECLÍPTICA DO NATAL (Eps)
    // A função swe_house_pos exige a obliquidade do mapa referencial (Radix)
    // Usamos uma data padrão ou calculamos para capturar o Eps natal exato
    if (swe_calc_ut(jd_natal, SE_ECL_NUT, SEFLG_SWIEPH, xx, serr) >= 0) {
        eps_natal = xx[0]; 
    } else {
        eps_natal = 23.439291; // Valor padrão de salvaguarda
    }

    // 3. LOOP PARA CALCULAR A PROJEÇÃO DE CADA PLANETA DA REVOLUÇÃO NAS CASAS NATAIS
    // 'rev_longitudes' e 'rev_latitudes' são os arrays que você já calculou para a Revolução nesta chart
    for (int p = 0; p < 7; p++) {
        double xpin[2];
        xpin[0] = rev_longitudes[p]; // Longitude do planeta na Revolução
        xpin[1] = rev_latitudes[p];  // Latitude do planeta na Revolução

        // Descobre astronamicamente em qual casa natal o planeta da revolução pisou
        double casa_dec = swe_house_pos(armc_natal, lat_natal, eps_natal, house_system, xpin, serr);
        
        int casa_inteira = (int)floor(casa_dec);
        if (casa_inteira < 1 || casa_inteira > 12) {
            casa_inteira = 1; // Proteção contra retornos fora do escopo
        }
        casas_planetas_natal_proj[p] = casa_inteira;
    }
}

void process_revolution_transits(double jd_natal, double *rev_longitudes, double *rev_latitudes, double armc_natal, double lat_natal, char house_system, int tipo_h_natal, int idx_hyleg_natal, double *longitudes_natal) {
    int casas_planetas_natal_proj[7]; // Guarda em qual casa natal cada planeta da rev caiu

    get_natal_houses_rev_planets(jd_natal, rev_longitudes, rev_latitudes, armc_natal, lat_natal, house_system, casas_planetas_natal_proj);
                        
    double lon_hyleg_radix_real = 0.0;                    
    char nome_hyleg_texto[20];
    char glifo_hyleg_texto[10];

    get_hyleg_data(tipo_h_natal, idx_hyleg_natal, longitudes_natal, &lon_hyleg_radix_real, nome_hyleg_texto, glifo_hyleg_texto);

    abrir_janela_transitos_revolucao(
        rev_longitudes,              // Longitudes da Revolução Solar (0 a 6)
        longitudes_natal,            // Longitudes do Natal (0 a 6) vindas por parâmetro
        casas_planetas_natal_proj,   // Array de projeções recém-calculado
        lon_hyleg_radix_real,        // Longitude real do Hyleg Natal
        nome_hyleg_texto,            // Nome limpo ("Jupiter")
        glifo_hyleg_texto            // Glifo limpo ("♃")
    );
}

void abrir_janela_transitos_revolucao(
    double *longitudes_rev, 
    double *longitudes_natal, 
    int *casas_planetas_rev,
    double lon_hyleg_natal,       /* ADICIONADO: Longitude do Hyleg do Radix */
    char *nome_hyleg_natal,       /* ADICIONADO: Nome do Hyleg (ex: "Jupiter", "Ascendant") */
    char *glifo_hyleg_natal)      /* ADICIONADO: Glifo do Hyleg (ex: "♃", "▲") */
{
    int p_max_y, p_max_x;
    getmaxyx(stdscr, p_max_y, p_max_x); 

    int i_height = p_max_y - 6;
    if (i_height > 26) i_height = 26; 
    int i_width = p_max_x - 12;
    if (i_width > 102) i_width = 102;   

    int i_start_y = (p_max_y - i_height) / 2;
    int i_start_x = (p_max_x - i_width) / 2;

    WINDOW *shadow_win = newwin(i_height, i_width, i_start_y + 1, i_start_x + 1);
    werase(shadow_win); wattron(shadow_win, COLOR_PAIR(9)); box(shadow_win, 0, 0); wattroff(shadow_win, COLOR_PAIR(9)); wrefresh(shadow_win);

    WINDOW *border_win = newwin(i_height, i_width, i_start_y, i_start_x);
    wbkgd(border_win, COLOR_PAIR(13)); box(border_win, 0, 0);
    wattron(border_win, A_BOLD);
    const char *title = _(" Annual Transits & Radical Projections ");
    mvwprintw(border_win, 0, (i_width - get_visual_width(title)) / 2, title);
    wattroff(border_win, A_BOLD);
    mvwprintw(border_win, i_height - 1, (i_width - 44) / 2, _(" [↓↑|JK: Scroll | Q|ESC: Return to Chart] "));
    wrefresh(border_win);

    int pad_lines = 200; 
    int pad_cols = i_width - 6; 
    WINDOW *pad = newpad(pad_lines, pad_cols);
    wbkgd(pad, COLOR_PAIR(13)); keypad(pad, TRUE); idlok(pad, TRUE); scrollok(pad, TRUE);

    wprintw(pad, "\n"); 

    wattron(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, _("  ANNUAL TRANSITS OVER THE RADIX BLUEPRINT\n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wprintw(pad, _("    The positions of the planets at the exact moment of your Solar Return act as a frozen\n"
                 "    layer of transits governing the next 12 months. Below is the mapping of where this year's\n"
                 "    forces physically position themselves over your life-long natal structure.\n\n"));

    const char *glifos[] = {"☉", "☽", "☿", "♀", "♂", "♃", "♄"};
    const char *nomes[]  = {_("Sun"), _("Moon"), _("Mercury"), _("Venus"), _("Mars"), _("Jupiter"), _("Saturn")};

    /* ======================================================================
       BLOCO ADICIONADO: SEÇÃO EXCLUSIVA DE TRÂNSITOS SOBRE O HYLEG NATAL
       ====================================================================== */
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattron(pad, A_BOLD | COLOR_PAIR(11) | A_REVERSE); // Cor de destaque máxima para a Vida/Saúde
    wprintw(pad, _("    [CRITICAL MONITOR] VITAL PROTECTION: TRANSITS OVER NATAL HYLEG (%s %s)\n"), 
            glifo_hyleg_natal, nome_hyleg_natal);
    wattroff(pad, A_BOLD | COLOR_PAIR(11) | A_REVERSE);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");

    bool encontrou_hyleg_transit = false;

    for (int p = 0; p < 7; p++) {
        double diff = fabs(longitudes_rev[p] - lon_hyleg_natal);
        if (diff > 180.0) diff = 360.0 - diff;

        if (diff <= 5.0) { // Orbe estreita de trânsito
            encontrou_hyleg_transit = true;
            
            wattron(pad, A_BOLD | COLOR_PAIR(11)); // Alerta Vermelho
            wprintw(pad, _("       [*] TRANSITING %s IS CONJUNCT YOUR NATAL HYLEG (%s) (Orb: %.2f°)\n"), 
                    nomes[p], nome_hyleg_natal, diff);
            wattroff(pad, A_BOLD | COLOR_PAIR(11));

            if (p == 6) { // Saturno pisando no Hyleg
                wattron(pad, A_BLINK | COLOR_PAIR(11));
                wprintw(pad, _("       [VITAL ALERT] Saturn (Great Malefic) is constricting your Natal Hyleg!\n"));
                wattroff(pad, A_BLINK | COLOR_PAIR(11));
                wprintw(pad, _("       This year promises severe depletion of vital energy, physical fatigue,\n"
                             "       or structural health tests. Rest, discipline, and caution are mandatory.\n\n"));
            } 
            else if (p == 4) { // Marte pisando no Hyleg
                wattron(pad, A_BLINK | COLOR_PAIR(11));
                wprintw(pad, _("       [VITAL ALERT] Mars (Lesser Malefic) is overheating your Natal Hyleg!\n"));
                wattroff(pad, A_BLINK | COLOR_PAIR(11));
                wprintw(pad, _("       High risk of acute inflammatory episodes, fevers, injuries, or sudden surgeries.\n"
                             "       Avoid reckless physical behavior and channel stress constructively.\n\n"));
            } 
            else if (p == 5) { // Júpiter pisando no Hyleg
                wattron(pad, A_BOLD | COLOR_PAIR(12) | A_REVERSE); // Verde/Sucesso
                wprintw(pad, _("       [GREAT PROTECTOR] Jupiter is magnifying your Natal Hyleg!\n"));
                wattroff(pad, A_BOLD | COLOR_PAIR(12) | A_REVERSE);
                wprintw(pad, _("       Excellent providential protection. A year of physical recovery, expansion of\n"
                             "       vital forces, and an invisible protective shield against major crises.\n\n"));
            } 
            else {
                wprintw(pad, _("       The annual transit of %s brings daily focus and minor adjustments to your\n"
                             "       vitality and immediate physical environment this year.\n\n"), nomes[p]);
            }
        }
    }

    if (!encontrou_hyleg_transit) {
        wprintw(pad, _("       No planetary conjunctions from the Solar Return are currently putting friction\n"
                     "       or direct pressure on your Natal Hyleg. Your vital root remains unbothered.\n\n"));
    }
    wprintw(pad, "\n");
    
    for (int p = 0; p < 7; p++) {
        wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
        wattron(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
        wprintw(pad, _("    %s  %s (Solar Return)\n"), glifos[p], nomes[p]);
        wattroff(pad, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
        wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");

        int casa_natal = casas_planetas_rev[p];
        wprintw(pad, _("    • House Projection: Operating inside your NATAL HOUSE %d.\n"), casa_natal);
        wprintw(pad, _("       This year, the physical affairs governed by your Natal House %d will be heavily\n"
                     "       stimulated, triggered, and reconfigured by the active expressions of %s.\n\n"), 
                     casa_natal, nomes[p]);

        wattron(pad, A_BOLD);
        wprintw(pad, _("    • Active Radical Conjunctions:\n"));
        wattroff(pad, A_BOLD);

           
        bool encontrou_conjuncao = false;
        for (int n = 0; n < 7; n++) {
            double diff = fabs(longitudes_rev[p] - longitudes_natal[n]);
            if (diff > 180.0) diff = 360.0 - diff;

            if (diff <= 5.0) {
                encontrou_conjuncao = true;
                wattron(pad, A_BOLD | COLOR_PAIR(11));
                wprintw(pad, _("       [*] TRANSITING %s IS CONJUNCT YOUR NATAL %s (Orbe: %.2f°)\n"), nomes[p], nomes[n], diff);
                wattroff(pad, A_BOLD | COLOR_PAIR(11));

                if (p == 6) {
                    wprintw(pad, _("       [CRITICAL] Saturn brings a strict reality check, heavy boundaries, obstacles,\n"
                                 "       or long-term structuring tasks to the affairs of your natal %s.\n\n"), nomes[n]);
                } else if (p == 5) {
                    wprintw(pad, _("       [BENEFIC] Jupiter injects providential protection, sudden opportunities, expansion,\n"
                                 "       and luck into the baseline promises of your natal %s.\n\n"), nomes[n]);
                } else if (p == 4) {
                    wprintw(pad, _("       [DYNAMISM] Mars triggers acute friction, conflicts, separation, or high physical\n"
                                 "       vitality expenditures upon your natal %s.\n\n"), nomes[n]);
                } else {
                    wprintw(pad, _("       This conjunction wakes up the natal potential of your %s, making its themes\n"
                                 "       highly prominent on a day-to-day level throughout this annual cycle.\n\n"), nomes[n]);
                }
            }
        }
        if (!encontrou_conjuncao) {
            wprintw(pad, _("       No exact conjunctions detected over your natal planets within a 5° orb.\n\n"));
        }
        wprintw(pad, "\n");
    }

    wattron(pad, A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wprintw(pad, _("  [TRANSITS END] - Press 'Q' or ESC to return to the Solar Return interface.\n"));
    wattroff(pad, A_DIM);

    // Loop de controle de scroll
    int pad_line_pos = 0;
    int max_scroll_y = 180;
    int ch;
    prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);
    while ((ch = wgetch(pad)) != 27 && ch != 'q' && ch != 'Q') {
        switch (ch) {
            case KEY_UP: case 'k': case 'K': if (pad_line_pos > 0) pad_line_pos--; break;
            case KEY_DOWN: case 'j': case 'J': if (pad_line_pos < max_scroll_y) pad_line_pos++; break;
        }
        prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);
    }

    delwin(pad); 
    delwin(border_win); 
    delwin(shadow_win);
}