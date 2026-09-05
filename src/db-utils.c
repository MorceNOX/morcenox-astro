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
#include <limits.h>
#include <sqlite3.h>
#include <locale.h>
#include <math.h>
#include "draw-chart.h"
#include "ini.h"
#include "selections.h"
#include "db-utils.h"
#include "var.h"
#include "planet_table.h"
#include "helper.h"


#include "var.h" // Garante o acesso à variável global_db e ao DB_PATH

sqlite3* open_database() {
    // Se o banco já estiver aberto, retorna a conexão existente instantaneamente
    if (global_db != NULL) {
        return global_db;
    }
    
    // Primeira execução: abre o arquivo fisicamente no disco
    int rc = sqlite3_open(DB_PATH, &global_db);
  
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(global_db));
        sqlite3_close(global_db);
        global_db = NULL;
        return NULL;
    }
    auto_vacuum(global_db);
    configurar_banco(global_db);
    //reconstruir_indice_cidades(global_db);

    return global_db;
}

void close_database(sqlite3 *db) {
    // Mantemos vazia para ignorar os fechamentos intermediários de cada query
    (void)db; 
}

void terminate_database() {
    // Fecha de verdade no encerramento do programa (chame na sua main)
    if (global_db != NULL) {
        sqlite3_close(global_db);
        global_db = NULL;
    }
}


int reconstruir_indice_cidades(sqlite3 *db) {
    char *zErrMsg = 0;
    
    // Lista de comandos SQL para executar em sequência
    const char *sql_commands[] = {
        "BEGIN TRANSACTION;",
        
        // 1. Exclui o índice antigo se ele já existir
        "DROP INDEX IF EXISTS idx_city_search;",
        
        // 2. Cria o novo índice robusto com a colação universal na cidade
        "CREATE INDEX idx_city_search ON cities (country, state COLLATE GLOBAL_SEM_ACENTO, city COLLATE GLOBAL_SEM_ACENTO);",

        "DROP INDEX IF EXISTS idx_chart_search;",
        "CREATE INDEX idx_chart_search ON charts (chart_name COLLATE GLOBAL_SEM_ACENTO);",
        
        "COMMIT;"
    };

    int num_commands = sizeof(sql_commands) / sizeof(sql_commands[0]);

    for (int i = 0; i < num_commands; i++) {
        if (sqlite3_exec(db, sql_commands[i], NULL, NULL, &zErrMsg) != SQLITE_OK) {
            printf("Erro ao executar: %s\nMotivo: %s\n", sql_commands[i], zErrMsg);
            sqlite3_free(zErrMsg);
            
            // Se falhar no meio, tenta desfazer para não deixar o banco travado
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            return 0;
        }
    }

    printf("Índice 'idx_city_search' excluído e reconstruído com sucesso!\n");
    printf("Índice 'idx_chart_search' excluído e reconstruído com sucesso!\n");
    return 1;
}


