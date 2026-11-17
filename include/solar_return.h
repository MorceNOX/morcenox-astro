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

#ifndef SOLAR_RETURN_H
#define SOLAR_RETURN_H



typedef struct {
    const char *object;           // Glifo do planeta ou ângulo ("AC", "MC", glifo planetário)
    const char *object_name;      // Nome por extenso ("Ascendant", "Moon", etc.)
    const TraditionalStar *star;  // Ponteiro para os metadados da estrela (nome, natureza)
    double star_longitude;        // Longitude tropical da estrela no ano da Revolução
    double orb_deg;               // Orbe exata da conjunção (em graus)
    bool is_natal_active;         // true se a estrela já tinha conjunção/Paran no mapa natal
    HeliacalState natal_heliacal; // Estado helíaco natal (Visible, Under Beams, Circumpolar)
} SRFixedStarHit;


int compute_sr_fixed_stars(
    const double sr_angles[4],        // Array com {SR_ASC, SR_MC, SR_DSC, SR_IC}
    const PlotObject *sr_plots,       // Planetas da Revolução Solar
    int sr_plot_count,
    CalculatedStar *sr_stars,
    int star_count,
    const bool natal_star_active[],   // Array booleano indicando quais das 51 estrelas são ativas no natal
    SRFixedStarHit *out_hits,
    CalculatedStar *natal_stars,
    double tjd_ut_natal, 
    double lon_natal, 
    double lat_natal, 
    double elev_natal,
    int max_hits
);
void display_sr_fixed_stars_table(SRFixedStarHit *hits, int hit_count);

void processar_confronto_natal_revolucao(
    int qtd_almuten_rev,
    int *id_almuten_rev,               
    double *longitude_almuten_rev,     
    double *latitude_almuten_rev,      
    int *dignidade_natal,           
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
    double asc_natal,                 /* ADICIONADO: ASC calculado para a cidade do aniversário */
    double *cusps_rev,
    double *cups_natal
);           


struct tm obter_tempo_local_revolucao(double jd_revolucao_ut, double fuso_horario_destino, struct tm *t_local);
double calcular_julian_day_retorno_solar(double jd_nascimento, int idade_selecionada, double sol_natal_exibido);
double calc_julian_day_retorno_solar(double jd_nascimento, int idade_selecionada);
int disparar_revolucao_solar(double julian_day, char *chart_name, double *cusps_natal, bool mapa_diurno, double lat_natal, double armc, PlanetDignities *dig, char *nome_anareta_natal, char *nome_s8_natal, int tipo_h_natal, int idx_hyleg_natal, double *longitudes_natal, int *strength_planets, ChartObject *obj_natal, int num_objects, CalculatedStar *natal_stars, int total_natal_stars, ParanEntry *natal_parans, int total_natal_parans, AspectStarMatrix *natal_star_matrix, DeclStarMatrix *natal_decl_star_matrix, double lon_natal, double elev_natal);

void abrir_janela_confronto_natal_revolucao(
    int qtd_almuten_rev,
    int *id_almuten_rev, 
    int *pontuacao_dignidade_natal, 
    int *casa_natal_transitada, 
    int id_senhor_profeccao, 
    int id_senhor_firdaria, 
    int id_senhor_subfirdaria,
    int casa_natal_do_asc,
    int *aproveitamento_almuten,
    int casa_rev_do_asc_natal);

void get_natal_houses_rev_planets(double jd_natal, double *rev_longitudes, double *rev_latitudes, double armc_natal, double lat_natal, char house_system, int *casas_planetas_natal_proj);
void get_hyleg_data(int tipo_h_natal, int idx_hyleg_natal, double *longitudes_natal, double *lon_hyleg_radix_real, char *nome_hyleg_texto, char *glifo_hyleg_texto);
void process_revolution_transits(double jd_natal, double *rev_longitudes, double *rev_latitudes, double armc_natal, double lat_natal, char house_system, int tipo_h_natal, int idx_hyleg_natal, double *longitudes_natal, double *cusps_natal, ChartObject *obj_natal, int num_natal_objects);
void abrir_janela_transitos_revolucao(
    double *longitudes_rev, 
    double *longitudes_natal, 
    int *casas_planetas_rev,
    double lon_hyleg_natal,       /* ADICIONADO: Longitude do Hyleg do Radix */
    char *nome_hyleg_natal,       /* ADICIONADO: Nome do Hyleg (ex: "Jupiter", "Ascendant") */
    char *glifo_hyleg_natal,
    ArabicPartCalculada *lista,
    int qtd_partes);

#endif