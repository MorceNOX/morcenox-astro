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
#include <wchar.h>
#include "var.h"
#include "helper.h"
#include "draw-chart.h"
#include "planet_table.h"
#include "db-utils.h"
#include "hyleg.h"
#include "motivation.h"

const char *get_gender_motivation_text(int sign) {
    switch (sign) {        
        // Masculine
        case 0: case 2: case 4: case 6: case 8: case 10:
            return _("Active, direct, outward-flowing, and projective (doing energy). "
                    "Tend to process experiences externally through action, speech, and social engagement. "
                    "Quick to initiate, take risks, and pursue goals with direct force. "
                    "Heavily driven by logic, concepts, big-picture visions, and tangible results.");   
    
        // Feminine
        case 1: case 3: case 5: case 7: case 9: case 11:
            return _("Receptive, stable, inward-flowing, and nurturing (being energy). "
                    "Tend to process experiences quietly, internally, and deeply before responding. "
                    "Draw things toward them rather than forcing action; focus on sustaining, containing, and stabilizing. "
                    "Guided by feeling-based wisdom, sensory tangible reality, empathy, and subtle nuances.");
    }

    return " ";
}


const char *get_element_motivation_text(int sign) {
    switch (sign) {
        case 0: case 4: case 8:  return _("Vitality, inspiration, and enthusiasm. "
                                          "Motivated by personal growth, excitement, and a desire to create or lead. "
                                          "Quick to act on instincts, highly competitive, and fiercely independent. "
                                          "Highly expressive but can be impatient, impulsive, or quick-tempered."); // fire
        
        case 1: case 5: case 9:  return _("Stability, practicality, and physical reality. "
                                          "Focused on material security, tangible results, and long-term structures. "
                                          "Approach life through logic, method, and sensory experience. "
                                          "Highly disciplined and patient, though they can sometimes resist change or become overly rigid."); // earth

        case 2: case 6: case 10: return _("Intellect, communication, and social connection. "
                                          "Motivated by ideas, information, mental stimulation, and abstract concepts. "
                                          "Focus heavily on relationships, community dynamics, and objective observation. "
                                          "Skilled at seeing multiple perspectives, though they can sometimes become detached or overly intellectual."); // air

        case 3: case 7: case 11: return _("Emotion, intuition, and psychic sensitivity. "
                                          "Guided by emotional undercurrents, empathy, and a strong sense of intuition. "
                                          "Seek deep emotional bonds and security, highly attuned to the moods of others. "
                                          "Highly creative and spiritually inclined, but susceptible to absorbing outside energies or becoming moody."); // water

    }

    return " ";
}


const char *get_mode_motivation_text(int sign) {
    switch (sign) {
        case 0: case 3: case 6: case 9:  return _("Initiative and assertiveness. Forward momentum, leadership, and crisis management."); // cardinals
        case 1: case 4: case 7: case 10: return _("Stability. Endurance, focus, concentration, and resistance to change."); // fixed
        case 2: case 5: case 8: case 11: return _("Adaptability. Flexibility, transition, synthesis, and resourcefulness. Prepared for change."); // muttable
    }

    return " ";
}



