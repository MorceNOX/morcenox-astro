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
#include "firdaria.h"
#include "hyleg.h"




#define RISCO_NENHUM   0
#define RISCO_SUB_LORD 1
#define RISCO_MAJOR    2


int checar_alerta_anaretico(int id_anareta, int id_major_firdaria, int id_sub_firdaria) {
    if (id_anareta <= 0) return RISCO_NENHUM;

    // Condição 1: O Anareta é o governante do macro-período atual de vida
    if (id_major_firdaria == id_anareta) {
        return RISCO_MAJOR;
    }
    
    // Condição 2: O Anareta assumiu o subperíodo do ano corrente
    if (id_sub_firdaria == id_anareta) {
        return RISCO_SUB_LORD;
    }

    return RISCO_NENHUM;
}




// Constantes tradicionais de tempo (Escala de IDs: 1=Sol, 2=Lua, 3=Mercúrio, 4=Vênus, 5=Marte, 6=Júpiter, 7=Saturno)
static const int FIRD_DURACOES[] = {0, 10, 9, 13, 8, 7, 12, 11, 0, 0, 0, 3, 2}; // IDs 11=NN, 12=NS
static const int ORDEM_SUB_CALDEIA[] = {7, 6, 5, 1, 4, 3, 2}; // ♄, ♃, ♂, ☉, ♀, ☿, ☽

// ────────────────────────────────────────────────────────────────────────
// 2. MOTORES MATEMÁTICOS DE SUPORTE
// ────────────────────────────────────────────────────────────────────────

// Soma frações de anos a uma data de nascimento real tratando o calendário
static DataFirdaria somar_tempo_firdaria(DataFirdaria base, double anos_a_somar) {
    DataFirdaria nova = base;
    double anos_inteiros = 0;
    double fracao_ano = modf(anos_a_somar, &anos_inteiros);
    
    nova.ano += (int)anos_inteiros;
    
    double meses_brutos = fracao_ano * 12.0;
    double meses_inteiros = 0;
    double fracao_mes = modf(meses_brutos, &meses_inteiros);
    
    nova.mes += (int)meses_inteiros;
    nova.dia += (int)(fracao_mes * 30.436); // Média precisa de dias por mês comercial
    
    while (nova.dia > 30) {
        nova.dia -= 30;
        nova.mes++;
    }
    while (nova.mes > 12) {
        nova.mes -= 12;
        nova.ano++;
    }
    return nova;
}

// Mapeia nomes e glifos pelos IDs numéricos do banco de dados
static void preencher_dados_texto_planeta(int id, char *nome, char *glifo) {
    switch (id) {
        case 1:  strcpy(nome, _("Sun"));     strcpy(glifo, "☉"); break;
        case 2:  strcpy(nome, _("Moon"));    strcpy(glifo, "☽"); break;
        case 3:  strcpy(nome, _("Mercury")); strcpy(glifo, "☿"); break;
        case 4:  strcpy(nome, _("Venus"));   strcpy(glifo, "♀"); break;
        case 5:  strcpy(nome, _("Mars"));    strcpy(glifo, "♂"); break;
        case 6:  strcpy(nome, _("Jupiter")); strcpy(glifo, "♃"); break;
        case 7:  strcpy(nome, _("Saturn"));  strcpy(glifo, "♄"); break;
        case 11: strcpy(nome, _("North Node")); strcpy(glifo, "☊"); break;
        case 12: strcpy(nome, _("South Node")); strcpy(glifo, "☋"); break;
        default: strcpy(nome, _("None"));    strcpy(glifo, "-"); break;
    }
}

