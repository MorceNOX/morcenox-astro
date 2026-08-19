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
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <string.h>
#include <wchar.h>
#include <time.h>
#include <sqlite3.h>
#include <menu.h>
#include <ctype.h>
#include <sys/stat.h>
#include <libintl.h>
#include "draw-chart.h"
#include "fuso-horario.h"
#include "db-utils.h"
#include "selections.h"
#include "helper.h"
#include "var.h"
#include "environment.h"

#ifndef VERSION
#define VERSION "0.0.0-unknown"
#endif

#ifndef APPLICATION_NAME
#define APPLICATION_NAME "MorceNOX-Astro"
#endif


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void call_chart_now();
void call_chart();




typedef struct {
    char *option;      // The text shown in the menu
    char *text;        // The description text shown under the menu or in the status bar
    void (*command)(void); // Pointer to the function to execute
} MenuOption;


const char *sol_7lines[] = {
    "      ",
    " ▞▀▀▖ ",
    "▐ ▗ ▐ ",
    "▝▖  ▞ ",
    " ▝▀▀  ",
    "      ",
    "      "
};

const char *lua_7lines[] = {
    "  ▗ ",
    " ▞▌ ",
    "▐▐  ",
    " ▚▌ ",
    "  ▝ ",
    "    ",
    "    "
};

const char *mercury_7lines[] = {
    "▖ ▖ ",
    "▞▀▖ ",
    "▌ ▌ ",
    "▝▛  ",
    "▄▙▖ ",
    " ▌  ",
    "    "
};

   
const char *venus_7lines[] = {
    "    ",
    "▞▀▖ ",
    "▌ ▌ ",
    "▝▛  ",
    "▄▙▖ ",
    " ▌  ",
    "    "
};

const char *pluto_7lines[] = {
    " ▗▄   ",
    "▖▚▄▘▖ ",
    "▝▄▄▞  ",
    " ▄▙▖  ",
    "  ▌   ",
    "      ",
    "      "
};

const char *Pluto_7lines[] = {
    "    ",
    "▛▀▖ ",
    "▙▄▘ ",
    "▌   ",
    "▀▀▘ ",
    "    ",
    "    "
};

const char *netuno_7lines[] = {
    "      ",
    "▌ ▌ ▌ ",
    "▝▄▙▞  ",
    " ▄▙▖  ",
    "  ▌   ",
    "      ",
    "      "
};
    
const char *urano_7lines[] = {
    "       ",
    "▖ ▗  ▖ ",
    "▐▄▟▄▟  ",
    "▞▗▟▄▝▖ ",
    " ▌  ▌  ",
    " ▝▀▀   ",
    "       "
};

const char *jupiter_7lines[] = {
    "      ",
    "▞▀▖   ",
    " ▗▘   ",
    "▗▘ ▌  ",
    "▀▀▀▛▀ ",
    "   ▘  ",
    "      "
};

const char *saturno_7lines[] = {
    "     ",
    " ▌   ",
    "▀▛▘  ",
    " ▀▀▖ ",
    "  ▗▘ ",
    "  ▘  ",
    "     "   
};

const char *marte_7lines[] = {
    "    ",
    "▗▚  ",
    "▘▌▘ ",
    "▞▀▖ ",
    "▌ ▌ ",
    "▝▀  ",
    "    "
};

const char *cauda_draconis_7lines[] = {
    "      ",    
    "▞▚ ▞▚ ",
    " ▞ ▝▖ ",
    "▐   ▐ ",
    " ▚▄▄▘ ",
    "      ",
    "      "
};

const char *caput_draconis_7lines[] = {
    "      ",
    " ▞▀▀▖ ",
    "▝▖  ▞ ",
    " ▐ ▞  ",
    "▚▞ ▚▞ ",
    "      ",
    "      "
};


const char *fortuna_7lines[] = {
    "       ",
    " ▞▛▀▛▖ ",
    "▐ ▝▞ ▐ ",
    "▝▖▞▝▖▞ ",
    " ▝▀▀▀  ",
    "       ",
    "       "
};


const char *sagittarius_[] = {
    "▗▚  ",
    "▘▌▘ ",
    "▄▙▖ ",
    " ▌  ",
    "    "
};

const char *taurus_[] = {
    "▝▖  ▞ ",
    " ▞▀▀▖ ",
    "▐   ▐ ",
    " ▚▄▄▘ ",
    "      "
};


const char *aries_[] = {
    "       ",
    "▞▀▖▞▀▖ ",
    "  ▐    ",
    "  ▐    ",
    "       "
};

const char *gemini_[] = {
    "▄▄▄▄▖ ",
    " ▌ ▌  ",
    " ▌ ▌  ",
    "▄▙▄▙▖ ",
    "      "
};

const char *leo_[] = {
    " ▞▀▀▖ ",
    "▞▚  ▞ ",
    "▚▞ ▐  ",
    "    ▚ ",
    "      "
};


const char *pisces_[] = {
    "▝▖  ▞ ",
    "▗▟▄▟▄ ",
    " ▞ ▝▖ ",
    "▝   ▝ ",
    "      "
};


const char *aquarius_[] = {
    "      ",
    "▞▚▞▚▞ ",
    "      ",
    "▞▚▞▚▞ ",
    "      "
};

const char *cancer_[] = {
    "      ",
    "▞▜▀▀  ",
    "▚▞ ▞▚ ",
    " ▄▄▙▞ ",
    "      "
};

const char *libra_[] = {
    "     ",
    " ▞▚  ",
    "▀▀▀▀ ",
    "▀▀▀▀ ",
    "     "
};

const char *scorpio_[] = {
    "     ",
    "▛▚▀▖ ",
    "▌▐ ▌ ",
    "▘▝ ▚ ",
    "     "
};

const char *virgo_[] = {
    "      ",
    "▛▚▀▞▚ ",
    "▌▐ ▌▞ ",
    "▘▝ ▜  ",
    "      "
};

const char *capricorn_[] = {
    "      ",
    "▚▗▜▞▚ ",
    " ▌ ▚▞ ",
    "   ▞  ",
    "      "
};

     
     
     

const char *LOGO[] = {
    "              ▖  ▖        ▖ ▖▄▖▖▖TM           ",
    "              ▛▖▞▌▛▌▛▘▛▘█▌▛▖▌▌▌▚▘             ",
    "              ▌▝ ▌▙▌▌ ▙▖▙▖▌▝▌▙▌▌▌             ",
    "                                              ",
    " ▄▄▄        ██████ ▄▄▄█████▓ ██▀███   ▒█████  ",
    "▒████▄    ▒██    ▒ ▓  ██▒ ▓▒▓██ ▒ ██▒▒██▒  ██▒",
    "▒██  ▀█▄  ░ ▓██▄   ▒ ▓██░ ▒░▓██ ░▄█ ▒▒██░  ██▒",
    "░██▄▄▄▄██   ▒   ██▒░ ▓██▓ ░ ▒██▀▀█▄  ▒██   ██░",
    " ▓█   ▓██▒▒██████▒▒  ▒██▒ ░ ░██▓ ▒██▒░ ████▓▒░",
    " ▒▒   ▓▒█░▒ ▒▓▒ ▒ ░  ▒ ░░   ░ ▒▓ ░▒▓░░ ▒░▒░▒░ ",
    "  ▒   ▒▒ ░░ ░▒  ░ ░    ░      ░▒ ░ ▒░  ░ ▒ ▒░ ",
    "  ░   ▒   ░  ░  ░    ░        ░░   ░ ░ ░ ░ ▒  ",
    "      ░  ░      ░              ░         ░ ░  "
};



