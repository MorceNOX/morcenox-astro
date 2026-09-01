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
#include "mind.h"


void calcular_qualidades_mente(DadosPlanetaMente mercurio, DadosPlanetaMente lua, int *res_almuten, int total_vencedores, ResultadoMente *res) {
    // --- AVALIAR A FORÇA DO INTELECTO (MERCÚRIO) ---
    int forca_mercurio = mercurio.total_essencial + mercurio.total_acidental;
    if (mercurio.total_essencial >= 5 && mercurio.total_acidental >= 0) {
        strcpy(res->condicao_intelecto, _("Brilliant, Virtuous and Highly Rational"));
    } else if (forca_mercurio >= 0) {
        strcpy(res->condicao_intelecto, _("Practical, Balanced and Functional Intellect"));
    } else {
        strcpy(res->condicao_intelecto, _("Erratic, Anxious or Subject to Mental Fog"));
    }

    // --- AVALIAR A ESTABILIDADE EMOCIONAL (LUA) ---
    int forca_lua = lua.total_essencial + lua.total_acidental;
    if (lua.total_essencial >= 5 && lua.total_acidental >= 0) {
        strcpy(res->condicao_emocional, _("Deeply Grounded, Steady and Retentive"));
    } else if (forca_lua >= 0) {
        strcpy(res->condicao_emocional, _("Adaptable, Normal Emotional Fluency"));
    } else {
        strcpy(res->condicao_emocional, _("Restless, Volatile and Prone to Anxiety"));
    }

    // Matrizes auxiliares de textos curtos para construção dinâmica em caso de empate
    const char *planeta_nomes[] = {"", _("THE SUN"), _("THE MOON"), _("MERCURY"), _("VENUS"), _("MARS"), _("JUPITER"), _("SATURN")};
    const char *planeta_perfil_curto[] = {
        "",
        _("purpose-driven clarity, noble aspirations and pride"),
        _("high intuition, rich imagination and emotional adaptability"),
        _("purely analytical logic, versatility and rapid communication"),
        _("artistic refinement, a strong quest for harmony and gentle relationships"),
        _("sharp competitiveness, incisive wit and rapid crisis management"),
        _("magnanimous wisdom, philosophical depth and excellent ethical judgment"),
        _("deeply disciplined focus, cautious strategic planning and serious research")
    };
    //const char *planeta_keywords[] = {"", "Noble", "Intuitive", "Analytical", "Refined", "Sharp", "Wise", "Deep"};
    int planeta_cores[] = {13, 11, 8, 12, 8, 11, 12, 7};

    // --- TRATAMENTO DOS GOVERNADORES (ÚNICO VS EMPATE) ---
    if (total_vencedores == 1) {
        int id = res_almuten[0];
        if (id >= 1 && id <= 7) {
            strcpy(res->governador_nome, planeta_nomes[id]);
            res->cor_governador = planeta_cores[id];
            
            // Perfis originais longos para governadores únicos
            if (id == 1) strcpy(res->perfil_psicologico, _("Noble, clear, and purpose-driven mind. Seeks leadership, honor, and recognition. Possesses a strong sense of personal dignity, clarity, and pride."));
            else if (id == 2) strcpy(res->perfil_psicologico, _("Highly intuitive, imaginative, and receptive mind. Deeply connected to moods, memories, and fluctuating environments. Highly adaptable and empathetic disposition."));
            else if (id == 3) strcpy(res->perfil_psicologico, _("Purely analytical, agile, and versatile intellect. Outstanding for mathematics, commerce, languages, and rapid communication. Extremely adaptable and logical."));
            else if (id == 4) strcpy(res->perfil_psicologico, _("Artistic, refined, and socially gifted mind. Strongly oriented towards harmony, poetry, arts, and human relationships. Gentle, pleasant, and peacemaking disposition."));
            else if (id == 5) strcpy(res->perfil_psicologico, _("Sharp, fast, and highly competitive mind. Incisive tongue, quick reflexes, and excellent for debate or crisis management, but prone to impatience and rash judgments."));
            else if (id == 6) strcpy(res->perfil_psicologico, _("Magnanimous, wise, and philosophical intellect. Natural understanding of law, ethics, and high principles. Optimistic mindset with excellent and balanced judgment."));
            else if (id == 7) strcpy(res->perfil_psicologico, _("Deep, silent, and highly analytical thinker. Gifted for long studies, heavy research, and strategic planning, but prone to melancholy or rigid thoughts."));
        }
    } else {
        // SE HOUVER EMPATE: Constrói dinamicamente os dados mistos
        strcpy(res->governador_nome, "MIXED INFLUENCE");
        res->cor_governador = 13; // Cor neutra
        
        // Inicializa a string de perfil vazia para começar a concatenação
        res->perfil_psicologico[0] = '\0';
        
        // Monta o texto unindo os perfis curtos dos planetas empatados
        snprintf(res->perfil_psicologico, sizeof(res->perfil_psicologico), 
                 _("Dual mental governance. The intellect dynamically blends the "));
        
        for (int i = 0; i < total_vencedores; i++) {
            int id = res_almuten[i];
            if (id >= 1 && id <= 7) {
                char bloco_temp[256];
                
                if (i == 0) {
                    snprintf(bloco_temp, sizeof(bloco_temp), "%s of %s", planeta_perfil_curto[id], planeta_nomes[id]);
                } else if (i == total_vencedores - 1) {
                    snprintf(bloco_temp, sizeof(bloco_temp), " with the %s of %s.", planeta_perfil_curto[id], planeta_nomes[id]);
                } else {
                    snprintf(bloco_temp, sizeof(bloco_temp), ", the %s of %s", planeta_perfil_curto[id], planeta_nomes[id]);
                }
                
                strncat(res->perfil_psicologico, bloco_temp, sizeof(res->perfil_psicologico) - strlen(res->perfil_psicologico) - 1);
            }
        }
    }
}


