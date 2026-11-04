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
#include "fuso-horario.h"


#include "var.h" // Garante o acesso à variável global_db e ao DB_PATH


sqlite3* open_database() {
    if (global_db != NULL) {
        return global_db;
    }

    sqlite3 *disk_db = NULL;
    sqlite3 *mem_db = NULL;

    // 1. Open the original database file from the disk
    int rc = sqlite3_open(DB_PATH, &disk_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open disk database: %s\n", sqlite3_errmsg(disk_db));
        if (disk_db) sqlite3_close(disk_db);
        return NULL;
    }

    // 2. Open a new, empty database in RAM
    rc = sqlite3_open(":memory:", &mem_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open in-memory database: %s\n", sqlite3_errmsg(mem_db));
        sqlite3_close(disk_db);
        return NULL;
    }

    // 3. Use the Backup API to copy disk content to RAM
    sqlite3_backup *pBackup;
    pBackup = sqlite3_backup_init(mem_db, "main", disk_db, "main");
    
    if (pBackup) {
        // -1 copies the entire database in one step
        rc = sqlite3_backup_step(pBackup, -1);
        if (rc != SQLITE_OK && rc != SQLITE_DONE) {
            fprintf(stderr, "Backup failed: %s\n", sqlite3_errmsg(mem_db));
        }
        sqlite3_backup_finish(pBackup);
    }

    // 4. Close the disk connection immediately
    // We no longer need the file on disk once it's in RAM
    sqlite3_close(disk_db);

    // 5. Switch global_db to the RAM connection
    global_db = mem_db;

    // 6. Re-apply connection-specific settings
    // Note: Custom collations (like ICU) and PRAGMAs must be re-applied 
    // to the new connection because they are connection-specific, not file-specific.
    activate_foreign_keys(global_db);
    auto_vacuum(global_db);
    configurar_banco(global_db);

    if (REINDEX_DB) {
        reconstruir_indice_cidades(global_db);
    }

    printf("Database successfully loaded into RAM.\n");
    return global_db;
}


void recarregar_banco_na_ram() {
    if (global_db == NULL) return;

    // 1. Fecha a conexão atual da RAM de forma bruta para limpar todos os ponteiros velhos
    sqlite3_close(global_db);
    global_db = NULL;

    // 2. Chama a sua função open_database() original. 
    // Como global_db está NULL, ela vai reabrir o arquivo físico do disco do zero
    // e criar um espelho zerado, limpo e atualizado na memória RAM!
    open_database();
    
    printf("Banco de dados RAM totalmente recarregado e sincronizado após INSERT.\n");
}



// Função auxiliar para sincronizar a RAM com o disco imediatamente
void persistir_no_disco() {
    if (global_db == NULL) return;
    
    sqlite3 *disk_db = NULL;
    if (sqlite3_open(DB_PATH, &disk_db) == SQLITE_OK) {
        sqlite3_backup *pBackup = sqlite3_backup_init(disk_db, "main", global_db, "main");
        if (pBackup) {
            sqlite3_backup_step(pBackup, -1);
            sqlite3_backup_finish(pBackup);
        }
        sqlite3_close(disk_db);
    }
}



int sincronizar_com_disco(sqlite3 *db, const char *sql_sync, void (*bind_func)(sqlite3_stmt*)) {
    char *zErrMsg = 0;
    int rc;

    // 1. Monta e executa o ATTACH dinâmico baseado na sua política de zero hardcoding
    char *sql_attach = sqlite3_mprintf("ATTACH DATABASE %Q AS disco;", DB_PATH);
    rc = sqlite3_exec(db, sql_attach, NULL, NULL, &zErrMsg);
    sqlite3_free(sql_attach); // Libera a memória imediatamente

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erro ao conectar ao disco para sincronizar: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }

    // 2. Prepara o comando SQL de sincronização recebido por parâmetro
    sqlite3_stmt *stmt_sync = NULL;
    rc = sqlite3_prepare_v2(db, sql_sync, -1, &stmt_sync, NULL);
    
    if (rc == SQLITE_OK) {
        // Se a chamada enviou uma função de bind (para evitar SQL Injection), nós a executamos aqui
        if (bind_func != NULL) {
            bind_func(stmt_sync);
        }

        // Executa a sincronização propriamente dita
        rc = sqlite3_step(stmt_sync);
        sqlite3_finalize(stmt_sync);
    } else {
        fprintf(stderr, "Erro ao preparar SQL de sincronização: %s\n", sqlite3_errmsg(db));
    }

    // 3. Garante o DETACH para liberar o arquivo físico no disco, aconteça o que acontecer
    sqlite3_exec(db, "DETACH DATABASE disco;", NULL, NULL, NULL);

    return (rc == SQLITE_DONE || rc == SQLITE_OK);
}