int configurar_banco(sqlite3 *db) {
    char *zErrMsg = 0;

    // libsqlite3 deve ter sido compilada com -DSQLITE_ENABLE_ICU

    // Cria a colação global universal utilizando a função SQL injetada pela ICU
    const char* sql_setup = "SELECT icu_load_collation('', 'GLOBAL_SEM_ACENTO', 'PRIMARY');";
    
    if (sqlite3_exec(db, sql_setup, NULL, NULL, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "Erro ao registrar colação universal: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }

    
    // Executa o comando de criação do índice logo após registrar a colação
    const char* sql_index = "CREATE INDEX IF NOT EXISTS idx_city_search ON cities (country, state COLLATE GLOBAL_SEM_ACENTO, city COLLATE GLOBAL_SEM_ACENTO);";

    if (sqlite3_exec(db, sql_index, NULL, NULL, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "Erro ao criar índice: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }

    const char* sql_index2 = "CREATE INDEX IF NOT EXISTS idx_chart_search ON charts (chart_name COLLATE GLOBAL_SEM_ACENTO);";

    if (sqlite3_exec(db, sql_index2, NULL, NULL, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "Erro ao criar índice: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }

    //fprintf(stderr, "Colação GLOBAL_SEM_ACENTO ativada usando a biblioteca SQLite externa!\n");
    return 1;
}



void auto_vacuum(sqlite3 *db) {
    if (!db) {
        return;
    }

    char *err_msg = 0;

    int rc = sqlite3_exec(db, "PRAGMA incremental_vacuum;", 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erro ao executar o vácuo incremental: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}


int execute_query(sqlite3 *db, const char *sql, sqlite3_stmt **stmt) {
    
    int rc = sqlite3_prepare_v2(db, sql, -1, stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }
    
    return SQLITE_OK;
}

int execute_with_database(const char *sql, int (*callback)(sqlite3*, sqlite3_stmt*)) {
    
    sqlite3 *db = open_database();
    if (!db) return 1;
    
    sqlite3_stmt *stmt;
    int rc = execute_query(db, sql, &stmt);
    if (rc != SQLITE_OK) {
        close_database(db);
        return 1;
    }
    
    // Execute callback
    rc = callback(db, stmt);
    
    sqlite3_finalize(stmt);
    close_database(db);
    return rc;
}


int prepare_statement(sqlite3 *db, const char *sql, sqlite3_stmt **stmt) {
    
    int rc = sqlite3_prepare_v2(db, sql, -1, stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return rc;
    }
    
    return SQLITE_OK;
}




int get_coordinates(char *city, char *country, char *state, double *lat, double *lon, double *elev, double *tz_offset, char *tz_iana) {    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_lat_lon_elev\n");
        return 0;
    }

    const char *sql_select = "SELECT lat, lon, elev, timezone, gmt_offset_secs FROM cities WHERE country = ? AND state = ? COLLATE GLOBAL_SEM_ACENTO AND city = ? COLLATE GLOBAL_SEM_ACENTO;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    sqlite3_bind_text(stmt, 1, country, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, state, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, city, -1, SQLITE_STATIC);
  
    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        *lat = sqlite3_column_double(stmt, 0);
        *lon = sqlite3_column_double(stmt, 1);
        *elev = sqlite3_column_double(stmt, 2);
        const char *timezone = (const char*)sqlite3_column_text(stmt, 3);
        *tz_offset = sqlite3_column_double(stmt, 4) / 3600;
        
        snprintf(tz_iana, 100, "%s", timezone);        
        
        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}


int get_house_meaning(int house, char *meaning) {   

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_house_meaning\n");
        return 0;
    }

    const char *sql_select4 = "SELECT meaning FROM houses WHERE id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select4, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, house);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        const char *mean = (const char*)sqlite3_column_text(stmt, 0);
        snprintf(meaning, 100, "%s", mean);
        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}






int get_planet_properties(int id, PrimitiveProperties *props) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_planet_properties\n");
        return 0;
    }

    const char *sql_select4 = "SELECT temperature, moisture FROM planets WHERE id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select4, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, id);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        int temperature = sqlite3_column_int(stmt, 0);
        int moisture = sqlite3_column_int(stmt, 1);

        props->temperature = temperature;
        props->moisture = moisture;
        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}



int get_sign_properties(int sign, PrimitiveProperties *props) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_sign_properties\n");
        return 0;
    }

    const char *sql_select4 = "SELECT e.temperature, e.moisture FROM signs s INNER JOIN elements e ON s.element = e.id WHERE s.id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select4, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, sign);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        int temperature = sqlite3_column_int(stmt, 0);
        int moisture = sqlite3_column_int(stmt, 1);

        props->temperature = temperature;
        props->moisture = moisture;
        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}




void get_countries_from_db(char ***countries, int *count) {
     
    sqlite3 *db;
    int rc;
    
    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_countries_from_db\n");
        return;
    }
    
    const char *sql_select = "SELECT DISTINCT country FROM cities ORDER BY country;";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        close_database(db);
        *count = 0;
        return;
    }
    
    // First pass: count rows
    int row_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        row_count++;
    }
    
    // Reset statement
    sqlite3_reset(stmt);
    
    // Allocate array
    *count = row_count;
    *countries = malloc(row_count * sizeof(char*));
    
    // Second pass: fill array
    int index = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *country = (const char*)sqlite3_column_text(stmt, 0);
        (*countries)[index] = strdup(country);
        index++;
    }
    
    sqlite3_finalize(stmt);
    close_database(db);
}

