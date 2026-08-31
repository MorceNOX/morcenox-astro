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

#define _XOPEN_SOURCE
#define NCURSES_WIDECHAR 1
#include "swephexp.h"
#include <ncurses.h>
#include <ncursesw/curses.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <string.h>
#include <wchar.h>
#include <time.h>
#include <menu.h>
#include <ctype.h>
#include <sqlite3.h>
#include <regex.h>
#include <stdio.h>
#include <libintl.h>

#include "helper.h"
#include "selections.h"
#include "var.h"
#include "number_helper.h"
#include "db-utils.h"


int show_confirm_yesno(const char *name, const char *text) {

    int term_w, term_h;
    getmaxyx(stdscr, term_h, term_w);

    int pop_w = 54;
    int pop_h = 8;
    int pop_x = (term_w - pop_w) / 2;
    int pop_y = (term_h - pop_h) / 2;

    WINDOW *pop_win = newwin(pop_h, pop_w, pop_y, pop_x);
    WINDOW *pop_shadow = newwin(pop_h, pop_w, pop_y + 1, pop_x + 1);

    nodelay(pop_win, FALSE);
    keypad(pop_win, TRUE);
    curs_set(0);

    // Desenha sombra
    werase(pop_shadow);
    wattron(pop_shadow, COLOR_PAIR(24));
    box(pop_shadow, 0, 0);
    wattroff(pop_shadow, COLOR_PAIR(24));
    wrefresh(pop_shadow);

    // Configura e desenha a estrutura fixa da janela principal
    werase(pop_win);
    wbkgd(pop_win, COLOR_PAIR(26) | FLAGS);
    wattron(pop_win, COLOR_PAIR(27) | A_BOLD);
    box(pop_win, 0, 0);

    const char *title = _(" CONFIRM ");

    mvwprintw(pop_win, 0, (pop_w - get_visual_width(title)) / 2, title);
    wattroff(pop_win, COLOR_PAIR(27) | A_BOLD);

    mvwprintw(pop_win, 2, 4, "%s:", text);
    mvwprintw(pop_win, 3, 4, "\"%.46s\" ?", name);

    int botao_focado = 0; // 0 = Confirmar, 1 = Cancelar
    int confirmado = 0;
    int ch;

    while (1) {
        // Redesenha os botões dinamicamente com base no foco
        // Botão CONFIRM
        int attr_confirm = (botao_focado == 0) ? (COLOR_PAIR(36) | A_REVERSE | A_BOLD) : (COLOR_PAIR(23));
        wattron(pop_win, attr_confirm);
        mvwprintw(pop_win, 5, 8, _("    YES    "));
        wattroff(pop_win, attr_confirm);

        // Botão CANCEL
        int attr_cancel = (botao_focado == 1) ? (COLOR_PAIR(36) | A_REVERSE | A_BOLD) : (COLOR_PAIR(23));
        wattron(pop_win, attr_cancel);
        mvwprintw(pop_win, 5, 32, _("    NO    "));
        wattroff(pop_win, attr_cancel);

        wrefresh(pop_win);

        ch = wgetch(pop_win);

        if (ch == KEY_LEFT || ch == KEY_RIGHT) {
            // Alterna o foco entre os dois botões (0 vira 1, 1 vira 0)
            botao_focado = 1 - botao_focado;
        } 
        else if (ch == 10) { // ENTER
            // Se der Enter no Confirm, retorna 1. Se for no Cancel, retorna 0.
            confirmado = (botao_focado == 0) ? 1 : 0;
            break;
        } 
        else if (ch == 'n' || ch == 'N' || ch == 27) { // Atalhos para cancelar direto
            confirmado = 0;
            break;
        }
        else if (ch == 'y' || ch == 'Y') { // Atalho para confirmar direto
            confirmado = 1;
            break;
        }
    }

    // Limpa a tela antes de fechar o popup
    delwin(pop_win);
    delwin(pop_shadow);
           
    return confirmado;
}



int show_confirm_delete_popup(const char *name) {
    int term_w, term_h;
    getmaxyx(stdscr, term_h, term_w);

    int pop_w = 54;
    int pop_h = 8;
    int pop_x = (term_w - pop_w) / 2;
    int pop_y = (term_h - pop_h) / 2;

    WINDOW *pop_win = newwin(pop_h, pop_w, pop_y, pop_x);
    WINDOW *pop_shadow = newwin(pop_h, pop_w, pop_y + 1, pop_x + 1);

    nodelay(pop_win, FALSE);
    keypad(pop_win, TRUE);
    curs_set(0);

    werase(pop_shadow);
    wattron(pop_shadow, COLOR_PAIR(24));
    box(pop_shadow, 0, 0);
    wattroff(pop_shadow, COLOR_PAIR(24));
    wrefresh(pop_shadow);

    werase(pop_win);
    wbkgd(pop_win, COLOR_PAIR(26) | FLAGS);
    wattron(pop_win, COLOR_PAIR(27) | A_BOLD);
    box(pop_win, 0, 0);

    const char *title =  _(" CONFIRM DELETE ");
    mvwprintw(pop_win, 0, (pop_w - get_visual_width(title)) / 2, title);
    wattroff(pop_win, COLOR_PAIR(27) | A_BOLD);

    mvwprintw(pop_win, 2, 4, _("Are you sure you want to permanently delete:"));
    mvwprintw(pop_win, 3, 4, "\"%.46s\" ?", name);

    int botao_focado = 0; // 0 = Confirmar, 1 = Cancelar
    int confirmado = 0;
    int ch;

    while (1) {
        // Redesenha os botões dinamicamente com base no foco
        // Botão CONFIRM
        int attr_confirm = (botao_focado == 0) ? (COLOR_PAIR(23) | A_REVERSE | A_BOLD) : COLOR_PAIR(3);
        wattron(pop_win, attr_confirm);
        mvwprintw(pop_win, 5, 8, _("  CONFIRM  "));
        wattroff(pop_win, attr_confirm);

        // Botão CANCEL
        int attr_cancel = (botao_focado == 1) ? (COLOR_PAIR(3) | A_REVERSE | A_BOLD) : COLOR_PAIR(3);
        wattron(pop_win, attr_cancel);
        mvwprintw(pop_win, 5, 32, _("  CANCEL  "));
        wattroff(pop_win, attr_cancel);

        wrefresh(pop_win);

        ch = wgetch(pop_win);

        if (ch == KEY_LEFT || ch == KEY_RIGHT) {
            // Alterna o foco entre os dois botões (0 vira 1, 1 vira 0)
            botao_focado = 1 - botao_focado;
        } 
        else if (ch == 10) { // ENTER
            // Se der Enter no Confirm, retorna 1. Se for no Cancel, retorna 0.
            confirmado = (botao_focado == 0) ? 1 : 0;
            break;
        } 
        else if (ch == 'n' || ch == 'N' || ch == 27) { // Atalhos para cancelar direto
            confirmado = 0;
            break;
        }
        else if (ch == 'y' || ch == 'Y') { // Atalho para confirmar direto
            confirmado = 1;
            break;
        }
    }

    // Limpa a tela antes de fechar o popup
    delwin(pop_win);
    delwin(pop_shadow);
           
    return confirmado;
}



void show_alert_popup(const char *txt_line1, const char *txt_line2) {
    int term_w, term_h;
    getmaxyx(stdscr, term_h, term_w);

    int w1 = get_visual_width(txt_line1);
    int w2 = get_visual_width(txt_line2);
    int width = (w1 > w2 ? w1 + 4 : w2 + 4);

    int pop_w = 54 > width ? 54 : width;
    int pop_h = 8;
    int pop_x = (term_w - pop_w) / 2;
    int pop_y = (term_h - pop_h) / 2;

    WINDOW *pop_win = newwin(pop_h, pop_w, pop_y, pop_x);
    WINDOW *pop_shadow = newwin(pop_h, pop_w, pop_y + 1, pop_x + 1);

    nodelay(pop_win, FALSE);
    keypad(pop_win, TRUE);
    curs_set(0);

    werase(pop_shadow);
    wattron(pop_shadow, COLOR_PAIR(24));
    box(pop_shadow, 0, 0);
    wattroff(pop_shadow, COLOR_PAIR(24));
    wrefresh(pop_shadow);

    werase(pop_win);
    wbkgd(pop_win, COLOR_PAIR(26) | FLAGS);
    wattron(pop_win, COLOR_PAIR(27) | A_BOLD);
    box(pop_win, 0, 0);

    const char *title =  _(" ALERT ");

    mvwprintw(pop_win, 0, (pop_w - get_visual_width(title)) / 2, title);
    wattroff(pop_win, COLOR_PAIR(27) | A_BOLD);

    mvwprintw(pop_win, 2, 4, "%.46s", txt_line1);
    mvwprintw(pop_win, 3, 4, "%.46s", txt_line2);

    int ch;

    while (1) {
        int attr_confirm = (COLOR_PAIR(23) | A_REVERSE | A_BOLD);
        wattron(pop_win, attr_confirm);
        mvwprintw(pop_win, 5, (pop_w - 10) / 2, _("    OK    "));
        wattroff(pop_win, attr_confirm);

        wrefresh(pop_win);

        ch = wgetch(pop_win);

        if (ch == 10) { // ENTER
            break;
        } 
        
    }

    delwin(pop_win);
    delwin(pop_shadow);
           
}






ChartOptions load_default_options() {
    ChartOptions options;
    
    options.dark_mode = DARK_MODE;
    options.house_system = HOUSE_SYSTEM;
    options.triplicity_system = 1;
    options.terms_system = 1;
    options.modern_planets_rulling = false;
    options.show_modern_planets = false;
    options.gender = GENDER;
    snprintf(options.language, 10, "%s", LANGUAGE);

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in load_default_options\n");
        return options;
    }

    const char *sql_select = "SELECT dark_mode, house_system, triplicity_system, terms_system, modern_planets_rulling, show_modern_planets, gender, language FROM profiles WHERE profile = ?;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement (load_default_options): %s\n", sqlite3_errmsg(db));
        close_database(db);
        return options;
    }
    sqlite3_bind_text(stmt, 1, "default", -1, SQLITE_STATIC);
    
    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {        
        int dark_mode = sqlite3_column_int(stmt, 0);
        const char *house_system = (const char*)sqlite3_column_text(stmt, 1);
        int triplicity_system = sqlite3_column_int(stmt, 2);
        int terms_system = sqlite3_column_int(stmt, 3);
        int modern_planets_rulling = sqlite3_column_int(stmt, 4);
        int show_modern_planets = sqlite3_column_int(stmt, 5);
        int gender_id = sqlite3_column_int(stmt, 6);
        const char *lang_cod = (const char*)sqlite3_column_text(stmt, 7);

        options.dark_mode = dark_mode;
        options.house_system = house_system[0];
        options.triplicity_system = triplicity_system;
        options.terms_system = terms_system;
        options.modern_planets_rulling = modern_planets_rulling;
        options.show_modern_planets = show_modern_planets;
        options.gender = gender_id;
        snprintf(options.language, 10, "%s", lang_cod);

        found = 1;
    }

    if (!found) {
        fprintf(stderr, "Failed to retrieve default options from the database in load_default_options\n");    
    }
    
    sqlite3_finalize(stmt);
    close_database(db);

    return options;
}