const char *get_motivation(int sign) {
    switch (sign) {
        case 0: return _("Immediate action, pioneering, and personal conquest. Initiates with raw action, courage, and impulse. It is the spark that starts the fire.");
        case 1: return _("Material security, comfort, and sensory stability. Sustains through physical comfort, patience, and routine. It is deeply stubborn and anchored in physical security.");
        case 2: return _("Curiosity, communication, and mental stimulation. Adapts through continuous learning, communication, and multitasking. It easily shifts from one thought or conversation to another.");
        case 3: return _("Belonging, emotional protection, and deep roots. Initiates through emotional connection, protection, and nurturing. It leads with the heart to establish a secure home base.");
        case 4: return _("Recognition, creative expression, and validation. Sustains through loyal passion, creative expression, and self-expression. It keeps the fire burning steadily with pride and warmth.");
        case 5: return _("Utility, order, and continuous improvement. Adapts through daily adjustments, problem-solving, and service. It refines details, organizes physical chaos, and stays highly resourceful.");
        case 6: return _("Harmony, relational balance, and social justice. Initiates through social connection, justice, and partnerships. It actively creates balance, ideas, and harmony between people.");
        case 7: return _("Depth, psychological transformation, and hidden truths. Sustains through deep emotional intensity, willpower, and psychological truth. It holds onto feelings tightly, offering ultimate loyalty or deep secrecy.");
        case 8: return _("Expansion, freedom, and seeking ultimate meaning. Adapts through exploration, philosophy, and freedom. It scatters its fiery inspiration across a wide horizon, always seeking the next big truth.");
        case 9: return _("Tangible achievement, status, and building legacy. Initiates through material ambition, structure, and hard work. It builds the foundation for long-term, tangible success.");
        case 10: return _("Innovation, collective freedom, and social progress. Sustains through ideological conviction, group ideals, and intellectual principles. It holds firmly to its vision for the future or society.");
        case 11: return _("Mystical union, compassion, and spiritual transcendence. Adapts through empathy, imagination, and spiritual dissolution. It is fluid like the ocean, easily blending its emotional boundaries with the universe.");
    }

    return " ";
}


const char *get_planet_motivation_modifier(int planet_id) {
    switch (planet_id) {
        case 0: return _("The Sun centralizes the motivation in the self, in the quest for personal brilliance, leadership, pride and visibility.");
        case 1: return _("The Moon turns the motivation highly fluctuating, guided by humors, instinct of protection, memory and emotional attachment.");
        case 2: return _("Mercury filters the motivation by logic, intelectualization, necessity of communication, analysis and data processing.");
        case 3: return _("Venus shifts the quest towards the pleasure, aesthetics, partnership, financial gain or social validation.");
        case 4: return _("Mars adds urgency, competitiveness, a need to win, haste, and a focus on direct action.");
        case 5: return _("Jupiter amplifies the motivation with idealism, excesses, quest for wisdom, luck or dogmatism.");
        case 6: return _("Saturn imposes limits, slowness, a sense of duty, a need for structure, fear of failure, and maturity.");
    }

    return " ";
}


const char *get_house_modifier(int house_num) {
    switch(house_num) {
        case 1:  return _("personal identity, vitality, and core temperament");
        case 2:  return _("material wealth, personal finances, and livelihood");
        case 3:  return _("the immediate environment, siblings, and short travels");
        case 4:  return _("ancestry, parental heritage, real estate, and private foundations");
        case 5:  return _("creative expression, children, pleasures, and speculation");
        case 6:  return _("physical illness, daily labor, and subordinate relationships");
        case 7:  return _("partnerships, marriage, public adversaries, and contracts");
        case 8:  return _("inheritances, psychological transformations, and shared resources");
        case 9:  return _("higher knowledge, philosophy, worldview, and long-distance journeys");
        case 10: return _("professional career, social status, and public reputation");
        case 11: return _("alliances, supportive networks, friendships, and long-term hopes");
        case 12: return _("hidden challenges, self-undoing, isolation, and confinement");
    }
    return " ";
}