// Helper function to get cities from database for a specific country
void get_cities_from_db(const char *country, char ***cities, char ***states, int *count) {
     
    sqlite3 *db;
    int rc;
    
    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_cities_from_db\n");
        return;
    }
    
    const char *sql_select = "SELECT city, state FROM cities WHERE country = ? ORDER BY city COLLATE GLOBAL_SEM_ACENTO;";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        close_database(db);
        *count = 0;
        return;
    }
    
    sqlite3_bind_text(stmt, 1, country, -1, SQLITE_TRANSIENT);
    
    // First pass: count rows
    int row_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        row_count++;
    }
    
    // Reset statement
    sqlite3_reset(stmt);
    
    // Rebind and allocate array
    sqlite3_bind_text(stmt, 1, country, -1, SQLITE_TRANSIENT);
    *count = row_count;
    *cities = malloc(row_count * sizeof(char*));
    *states = malloc(row_count * sizeof(char*));
    
    // Second pass: fill array
    int index = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *city = (const char*)sqlite3_column_text(stmt, 0);
        const char *state = (const char*)sqlite3_column_text(stmt, 1);
        (*cities)[index] = strdup(city);
        (*states)[index] = strdup(state);
        index++;
    }
    
    sqlite3_finalize(stmt);
    close_database(db);
}

// Helper function to free string array
void free_string_array(char **array, int count) {
    if (array) {
        for (int i = 0; i < count; i++) {
            free(array[i]);
        }
        free(array);
    }
}

int get_planet_orbis(int planet_ids[], double planet_orbis[], char planet_symbols[][10], int max_planets) {
    
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;
    int count = 0;
    
    // Open database
    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_planet_orbis\n");
        return -1;
    }

    const char *sql_select = "SELECT id, symbol, orbis FROM planets ORDER BY id;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return -1;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_planets) {
        int id = sqlite3_column_int(stmt, 0);
        const char *symbol = (const char *)sqlite3_column_text(stmt, 1);
        double orbis = sqlite3_column_double(stmt, 2);
        
        planet_ids[count] = id;
        planet_orbis[count] = orbis;
        snprintf(planet_symbols[count], 10, "%s", symbol);
        count++;
    }
    
    // Clean up statement resources
    sqlite3_finalize(stmt);
    close_database(db);
    
    return count; // Return the number of planets retrieved
}


int get_rulers(char *sign_symbol, int *n_ruler, int *n_exalted, int *n_exile, int *n_fall, int *n_tri1, int *n_tri2, int *n_tri3) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_rulers\n");
        return 0;
    }

    const char *sql_select = "SELECT s.ruler, s.exalted, s.exile, s.fall, e.ruler1, e.ruler2, e.ruler3 FROM signs s INNER JOIN elements e ON s.element = e.id WHERE s.symbol = ?;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, sign_symbol, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        *n_ruler = sqlite3_column_int(stmt, 0);
        *n_exalted = sqlite3_column_int(stmt, 1); 
        *n_exile = sqlite3_column_int(stmt, 2);
        *n_fall = sqlite3_column_int(stmt, 3);
        *n_tri1 = sqlite3_column_int(stmt, 4);
        *n_tri2 = sqlite3_column_int(stmt, 5); 
        *n_tri3 = sqlite3_column_int(stmt, 6);

        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);

    return found;

}



int get_rulers_by_lon(double longitude, bool consider_modern_planets_rulling, int *n_dom, int *n_exalted, int *n_tri1, int *n_tri2, int *n_tri3, int *n_term, int *n_dec) {

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_rulers_by_sign_id\n");
        return 0;
    }

    const char *sql_select = "SELECT s.ruler, s.exalted, e.ruler1, e.ruler2, e.ruler3 FROM signs s INNER JOIN elements e ON s.element = e.id WHERE s.id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    int sign = (int)floor(longitude / 30) + 1;

    sqlite3_bind_int(stmt, 1, sign);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        *n_dom = sqlite3_column_int(stmt, 0);
        *n_exalted = sqlite3_column_int(stmt, 1); 
        *n_tri1 = sqlite3_column_int(stmt, 2);
        *n_tri2 = sqlite3_column_int(stmt, 3); 
        *n_tri3 = sqlite3_column_int(stmt, 4);
        
        if (!consider_modern_planets_rulling && (*n_exalted >= 8 && *n_exalted <= 10)) {
            *n_exalted = 0;
        }

        found = 1;
    }

    int ndec = get_decan(longitude);
    int decan_ruler = get_decan_ruler(ndec);
 
    int regente_do_termo = get_term_ruler(longitude); 
    
    *n_term = converter_codigo_planeta(regente_do_termo);
    *n_dec = converter_codigo_planeta(decan_ruler);

    sqlite3_finalize(stmt);
    close_database(db);

    return found;

}