// Executa o cálculo cronológico base de firdárias buscando o período macro
RelatorioFirdaria processar_dados_firdaria(double idade_fracao, bool mapa_diurno) {
    RelatorioFirdaria rel;
    memset(&rel, 0, sizeof(RelatorioFirdaria));
    
    DataFirdaria nasc = {DD, MM, YY};
    double idade_double = idade_fracao;

    int seq_major[9];
    if (mapa_diurno) {
        int ordem[] = {1, 4, 3, 2, 7, 6, 5, 11, 12}; // ☉ -> ♀ -> ☿ -> ☽ -> ♄ -> ♃ -> ♂ -> ☊ -> ☋
        memcpy(seq_major, ordem, sizeof(ordem));
    } else {
        int ordem[] = {2, 5, 6, 7, 1, 4, 3, 11, 12}; // ☽ -> ♂ -> ♃ -> ♄ -> ☉ -> ♀ -> ☿ -> ☊ -> ☋
        memcpy(seq_major, ordem, sizeof(ordem));
    }

    int ordem_caldeia[] = {1, 4, 3, 2, 7, 6, 5}; 

    // 1. Localiza o Governante Principal (Major Lord) acumulando os tempos
    double acumulado = 0.0;
    bool encontrado = false;
    double duracao_major = 0.0;

    for (int i = 0; i < 9; i++) {
        int id_p = seq_major[i];
        double duracao = (double)FIRD_DURACOES[id_p];
        double fim_bloco = acumulado + duracao;
        
        if (idade_double >= acumulado && idade_double < fim_bloco) {
            rel.id_major = id_p;
            rel.inicio_major = somar_tempo_firdaria(nasc, acumulado);
            rel.fim_major = somar_tempo_firdaria(nasc, fim_bloco);
            duracao_major = duracao;
            encontrado = true;
            break;
        }
        acumulado += duracao;
    }
    
    // CORREÇÃO PARA CENTENÁRIOS
    if (!encontrado || rel.id_major == 0) {
        double idade_reiniciada = fmod(idade_double, 75.0);
        int ciclos_completos = (int)(idade_double / 75.0);
        RelatorioFirdaria rel_secundario = processar_dados_firdaria(idade_reiniciada, mapa_diurno);
        
        rel_secundario.inicio_major.ano += (ciclos_completos * 75);
        rel_secundario.fim_major.ano += (ciclos_completos * 75);
        rel_secundario.inicio_sub.ano += (ciclos_completos * 75);
        rel_secundario.fim_sub.ano += (ciclos_completos * 75);
        return rel_secundario;
    }

    preencher_dados_texto_planeta(rel.id_major, rel.nome_major, rel.glifo_major);

    // 2. CÁLCULO EXATO DO SUB-REGENTE BASEADO NAS COORDENADAS HISTÓRICAS DA SUA TABELA
    if (rel.id_major == 11 || rel.id_major == 12) {
        rel.id_sub = rel.id_major;
        strcpy(rel.nome_sub, rel.nome_major);
        strcpy(rel.glifo_sub, rel.glifo_major);
        rel.inicio_sub = rel.inicio_major;
        rel.fim_sub = rel.fim_major;
    } 
    else {
        int p_partida = 0;
        for (int c = 0; c < 7; c++) {
            if (ordem_caldeia[c] == rel.id_major) { p_partida = c; break; }
        }

        double fatia = duracao_major / 7.0;
        
        // Mapeia a idade contra a janela matemática real em anos a partir do nascimento */
        for (int s = 0; s < 7; s++) {
            // Distância em anos acumulados dentro do bloco maior
            double anos_inicio_sub = acumulado + (s * fatia);
            double anos_fim_sub = acumulado + ((s + 1) * fatia);

            if (idade_double >= anos_inicio_sub && idade_double < anos_fim_sub) {
                rel.id_sub = ordem_caldeia[(p_partida + s) % 7];
                rel.inicio_sub = somar_tempo_firdaria(nasc, anos_inicio_sub);
                rel.fim_sub = somar_tempo_firdaria(nasc, anos_fim_sub);
                break;
            }
        }
        preencher_dados_texto_planeta(rel.id_sub, rel.nome_sub, rel.glifo_sub);
    }

    return rel;
}


// ────────────────────────────────────────────────────────────────────────
// 3. INTERFACE VISUAL INTERATIVA (NCURSES)
// ────────────────────────────────────────────────────────────────────────