const char *get_planet_characteristics(char *planet) {
    
    if (strcmp(planet, "☉") == 0) {
        return _("Core identity, ego, life purpose, vital energy, conscious will");
    }
    else if (strcmp(planet, "☾") == 0) {
        return _("Emotional inner world, intuition, subconscious habits, instinctual needs for comfort and security, memories, sense of safety");
    }
    else if (strcmp(planet, "☿") == 0) {
        return _("Communication, intellect, learning style, logical reasoning, the mind, communication style, how one processes and share information");
    }
    else if (strcmp(planet, "♀") == 0) {
        return _("Love, relationships, beauty, aesthetics, what is valuable for the person, financial habits");
    }
    else if (strcmp(planet, "♂") == 0) {
        return _("Drive, ambition, physical energy, aggression, passion");
    }
    else if (strcmp(planet, "♃") == 0) {
        return _("Luck, abundance, wisdom, philosophical expansion");
    }
    else if (strcmp(planet, "♄") == 0) {
        return _("Discipline, boundaries, responsibility, fears, life lessons. The \"taskmaster\" that rules discipline, responsibility, limitations, and the lessons one learns through hard work");
    }
    else if (strcmp(planet, "♅") == 0) {
        return _("Rebellion, innovation, sudden change, freedom, genius, individuation, breakthroughs");
    }
    else if (strcmp(planet, "♆") == 0) {
        return _("Dreams, illusion, spirituality, creativity, escapism, psychic intuition, spiritual dissolution");
    }
    else if (strcmp(planet, "⯓") == 0) {
        return _("Transformation, power, death, rebirth, subconscious shadows, hidden truths, power dynamics");
    }
    else if (strcmp(planet, "☊") == 0) {
        return _("It acts like a magnifying glass or a funnel, swelling and increasing the quantity of the conditions it touches. It exaggerates the traits of the zodiac sign and brings intense activity, hunger, or focus to the affairs of that specific house. If conjunct a planet, it expands that planet's power");
    }
    else if (strcmp(planet, "☋") == 0) {
        return _("It acts like a drain, a filter, or a point of release, shrinking and emptying the conditions it touches. It minimizes or devalues the traits of the zodiac sign and indicates an area of life (the house) where things are drained, sacrificed, or let go. If conjunct a planet, it weakens, eclipses, or \"purges\" that planet's expression");
    }

    return NULL;
       
}