void set_data() {
    char chart_name[100];

    DataNascimento dt;
    DateEdition ed = selecionar_data();

    if (!ed.changed) {
        return;
    }

    dt = ed.date;

    YY = dt.ano;
    MM = dt.mes;
    DD = dt.dia;

    Hora hr;
    HoraEdition ed_h = selecionar_hora();

    if (!ed_h.changed) {
        return;
    }

    hr = ed_h.hora;

    HH = hr.hora;
    MIN = hr.min;
    SEC = hr.sec;

    int gender_id = select_gender();
    if (!gender_id) {
        return;
    }

    GENDER = gender_id;

    double lat = 0.0, lon = 0.0, elev = 0.0;
                                  
    if (load_city_coordinates(CITY, COUNTRY, STATE, TZ_IANA, &TZ_OFFSET, &lat, &lon, &elev)) {    
        set_tz();
        set_dst();
        set_chart_name(chart_name, sizeof(chart_name));                                
        snprintf(CHART_NAME, sizeof(CHART_NAME), "%s", chart_name);               
    
        call_chart();
        
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Chart data have been changed."));
    }
    else {
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Data have not been changed."));
    }
}


void save_chart() {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in save_chart\n");
        return;
    }

    set_chart_name(CHART_NAME, sizeof(CHART_NAME));                                


    int confirm = show_confirm_yesno(CHART_NAME, _("Are you sure you want to SAVE the chart"));

    if (!confirm) {
        snprintf(MESSAGE, sizeof(MESSAGE), _("Chart not saved!"));
        return;
    }


    const char *sql_load = "SELECT year, month, day, hour, min, sec, dst, tz_offset, country, state, city, city_id, dst_offset FROM charts WHERE chart_name = ? COLLATE GLOBAL_SEM_ACENTO;";
    rc = sqlite3_prepare_v2(db, sql_load, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare load statement: %s\n", sqlite3_errmsg(db));
        
        sqlite3_close(db);
        return;
    }
    
    sqlite3_bind_text(stmt, 1, CHART_NAME, -1, SQLITE_STATIC);
    
    int found = 0;
    int city_id = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        city_id = sqlite3_column_int(stmt, 11);
        found = 1;
    }
    DST_OFFSET = obter_segundos_dst_na_data(TZ_IANA, YY, MM, DD, HH, MIN) / 3600;

    const char *sql_select;
    if (found) {
        sql_select = "UPDATE charts SET chart_name = ?, year = ?, month = ?, day = ?, hour = ?, min = ?, sec = ?, dst = ?, tz_offset = ?, country = ?, state = ?, city = ?, dst_offset = ?, gender = ?, city_id = ? WHERE chart_name = ? COLLATE GLOBAL_SEM_ACENTO;";

        rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);

        if (rc != SQLITE_OK) {
            //fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
            show_alert_popup(_("Chart not saved!"), sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            exit(1);
        }

        sqlite3_bind_text(stmt, 1, CHART_NAME, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, YY);
        sqlite3_bind_int(stmt, 3, MM);
        sqlite3_bind_int(stmt, 4, DD);
        sqlite3_bind_int(stmt, 5, HH);
        sqlite3_bind_int(stmt, 6, MIN);
        sqlite3_bind_double(stmt, 7, SEC);
        sqlite3_bind_int(stmt, 8, DST);
        sqlite3_bind_double(stmt, 9, TZ_OFFSET);
        sqlite3_bind_text(stmt, 10, COUNTRY, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 11, STATE, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 12, CITY, -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 13, DST_OFFSET);
        sqlite3_bind_int(stmt, 14, GENDER);
        sqlite3_bind_int(stmt, 15, city_id);
        sqlite3_bind_text(stmt, 16, CHART_NAME, -1, SQLITE_STATIC);
    }
    else {
        const char *sql_load2 = "SELECT id FROM cities WHERE country = ? AND state = ? AND city = ?;";
        rc = sqlite3_prepare_v2(db, sql_load2, -1, &stmt, NULL);
        
        if (rc != SQLITE_OK) {
            //fprintf(stderr, "Failed to prepare load statement: %s\n", sqlite3_errmsg(db));
            show_alert_popup(_("Chart not saved!"), sqlite3_errmsg(db));
            sqlite3_close(db);
            return;
        }
                
        sqlite3_bind_text(stmt, 1, COUNTRY, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, STATE, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, CITY, -1, SQLITE_STATIC);
        city_id = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            city_id = sqlite3_column_int(stmt, 0);
        
            sql_select = "INSERT INTO charts (chart_name, year, month, day, hour, min, sec, dst, tz_offset, country, state, city, dst_offset, gender, city_id) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? ,? ,?, ?);";
            rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);

            if (rc != SQLITE_OK) {
                show_alert_popup(_("Chart not saved!"), sqlite3_errmsg(db));
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                return;
            }

            sqlite3_bind_text(stmt, 1, CHART_NAME, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, YY);
            sqlite3_bind_int(stmt, 3, MM);
            sqlite3_bind_int(stmt, 4, DD);
            sqlite3_bind_int(stmt, 5, HH);
            sqlite3_bind_int(stmt, 6, MIN);
            sqlite3_bind_double(stmt, 7, SEC);
            sqlite3_bind_int(stmt, 8, DST);
            sqlite3_bind_double(stmt, 9, TZ_OFFSET);
            sqlite3_bind_text(stmt, 10, COUNTRY, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 11, STATE, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 12, CITY, -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 13, DST_OFFSET);
            sqlite3_bind_int(stmt, 14, GENDER);
            
            sqlite3_bind_int(stmt, 15, city_id);
        }
        else {
            show_alert_popup(_("Chart not saved!"), "");
        }
    }


    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        //fprintf(stderr, "Failed saving chart: %s\n", sqlite3_errmsg(db));
        show_alert_popup(_("Failed saving chart!"), sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return;
    }

    snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Chart saved!"));

    // 6. Clean up resources
    sqlite3_finalize(stmt);
    close_database(db);

}