void close_database(sqlite3 *db) {
    // Mantemos vazia para ignorar os fechamentos intermediários de cada query
    (void)db; 
}

void terminate_database() {
    // Fecha de verdade no encerramento do programa (chamar na main)
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

        "DROP INDEX IF EXISTS idx_coordenadas;",

        "CREATE INDEX IF NOT EXISTS idx_coordenadas ON cities (lat, lon);",

        "DROP INDEX IF EXISTS idx_coordenadas_pais;",

        "CREATE INDEX IF NOT EXISTS idx_coordenadas_pais ON cities (lat, lon, country_code);",

        "DROP INDEX IF EXISTS idx_planet_by_symbol;",

        "CREATE INDEX IF NOT EXISTS idx_planet_by_symbol ON planets (symbol);",

        "DROP INDEX IF EXISTS idx_sign_by_symbol;",

        "CREATE INDEX IF NOT EXISTS idx_sign_by_symbol ON signs (symbol);",

        "DROP INDEX IF EXISTS idx_country;",

        "CREATE INDEX IF NOT EXISTS idx_country ON cities (country);",

        "DROP INDEX IF EXISTS idx_country_state;",

        "CREATE INDEX IF NOT EXISTS idx_country_state ON cities (country, state COLLATE GLOBAL_SEM_ACENTO);",
        
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

    printf("Índices excluídos e reconstruídos com sucesso!\n");
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

    const char* sql_index3 = "CREATE INDEX IF NOT EXISTS idx_coordenadas ON cities (lat, lon);";

    if (sqlite3_exec(db, sql_index3, NULL, NULL, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "Erro ao criar índice: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }

    const char* sql_index4 = "CREATE INDEX IF NOT EXISTS idx_coordenadas_pais ON cities (lat, lon, country_code);";

    if (sqlite3_exec(db, sql_index4, NULL, NULL, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "Erro ao criar índice: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }

    const char* sql_index5 = "CREATE INDEX IF NOT EXISTS idx_planet_by_symbol ON planets (symbol);";

    if (sqlite3_exec(db, sql_index5, NULL, NULL, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "Erro ao criar índice: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }

    const char* sql_index6 = "CREATE INDEX IF NOT EXISTS idx_sign_by_symbol ON signs (symbol);";

    if (sqlite3_exec(db, sql_index6, NULL, NULL, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "Erro ao criar índice: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }

    const char* sql_index7 = "CREATE INDEX IF NOT EXISTS idx_country ON cities (country);";

    if (sqlite3_exec(db, sql_index7, NULL, NULL, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "Erro ao criar índice: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }

    const char* sql_index8 = "CREATE INDEX IF NOT EXISTS idx_country_state ON cities (country, state COLLATE GLOBAL_SEM_ACENTO);";

    if (sqlite3_exec(db, sql_index8, NULL, NULL, &zErrMsg) != SQLITE_OK) {
        fprintf(stderr, "Erro ao criar índice: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    }

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


void activate_foreign_keys(sqlite3 *db) {
    if (!db) {
        return;
    }

    char *err_msg = 0;

    int rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, &err_msg);

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



int buscar_cidade_mais_proxima_pais(double lat_busca, double lon_busca, double raio_km, char *cod_pais_iso, CidadeVizinha *resultado) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in buscar_cidade_mais_proxima\n");
        return 0;
    }
    
    // 1. Criação da Bounding Box (Filtro rápido de ~50km de margem)
    double margem_lat = raio_km / 111.0; 
    // Ajusta a margem de longitude com base na latitude atual para evitar distorções perto dos polos
    double margem_lon = raio_km / (111.0 * cos(lat_busca * M_PI / 180.0)); 

    double lat_min = lat_busca - margem_lat;
    double lat_max = lat_busca + margem_lat;
    double lon_min = lon_busca - margem_lon;
    double lon_max = lon_busca + margem_lon;

    // 2. Query SQL Otimizada com Haversine nativo do SQLite
    const char *sql = 
        "SELECT state, elev, "
        "       (6371 * acos("
        "           cos(radians(?1)) * cos(radians(lat)) * "
        "           cos(radians(lon) - radians(?2)) + "
        "           sin(radians(?1)) * sin(radians(lat))"
        "       )) AS distancia_km "
        "FROM cities "
        "WHERE lat BETWEEN ?3 AND ?4 " // Filtro rápido usando índices (Bounding Box)
        "  AND lon BETWEEN ?5 AND ?6 "
        "  AND country_code = ?7 "
        "ORDER BY distancia_km ASC "
        "LIMIT 1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Erro ao preparar query: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    // 3. Vincula os parâmetros (Binds) de forma segura
    sqlite3_bind_double(stmt, 1, lat_busca);
    sqlite3_bind_double(stmt, 2, lon_busca);
    sqlite3_bind_double(stmt, 3, lat_min);
    sqlite3_bind_double(stmt, 4, lat_max);
    sqlite3_bind_double(stmt, 5, lon_min);
    sqlite3_bind_double(stmt, 6, lon_max);
    sqlite3_bind_text(stmt, 7, cod_pais_iso, -1, SQLITE_STATIC);

    // 4. Executa a busca
    int sucesso = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        snprintf(resultado->state, sizeof(resultado->state), "%s", (const char*)sqlite3_column_text(stmt, 0));
        
        resultado->elev = sqlite3_column_double(stmt, 1);
        
        resultado->distance_km = sqlite3_column_double(stmt, 2);

        sucesso = 1;
    }

    sqlite3_finalize(stmt);
    return sucesso; // Retorna 1 se achou uma cidade próxima, 0 se não houver nenhuma no quadrado de 50km
}


int buscar_cidade_mais_proxima(double lat_busca, double lon_busca, double raio_km, CidadeVizinha *resultado) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in buscar_cidade_mais_proxima\n");
        return 0;
    }
    
    // 1. Criação da Bounding Box (Filtro rápido de ~50km de margem)
    double margem_lat = raio_km / 111.0; 
    // Ajusta a margem de longitude com base na latitude atual para evitar distorções perto dos polos
    double margem_lon = raio_km / (111.0 * cos(lat_busca * M_PI / 180.0)); 

    double lat_min = lat_busca - margem_lat;
    double lat_max = lat_busca + margem_lat;
    double lon_min = lon_busca - margem_lon;
    double lon_max = lon_busca + margem_lon;

    // 2. Query SQL Otimizada com Haversine nativo do SQLite
    const char *sql = 
        "SELECT state, elev, "
        "       (6371 * acos("
        "           cos(radians(?1)) * cos(radians(lat)) * "
        "           cos(radians(lon) - radians(?2)) + "
        "           sin(radians(?1)) * sin(radians(lat))"
        "       )) AS distancia_km "
        "FROM cities "
        "WHERE lat BETWEEN ?3 AND ?4 " // Filtro rápido usando índices (Bounding Box)
        "  AND lon BETWEEN ?5 AND ?6 "
        "ORDER BY distancia_km ASC "
        "LIMIT 1;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Erro ao preparar query: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    // 3. Vincula os parâmetros (Binds) de forma segura
    sqlite3_bind_double(stmt, 1, lat_busca);
    sqlite3_bind_double(stmt, 2, lon_busca);
    sqlite3_bind_double(stmt, 3, lat_min);
    sqlite3_bind_double(stmt, 4, lat_max);
    sqlite3_bind_double(stmt, 5, lon_min);
    sqlite3_bind_double(stmt, 6, lon_max);

    // 4. Executa a busca
    int sucesso = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        snprintf(resultado->state, sizeof(resultado->state), "%s", (const char*)sqlite3_column_text(stmt, 0));
        
        resultado->elev = sqlite3_column_double(stmt, 1);
        
        resultado->distance_km = sqlite3_column_double(stmt, 2);

        sucesso = 1;
    }

    sqlite3_finalize(stmt);
    return sucesso; // Retorna 1 se achou uma cidade próxima, 0 se não houver nenhuma no quadrado de 50km
}



int get_data_from_nearest_city(double lat_busca, double lon_busca, char *pais, CidadeVizinha *resultado) {
    
    if (buscar_cidade_mais_proxima_pais(lat_busca, lon_busca, 50, pais, resultado)) {
        return 1;
    }

    show_alert_popup(_("No city found in 50km!"), _("Trying 200km."));
    if (buscar_cidade_mais_proxima_pais(lat_busca, lon_busca, 200, pais, resultado)) {
        return 1;
    }

    show_alert_popup(_("No city found in 200km!"), _("Trying 1000km."));
    if (buscar_cidade_mais_proxima_pais(lat_busca, lon_busca, 1000, pais, resultado)) {
        return 1;
    }

    show_alert_popup(_("No city found in 1000km!"), "");

    return 0;
}


int get_country_code(char *country, char *country_code) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_country_code\n");
        return 0;
    }
    const char *sql = "SELECT country_code FROM cities "
                      "WHERE country = ? "
                      "GROUP BY country_code;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        show_alert_popup(_("Failed prepare statement!"), sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, country, -1, SQLITE_STATIC);
    
    int found = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {            
        const char *code = (const char*)sqlite3_column_text(stmt, 0);
        snprintf(country_code, 100, "%s", code);
        found = 1;
    }

    sqlite3_finalize(stmt);
    close_database(db);
    
    return found;

}

int save_city() {
    sqlite3 *db;
    sqlite3_stmt *stmt_select = NULL;
    sqlite3_stmt *stmt_ram = NULL;
    sqlite3_stmt *stmt_disco = NULL;
    int rc;

    db = open_database();
    if (!db) return 0;

    // Buscando dados do país/estado na RAM
    const char *sql = "SELECT country_id, country_code, state_id, state_code FROM cities "
                      "WHERE country = ? AND state = ? COLLATE GLOBAL_SEM_ACENTO LIMIT 1;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt_select, NULL);
    if (rc != SQLITE_OK) {
        show_alert_popup(_("Failed prepare statement!"), sqlite3_errmsg(db));
        return 0;
    }

    char location_type[100];
    if (!set_elevation()) {
        show_alert_popup(_("Operation canceled by user!"), "");
        sqlite3_finalize(stmt_select);
        return 0;
    }
    if (!set_country(COUNTRY, sizeof(COUNTRY))) {
        show_alert_popup(_("Operation canceled by user!"), "");
        sqlite3_finalize(stmt_select);
        return 0;
    }
    if (!set_state(STATE, sizeof(STATE))) {
        show_alert_popup(_("Operation canceled by user!"), "");
        sqlite3_finalize(stmt_select);
        return 0;
    }
    if (!set_location_name(CITY, sizeof(CITY))) {
        show_alert_popup(_("Operation canceled by user!"), "");
        sqlite3_finalize(stmt_select);
        return 0;
    }
    if (!select_location_type(location_type)) {
        show_alert_popup(_("Operation canceled by user!"), "");
        sqlite3_finalize(stmt_select);
        return 0;
    }

    trim(COUNTRY);
    trim(STATE);
    trim(CITY);

    sqlite3_bind_text(stmt_select, 1, COUNTRY, -1, SQLITE_STATIC); 
    sqlite3_bind_text(stmt_select, 2, STATE, -1, SQLITE_STATIC);

    int country_id = 0;
    const char *country_code = NULL;
    int state_id = 0;
    const char *state_code = NULL;

    // CORREÇÃO 1: É obrigatório dar o 'step' para poder ler as colunas!
    if (sqlite3_step(stmt_select) == SQLITE_ROW) {
        country_id = sqlite3_column_int(stmt_select, 0);
        country_code = (const char*)sqlite3_column_text(stmt_select, 1);
        state_id = sqlite3_column_int(stmt_select, 2);
        state_code = (const char*)sqlite3_column_text(stmt_select, 3);
    } else {
        show_alert_popup(_("Failed saving city! (Country/State not found)"), "");
        sqlite3_finalize(stmt_select);
        return 0;
    }

    // 1. Conecta o arquivo físico do disco dinamicamente (Sem hardcoding)
    char *sql_attach = sqlite3_mprintf("ATTACH DATABASE %Q AS disco;", DB_PATH);
    sqlite3_exec(db, sql_attach, NULL, NULL, NULL);
    sqlite3_free(sql_attach);

    // O mesmo comando SQL serve para a RAM (main) e para o DISCO (disco)
    const char *sql_ins_ram   = "INSERT INTO main.cities (city, country, country_id, country_code, state, state_id, state_code, type, lat, lon, elev, timezone, gmt_offset_standard, gmt_offset_secs) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    const char *sql_ins_disco = "INSERT INTO disco.cities (city, country, country_id, country_code, state, state_id, state_code, type, lat, lon, elev, timezone, gmt_offset_standard, gmt_offset_secs) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    // 2. Prepara os dois statements lógicos
    sqlite3_prepare_v2(db, sql_ins_ram, -1, &stmt_ram, NULL);
    sqlite3_prepare_v2(db, sql_ins_disco, -1, &stmt_disco, NULL);

    // CORREÇÃO 3: Removida a duplicação das variáveis de tempo
    int secs = (int)obter_segundos_gmt_na_data(TZ_IANA, YY, MM, DD, HH, MIN);
    char gmt_str[10];
    get_degree_str((double)secs / 3600.0, gmt_str);

    // 3. Aplica os BINDS nos dois statements sequencialmente (No mesmo lugar!)
    sqlite3_stmt *statements[2] = {stmt_ram, stmt_disco};
    for (int i = 0; i < 2; i++) {
        if (!statements[i]) continue;
        sqlite3_bind_text(statements[i], 1, CITY, -1, SQLITE_STATIC);
        sqlite3_bind_text(statements[i], 2, COUNTRY, -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[i], 3, country_id);
        sqlite3_bind_text(statements[i], 4, country_code, -1, SQLITE_STATIC);
        sqlite3_bind_text(statements[i], 5, STATE, -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[i], 6, state_id);
        sqlite3_bind_text(statements[i], 7, state_code, -1, SQLITE_STATIC);
        sqlite3_bind_text(statements[i], 8, location_type, -1, SQLITE_STATIC);
        sqlite3_bind_double(statements[i], 9, LAT);
        sqlite3_bind_double(statements[i], 10, LON);
        sqlite3_bind_double(statements[i], 11, ELEV);
        sqlite3_bind_text(statements[i], 12, TZ_IANA, -1, SQLITE_STATIC);
        sqlite3_bind_text(statements[i], 13, gmt_str, -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[i], 14, secs);
    }

    // 4. Executa na RAM e no Disco
    int gravou_ram = (sqlite3_step(stmt_ram) == SQLITE_DONE);
    int gravou_disco = (sqlite3_step(stmt_disco) == SQLITE_DONE);

    // Finaliza os comandos de inserção
    sqlite3_finalize(stmt_ram);
    sqlite3_finalize(stmt_disco);
    
    // CORREÇÃO 2: Finaliza o comando de seleção inicial para evitar vazamento de memória
    sqlite3_finalize(stmt_select);

    // 5. Desconecta o disco imediatamente para liberar o arquivo físico
    sqlite3_exec(db, "DETACH DATABASE disco;", NULL, NULL, NULL);

    if (gravou_ram && gravou_disco) {
        show_alert_popup(_("City saved to RAM and Disk!"), "");
    } else {
        show_alert_popup(_("Failed to persist data correctly!"), sqlite3_errmsg(db));
        return 0;
    }

    close_database(db);
    return 1;
}




int get_coordinates(char *city, char *country, char *state, double *lat, double *lon, double *elev, double *tz_offset, char *tz_iana) {    
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_coordinates\n");
        return 0;
    }

    const char *sql_select = "SELECT lat, lon, elev, timezone, gmt_offset_secs FROM cities WHERE country = ? AND state = ? COLLATE GLOBAL_SEM_ACENTO AND city = ? COLLATE GLOBAL_SEM_ACENTO;";
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        close_database(db);
        return 0;
    }

    sqlite3_bind_text(stmt, 1, country, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, state, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, city, -1, SQLITE_STATIC);
  
    int found = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *lat = sqlite3_column_double(stmt, 0);
        *lon = sqlite3_column_double(stmt, 1);
        *elev = sqlite3_column_double(stmt, 2);
        const char *timezone = (const char*)sqlite3_column_text(stmt, 3);
        *tz_offset = sqlite3_column_double(stmt, 4) / 3600.0;
        
        snprintf(tz_iana, 100, "%s", timezone ? timezone : "");        
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



void get_cities_from_db_filter(const char *country, char ***cities, char ***states, int *count, char *location) {
     
    sqlite3 *db;
    int rc;
    
    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in get_cities_from_db_filter\n");
        return;
    }
    
    const char *sql_select = "SELECT city, state FROM cities WHERE country = ? AND city LIKE ('%' || ? || '%') ORDER BY city COLLATE GLOBAL_SEM_ACENTO;";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql_select, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        close_database(db);
        *count = 0;
        return;
    }
    
    sqlite3_bind_text(stmt, 1, country, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, location, -1, SQLITE_TRANSIENT);
    
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
        close_database(db);
        return 0;
    }

    const char *sql_select4 = "SELECT gender FROM profiles WHERE profile = 'default';";
    rc = sqlite3_prepare_v2(db, sql_select4, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
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



int get_weights(double weights[100], bool show_modern_planets) {
    

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
    double ant_orbis = 1.5;  // Orbe padrão se o banco falhar
    sqlite3 *db = open_database();
    if (db) {
        sqlite3_stmt *stmt;
        const char *sql_select_p_orbis = "SELECT antissia_orb FROM profiles WHERE profile = 'default';";
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
        fprintf(stderr, "Failed to open database in get_house_system_name\n");
        close_database(db);
        return 0;
    }

    const char *sql_select2 = "SELECT name FROM house_system WHERE id = ?;";
    rc = sqlite3_prepare_v2(db, sql_select2, -1, &stmt, NULL);
    
    char id[10] = "";
    snprintf(id, sizeof(id), "%c", house_system_id);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
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
    sqlite3_stmt *stmt_ram = NULL;
    sqlite3_stmt *stmt_disc = NULL;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in update_triplicity_rulers\n");
        return 0;
    }

    // 1. Conecta o arquivo físico do disco dinamicamente usando a política do .env
    char *sql_attach = sqlite3_mprintf("ATTACH DATABASE %Q AS disco;", DB_PATH);
    sqlite3_exec(db, sql_attach, NULL, NULL, NULL);
    sqlite3_free(sql_attach); 

    // CORREÇÃO: Definidas as queries corretas e adicionado "disco." no escopo do arquivo físico
    const char *sql_update_ram  = "UPDATE main.elements SET (ruler1, ruler2, ruler3) = (SELECT ruler1, ruler2, ruler3 FROM main.triplicity_rulers WHERE system = ? AND element = ?) WHERE id = ?;";
    const char *sql_update_disc = "UPDATE disco.elements SET (ruler1, ruler2, ruler3) = (SELECT ruler1, ruler2, ruler3 FROM main.triplicity_rulers WHERE system = ? AND element = ?) WHERE id = ?;";

    // 2. Prepara os dois statements APENAS UMA VEZ antes do laço dos elementos
    if (sqlite3_prepare_v2(db, sql_update_ram, -1, &stmt_ram, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db, sql_update_disc, -1, &stmt_disc, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statements: %s\n", sqlite3_errmsg(db));
        if (stmt_ram) sqlite3_finalize(stmt_ram);
        if (stmt_disc) sqlite3_finalize(stmt_disc);
        sqlite3_exec(db, "DETACH DATABASE disco;", NULL, NULL, NULL);
        return 0;
    }

    int sucesso = 1;
    sqlite3_stmt *statements[2] = {stmt_ram, stmt_disc};

    // 3. O TRUQUE: Um único laço que gerencia dinamicamente os 4 Elementos (1=Fire, 2=Water, 3=Air, 4=Earth)
    for (int elemento = 1; elemento <= 4; elemento++) {
        
        // Passa pelos statements populando e executando um por um
        for (int i = 0; i < 2; i++) {
            if (!statements[i]) continue;

            // Limpa os binds anteriores para permitir que novos valores sejam injetados com segurança
            sqlite3_reset(statements[i]);

            // Vincula os parâmetros corretos para o elemento da rodada atual
            sqlite3_bind_int(statements[i], 1, triplicity_system);
            sqlite3_bind_int(statements[i], 2, elemento);
            sqlite3_bind_int(statements[i], 3, elemento);

            rc = sqlite3_step(statements[i]);
            if (rc != SQLITE_DONE) {
                fprintf(stderr, "Failed saving triplicity rulers for element %d (index %d): %s\n", elemento, i, sqlite3_errmsg(db));
                sucesso = 0;
            }
        }
    }

    // 4. Finaliza os statements uma única vez de forma segura
    sqlite3_finalize(stmt_ram);
    sqlite3_finalize(stmt_disc);

    // 5. CORREÇÃO: Desconecta o arquivo físico liberando o arquivo em disco
    sqlite3_exec(db, "DETACH DATABASE disco;", NULL, NULL, NULL);

    close_database(db);
    return sucesso;
}





int update_settings(ChartOptions options) {
    sqlite3 *db;
    sqlite3_stmt *stmt_ram = NULL;
    sqlite3_stmt *stmt_disc = NULL;
    int rc;

    db = open_database();
    if (!db) {
        fprintf(stderr, "Failed to open database in update_settings\n");
        return 0;
    }

    char *sql_attach = sqlite3_mprintf("ATTACH DATABASE %Q AS disco;", DB_PATH);
    sqlite3_exec(db, sql_attach, NULL, NULL, NULL);
    sqlite3_free(sql_attach); // Libera o ponteiro gerado pelo mprintf

    const char *sql_update      = "UPDATE main.profiles SET dark_mode = ?, house_system = ?, triplicity_system = ?, terms_system = ?, modern_planets_rulling = ?, show_modern_planets = ?, gender = ?, language = ?, time_key = ?, antiscia_as_promissor = ?, zod_pd_with_lat = ?, pd_arc_calc = ? WHERE profile = ?;";
    const char *sql_update_disc = "UPDATE disco.profiles SET dark_mode = ?, house_system = ?, triplicity_system = ?, terms_system = ?, modern_planets_rulling = ?, show_modern_planets = ?, gender = ?, language = ?, time_key = ?, antiscia_as_promissor = ?, zod_pd_with_lat = ?, pd_arc_calc = ? WHERE profile = ?;";

    if (sqlite3_prepare_v2(db, sql_update, -1, &stmt_ram, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(db, sql_update_disc, -1, &stmt_disc, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statements: %s\n", sqlite3_errmsg(db));
        if (stmt_ram) sqlite3_finalize(stmt_ram);
        if (stmt_disc) sqlite3_finalize(stmt_disc);
        sqlite3_exec(db, "DETACH DATABASE disco;", NULL, NULL, NULL);
        return 0;
    }

    char house_system[10];
    snprintf(house_system, sizeof(house_system), "%c", options.house_system);

    int sucesso = 1;
    sqlite3_stmt *statements[2] = {stmt_ram, stmt_disc};
    
    for (int i = 0; i < 2; i++) {        
        if (!statements[i]) continue;

        sqlite3_bind_int(statements[i], 1, options.dark_mode);    
        sqlite3_bind_text(statements[i], 2, house_system, -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[i], 3, options.triplicity_system);
        sqlite3_bind_int(statements[i], 4, options.terms_system);
        sqlite3_bind_int(statements[i], 5, options.modern_planets_rulling);
        sqlite3_bind_int(statements[i], 6, options.show_modern_planets);
        sqlite3_bind_int(statements[i], 7, options.gender);
        sqlite3_bind_text(statements[i], 8, options.language, -1, SQLITE_STATIC);
        sqlite3_bind_int(statements[i], 9, options.time_key);
        sqlite3_bind_int(statements[i], 10, options.ant_prom);
        sqlite3_bind_int(statements[i], 11, options.zod_pd_with_lat);
        sqlite3_bind_int(statements[i], 12, options.pd_arc_calc);
        sqlite3_bind_text(statements[i], 13, "default", -1, SQLITE_STATIC);

        rc = sqlite3_step(statements[i]);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Failed saving profile options (index %d): %s\n", i, sqlite3_errmsg(db));
            sucesso = 0;
        }
    }

    sqlite3_finalize(stmt_ram);
    sqlite3_finalize(stmt_disc);

    // 3. Desconecta o arquivo físico liberando o arquivo em disco
    sqlite3_exec(db, "DETACH DATABASE disco;", NULL, NULL, NULL);

    close_database(db);
    return sucesso;
}