OptionsEdition select_options() {
    ChartOptions options = load_default_options();

    OptionsEdition ed;
    ed.options = options;
    ed.changed = 0;

    int campo_atual = 0;
    int key;
    int options_confirmed = 0;

    unsigned short term_w = 80, term_h = 24;
    getmaxyx(stdscr, term_h, term_w); // Captura o tamanho do terminal atual
    
    int w_width = 80, w_height = 30;
    WINDOW *win = newwin(w_height, w_width, (term_h - w_height)/2, (term_w - w_width)/2);
    WINDOW *shadow = newwin(w_height, w_width, (term_h - w_height)/2 + 1, (term_w - w_width)/2 + 1);
    keypad(win, TRUE);
    curs_set(0); // Oculta o cursor piscante para navegação visual

    // Load house systems from database
    char **house_systems = NULL;
    char **house_system_names = NULL;
    int house_system_count = 0;
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (db) {
        const char *sql_select_house_systems = "SELECT id, name FROM house_system ORDER BY id;";
        rc = sqlite3_prepare_v2(db, sql_select_house_systems, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char *id = (const char*)sqlite3_column_text(stmt, 0);
                const char *name = (const char*)sqlite3_column_text(stmt, 1);
                
                // Resize arrays
                house_systems = realloc(house_systems, (house_system_count + 1) * sizeof(char*));
                house_system_names = realloc(house_system_names, (house_system_count + 1) * sizeof(char*));
                
                // Store house system ID as char (first character) and name
                house_systems[house_system_count] = malloc(2);
                house_systems[house_system_count][0] = id[0];
                house_systems[house_system_count][1] = '\0';
                
                house_system_names[house_system_count] = malloc(strlen(name) + 1);
                strcpy(house_system_names[house_system_count], name);
                
                house_system_count++;
            }
            sqlite3_finalize(stmt);
        }
        close_database(db);
    }

    // Load triplicity systems from database
    int *triplicity_ids = NULL;
    char **triplicity_names = NULL;
    int triplicity_count = 0;

    db = open_database();
    if (db) {
        const char *sql_select_triplicity_systems = "SELECT id, name FROM triplicity_systems ORDER BY id;";
        rc = sqlite3_prepare_v2(db, sql_select_triplicity_systems, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                const char *name = (const char*)sqlite3_column_text(stmt, 1);
                
                // Resize arrays
                triplicity_ids = realloc(triplicity_ids, (triplicity_count + 1) * sizeof(int));
                triplicity_names = realloc(triplicity_names, (triplicity_count + 1) * sizeof(char*));
                
                // Store triplicity system ID and name
                triplicity_ids[triplicity_count] = id;
                
                triplicity_names[triplicity_count] = malloc(strlen(name) + 1);
                strcpy(triplicity_names[triplicity_count], name);
                
                triplicity_count++;
            }
            sqlite3_finalize(stmt);
        }
        close_database(db);
    }



    // Load terms systems from database
    int *terms_ids = NULL;
    char **terms_names = NULL;
    int terms_count = 0;

    db = open_database();
    if (db) {
        const char *sql_select_terms_systems = "SELECT id, name FROM terms_systems ORDER BY id;";
        rc = sqlite3_prepare_v2(db, sql_select_terms_systems, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                const char *name = (const char*)sqlite3_column_text(stmt, 1);
                
                // Resize arrays
                terms_ids = realloc(terms_ids, (terms_count + 1) * sizeof(int));
                terms_names = realloc(terms_names, (terms_count + 1) * sizeof(char*));
                
                // Store terms system ID and name
                terms_ids[terms_count] = id;
                
                terms_names[terms_count] = malloc(strlen(name) + 1);
                strcpy(terms_names[terms_count], name);
                
                terms_count++;
            }
            sqlite3_finalize(stmt);
        }
        close_database(db);
    }




    // Load planet orbis values from database
    double planet_orbis[12] = {0.0}; // Sun through Pluto
    db = open_database();
    if (db) {
        const char *sql_select_orbis = "SELECT orbis FROM planets WHERE id BETWEEN 1 AND 11 ORDER BY id;";
        rc = sqlite3_prepare_v2(db, sql_select_orbis, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            int index = 0;
            while (sqlite3_step(stmt) == SQLITE_ROW && index < 11) {
                planet_orbis[index] = sqlite3_column_double(stmt, 0);
                index++;
            }
            sqlite3_finalize(stmt);
        }
        close_database(db);
    }

    // Load parallel aspect orbis from database
    double parallel_orbis = 0.0;
    db = open_database();
    if (db) {
        const char *sql_select_p_orbis = "SELECT parallel_orbis FROM profiles WHERE profile = 'default';";
        rc = sqlite3_prepare_v2(db, sql_select_p_orbis, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                parallel_orbis = sqlite3_column_double(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        close_database(db);
    }


    // Load languages from database
    char **lang_cods = NULL;
    char **lang_names = NULL;
    int lang_count = 0;

    db = open_database();
    if (db) {
        const char *sql_select_lang = "SELECT cod, name FROM languages ORDER BY name;";
        rc = sqlite3_prepare_v2(db, sql_select_lang, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char *cod = (const char*)sqlite3_column_text(stmt, 0);
                const char *name = (const char*)sqlite3_column_text(stmt, 1);
                
                // Resize arrays
                lang_cods = realloc(lang_cods, (lang_count + 1) * sizeof(char*));
                lang_names = realloc(lang_names, (lang_count + 1) * sizeof(char*));
                
                lang_cods[lang_count] = malloc(strlen(cod) + 1);
                strcpy(lang_cods[lang_count], cod);
                
                lang_names[lang_count] = malloc(strlen(name) + 1);
                strcpy(lang_names[lang_count], name);
                
                lang_count++;
            }
            sqlite3_finalize(stmt);
        }
        close_database(db);
    }

    while (!options_confirmed) {
        werase(shadow);
        wattron(shadow, COLOR_PAIR(4));
        box(shadow, 0, 0);
        wattroff(shadow, COLOR_PAIR(4));
        wrefresh(shadow);

        werase(win);
        wattron(win, COLOR_PAIR(2));
        wbkgd(win, COLOR_PAIR(2) | FLAGS);
        box(win, 0, 0);
        
        wattron(win, A_BOLD);
        const char *title =  _(" Settings ");

        mvwprintw(win, 0, (w_width - get_visual_width(title)) / 2, title);

        wattroff(win, A_BOLD);
        
        mvwprintw(win, w_height - 1, 2, _("Use [←↓↑→] to ajust. [Enter] confirm. [ESC] Cancel."));
        wattroff(win, COLOR_PAIR(2));

        

        // Renderização dos campos com destaque no selecionado
        for (int i = 0; i < 20; i++) {
            if (i == campo_atual) wattron(win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
            else wattron(win, COLOR_PAIR(2));

            if (i == 0) {
                // Show 'yes' or 'no' instead of 0 or 1
                const char *dark_mode_str = options.dark_mode ? _("yes") : _("no");

                const char *dark_mode_text = _("Dark Mode");
                mvwprintw(win, 2, 5, "%s: %s ", dark_mode_text, dark_mode_str);
            }
            if (i == 1) {
                // Show the char id and the name of the house system
                char house_system_name[128];
                snprintf(house_system_name, 128, "%s", _("Unknown"));
                if (house_system_count > 0) {
                    // Find the name for the current house system
                    for (int j = 0; j < house_system_count; j++) {
                        if (house_systems[j][0] == options.house_system) {
                            snprintf(house_system_name, sizeof(house_system_name), "%c (%s)", 
                                    options.house_system, house_system_names[j]);
                            break;
                        }
                    }
                }
                const char *house_system_text = _("House System");
                mvwprintw(win, 4, 5, "%s: %s ", house_system_text, house_system_name);
            }
            if (i == 2) {
                // Show the name of the triplicity system instead of just the number
                char triplicity_system_name[128];
                snprintf(triplicity_system_name, 128, "%s", _("Unknown"));
                if (triplicity_count > 0) {
                    // Find the name for the current triplicity system
                    for (int j = 0; j < triplicity_count; j++) {
                        if (triplicity_ids[j] == options.triplicity_system) {
                            snprintf(triplicity_system_name, sizeof(triplicity_system_name), "%d (%s)", 
                                    options.triplicity_system, triplicity_names[j]);
                            break;
                        }
                    }
                }
                const char *trip_text = _("Triplicity System");

                mvwprintw(win, 5, 5, "%s: %s ", trip_text, triplicity_system_name);
            }
            if (i == 3) {
                // Show the name of the terms system instead of just the number
                char terms_system_name[128];
                snprintf(terms_system_name, 128, "%s", _("Unknown"));
                if (terms_count > 0) {
                    // Find the name for the current terms system
                    for (int j = 0; j < terms_count; j++) {
                        if (terms_ids[j] == options.terms_system) {
                            snprintf(terms_system_name, sizeof(terms_system_name), "%d (%s)", 
                                    options.terms_system, terms_names[j]);
                            break;
                        }
                    }
                }

                const char *terms_text = _("Terms (bounds) System");
                mvwprintw(win, 6, 5, "%s: %s ", terms_text, terms_system_name);
            }
            
            
            if (i == 4) {
                const char *orb1 = _("Sun orb");
                mvwprintw(win, 8, 5, "%s: %.1f ", orb1, planet_orbis[0]);
            }
            if (i == 5) {
                const char *orb2 = _("Moon orb");
                mvwprintw(win, 9, 5, "%s: %.1f ", orb2, planet_orbis[1]);
            }
            if (i == 6) {
                const char *orb3 = _("Mercury orb");
                mvwprintw(win, 10, 5, "%s: %.1f ", orb3, planet_orbis[2]);
            }
            if (i == 7) {
                const char *orb4 = _("Venus orb");
                mvwprintw(win, 11, 5, "%s: %.1f ", orb4, planet_orbis[3]);
            }
            if (i == 8) {
                const char *orb5 = _("Mars orb");
                mvwprintw(win, 12, 5, "%s: %.1f ", orb5, planet_orbis[4]);
            }
            if (i == 9) {
                const char *orb6 = _("Jupiter orb");
                mvwprintw(win, 13, 5, "%s: %.1f ", orb6, planet_orbis[5]);
            }
            if (i == 10) {
                const char *orb7 = _("Saturn orb");
                mvwprintw(win, 14, 5, "%s: %.1f ", orb7, planet_orbis[6]);
            }
            if (i == 11) {
                const char *orb8 = _("Uranus orb");
                mvwprintw(win, 15, 5, "%s: %.1f ", orb8, planet_orbis[7]);
            }
            if (i == 12) {
                const char *orb9 = _("Neptune orb");
                mvwprintw(win, 16, 5, "%s: %.1f ", orb9, planet_orbis[8]);
            }
            if (i == 13) {
                const char *orb10 = _("Pluto orb");
                mvwprintw(win, 17, 5, "%s: %.1f ", orb10, planet_orbis[9]);
            }
            if (i == 14) {
                const char *orb11 = _("Lunar Nodes orb");
                mvwprintw(win, 18, 5, "%s: %.1f ", orb11, planet_orbis[10]);
            }

            if (i == 15) {
                const char *orb12 = _("Parallel / Contra-parallel orb");
                mvwprintw(win, 19, 5, "%s: %.1f ", orb12, parallel_orbis);
            }

            if (i == 16) {
                // Show 'yes' or 'no' instead of 0 or 1
                const char *consider_modern_planets_str = options.modern_planets_rulling ? _("yes") : _("no");
                const char *consider_text = _("Consider Modern Planets Rulling by Exaltation?");
                mvwprintw(win, 21, 5, "%s %s ", consider_text, consider_modern_planets_str);
            }

            if (i == 17) {
                // Show 'yes' or 'no' instead of 0 or 1
                const char *show_modern_planets_str = options.show_modern_planets ? _("yes") : _("no");
                const char *use_text = _("Use Modern Planets?");

                mvwprintw(win, 22, 5, "%s %s ", use_text, show_modern_planets_str);
            }

            if (i == 18) {
                const char *gender_str = options.gender == 1 ? _("Male") : (options.gender == 2 ? _("Female") : _("Neuter"));
                const char *gen_text = _("Default Gender");

                mvwprintw(win, 24, 5, "%s: %s ", gen_text, gender_str);
            }

            if (i == 19) {
                char language_name[128];
                snprintf(language_name, 128, "%s", _("Unknown"));
                if (lang_count > 0) {
                    for (int j = 0; j < lang_count; j++) {
                        if (strcmp(lang_cods[j], options.language) == 0) {
                            snprintf(language_name, sizeof(language_name), "%s (%s)", 
                                    options.language, lang_names[j]);
                            break;
                        }
                    }
                }
                const char *lang_text = _("Interface Language");

                mvwprintw(win, 26, 5, "%s: %s ", lang_text, language_name);
            }

            if (i == campo_atual) wattroff(win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
            else wattroff(win, COLOR_PAIR(2));
        }

        wrefresh(win);
        key = wgetch(win);

        switch (key) {
            case KEY_UP:
                campo_atual = (campo_atual - 1 + 20) % 20; // Now 13 fields
                break;
            case KEY_DOWN:
                campo_atual = (campo_atual + 1) % 20;
                break;
            case KEY_RIGHT:
                if (campo_atual == 0) {
                    // Toggle dark mode
                    options.dark_mode = !options.dark_mode;
                }
                else if (campo_atual == 1) { 
                    // Cycle through house systems
                    if (house_system_count > 0) {
                        // Find current position
                        int current_pos = -1;
                        for (int j = 0; j < house_system_count; j++) {
                            if (house_systems[j][0] == options.house_system) {
                                current_pos = j;
                                break;
                            }
                        }
                        
                        // Move to next system
                        if (current_pos >= 0) {
                            int next_pos = (current_pos + 1) % house_system_count;
                            options.house_system = house_systems[next_pos][0];
                        } else {
                            // If not found, start with first
                            options.house_system = house_systems[0][0];
                        }
                    }
                }
                else if (campo_atual == 2) {
                    // Cycle through triplicity systems
                    if (triplicity_count > 0) {
                        // Find current position
                        int current_pos = -1;
                        for (int j = 0; j < triplicity_count; j++) {
                            if (triplicity_ids[j] == options.triplicity_system) {
                                current_pos = j;
                                break;
                            }
                        }
                        
                        // Move to next system
                        if (current_pos >= 0) {
                            int next_pos = (current_pos + 1) % triplicity_count;
                            options.triplicity_system = triplicity_ids[next_pos];
                        } else {
                            // If not found, start with first
                            options.triplicity_system = triplicity_ids[0];
                        }
                    }
                }
                else if (campo_atual == 3) {
                    // Cycle through terms systems
                    if (terms_count > 0) {
                        // Find current position
                        int current_pos = -1;
                        for (int j = 0; j < terms_count; j++) {
                            if (terms_ids[j] == options.terms_system) {
                                current_pos = j;
                                break;
                            }
                        }
                        
                        // Move to next system
                        if (current_pos >= 0) {
                            int next_pos = (current_pos + 1) % terms_count;
                            options.terms_system = terms_ids[next_pos];
                        } else {
                            // If not found, start with first
                            options.terms_system = terms_ids[0];
                        }
                    }
                }
                else if (campo_atual >= 4 && campo_atual <= 14) {
                    // Increment planet orbis value
                    int planet_index = campo_atual - 3;
                    if (planet_orbis[planet_index] < 30.0) {
                        planet_orbis[planet_index] += 0.5;
                    }
                }
                else if (campo_atual == 15) {
                    if (parallel_orbis < 5.0) {
                        parallel_orbis += 0.5;
                    }
                }
                else if (campo_atual == 16) {
                    options.modern_planets_rulling = !options.modern_planets_rulling;
                }
                else if (campo_atual == 17) {
                    options.show_modern_planets = !options.show_modern_planets;
                }
                else if (campo_atual == 18) {
                    if (options.gender < 4) options.gender++;
                    if (options.gender == 4) options.gender = 1;            
                }
                else if (campo_atual == 19) {
                    if (lang_count > 0) {
                        // Find current position
                        int current_pos = -1;
                        for (int j = 0; j < lang_count; j++) {
                            if (strcmp(lang_cods[j], options.language) == 0) {
                                current_pos = j;
                                break;
                            }
                        }
                        
                        if (current_pos >= 0) {
                            int next_pos = (current_pos + 1) % lang_count;
                            snprintf(options.language, 10, "%s", lang_cods[next_pos]);
                        } else {
                            snprintf(options.language, 10, "%s", lang_cods[0]);
                        }
                    }
                }
                break;

            case KEY_LEFT:
                if (campo_atual == 0) {
                    // Toggle dark mode
                    options.dark_mode = !options.dark_mode;
                }
                else if (campo_atual == 1) { 
                    // Cycle backwards through house systems
                    if (house_system_count > 0) {
                        // Find current position
                        int current_pos = -1;
                        for (int j = 0; j < house_system_count; j++) {
                            if (house_systems[j][0] == options.house_system) {
                                current_pos = j;
                                break;
                            }
                        }
                        
                        // Move to previous system
                        if (current_pos >= 0) {
                            int prev_pos = (current_pos - 1 + house_system_count) % house_system_count;
                            options.house_system = house_systems[prev_pos][0];
                        } else {
                            // If not found, start with first
                            options.house_system = house_systems[0][0];
                        }
                    }
                }
                else if (campo_atual == 2) {
                    // Cycle backwards through triplicity systems
                    if (triplicity_count > 0) {
                        // Find current position
                        int current_pos = -1;
                        for (int j = 0; j < triplicity_count; j++) {
                            if (triplicity_ids[j] == options.triplicity_system) {
                                current_pos = j;
                                break;
                            }
                        }
                        
                        // Move to previous system
                        if (current_pos >= 0) {
                            int prev_pos = (current_pos - 1 + triplicity_count) % triplicity_count;
                            options.triplicity_system = triplicity_ids[prev_pos];
                        } else {
                            // If not found, start with first
                            options.triplicity_system = triplicity_ids[0];
                        }
                    }
                }
                else if (campo_atual == 3) {
                    // Cycle backwards through terms systems
                    if (terms_count > 0) {
                        // Find current position
                        int current_pos = -1;
                        for (int j = 0; j < terms_count; j++) {
                            if (terms_ids[j] == options.terms_system) {
                                current_pos = j;
                                break;
                            }
                        }
                        
                        // Move to previous system
                        if (current_pos >= 0) {
                            int prev_pos = (current_pos - 1 + terms_count) % terms_count;
                            options.terms_system = terms_ids[prev_pos];
                        } else {
                            // If not found, start with first
                            options.terms_system = terms_ids[0];
                        }
                    }
                }
                else if (campo_atual >= 4 && campo_atual <= 14) {
                    // Decrement planet orbis value
                    int planet_index = campo_atual - 3;
                    if (planet_orbis[planet_index] > 2.0) {
                        planet_orbis[planet_index] -= 0.5;
                    }
                }
                else if (campo_atual == 15) {
                    if (parallel_orbis > 1.0) {
                        parallel_orbis -= 0.5;
                    }
                }
                else if (campo_atual == 16) {
                    options.modern_planets_rulling = !options.modern_planets_rulling;
                }
                else if (campo_atual == 17) {
                    options.show_modern_planets = !options.show_modern_planets;
                }
                else if (campo_atual == 18) {
                    if (options.gender > 0) options.gender--;
                    if (options.gender == 0) options.gender = 3;
                }
                else if (campo_atual == 19) {
                    if (lang_count > 0) {
                        // Find current position
                        int current_pos = -1;
                        for (int j = 0; j < lang_count; j++) {
                            if (strcmp(lang_cods[j], options.language) == 0) {
                                current_pos = j;
                                break;
                            }
                        }
                        
                        // Move to previous system
                        if (current_pos >= 0) {
                            int prev_pos = (current_pos - 1 + lang_count) % lang_count;
                            snprintf(options.language, 10, "%s", lang_cods[prev_pos]);
                        } else {
                            snprintf(options.language, 10, "%s", lang_cods[0]);
                        }
                    }
                }
                break;
            case 10: // Enter
                // Update the database with new planet orbis values
                db = open_database();
                if (db) {
                    for (int i = 0; i < 12; i++) {
                        
                        // for south node
                        if (i == 11) planet_orbis[i] = planet_orbis[i - 1];
                        
                        const char *sql_update = "UPDATE planets SET orbis = ? WHERE id = ?";
                        rc = sqlite3_prepare_v2(db, sql_update, -1, &stmt, NULL);
                        if (rc == SQLITE_OK) {
                            sqlite3_bind_double(stmt, 1, planet_orbis[i]);
                            sqlite3_bind_int(stmt, 2, i + 1);
                            sqlite3_step(stmt);
                            sqlite3_finalize(stmt);
                        }
                    }

                    const char *sql_update2 = "UPDATE profiles SET parallel_orbis = ? WHERE profile = 'default";
                    rc = sqlite3_prepare_v2(db, sql_update2, -1, &stmt, NULL);
                    if (rc == SQLITE_OK) {
                        sqlite3_bind_double(stmt, 1, parallel_orbis);
                        sqlite3_step(stmt);
                        sqlite3_finalize(stmt);
                    }

                    close_database(db);
                }
                options_confirmed = 1;
                ed.options = options;
                ed.changed = 1;
                break;
            case 27: // ESC
                // Cleanup allocated memory
                if (house_systems) {
                    for (int i = 0; i < house_system_count; i++) {
                        free(house_systems[i]);
                    }
                    free(house_systems);
                }
                if (house_system_names) {
                    for (int i = 0; i < house_system_count; i++) {
                        free(house_system_names[i]);
                    }
                    free(house_system_names);
                }
                if (triplicity_ids) {
                    free(triplicity_ids);
                }
                if (triplicity_names) {
                    for (int i = 0; i < triplicity_count; i++) {
                        free(triplicity_names[i]);
                    }
                    free(triplicity_names);
                }
                if (terms_ids) {
                    free(terms_ids);
                }
                if (terms_names) {
                    for (int i = 0; i < terms_count; i++) {
                        free(terms_names[i]);
                    }
                    free(terms_names);
                }
                if (lang_names) {
                    for (int i = 0; i < lang_count; i++) {
                        free(lang_names[i]);
                    }
                    free(lang_names);
                }
                if (lang_cods) {
                    for (int i = 0; i < lang_count; i++) {
                        free(lang_cods[i]);
                    }
                    free(lang_cods);
                }
                delwin(win);
                ed.changed = 0;
                return ed;
        }
    }

    // Cleanup allocated memory
    if (house_systems) {
        for (int i = 0; i < house_system_count; i++) {
            free(house_systems[i]);
        }
        free(house_systems);
    }
    if (house_system_names) {
        for (int i = 0; i < house_system_count; i++) {
            free(house_system_names[i]);
        }
        free(house_system_names);
    }
    if (triplicity_ids) {
        free(triplicity_ids);
    }
    if (triplicity_names) {
        for (int i = 0; i < triplicity_count; i++) {
            free(triplicity_names[i]);
        }
        free(triplicity_names);
    }
    if (terms_ids) {
        free(terms_ids);
    }
    if (terms_names) {
        for (int i = 0; i < terms_count; i++) {
            free(terms_names[i]);
        }
        free(terms_names);
    }
    if (lang_names) {
        for (int i = 0; i < lang_count; i++) {
            free(lang_names[i]);
        }
        free(lang_names);
    }
    if (lang_cods) {
        for (int i = 0; i < lang_count; i++) {
            free(lang_cods[i]);
        }
        free(lang_cods);
    }
    
    delwin(win);
    return ed;
}



double selecionar_idade_visual_fracionada(double idade_inicial) {
    double idade = idade_inicial;
    int confirmado = 0;
    int key;

    unsigned short term_w = 80, term_h = 24;
    getmaxyx(stdscr, term_h, term_w);
    
    int w_width = 45, w_height = 8;
    WINDOW *win = newwin(w_height, w_width, (term_h - w_height)/2, (term_w - w_width)/2);
    WINDOW *shadow = newwin(w_height, w_width, (term_h - w_height)/2 + 1, (term_w - w_width)/2 + 1);
    
    keypad(win, TRUE);
    curs_set(0); /* Oculta o cursor piscante */

    while (!confirmado) {
        /* Renderiza sombra (Usando seu par de cor 9) */
        werase(shadow);
        wattron(shadow, COLOR_PAIR(9)); 
        box(shadow, 0, 0);
        wattroff(shadow, COLOR_PAIR(9));
        wrefresh(shadow); /* Mantendo seu refresh de sombra */

        /* Renderiza Janela Principal (Pares de cor 13) */
        werase(win);
        wattron(win, COLOR_PAIR(13));
        wbkgd(win, COLOR_PAIR(13) | FLAGS);
        box(win, 0, 0);

        wattron(win, A_BOLD);

        const char *title = _(" Target Age Selection ");

        mvwprintw(win, 0, (w_width - get_visual_width(title)) / 2, title);
        wattroff(win, A_BOLD);

        mvwprintw(win, 4, 3, _("Use [↑/↓] [PgUp/PgDn] to adjust."));
        mvwprintw(win, 5, 3, _("[Enter] confirm | [ESC] cancel."));
        wattroff(win, COLOR_PAIR(13));

        /* Renderiza o campo de idade destacado com uma casa decimal (%.1f) */
        wattron(win, COLOR_PAIR(8) | A_BOLD | A_REVERSE);

        const char *age_text = _("Age");
        const char *years_text = _("years");
        mvwprintw(win, 2, 12, "  %s: %8.4f %s  ", age_text, idade, years_text);
        wattroff(win, COLOR_PAIR(8) | A_BOLD | A_REVERSE);

        wrefresh(win);
        key = wgetch(win);

        switch (key) {
            case KEY_UP:
                /* Incrementa em frações de 0.1 (uma casa decimal por clique) */
                if (idade < 120.0) {
                    idade += 0.1;
                }
                break;
            case KEY_DOWN:
                /* Decrementa em frações de 0.1 */
                if (idade > 0.0) {
                    idade -= 0.1;
                }
                else {
                    idade = 0.0;
                }
                break;
            case KEY_PPAGE:
                /* Incrementa em 1 */
                if (idade < 120.0) {
                    idade += 1.0;
                }
                break;
            case KEY_NPAGE:
                /* Decrementa em 1 */
                if (idade > 0.0) {
                    idade -= 1.0;
                }
                else {
                    idade = 0.0;
                }
                break;
            case 10: /* ENTER */
                confirmado = 1;
                break;
            case 27: /* ESC */
                delwin(win);
                delwin(shadow);
                return -1.0; /* Retorna flag de cancelado em double */
        }
    }

    delwin(win);
    delwin(shadow);
    return idade;
}




int selecionar_idade_visual(int idade_inicial) {
    int idade = idade_inicial;
    int confirmado = 0;
    int key;

    unsigned short term_w = 80, term_h = 24;
    getmaxyx(stdscr, term_h, term_w);
    
    int w_width = 45, w_height = 8;
    WINDOW *win = newwin(w_height, w_width, (term_h - w_height)/2, (term_w - w_width)/2);
    WINDOW *shadow = newwin(w_height, w_width, (term_h - w_height)/2 + 1, (term_w - w_width)/2 + 1);
    
    keypad(win, TRUE);
    curs_set(0); // Oculta o cursor piscante

    while (!confirmado) {
        // Renderiza sombra (Usando seu par de cor 4 ou 9 dependendo do seu setup)
        werase(shadow);
        wattron(shadow, COLOR_PAIR(9)); 
        box(shadow, 0, 0);
        wattroff(shadow, COLOR_PAIR(9));
        wrefresh(shadow);

        // Renderiza Janela Principal (Pares de cor 2 ou 13 conforme seu padrão)
        werase(win);
        wattron(win, COLOR_PAIR(13));
        wbkgd(win, COLOR_PAIR(13) | FLAGS);
        box(win, 0, 0);

        wattron(win, A_BOLD);
        const char *title = _(" Target Age Selection ");

        mvwprintw(win, 0, (w_width - get_visual_width(title)) / 2, title);
        wattroff(win, A_BOLD);
        
        mvwprintw(win, 4, 3, _("Use [↑/↓] to adjust. [Enter] to confirm."));
        mvwprintw(win, 5, 3, _("[ESC] to cancel."));
        wattroff(win, COLOR_PAIR(13));

        // Renderiza o campo de idade destacado (Pares de cor 3 ou 8)
        wattron(win, COLOR_PAIR(8) | A_BOLD | A_REVERSE);
        const char *age_text = _("Age");
        const char *years_text = _("years");
        mvwprintw(win, 2, 12, "  %s: %3d %s  ", age_text, idade, years_text);
        wattroff(win, COLOR_PAIR(8) | A_BOLD | A_REVERSE);

        wrefresh(win);
        key = wgetch(win);

        switch (key) {
            case KEY_UP:
                if (idade < 120) idade++; // Limite humano seguro
                break;
            case KEY_DOWN:
                if (idade > 0) idade--;
                break;
            case 10: // ENTER
                confirmado = 1;
                break;
            case 27: // ESC
                delwin(win);
                delwin(shadow);
                return -1; // Retorna flag de cancelado
        }
    }

    delwin(win);
    delwin(shadow);
    return idade;
}





int select_gender() {
    int gender_id = 1;
    int confirmado = 0;
    int key;

    unsigned short term_w = 80, term_h = 24;
    getmaxyx(stdscr, term_h, term_w);
    
    int w_width = 45, w_height = 8;
    WINDOW *win = newwin(w_height, w_width, (term_h - w_height)/2, (term_w - w_width)/2);
    WINDOW *shadow = newwin(w_height, w_width, (term_h - w_height)/2 + 1, (term_w - w_width)/2 + 1);
    
    keypad(win, TRUE);
    curs_set(0); // Oculta o cursor piscante

    while (!confirmado) {
        // Renderiza sombra (Usando seu par de cor 4 ou 9 dependendo do seu setup)
        werase(shadow);
        wattron(shadow, COLOR_PAIR(4)); 
        box(shadow, 0, 0);
        wattroff(shadow, COLOR_PAIR(4));
        wrefresh(shadow);

        // Renderiza Janela Principal (Pares de cor 2 ou 13 conforme seu padrão)
        werase(win);
        wattron(win, COLOR_PAIR(2));
        wbkgd(win, COLOR_PAIR(2) | FLAGS);
        box(win, 0, 0);
        
        wattron(win, A_BOLD);
        const char *title = _(" Gender Selection ");
        mvwprintw(win, 0, (w_width - get_visual_width(title)) / 2, title);
        wattroff(win, A_BOLD);
        
        mvwprintw(win, 4, 3, _("Use [↑/↓] to adjust. [Enter] to confirm."));
        mvwprintw(win, 5, 3, _("[ESC] to cancel."));
        wattroff(win, COLOR_PAIR(2));

        // Renderiza o campo gênero destacado (Pares de cor 3 ou 8)
        wattron(win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);

        const char *gen_text = _("Gender");
        mvwprintw(win, 2, 14, "  %s: %3s  ", gen_text, (gender_id == 1)?_("Masculine"):((gender_id == 2)?_("Feminine"):_("Neuter")));
        wattroff(win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);

        wrefresh(win);
        key = wgetch(win);

        switch (key) {
            case KEY_UP:
                if (gender_id < 4) gender_id++;
                if (gender_id == 4) gender_id = 1;
                break;
            case KEY_DOWN:
                if (gender_id > 0) gender_id--;
                if (gender_id == 0) gender_id = 3;
                break;
            case 10: // ENTER
                confirmado = 1;
                break;
            case 27: // ESC
                delwin(win);
                delwin(shadow);
                return 0;
        }
    }

    delwin(win);
    delwin(shadow);
    return gender_id;
}



DateEdition selecionar_data() {
    //DataNascimento dt = {1850, 3, 15}; // Padrão inicial histórico
    DataNascimento dt = {YY, MM, DD};
    DataNascimento dt_prev = {YY, MM, DD};

    DateEdition ed;
    ed.date = dt;
    ed.changed = 0;
    
    int campo_atual = 0; // 0=Ano, 1=Mês, 2=Dia
    int key;
    int data_confirmada = 0;

    unsigned short term_w = 80, term_h = 24;
    getmaxyx(stdscr, term_h, term_w); // Captura o tamanho do terminal atual
    
    int w_width = 50, w_height = 9;
    WINDOW *win = newwin(w_height, w_width, (term_h - w_height)/2, (term_w - w_width)/2);
    WINDOW *shadow = newwin(w_height, w_width, (term_h - w_height)/2 + 1, (term_w - w_width)/2 + 1);
    keypad(win, TRUE);
    curs_set(0); // Oculta o cursor piscante para navegação visual

    int max_dias[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    while (!data_confirmada) {
        werase(shadow);
        wattron(shadow, COLOR_PAIR(4));
        box(shadow, 0, 0);
        wattroff(shadow, COLOR_PAIR(4));
        wrefresh(shadow);

        werase(win);
        wattron(win, COLOR_PAIR(2));
        wbkgd(win, COLOR_PAIR(2) | FLAGS);
        box(win, 0, 0);
        
        wattron(win, A_BOLD);
        const char *title = _(" Date ");
        mvwprintw(win, 0, (w_width - get_visual_width(title)) / 2, title);
        wattroff(win, A_BOLD);
        
        mvwprintw(win, 5, 2, _("Use [←↓↑→] to ajust. [Enter] confirm."));
        mvwprintw(win, 6, 2, _("[ESC] Cancel."));
        wattroff(win, COLOR_PAIR(2));

        // Validação simples de ano bissexto para o mês de Fevereiro
        if ((dt.ano % 4 == 0 && dt.ano % 100 != 0) || (dt.ano % 400 == 0)) {
            max_dias[1] = 29;
        } else {
            max_dias[1] = 28;
        }
        if (dt.dia > max_dias[dt.mes - 1]) dt.dia = max_dias[dt.mes - 1];

        // Renderização dos campos com destaque no selecionado
        for (int i = 0; i < 3; i++) {
            if (i == campo_atual) wattron(win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
            else wattron(win, COLOR_PAIR(2));

            const char *yt = _("Year");
            const char *mt = _("Month");
            const char *dayt = _("Day");

            if (i == 0) mvwprintw(win, 2, 5, " %s: %04d ", yt, dt.ano);
            if (i == 1) mvwprintw(win, 2, 18, " %s: %02d ", mt, dt.mes);
            if (i == 2) mvwprintw(win, 2, 29, " %s: %02d ", dayt, dt.dia);

            if (i == campo_atual) wattroff(win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
            else wattroff(win, COLOR_PAIR(2));
        }

        wrefresh(win);
        key = wgetch(win);

        switch (key) {
            case KEY_LEFT:
                campo_atual = (campo_atual - 1 + 3) % 3;
                break;
            case KEY_RIGHT:
                campo_atual = (campo_atual + 1) % 3;
                break;
            case KEY_UP:
                if (campo_atual == 0) dt.ano++;
                if (campo_atual == 1) { dt.mes++; if(dt.mes > 12) dt.mes = 1; }
                if (campo_atual == 2) { dt.dia++; if(dt.dia > max_dias[dt.mes - 1]) dt.dia = 1; }
                break;
            case KEY_DOWN:
                if (campo_atual == 0) dt.ano--;
                if (campo_atual == 1) { dt.mes--; if(dt.mes < 1) dt.mes = 12; }
                if (campo_atual == 2) { dt.dia--; if(dt.dia < 1) dt.dia = max_dias[dt.mes - 1]; }
                break;
            case 10: // Enter
                data_confirmada = 1;
                ed.date = dt;
                ed.changed = 1;
                break;
            case 27: // ESC
                delwin(win);
                ed.changed =0;
                ed.date = dt_prev;
                return ed;
        }
    }

    delwin(win);
    return ed;
}


HoraEdition selecionar_hora() {
    Hora hn = {HH, MIN, SEC};
    Hora hr_prev = {HH, MIN, SEC};

    HoraEdition ed;
    ed.hora = hn;
    ed.changed = 0;

    int campo_atual = 0; // 0=Hora, 1=Minuto
    int key;
    int horario_confirmado = 0;

    // Captura o tamanho atual do terminal
    unsigned short term_w = 80, term_h = 24;
    getmaxyx(stdscr, term_h, term_w);
    
    // Dimensões da janela centralizada
    int w_width = 50, w_height = 9;
    WINDOW *win = newwin(w_height, w_width, (term_h - w_height)/2, (term_w - w_width)/2);
    WINDOW *shadow = newwin(w_height, w_width, (term_h - w_height)/2 + 1, (term_w - w_width)/2 + 1);
    keypad(win, TRUE);
    curs_set(0); // Oculta o cursor para navegação puramente visual

    while (!horario_confirmado) {
        werase(shadow);
        wattron(shadow, COLOR_PAIR(4));
        box(shadow, 0, 0);
        wattroff(shadow, COLOR_PAIR(4));
        wrefresh(shadow);

        werase(win);
        wattron(win, COLOR_PAIR(2));
        wbkgd(win, COLOR_PAIR(2) | FLAGS);
        box(win, 0, 0);
        
        wattron(win, A_BOLD);
        const char *title = _(" Time ");
        mvwprintw(win, 0, (w_width - get_visual_width(title)) / 2, title);
        wattroff(win, A_BOLD);
        
        mvwprintw(win, 5, 2, _("Use [←↓↑→] to ajust. [Enter] confirm."));
        mvwprintw(win, 6, 2, _("[ESC] Cancel."));
        wattroff(win, COLOR_PAIR(2));

        // Renderização dos campos com destaque reverso no ativo
        for (int i = 0; i < 3; i++) {
            if (i == campo_atual) {
                wattron(win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
            } else {
                wattron(win, COLOR_PAIR(2));
            }

            const char *ht = _("Hour");
            const char *mt = _("Min");
            const char *st = _("Sec");
            if (i == 0) mvwprintw(win, 2, 5, " %s: %02d ", ht, hn.hora);
            if (i == 1) mvwprintw(win, 2, 18, " %s: %02d ", mt, hn.min);
            if (i == 2) mvwprintw(win, 2, 29, " %s: %02d ", st, hn.sec);

            if (i == campo_atual) {
                wattroff(win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
            } else {
                wattroff(win, COLOR_PAIR(2));
            }
        }

        wrefresh(win);
        key = wgetch(win);

        switch (key) {
            case KEY_LEFT:
                campo_atual = (campo_atual - 1 + 3) % 3;
                break;
            case KEY_RIGHT:
                campo_atual = (campo_atual + 1) % 3;
                break;
                
            case KEY_UP:
                if (campo_atual == 0) {
                    hn.hora = (hn.hora + 1) % 24; // Vai de 00 a 23 de forma cíclica
                } else if (campo_atual == 1) {
                    hn.min = (hn.min + 1) % 60; // Vai de 00 a 59 de forma cíclica
                } else {
                    hn.sec = (hn.sec + 1) % 60;
                }
                break;
                
            case KEY_DOWN:
                if (campo_atual == 0) {
                    hn.hora = (hn.hora - 1 + 24) % 24;
                } else if (campo_atual == 1) {
                    hn.min = (hn.min - 1 + 60) % 60;
                } else {
                    hn.sec = (hn.sec - 1 + 60) % 60;
                }
                break;
                
            case 10: // Enter
                horario_confirmado = 1;
                ed.hora = hn;
                ed.changed = 1;
                break;
                
            case 27: // ESC
                delwin(win);
                ed.hora = hr_prev;
                ed.changed = 0;
                return ed;
        }
    }

    delwin(win);
    return ed;
}



int set_tz() {
    char input[10] = "";
    int input_pos = 0;
    int input_str_len = 0;
    int tz_selected = 0;
    int key;
    int show_error = 0; // Controla a exibição da mensagem de erro

    // Inicializa o buffer com o offset atual
    snprintf(input, sizeof(input), "%.2f", TZ_OFFSET);
    input_str_len = strlen(input);
    input_pos = input_str_len;

    // Obtém as dimensões do terminal
    unsigned short term_w = get_terminal_width();
    unsigned short term_h = get_terminal_height();
    
    // Calcula o tamanho da janela
    int menu_width = 25;
    int menu_height = 8;
    int menu_start_x = (term_w - menu_width) / 2;
    int menu_start_y = (term_h - menu_height) / 2;
    
    // Cria as janelas
    WINDOW *tz_win = newwin(menu_height, menu_width, menu_start_y, menu_start_x);
    WINDOW *tz_shadow = newwin(menu_height, menu_width, menu_start_y + 1, menu_start_x + 1);
    
    // Inicializa o ncurses para este menu
    nodelay(tz_win, FALSE);
    keypad(tz_win, TRUE);
    curs_set(1);
    
    while (!tz_selected) {
        // Limpa e desenha a sombra
        werase(tz_shadow);
        wattron(tz_shadow, COLOR_PAIR(4));
        box(tz_shadow, 0, 0);
        wattroff(tz_shadow, COLOR_PAIR(4));
        wrefresh(tz_shadow);
        
        // Limpa e desenha a janela principal
        werase(tz_win);
        wattron(tz_win, COLOR_PAIR(2) | A_DIM);
        box(tz_win, 0, 0);
        
        wattron(tz_win, A_BOLD);

        const char *title = _(" Enter TZ Offset ");
        mvwprintw(tz_win, 0, (menu_width - get_visual_width(title)) / 2, title);
        wattroff(tz_win, A_BOLD);

        wbkgd(tz_win, COLOR_PAIR(2) | FLAGS);
                
        // Desenha as instruções ou mensagem de erro
        if (show_error) {
            wattron(tz_win, COLOR_PAIR(3) | A_BOLD | A_BLINK); 
            mvwprintw(tz_win, 3, 1, _("Error: Max -12 to +14"));
            wattroff(tz_win, COLOR_PAIR(3) | A_BOLD | A_BLINK);
            mvwprintw(tz_win, 4, 1, _("Press [Enter] to save"));
        } else {
            mvwprintw(tz_win, 4, 1, _("Press [Enter] to save"));
        }
        
        mvwprintw(tz_win, 5, 1, _("Press [ESC] to cancel"));
        mvwprintw(tz_win, 2, 1, _("Value: "));
        wattroff(tz_win, COLOR_PAIR(2) | A_DIM);

        // Exibe o texto digitado
        wattron(tz_win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(tz_win, 2, 8, "%s", input);
        wattroff(tz_win, COLOR_PAIR(3) | A_BOLD);
      
        // Move o cursor físico para a posição exata da edição gráfica
        wmove(tz_win, 2, 8 + input_pos);
        wrefresh(tz_win);
        
        key = wgetch(tz_win);
        
        switch(key) {
            case 10: // Enter (\n)
            case 13: // Carriage Return (\r)
                {
                    double val = atof(input);
                    // Validação de intervalo matemático real do fuso horário mundial
                    if (val < -12.0 || val > 14.0) {
                        show_error = 1; // Ativa o aviso de erro na tela
                    } else {
                        TZ_OFFSET = val;
                        tz_selected = 1;
                    }
                }
                break;
                
            case 27: // ESC
                delwin(tz_win);
                delwin(tz_shadow);
                return 0;

            case KEY_LEFT:
                if (input_pos > 0) input_pos--;
                break;
                
            case KEY_RIGHT:
                if (input_pos < input_str_len) input_pos++;
                break;
                
            case KEY_BACKSPACE:
            case 127: 
            case 8:   
                if (input_pos > 0) {
                    for (int i = input_pos - 1; i < input_str_len; i++) {
                        input[i] = input[i + 1];
                    }
                    input_str_len--;
                    input_pos--;
                    show_error = 0; // Reseta o aviso de erro quando o usuário edita
                }
                break;
                
            case KEY_DC: // Tecla DEL
                if (input_pos < input_str_len) {
                    for (int i = input_pos; i < input_str_len; i++) {
                        input[i] = input[i + 1];
                    }
                    input_str_len--;
                    show_error = 0;
                }
                break;
                
            default:
                {
                    int is_valid_char = 0;

                    if (isdigit(key)) {
                        is_valid_char = 1;
                    } 
                    // Regra do sinal de menos: posição 0 E não pode existir outro na string
                    else if (key == '-') {
                        if (input_pos == 0 && strchr(input, '-') == NULL) {
                            is_valid_char = 1;
                        }
                    } 
                    // Regra do ponto/vírgula: só um no texto todo
                    else if (key == '.' || key == ',') {
                        if (strchr(input, '.') == NULL && strchr(input, ',') == NULL) {
                            is_valid_char = 1;
                        }
                    }

                    // Se passou nas validações de caracteres, faz a inserção por deslocamento
                    if (is_valid_char) {
                        if (input_str_len < (int)(sizeof(input) - 1)) {
                            for (int i = input_str_len; i >= input_pos; i--) {
                                input[i + 1] = input[i];
                            }
                            input[input_pos] = (char)key;
                            input_str_len++;
                            input_pos++;
                            show_error = 0; // Limpa o erro se o usuário estiver digitando
                        }
                    }
                }
                break;
        }
    }    
    delwin(tz_win);
    delwin(tz_shadow);
    return 1;
}



int set_dst() {
    char dsts[3][10];
    int dst_count = 3;
    snprintf(dsts[0], 10, " %s ", _("Auto"));
    snprintf(dsts[1], 10, " %s ", _("No  "));
    snprintf(dsts[2], 10, " %s ", _("Yes "));
    
    // Get tersecal dimensions
    unsigned short term_w = get_terminal_width();
    unsigned short term_h = get_terminal_height();
    
    // Calculate window size for country menu
    int menu_width = 26;
    int menu_height = 8;
    int menu_start_x = (term_w - menu_width) / 2;
    int menu_start_y = (term_h - menu_height) / 2;
    
    // Create windows
    WINDOW *dst_win = newwin(menu_height, menu_width, menu_start_y, menu_start_x);
    WINDOW *dst_shadow = newwin(menu_height, menu_width, menu_start_y + 1, menu_start_x + 1);
    
    int selected_dst_index = 0;  // This tracks the actual index in the array
    int dst_scroll_offset = 0;   // This tracks which item is at the top of the visible list
    int max_display_items = menu_height - 2;  // Adjust for title and borders
    
    // Initialize ncurses for this menu
    nodelay(dst_win, FALSE);
    keypad(dst_win, TRUE);
    curs_set(0);
    
    int dst_selected = 0;
    int key;
    
    while (!dst_selected) {
        // Clear and redraw shadow
        werase(dst_shadow);
        wattron(dst_shadow, COLOR_PAIR(4));
        box(dst_shadow, 0, 0);
        wattroff(dst_shadow, COLOR_PAIR(4));
        wrefresh(dst_shadow);
        
        // Clear and redraw main menu
        werase(dst_win);
        wattron(dst_win, COLOR_PAIR(2) | A_DIM);
        wbkgd(dst_win, COLOR_PAIR(2) | FLAGS);
        box(dst_win, 0, 0);        
        wattroff(dst_win, COLOR_PAIR(2) | A_DIM);

        wattron(dst_win, COLOR_PAIR(2) | A_BOLD);
        const char *title = _(" Select DST ");
        mvwprintw(dst_win, 0, (menu_width - get_visual_width(title)) / 2, title);
        wattroff(dst_win, COLOR_PAIR(2) | A_BOLD);

        wrefresh(dst_win);
        
        for (int i = 0; i < max_display_items; i++) {
            int item_index = i + dst_scroll_offset;
            if (item_index < dst_count) {
                int attr = (item_index == selected_dst_index) ? (COLOR_PAIR(3) | A_REVERSE | A_BOLD) : COLOR_PAIR(2);
                wattron(dst_win, attr);
                mvwprintw(dst_win, i + 2, 0 + (menu_width - get_visual_width(dsts[item_index]) - 2) / 2, " %s ", dsts[item_index]);
                wattroff(dst_win, attr);
            }
        }
        wrefresh(dst_win);
        
        key = wgetch(dst_win);
        
        // Handle letter jumping
        if (isalpha(key)) {
            int new_index = find_first_item_with_letter((const char**)dsts, dst_count, selected_dst_index, key);
            if (new_index != selected_dst_index) {
                selected_dst_index = new_index;
                // Adjust scroll offset to keep selected item visible
                if (selected_dst_index < dst_scroll_offset) {
                    dst_scroll_offset = selected_dst_index;
                } else if (selected_dst_index >= dst_scroll_offset + max_display_items) {
                    dst_scroll_offset = selected_dst_index - max_display_items + 1;
                }
                continue; // Skip the normal key processing
            }
        }
        
        switch(key) {
            case KEY_UP:
                if (selected_dst_index > 0) {
                    selected_dst_index--;
                    // Adjust scroll offset if needed
                    if (selected_dst_index < dst_scroll_offset) {
                        dst_scroll_offset = selected_dst_index;
                    }
                }
                break;
            case KEY_DOWN:
                if (selected_dst_index < dst_count - 1) {
                    selected_dst_index++;
                    // Adjust scroll offset if needed
                    if (selected_dst_index >= dst_scroll_offset + max_display_items) {
                        dst_scroll_offset = selected_dst_index - max_display_items + 1;
                    }
                }
                break;
            case 10: // Enter
                dst_selected = 1;
                break;
            case 27: // ESC
                // Cleanup and return
                //free_string_array(dsts, dst_count);
                delwin(dst_win);
                delwin(dst_shadow);
                return 0;
        }
    }
    
    // Get the selected country
    char *selected_dst = dsts[selected_dst_index];

    if (strcmp(selected_dst, _("Yes ")) == 0) {
        DST = 1; 
    }
    else if (strcmp(selected_dst, _("No  ")) == 0) {
        DST = 0; 
    }
    else {
        DST = -1;
    }

    delwin(dst_win);
    delwin(dst_shadow);

    return 1;

}





void set_chart_name(char *chart_name, size_t max_length) {
    unsigned short term_w = get_terminal_width();
    unsigned short term_h = get_terminal_height();
    
    int dialog_width = 55;
    int dialog_height = 8;
    int dialog_start_x = (term_w - dialog_width) / 2;
    int dialog_start_y = (term_h - dialog_height) / 2;
    
    WINDOW *dialog_win = newwin(dialog_height, dialog_width, dialog_start_y, dialog_start_x);
    WINDOW *dialog_shadow = newwin(dialog_height, dialog_width, dialog_start_y + 1, dialog_start_x + 1);
    
    nodelay(dialog_win, FALSE);
    keypad(dialog_win, TRUE);
    curs_set(1); 
    
    // Desenha a sombra
    werase(dialog_shadow);
    wattron(dialog_shadow, COLOR_PAIR(4));
    box(dialog_shadow, 0, 0);
    wattroff(dialog_shadow, COLOR_PAIR(4));
    wrefresh(dialog_shadow);
    
    // Cria um buffer interno de caracteres largos (wchar_t) para evitar quebra de UTF-8
    wchar_t w_buffer[max_length];
    size_t w_len = 0;
    int input_pos = 0;

    // Converte a string de entrada atual (se houver) de char* para wchar_t*
    if (chart_name != NULL && get_visual_width(chart_name) > 0) {
        w_len = mbstowcs(w_buffer, chart_name, max_length - 1);
        if (w_len == (size_t)-1) {
            w_len = 0;
        }
        input_pos = w_len;
    }
    w_buffer[w_len] = L'\0';

    int done = 0;
    wint_t key; // Variável correta para wget_wch (suporta códigos especiais e wchar_t)
    int key_type;
    
    while (!done) {
        // Renderiza e limpa a janela com segurança
        werase(dialog_win);
        wbkgd(dialog_win, COLOR_PAIR(2) | FLAGS);
        
        wattron(dialog_win, COLOR_PAIR(2) | A_DIM);
        box(dialog_win, 0, 0);
        wattroff(dialog_win, COLOR_PAIR(2) | A_DIM);
        
        wattron(dialog_win, A_BOLD);
        const char *title = _(" Chart's Name ");

        mvwprintw(dialog_win, 0, (dialog_width - get_visual_width(title)) / 2, title);
        wattroff(dialog_win, A_BOLD);

        wattron(dialog_win, COLOR_PAIR(2));

        const char *str1 = _("Enter a name for your chart (max");
        const char *str2 = _("characters)");

        mvwprintw(dialog_win, 3, 1, "%s %zu %s:", str1, max_length - 1, str2);
        wattroff(dialog_win, COLOR_PAIR(2));

        // Exibe a string usando a função de caracteres largos do ncursesw
        wattron(dialog_win, COLOR_PAIR(28) | A_REVERSE);
        mvwprintw(dialog_win, 5, 2, ">                                                  ");
        mvwaddwstr(dialog_win, 5, 4, w_buffer);
        wattroff(dialog_win, COLOR_PAIR(28) | A_REVERSE);
        
        // Move o cursor físico para a posição baseada no número de caracteres (e não de bytes)
        wmove(dialog_win, 5, 4 + input_pos);
        wrefresh(dialog_win);
        
        // wget_wch retorna se é uma tecla especial (KEY_CODE_YES) ou um caractere comum
        key_type = wget_wch(dialog_win, &key);
        
        if (key_type == KEY_CODE_YES) {
            // Tratamento de teclas especiais mapeadas pelo ncurses
            switch (key) {
                case KEY_LEFT:
                    if (input_pos > 0) input_pos--;
                    break;
                    
                case KEY_RIGHT:
                    if (input_pos < (int)w_len) input_pos++;
                    break;
                    
                case KEY_BACKSPACE:
                    if (input_pos > 0) {
                        for (int i = input_pos - 1; i < (int)w_len; i++) {
                            w_buffer[i] = w_buffer[i + 1];
                        }
                        w_len--;
                        input_pos--;
                    }
                    break;
                    
                case KEY_DC: // Tecla DEL física
                    if (input_pos < (int)w_len) {
                        for (int i = input_pos; i < (int)w_len; i++) {
                            w_buffer[i] = w_buffer[i + 1];
                        }
                        w_len--;
                    }
                    break;
            }
        } else {
            // Tratamento de caracteres normais e emuladores de terminal brutos
            switch (key) {
                case 10: // Enter (\n)
                case 13: // Carriage Return (\r)
                    done = 1;
                    break;
                    
                case 27: // ESC
                    w_buffer[0] = L'\0';
                    w_len = 0;
                    done = 1;
                    break;
                    
                case 127: // Backspace emulado como DEL por terminais modernos
                case 8:   // Backspace clássico Ctrl+H
                    if (input_pos > 0) {
                        for (int i = input_pos - 1; i < (int)w_len; i++) {
                            w_buffer[i] = w_buffer[i + 1];
                        }
                        w_len--;
                        input_pos--;
                    }
                    break;
                    
                default:
                    // Verifica se ainda há espaço no buffer e se é um caractere imprimível válido
                    if (w_len < (max_length - 1) && iswprint(key)) {
                        // Abre espaço para inserção no meio do texto (Shift Right)
                        for (int i = (int)w_len; i >= input_pos; i--) {
                            w_buffer[i + 1] = w_buffer[i];
                        }
                        w_buffer[input_pos] = key;
                        w_len++;
                        input_pos++;
                    }
                    break;
            }
        }
    }
    
    curs_set(0);
    delwin(dialog_win);
    delwin(dialog_shadow);
    
    // Converte o buffer largo de volta para a string char* UTF-8 de saída
    if (chart_name != NULL) {
        if (w_len == 0) {
            strncpy(chart_name, _("Unnamed Chart"), max_length);
        } else {
            wcstombs(chart_name, w_buffer, max_length);
        }
    }
}


#define MAX_LINHA_TAM 256


int *get_topics_grep(char ***lines, int *line_count, char *file) {
    if (file == NULL || strlen(file) == 0) {
        *line_count = 0;
        return NULL;
    }

    // Abre o arquivo diretamente pelo C (sem popen)
    FILE *fp = fopen(file, "r");
    if (fp == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", file);
        *line_count = 0;
        return NULL;
    }

    regex_t regex;
    int re_status = regcomp(&regex, "^Index [0-9][0-9]:", REG_EXTENDED);
    if (re_status != 0) {
        fclose(fp);
        *line_count = 0;
        return NULL;
    }

    char buffer[MAX_LINHA_TAM];
    int *indices = NULL;
    int contador = 0;
    int numero_linha_atual = 0;

    // Lê o arquivo linha por linha
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        numero_linha_atual++; // Controla o número da linha nativamente
        
        // Executa a busca da regex na linha atual
        if (regexec(&regex, buffer, 0, NULL, 0) == 0) {
            // Remove o '\n' do fim da linha
            buffer[strcspn(buffer, "\n")] = '\0';

            // Aloca o índice (número da linha atual do arquivo)
            int *temp_indices = realloc(indices, (contador + 1) * sizeof(int));
            if (temp_indices == NULL) break;
            indices = temp_indices;
            indices[contador] = numero_linha_atual;

            // Aloca espaço no array de strings
            char **temp_lines = realloc(*lines, (contador + 1) * sizeof(char*));
            if (temp_lines == NULL) break;
            *lines = temp_lines;

            // Copia o conteúdo inteiro da linha encontrada
            (*lines)[contador] = malloc(strlen(buffer) + 1);
            if ((*lines)[contador] != NULL) {
                strcpy((*lines)[contador], buffer);
            }
            contador++;
        }
    }

    // Libera a memória da regex e fecha o arquivo
    regfree(&regex);
    fclose(fp);

    *line_count = contador;
    return indices;
}


int select_topic(char *file) {
           
    // Get list of countries
    char **topics = NULL;
    int *indices;
    int topic_count = 0;
    
    indices = get_topics_grep(&topics, &topic_count, file);
    
    if (topic_count == 0) {
        return -1; 
    }

    
    // Get terminal dimensions
    unsigned short term_w = get_terminal_width();
    unsigned short term_h = get_terminal_height();
    
    // Calculate window size for country menu
    int menu_width = 64;
    int menu_height = 15;
    int menu_start_x = (term_w - menu_width) / 2;
    int menu_start_y = (term_h - menu_height) / 2;
    
    // Create windows
    WINDOW *win = newwin(menu_height, menu_width, menu_start_y, menu_start_x);
    WINDOW *shadow = newwin(menu_height, menu_width, menu_start_y + 1, menu_start_x + 1);
    
    // Country selection variables
    int selected_topic_index = 0;  // This tracks the actual index in the array
    int topic_scroll_offset = 0;   // This tracks which item is at the top of the visible list
    int max_display_items = menu_height - 2;  // Adjust for title and borders
    
    // Initialize ncurses for this menu
    nodelay(win, FALSE);
    keypad(win, TRUE);
    curs_set(0);
    
    int topic_selected = 0;
    int key;

    // Clear and draw shadow
    werase(shadow);
    wattron(shadow, COLOR_PAIR(4));
    box(shadow, 0, 0);
    wattroff(shadow, COLOR_PAIR(4));
    wrefresh(shadow);
        
    while (!topic_selected) {

        // Clear and redraw main menu
        werase(win);
        wattron(win, COLOR_PAIR(2) | A_DIM);
        box(win, 0, 0);        
        wbkgd(win, COLOR_PAIR(2) | FLAGS);
        wattroff(win, COLOR_PAIR(2) | A_DIM);
        wrefresh(win);
        
        wattron(win, A_BOLD);
        const char *title = _(" Select a Topic ");
        mvwprintw(win, 0, (menu_width - get_visual_width(title)) / 2, title);
        wattroff(win, A_BOLD);
        
        
        // Draw topic items with proper scrolling
        for (int i = 0; i < max_display_items; i++) {
            int item_index = i + topic_scroll_offset;
            if (item_index < topic_count) {
                int attr = (item_index == selected_topic_index) ? (COLOR_PAIR(23) | A_REVERSE | A_BOLD) : COLOR_PAIR(22);
                wattron(win, attr);
                mvwprintw(win, i + 1, 1, " %s%*s ", topics[item_index], (menu_width - 4) - get_visual_width(topics[item_index]), " ");
                wattroff(win, attr);
            }
        }
        wrefresh(win);
        
        key = wgetch(win);
        
        // Handle letter jumping
        if (isalpha(key)) {
            int new_index = find_first_item_with_letter_offset((const char**)topics, topic_count, selected_topic_index, key, 9);
            if (new_index != selected_topic_index) {
                selected_topic_index = new_index;
                // Adjust scroll offset to keep selected item visible
                if (selected_topic_index < topic_scroll_offset) {
                    topic_scroll_offset = selected_topic_index;
                } else if (selected_topic_index >= topic_scroll_offset + max_display_items) {
                    topic_scroll_offset = selected_topic_index - max_display_items + 1;
                }
                continue; // Skip the normal key processing
            }
        }
        
        switch(key) {
            case KEY_UP:
                if (selected_topic_index > 0) {
                    selected_topic_index--;
                    // Adjust scroll offset if needed
                    if (selected_topic_index < topic_scroll_offset) {
                        topic_scroll_offset = selected_topic_index;
                    }
                }
                break;
            case KEY_DOWN:
                if (selected_topic_index < topic_count - 1) {
                    selected_topic_index++;
                    // Adjust scroll offset if needed
                    if (selected_topic_index >= topic_scroll_offset + max_display_items) {
                        topic_scroll_offset = selected_topic_index - max_display_items + 1;
                    }
                }
                break;
            case 10: // Enter
                topic_selected = 1;
                break;
            case 27: // ESC
                // Cleanup and return
                if (topic_count > 0) {
                    for (int i = 0; i < topic_count; i++) {
                        free(topics[i]);
                    }
                    free(topics);                    
                    free(indices);
                }
                
                delwin(win);
                delwin(shadow);
                return 0;
        }
    }

    int resultado_linha = 0;
    if (topic_count > 0 && indices != NULL) {
        resultado_linha = indices[selected_topic_index];
    }

    if (topic_count > 0) {
        for (int i = 0; i < topic_count; i++) {
            free(topics[i]);
        }
        free(topics);
        free(indices);
    }
    
    delwin(win);
    delwin(shadow);

    return resultado_linha; 
}


int load_city_coordinates(char *city_chart, char *country_chart, char *state_chart, char *tz_iana_chart, double *tz_offset_chart, double *lat, double *lon, double *elev) {
     
    sqlite3 *db;
    int rc;
    
    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in load_city_coordinates\n");
        return 0;
    }
    
    // Get list of countries
    char **countries = NULL;
    int country_count = 0;
    get_countries_from_db(&countries, &country_count);
    
    if (country_count == 0) {
        close_database(db);
        return 0;
    }
    
    // Get terminal dimensions
    unsigned short term_w = get_terminal_width();
    unsigned short term_h = get_terminal_height();
    
    // Calculate window size for country menu
    int menu_width = 87;
    int menu_height = 20;
    int menu_start_x = (term_w - menu_width) / 2;
    int menu_start_y = (term_h - menu_height) / 2;
    
    // Create windows
    WINDOW *country_win = newwin(menu_height, menu_width, menu_start_y, menu_start_x);
    WINDOW *country_shadow = newwin(menu_height, menu_width, menu_start_y + 1, menu_start_x + 1);
    
    // Country selection variables
    int selected_country_index = 0;  // This tracks the actual index in the array
    int country_scroll_offset = 0;   // This tracks which item is at the top of the visible list
    int max_display_items = menu_height - 4;  // Adjust for title and borders
    
    // Initialize ncurses for this menu
    nodelay(country_win, FALSE);
    keypad(country_win, TRUE);
    curs_set(0);
    
    int country_selected = 0;
    int key;

    // Clear and draw shadow
    werase(country_shadow);
    wattron(country_shadow, COLOR_PAIR(24));
    box(country_shadow, 0, 0);
    wattroff(country_shadow, COLOR_PAIR(24));
    wrefresh(country_shadow);
    
    while (!country_selected) {

        // Clear and redraw main menu
        werase(country_win);
        wattron(country_win, COLOR_PAIR(22) | A_DIM);
        box(country_win, 0, 0);        
        wbkgd(country_win, COLOR_PAIR(22) | FLAGS);
        wattroff(country_win, COLOR_PAIR(22) | A_DIM);

        wattron(country_win, A_BOLD);

        const char *title1 = _("Select Country");
        mvwprintw(country_win, 0, (menu_width - get_visual_width(title1)) / 2, title1);
        mvwprintw(country_win, 1, 2, _("Country"));
        wattroff(country_win, A_BOLD);

        wattron(country_win, COLOR_PAIR(29) | A_DIM);
        mvwprintw(country_win, 2, 2, "────────────────────────────────────────────────────────────────────────────────────");
        wattroff(country_win, COLOR_PAIR(29) | A_DIM);
        
        wrefresh(country_win);
        
        // Draw country items with proper scrolling
        for (int i = 0; i < max_display_items; i++) {
            int item_index = i + country_scroll_offset;
            if (item_index < country_count) {
                int attr = (item_index == selected_country_index) ? (COLOR_PAIR(23) | A_REVERSE | A_BOLD) : COLOR_PAIR(22);
                wattron(country_win, attr);
                mvwprintw(country_win, i + 3, 1, " %s%*s ", countries[item_index], 83 - get_visual_width(countries[item_index]), "");
                wattroff(country_win, attr);
            }
        }
        wrefresh(country_win);
        
        key = wgetch(country_win);
        
        // Handle letter jumping
        if (isalpha(key)) {
            int new_index = find_first_item_with_letter((const char**)countries, country_count, selected_country_index, key);
            if (new_index != selected_country_index) {
                selected_country_index = new_index;
                // Adjust scroll offset to keep selected item visible
                if (selected_country_index < country_scroll_offset) {
                    country_scroll_offset = selected_country_index;
                } else if (selected_country_index >= country_scroll_offset + max_display_items) {
                    country_scroll_offset = selected_country_index - max_display_items + 1;
                }
                continue; // Skip the normal key processing
            }
        }
        
        switch(key) {
            case KEY_UP:
                if (selected_country_index > 0) {
                    selected_country_index--;
                    // Adjust scroll offset if needed
                    if (selected_country_index < country_scroll_offset) {
                        country_scroll_offset = selected_country_index;
                    }
                }
                break;
            case KEY_DOWN:
                if (selected_country_index < country_count - 1) {
                    selected_country_index++;
                    // Adjust scroll offset if needed
                    if (selected_country_index >= country_scroll_offset + max_display_items) {
                        country_scroll_offset = selected_country_index - max_display_items + 1;
                    }
                }
                break;
            case 10: // Enter
                country_selected = 1;
                break;
            case 27: // ESC
                // Cleanup and return
                free_string_array(countries, country_count);
                delwin(country_win);
                delwin(country_shadow);
                close_database(db);
                return 0;
        }
    }
    
    // Get the selected country
    const char *selected_country_name = countries[selected_country_index];
    //snprintf(COUNTRY, sizeof(COUNTRY), "%s", selected_country_name); 
    

    // Get cities for this country
    char **cities = NULL;
    char **states = NULL;
    int city_count = 0;
    get_cities_from_db(selected_country_name, &cities, &states, &city_count);
    
    if (city_count == 0) {
        free_string_array(countries, country_count);
        free_string_array(cities, city_count);
        free_string_array(states, city_count);
        delwin(country_win);
        delwin(country_shadow);
        close_database(db);
        return 0;
    }
    
    // Create window for city selection
    WINDOW *city_win = newwin(menu_height, menu_width, menu_start_y, menu_start_x);
    WINDOW *city_shadow = newwin(menu_height, menu_width, menu_start_y + 1, menu_start_x + 1);
    
    // City selection variables
    int selected_city_index = 0;
    int city_scroll_offset = 0;
    
    int city_selected = 0;
    
    // Initialize ncurses for this menu
    nodelay(city_win, FALSE);
    keypad(city_win, TRUE);
    curs_set(0);

    // Clear and draw shadow
    werase(city_shadow);
    wattron(city_shadow, COLOR_PAIR(24));
    box(city_shadow, 0, 0);
    wattroff(city_shadow, COLOR_PAIR(24));
    wrefresh(city_shadow);
    
    while (!city_selected) {
                
        // Clear and redraw main menu
        werase(city_win);
        wattron(city_win, COLOR_PAIR(22) | A_DIM);
        wbkgd(city_win, COLOR_PAIR(22) | FLAGS);
        box(city_win, 0, 0);
        wattroff(city_win, COLOR_PAIR(22) | A_DIM);

        wattron(city_win, A_BOLD);

        const char *title2 = _("Select City in");
        mvwprintw(city_win, 0, (menu_width - get_visual_width(title2) - get_visual_width(selected_country_name)) / 2, "%s %s", title2, selected_country_name);
        mvwprintw(city_win, 1, 2, _("City                                          State/County/Region/Province"));
        wattroff(city_win, A_BOLD);
        
        wattron(city_win, COLOR_PAIR(29) | A_DIM);
        mvwprintw(city_win, 2, 2, "────────────────────────────────────────────────────────────────────────────────────");
        wattroff(city_win, COLOR_PAIR(29) | A_DIM);

        wrefresh(city_win);
        
        
        for (int i = 0; i < max_display_items; i++) {
            int item_index = i + city_scroll_offset;
            if (item_index < city_count) {
                int attr = (item_index == selected_city_index) ? (COLOR_PAIR(23) | A_REVERSE | A_BOLD) : COLOR_PAIR(22);
                wattron(city_win, attr);
                
                // Create a proper formatted string with better UTF-8 handling
                char display_line[256];
                int city_width = get_visual_width(cities[item_index]);
                int padding = 46 - city_width;
                if (padding < 0) padding = 0;
                int adm_width = get_visual_width(states[item_index]);
                int padding2 = 37 - adm_width;
                if (padding2 < 0) padding2 = 0;
                
                // Make sure we're handling UTF-8 properly - use mbstowcs for length calculation
                snprintf(display_line, sizeof(display_line), " %s%*s%s%*s ", 
                         cities[item_index], 
                         padding, 
                         "", 
                         states[item_index],
                         padding2,
                         "");
                
                mvwprintw(city_win, i + 3, 1, "%s", display_line);
                wattroff(city_win, attr);
            }
        }
        wrefresh(city_win);
        
        key = wgetch(city_win);
        
        // Handle letter jumping for cities
        if (isalpha(key)) {
            int new_index = find_first_item_with_letter((const char**)cities, city_count, selected_city_index, key);
            if (new_index != selected_city_index) {
                selected_city_index = new_index;
                // Adjust scroll offset to keep selected item visible
                if (selected_city_index < city_scroll_offset) {
                    city_scroll_offset = selected_city_index;
                } else if (selected_city_index >= city_scroll_offset + max_display_items) {
                    city_scroll_offset = selected_city_index - max_display_items + 1;
                }
                continue; // Skip the normal key processing
            }
        }
        
        switch(key) {
            case KEY_UP:
                if (selected_city_index > 0) {
                    selected_city_index--;
                    // Adjust scroll offset if needed
                    if (selected_city_index < city_scroll_offset) {
                        city_scroll_offset = selected_city_index;
                    }
                }
                break;
            case KEY_DOWN:
                if (selected_city_index < city_count - 1) {
                    selected_city_index++;
                    // Adjust scroll offset if needed
                    if (selected_city_index >= city_scroll_offset + max_display_items) {
                        city_scroll_offset = selected_city_index - max_display_items + 1;
                    }
                }
                break;
            case 10: // Enter
                city_selected = 1;
                break;
            case 27: // ESC
                // Cleanup and return
                free_string_array(countries, country_count);
                free_string_array(cities, city_count);
                free_string_array(states, city_count);
                delwin(country_win);
                delwin(country_shadow);
                delwin(city_win);
                delwin(city_shadow);
                close_database(db);
                return 0;
        }
    }
    
    // Now we have the selected city
    const char *selected_city_name = cities[selected_city_index];
    const char *selected_state = states[selected_city_index];
    
    // Get coordinates for the selected city
    sqlite3_stmt *stmt;
    const char *sql_select = "SELECT lat, lon, elev, country, state, gmt_offset_secs, timezone FROM cities WHERE country = ? AND state = ? COLLATE GLOBAL_SEM_ACENTO AND city = ? COLLATE GLOBAL_SEM_ACENTO;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        //fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        show_alert_popup(_("Failed to prepare statement:"), sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        close_database(db);
        free_string_array(countries, country_count);
        free_string_array(cities, city_count);
        free_string_array(states, city_count);
        delwin(country_win);
        delwin(country_shadow);
        delwin(city_win);
        delwin(city_shadow);
        
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, selected_country_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, selected_state, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, selected_city_name, -1, SQLITE_TRANSIENT);

    //double lat = 0.0, lon = 0.0;
    char country[100];
    char state[100];
    char tz_iana[100];
    int found = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        *lat = sqlite3_column_double(stmt, 0);
        *lon = sqlite3_column_double(stmt, 1);
        *elev = sqlite3_column_double(stmt, 2);
        snprintf(country, sizeof(country), "%s", sqlite3_column_text(stmt, 3));
        snprintf(state, sizeof(state), "%s", sqlite3_column_text(stmt, 4));
        int gmt_offset_secs = sqlite3_column_int(stmt, 5);
        snprintf(tz_iana, sizeof(tz_iana), "%s", sqlite3_column_text(stmt, 6));

        snprintf(city_chart, 100, "%s", selected_city_name);
        snprintf(country_chart, 100, "%s", selected_country_name);
        snprintf(state_chart, 100, "%s", selected_state);
        snprintf(tz_iana_chart, 100, "%s", tz_iana);

        double tz_offset = -1;
        tz_offset = (double)(gmt_offset_secs / 3600);
        *tz_offset_chart = tz_offset;
        

        found = 1;
    }

    if (!found) {
        fprintf(stderr, "Warning: City '%s' not found in database.\n", selected_city_name);
        show_alert_popup(_("Warning: City not found in database."), selected_city_name);
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    close_database(db);
    
    free_string_array(countries, country_count);
    free_string_array(cities, city_count);
    free_string_array(states, city_count);
    delwin(country_win);
    delwin(country_shadow);
    delwin(city_win);
    delwin(city_shadow);   

    return 1;

}


void set_default_city() {
    char city_bkp[100] = "";
    char country_bkp[100] = "";
    char state_bkp[100] = "";
    char tz_iana_bkp[100] = "";
    double tz_offset_bkp = TZ_OFFSET;
    snprintf(city_bkp, 100, "%s", CITY);
    snprintf(country_bkp, 100, "%s", COUNTRY);
    snprintf(state_bkp, 100, "%s", STATE);
    snprintf(tz_iana_bkp, 100, "%s", TZ_IANA);

    double lat = 0.0, lon = 0.0, elev = 0.0;

    if (! load_city_coordinates(DEFAULT_CITY, DEFAULT_COUNTRY, DEFAULT_STATE, DEFAULT_TZ_IANA, &DEFAULT_TZ_OFFSET, &lat, &lon, &elev)) {
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Default city has not changed"));

        return;
    }
    snprintf(DEFAULT_CITY, sizeof(DEFAULT_CITY), "%s", CITY);
    snprintf(DEFAULT_STATE, sizeof(DEFAULT_STATE), "%s", STATE);
    snprintf(DEFAULT_COUNTRY, sizeof(DEFAULT_COUNTRY), "%s", COUNTRY);

    snprintf(CITY, 100, "%s", city_bkp);
    snprintf(COUNTRY, 100, "%s", country_bkp);
    snprintf(STATE, 100, "%s", state_bkp);
    snprintf(TZ_IANA, 100, "%s", tz_iana_bkp);
    TZ_OFFSET = tz_offset_bkp;

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in set_default_city\n");
        return;
    }

    const char *sql_select_city = "SELECT id FROM cities WHERE country = ? AND state = ? GLOBAL_SEM_ACENTO AND city = ? GLOBAL_SEM_ACENTO;";
    rc = sqlite3_prepare_v2(db, sql_select_city, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return;
    }
    sqlite3_bind_text(stmt, 1, DEFAULT_COUNTRY, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, DEFAULT_STATE, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, DEFAULT_CITY, -1, SQLITE_STATIC);

    int found = 0;
    int city_id = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        city_id = sqlite3_column_int(stmt, 0);
        found = 1;
    }
    
    if (found) {
        const char *sql_select;
        sql_select = "UPDATE profiles SET city_id = ? WHERE profile = ?;";

        rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
            close_database(db);
            return;
        }

        sqlite3_bind_int(stmt, 1, city_id);
        sqlite3_bind_text(stmt, 2, "default", -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed saving default location: %s\n", sqlite3_errmsg(db));
            return;
        }
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Default city has changed"));
    }
    
    sqlite3_finalize(stmt);
    close_database(db);


}