void del_chart() {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;
    
    // Open database
    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in del_chart\n");
        return;
    }
    
    // Get list of chart names
    const char *sql_select = "SELECT chart_name FROM charts ORDER BY chart_name COLLATE GLOBAL_SEM_ACENTO;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }
    
    // First pass: count rows
    int row_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        row_count++;
    }
    
    // Reset statement
    sqlite3_reset(stmt);
    
    // If no charts found, return
    if (row_count == 0) {
        sqlite3_finalize(stmt);
        close_database(db);
        // Print message that no charts exist
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("No charts available."));
        return;
    }
    
    // Allocate array for chart names
    char **chart_names = malloc(row_count * sizeof(char*));
    if (!chart_names) {
        sqlite3_finalize(stmt);
        close_database(db);
        return;
    }
    
    // Second pass: fill array with chart names
    int index = 0;
    sqlite3_reset(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *chart_name = (const char*)sqlite3_column_text(stmt, 0);
        chart_names[index] = strdup(chart_name);
        index++;
    }
    
    sqlite3_finalize(stmt);
    
    // Get terminal dimensions
    int term_w, term_h;
    getmaxyx(stdscr, term_h, term_w);
    
    // Calculate window size for chart menu
    int menu_width = 70;
    int menu_height = 20;
    int menu_start_x = (term_w - menu_width) / 2;
    int menu_start_y = (term_h - menu_height) / 2;
    
    // Create windows
    WINDOW *chart_win = newwin(menu_height, menu_width, menu_start_y, menu_start_x);
    WINDOW *chart_shadow = newwin(menu_height, menu_width, menu_start_y + 1, menu_start_x + 1);
    
    // Chart selection variables
    int selected_chart_index = 0;  // This tracks the actual index in the array
    int chart_scroll_offset = 0;   // This tracks which item is at the top of the visible list
    int max_display_items = menu_height - 2;  // Adjust for title and borders
    
    // Initialize ncurses for this menu
    nodelay(chart_win, FALSE);
    keypad(chart_win, TRUE);
    curs_set(0);
    
    int chart_selected = 0;
    int key;
    
    // Clear and redraw shadow
    werase(chart_shadow);
    wattron(chart_shadow, COLOR_PAIR(4));
    box(chart_shadow, 0, 0);
    wattroff(chart_shadow, COLOR_PAIR(4));
    wrefresh(chart_shadow);

    while (!chart_selected) {
               
        // Clear and redraw main menu
        werase(chart_win);
        wattron(chart_win, COLOR_PAIR(2) | A_DIM);
        box(chart_win, 0, 0);
        wbkgd(chart_win, COLOR_PAIR(2));
        
        const char *title = _("Select Chart to Delete");

        mvwprintw(chart_win, 0, (menu_width - get_visual_width(title)) / 2, title);
        wattroff(chart_win, COLOR_PAIR(2) | A_DIM);
        wrefresh(chart_win);
        
        // Draw chart items with proper scrolling
        int display_count = (row_count < max_display_items) ? row_count : max_display_items;
        for (int i = 0; i < display_count; i++) {
            int item_index = i + chart_scroll_offset;
            if (item_index < row_count) {
                int attr = (item_index == selected_chart_index) ? (COLOR_PAIR(3) | A_REVERSE | A_BOLD) : COLOR_PAIR(2);
                wattron(chart_win, attr);
                mvwprintw(chart_win, i + 1, 1, " %s%*s ", chart_names[item_index], menu_width - 4 - get_visual_width(chart_names[item_index]), "");
                wattroff(chart_win, attr);
            }
        }
        wrefresh(chart_win);
        
        key = wgetch(chart_win);
        
        // Handle letter jumping
        if (isalpha(key)) {
            int new_index = find_first_item_with_letter((const char**)chart_names, row_count, selected_chart_index, key);
            if (new_index != selected_chart_index) {
                selected_chart_index = new_index;
                // Adjust scroll offset to keep selected item visible
                if (selected_chart_index < chart_scroll_offset) {
                    chart_scroll_offset = selected_chart_index;
                } else if (selected_chart_index >= chart_scroll_offset + max_display_items) {
                    chart_scroll_offset = selected_chart_index - max_display_items + 1;
                }
                continue; // Skip the normal key processing
            }
        }
        
        switch(key) {
            case KEY_UP:
                if (selected_chart_index > 0) {
                    selected_chart_index--;
                    // Adjust scroll offset if needed
                    if (selected_chart_index < chart_scroll_offset) {
                        chart_scroll_offset = selected_chart_index;
                    }
                }
                break;
            case KEY_DOWN:
                if (selected_chart_index < row_count - 1) {
                    selected_chart_index++;
                    // Adjust scroll offset if needed
                    if (selected_chart_index >= chart_scroll_offset + max_display_items) {
                        chart_scroll_offset = selected_chart_index - max_display_items + 1;
                    }
                }
                break;
            case 10: // Enter
                // INTERCEPTAÇÃO: Mostra o popup antes de fechar o menu
                if (show_confirm_delete_popup(chart_names[selected_chart_index])) {
                    chart_selected = 1; // Se confirmou, sai do loop e vai deletar
                } else {
                    // Se cancelou, redesenha o menu principal e continua nele
                    touchwin(chart_shadow);
                    touchwin(chart_win);
                    wrefresh(chart_shadow);
                    wrefresh(chart_win);
                }
                break;

            case 27: // ESC
                // Cleanup and return
                for (int i = 0; i < row_count; i++) {
                    free(chart_names[i]);
                }
                free(chart_names);
                delwin(chart_win);
                delwin(chart_shadow);
                close_database(db);
                return;
        }
    }
    
    // Get the selected chart name
    const char *selected_chart_name = chart_names[selected_chart_index];

    // Now load the full chart data
    const char *sql_load = "DELETE FROM charts WHERE chart_name = ? COLLATE GLOBAL_SEM_ACENTO;";
    rc = sqlite3_prepare_v2(db, sql_load, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare load statement: %s\n", sqlite3_errmsg(db));
        // Cleanup
        for (int i = 0; i < row_count; i++) {
            free(chart_names[i]);
        }
        free(chart_names);
        delwin(chart_win);
        delwin(chart_shadow);
        sqlite3_finalize(stmt);
        close_database(db);
        return;
    }
    
    sqlite3_bind_text(stmt, 1, selected_chart_name, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed deleting chart: %s\n", sqlite3_errmsg(db));
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Error deleting chart."));
        sqlite3_finalize(stmt);
        return;
    }
    snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Chart deleted successfully."));
    
    sqlite3_finalize(stmt);
    close_database(db);
    
    // Cleanup
    for (int i = 0; i < row_count; i++) {
        free(chart_names[i]);
    }
    free(chart_names);
    delwin(chart_win);
    delwin(chart_shadow);
    
}