int get_rulers_by_sign_id(int sign, int *n_ruler, int *n_exalted, int *n_exile, int *n_fall, int *n_tri1, int *n_tri2, int *n_tri3) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_rulers_by_sign_id\n");
        return 0;
    }

    const char *sql_select = "SELECT s.ruler, s.exalted, s.exile, s.fall, e.ruler1, e.ruler2, e.ruler3 FROM signs s INNER JOIN elements e ON s.element = e.id WHERE s.id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, sign);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        *n_ruler = sqlite3_column_int(stmt, 0);
        *n_exalted = sqlite3_column_int(stmt, 1); 
        *n_exile = sqlite3_column_int(stmt, 2);
        *n_fall = sqlite3_column_int(stmt, 3);
        *n_tri1 = sqlite3_column_int(stmt, 4);
        *n_tri2 = sqlite3_column_int(stmt, 5); 
        *n_tri3 = sqlite3_column_int(stmt, 6);

        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);

    return found;

}


int get_ruler_dom_by_sign_id(int sign, int *n_ruler) {    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_ruler_dom_by_sign_id\n");
        return 0;
    }

    const char *sql_select = "SELECT ruler FROM signs WHERE id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, sign);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        *n_ruler = sqlite3_column_int(stmt, 0);
        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);

    return found;

}


int get_ruler_exalt_by_sign_id(int sign, int *n_ruler, bool consider_modern_planets_rulling) {    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_ruler_exalt_by_sign_id\n");
        return 0;
    }

    const char *sql_select = "SELECT exalted FROM signs WHERE id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, sign);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        *n_ruler = sqlite3_column_int(stmt, 0);

        if (!consider_modern_planets_rulling && (*n_ruler > 7 && *n_ruler < 11)) {
            *n_ruler = 0;
        }
        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);

    return found;

}



int get_planet_gender(char *planet_symbol, int *gender_id) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_planet_gender\n");
        return 0;
    }

    const char *sql_select4 = "SELECT gender FROM planets WHERE symbol = ?;";
    rc = sqlite3_prepare_v2(db, sql_select4, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, planet_symbol, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        *gender_id = sqlite3_column_int(stmt, 0);
        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}




int get_quadrant_gender(int house, int *gender_id) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_quadrant_gender\n");
        return 0;
    }

    const char *sql_select4 = "SELECT q.gender FROM quadrant q INNER JOIN houses h ON h.quadrant = q.id WHERE h.id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select4, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, house);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        *gender_id = sqlite3_column_int(stmt, 0);
        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}


int get_default_gender() {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_default_gender\n");
        return 0;
    }

    const char *sql_select4 = "SELECT gender FROM profiles WHERE profile = 'default';";
    rc = sqlite3_prepare_v2(db, sql_select4, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    int found = 0;
    int id = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        id = sqlite3_column_int(stmt, 0);
        found = 1;
    }

    if (!found) {
        fprintf(stderr, "Default gender not found\n");
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return id;

}



int get_default_terms_system() {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_default_terms_system\n");
        return 0;
    }

    const char *sql_select4 = "SELECT terms_system FROM profiles WHERE profile = 'default';";
    rc = sqlite3_prepare_v2(db, sql_select4, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    int found = 0;
    int id = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        id = sqlite3_column_int(stmt, 0);
        found = 1;
    }

    if (!found) {
        fprintf(stderr, "Default terms system not found\n");
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return id;

}


