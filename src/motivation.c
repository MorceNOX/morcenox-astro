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
#include "hyleg.h"
#include "motivation.h"

const char *get_motivation(int sign) {
    switch (sign) {
        case 0: return _("Immediate action, pioneering, and personal conquest.");
        case 1: return _("Material security, comfort, and sensory stability.");
        case 2: return _("Curiosity, communication, and mental stimulation.");
        case 3: return _("Belonging, emotional protection, and deep roots.");
        case 4: return _("Recognition, creative expression, and validation.");
        case 5: return _("Utility, order, and continuous improvement.");
        case 6: return _("Harmony, relational balance, and social justice.");
        case 7: return _("Depth, psychological transformation, and hidden truths.");
        case 8: return _("Expansion, freedom, and seeking ultimate meaning.");
        case 9: return _("Tangible achievement, status, and building legacy.");
        case 10: return _("Innovation, collective freedom, and social progress.");
        case 11: return _("Mystical union, compassion, and spiritual transcendence.");
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


void display_motivation(PlotObject *plots) {

    int object_diff = show_modern_planets ? 0 : 3;

    int term_w, term_h;
    getmaxyx(stdscr, term_h, term_w);
    
    int table_width = term_w - 10;
    int table_height = 29;
    int start_x = (term_w - table_width) / 2;
    int start_y = (term_h - table_height) / 2;

    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);

    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    werase(table_win);
    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13)); 
    
    wattron(table_win, A_BOLD);
    const char *title = _(" Primary Motivation ");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
    wattroff(table_win, A_BOLD);

   
       
    // Bloco 1: Condição das Faculdades Mentais
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 2, 4, _("Sign of Ascendant:"));
    wattroff(table_win, A_BOLD);
    
    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 3, 4, "───────────────────────────────────────────────────────────────────────────────────");
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    wattron(table_win, COLOR_PAIR(15) | A_BOLD);
    mvwprintw(table_win, 4, 4, "%s %s - %s  ", plots[P_ASC - object_diff].sign, get_sign_name((int)plots[P_ASC - object_diff].longitude / 30), get_sign_element((int)plots[P_ASC - object_diff].longitude / 30));
    wattroff(table_win, COLOR_PAIR(15) | A_BOLD);

    const char *motivation = get_motivation((int)plots[P_ASC - object_diff].longitude / 30);
    
    wattron(table_win, COLOR_PAIR(22) | A_BOLD);
    mvwprintw(table_win, 6, 6, _("The Core Motivation:"));
    wattroff(table_win, COLOR_PAIR(22) | A_BOLD);

    wattron(table_win, A_ITALIC);
    mvwprintw(table_win, 7, 8, "%s", motivation);
    wattroff(table_win, A_ITALIC);


    int ruler = obter_regente_tradicional((int)plots[P_ASC - object_diff].longitude / 30 + 1);

    wattron(table_win, COLOR_PAIR(28) | A_BOLD | A_REVERSE);
    mvwprintw(table_win, 9, 4, "• %s: %s (%s) ", _("Ruler of The Ascendant"), obter_glifo_planeta_por_id(ruler), obter_nome_planeta_por_id(ruler));
    wattroff(table_win, COLOR_PAIR(28) | A_BOLD | A_REVERSE);

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 10, 4, "───────────────────────────────────────────────────────────────────────────────────");
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    const char *planet_modifier = get_planet_motivation_modifier(ruler - 1);

    wattron(table_win, A_ITALIC);

    
    mvwprintw(table_win, 12, 6, "%.80s", planet_modifier);    
    size_t n = 0;
    while (planet_modifier[80 + n] != 32 && strlen(planet_modifier) > 80 + n) {
        mvwprintw(table_win, 12, 80 + n, "%.1s", planet_modifier + 80 + n);
        n++;
    }
    mvwprintw(table_win, 13, 6, "%s", planet_modifier + 81 + n);
    
    
    wattroff(table_win, A_ITALIC);

    wattron(table_win, COLOR_PAIR(21) | A_BOLD);
    mvwprintw(table_win, 15, 4, "• %s: %s (%s) %s  ", _("Sign of the Ruler"), plots[ruler - 1].sign, get_sign_name((int)plots[ruler - 1].longitude / 30), get_sign_element((int)plots[ruler - 1].longitude / 30));
    wattroff(table_win, COLOR_PAIR(21) | A_BOLD);

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 16, 4, "───────────────────────────────────────────────────────────────────────────────────");
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    wattron(table_win, A_ITALIC);

    int ruler_sign = (int)plots[ruler - 1].longitude / 30;
    
    if (strcmp(get_sign_element_name(ruler_sign), _("Fire")) == 0) {
        mvwprintw(table_win, 18, 6, _("This drive is animated by assertive energy, dynamism, and leadership,"));
        mvwprintw(table_win, 19, 6, _("infusing the core motivation with passion and a vital quest for expansion."));
    }
    else if (strcmp(get_sign_element_name(ruler_sign), _("Earth")) == 0) {
        mvwprintw(table_win, 18, 6, _("This drive is anchored by pragmatism, enduring focus, and deliberation,"));
        mvwprintw(table_win, 19, 6, _("steering the core motivation toward tangible results and material stability."));
    }
    /* Corrigido para 'else if' para otimizar o fluxo de execução */
    else if (strcmp(get_sign_element_name(ruler_sign), _("Air")) == 0) {
        mvwprintw(table_win, 18, 6, _("This drive is channeled into intellectual versatility, logical synthesis,"));
        mvwprintw(table_win, 19, 6, _("and social exchange, occasionally spreading the core motivation across varied interests."));
    }
    else if (strcmp(get_sign_element_name(ruler_sign), _("Water")) == 0) {
        mvwprintw(table_win, 18, 6, _("This drive is shaped by emotional fluidity, intuition, and psychological depth,"));
        mvwprintw(table_win, 19, 6, _("rendering the core motivation highly adaptable to inner and outer currents."));
    }

    wattroff(table_win, A_ITALIC);
    
    wattron(table_win, COLOR_PAIR(27)| A_REVERSE | A_BOLD);
    mvwprintw(table_win, 21, 4, "• %s: %s ", _("House of the Ruler"), plots[ruler - 1].house);    
    wattroff(table_win, COLOR_PAIR(27)| A_REVERSE | A_BOLD);

    wattron(table_win, COLOR_PAIR(10) | A_DIM);
    mvwprintw(table_win, 22, 4, "───────────────────────────────────────────────────────────────────────────────────");
    wattroff(table_win, COLOR_PAIR(10) | A_DIM);

    wattron(table_win, COLOR_PAIR(22) | A_BOLD);
    mvwprintw(table_win, 24, 6, _("Areas of life where the Primary Motivation is applied or directed:"));
    wattroff(table_win, COLOR_PAIR(22) | A_BOLD);

    char area[100] = " ";

    if (strcmp(LANGUAGE, "en") == 0) {
        get_house_meaning(romanToInt(plots[ruler - 1].house), area);
    }
    else {
        int house_num = romanToInt(plots[ruler - 1].house);
        switch(house_num) {
            case 1:
                snprintf(area, 100, "%s", _("The native’s physical body, temperament, personal identity"));
                break;
            case 2:
                snprintf(area, 100, "%s", _("Wealth, personal finances, moveable possessions"));
                break;
            case 3:
                snprintf(area, 100, "%s", _("Immediate environment, short travels, siblings, early education"));
                break;
            case 4:
                snprintf(area, 100, "%s", _("Ancestry, the father, real estate, the end of life"));
                break;
            case 5:
                snprintf(area, 100, "%s", _("Creativity, children, pleasure, speculative ventures"));
                break;
            case 6:
                snprintf(area, 100, "%s", _("Illness, servitude, small domestic animals"));
                break;
            case 7:
                snprintf(area, 100, "%s", _("Partnerships, marriage, open enemies, contracts"));
                break;
            case 8:
                snprintf(area, 100, "%s", _("Death, inheritances, transformations, other people's money"));
                break;
            case 9:
                snprintf(area, 100, "%s", _("Higher education, philosophy, religion, long-distance travel"));
                break;
            case 10:
                snprintf(area, 100, "%s", _("Career, social status, public reputation, the mother"));
                break;
            case 11:
                snprintf(area, 100, "%s", _("Friendships, alliances, group affiliations, hopes"));
                break;
            case 12:
                snprintf(area, 100, "%s", _("Hidden enemies, self-undoing, sorrow, imprisonment, confinements"));
                break;
        }
    }
    wattron(table_win, A_ITALIC);
    mvwprintw(table_win, 25, 6, "%s", area);
    wattroff(table_win, A_ITALIC);
    
    mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
    wrefresh(table_win);

    keypad(table_win, TRUE);
    nodelay(table_win, FALSE);
    int ch;
    do {
        ch = wgetch(table_win);
    } while (ch != 27 && ch != 'q');
    
    delwin(shadow_win);
    delwin(table_win);
}