void load_chart() {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;
    
    // Open database
    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in load_chart\n");
        return;
    }
    
    // Get list of chart names
    const char *sql_select = "SELECT chart_name FROM charts ORDER BY chart_name COLLATE GLOBAL_SEM_ACENTO;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }
    
    // First pass: count rows
    int row_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        row_count++;
    }
    
    // Reset statement
    sqlite3_reset(stmt);
    
    // If no charts found, return
    if (row_count == 0) {
        sqlite3_finalize(stmt);
        close_database(db);
        // Print message that no charts exist
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("No charts available."));
        return;
    }
    
    // Allocate array for chart names
    char **chart_names = malloc(row_count * sizeof(char*));
    if (!chart_names) {
        sqlite3_finalize(stmt);
        close_database(db);
        return;
    }
    
    // Second pass: fill array with chart names
    int index = 0;
    sqlite3_reset(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *chart_name = (const char*)sqlite3_column_text(stmt, 0);
        chart_names[index] = strdup(chart_name);
        index++;
    }
    
    sqlite3_finalize(stmt);
    
    // Get terminal dimensions
    unsigned short term_w = get_terminal_width();
    unsigned short term_h = get_terminal_height();
    
    // Calculate window size for chart menu
    int menu_width = 70;
    int menu_height = 20;
    int menu_start_x = (term_w - menu_width) / 2;
    int menu_start_y = (term_h - menu_height) / 2;
    
    // Create windows
    WINDOW *chart_win = newwin(menu_height, menu_width, menu_start_y, menu_start_x);
    WINDOW *chart_shadow = newwin(menu_height, menu_width, menu_start_y + 1, menu_start_x + 1);
    
    // Chart selection variables
    int selected_chart_index = 0;  // This tracks the actual index in the array
    int chart_scroll_offset = 0;   // This tracks which item is at the top of the visible list
    int max_display_items = menu_height - 2;  // Adjust for title and borders
    
    // Initialize ncurses for this menu
    nodelay(chart_win, FALSE);
    keypad(chart_win, TRUE);
    curs_set(0);
    
    int chart_selected = 0;
    int key;
    
    // Clear and redraw shadow
    werase(chart_shadow);
    wattron(chart_shadow, COLOR_PAIR(4));
    box(chart_shadow, 0, 0);
    wattroff(chart_shadow, COLOR_PAIR(4));
    wrefresh(chart_shadow);

    while (!chart_selected) {
                
        // Clear and redraw main menu
        werase(chart_win);
        wattron(chart_win, COLOR_PAIR(2) | A_DIM);
        box(chart_win, 0, 0);
        wbkgd(chart_win, COLOR_PAIR(2));
        wattroff(chart_win, COLOR_PAIR(2) | A_DIM);
        wrefresh(chart_win);

        wattron(chart_win, A_BOLD);
        const char *title = _("Select Chart to Load");
        
        mvwprintw(chart_win, 0, (menu_width - get_visual_width(title)) / 2, title);
        wattroff(chart_win, A_BOLD);

        
        // Draw chart items with proper scrolling
        int display_count = (row_count < max_display_items) ? row_count : max_display_items;
        for (int i = 0; i < display_count; i++) {
            int item_index = i + chart_scroll_offset;
            if (item_index < row_count) {
                int attr = (item_index == selected_chart_index) ? (COLOR_PAIR(3) | A_REVERSE | A_BOLD) : COLOR_PAIR(2);
                wattron(chart_win, attr);
                mvwprintw(chart_win, i + 1, 1, " %s%*s ", chart_names[item_index], menu_width - 4 - get_visual_width(chart_names[item_index]), "");
                wattroff(chart_win, attr);
            }
        }
        wrefresh(chart_win);
        
        key = wgetch(chart_win);
        
        // Handle letter jumping
        if (isalpha(key)) {
            int new_index = find_first_item_with_letter((const char**)chart_names, row_count, selected_chart_index, key);
            if (new_index != selected_chart_index) {
                selected_chart_index = new_index;
                // Adjust scroll offset to keep selected item visible
                if (selected_chart_index < chart_scroll_offset) {
                    chart_scroll_offset = selected_chart_index;
                } else if (selected_chart_index >= chart_scroll_offset + max_display_items) {
                    chart_scroll_offset = selected_chart_index - max_display_items + 1;
                }
                continue; // Skip the normal key processing
            }
        }
        
        switch(key) {
            case KEY_UP:
                if (selected_chart_index > 0) {
                    selected_chart_index--;
                    // Adjust scroll offset if needed
                    if (selected_chart_index < chart_scroll_offset) {
                        chart_scroll_offset = selected_chart_index;
                    }
                }
                break;
            case KEY_DOWN:
                if (selected_chart_index < row_count - 1) {
                    selected_chart_index++;
                    // Adjust scroll offset if needed
                    if (selected_chart_index >= chart_scroll_offset + max_display_items) {
                        chart_scroll_offset = selected_chart_index - max_display_items + 1;
                    }
                }
                break;
            case 10: // Enter
                chart_selected = 1;
                break;
            case 27: // ESC
                // Cleanup and return
                for (int i = 0; i < row_count; i++) {
                    free(chart_names[i]);
                }
                free(chart_names);
                delwin(chart_win);
                delwin(chart_shadow);
                close_database(db);
                return;
        }
    }
    
    // Get the selected chart name
    const char *selected_chart_name = chart_names[selected_chart_index];
    
    // Now load the full chart data
    const char *sql_load = "SELECT c.year, c.month, c.day, c.hour, c.min, c.sec, c.dst, i.gmt_offset_secs, c.country, c.state, c.city, c.dst_offset, i.timezone, c.gender FROM charts c INNER JOIN cities i ON c.country = i.country AND c.state = i.state AND c.city = i.city WHERE c.chart_name = ? COLLATE GLOBAL_SEM_ACENTO;";
    rc = sqlite3_prepare_v2(db, sql_load, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare load statement: %s\n", sqlite3_errmsg(db));
        // Cleanup
        for (int i = 0; i < row_count; i++) {
            free(chart_names[i]);
        }
        free(chart_names);
        delwin(chart_win);
        delwin(chart_shadow);
        sqlite3_finalize(stmt);
        close_database(db);
        return;
    }
    
    sqlite3_bind_text(stmt, 1, selected_chart_name, -1, SQLITE_STATIC);
    
    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        // Load data into global variables
        YY = sqlite3_column_int(stmt, 0);
        MM = sqlite3_column_int(stmt, 1);
        DD = sqlite3_column_int(stmt, 2);
        HH = sqlite3_column_int(stmt, 3);
        MIN = sqlite3_column_int(stmt, 4);
        SEC = sqlite3_column_double(stmt, 5);
        DST = sqlite3_column_int(stmt, 6);
        TZ_OFFSET = sqlite3_column_double(stmt, 7) / 3600;
        
        const char *country = (const char*)sqlite3_column_text(stmt, 8);
        const char *state = (const char*)sqlite3_column_text(stmt, 9);
        const char *city = (const char*)sqlite3_column_text(stmt, 10);

        DST_OFFSET = sqlite3_column_double(stmt, 11);
        const char *timezone = (const char*)sqlite3_column_text(stmt, 12);

        GENDER = sqlite3_column_int(stmt, 13);
        
        if (country) {
            snprintf(COUNTRY, sizeof(COUNTRY), "%s", country);
        }
        if (state) {
            snprintf(STATE, sizeof(STATE), "%s", state);
        }
        if (city) {
            snprintf(CITY, sizeof(CITY), "%s", city);
        }
        if (timezone) {
            snprintf(TZ_IANA, sizeof(TZ_IANA), "%s", timezone);
        }
        
        // Set chart name
        snprintf(CHART_NAME, sizeof(CHART_NAME), "%s", selected_chart_name);
        
        found = 1;
    }
    
    sqlite3_finalize(stmt);
    close_database(db);
    
    // Cleanup
    for (int i = 0; i < row_count; i++) {
        free(chart_names[i]);
    }
    free(chart_names);
    delwin(chart_win);
    delwin(chart_shadow);
    
    if (!found) {
        fprintf(stderr, "Warning: Chart '%s' not found in database.\n", selected_chart_name);
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Chart not found!"));
    }
    else {
        //get_iana_timezone(CITY, STATE, COUNTRY, &TZ_IANA);

        if (DST != 0) {
            DST_OFFSET = obter_segundos_dst_na_data(TZ_IANA, YY, MM, DD, HH, MIN) / 3600;
        }
        else {
            DST_OFFSET = 0.0;
        }
        call_chart();

        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Loaded chart data."));
    }
}