int get_sign_gender(char *sign_symbol, int *gender_id) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_sign_gender\n");
        return 0;
    }

    const char *sql_select5 = "SELECT gender FROM signs WHERE symbol = ?;";
    rc = sqlite3_prepare_v2(db, sql_select5, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, sign_symbol, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        *gender_id = sqlite3_column_int(stmt, 0);
        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}


int get_planet_sect(char *planet_symbol, int *sect_id) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_planet_sect\n");
        return 0;
    }

    const char *sql_select6 = "SELECT sect FROM planets WHERE symbol = ?;";
    rc = sqlite3_prepare_v2(db, sql_select6, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, planet_symbol, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        *sect_id = sqlite3_column_int(stmt, 0); 
        found = 1;  
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}

int get_planet_orientality(char *planet_symbol, char **orientality) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_planet_orientality\n");
        return 0;
    }

    const char *sql_select2 = "SELECT o.name FROM planets p INNER JOIN orientality o ON p.orientality = o.id WHERE p.symbol = ?;";
    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, planet_symbol, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) { 
        *orientality = strdup((char *)sqlite3_column_text(stmt, 0));
        //snprintf(*ruler_symbol, 10, "%s", sqlite3_column_text(stmt, 0));
        found = 1;      
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;
  
}


int get_sign_ruler_by_domicile(char *sign_symbol, char **ruler_symbol) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_sign_ruler_by_domicile\n");
        return 0;
    }

    const char *sql_select2 = "SELECT planets.symbol FROM signs INNER JOIN planets ON signs.ruler = planets.id WHERE signs.symbol = ?;";
    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, sign_symbol, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) { 
        *ruler_symbol = strdup((char *)sqlite3_column_text(stmt, 0));
        //snprintf(*ruler_symbol, 10, "%s", sqlite3_column_text(stmt, 0));
        found = 1;      
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}



int get_season(char *sign_symbol, char **season) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_season\n");
        return 0;
    }

    const char *sql_select2 = "SELECT s.name FROM seasons s INNER JOIN signs g ON g.season = s.id WHERE g.symbol = ?;";
    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, sign_symbol, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) { 
        *season = strdup((char *)sqlite3_column_text(stmt, 0));
        //snprintf(*ruler_symbol, 10, "%s", sqlite3_column_text(stmt, 0));
        found = 1;      
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}


int get_season_id(int sign, int *season) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_season_id\n");
        return 0;
    }

    const char *sql_select2 = "SELECT s.id FROM seasons s INNER JOIN signs g ON g.season = s.id WHERE g.id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, sign);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) { 
        *season = sqlite3_column_int(stmt, 0);
        found = 1;      
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}



int get_season_temperament(char *season, char **temperament) {

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_season_temperament\n");
        return 0;
    }

    const char *sql_select2 = "SELECT t.name FROM temperament t INNER JOIN seasons s ON s.element = t.element WHERE s.name = ?;";
    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, season, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) { 
        *temperament = strdup((char *)sqlite3_column_text(stmt, 0));
        //snprintf(*ruler_symbol, 10, "%s", sqlite3_column_text(stmt, 0));
        found = 1;      
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}


int get_season_temperament_by_season_id(int season_id, char **temperament) {

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_season_temperament_by_season_id\n");
        return 0;
    }

    const char *sql_select2 = "SELECT t.name FROM temperament t INNER JOIN seasons s ON s.element = t.element WHERE s.id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, season_id);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) { 
        *temperament = strdup((char *)sqlite3_column_text(stmt, 0));
        //snprintf(*ruler_symbol, 10, "%s", sqlite3_column_text(stmt, 0));
        found = 1;      
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}



int get_moon_temperament(const char *phase, char **temperament) {

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_moon_temperament\n");
        return 0;
    }

    const char *sql_select2 = "SELECT t.name FROM moon_phases m INNER JOIN temperament t ON m.temperament = t.id WHERE m.phase_display = ?;";
    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, phase, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) { 
        *temperament = strdup((char *)sqlite3_column_text(stmt, 0));
        found = 1;      
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;
   
}


int get_moon_temperament_by_phase_id(int phase_id, char **temperament) {

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_moon_temperament_by_phase_id\n");
        return 0;
    }

    const char *sql_select2 = "SELECT t.name FROM moon_phases m INNER JOIN temperament t ON m.temperament = t.id WHERE m.id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, phase_id);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) { 
        *temperament = strdup((char *)sqlite3_column_text(stmt, 0));
        found = 1;      
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;
   
}