void display_firdaria(PlotObject *plots, AspectMatrix *matrix, PlanetDignities *dig, PontosHylegiacos pontos, int signo_casa8, int regente_dia, int regente_hora, int tipo_san) {

    // Calcula idade exata de aniversário hoje via time.h
    time_t t_bruto = time(NULL);
    struct tm *t_loc = localtime(&t_bruto);
    int ano_at = t_loc->tm_year + 1900;
    int mes_at = t_loc->tm_mon + 1;
    int dia_at = t_loc->tm_mday;
    
    // Abre a caixinha de setas
    double idade_padrao = obter_idade_padrao_mapa_double();
    double idade_selecionada = selecionar_idade_visual_fracionada(idade_padrao);
    if (idade_selecionada < 0.0) { 
        touchwin(stdscr); 
        refresh(); 
        return; 
    }

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 23;
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

    wattron(table_win, A_BOLD);
    const char *title = _(" Planetary Firdaria Chronocrators ");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
    wattroff(table_win, A_BOLD);

    // Dados de hoje do sistema para acender a linha ativa em tempo real
    double hoje_fracao = (double)(ano_at - YY) + ((double)(mes_at - MM) / 12.0) + ((double)(dia_at - DD) / 365.0);
    
    //double tempo_calculo = (idade_selecionada == idade_padrao) ? hoje_fracao : (double)idade_selecionada;

    // ATUALIZADO: Agora passamos 'tempo_calculo' (double) em vez de 'idade_selecionada' (int)
    RelatorioFirdaria fird = processar_dados_firdaria(idade_selecionada, MAPA_DIURNO);
    


    int id_almuten_ref = 0;
    int object_diff = show_modern_planets ? 0 : 3;
    
    // Recupera o Hileg calculado pelo sistema para passar as coordenadas de aspectos
    int tipo_h = get_hyleg(pontos, plots, matrix, &id_almuten_ref, regente_dia, regente_hora, tipo_san);
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


    // Cabeçalho informando a idade e a seita do mapa
    wattron(table_win, A_BOLD | COLOR_PAIR(13)); 
    mvwprintw(table_win, 2, 4, "Target Timeline: ");
    wattroff(table_win, A_BOLD | COLOR_PAIR(13));

    wattron(table_win, A_BOLD | COLOR_PAIR(7)); 
    wprintw(table_win, "Age %f", idade_selecionada); 
    wattroff(table_win, A_BOLD | COLOR_PAIR(7));


    wprintw(table_win, " | %s: %s", _("Sect"), MAPA_DIURNO ? _("DIURNAL (Sun)") : _("NOCTURNAL (Moon)"));

    wattron(table_win, A_BOLD | COLOR_PAIR(13)); 
    mvwprintw(table_win, 4, 4, _("CURRENT MAJOR LORD: "));
    wattroff(table_win, A_BOLD | COLOR_PAIR(13)); 

    wattron(table_win, COLOR_PAIR(7) | A_BOLD | A_REVERSE); 
    wprintw(table_win, " %s ", fird.glifo_major); 
    wattroff(table_win, COLOR_PAIR(7) | A_BOLD| A_REVERSE);

    wprintw(table_win, " %s %s %02d/%02d/%04d %s %02d/%02d/%04d]", 
             fird.nome_major, _("Period [From:"), fird.inicio_major.dia, fird.inicio_major.mes, fird.inicio_major.ano, _("to"),
             fird.fim_major.dia, fird.fim_major.mes, fird.fim_major.ano);



    // ALERTA MACRO SEPARADO: Se o dono do período de longa duração for o Anareta do mapa!
    if (anar.id_anareta > 0 && fird.id_major == anar.id_anareta) {
        wattron(table_win, COLOR_PAIR(11) | A_BOLD | A_BLINK); // Vermelho Piscante de Alerta
        wprintw(table_win, _("  [CRITICAL ANARETIC CYCLE]"));
        wattroff(table_win, COLOR_PAIR(11) | A_BOLD | A_BLINK);
    }




    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 6, 2, "────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    // Colunas Alinhadas da Tabela
    int col_glifo = 6, col_nome = 14, col_inicio = 30, col_fim = 50, col_status = 70;
    
    wattron(table_win, A_BOLD | COLOR_PAIR(13));
    mvwprintw(table_win, 7, col_glifo, _("Lord")); mvwprintw(table_win, 7, col_nome, _("Subperiod"));
    mvwprintw(table_win, 7, col_inicio, _("Start Date")); mvwprintw(table_win, 7, col_fim, _("End Date"));
    mvwprintw(table_win, 7, col_status, _("Status"));
    wattroff(table_win, A_BOLD | COLOR_PAIR(13));

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 8, 2, "────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    // Rendering dos 7 subperíodos cronológicos
    int row_tabela = 9;
    DataFirdaria nasc = {DD, MM, YY};

    if (fird.id_major == 11 || fird.id_major == 12) {
        mvwprintw(table_win, row_tabela, col_glifo, "%s", _("  -  [Node periods are focused and do not have sub-governors]"));
    } 
    else {
        double duracao_total = (double)FIRD_DURACOES[fird.id_major];
        double fatia = duracao_total / 7.0;
        
        int p_partida = 0;
        for (int c = 0; c < 7; c++) {
            if (ORDEM_SUB_CALDEIA[c] == fird.id_major) { p_partida = c; break; }
        }

        // Reconstrói as 7 fatias com as datas exatas baseadas no nascimento
        for (int s = 0; s < 7; s++) {
            int id_sub_linha = ORDEM_SUB_CALDEIA[(p_partida + s) % 7];

             
            // Distância matemática real em anos a partir do nascimento
            double anos_inicio_sub = (double)(fird.inicio_major.ano - YY) + (s * fatia);
            double anos_fim_sub = (double)(fird.inicio_major.ano - YY) + ((s + 1) * fatia);


            
           
            DataFirdaria d_ini = somar_tempo_firdaria(nasc, anos_inicio_sub);
            DataFirdaria d_fim = somar_tempo_firdaria(nasc, anos_fim_sub);

            char g_line[10] = "";
            char n_line[20] = "";
            preencher_dados_texto_planeta(id_sub_linha, n_line, g_line);

            // Se o usuário está olhando para a idade atual de hoje, usamos a 'hoje_fracao' (ex: 49.52)
            // se ele mudou as setas para o futuro/passado, usamos o valor plano inteiro selecionado.
            
            // 1. Determina o tempo de comparação (Fração exata ou Seta inteira)
            double tempo_comparacao = (idade_selecionada == idade_padrao) ? hoje_fracao : (double)idade_selecionada;

            bool linha_ativa = false;
            char status_texto[15] = "";

            // 2. Classifica a linha em Passada, Ativa ou Pendente baseado na janela de anos
            if (tempo_comparacao >= anos_inicio_sub && tempo_comparacao < anos_fim_sub) {
                linha_ativa = true;
                strcpy(status_texto, "[ ACTIVE ]");
            } 
            else if (tempo_comparacao >= anos_fim_sub) {
                // Se o tempo de vida já ultrapassou o fim deste subperíodo, ele já PASSOU
                strcpy(status_texto, "  Passed  ");
            } 
            else {
                // Se o tempo de vida ainda não atingeu o início deste subperíodo, ele está PENDENTE
                strcpy(status_texto, "  Pending ");
            }

            // 3. Aplica o Realce visual apenas se for a linha ativa
            // --- DENTRO DO LOOP FOR DOS 7 SUBPERÍODOS DA DISPLAY_FIRDARIA ---
          
            // Verifica se o planeta desta linha específica é o Anareta destruidor
            bool linha_anaretica = (anar.id_anareta > 0 && id_sub_linha == anar.id_anareta);

            // Realce visual de fundo apenas se for a linha ativa cronológica de hoje
            if (linha_ativa) {
                wattron(table_win, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
                for (int x = 2; x < table_width - 2; x++) mvwprintw(table_win, row_tabela, x, " ");
            } else {
                if (strcmp(status_texto, "  Passed  ") == 0) wattron(table_win, A_DIM);
            }

            // Impressão das colunas alinhadas da tabela
            mvwprintw(table_win, row_tabela, col_glifo, " %s", g_line);
            mvwprintw(table_win, row_tabela, col_nome, "%s", n_line);
            mvwprintw(table_win, row_tabela, col_inicio, "%02d/%02d/%04d", d_ini.dia, d_ini.mes, d_ini.ano);
            mvwprintw(table_win, row_tabela, col_fim, "%02d/%02d/%04d", d_fim.dia, d_fim.mes, d_fim.ano);
            
            // --- IMPRESSÃO DA TARJA DE STATUS MODIFICADA ---
            if (linha_anaretica) {
                // Se for a linha do Anareta, exibe o aviso de gatilho vital em destaque vermelho!
                wattroff(table_win, COLOR_PAIR(7) | A_REVERSE); // Desliga temporariamente o vídeo reverso para o texto respirar
                wattron(table_win, COLOR_PAIR(11) | A_BOLD);    // Liga o Vermelho Alerta
                
                if (linha_ativa) {
                    mvwprintw(table_win, row_tabela, col_status, _("[ANARETIC RISK]")); // Ativo hoje!
                } else {
                    mvwprintw(table_win, row_tabela, col_status, _(" Trigger Zone ")); // Janela futura ou passada
                }
                
                wattroff(table_win, COLOR_PAIR(11) | A_BOLD);
                if (linha_ativa) wattron(table_win, A_BOLD | COLOR_PAIR(12) | A_REVERSE); // Restaura se necessário
            } else {
                // Se não for anarética, imprime o texto normal (Passed, ACTIVE ou Pending)
                mvwprintw(table_win, row_tabela, col_status, status_texto);
            }

            // Desliga os atributos de fechamento da linha
            if (linha_ativa) wattroff(table_win, A_BOLD | COLOR_PAIR(7) | A_REVERSE);
            else {
                if (strcmp(status_texto, _("  Passed  ")) == 0) wattroff(table_win, A_DIM);
            }

            row_tabela++;

        }

        wattron(table_win, COLOR_PAIR(10) | A_DIM);
        mvwprintw(table_win, 17, 2, "────────────────────────────────────────────────────────────────────────────────────");
        wattroff(table_win, COLOR_PAIR(10) | A_DIM);
        wattron(table_win, A_DIM);
        mvwprintw(table_win, 18, 4, _("Interpretation Guide:"));
        mvwprintw(table_win, 19, 6, _("The highlighted subperiod marks the active executor of your current timeline."));
        mvwprintw(table_win, 20, 6, _("Watch transits and solar return positions of the Active Lord for concrete events."));
        wattroff(table_win, A_DIM);mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
        
        wrefresh(table_win);
        
        keypad(table_win, TRUE);
        nodelay(table_win, FALSE);
        
        int ch;
        do { 
            ch = wgetch(table_win); 
        } while (ch != 27 && ch != 'q');
        
        delwin(shadow_win); 
        delwin(table_win); 
        touchwin(stdscr); 
        refresh();
    }
}