int menu(MenuOption *options, int n_choices, int *highlight) {
    WINDOW *menu_win, *shadow_win, *shadow_bar, *bar_win;
    
    int choice = -1;
    int c;

    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    init_pair(3, COLOR_RED, COLOR_WHITE);
    init_pair(4, COLOR_BLACK, COLOR_BLACK);
    init_pair(5, COLOR_CYAN, COLOR_BLUE);
    init_pair(6, COLOR_YELLOW, COLOR_BLACK);
    init_pair(7, COLOR_YELLOW, COLOR_BLUE);
    init_pair(8, COLOR_MAGENTA, COLOR_BLUE);
    init_pair(9, COLOR_BLUE, COLOR_BLUE);
    init_pair(10, COLOR_BLACK, COLOR_CYAN);
    init_pair(11, COLOR_CYAN, COLOR_BLUE);
    init_pair(12, COLOR_WHITE, COLOR_BLUE);
    init_pair(13, COLOR_GREEN, COLOR_BLUE);
    init_pair(14, COLOR_RED, COLOR_BLUE);

    init_pair(22, COLOR_BLACK, COLOR_WHITE);
    init_pair(24, COLOR_BLACK, COLOR_BLACK);
    init_pair(23, COLOR_RED, COLOR_WHITE);
    init_pair(24, COLOR_BLACK, COLOR_BLACK);
    init_pair(25, COLOR_YELLOW, COLOR_BLACK);
    init_pair(26, COLOR_BLACK, COLOR_CYAN);
    init_pair(27, COLOR_RED, COLOR_CYAN);
    init_pair(28, COLOR_MAGENTA, COLOR_WHITE);
    init_pair(29, COLOR_WHITE, COLOR_WHITE);
    init_pair(30, COLOR_MAGENTA, COLOR_CYAN);
    
    init_pair(31, COLOR_BLUE, COLOR_YELLOW);
    init_pair(32, COLOR_BLUE, COLOR_GREEN);
    init_pair(33, COLOR_BLUE, COLOR_CYAN);

    
    cchar_t ls, rs, ts, bs, tl, tr, bl, br;

    // Set standard straight lines for borders
    setcchar(&ls, L"│", 0, 0, NULL); // Left side
    setcchar(&rs, L"│", 0, 0, NULL); // Right side
    setcchar(&ts, L"─", 0, 0, NULL); // Top side
    setcchar(&bs, L"─", 0, 0, NULL); // Bottom side

    // Set rounded corner glyphs (Unicode Box Drawing characters)
    setcchar(&tl, L"╭", 0, 0, NULL); // Top Left Corner
    setcchar(&tr, L"╮", 0, 0, NULL); // Top Right Corner
    setcchar(&bl, L"╰", 0, 0, NULL); // Bottom Left Corner
    setcchar(&br, L"╯", 0, 0, NULL); // Bottom Right Corner

    while(1) {
        bkgd(COLOR_PAIR(9) | A_DIM | A_REVERSE);
        clear();

        
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        int center_y = max_y / 2;
        int center_x = max_x / 2;


        // 0. Draw Circles

        // if (DARK_MODE) {
        //     attron(COLOR_PAIR(7) | A_DIM);
        // }
        // else {
        //     attron(COLOR_PAIR(31));
        // }
        // draw_circle_filled(center_y, center_x, 13, 2.0, 1.0, L"█▓▒░");
        // //draw_circle_points(center_y, center_x, 20, 1.75, 1.0, L"█▓▒░");
        
        // if (DARK_MODE) {
        //     attroff(COLOR_PAIR(7) | A_DIM);
        // }
        // else {
        //     attroff(COLOR_PAIR(31));
        // }

        unsigned short term_w = get_terminal_width();
        unsigned short term_h = get_terminal_height();
        
        // 1. Draw Logo
        
        for (int i = 0; i < (int)ARRAY_SIZE(LOGO); i++) {
            draw_centered_text(stdscr, 1 + i, 0, term_w, LOGO[i], COLOR_PAIR(32));
        }
        
        
        
        // 5. Decorations
        wattron(stdscr, COLOR_PAIR(33) | A_DIM);
        for (int i = 0; i < 6; i++) {
            mvwprintw(stdscr, 1+i, 2, jupiter_7lines[i]);
        }
        
        for (int i = 0; i < 6; i++) {
            mvwprintw(stdscr, 3+i, 8, saturno_7lines[i]);
        }
        
        for (int i = 0; i < 6; i++) {
            mvwprintw(stdscr, 2+i, 12, sol_7lines[i]);
        }

        for (int i = 0; i < 6; i++) {
            mvwprintw(stdscr, 1+i, term_w - 5, mercury_7lines[i]);
        }
        
        for (int i = 0; i < 6; i++) {
            mvwprintw(stdscr, 3+i, term_w - 10, venus_7lines[i]);
        }

        for (int i = 0; i < 6; i++) {
            mvwprintw(stdscr, 1+i, term_w - 15, marte_7lines[i]);
        }
        wattroff(stdscr, COLOR_PAIR(33) | A_DIM);

 
        if (DARK_MODE) {
            attron(COLOR_PAIR(7) | A_DIM);
        }
        else {
            attron(COLOR_PAIR(31));
        }    
        //draw_circle_points(center_y, center_x, 22, 2.0, 1.0, L"█▓▒░");
        draw_circle_points(center_y, center_x, 24, 2.0, 1.0, L"░▒▓█▓▒░");
        if (DARK_MODE) {
            attroff(COLOR_PAIR(7));
            attron(COLOR_PAIR(9) | A_DIM);
        }
        else {
            attroff(COLOR_PAIR(31));
            attron(COLOR_PAIR(9) | A_DIM);
        }
        draw_circle_points(center_y, center_x, 26, 2.0, 1.0, L"░▒▓█▓▒░");
        if (DARK_MODE) {
            attroff(COLOR_PAIR(7) | COLOR_PAIR(9) | A_DIM);
        }
        else {
            attroff(COLOR_PAIR(31) | COLOR_PAIR(9) | A_BOLD | A_DIM);
        }
        
        // Version
        char version_str[30];
        snprintf(version_str, 30, "v%s", VERSION);
        attron(COLOR_PAIR(33) | A_BOLD);
        mvwprintw(stdscr, term_h - 6, term_w - strlen(version_str) - 3, version_str);
        attroff(COLOR_PAIR(33) | A_BOLD);
        
        refresh();

        noecho();
        cbreak();
        
        // 2. Calculate Menu Box Dimensions
        int max_choice_width = 0;
        for (int j = 0; j < n_choices; j++) {
            int w = get_visual_width(options[j].option);
            if (w > max_choice_width) max_choice_width = w;
        }
        
        int box_width = max_choice_width + 4;
        if (box_width % 2 != 0) box_width++;
        
        int box_padding_x = (term_w - box_width) / 2;
        int logo_lines = ARRAY_SIZE(LOGO);
        int menu_y_start = logo_lines + 2;

        menu_win = newwin(n_choices + 2, box_width, menu_y_start, box_padding_x);
        shadow_win = newwin(n_choices + 2, box_width, menu_y_start + 1, box_padding_x + 1);
    
        keypad(menu_win, TRUE);
        curs_set(0);


        // 5. Apply the rounded border to the window
        wborder_set(menu_win, &ls, &rs, &ts, &bs, &tl, &tr, &bl, &br);

        // Draw Shadow
        wattron(shadow_win, COLOR_PAIR(4));
        box(shadow_win, 0, 0);
        wattroff(shadow_win, COLOR_PAIR(4));
        wrefresh(shadow_win);
    
        // Draw Menu Border
        wattron(menu_win, COLOR_PAIR(10) | A_DIM);
        //box(menu_win, 0, 0);
        wbkgd(menu_win, COLOR_PAIR(10));
        wattroff(menu_win, COLOR_PAIR(10) | A_DIM);
        
        // 3. Draw Menu Options using the new function
        for(int i = 0; i < n_choices; i++) {
            int attr = (i == *highlight) ? (COLOR_PAIR(3) | A_REVERSE | A_BOLD) : COLOR_PAIR(10);
            
            // The usable width inside the box is box_width - 2 (to account for borders)
            draw_centered_text(menu_win, i + 1, 1, box_width - 2, options[i].option, attr);
        }
        wrefresh(menu_win);
        
        // 4. Draw Description Text using the new function
        //int desc_y = menu_y_start + (n_choices + 2) + 2;
        //draw_centered_text(stdscr, desc_y, 0, term_w, options[*highlight].text, COLOR_PAIR(7) | A_BOLD);
        
                        
        // 6. Draw the Status Bar
        bar_win = newwin(3, term_w - 3, term_h - 5, 1);
        shadow_bar = newwin(3, term_w - 3, term_h - 4, 2);
        
        

        wborder_set(bar_win, &ls, &rs, &ts, &bs, &tl, &tr, &bl, &br);

        wattron(shadow_bar, COLOR_PAIR(4));
        //box(shadow_bar, 0 , 0);
        wattroff(shadow_bar, COLOR_PAIR(4));
        wrefresh(shadow_bar);
        
        wattron(bar_win, COLOR_PAIR(11));
        //box(bar_win, 0, 0);
        wbkgd(bar_win, COLOR_PAIR(11));
        wattroff(bar_win, COLOR_PAIR(11));
        //wrefresh(bar_win);
        
        char str_bar[term_w];
        snprintf(str_bar, sizeof(str_bar), "%s", CITY);
        
        // Debug: print the raw string to see what it contains
        // printf("DEBUG: DEFAULT_CITY = '%s'\n", DEFAULT_CITY);
        // printf("DEBUG: str_bar = '%s'\n", str_bar);
        
        // Use wcwidth to properly calculate width for wide characters
        int city_width = get_visual_width(str_bar);
        // Also ensure we're calculating the width correctly for the status bar area
        int bar_width = term_w - 5;
        
        draw_centered_text(bar_win, 1, 1, bar_width, "", COLOR_PAIR(11) | A_BOLD);
        wattron(bar_win, COLOR_PAIR(11) | A_BOLD);
        mvwprintw(bar_win, 1, 1, _("Location: "));
        wattroff(bar_win, COLOR_PAIR(11) | A_BOLD);

        wattron(bar_win, COLOR_PAIR(6));
        mvwprintw(bar_win, 1, 11, " %s ", str_bar);
        wattroff(bar_win, COLOR_PAIR(6));

        wattron(bar_win, COLOR_PAIR(11) | A_BOLD);
        mvwprintw(bar_win, 1, 11 + city_width + 2, _("┃ Date/Time: "));        
        wattroff(bar_win, COLOR_PAIR(11) | A_BOLD);

        wattron(bar_win, COLOR_PAIR(6));
        mvwprintw(bar_win, 1, 11 + city_width + 2 + 13, " %02d/%02d/%04d %02d:%02d:%02d ", DD, MM, YY, HH, MIN, SEC);
        wattroff(bar_win, COLOR_PAIR(6));

        wattron(bar_win, COLOR_PAIR(11) | A_BOLD);
        mvwprintw(bar_win, 1, 11 + city_width + 2 + 13 + 2 + 18, _("┃ Dst: "));        
        wattroff(bar_win, COLOR_PAIR(5) | A_BOLD);

        wattron(bar_win, COLOR_PAIR(6));
        mvwprintw(bar_win, 1, 11 + city_width + 2 + 13 + 2 + 18 + 7, " %2s ", (DST == 1)?_("Yes "):(DST == 0)?_("No  "):_("Auto"));
        wattroff(bar_win, COLOR_PAIR(6));

        wattron(bar_win, COLOR_PAIR(11) | A_BOLD);
        mvwprintw(bar_win, 1, 11 + city_width + 2 + 13 + 2 + 18 + 7 + 5, _("┃ Gen: "));        
        wattroff(bar_win, COLOR_PAIR(11) | A_BOLD);

        wattron(bar_win, COLOR_PAIR(6));
        mvwprintw(bar_win, 1, 11 + city_width + 2 + 13 + 2 + 18 + 7 + 5 + 7, " %s", (GENDER == 1)?"M":((GENDER == 2)?"F":"N"));
        wattroff(bar_win, COLOR_PAIR(6));

        wattron(bar_win, COLOR_PAIR(11) | A_BOLD);
        mvwprintw(bar_win, 1, 11 + city_width + 2 + 13 + 2 + 18 + 7 + 5 + 7 + 2, "┃");        
        wattroff(bar_win, COLOR_PAIR(11) | A_BOLD);

        wattron(bar_win, COLOR_PAIR(12) | A_BOLD);
        if (strcmp(MESSAGE, "") != 0) {
            wattron(bar_win, A_BLINK);
        }
        
        // === CÁLCULO DA MENSAGEM COM RETICÊNCIAS ===
        int msg_start_x = 11 + city_width + 2 + 13 + 2 + 18 + 7 + 5 + 7 + 2 + 2 + 1;
        int limite_borda_direita = bar_width - 1;
        int espaco_remanescente = limite_borda_direita - msg_start_x;

        if (espaco_remanescente < 0) espaco_remanescente = 0;

        wattron(bar_win, COLOR_PAIR(12) | A_BOLD);
        if (strcmp(MESSAGE, "") != 0) {
            wattron(bar_win, A_BLINK);
        }

        char *msg_para_exibir = (strcmp(MESSAGE, "") == 0) ? options[*highlight].text : MESSAGE;
        char msg_final[term_w];

        // Se a mensagem original for maior do que o espaço que temos na tela
        if (get_visual_width(msg_para_exibir) > espaco_remanescente) {
            // Reserva apenas 1 espaço para a reticência Unicode ("…")
            int espaco_para_texto = espaco_remanescente - 1;
            if (espaco_para_texto < 0) espaco_para_texto = 0;

            // Corta o texto e concatena o caractere único de reticências
            snprintf(msg_final, sizeof(msg_final), "%.*s…", espaco_para_texto, msg_para_exibir);
        } else {
            snprintf(msg_final, sizeof(msg_final), "%s", msg_para_exibir);
        }


        mvwprintw(bar_win, 1, msg_start_x, "%s", msg_final);        
        wattroff(bar_win, COLOR_PAIR(12) | A_BOLD | A_BLINK);

        wrefresh(bar_win);

        snprintf(MESSAGE, sizeof(MESSAGE), "%s", "");
        
        
        c = wgetch(menu_win);
        switch(c) {
            case KEY_UP:
            case KEY_LEFT:
                snprintf(MESSAGE, sizeof(MESSAGE), "%s", "");
                *highlight = (*highlight == 0) ? n_choices - 1 : *highlight - 1;
                break;
            case KEY_DOWN:
            case KEY_RIGHT:
                snprintf(MESSAGE, sizeof(MESSAGE), "%s", "");
                *highlight = (*highlight == n_choices - 1) ? 0 : *highlight + 1;
                break;
            case 10: // Enter
                choice = *highlight;
                if (options[choice].command != NULL) options[choice].command();
                break;
            case 27: // ESC
                choice = n_choices;
                break;
        }
        if(choice != -1) break; 
    }

    delwin(menu_win);
    delwin(bar_win);
    delwin(shadow_win);
    delwin(shadow_bar);
    
    return choice;
}