void display_natal_mind_analysis(
    DadosPlanetaMente mercurio,
    DadosPlanetaMente lua,
    int mercurio_retrogrado, // 1 se retrógrado, 0 se direto
    int fase_lunar_id,       // 1=Nova, 2=Crescente, 3=Cheia, 4=Minguante
    AspectMatrix *aspecto_matriz,
    PontosHylegiacos pontos,
    PlotObject *plots       // 0=Sol, 1=Lua, 2=Merc, 3=Ven, 4=Mar, 5=Jup, 6=Sat
) {
    int term_w, term_h;
    getmaxyx(stdscr, term_h, term_w);
    
    int table_width = term_w - 10;
    int table_height = 28;
    int start_x = (term_w - table_width) / 2;
    int start_y = (term_h - table_height) / 2;

    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);

    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);

    werase(table_win);
    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13) | FLAGS); 
    
    wattron(table_win, A_BOLD);
    const char *title = _(" Analysis of the Natal Mind ");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
    wattroff(table_win, A_BOLD);

    // --- 1. EXECUÇÃO DO MOTOR DE CÁLCULO ---
    // CORREÇÃO: Usando a longitude direto do array plots conforme solicitado
    // Índices mapeados: 2 para Mercúrio, 1 para Lua
    double pontos_da_mente[2] = {plots[2].longitude, plots[1].longitude};
    int res_almuten[12] = {0};
    
    int total_vencedores = get_almuten_multiplo(pontos_da_mente, 2, res_almuten, aspecto_matriz, pontos, plots);
    
    int id_governador_final;
    if (total_vencedores > 1) {
        id_governador_final = 0; // Ativa o perfil "MIXED INFLUENCE"
    } else {
        id_governador_final = res_almuten[0];
    }

    ResultadoMente res;
    calcular_qualidades_mente(mercurio, lua, res_almuten, total_vencedores, &res);


    // --- 2. REFINAMENTO ADICIONAL DA CONDIÇÃO DE MERCÚRIO E LUA ---
    if (mercurio_retrogrado) {
        if (mercurio.total_essencial < 0 || (mercurio.total_essencial + mercurio.total_acidental) < 0) {
            snprintf(res.condicao_intelecto, sizeof(res.condicao_intelecto), _("Deeply Introverted, Hesitant or Prone to Miscommunications [℞]"));
        } else {
            snprintf(res.condicao_intelecto, sizeof(res.condicao_intelecto), _("Analytical but Introspective; Processes Thoughts Inwardly [℞]"));
        }
    }

    char fase_texto[30] = "";
    switch(fase_lunar_id) {
        case 1: strcpy(fase_texto, _(" (New Moon - Internal)")); break;
        case 2: strcpy(fase_texto, _(" (Crescent - Active)")); break;
        case 3: strcpy(fase_texto, _(" (Full Moon - Exposed)")); break;
        case 4: strcpy(fase_texto, _(" (Waning - Reflective)")); break;
    }
    strncat(res.condicao_emocional, fase_texto, sizeof(res.condicao_emocional) - get_visual_width(res.condicao_emocional) - 1);

    // --- 3. RENDERIZAÇÃO DOS BLOCOS DE TEXTO NO NCURSES ---
    
    // Bloco 1: Condição das Faculdades Mentais
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 2, 4, _("Mental Faculties Status:"));
    wattroff(table_win, A_BOLD);
    
    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 3, 4, "────────────────────────────────────────────────────────────────────");
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    // Mercúrio (Índice 2 fixo no array plots)
    wattron(table_win, COLOR_PAIR(7) | A_BOLD);
    mvwprintw(table_win, 4, 6, "%s %s (%s): ", plots[2].object, plots[2].object_name, _("Intellect"));
    wattroff(table_win, COLOR_PAIR(7) | A_BOLD);

    wattron(table_win, A_ITALIC);
    mvwprintw(table_win, 5, 8, "%s", res.condicao_intelecto);
    wattroff(table_win, A_ITALIC);

    // Lua (Índice 1 fixo no array plots)
    wattron(table_win, COLOR_PAIR(8) | A_BOLD);
    mvwprintw(table_win, 7, 6, "%s %s (%s): ", plots[1].object, plots[1].object_name, _("Emotional Soul"));
    wattroff(table_win, COLOR_PAIR(8) | A_BOLD);

    wattron(table_win, A_ITALIC);
    mvwprintw(table_win, 8, 8, "%s", res.condicao_emocional);
    wattroff(table_win, A_ITALIC);

    // Bloco 2: O Governador da Alma / Mente
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 11, 4, _("Primary Governor of the Mind (Almuten of the Mind):"));
    wattroff(table_win, A_BOLD);
    
    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 12, 4, "────────────────────────────────────────────────────────────────────");
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 13, 6, _("Lord of Thought: "));
    wattroff(table_win, A_BOLD);
    
    if (total_vencedores == 1) {
        // CORREÇÃO: IDs de 1 a 7 convertidos perfeitamente para índice 0-6 fazendo ID - 1
        int idx_plot = id_governador_final - 1; 
        if (idx_plot >= 0 && idx_plot <= 6) {
            wattron(table_win, COLOR_PAIR(res.cor_governador) | A_BOLD);
            wprintw(table_win, "%s %s", plots[idx_plot].object, plots[idx_plot].object_name);
            wattroff(table_win, COLOR_PAIR(res.cor_governador) | A_BOLD);
        }
    } else {
        wattron(table_win, COLOR_PAIR(res.cor_governador) | A_BOLD);
        wprintw(table_win, "%s", res.governador_nome);
        wattroff(table_win, COLOR_PAIR(res.cor_governador) | A_BOLD);
        
        wprintw(table_win, " (");
        for (int i = 0; i < total_vencedores; i++) {
            // CORREÇÃO: Conversão de ID para índice aplicada também no laço de empates
            int idx_empate = res_almuten[i] - 1; 
            if (idx_empate >= 0 && idx_empate <= 6) {
                wprintw(table_win, "%s %s", plots[idx_empate].object, plots[idx_empate].object_name);
                if (i < total_vencedores - 1) wprintw(table_win, " + ");
            }
        }
        wprintw(table_win, ")");
    }

    // Bloco 3: Perfil Psicológico Tradicional Detalhado (Com Word Wrapping Inteligente)
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 15, 6, _("Psychological Profile & Disposition:"));
    wattroff(table_win, A_BOLD);

    wattron(table_win, A_DIM);
    
    // Configurações do Wrapping
    int max_linha_width = 62;   // Largura máxima do texto dentro da janela
    int start_coluna = 8;       // Recuo (indentação) horizontal
    int linha_impressao = 17;   // Linha inicial para começar o texto
    
    // CORREÇÃO: O limite agora é a linha 23 (onde fica a sua linha separadora "───")
    int limite_linha_tabela = 23; 
    
    char *texto_restante = res.perfil_psicologico;
    
    // CORREÇÃO: Atualizada a condição de parada para o novo limite dinâmico
    while (get_visual_width(texto_restante) > 0 && linha_impressao < limite_linha_tabela) {
        // Se o texto restante couber inteiro na linha, imprime e finaliza
        if ((int)get_visual_width(texto_restante) <= max_linha_width) {
            mvwprintw(table_win, linha_impressao, start_coluna, "%s", texto_restante);
            break;
        }
        
        // Caso contrário, procura o último espaço antes do limite de caracteres
        int ponto_quebra = max_linha_width;
        while (ponto_quebra > 0 && texto_restante[ponto_quebra] != ' ') {
            ponto_quebra--;
        }
        
        // Se não houver espaços (uma palavra giga), força a quebra no limite máximo
        if (ponto_quebra == 0) {
            ponto_quebra = max_linha_width;
        }
        
        // Imprime apenas o pedaço da string até o ponto de quebra encontrado
        mvwprintw(table_win, linha_impressao, start_coluna, "%.*s", ponto_quebra, texto_restante);
        
        // Avança o ponteiro do texto para o próximo bloco (pulando o espaço da quebra)
        texto_restante += ponto_quebra;
        if (*texto_restante == ' ') {
            texto_restante++; 
        }
        
        linha_impressao++; // Move para a próxima linha da janela ncurses
    }
    
    wattroff(table_win, A_DIM);


    wattron(table_win, COLOR_PAIR(13));
    mvwprintw(table_win, 23, 4, "────────────────────────────────────────────────────────────────────");
    wattroff(table_win, COLOR_PAIR(13));

    mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
    wnoutrefresh(table_win);

    doupdate();

    keypad(table_win, TRUE);
    nodelay(table_win, FALSE);
    int ch;
    do {
        ch = wgetch(table_win);
    } while (ch != 27 && ch != 'q');
    
    delwin(shadow_win);
    delwin(table_win);
}