int get_sign_ruler_by_exaltation(char *sign_symbol, char **ruler_symbol, int consider_modern_planets) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_sign_ruler_by_exaltation\n");
        return 0;
    }

    const char *sql_select2 = "SELECT planets.id, planets.symbol FROM signs INNER JOIN planets ON signs.exalted = planets.id WHERE signs.symbol = ?;";
    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, sign_symbol, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        
        if (consider_modern_planets) {
            *ruler_symbol = strdup((char *)sqlite3_column_text(stmt, 1));    
        }
        else if (!consider_modern_planets && (id > 7 && id < 11)) {
            *ruler_symbol = strdup("");
        }
        else {
            *ruler_symbol = strdup((char *)sqlite3_column_text(stmt, 1));
        }
        found = 1;      
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}


int get_planet_speed(char *planet_symbol, double *speed) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_planet_speed\n");
        return 0;
    }

    const char *sql_select4 = "SELECT speed FROM planets WHERE symbol = ?;";
    rc = sqlite3_prepare_v2(db, sql_select4, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, planet_symbol, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        *speed = sqlite3_column_double(stmt, 0);
        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}



int get_weights(double weights[50], bool show_modern_planets) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_weights\n");
        return 0;
    }

    const char *sql_select4 = "SELECT force FROM objects WHERE id <= 19;";
    const char *sql_select5 = "SELECT force FROM objects WHERE (id BETWEEN 1 AND 7) OR (id BETWEEN 11 AND 19);";
    
    if (show_modern_planets) {
        rc = sqlite3_prepare_v2(db, sql_select4, -1, &stmt, NULL);
    }
    else {
        rc = sqlite3_prepare_v2(db, sql_select5, -1, &stmt, NULL);
    }
        
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }
    
    //sqlite3_bind_text(stmt, 1, planet_symbol, -1, SQLITE_STATIC);

    int index = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        weights[index] = sqlite3_column_double(stmt, 0);

        index++;
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return index;

}




int get_chart_objects(ChartObject obj[100]) {   

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_chart_objects\n");
        return 0;
    }

    const char *sql_select5 = "SELECT id, symbol, name, type, fixed_longitude, object_ref FROM objects ORDER BY type ASC, id ASC;";
    rc = sqlite3_prepare_v2(db, sql_select5, -1, &stmt, NULL);
    
        
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        close_database(db);
        return 0;
    }
    
    int index = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {   
        int id = sqlite3_column_int(stmt, 0);
        const char *symbol = (const char *)sqlite3_column_text(stmt, 1);
        const char *name = (const char *)sqlite3_column_text(stmt, 2);  
        int type = sqlite3_column_int(stmt, 3);
        double lon = sqlite3_column_double(stmt, 4);
        int object_ref = sqlite3_column_int(stmt, 5);

        obj[index].id = id;
        snprintf(obj[index].object, 10, "%s", symbol);
        snprintf(obj[index].object_name, 30, "%s", name);
        obj[index].type = type;
        obj[index].longitude = lon;
        if (object_ref) {
            obj[index].object_ref = object_ref;
        }
        index++;
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return index;

}