void action_exit() {
    printf("Exiting program...\n");
    endwin();
    
    terminate_database();

    exit(0);
}

void call_chart_now() {
    struct tm *local_time;
    double lat = 0.0, lon = 0.0, elev = 0.0; // Initialized to default values
    double tz_offset = 0.0;
    char country[100];
    char tz_iana[100];

    time_t raw_time;
    time(&raw_time);

    local_time = localtime(&raw_time);

    YY = local_time->tm_year + 1900;
    MM = local_time->tm_mon + 1;
    DD = local_time->tm_mday;
    HH = local_time->tm_hour;
    MIN = local_time->tm_min;
    SEC = local_time->tm_sec;

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in call_chart_now\n");
        return;
    }
    
    const char *sql_select = "SELECT lat, lon, elev, country, gmt_offset_secs, timezone FROM cities WHERE country = ? AND state = ? COLLATE GLOBAL_SEM_ACENTO AND city = ? COLLATE GLOBAL_SEM_ACENTO;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement in call_chart_now: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    char city[100];
    snprintf(city, sizeof(city), "%s", DEFAULT_CITY);

    sqlite3_bind_text(stmt, 1, DEFAULT_COUNTRY, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, DEFAULT_STATE, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, city, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        lat = sqlite3_column_double(stmt, 0);
        lon = sqlite3_column_double(stmt, 1);
        elev = sqlite3_column_double(stmt, 2);
        snprintf(country, sizeof(country), "%s", sqlite3_column_text(stmt, 3));
        tz_offset = sqlite3_column_double(stmt, 4) / 3600;
        snprintf(tz_iana, sizeof(tz_iana), "%s", sqlite3_column_text(stmt, 5));
        found = 1;
    }
    
    // Clean up statement resources
    sqlite3_finalize(stmt);
    close_database(db);

    if (!found) {
        fprintf(stderr, "Warning: City '%s' not found in database.\n", city);
    }

    double dst_offset = 0.0;
    dst_offset = (double)obter_segundos_dst_na_data(tz_iana, YY, MM, DD, HH, MIN) / 3600;        
    
    tz_offset = tz_offset + dst_offset;

    TZ_OFFSET = tz_offset;
    DST_OFFSET = dst_offset;

    GENDER = get_default_gender();

    snprintf(COUNTRY, sizeof(COUNTRY), "%s", country);
    snprintf(CITY, sizeof(CITY), "%s", city);
    snprintf(STATE, sizeof(STATE), "%s", DEFAULT_STATE);
    snprintf(TZ_IANA, sizeof(TZ_IANA), "%s", tz_iana);

    //PlanetDignities *dig_natal;
    //double cusps[13] = {0};

    chart(local_time, lat, lon, elev, tz_offset, city, country, true, 1, _("Here And Now"), HOUSE_SYSTEM, GENDER, DARK_MODE, false, 0, 0, 0, 0.0, 0.0, NULL, NULL, NULL, -1, -1, NULL, 0.0, NULL, 0.0, NULL);
}