void display_motivation(PlotObject *plots, int *house_rulers, AspectMatrix *matrix) {

    int object_diff = show_modern_planets ? 0 : 3;

    int term_w, term_h;
    getmaxyx(stdscr, term_h, term_w);
    
    int table_width = term_w - 50 > 90 ? term_w - 50 : 90;
    int table_height = 24;
    int start_x = (term_w - table_width) / 2;
    int start_y = (term_h - table_height) / 2;

    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    int row_pad = 0;

    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);

    werase(table_win);
    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13) | FLAGS); 
    
    wattron(table_win, A_BOLD);
    const char *title = _(" Primary Motivation ");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
    wattroff(table_win, A_BOLD);

    // 2. Desenha o botão [X] no canto superior direito
    int col_fechar = getmaxx(table_win) - 4; // Abre espaço para 3 caracteres: '[', 'X', ']'

    wattron(table_win, COLOR_PAIR(13)); 
    mvwprintw(table_win, 0, col_fechar, "[");
    mvwprintw(table_win, 0, col_fechar + 2, "]");
    wattroff(table_win, COLOR_PAIR(13));

    wattron(table_win, COLOR_PAIR(13) | A_BOLD);
    mvwprintw(table_win, 0, col_fechar + 1, "✖");
    wattroff(table_win, COLOR_PAIR(13) | A_BOLD);

    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED, NULL);
    mouseinterval(100);


    //int max_linhas_dados = table_height - 6;
    WINDOW *pad = newpad(120, table_width - 5);

    idlok(pad, TRUE);
    scrollok(pad, TRUE);

    wbkgd(pad, COLOR_PAIR(13) | FLAGS);


    char planeta_na_casa1_glifo[12][10] = {0};
    char planeta_na_casa1_nome[12][10] = {0};
    
    int qtd_planetas_casa1 = 0;
    for (int i = 0; i < 12 - object_diff; i++) {
        if (romanToInt(plots[i].house) == 1) {
            snprintf(planeta_na_casa1_glifo[qtd_planetas_casa1], 10, "%s", plots[i].object);
            snprintf(planeta_na_casa1_nome[qtd_planetas_casa1], 10, "%s", plots[i].object_name);
            qtd_planetas_casa1++;
        }
    }
                
    typedef struct {
        int id;
        char object[10];
        char object_name[30];
        char text[512];
    } AspectAsc;

    AspectAsc influences[12];
    int qtd_asp = 0;

    for (int j = 0; j < 12 - object_diff; j++) {
        AspectCell c1 = matrix->grid[j][P_ASC - object_diff];
        AspectCell c2 = matrix->grid[P_ASC - object_diff][j];
        
        if (c1.has_aspect || c2.has_aspect) {
            influences[qtd_asp].id = j;
            snprintf(influences[qtd_asp].object, 10, "%s", plots[j].object);
            snprintf(influences[qtd_asp].object_name, 30, "%s", plots[j].object_name);

            snprintf(influences[qtd_asp].text, 256, "• %s (%s). %s.\n", plots[j].object, plots[j].object_name, get_planet_characteristics(plots[j].object));
            qtd_asp++;
        }
    }

   
    // Bloco 1: Condição das Faculdades Mentais
    wattron(pad, A_BOLD);
    mvwprintw(pad, 1, 4, _("Sign of Ascendant:"));
    wattroff(pad, A_BOLD);
    
    row_pad++;

    wattron(pad, COLOR_PAIR(10) | A_DIM);
    wmove(pad, 2, 4);
    whline(pad, ACS_HLINE, table_width - 6);
    wattroff(pad, COLOR_PAIR(10) | A_DIM);

    row_pad++;

    wattron(pad, COLOR_PAIR(15) | A_BOLD);
    mvwprintw(pad, 3, 4, "%s %s - %s  ", plots[P_ASC - object_diff].sign, get_sign_name((int)plots[P_ASC - object_diff].longitude / 30), get_sign_element((int)plots[P_ASC - object_diff].longitude / 30));
    wattroff(pad, COLOR_PAIR(15) | A_BOLD);

    row_pad += 2;

    int sign = (int)floor(plots[P_ASC - object_diff].longitude / 30);

    const char *gender_mot = get_gender_motivation_text(sign);
    const char *element_mot = get_element_motivation_text(sign);
    const char *mode_mot = get_mode_motivation_text(sign);
    const char *motivation = get_motivation(sign);
    
    wattron(pad, COLOR_PAIR(6) | A_BOLD);
    mvwprintw(pad, row_pad + 1, 6, _("The Main Characteristics of The Motivation:"));
    wattroff(pad, COLOR_PAIR(6) | A_BOLD);

    row_pad += 3;

    row_pad += print_text_multiline(pad, row_pad, 6, 80, gender_mot);

    row_pad += 2;

    row_pad += print_text_multiline(pad, row_pad, 6, 80, element_mot);

    row_pad += 2;

    row_pad += print_text_multiline(pad, row_pad, 6, 80, mode_mot);

    row_pad += 2;

    wattron(pad, COLOR_PAIR(6) | A_BOLD | A_REVERSE);
    mvwprintw(pad, row_pad + 1, 6, _("The Core Motivation:"));
    wattroff(pad, COLOR_PAIR(6) | A_BOLD | A_REVERSE);

    row_pad += 3;

    row_pad += print_text_multiline(pad, row_pad, 6, 80, motivation);

    row_pad += 2;

    wattron(pad, COLOR_PAIR(6) | A_BOLD);
    mvwprintw(pad, row_pad++, 6, _("Influences by Presence:"));
    wattroff(pad, COLOR_PAIR(6) | A_BOLD);

    if (qtd_planetas_casa1 == 0) {
        mvwprintw(pad, row_pad++, 6, _("No planets in the Ascendant (House 1)."));
    }
    else {
        row_pad += print_text_multiline(pad, row_pad + 1, 6, 80, _("The following planets are sharing their influences to the Core Motivativation by presence in the House 1.\n\n"));   
    }

    for (int i = 0; i < qtd_planetas_casa1; i++) {
        char p_text[512] = "";
        snprintf(p_text, 512, "• %s (%s). %s.\n", planeta_na_casa1_glifo[i], planeta_na_casa1_nome[i], get_planet_characteristics(planeta_na_casa1_glifo[i]));
        row_pad += print_text_multiline(pad, row_pad + 2, 6, 80, p_text);
        row_pad++; 
    }

    row_pad += 2;

    wattron(pad, COLOR_PAIR(6) | A_BOLD);
    mvwprintw(pad, row_pad++, 6, _("Influences by Aspect:"));
    wattroff(pad, COLOR_PAIR(6) | A_BOLD);

    if (qtd_asp == 0) {
        mvwprintw(pad, row_pad++, 6, _("No planets aspecting the Ascendant."));
    }
    else {
        row_pad += print_text_multiline(pad, row_pad + 1, 6, 80, _("The following planets are aspecting the Ascendant and sharing their influences to the Core Motivativation.\n\n"));   
    }
        
    for (int i = 0; i < qtd_asp; i++) {
        row_pad += print_text_multiline(pad, row_pad + 2, 6, 80, influences[i].text);
        row_pad++;       
    }

    row_pad += 2;

    int ruler = obter_regente_tradicional((int)plots[P_ASC - object_diff].longitude / 30 + 1);

    wattron(pad, COLOR_PAIR(28) | A_BOLD | A_REVERSE);
    mvwprintw(pad, row_pad + 2, 4, "• %s: %s (%s) ", _("Ruler of The Ascendant"), obter_glifo_planeta_por_id(ruler), obter_nome_planeta_por_id(ruler));
    wattroff(pad, COLOR_PAIR(28) | A_BOLD | A_REVERSE);

    row_pad += 2;

    wattron(pad, COLOR_PAIR(10) | A_DIM);
    wmove(pad, row_pad + 1, 4);
    whline(pad, ACS_HLINE, table_width - 6);    
    wattroff(pad, COLOR_PAIR(10) | A_DIM);

    row_pad += 1;

    const char *planet_modifier = get_planet_motivation_modifier(ruler - 1);

    wattron(pad, A_ITALIC);

    


    row_pad += print_text_multiline(pad, row_pad + 2, 6, 80, planet_modifier);
    row_pad += 2;


        
    wattroff(pad, A_ITALIC);

    wattron(pad, COLOR_PAIR(21) | A_BOLD);
    mvwprintw(pad, row_pad + 2, 4, "• %s: %s (%s) %s  ", _("Sign of the Ruler"), plots[ruler - 1].sign, get_sign_name((int)plots[ruler - 1].longitude / 30), get_sign_element((int)plots[ruler - 1].longitude / 30));
    wattroff(pad, COLOR_PAIR(21) | A_BOLD);

    row_pad += 3;

    wattron(pad, COLOR_PAIR(10) | A_DIM);
    wmove(pad, row_pad + 1, 4);
    whline(pad, ACS_HLINE, table_width - 6);
    wattroff(pad, COLOR_PAIR(10) | A_DIM);
    
    row_pad += 2;

    wattron(pad, A_ITALIC);

    int ruler_sign = (int)plots[ruler - 1].longitude / 30;
    
    if (strcmp(get_sign_element_name(ruler_sign), _("Fire")) == 0) {
        mvwprintw(pad, row_pad + 1, 6, _("This drive is animated by assertive energy, dynamism, and leadership,"));
        mvwprintw(pad, row_pad + 2, 6, _("infusing the core motivation with passion and a vital quest for expansion."));
    }
    else if (strcmp(get_sign_element_name(ruler_sign), _("Earth")) == 0) {
        mvwprintw(pad, row_pad + 1, 6, _("This drive is anchored by pragmatism, enduring focus, and deliberation,"));
        mvwprintw(pad, row_pad + 2, 6, _("steering the core motivation toward tangible results and material stability."));
    }
    else if (strcmp(get_sign_element_name(ruler_sign), _("Air")) == 0) {
        mvwprintw(pad, row_pad + 1, 6, _("This drive is channeled into intellectual versatility, logical synthesis,"));
        mvwprintw(pad, row_pad + 2, 6, _("and social exchange, occasionally spreading the core motivation across "));
        mvwprintw(pad, row_pad + 3, 6, _("varied interests."));
    }
    else if (strcmp(get_sign_element_name(ruler_sign), _("Water")) == 0) {
        mvwprintw(pad, row_pad + 1, 6, _("This drive is shaped by emotional fluidity, intuition, and psychological depth,"));
        mvwprintw(pad, row_pad + 2, 6, _("rendering the core motivation highly adaptable to inner and outer currents."));
    }

    row_pad += 2;

    wattroff(pad, A_ITALIC);
    
    wattron(pad, COLOR_PAIR(27)| A_REVERSE | A_BOLD);
    mvwprintw(pad, row_pad + 3, 4, "• %s: %s ", _("House of the Ruler"), plots[ruler - 1].house);    
    wattroff(pad, COLOR_PAIR(27)| A_REVERSE | A_BOLD);

    row_pad += 3;

    wattron(pad, COLOR_PAIR(10) | A_DIM);
    wmove(pad, row_pad + 1, 4);
    whline(pad, ACS_HLINE, table_width - 6);
    wattroff(pad, COLOR_PAIR(10) | A_DIM);
   
    row_pad += 2;

    wattron(pad, COLOR_PAIR(6) | A_BOLD);
    mvwprintw(pad, row_pad + 1, 6, _("Areas of life where the Primary Motivation is applied or directed:"));
    wattroff(pad, COLOR_PAIR(6) | A_BOLD);

    row_pad++;

    int house_pos = romanToInt(plots[ruler - 1].house);
    const char *area_foco = get_house_modifier(house_pos);

    int casas_governadas[2] = {0, 0};
    int qtd_governadas = 0;

    for (int i = 1; i <= 12; i++) {
        if (house_rulers[i] == ruler) {
            casas_governadas[qtd_governadas] = i;
            qtd_governadas++;
            if (qtd_governadas >= 2) break; // Máximo de duas casas por planeta na astrologia tradicional
        }
    }

    char influence[1024] = "";

    if (qtd_governadas == 1) {
        // Caso do Sol, Lua, ou interceptações onde apenas uma casa é governada
        snprintf(influence, sizeof(influence),
            _("The native's primary drives are directed toward %s (House %d). "
              "The raw material and circumstances to fulfill this motivation will be "
              "drawn fundamentally from the affairs of %s (House %d)."),
            area_foco, house_pos, 
            get_house_modifier(casas_governadas[0]), casas_governadas[0]);
    } 
    else if (qtd_governadas == 2) {
        // Caso padrão dos planetas tradicionais (Mercúrio, Vênus, Marte, Júpiter, Saturno)
        snprintf(influence, sizeof(influence),
            _("The native's primary drives are directed toward %s (House %d). "
              "The fuel, tools, and background experiences for these matters are supplied "
              "by a dual matrix: the affairs of %s (House %d) and %s (House %d)."),
            area_foco, house_pos, 
            get_house_modifier(casas_governadas[0]), casas_governadas[0],
            get_house_modifier(casas_governadas[1]), casas_governadas[1]);
    }

    wattron(pad, A_ITALIC);
    row_pad += print_text_multiline(pad, row_pad + 2, 6, 80, influence);
    
    row_pad++;
    wattroff(pad, A_ITALIC);

    
    mvwprintw(table_win, table_height - 1, 2, _(" Press ESC to return to chart | [↓↑] Scroll "));
    wnoutrefresh(table_win);

    doupdate();

    // 6. LOOP DE INTERAÇÃO E SCROLL DA PAD
    int pad_line_pos = 0;
    //int max_scroll_y = (qtd_vencedores > 1) ? 90 : 45; // Adapta a profundidade de scroll
    int ch;
    
    row_pad += 5;
   
    // Altura visível real onde o texto do pad aparece na tela
    int visible_height = (start_y + table_height - 4) - (start_y + 1) + 1;
    
    // A altura física da scrollbar deve bater com o espaço vertical interno da table_win
    //int scrollbar_height = table_height - 2; 

    // Habilita as setas do teclado na janela de borda para o wgetch capturar corretamente
    keypad(table_win, TRUE);

    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(100);

    drag_and_drop_on();

    int running = 1;
    while (running) {
        
        int flag = 0;
        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
        wattron(table_win, COLOR_PAIR(28) | flag);
        desenhar_scrollbar(table_win, pad_line_pos, row_pad, visible_height, 1);
        wattroff(table_win, COLOR_PAIR(28) | flag);
        wnoutrefresh(table_win);

        mvwaddstr(table_win, 1, table_width - 2, "▲");
        mvwaddstr(table_win, 2 + visible_height, table_width - 2, "▼");

        // prefresh envia os dados do pad diretamente para a tela virtual
        prefresh(pad, pad_line_pos, 0, start_y + 1, start_x + 3, start_y + table_height - 3, start_x + table_width - 5);
        doupdate(); // Executa a pintura unificada na tela física

        // --- 3. CAPTURA DE INPUT (Na table_win, não no pad) ---
        ch = wgetch(table_win);
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
                if (pad_line_pos < (row_pad - visible_height)) pad_line_pos++; 
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
                            running = 0;
                            break; // Sai do switch do mouse e fecha a janela
                        }
                    }

                    // 1. Descobre a coluna onde a barra é desenhada
                    int col_scrollbar_absoluta = getbegx(table_win) + (getmaxx(table_win) - 2);

                    // 2. Verifica se o clique do mouse ocorreu exatamente na coluna da barra de rolagem
                    if (event.x == col_scrollbar_absoluta) {
                        
                        // Definição das posições verticais (Seta de Cima é linha 1, Canal inicia na linha 2)
                        int linha_seta_cima = 1;
                        int offset_inicio_barra = 2; 
                        int linha_seta_baixo = offset_inicio_barra + visible_height; // Linha logo após o canal

                        int max_scroll_y = row_pad - visible_height;
                        if (max_scroll_y < 0) max_scroll_y = 0;

                        // ========================================================
                        // A: CLIQUE NA SETA PARA CIMA (▲) - ROLAGEM CONTÍNUA
                        // ========================================================
                        if (linha_clique_janela == linha_seta_cima) {
                            if (event.bstate & BUTTON1_PRESSED) {
                                nodelay(table_win, TRUE);
                                while (1) {
                                    if (pad_line_pos > 0) {
                                        pad_line_pos--;

                                        // Redesenho imediato do Pad e da Scrollbar
                                        int flag = 0;
                                        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
                                        wattron(table_win, COLOR_PAIR(28) | flag);
                                        desenhar_scrollbar(table_win, pad_line_pos, row_pad, visible_height, 1);
                                        wattroff(table_win, COLOR_PAIR(28) | flag);

                                        wnoutrefresh(table_win);
                                        prefresh(pad, pad_line_pos, 0, start_y + 1, start_x + 3, start_y + table_height - 3, start_x + table_width - 4);
                                        doupdate();
                                    }

                                    napms(80); // Velocidade confortável das setas

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
                        // B: CLIQUE NA SETA PARA BAIXO (▼) - ROLAGEM CONTÍNUA
                        // ========================================================
                        else if (linha_clique_janela == linha_seta_baixo) {
                            if (event.bstate & BUTTON1_PRESSED) {
                                nodelay(table_win, TRUE);
                                while (1) {
                                    if (pad_line_pos < max_scroll_y) {
                                        pad_line_pos++;

                                        // Redesenho imediato do Pad e da Scrollbar
                                        int flag = 0;
                                        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
                                        wattron(table_win, COLOR_PAIR(28) | flag);
                                        desenhar_scrollbar(table_win, pad_line_pos, row_pad, visible_height, 1);
                                        wattroff(table_win, COLOR_PAIR(28) | flag);

                                        wnoutrefresh(table_win);
                                        prefresh(pad, pad_line_pos, 0, start_y + 1, start_x + 3, start_y + table_height - 3, start_x + table_width - 4);
                                        doupdate();
                                    } else {
                                        break;
                                    }

                                    napms(80);

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
                        // C: TRILHO DO PAD (CLIQUE E ARRASTO SEM SALTO)
                        // ========================================================
                        else if (event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED | REPORT_MOUSE_POSITION)) {
                            nodelay(table_win, TRUE);

                            // Calcula de forma reversa a posição visual exata do indicador do trilho
                            int linha_trilho_atual = 0;
                            if (max_scroll_y > 0 && (visible_height - 1) > 0) {
                                linha_trilho_atual = (pad_line_pos * (visible_height - 1)) / max_scroll_y;
                            }
                            int linha_trilho_janela = linha_trilho_atual + offset_inicio_barra;

                            // Se clicou sobre o trilho existente, ativa o modo de arrasto relativo
                            int modo_arrasto_trilho = (linha_clique_janela == linha_trilho_janela);
                            int linha_inicial_mouse = linha_clique_janela;
                            int pad_line_pos_original = pad_line_pos;

                            while (1) {
                                int novo_offset = pad_line_pos;
                                int linha_clique_barra = linha_clique_janela - offset_inicio_barra;

                                if (modo_arrasto_trilho) {
                                    // Arrasto relativo baseado na movimentação da mão
                                    int delta_mouse = linha_clique_janela - linha_inicial_mouse;
                                    if ((visible_height - 1) > 0 && max_scroll_y > 0) {
                                        int delta_scroll = (delta_mouse * max_scroll_y) / (visible_height - 1);
                                        novo_offset = pad_line_pos_original + delta_scroll;
                                    }
                                } else {
                                    // Clique absoluto fora do trilho (teleporte de página)
                                    if (linha_clique_barra >= 0 && linha_clique_barra < visible_height) {
                                        if ((visible_height - 1) > 0 && max_scroll_y > 0) {
                                            novo_offset = (linha_clique_barra * max_scroll_y) / (visible_height - 1);
                                        }
                                    }
                                }

                                if (novo_offset < 0) novo_offset = 0;
                                if (novo_offset > max_scroll_y) novo_offset = max_scroll_y;

                                if (pad_line_pos != novo_offset) {
                                    pad_line_pos = novo_offset;
                                    
                                    // Redesenho imediato
                                    int flag = 0;
                                    if (DARK_MODE) flag |= A_DIM | A_REVERSE;
                                    wattron(table_win, COLOR_PAIR(28) | flag);
                                    desenhar_scrollbar(table_win, pad_line_pos, row_pad, visible_height, 1);
                                    wattroff(table_win, COLOR_PAIR(28) | flag);

                                    wnoutrefresh(table_win);
                                    prefresh(pad, pad_line_pos, 0, start_y + 1, start_x + 3, start_y + table_height - 3, start_x + table_width - 4);
                                    doupdate();
                                }
    
    
                                // Se for um clique estático fora do trilho, encerra o loop na hora
                                if ((event.bstate & BUTTON1_CLICKED) && !modo_arrasto_trilho) {
                                    break;
                                }
                                napms(10); // Rastreamento fino de arrasto
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
        }
    }
    
    // CLEAN UP: Desaloca todas as janelas do escopo e devolve o controle para a stdscr limpa
    delwin(pad);
    delwin(shadow_win);
    delwin(table_win);
    refresh();

    drag_and_drop_off();
}