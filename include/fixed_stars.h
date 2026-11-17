#ifndef FIXED_STARS_H
#define FIXED_STARS_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "planet_table.h"



typedef enum {
    HELIACAL_VISIBLE = 0,      // Estrela visível no céu noturno/crepuscular
    HELIACAL_HIDDEN = 1,           // Oculta sob os raios do Sol (combusta)
    HELIACAL_CIRCUMPOLAR = 2,      // Nunca se põe nesta latitude (sempre visível)
    HELIACAL_UNKNOWN = 3          // Erro ou estrela nunca visível nesta latitude
} HeliacalState;

typedef struct {
    HeliacalState state;
    double last_rising_tjd;    // Data juliana do Nascer Helíaco mais recente
    double last_setting_tjd;   // Data juliana do Pôr Helíaco mais recente
    double next_rising_tjd;    // Próximo Nascer Helíaco após o nascimento
} HeliacalInfo;

typedef enum {
    CAT_BEHENIAN   = 1 << 0, // As 15 estrelas mágicas/medievais
    CAT_ROYAL      = 1 << 1, // As 4 Estrelas Reais da Pérsia
    CAT_PTOLEMAIC  = 1 << 2, // Estrelas clássicas (Ptolomeu / Anônimo 379)
    CAT_NEBULA     = 1 << 3  // Nebulosas e aglomerados (críticos para saúde/visão)
} StarCategory;

typedef struct {
    const char *object;
    const char *swe_name;
    const char *display_name;
    const char *bayer;
    const char *nature;
    double default_orb;
    unsigned int categories;
    const char *delineation; // Texto interpretativo tradicional
} TraditionalStar;


typedef struct {
    const TraditionalStar *meta; // Ponteiro para os dados estáticos (nome, natureza, orbe)
    double longitude;            // Longitude eclíptica tropical (0° a 360°)
    double latitude;             // Latitude eclíptica
    double declination;          // Declinação equatorial (para Paralelos/Contraparalelos)
    double ra;
    double magnitude;            // Magnitude aparente retornada pela Swiss Ephemeris
    HeliacalState heliacal_state;
    bool heliacal_calculated;
} CalculatedStar;




typedef enum {
    ANGLE_RISING = 0,
    ANGLE_CULMINATING = 1,
    ANGLE_SETTING = 2,
    ANGLE_IC = 3,
    ANGLE_COUNT = 4
} ParanAngle;

typedef struct {
    CalculatedStar *stars;
    int star_index;          // Índice no array CalculatedStar
    int planet_id;           // ID do planeta (SE_SUN .. SE_SATURN)
    ParanAngle star_angle;   // Ângulo da estrela (ex: ANGLE_RISING)
    ParanAngle planet_angle; // Ângulo do planeta (ex: ANGLE_CULMINATING)
    double time_diff_mins;   // Diferença em minutos (orbe temporal)
} ParanResult;




typedef struct {
    CalculatedStar *stars;
    const char *object;
    const char *planet_name;     // Nome vindo de plots[i].object
    const char *planet_angle;    // "Rising", "Setting", "Culminating", "Anti-Culm."
    int star_index;
    const char *star_name;       // Nome da estrela fixa
    const char *star_nature;     // Temperamento (ex: "Mar/Jup")
    const char *star_angle;      // "Rising", "Setting", "Culminating", "Anti-Culm."
    int angle_planet;      
    double star_time_hours;      // Horário UT em horas decimais (0..24) para exibir local
    double diff_minutes;         // Orbe em minutos
    int planet_order;
    int brady_phase;     // 0: Rising, 1: Culminating, 2: Setting, 3: Anti-Culmination
} ParanEntry;



int32_t swe_heliacal_ut(
    double tjdstart_ut,   // Data inicial da busca (ex: tjd_ut - 365.25)
    double *geopos,       // {geo_lon, geo_lat, geo_alt}
    double *datm,         // Condições atmosféricas: {pressão_mbar, temp_C, umidade_%, visibilidade_km}
    double *dobs,         // Parâmetros do observador: {idade, acuidade_visual, ...} (passar zeros usa padrão)
    char *ObjectName,     // Nome da estrela (ex: "Sirius")
    int32_t TypeEvent,      // SE_HELIACAL_RISING, SE_HELIACAL_SETTING, SE_MORNING_LAST, SE_EVENING_FIRST
    int32_t helflag,        // SE_HELFLAG_AV (usa valores padrão da biblioteca)
    double *dret,         // Array de saída double dret[50]: dret[0] = TJD do início da visibilidade/evento
    char *serr            // Buffer de erro
);


typedef struct {
    const TraditionalStar *meta;
    double longitude;
    double magnitude;
    HeliacalInfo info;
    double days_since_rising; // Dias decorridos desde o último Nascer Helíaco até o nascimento
} HeliacalRow;



extern const TraditionalStar TRADITIONAL_STARS[];
extern const size_t TRADITIONAL_STARS_COUNT;

int calculate_traditional_stars(double tjd_ut, unsigned int active_categories, int extra_iflag,  CalculatedStar *results, char *err_log);
void display_parans_table(ParanEntry *parans, int paran_count, CalculatedStar *stars, double tz_offset, double max_orb_minutes);
int compute_parans(double tjd_ut, double geo_lon, double geo_lat, double geo_alt, int iflag, int *planets, int planet_count, CalculatedStar *stars, int star_count, double max_orb_minutes, ParanEntry *out_list, int max_entries);
HeliacalInfo get_star_heliacal_info(double tjd_ut, double geo_lon, double geo_lat, double geo_alt, const CalculatedStar *star);
void display_heliacal_phases_table(double tjd_ut, double geo_lon, double geo_lat, double geo_alt, double tz_offset, const CalculatedStar *stars, int star_count);

void abrir_janela_interpretacao_estrelas(
    const PlotObject *plots,
    int plot_count,
    CalculatedStar *stars,
    int star_count,
    const bool star_active[],
    const ParanEntry *parans,
    int paran_count,
    double lon_hyleg_natal,
    const char *nome_hyleg_natal,
    const char *glifo_hyleg_natal,
    double tjd_ut,
    double lon,
    double lat,
    double elev,
    bool is_solar_return,            // <-- false no Mapa Radical | true na Revolução Solar
    const bool radix_star_active[]
);
#endif // FIXED_STARS_H