void call_chart() {
    struct tm local_time_struct;
    double lat = 0.0, lon = 0.0, elev = 0.0; // Initialized to default values
    char country[100];

    time_t raw_time;
    time(&raw_time);

    struct tm *temp_tm = localtime(&raw_time);
    local_time_struct = *temp_tm;

    local_time_struct.tm_year = YY - 1900;
    local_time_struct.tm_mon = MM - 1;
    local_time_struct.tm_mday = DD;
    local_time_struct.tm_hour = HH;
    local_time_struct.tm_min = MIN;
    local_time_struct.tm_sec = SEC;
    local_time_struct.tm_isdst = DST;

    mktime(&local_time_struct);

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in call_chart\n");
        return;
    }
    
    const char *sql_select = "SELECT lat, lon, elev, country, gmt_offset_secs, timezone FROM cities WHERE country = ? AND state = ? COLLATE GLOBAL_SEM_ACENTO AND city = ? COLLATE GLOBAL_SEM_ACENTO;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement in call_chart: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    char city[100];
    snprintf(city, sizeof(city), "%s", CITY);

    sqlite3_bind_text(stmt, 1, COUNTRY, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, STATE, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, city, -1, SQLITE_STATIC);

    double tz_offset = TZ_OFFSET;
        
    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        lat = sqlite3_column_double(stmt, 0);
        lon = sqlite3_column_double(stmt, 1);
        elev = sqlite3_column_double(stmt, 2);
        snprintf(country, sizeof(country), "%s", sqlite3_column_text(stmt, 3));
        //tz_offset = sqlite3_column_double(stmt, 4) / 3600;
        const char *timezone = (const char*)sqlite3_column_text(stmt, 5);
                
        snprintf(TZ_IANA, 100, "%s", timezone);
        tz_offset = obter_segundos_gmt_na_data(timezone, YY, MM, DD, HH, MIN) / 3600;
        
        found = 1;

    }
    
    // Clean up statement resources
    sqlite3_finalize(stmt);
    close_database(db);

    // Optional: Only trigger chart if data was actually found
    if (!found) {
        fprintf(stderr, "Warning: City '%s' not found in database.\n", city);
    }

    double dst_offset = 0.0;
    if (DST != 0) {
        dst_offset = (double)obter_segundos_dst_na_data(TZ_IANA, YY, MM, DD, HH, MIN) / 3600;        
    }
    else {
        dst_offset = 0.0;
    }

    tz_offset = tz_offset + dst_offset;

    //PlanetDignities *dig_natal;
    //double cusps[13] = {0};

    chart(&local_time_struct, lat, lon, elev, tz_offset, city, country, false, 0, CHART_NAME, HOUSE_SYSTEM, GENDER, DARK_MODE, false, 0, 0, 0, 0.0, 0.0, NULL, NULL, NULL, -1, -1, NULL, 0.0, NULL, 0.0, NULL);
}


void load_default_values() {
    time_t tempo_atual = time(NULL);
    struct tm *local = localtime(&tempo_atual);

    YY = local->tm_year + 1900;
    MM = local->tm_mon + 1;
    DD = local->tm_mday;
    HH = local->tm_hour;
    MIN = local->tm_min;
    SEC = local->tm_sec;

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in load_default_values\n");
        return;
    }

    const char *sql_select_city = "SELECT c.city, c.country, c.state, c.timezone, p.dst, p.dark_mode, p.house_system, p.triplicity_system, p.gender, p.show_modern_planets, p.language FROM profiles p INNER JOIN cities c ON p.city_id = c.id WHERE p.profile = ?;";
    rc = sqlite3_prepare_v2(db, sql_select_city, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement (main): %s\n", sqlite3_errmsg(db));
        close_database(db);
        return;
    }
    sqlite3_bind_text(stmt, 1, "default", -1, SQLITE_STATIC);
    
    //int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {        
        const char *city = (const char*)sqlite3_column_text(stmt, 0);
        const char *country = (const char*)sqlite3_column_text(stmt, 1);
        const char *state = (const char*)sqlite3_column_text(stmt, 2);
        const char *timezone = (const char*)sqlite3_column_text(stmt, 3);
        int dst = sqlite3_column_int(stmt, 4);
        int dark_mode = sqlite3_column_int(stmt, 5);
        const char *db_house_system = (const char*)sqlite3_column_text(stmt, 6);
        //int triplicity_system = sqlite3_column_int(stmt, 6);
        int gender_id = sqlite3_column_int(stmt, 8);
        int show_mod = sqlite3_column_int(stmt, 9);
        const char *def_lang = (const char*)sqlite3_column_text(stmt, 10);
    
        if (city) snprintf(DEFAULT_CITY, sizeof(DEFAULT_CITY), "%s", city);
        if (country) snprintf(DEFAULT_COUNTRY, sizeof(DEFAULT_COUNTRY), "%s", country);
        if (state) snprintf(DEFAULT_STATE, sizeof(DEFAULT_STATE), "%s", state);
        if (timezone) snprintf(DEFAULT_TZ_IANA, 100, "%s", timezone);
        
        if (city) snprintf(CITY, sizeof(CITY), "%s", city);
        if (country) snprintf(COUNTRY, sizeof(COUNTRY), "%s", country);
        if (state) snprintf(STATE, sizeof(STATE), "%s", state);
        if (timezone) snprintf(TZ_IANA, 100, "%s", timezone);

        if (def_lang) snprintf(LANGUAGE, 10, "%s", def_lang);

        DST = dst;
        DARK_MODE = dark_mode;
        if (db_house_system) {
            char *house_system = strdup(db_house_system);
            HOUSE_SYSTEM = house_system[0];
            free(house_system);
        }

        GENDER = gender_id;

        if (show_mod > 0) {
            show_modern_planets = true;
        }
        else {
            show_modern_planets = false;
        }
            
        //found = 1;
    }
    // if (found && strcmp(TZ_IANA, "") == 0)
    //     get_iana_timezone(CITY, STATE, COUNTRY, &TZ_IANA);
    
    sqlite3_finalize(stmt);
    close_database(db);
}


