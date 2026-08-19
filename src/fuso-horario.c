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
#include <string.h>
#include <unicode/utypes.h>
#include <unicode/ucal.h>
#include <unicode/ustring.h>
#include <sqlite3.h>

#include "fuso-horario.h"
#include "db-utils.h"


/**
 * Obtém o deslocamento padrão do fuso horário (GMT Offset) em segundos para qualquer data.
 * Ignora variações de Horário de Verão (DST), mas mantém o histórico de mudanças de fuso.
 * 
 * @param tz_iana String do fuso horário (ex: "America/Sao_Paulo", "Asia/Kathmandu")
 * @return O número de segundos do fuso em relação ao GMT. Retorna -9999 em caso de erro.
 */
int obter_segundos_gmt_na_data(const char* tz_iana, int ano, int mes, int dia, int hora, int minuto) {
    UErrorCode status = U_ZERO_ERROR;
    
    int32_t tz_len = strlen(tz_iana);
    UChar tz_u[64]; 
    if (tz_len >= 64) return -9999;
    
    u_charsToUChars(tz_iana, tz_u, tz_len);
    tz_u[tz_len] = 0;

    UCalendar *cal = ucal_open(tz_u, tz_len, "en_US", UCAL_GREGORIAN, &status);
    if (U_FAILURE(status)) {
        return -9999; 
    }

    ucal_setDateTime(cal, ano, mes - 1, dia, hora, minuto, 0, &status);

    // UCAL_ZONE_OFFSET retorna o fuso horário base (Raw Offset) em milissegundos
    int32_t gmt_offset_ms = ucal_get(cal, UCAL_ZONE_OFFSET, &status);

    ucal_close(cal);

    if (U_FAILURE(status)) {
        return -9999;
    }

    // Converte milissegundos diretamente para segundos
    return gmt_offset_ms / 1000;
}



/**
 * Obtém o deslocamento exato do Horário de Verão (DST) em segundos para qualquer data.
 * Gerencia automaticamente fusos fracionados (ex: 30 ou 45 min) e variações históricas.
 * 
 * @param tz_iana String do fuso horário (ex: "America/Sao_Paulo", "Asia/Kathmandu")
 * @return O número de segundos do DST na data (ex: 0, 30, 60, 120). Retorna -9999 em caso de erro.
 */
int obter_segundos_dst_na_data(const char* tz_iana, int ano, int mes, int dia, int hora, int minuto) {
    UErrorCode status = U_ZERO_ERROR;
    
    int32_t tz_len = strlen(tz_iana);
    UChar tz_u[64]; 
    if (tz_len >= 64) return -9999;
    
    u_charsToUChars(tz_iana, tz_u, tz_len);
    tz_u[tz_len] = 0;

    UCalendar *cal = ucal_open(tz_u, tz_len, "en_US", UCAL_GREGORIAN, &status);
    if (U_FAILURE(status)) {
        return -9999; 
    }

    ucal_setDateTime(cal, ano, mes - 1, dia, hora, minuto, 0, &status);

    // UCAL_DST_OFFSET retorna o valor exato do DST em milissegundos para este instante
    int32_t dst_offset_ms = ucal_get(cal, UCAL_DST_OFFSET, &status);

    ucal_close(cal);

    if (U_FAILURE(status)) {
        return -9999;
    }

    // Converte milissegundos diretamente para segundos (garante compatibilidade com frações)
    return dst_offset_ms / (1000);
}


int get_gmt_offset(char *city, char *state, char *country) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;
    
    // Open database
    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_gmt_offset\n");
        return 0;
    }
    
    // Get list of chart names
    const char *sql_select = "SELECT gmt_offset_secs FROM cities WHERE city = ? AND country = ? AND state = ?;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement (get_gmt_offset): %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, city, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, country, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, state, -1, SQLITE_STATIC);

    int gmt_offset_secs = 0;
   
    int found = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        gmt_offset_secs = sqlite3_column_int(stmt, 0);
        found = 1;
    }

    if (!found) {
        fprintf(stderr, "Warning: City '%s' not found in database.\n", city);
        return 0;
    }

    sqlite3_finalize(stmt);
    close_database(db);

    return gmt_offset_secs;
}



// void get_iana_timezone(char *city, char *state, char *country, char **timezone) {
//     sqlite3 *db;
//     sqlite3_stmt *stmt;
//     int rc;
    
//     // Open database
//     db = open_database();
//     if (!db) {
//         fprintf(stderr, "Failed to open database in get_iana_timezone\n");
//         return;
//     }
    
//     // Get list of chart names
//     const char *sql_select = "SELECT timezone FROM cities WHERE city = ? AND country = ? AND state = ?;";
//     rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
//     if (rc != SQLITE_OK) {
//         fprintf(stderr, "Failed to prepare statement (get_iana_timezone): %s\n", sqlite3_errmsg(db));
//         close_database(db);
//         return;
//     }

//     sqlite3_bind_text(stmt, 1, city, -1, SQLITE_STATIC);
//     sqlite3_bind_text(stmt, 2, country, -1, SQLITE_STATIC);
//     sqlite3_bind_text(stmt, 3, state, -1, SQLITE_STATIC);
   
//     int found = 0;
    
//     while (sqlite3_step(stmt) == SQLITE_ROW) {
//         *timezone = strdup((char *)sqlite3_column_text(stmt, 0));
//         found = 1;
//     }

//     if (!found) {
//         fprintf(stderr, "Warning: City '%s' not found in database.\n", city);
//         return;
//     }

//     sqlite3_finalize(stmt);
//     close_database(db);

// }