double get_decl_orbis() {
    double decl_orbis = 1.0;  // Orbe padrão se o banco falhar
    sqlite3 *db = open_database();
    if (db) {
        sqlite3_stmt *stmt;
        const char *sql_select_p_orbis = "SELECT parallel_orbis FROM profiles WHERE profile = 'default';";
        int rc = sqlite3_prepare_v2(db, sql_select_p_orbis, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                decl_orbis = sqlite3_column_double(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        close_database(db);
    }

    return decl_orbis;
}


double get_antissia_orbis() {
    double ant_orbis = 1.0;  // Orbe padrão se o banco falhar
    sqlite3 *db = open_database();
    if (db) {
        sqlite3_stmt *stmt;
        const char *sql_select_p_orbis = "SELECT antisia_orb FROM profiles WHERE profile = 'default';";
        int rc = sqlite3_prepare_v2(db, sql_select_p_orbis, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                ant_orbis = sqlite3_column_double(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        close_database(db);
    }

    return ant_orbis;
}


int get_house_system_name(char house_system_id, char **house_system_name) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_sign_ruler_by_domicile\n");
        return 0;
    }

    const char *sql_select2 = "SELECT name FROM house_system WHERE id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);
    
    char id[10] = "";
    snprintf(id, sizeof(id), "%c", house_system_id);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, id, -1, SQLITE_STATIC);

    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) { 
        *house_system_name = strdup((char *)sqlite3_column_text(stmt, 0));
        found = 1;      
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}

int update_triplicity_rulers(int triplicity_system) {
    

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in update_triplicity_rulers\n");
        return 0;
    }

    // Fire
    const char *sql_select0;
    sql_select0 = "UPDATE elements SET (ruler1, ruler2, ruler3) = (SELECT ruler1, ruler2, ruler3 FROM triplicity_rulers WHERE system = ? AND element = ?) WHERE id = ?;";

    rc = sqlite3_prepare_v2(db, sql_select0, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement in update_triplicity_rulers: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, triplicity_system);
    sqlite3_bind_int(stmt, 2, 1);
    sqlite3_bind_int(stmt, 3, 1);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed saving triplicity rulers: %s\n", sqlite3_errmsg(db));
        return 0;
    }


    // Water
    const char *sql_select1;
    sql_select1 = "UPDATE elements SET (ruler1, ruler2, ruler3) = (SELECT ruler1, ruler2, ruler3 FROM triplicity_rulers WHERE system = ? AND element = ?) WHERE id = ?;";

    rc = sqlite3_prepare_v2(db, sql_select1, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement in update_triplicity_rulers: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, triplicity_system);
    sqlite3_bind_int(stmt, 2, 2);
    sqlite3_bind_int(stmt, 3, 2);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed saving triplicity rulers: %s\n", sqlite3_errmsg(db));
        return 0;
    }


    // Air
    const char *sql_select2;
    sql_select2 = "UPDATE elements SET (ruler1, ruler2, ruler3) = (SELECT ruler1, ruler2, ruler3 FROM triplicity_rulers WHERE system = ? AND element = ?) WHERE id = ?;";

    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement in update_triplicity_rulers: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, triplicity_system);
    sqlite3_bind_int(stmt, 2, 3);
    sqlite3_bind_int(stmt, 3, 3);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed saving triplicity rulers: %s\n", sqlite3_errmsg(db));
        return 0;
    }


    // Earth
    const char *sql_select3;
    sql_select3 = "UPDATE elements SET (ruler1, ruler2, ruler3) = (SELECT ruler1, ruler2, ruler3 FROM triplicity_rulers WHERE system = ? AND element = ?) WHERE id = ?;";
    
    rc = sqlite3_prepare_v2(db, sql_select3, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement in update_triplicity_rulers: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, triplicity_system);
    sqlite3_bind_int(stmt, 2, 4);
    sqlite3_bind_int(stmt, 3, 4);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed saving triplicity rulers: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_finalize(stmt);
    close_database(db);

    return 1;

}

int update_settings(ChartOptions options) {
    
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in update_settings\n");
        return 0;
    }

    const char *sql_select;
    sql_select = "UPDATE profiles SET dark_mode = ?, house_system = ?, triplicity_system = ?, terms_system = ?, modern_planets_rulling = ?, show_modern_planets = ?, gender = ?, language = ? WHERE profile = ?;";

    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    char house_system[10];
    snprintf(house_system, sizeof(house_system), "%c", options.house_system);

    sqlite3_bind_int(stmt, 1, options.dark_mode);    
    sqlite3_bind_text(stmt, 2, house_system, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, options.triplicity_system);
    sqlite3_bind_int(stmt, 4, options.terms_system);
    sqlite3_bind_int(stmt, 5, options.modern_planets_rulling);
    sqlite3_bind_int(stmt, 6, options.show_modern_planets);
    sqlite3_bind_int(stmt, 7, options.gender);
    sqlite3_bind_text(stmt, 8, options.language, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, "default", -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed saving default options: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_finalize(stmt);
    close_database(db);

    return 1;

}