void show_text_file(const char* filename, const char* title, int from_line) {
    
    char help_path[512];
    snprintf(help_path, sizeof(help_path), "%s/%s", CONFIG_PATH, filename);
    
    char* file_content = load_file_content(help_path);
    
    char** help_lines;
    int line_count;

    // Get terminal dimensions
    int term_w = getmaxx(stdscr);
    int term_h = getmaxy(stdscr);
    
    // Calculate window dimensions
    int win_w = (term_w > 80) ? 70 : term_w - 4;
    int win_h = (term_h > 20) ? 20 : term_h - 4;
    int win_x = (term_w - win_w) / 2;
    int win_y = (term_h - win_h) / 2;
    
    if (file_content) {
        // Successfully loaded file, split into lines
        help_lines = split_lines_wrap(file_content, &line_count, win_w - 1);
        if (!help_lines) {
            // Fallback to hardcoded text if splitting fails
            free(file_content);
            help_lines = NULL;
        }
    }
    
    // If file loading failed or file_content is NULL, use hardcoded text
    if (!help_lines) {
        const char *default_help_text[] = {
            "Welcome to the MorceNOX™ ASTRO!",
            "===============================",
            "",
            "This program allows you to create and view astrological charts.",
            "",
            "Main Features:",
            "- Quick Chart: View chart for current time and location",
            "- New Chart: Configure chart parameters",
            "- Load Chart: Load previously saved charts",
            "- Save Chart: Save current chart configuration",
            "- Delete Chart: Delete a previously saved chart",
            "- Display Chart: Show the full astrological chart",
            "- Change Default Location: Set your default city",
            "",
            "Navigation:",
            "  Arrow keys: Move between menu options",
            "  Enter: Select current item",
            "  ESC: Exit menu",
            "",
            "For more information about astrology or specific features,",
            "please refer to the documentation or online resources.",
            "",
            "Press Q or ESC to return to menu.",
            NULL  // Sentinel to mark end of array
        };
        
        // Make a copy of the array to work with
        line_count = 0;
        while (default_help_text[line_count] != NULL) {
            line_count++;
        }
        
        help_lines = malloc((line_count + 1) * sizeof(char*));
        if (help_lines) {
            for (int i = 0; i < line_count; i++) {
                help_lines[i] = strdup(default_help_text[i]);
            }
            help_lines[line_count] = NULL;
        }
    }
    
    if (!help_lines) {
        return; // Failed to load help content
    }
    
    // Create windows
    WINDOW *help_win = newwin(win_h, win_w, win_y, win_x);
    WINDOW *shadow_win = newwin(win_h, win_w, win_y + 1, win_x + 1);
    
    // Check if windows were created successfully
    if (!help_win || !shadow_win) {
        if (help_win) delwin(help_win);
        if (shadow_win) delwin(shadow_win);
        
        // Clean up allocated memory
        for (int i = 0; help_lines[i] != NULL; i++) {
            free(help_lines[i]);
        }
        free(help_lines);
        if (file_content) free(file_content);
        return;
    }

    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    init_pair(3, COLOR_RED, COLOR_WHITE);
    init_pair(4, COLOR_BLACK, COLOR_BLACK);
    init_pair(5, COLOR_CYAN, COLOR_BLUE);
    init_pair(6, COLOR_YELLOW, COLOR_BLACK);
    
    // Draw shadow
    wbkgd(shadow_win, COLOR_PAIR(4));
    box(shadow_win, 0, 0);
    wrefresh(shadow_win);
    
    // Draw main window with border
    wbkgd(help_win, COLOR_PAIR(1));
    box(help_win, 0, 0);
    mvwprintw(help_win, 0, (win_w - get_visual_width(title)) / 2, title);
    wrefresh(help_win);
    
    // Calculate how many lines we can display
    int max_lines = win_h - 4;  // Leave space for border and title
    int start_line = from_line;
    int key;
    int done = 0;

    nodelay(help_win, FALSE);
    keypad(help_win, TRUE);
    curs_set(0);
    
    while (!done) {
        // Clear the text area
        werase(help_win);
        
        // Redraw borders
        wbkgd(help_win, COLOR_PAIR(2));
        box(help_win, 0, 0);
        mvwprintw(help_win, 0, (win_w - get_visual_width(title)) / 2, title);
        
        // Draw text content
        int display_lines = 0;
        for (int i = start_line; i < line_count && display_lines < max_lines; i++) {
            if (help_lines[i]) {
                mvwprintw(help_win, 2 + display_lines, 1, "%s", help_lines[i]);
                display_lines++;
            }
        }
        
        // Draw scrollbar if needed
        if (line_count > max_lines) {
            int scrollbar_height = max_lines;
            int scrollbar_pos = (start_line * scrollbar_height) / (line_count - max_lines);
            
            for (int i = 0; i < scrollbar_height; i++) {
                if (i >= scrollbar_pos && i < scrollbar_pos + 1) {
                    mvwaddch(help_win, 2 + i, win_w - 2, ACS_BLOCK);
                } else {
                    mvwaddch(help_win, 2 + i, win_w - 2, ' ');
                }
            }
        }
        wrefresh(help_win);
        
        key = wgetch(help_win);
        
        switch (key) {
            case KEY_UP:
                if (start_line > 0) {
                    start_line--;
                }
                break;
            case KEY_DOWN:
                if (start_line < line_count - max_lines) {
                    start_line++;
                }
                break;
            case KEY_PPAGE:
                if (start_line > max_lines) {
                    start_line -= max_lines;
                } else {
                    start_line = 0;
                }
                break;
            case KEY_NPAGE:
                if (start_line < line_count - max_lines) {
                    start_line += max_lines;
                } else {
                    start_line = line_count - max_lines;
                }
                if (start_line < 0) start_line = 0;
                break;
            case 'q':
            case 27: // ESC
                done = 1;
                break;
        }
    }
    
    // Clean up
    for (int i = 0; help_lines[i] != NULL; i++) {
        free(help_lines[i]);
    }
    free(help_lines);
    if (file_content) free(file_content);
    delwin(help_win);
    delwin(shadow_win);
}

void show_help_screen() {
    char path[285];
    char filename[21];

    snprintf(filename, sizeof(filename), "%s_%s.txt", "help", LANGUAGE);
    snprintf(path, sizeof(path), "%s/%s", CONFIG_PATH, filename);

    int line = select_topic(path);

    if (line < 0) {
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Help file could not be loaded!"));
        return;
    }
    else if (line > 0) {
        show_text_file(filename, _("Help"), line);
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Help file loaded successfully!"));
    }
}

void show_topics() {
    char path[287];
    char filename[21];

    snprintf(filename, sizeof(filename), "%s_%s.txt", "topics", LANGUAGE);
    snprintf(path, sizeof(path), "%s/%s", CONFIG_PATH, filename);

    int line = select_topic(path);

    if (line < 0) {
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Topics file could not be loaded!"));
        return;
    }
    else if (line > 0) {
        show_text_file(filename, _("Technical Topics"), line);
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Topics file loaded successfully!"));

    }
}
    

void set_options() {
    OptionsEdition ed = select_options();

    if (!ed.changed) {
        snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Settings unchanged!"));
        return;
    }

    DARK_MODE = ed.options.dark_mode;
    HOUSE_SYSTEM = ed.options.house_system;
    GENDER = ed.options.gender;
    snprintf(LANGUAGE, 10, "%s", ed.options.language);

    update_interface_language();

    if (! update_triplicity_rulers(ed.options.triplicity_system)) {
        fprintf(stderr, "Failed updating triplicity rulers!");
        return;
    }
    else {
        if (! update_settings(ed.options)) {
            fprintf(stderr, "Failed updating settings!");
            return;
        }
    }

    snprintf(MESSAGE, sizeof(MESSAGE), "%s", _("Settings updated!"));
}


void load_env_configuration() {
    const char *home_dir = getenv("HOME");
        
    snprintf(CONFIG_PATH, 256, "%s/%s/%s", home_dir, ".config", APPLICATION_NAME);

    char env_path[512];
    snprintf(env_path, sizeof(env_path), "%s/%s", CONFIG_PATH, ".env");

    load_env_file(env_path);

    DB_PATH = getenv("DB_PATH");
}



int main() {
    setlocale(LC_ALL, "");

    load_env_configuration();
    load_default_values();

    update_interface_language();

   
    
    
    //int EXIT_CODE = n_choices - 1;

    int highlight = 0;
    initscr();

    while(1) {
        MenuOption options[] = { 
            {_("Quick Chart"), _("View the chart for the current moment in the default city."), call_chart_now},
            {_("New Chart"), _("Set the data to load a new chart."), set_data},
            {_("Load Chart"), _("Load a previously saved chart."), load_chart},
            {_("Save the Chart"), _("Save the current chart."), save_chart},
            {_("Delete Chart"), _("Delete a previously saved chart."), del_chart},
            {_("Display Chart"), _("Display the current chart."), call_chart},
            {_("Change the Default Location"), _("Select a new city as default for the Quick Chart."), set_default_city},
            {_("Settings"), _("Modify the default settings."), set_options},
            {_("Help"), _("See the help window for this application."), show_help_screen},
            {_("Technical Topics"), _("Technical topics used in this program."), show_topics},
            {_("Exit"), _("Exit the program."), action_exit}
        };
        int n_choices = ARRAY_SIZE(options);
        int choice = 0;

        choice = menu(options, n_choices, &highlight);
        
        // If the user pressed ESC (choice == n_choices), break the loop
        if (choice >= n_choices) break;

        //endwin();
    }

    endwin();
    //free(TZ_IANA);

    
    // close_database(db);
    terminate_database();

    return 1;
}

