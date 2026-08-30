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

#define _XOPEN_SOURCE_EXTENDED 1
#define NCURSES_WIDECHAR 1
#include "swephexp.h"
#include "sweph.h"

#include <ncursesw/curses.h>
#include <math.h>
#include <locale.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <sqlite3.h>
#include <ctype.h>
#include "number_helper.h"
#include "db-utils.h"
#include "helper.h"
#include "aspects.h"
#include "planet_table.h"
#include "selections.h"
#include <time.h>
#include "var.h"
#include "arabic_parts.h"
#include "draw-chart.h"
#include "temperament.h"
#include "firdaria.h"
#include "directions.h"
#include "almuten.h"
#include "hyleg.h"
#include "profections.h"
#include "aspects.h"
#include "mind.h"
#include "solar_return.h"
#include "motivation.h"

#ifndef SE_KEEP_GREG_CAL
#define SE_KEEP_GREG_CAL 2  /* 0 = Juliano, 1 = Gregoriano, 2 = Misto automático */
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define PH_SOL 0
#define PH_VENUS 1
#define PH_MERCURY 2
#define PH_LUNA 3
#define PH_SATURN 4
#define PH_JUPITER 5
#define PH_MARS 6

char *conjunction_str = "☌";
char *sextile_str = "⚹";
char *square_str = "□";
char *trine_str = "△";
char *opposition_str = "☍";

int hour_regents[7][24] = {
    {0,1,2,3,4,5,6,0,1,2,3,4,5,6,0,1,2,3,4,5,6,0,1,2},
    {3,4,5,6,0,1,2,3,4,5,6,0,1,2,3,4,5,6,0,1,2,3,4,5},
    {6,0,1,2,3,4,5,6,0,1,2,3,4,5,6,0,1,2,3,4,5,6,0,1},
    {2,3,4,5,6,0,1,2,3,4,5,6,0,1,2,3,4,5,6,0,1,2,3,4},
    {5,6,0,1,2,3,4,5,6,0,1,2,3,4,5,6,0,1,2,3,4,5,6,0},
    {1,2,3,4,5,6,0,1,2,3,4,5,6,0,1,2,3,4,5,6,0,1,2,3},
    {4,5,6,0,1,2,3,4,5,6,0,1,2,3,4,5,6,0,1,2,3,4,5,6}
};

int decans_to_print[36] = {
    6, 5, 4, 3, 2, 1,
    0, 6, 5, 4, 3, 2,
    1, 0, 6, 5, 4, 3,
    2, 1, 0, 6, 5, 4,
    3, 2, 1, 0, 6, 5,
    4, 3, 2, 1, 0, 6
};

#define SOL     0
#define VENUS   1
#define MERCURIO 2
#define LUA     3
#define SATURNO 4
#define JUPITER 5
#define MARTE   6

#define A_NENHUM   0
#define A_PLANETA  1

#define OBJ_PLANET 1
#define OBJ_ANGLE 2
#define OBJ_POINT 3
#define OBJ_ARABIC_PART 4
#define OBJ_ZODIAC_DEGREE 5
#define OBJ_CUSP 6
#define OBJ_FIXED_STAR 7
#define OBJ_DISPOSITOR 8




Termo tabela_termos_egipcios[12][5] = {
    
    {{6, JUPITER}, {12, VENUS}, {20, MERCURIO}, {25, MARTE}, {30, SATURNO}}, // Áries (0)   
    {{8, VENUS}, {14, MERCURIO}, {22, JUPITER}, {27, SATURNO}, {30, MARTE}}, // Touro (1)   
    {{7, MERCURIO}, {14, JUPITER}, {21, VENUS}, {28, SATURNO}, {30, MARTE}}, // Gêmeos (2)   
    {{7, MARTE}, {13, VENUS}, {19, MERCURIO}, {26, JUPITER}, {30, SATURNO}}, // Câncer (3)   
    {{6, JUPITER}, {13, VENUS}, {19, SATURNO}, {25, MERCURIO}, {30, MARTE}}, // Leão (4)   
    {{7, MERCURIO}, {17, VENUS}, {21, JUPITER}, {28, MARTE}, {30, SATURNO}}, // Virgem (5)   
    {{6, SATURNO}, {14, MERCURIO}, {21, JUPITER}, {28, VENUS}, {30, MARTE}}, // Libra (6)   
    {{7, MARTE}, {11, VENUS}, {19, MERCURIO}, {24, JUPITER}, {30, SATURNO}}, // Escorpião (7)   
    {{12, JUPITER}, {17, VENUS}, {21, MERCURIO}, {26, SATURNO}, {30, MARTE}}, // Sagitário (8)   
    {{7, MERCURIO}, {14, JUPITER}, {22, VENUS}, {26, SATURNO}, {30, MARTE}}, // Capricórnio (9)   
    {{7, MERCURIO}, {13, VENUS}, {20, JUPITER}, {25, MARTE}, {30, SATURNO}}, // Aquário (10)   
    {{12, VENUS}, {16, JUPITER}, {19, MERCURIO}, {28, MARTE}, {30, SATURNO}} // Peixes (11)
};


Termo tabela_termos_ptolomeu[12][5] = {
    {{6, JUPITER}, {14, VENUS}, {21, MERCURIO}, {26, MARTE}, {30, SATURNO}}, // Áries (0)
    {{8, VENUS}, {14, MERCURIO}, {22, JUPITER}, {27, SATURNO}, {30, MARTE}}, // Touro (1)
    {{7, MERCURIO}, {13, JUPITER}, {20, VENUS}, {26, MARTE}, {30, SATURNO}}, // Gêmeos (2)
    {{6, MARTE}, {13, JUPITER}, {20, MERCURIO}, {27, VENUS}, {30, SATURNO}}, // Câncer (3)
    {{6, JUPITER}, {11, VENUS}, {18, SATURNO}, {24, MERCURIO}, {30, MARTE}}, // Leão (4)
    {{7, MERCURIO}, {13, VENUS}, {18, JUPITER}, {24, MARTE}, {30, SATURNO}}, // Virgem (5)
    {{6, SATURNO}, {11, VENUS}, {19, JUPITER}, {24, MERCURIO}, {30, MARTE}}, // Libra (6)
    {{6, MARTE}, {13, JUPITER}, {21, VENUS}, {27, MERCURIO}, {30, SATURNO}}, // Escorpião (7)
    {{8, JUPITER}, {14, VENUS}, {19, MERCURIO}, {25, SATURNO}, {30, MARTE}}, // Sagitário (8)
    {{6, MERCURIO}, {12, JUPITER}, {19, VENUS}, {26, SATURNO}, {30, MARTE}}, // Capricórnio (9)
    {{6, SATURNO}, {12, MERCURIO}, {20, VENUS}, {25, JUPITER}, {30, MARTE}}, // Aquário (10)
    {{6, VENUS}, {12, JUPITER}, {19, MERCURIO}, {26, MARTE}, {30, SATURNO}}  // Peixes (11)
};


const char *sagittarius[] = {
    "▗▚ ",
    "▘▌▘",
    "▄▙▖",
    " ▌ "
};

const char *taurus[] = {
    "▝▖  ▞",
    " ▞▀▀▖",
    "▐   ▐",
    " ▚▄▄▘"
};


const char *aries[] = {
    "      ",
    "▞▀▖▞▀▖",
    "  ▐   ",
    "  ▐   "
};

const char *gemini[] = {
    "▄▄▄▄▖",
    " ▌ ▌ ",
    " ▌ ▌ ",
    "▄▙▄▙▖"
};

const char *leo[] = {
    " ▞▀▀▖",
    "▞▚  ▞",
    "▚▞ ▐ ",
    "    ▚"
};


const char *pisces[] = {
    "▝▖  ▞",
    "▗▟▄▟▄",
    " ▞ ▝▖",
    "▝   ▝"
};


const char *aquarius[] = {
    "     ",
    "▞▚▞▚▞",
    "     ",
    "▞▚▞▚▞"
};

const char *cancer[] = {
    "     ",
    "▞▜▀▀ ",
    "▚▞ ▞▚",
    " ▄▄▙▞"
};

const char *libra[] = {
    "    ",
    " ▞▚ ",
    "▀▀▀▀",
    "▀▀▀▀"
};

const char *scorpio[] = {
    "    ",
    "▛▚▀▖",
    "▌▐ ▌",
    "▘▝ ▚"
};

const char *virgo[] = {
    "     ",
    "▛▚▀▞▚",
    "▌▐ ▌▞",
    "▘▝ ▜ "
};

const char *capricorn[] = {
    "     ",
    "▚▗▜▞▚",
    " ▘ ▚▞",
    "   ▞ "
};




const char *sol_ascii[] = {
    "      ",
    " ▞▀▀▖ ",
    "▐ ▗ ▐ ",
    "▝▖  ▞ ",
    " ▝▀▀  ",
    "      "
};

const char *lua_ascii[] = {
    "  ▗ ",
    " ▞▌ ",
    "▐▐  ",
    " ▚▌ ",
    "  ▝ ",
    "    "
};

const char *mercury_ascii[] = {
    "▖ ▖ ",
    "▞▀▖ ",
    "▌ ▌ ",
    "▝▛  ",
    "▄▙▖ ",
    " ▌  "
};
   
const char *venus_ascii[] = {
    "    ",
    "▞▀▖ ",
    "▌ ▌ ",
    "▝▛  ",
    "▄▙▖ ",
    " ▌  "
};

const char *jupiter_ascii[] = {
    "      ",
    "▞▀▖   ",
    " ▗▘   ",
    "▗▘ ▌  ",
    "▀▀▀▛▀ ",
    "   ▘  "
};

const char *saturno_ascii[] = {
    "     ",
    " ▌   ",
    "▀▛▘  ",
    " ▀▀▖ ",
    "  ▗▘ ",
    "  ▘  "
};

const char *marte_ascii[] = {
    "▗▚  ",
    "▘▌▘ ",
    "▞▀▖ ",
    "▌ ▌ ",
    "▝▀  ",
    "    "
};

const char *pluto_ascii[] = {
    " ▗▄   ",
    "▖▚▄▘▖ ",
    "▝▄▄▞  ",
    " ▄▙▖  ",
    "  ▌   ",
    "      "
};

const char *Pluto_ascii[] = {
    "    ",
    "▛▀▖ ",
    "▙▄▘ ",
    "▌   ",
    "▀▀▘ ",
    "    "
};

const char *netuno_ascii[] = {
    "      ",
    "▌ ▌ ▌ ",
    "▝▄▙▞  ",
    " ▄▙▖  ",
    "  ▌   ",
    "      "
};
    
const char *urano_ascii[] = {
    "▖ ▗  ▖ ",
    "▐▄▟▄▟  ",
    "▞▗▟▄▝▖ ",
    " ▌  ▌  ",
    " ▝▀▀   ",
    "       "
};

const char *cauda_draconis_ascii[] = {
    "      ",    
    "▞▚ ▞▚ ",
    " ▞ ▝▖ ",
    "▐   ▐ ",
    " ▚▄▄▘ ",
    "      "
};

const char *caput_draconis_ascii[] = {
    "      ",
    " ▞▀▀▖ ",
    "▝▖  ▞ ",
    " ▐ ▞  ",
    "▚▞ ▚▞ ",
    "      "
};


const char *fortuna_ascii[] = {
    "       ",
    " ▞▛▀▛▖ ",
    "▐ ▝▞ ▐ ",
    "▝▖▞▝▖▞ ",
    " ▝▀▀▀  ",
    "       "
};


const char * const * planet_ascii[] = { 
    sol_ascii, 
    lua_ascii, 
    mercury_ascii, 
    venus_ascii, 
    marte_ascii, 
    jupiter_ascii, 
    saturno_ascii, 
    urano_ascii, 
    netuno_ascii, 
    pluto_ascii, 
    caput_draconis_ascii, 
    cauda_draconis_ascii,
    fortuna_ascii
};


const char **get_planet_ascii(int planet_id) {
    return (const char **)planet_ascii[planet_id - 1];
}


const char **get_planet_ascii_by_name(char *planet_name) {
    
    if (strcmp(planet_name, _("Sun")) == 0 || strcmp(planet_name, _("Sol")) == 0) {
        return (const char **)planet_ascii[0];
    }
    else if (strcmp(planet_name, _("Luna")) == 0 || strcmp(planet_name, _("Moon")) == 0) {
        return (const char **)planet_ascii[1];
    }
    else if (strcmp(planet_name, _("Mercury")) == 0) {
        return (const char **)planet_ascii[2];
    }
    else if (strcmp(planet_name, _("Venus")) == 0) {
        return (const char **)planet_ascii[3];
    }
    else if (strcmp(planet_name, _("Mars")) == 0) {
        return (const char **)planet_ascii[4];
    }
    else if (strcmp(planet_name, _("Jupiter")) == 0) {
        return (const char **)planet_ascii[5];
    }
    else if (strcmp(planet_name, _("Saturn")) == 0) {
        return (const char **)planet_ascii[6];
    }
    else if (strcmp(planet_name, _("Uranus")) == 0) {
        return (const char **)planet_ascii[7];
    }
    else if (strcmp(planet_name, _("Neptune")) == 0) {
        return (const char **)planet_ascii[8];
    }
    else if (strcmp(planet_name, _("Pluto")) == 0) {
        return (const char **)planet_ascii[9];
    }
    else if (strcmp(planet_name, _("North Node")) == 0 || strcmp(planet_name, _("Caput Draconis")) == 0) {
        return (const char **)planet_ascii[10];
    }
    else if (strcmp(planet_name, _("South Node")) == 0 || strcmp(planet_name, _("Cauda Draconis")) == 0) {
        return (const char **)planet_ascii[11];
    }
    else if (strcmp(planet_name, _("Part of Fortune")) == 0) {
        return (const char **)planet_ascii[12];
    }

    return NULL;   
}


const char **get_planet_ascii_by_gliph(char *planet) {
    
    if (strcmp(planet, "☉") == 0) {
        return (const char **)planet_ascii[0];
    }
    else if (strcmp(planet, "☽") == 0) {
        return (const char **)planet_ascii[1];
    }
    else if (strcmp(planet, "☿") == 0) {
        return (const char **)planet_ascii[2];
    }
    else if (strcmp(planet, "♀") == 0) {
        return (const char **)planet_ascii[3];
    }
    else if (strcmp(planet, "♂") == 0) {
        return (const char **)planet_ascii[4];
    }
    else if (strcmp(planet, "♃") == 0) {
        return (const char **)planet_ascii[5];
    }
    else if (strcmp(planet, "♄") == 0) {
        return (const char **)planet_ascii[6];
    }
    else if (strcmp(planet, "♅") == 0) {
        return (const char **)planet_ascii[7];
    }
    else if (strcmp(planet, "♆") == 0) {
        return (const char **)planet_ascii[8];
    }
    else if (strcmp(planet, "⯓") == 0) {
        return (const char **)planet_ascii[9];
    }
    else if (strcmp(planet, "☊") == 0) {
        return (const char **)planet_ascii[10];
    }
    else if (strcmp(planet, "☋") == 0) {
        return (const char **)planet_ascii[11];
    }
    else if (strcmp(planet, "🝴") == 0) {
        return (const char **)planet_ascii[12];
    }

    return NULL;
    
}



void get_terms_longitude_to_print(Termo tabela_termos[12][5], Termo termos_lon[12][5]) {

    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 5; j++) {
            termos_lon[i][j].regente = tabela_termos[i][j].regente;
            if (j == 0) {
                termos_lon[i][j].grau_limite = i * 30 + 1;
            }
            else {
                termos_lon[i][j].grau_limite = (i * 30) + (tabela_termos[i][j - 1].grau_limite);
            }
        }
    }
}



int obter_idade_padrao_mapa() {
    // 1. Captura o tempo atual do sistema
    time_t tempo_bruto = time(NULL);
    struct tm *tempo_local = localtime(&tempo_bruto);
    
    // 2. Extrai o ano atual (Nota: O SQLite/C armazena os anos desde 1900, 
    // então tempo_local->tm_year retorna (Ano Atual - 1900). Somamos 1900 para ter o ano cheio)
    int ano_atual = tempo_local->tm_year + 1900;
    
    // 3. Calcula a idade base subtraindo do ano do mapa (YY)
    int idade_calculada = ano_atual - YY;
    
    // Validação de segurança simples (caso o mês/dia atual ainda não tenha chegado ao aniversário)
    // Se o mês atual for menor que o mês do mapa, ou se for o mesmo mês mas o dia atual for menor:
    int mes_atual = tempo_local->tm_mon + 1; // tm_mon vai de 0 a 11
    int dia_atual = tempo_local->tm_mday;
    
    if (mes_atual < MM || (mes_atual == MM && dia_atual < DD)) {
        idade_calculada--; // Ainda não fez aniversário no ano corrente
    }
    
    // Proteção contra mapas futuros ou idades negativas
    if (idade_calculada < 0) {
        idade_calculada = 0;
    }
    
    return idade_calculada;
}



double obter_idade_padrao_mapa_double() {
    /* 1. Captura o tempo atual do sistema */
    time_t tempo_bruto = time(NULL);
    struct tm *tempo_local = localtime(&tempo_bruto);
    
    int ano_atual = tempo_local->tm_year + 1900;
    int mes_atual = tempo_local->tm_mon + 1; /* tm_mon vai de 0 a 11 */
    int dia_atual = tempo_local->tm_mday;
    
    /* 2. Calcula a idade base inteira */
    int idade_inteira = ano_atual - YY;
    
    /* 3. Validação clássica para saber se o aniversário deste ano já passou */
    if (mes_atual < MM || (mes_atual == MM && dia_atual < DD)) {
        idade_inteira--; 
    }
    
    /* Proteção contra mapas futuros */
    if (idade_inteira < 0) {
        return 0.0;
    }

    /* 4. CÁLCULO CIENTÍFICO DA FRAÇÃO DECIMAL DO ANO
          Descobrimos quantos meses e dias se passaram desde o último aniversário. */
    double meses_decorridos = (double)(mes_atual - MM);
    double dias_decorridos  = (double)(dia_atual - DD);

    /* Se o dia atual for menor que o dia de nascimento, ajustamos a fração dos meses */
    if (dias_decorridos < 0) {
        meses_decorridos -= 1.0;
        /* Adiciona os dias proporcionais baseados em um mês padrão de 30 dias */
        dias_decorridos += 30.0; 
    }

    /* Se os meses decorridos ficarem negativos, significa que o aniversário foi no ano passado */
    if (meses_decorridos < 0) {
        meses_decorridos += 12.0;
    }

    /* Converte o saldo de meses e dias em uma fração decimal pura de 1 ano completo */
    double fracao_do_ano = (meses_decorridos / 12.0) + (dias_decorridos / 365.25);

    /* 5. RETORNO CONSOLIDADO
          Idade Inteira + Fração exata vivida após o aniversário */
    return (double)idade_inteira + fracao_do_ano;
}



const char* obter_glifo_planeta_por_id(int id_planeta) {
    switch (id_planeta) {
        case 1:  return "☉";  // Sun (Sol)
        case 2:  return "☽";  // Luna (Lua)
        case 3:  return "☿";  // Mercury (Mercúrio)
        case 4:  return "♀";  // Venus (Vênus)
        case 5:  return "♂";  // Mars (Marte)
        case 6:  return "♃";  // Jupiter (Júpiter)
        case 7:  return "♄";  // Saturn (Saturno)
        case 8:  return "♅";  // Uranus (Urano)
        case 9:  return "♆";  // Neptune (Netuno)
        case 10: return "⯓";  // Pluto (Plutão)
        case 11: return "☊";  // North Node (Nodo Norte)
        case 12: return "☋";  // South Node (Nodo Sul)
        default: return "?";
    }
}

const char* obter_nome_planeta_por_id(int id_planeta) {
    switch (id_planeta) {
        case 1:  return _("Sol");
        case 2:  return _("Luna");
        case 3:  return _("Mercury");
        case 4:  return _("Venus");
        case 5:  return _("Mars");
        case 6:  return _("Jupiter");
        case 7:  return _("Saturn");
        case 8:  return _("Uranus");
        case 9:  return _("Neptune");
        case 10: return _("Pluto");
        case 11: return _("North Node");
        case 12: return _("South Node");
        default: return _("?");
    }
}

int converter_codigo_planeta(int codigo_antigo) {
    switch (codigo_antigo) {
        case SOL:       return 1;  // Sol
        case LUA:       return 2;  // Luna
        case MERCURIO:  return 3;  // Mercury
        case VENUS:     return 4;  // Venus
        case MARTE:     return 5;  // Mars
        case JUPITER:   return 6;  // Jupiter
        case SATURNO:   return 7;  // Saturn
        default:        return -1; // Código inválido ou não mapeado
    }
}

// const char* get_house_roman(double longitude, double *cusps) {
//     // Array of Roman numerals mapped to house indices (0 to 11 maps to I to XII)
//     static const char* roman_houses[] = {
//         "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII"
//     };

//     for (int i = 1; i <= 12; i++) {
//         double current_cusp = cusps[i];
//         // The next cusp wraps around from 12 back to 1
//         double next_cusp = (i == 12) ? cusps[1] : cusps[i + 1];

//         if (current_cusp < next_cusp) {
//             // Normal case: house stays within the 0-360 boundaries
//             if (longitude >= current_cusp && longitude < next_cusp) {
//                 return roman_houses[i - 1];
//             }
//         } else {
//             // Wraparound case: house crosses over 360° / 0° Aries
//             if (longitude >= current_cusp || longitude < next_cusp) {
//                 return roman_houses[i - 1];
//             }
//         }
//     }
//     return "I"; // Fallback safety default
// }


const char* get_house_roman(double longitude, double *cusps) {
    static const char* roman_houses[] = {
        "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII"
    };

    // Uma tolerância infinitesimal para engolir imprecisões do double (1e-9 graus)
    const double EPSILON = 1e-9; 

    for (int i = 1; i <= 12; i++) {
        double current_cusp = cusps[i];
        double next_cusp = (i == 12) ? cusps[1] : cusps[i + 1];

        if (current_cusp < next_cusp) {
            // Caso Normal: adicionamos o EPSILON para garantir que limites exatos entrem na casa correta
            if (longitude >= (current_cusp - EPSILON) && longitude < (next_cusp - EPSILON)) {
                return roman_houses[i - 1];
            }
        } else {
            // Caso de cruzamento de 360°/0° Áries
            if (longitude >= (current_cusp - EPSILON) || longitude < (next_cusp - EPSILON)) {
                return roman_houses[i - 1];
            }
        }
    }

    // Se houver uma imprecisão bizarra na borda superior da Casa XII, 
    // o fallback mais seguro astrologicamente é a Casa XII, e não a Casa I.
    return "XII"; 
}



int get_hour_regent(int week_day, int planetary_hour) {
    return hour_regents[week_day][planetary_hour];
}



int get_term_ruler(double longitude_total) {
    // 1. Extrair o número do signo (0 = Áries, 1 = Touro...)
        int signo = (int)(longitude_total / 30.0);
        if (signo >= 12) signo = 11; // Proteção para 360.0°
        
        // 2. Extrair a posição exata em graus dentro do signo (0.0 a 29.99)
        double grau_no_signo = fmod(longitude_total, 30.0);
        
        // 3. Encontrar em qual termo o planeta caiu
        int regente_do_termo = -1;

                    
        if (terms_system == 1) {
            for (int t = 0; t < 5; t++) {
                // Como a tabela armazena onde o termo TERMINA, checamos se o planeta está antes desse limite
                if (grau_no_signo < (double)tabela_termos_egipcios[signo][t].grau_limite) {
                    regente_do_termo = tabela_termos_egipcios[signo][t].regente;
                    break; // Encontrou a fatia correta, interrompe o laço de busca
                }
            }
        }
        else {
            for (int t = 0; t < 5; t++) {
                // Como a tabela armazena onde o termo TERMINA, checamos se o planeta está antes desse limite
                if (grau_no_signo < (double)tabela_termos_ptolomeu[signo][t].grau_limite) {
                    regente_do_termo = tabela_termos_ptolomeu[signo][t].regente;
                    break; // Encontrou a fatia correta, interrompe o laço de busca
                }
            }
        }

        return regente_do_termo;
}


int get_decan(double longitude_total) {
    return (int)longitude_total / 10;
}

int get_decan_ruler(int ndec) {
    int decans[36] = {
        6, 0, 1, 2, 3, 4,
        5, 6, 0, 1, 2, 3,
        4, 5, 6, 0, 1, 2,
        3, 4, 5, 6, 0, 1,
        2, 3, 4, 5, 6, 0,
        1, 2, 3, 4, 5, 6
    };

    return decans[ndec];
}



int get_planetary_joy(int id_planet) {
    switch (id_planet) {
        case 1: return 9;  // Sol = casa 9
        case 2: return 3;  // Lua = casa 3
        case 3: return 1;  // Mercúrio = casa 1
        case 4: return 5;  // Vênus = casa 5
        case 5: return 6;  // Marte = casa 6
        case 6: return 11; // Júpiter = casa 11
        case 7: return 12; // Saturno = casa 12
        default: return 0;
    }
}

int get_sign_joy(int id_planet) {
    switch (id_planet) {
        case 1: return 5;  // Sol = Leo
        case 2: return 4;  // Lua = Cancer
        case 3: return 6;  // Mercúrio = Virgo
        case 4: return 2;  // Vênus = Touro5
        case 5: return 8;  // Marte = Scorpio
        case 6: return 9;  // Júpiter = Sagittarius
        case 7: return 11; // Saturno = Aquarius
        default: return 0;
    }
}


const char* get_moon_phase_name(double elongation) {
    if (elongation < 12.0) return _("🌑 New Moon"); // Nova
    else if (elongation < 89.99) return _("🌘 Waxing Crescent"); // Crescente
    else if (elongation < 102.0) return _("🌗 First Quarter"); // Quarto Crescente
    else if (elongation < 179.99) return _("🌖 Waxing Gibbous"); // Crescente Gibosa
    else if (elongation < 192.0) return _("🌕 Full Moon"); // Cheia
    else if (elongation < 269.99) return _("🌔 Waning Gibbous"); // Minguante Gibosa
    else if (elongation < 282.0) return _("🌓 Last Quarter"); // Quarto Minguante
    else if (elongation < 359.99) return _("🌒 Waning Crescent"); // Minguante
    else return "🌑 New Moon"; // Nova
}

const char* get_moon_phase_name_by_id(int id) {
    switch (id) {
        case 1: return _("🌑 New Moon");
        case 2: return _("🌘 Waxing Crescent");
        case 3: return _("🌗 First Quarter");
        case 4: return _("🌖 Waxing Gibbous");
        case 5: return _("🌕 Full Moon");
        case 6: return _("🌔 Waning Gibbous");
        case 7: return _("🌓 Last Quarter");
        case 8: return _("🌒 Waning Crescent");
        default: return _("🌑 New Moon");
    }

    return " ";    
}



int get_moon_phase_id(double elongation) {
    if (elongation < 12.0) return 1; // Nova
    else if (elongation < 89.99) return 2; // Crescente
    else if (elongation < 102.0) return 3; // Quarto Crescente
    else if (elongation < 179.99) return 4; // Crescente Gibosa
    else if (elongation < 192.0) return 5; // Cheia
    else if (elongation < 269.99) return 6; // Minguante Gibosa
    else if (elongation < 282.0) return 7; // Quarto Minguante
    else if (elongation < 359.99) return 8; // Minguante
    else return 1; // Nova
}


int get_moon_quarter(double elongation) {
    if (elongation <= 89.99) return 1; // Nova
    else if (elongation <= 179.99) return 2; // Crescente
    else if (elongation <= 269.99) return 3; // Cheia
    else if (elongation <= 359.99) return 4; // Minguante
    else return 1; // Nova
}


int get_moon_quarte_by_phase_id(int id) {
    switch (id) {
        case 1: return 1;
        case 2: return 1;
        case 3: return 2;
        case 4: return 2;
        case 5: return 3;
        case 6: return 3;
        case 7: return 4;
        case 8: return 4;
        default: return 1;
    }

    return 1;    
}


// Calculate phase based on Moon's ecliptic longitude difference
double get_moon_phase_by_longitude(double julian_day) {
    double earth_pos[6], moon_pos[6];
    
    // Get positions
    swe_calc(julian_day, SE_SUN, SEFLG_SPEED, earth_pos, 0);
    swe_calc(julian_day, SE_MOON, SEFLG_SPEED, moon_pos, 0);
    
    // Get ecliptic longitudes (position[0])
    double earth_lon = earth_pos[0];
    double moon_lon = moon_pos[0];
    
    // Calculate the angular difference
    double diff = moon_lon - earth_lon;
    
    // Normalize to 0-360 range
    while (diff < 0) diff += 360.0;
    while (diff >= 360.0) diff -= 360.0;
    
    return diff;
}


// Helper to get the Sun's altitude at a specific Julian Day
double get_sun_altitude(double tjd_ut, double lat, double lon, double elev) {
    double xx[6];          // Armazena os resultados de swe_calc
    double xequ[3];        // Armazena as coordenadas equatoriais (RA e DEC)
    double xaz[3];         // Armazena os resultados horizontais
    char serr[256];        // Buffer para mensagens de erro
    
    double geopos[3] = {lon, lat, elev};

    // 1. Converter UT para Ephemeris Time (Tempo Dinâmico Terrestre)
    double delta_t = swe_deltat(tjd_ut);
    double tjd_et = tjd_ut + delta_t;

    // Usaremos a flag básica para coordenadas equatoriais em relação ao ET
    int flag = SEFLG_SWIEPH | SEFLG_EQUATORIAL;

    // 2. Calcular a posição do Sol usando o tempo orbital exato (tjd_et)
    if (swe_calc(tjd_et, SE_SUN, flag, xx, serr) < 0) {
        printf("Erro no swe_calc: %s\n", serr);
        return -1;
    }

    xequ[0] = xx[0]; // Ascensão Reta (RA)
    xequ[1] = xx[1]; // Declinação (DEC)
    xequ[2] = xx[2]; // Distância

    // 3. Converter para o horizonte local
    // Importante: swe_azalt exige explicitamente o tempo em UT (tjd_ut) na entrada
    swe_azalt(tjd_ut, SE_EQU2HOR, geopos, 0, 0, xequ, xaz);
    
    // Retorna xaz[1] (altitude verdadeira). O algoritmo de bisseção se encarrega
    // de cruzar o horizonte visual usando o alvo de -0.833333 graus.
    return xaz[1];
}


double find_sun_event(double jd_start_midnight_ut, double lat, double lon, double elev, bool is_sunrise) {
    double target_alt = -50.0 / 60.0; // -0.8333333... exato da efeméride
    double step = 1.0 / 24.0;        // Passo de 1 hora
    
    // Varre as 24 horas do dia UT (de 0.0 a 1.0)
    for (double t = jd_start_midnight_ut; t < jd_start_midnight_ut + 1.0; t += step) {
        double alt_now = get_sun_altitude(t, lat, lon, elev);
        double alt_next = get_sun_altitude(t + step, lat, lon, elev);

        bool crossed = false;
        if (is_sunrise) {
            if (alt_now < target_alt && alt_next >= target_alt) crossed = true;
        } else {
            if (alt_now >= target_alt && alt_next < target_alt) crossed = true;
        }

        if (crossed) {
            // Refinamento por Bisseção Numérica Estrita
            double low = t;
            double high = t + step;
            
            for (int i = 0; i < 25; i++) {
                double mid = (low + high) / 2.0;
                double mid_alt = get_sun_altitude(mid, lat, lon, elev);
                
                // Se estamos no Sunrise, a altitude cresce com o tempo
                if (is_sunrise) {
                    if (mid_alt < target_alt) {
                        low = mid;
                    } else {
                        high = mid;
                    }
                } 
                // Se estamos no Sunset, a altitude decresce com o tempo
                else {
                    if (mid_alt > target_alt) {
                        low = mid; // Ainda não caiu o suficiente
                    } else {
                        high = mid; // Caiu demais, o ponto está para trás
                    }
                }
            }
            return (low + high) / 2.0; // Retorna o centro do intervalo refinado
        }
    }
    return -1.0; // Evento não ocorre (ex: regiões polares)
}


// Função auxiliar para normalizar valores entre 0 e 360 graus
double normalize360(double val) {
    val = fmod(val, 360.0);
    if (val < 0) val += 360.0;
    return val;
}


// Helper to normalize angles to [-180, 180]
double normalize_angle(double angle) {
    while (angle > 180.0) angle -= 360.0;
    while (angle < -180.0) angle += 360.0;
    return angle;
}


// Helper to get the difference between Sun and Moon longitudes
double get_sun_moon_diff(double jd) {
    double x2[6];
    char err[256];
    // We need the longitudes of both Sun and Moon
    swe_calc_ut(jd, SE_SUN, SEFLG_SPEED, x2, err);
    double sun_long = x2[0];
    
    swe_calc_ut(jd, SE_MOON, SEFLG_SPEED, x2, err);
    double moon_long = x2[0];
    
    return normalize_angle(sun_long - moon_long);
}


/**
* Finds the last occurrence of a specific event.
* @param start_jd The Julian Day to start searching backward from.
* @param is_opposition Set to true for 180 deg, false for 0 deg (conjunction).
* @return The Julian Day of the last event, or -1 if not found within search limit.
*/
double find_last_astrological_event(double start_jd, bool is_opposition) {
    double target = is_opposition ? 180.0 : 0.0;
    double jd = start_jd;
    double step = 1.0; // Search in 1-day increments
    int max_search_days = 60; // Limit search to avoid infinite loops
    
    double current_diff = get_sun_moon_diff(jd);
    double prev_diff = get_sun_moon_diff(jd - step);

    // Check if the target was crossed between (jd - step) and (jd)
    // We normalize the difference relative to the target
    double val_now = normalize_angle(current_diff - target);
    double val_prev = normalize_angle(prev_diff - target);

    // If the sign changed, the event happened in this window
    if (((val_now >= 0 && val_prev < 0) || (val_now < 0 && val_prev >= 0)) 
        && fabs(val_now - val_prev) < 180.0) {
        // Refinement: Bisection Method
        double low = jd - step;
        double high = jd;
        
        for (int i = 0; i < 25; i++) { // 25 iterations provides sub-second precision
            double mid = (low + high) / 2.0;
            double mid_diff = normalize_angle(get_sun_moon_diff(mid) - target);
            
            // Check if the sign change is between 'low' and 'mid'
            double low_diff = normalize_angle(get_sun_moon_diff(low) - target);
            
            if ((mid_diff >= 0 && low_diff >= 0) || (mid_diff < 0 && low_diff < 0)) {
                low = mid;
            } else {
                high = mid;
            }
        }
        return low;
    }

    // If not found in the first day, keep stepping back
    for (int i = 1; i < max_search_days; i++) {
        jd -= step;
        double next_diff = get_sun_moon_diff(jd);
        double next_prev = get_sun_moon_diff(jd - step);
        
        double v_now = normalize_angle(next_diff - target);
        double v_prev = normalize_angle(next_prev - target);

        if (((v_now >= 0 && v_prev < 0) || (v_now < 0 && v_prev >= 0))
        && fabs(v_now - v_prev) < 180.0) {
            // Found the window, now perform bisection (same logic as above)
            double low = jd - step;
            double high = jd;
            for (int b_iter = 0; b_iter < 25; b_iter++) {
                double mid = (low + high) / 2.0;
                double m_diff = normalize_angle(get_sun_moon_diff(mid) - target);
                double l_diff = normalize_angle(get_sun_moon_diff(low) - target);
                if ((m_diff >= 0 && l_diff >= 0) || (m_diff < 0 && l_diff < 0)) low = mid;
                else high = mid;
            }
            return low;
        }
    }

    return -1.0; // Event not found in recent history
}


float get_x_width(float r, float y_offset, float aspect_ratio) {
    float inner_val = (r * r) - (y_offset * y_offset);
    float x_width = sqrt(fmax(0, inner_val));
    return x_width * aspect_ratio;
}

void draw_circle_filled(int center_y, int center_x, float radius, 
                       float aspect_ratio, float current_scale, 
                        const wchar_t* character) {
    float r = radius * current_scale;
    
    for (int y_offset = (int)-r; y_offset <= (int)r; y_offset++) {
        int y = center_y + y_offset;
        if (y >= 0 && y < LINES) {
            float x_offset_max = get_x_width(r, (float)y_offset, aspect_ratio);
            int x_offset_limit = (int)x_offset_max;
            
            for (int x_offset = -x_offset_limit; x_offset <= x_offset_limit; x_offset++) {
                int x = center_x + x_offset;
                if (x >= 0 && x < COLS) {
                    mvaddwstr(y, x, character);
                }
            }
        }
    }
}

void draw_circle_outline(int center_y, int center_x, float radius, 
                         float aspect_ratio, float current_scale, 
                         const wchar_t* character) {
    float r = radius * current_scale;
    
    for (int y_offset = (int)-r; y_offset <= (int)r; y_offset++) {
        int y = center_y + y_offset;
        if (y >= 0 && y < LINES) {
            float x_offset_max = get_x_width(r, (float)y_offset, aspect_ratio);
            int x_offset_limit = (int)x_offset_max;
            
            int x_left = center_x - x_offset_limit;
            int x_right = center_x + x_offset_limit;
            
            // To prevent gaps, we draw the edge pixel AND the pixel immediately next to it.
            // This ensures that if the x-coordinate jumps by 2, the "thickness" covers the jump.
            for (int thickness = 0; thickness <= 1; thickness++) {
                // Check left edge
                int lx = x_left + thickness;
                if (lx >= 0 && lx < COLS) {
                    mvaddwstr(y, lx, character);
                }
                
                // Check right edge
                int rx = x_right - thickness;
                if (rx >= 0 && rx < COLS) {
                    mvaddwstr(y, rx, character);
                }
            }
        }
    }
}


// Helper function to draw circle points
void draw_circle_points(int center_y, int center_x, float radius, 
                       float aspect_ratio, float current_scale, 
                       const wchar_t* character) {
    int steps = 360;  // More precise circle
    float r = radius * current_scale;
    
    for (int i = 0; i < steps; i++) {
        float angle = i * PI / 180.0;
        int y = (int)(center_y + r * sin(angle));
        int x = (int)(center_x + aspect_ratio * r * cos(angle));
        
        if (y >= 0 && y < LINES && x >= 0 && x < COLS) {
            mvaddwstr(y, x, character);
        }
    }
}


/**
 * Desenha um círculo com suporte a animação de traçado em tempo real.
 * 
 * @param delay_ms Tempo de pausa em milisegundos a cada ponto (0 para instantâneo).
 */
void draw_circle_points_delay(int center_y, int center_x, float radius, 
                       float aspect_ratio, float current_scale, 
                       const wchar_t* character, int delay_ms, bool clockwise) {
    int steps = 360;  // Círculo preciso
    float r = radius * current_scale;
    
    for (int i = 0; i < steps; i++) {
        float angle = i * M_PI / 180.0; // Usando M_PI padrão ou seu PI definido

        float my = 0.0, mx = 0.0;
        if (clockwise) {
            my = sin(angle);
            mx = cos(angle);
        }
        else {
            mx = sin(angle);
            my = cos(angle);
        }
        int y = (int)(center_y + r * my);
        int x = (int)(center_x + aspect_ratio * r * mx);
        
        if (y >= 0 && y < LINES && x >= 0 && x < COLS) {
            mvaddwstr(y, x, character);
            
            // Se houver um delay configurado, força a renderização do ponto e pausa
            if (delay_ms > 0) {
                refresh();        // Força o ncurses a jogar o ponto atual na tela física
                napms(delay_ms);  // Pausa nativa do ncurses em milisegundos
            }
        }
    }
    
    // Se desenhou instantaneamente (delay == 0), dá um único refresh no final de tudo
    //if (delay_ms == 0) {
    //    refresh();
    //}
}





// Calcula a menor distância angular em um círculo (trata a virada de 360°)
double diferenca_angular_minima(double a, double b) {
    double diff = b - a;
    diff = fmod(diff, 360.0);
    if (diff > 180.0) diff -= 360.0;
    if (diff < -180.0) diff += 360.0;
    return diff;
}

int normalizar_grau_int(int angulo) {
    // Adiciona 360 antes do módulo para garantir que o dividendo seja sempre positivo,
    // mesmo que o ângulo seja ligeiramente negativo (como -1, -5, etc).
    int n = (angulo % 360 + 360) % 360;
    return n;
}

// void resolver_sobreposicao_planetas(PlotObject *plots, int object_count, int *longitude_saida, double *cusps) {
//     // 1. Inicializa o array de saída (já assumindo que plots está ordenado por qsort)
//     (void)cusps;

//     for (int i = 0; i < object_count; i++) {
//         longitude_saida[i] = (int)round(plots[i].longitude);
//     }

//     // AuxOrdenacao *fila = malloc(object_count * sizeof(AuxOrdenacao));
//     //     double longitude_ref = plots[0].longitude;

//     // for (int i = 0; i < object_count; i++) {
//     //     fila[i].index_original = i;
        
//     //     if (strcmp(plots[i].object, "IC") == 0 || strcmp(plots[i].object, "MC") == 0 ||
//     //         strcmp(plots[i].object, "AC") == 0 || strcmp(plots[i].object, "DC") == 0)
//     //     {
//     //         fila[i].type = Z_ANGLE;
//     //     }
//     //     else {
//     //         fila[i].type = Z_PLANET;
//     //     }
        
//     //     double diff = plots[i].longitude - longitude_ref;
//     //     diff = fmod(diff, 360.0);
//     //     if (diff < 0) diff += 360.0;
//     //     fila[i].dist_relativa = diff;
//     // }

//     //qsort(fila, object_count, sizeof(AuxOrdenacao), comparar_distantes);

//     const int DISTANCIA_MINIMA = 7; // Distância mínima desejada em graus
//     const int MAX_TENTATIVAS = 200; // Mais passadas para acomodar restrições rígidas
//     int houve_modificacao;

//     // int *casas_originais = malloc(object_count * sizeof(int));
//     // for (int i = 0; i < object_count; i++) {
//     //     casas_originais[i] = get_house(plots[i].longitude, cusps);
//     // }

//     // 2. Laço de Relaxamento com Restrição de Ordem
//     for (int tentativa = 0; tentativa < MAX_TENTATIVAS; tentativa++) {
//         houve_modificacao = 0;

//         // Varre os planetas vizinhos par a par
//         for (int i = 0; i < object_count - 1; i++) {
//             int j = (i == object_count - 1) ? 0 : ((i == 0) ? object_count - 1 : i + 1);

//             double diff = diferenca_angular_minima(longitude_saida[i], longitude_saida[j]);

//             // Se a distância for menor que o mínimo, precisamos afastar
//             if (fabs(diff) < DISTANCIA_MINIMA) {
                
//                 // Como o array é ordenado, o esperado é que j esteja na frente de i (diff > 0)
//                 // Se por erro de arredondamento inicial ou empurrão severo eles colidirem:
                
//                 // Tenta empurrar o planeta 'i' para trás, mas APENAS se não colidir/ultrapassar o planeta anterior (i-1)
//                 if (i > 0) {
//                     double dist_ant = diferenca_angular_minima(longitude_saida[i-1], longitude_saida[i]);
                                       
//                     if (fabs(dist_ant) > 1.0) { // Tem espaço atrás?
//                         longitude_saida[i]--;
//                         houve_modificacao = 1;
//                     }
//                 } else {
//                     double dist_ant = diferenca_angular_minima(longitude_saida[object_count - 1] - 360, longitude_saida[i]);
//                     if (fabs(dist_ant) > 1.0) { // Tem espaço atrás?
//                         longitude_saida[i]--;
//                         houve_modificacao = 1;
//                     }
//                 }

//                 // Tenta empurrar o planeta 'j' para frente, mas APENAS se não colidir/ultrapassar o próximo planeta (j+1)
//                 if (j < object_count - 1) {
//                     double dist_prox = diferenca_angular_minima(longitude_saida[j], longitude_saida[j+1]);
//                     if (fabs(dist_prox) > 1.0) { // Tem espaço à frente?
//                         longitude_saida[j]++;
//                         houve_modificacao = 1;
//                     }
//                 } else {
//                     double dist_prox = diferenca_angular_minima(longitude_saida[j], longitude_saida[0] + 360);
//                     if (fabs(dist_prox) > 1.0) { // Tem espaço à frente?
//                         longitude_saida[j]++;
//                         houve_modificacao = 1;
//                     }
//                 }

//                 // Normaliza os valores alterados imediatamente
//                 longitude_saida[i] = normalizar_grau_int(longitude_saida[i]);
//                 longitude_saida[j] = normalizar_grau_int(longitude_saida[j]);
//             }
//         }

//         // Se o laço rodar sem precisar mover mais ninguém, o mapa estabilizou perfeitamente
//         if (!houve_modificacao) {
//             break;
//         }
//     }
// }


void resolver_sobreposicao_planetas(PlotObject *plots, int object_count, int *longitude_saida, double *cusps) {
    if (object_count <= 0) return;

    int *casas = malloc(object_count * sizeof(int));
    int *signs = malloc(object_count * sizeof(int));

    // 1. Inicializa a saída e mapeia as casas originais
    for (int i = 0; i < object_count; i++) {
        longitude_saida[i] = (int)round(plots[i].longitude);
        casas[i] = get_house(plots[i].longitude, cusps);
        signs[i] = (int)floor(plots[i].longitude / 30);
    }
    if (object_count == 1) {
        free(casas);
        return;
    }

    // 2. ORDENAÇÃO CIRCULAR
    AuxOrdenacao *fila = malloc(object_count * sizeof(AuxOrdenacao));
    double longitude_ref = plots[0].longitude;

    for (int i = 0; i < object_count; i++) {
        fila[i].index_original = i;
        
        if (strcmp(plots[i].object, "IC") == 0 || strcmp(plots[i].object, "MC") == 0 ||
            strcmp(plots[i].object, "AC") == 0 || strcmp(plots[i].object, "DC") == 0)
        {
            fila[i].type = Z_ANGLE;
        }
        else {
            fila[i].type = Z_PLANET;
        }
        
        double diff = plots[i].longitude - longitude_ref;
        diff = fmod(diff, 360.0);
        if (diff < 0) diff += 360.0;
        fila[i].dist_relativa = diff;
    }

    qsort(fila, object_count, sizeof(AuxOrdenacao), comparar_distantes);

    const int DISTANCIA_MINIMA = 7;
    const int MAX_TENTATIVAS = 200;
    int houve_modificacao;

    // 3. Laço de Relaxamento Circular Ponderado
    for (int tentativa = 0; tentativa < MAX_TENTATIVAS; tentativa++) {
        houve_modificacao = 0;

        for (int i = 0; i < object_count; i++) {
            int proximo_i = (i == object_count - 1) ? 0 : i + 1;
           
            int idx_atual = fila[i].index_original;
            int idx_prox  = fila[proximo_i].index_original;
            
            double diff = diferenca_angular_minima(longitude_saida[idx_atual], longitude_saida[idx_prox]);
            
            // Caso 1: Ordem normal na roda, mas estão espremidos
            if (diff >= 0.0 && diff < DISTANCIA_MINIMA) {
                
                // Regra do objeto ATUAL ir para TRÁS (diminuir longitude):
                // Se for ÂNGULO, ele pode ir livremente. Se for PLANETA, só vai se continuar na mesma casa.
                int pode_mover_atual = (fila[idx_atual].type == Z_ANGLE) || 
                    (casas[idx_atual] == get_house(normalizar_grau_int(longitude_saida[idx_atual] - 1), cusps));

                // Regra do objeto PRÓXIMO ir para FRENTE (aumentar longitude):
                // Se for ÂNGULO, pode ir livremente. Se for PLANETA, só se continuar na mesma casa.
                int pode_mover_prox = (fila[idx_prox].type == Z_ANGLE) || 
                    (casas[idx_prox] == get_house(normalizar_grau_int(longitude_saida[idx_prox] + 1), cusps));

                if (pode_mover_atual && pode_mover_prox) {
                    // Cenário Ideal: Ambos se afastam 1 grau
                    longitude_saida[idx_atual] = normalizar_grau_int(longitude_saida[idx_atual] - 1);
                    longitude_saida[idx_prox]  = normalizar_grau_int(longitude_saida[idx_prox] + 1);
                    houve_modificacao = 1;
                } 
                else if (pode_mover_atual && !pode_mover_prox) {
                    // O próximo (planeta) travou na cúspide. O atual absorve o impacto duplo sozinho!

                    if (signs[idx_atual] == (int)(normalizar_grau_int(longitude_saida[idx_atual] - 8) / 30)) {
                        longitude_saida[idx_atual] = normalizar_grau_int(longitude_saida[idx_atual] - 8);
                        houve_modificacao = 1;
                    }
                    else if (signs[idx_atual] == (int)(normalizar_grau_int(longitude_saida[idx_atual] - 7) / 30)) {
                        longitude_saida[idx_atual] = normalizar_grau_int(longitude_saida[idx_atual] - 7);
                        houve_modificacao = 1;
                    }
                    else  if (signs[idx_atual] == (int)(normalizar_grau_int(longitude_saida[idx_atual] - 6) / 30)) {
                        longitude_saida[idx_atual] = normalizar_grau_int(longitude_saida[idx_atual] - 6);
                        houve_modificacao = 1;
                    }
                    else  if (signs[idx_atual] == (int)(normalizar_grau_int(longitude_saida[idx_atual] - 5) / 30)) {
                        longitude_saida[idx_atual] = normalizar_grau_int(longitude_saida[idx_atual] - 5);
                        houve_modificacao = 1;
                    }
                    else  if (signs[idx_atual] == (int)(normalizar_grau_int(longitude_saida[idx_atual] - 4) / 30)) {
                        longitude_saida[idx_atual] = normalizar_grau_int(longitude_saida[idx_atual] - 4);
                        houve_modificacao = 1;
                    }
                    else if (signs[idx_atual] == (int)(normalizar_grau_int(longitude_saida[idx_atual] - 1) / 30)) {
                       longitude_saida[idx_atual] = normalizar_grau_int(longitude_saida[idx_atual] - 1);
                       houve_modificacao = 1;
                    }                    
                } 
                else if (!pode_mover_atual && pode_mover_prox) {
                    // O atual (planeta) travou na cúspide. O próximo absorve o avanço duplo sozinho!
                    if (signs[idx_prox] == (int)(normalizar_grau_int(longitude_saida[idx_prox] + 2) / 30)) {
                        longitude_saida[idx_prox]  = normalizar_grau_int(longitude_saida[idx_prox] + 2);
                        houve_modificacao = 1;
                    }
                    else if (signs[idx_prox] == (int)(normalizar_grau_int(longitude_saida[idx_prox] + 1) / 30)) {
                        longitude_saida[idx_prox]  = normalizar_grau_int(longitude_saida[idx_prox] + 1);
                        houve_modificacao = 1;
                    }
                    
                }                
            }
            // Caso 2: Atropelo (Inversão geométrica de ordem na roda)
            else if (diff < 0.0 && diff > -DISTANCIA_MINIMA) {
                
                int pode_mover_atual = (fila[idx_atual].type == Z_ANGLE) || 
                    (casas[idx_atual] == get_house(normalizar_grau_int(longitude_saida[idx_atual] - 2), cusps));

                int pode_mover_prox = (fila[idx_prox].type == Z_ANGLE) || 
                    (casas[idx_prox] == get_house(normalizar_grau_int(longitude_saida[idx_prox] + 2), cusps));

                if (pode_mover_atual && pode_mover_prox) {
                    if (signs[idx_atual] == (int)(normalizar_grau_int(longitude_saida[idx_atual] - 2) / 30)) {
                        longitude_saida[idx_atual] = normalizar_grau_int(longitude_saida[idx_atual] - 2);
                    }
                    else {
                        longitude_saida[idx_atual] = normalizar_grau_int(longitude_saida[idx_atual] - 1);
                    }
                    
                    if (signs[idx_prox] == (int)(normalizar_grau_int(longitude_saida[idx_prox] + 2) / 30)) {
                        longitude_saida[idx_prox]  = normalizar_grau_int(longitude_saida[idx_prox] + 2);
                    }
                    else {
                        longitude_saida[idx_prox]  = normalizar_grau_int(longitude_saida[idx_prox] + 1);
                    }
                    houve_modificacao = 1;
                }
                else if (pode_mover_atual && !pode_mover_prox) {
                    if (signs[idx_atual] == (int)(normalizar_grau_int(longitude_saida[idx_atual] - 4) / 30)) {
                        longitude_saida[idx_atual] = normalizar_grau_int(longitude_saida[idx_atual] - 4);
                    }
                    else {
                        longitude_saida[idx_atual] = normalizar_grau_int(longitude_saida[idx_atual] - 1);
                    }
                    houve_modificacao = 1;
                }
                else if (!pode_mover_atual && pode_mover_prox) {
                    if (signs[idx_prox] == (int)(normalizar_grau_int(longitude_saida[idx_prox] + 4) / 30)) {
                        longitude_saida[idx_prox]  = normalizar_grau_int(longitude_saida[idx_prox] + 4);
                    }
                    else {
                        longitude_saida[idx_prox]  = normalizar_grau_int(longitude_saida[idx_prox] + 1);
                    }
                    houve_modificacao = 1;
                }
            }
            
        }

        if (!houve_modificacao) {
            break;
        }
    }

    free(fila);
    free(casas);
}


// Função auxiliar para encontrar em qual casa um planeta original está (Retorna 1 a 12)
int get_house(double longitude, double *cusps) {
    const double EPSILON = 1e-9;
    for (int i = 1; i <= 12; i++) {
        double current_cusp = cusps[i];
        double next_cusp = (i == 12) ? cusps[1] : cusps[i + 1];

        if (current_cusp < next_cusp) {
            if (longitude >= (current_cusp - EPSILON) && longitude < (next_cusp - EPSILON)) return i;
        } else {
            if (longitude >= (current_cusp - EPSILON) || longitude < (next_cusp - EPSILON)) return i;
        }
    }
    return 12;
}


// Helper function to draw objects at specific radius
void draw_objects_at_radius(int radius_multiplier, int object_count, 
                           PlotObject *plots, int n, 
                           int display_center_y, int display_center_x, 
                           float current_scale, float aspect_ratio, int asc, double *cusps) {
    
    
    float radius = radius_multiplier * current_scale;
    int steps = 330;
    (void)n;

    // This block is to separate objects hidden by proximity. Unhide and display hidden objects.
    int longitude[object_count];

    qsort(plots, object_count, sizeof(PlotObject), comparar_plots_por_longitude);
     
    for (int z = 0; z < object_count; z++) {
        int degree = (int)round(plots[z].longitude);
        longitude[z] = degree;
    }

    resolver_sobreposicao_planetas(plots, object_count, longitude, cusps);
               
    for (int i = -30; i < steps; i++) {
        float angle = i * PI / 180.0;
        int y = (int)(display_center_y + radius * sin(angle));
        int x = (int)(display_center_x + aspect_ratio * radius * cos(angle));
        
        for (int j = 0; j < object_count; j++) {
            if (i == (180 - (longitude[j] - asc) % 360) || 
                i == (180 - (longitude[j] - asc) % 360) + 360|| 
                i == (180 - (longitude[j] - asc) % 360) - 360) {
                
                // Set color based on object type
                if (strcmp(plots[j].object_name, _("Ascendant")) == 0 ||
                    strcmp(plots[j].object_name, _("Midheaven")) == 0 ||
                    strcmp(plots[j].object_name, _("Nadir")) == 0 ||
                    strcmp(plots[j].object_name, _("Descendant")) == 0 ||
                    strcmp(plots[j].object_name, _("Part of Fortune")) == 0 ||
                    strcmp(plots[j].object_name, _("SAN")) == 0 ||
                    strcmp(plots[j].object_name, _("South Node")) == 0 ||
                    strcmp(plots[j].object_name, _("Vertex")) == 0 ||
                    strcmp(plots[j].object_name, _("North Node")) == 0) {
                    attron(COLOR_PAIR(16) | A_BOLD);
                }
                else if (strcmp(plots[j].object_name, _("Neptune")) == 0 ||
                         strcmp(plots[j].object_name, _("Uranus")) == 0 ||
                         strcmp(plots[j].object_name, _("Pluto")) == 0) {
                    if (show_modern_planets) {
                        attron(COLOR_PAIR(17) | A_BOLD);
                    }
                    else {
                        attron(COLOR_PAIR(10) | A_BOLD);
                    }
                    
                }
                else {
                    attron(COLOR_PAIR(18) | A_BOLD);
                }
                
                // Draw the appropriate text
                const char* text_to_draw = NULL;
                switch(radius_multiplier) {
                    case 14: text_to_draw = plots[j].object; break;
                    case 12: text_to_draw = plots[j].degree; break;
                    case 11: text_to_draw = plots[j].sign; break;
                    case 10: text_to_draw = plots[j].min; break;
                    case 8: text_to_draw = plots[j].retrograde; break;
                    case 7: text_to_draw = plots[j].house; break;
                }
                
                if (text_to_draw) {
                    mvaddstr(y, x , text_to_draw);
                }
                
                attroff(COLOR_PAIR(16) | A_BOLD);
                attroff(COLOR_PAIR(17) | A_BOLD);
                attroff(COLOR_PAIR(18) | A_BOLD);
                attroff(COLOR_PAIR(10) | A_BOLD);
            }
        }
    }
    qsort(plots, object_count, sizeof(PlotObject), comparar_plots_por_id);
}



void draw_cusps(int radius_multiplier, int object_count, 
                double *cusps, int n, 
                int display_center_y, int display_center_x, 
                float current_scale, float aspect_ratio) {
    float radius = radius_multiplier * current_scale;
    
    int asc = (int)cusps[1];
    (void)n;

    int imin;
    int imax;
    if (fmod(cusps[1], 30) < 15 ) {
        imin = -75;
        imax = 285;
    }
    else {
        imin = -105;
        imax = 255; 
    }

    attron(COLOR_PAIR(14) | A_BOLD);
    for (int i = imin; i < imax; i++) {
        float angle = i * PI / 180.0;
        int y = (int)(display_center_y + radius * sin(angle));
        int x = (int)(display_center_x + aspect_ratio * radius * cos(angle));
        
        for (int j = 1; j <= object_count; j++) {
            if (i == (180 - ((int)(cusps[j]) - asc) % 360) || 
                i == (180 - ((int)(cusps[j]) - asc) % 360) + 360 ||
                i == (180 - ((int)(cusps[j]) - asc) % 360) - 360) {
                                               
                // Draw the appropriate text
                char text_to_draw[12];
                snprintf(text_to_draw, 12, "%d", j);
                
                //if (text_to_draw) {
                    mvaddstr(y, x, text_to_draw);
                //}
            }
        }
    }
    attroff(COLOR_PAIR(14) | A_BOLD);
    
}



void draw_cusps_div(int object_count, 
                double *cusps, int n, 
                int display_center_y, int display_center_x, 
                float current_scale, float aspect_ratio) {
    
    int asc = (int)cusps[1];
    (void)n;
    
    for (int r = 8; r < 20; r++) {    
        float radius = r * current_scale;
        
        int imin;
        int imax;
        if (fmod(cusps[1], 30) < 15 ) {
            imin = -75;
            imax = 285;
        }
        else {
            imin = -105;
            imax = 255; 
        }
        
        for (int i = imin; i < imax; i++) {
            float angle = i * PI / 180.0;
            int y = (int)(display_center_y + radius * sin(angle));
            int x = (int)(display_center_x + aspect_ratio * radius * cos(angle));
            
            for (int j = 1; j <= object_count; j++) {
                if (i == (180 - ((int)(cusps[j]) - asc) % 360) || 
                    i == (180 - ((int)(cusps[j]) - asc) % 360) + 360 ||
                    i == (180 - ((int)(cusps[j]) - asc) % 360) - 360) {
                                                
                    // Draw the appropriate text
                    if (j != 1 && j != 4 && j != 7 && j != 10) {
                        if (angle <= -1.79) {
                            mvaddstr(y, x, "▚");  // casa 11
                        } else if (angle <= -1.35 || (angle >= 4.5 && angle < 4.93)) {
                            mvaddstr(y, x, "▍"); // casa 10
                        } else if (angle <= -0.8 || angle >= 4.93) {
                            mvaddstr(y, x, "▞");  // casa 9 
                        } else if (angle <= -0.2) {
                            mvaddstr(y, x, "🙼");  // casa 8
                        } else if (angle <= 0.2) {
                            mvaddstr(y-1, x, "▁▁");
                            mvaddstr(y,   x, "▔▔");  // casa 7
                        } else if (angle <= 0.95) {
                            mvaddstr(y, x, "🙽");  // casa 6
                        } else if (angle < 1.35) {
                            mvaddstr(y, x, "▚"); // casa 5
                        } else if (angle < 1.79) {
                            mvaddstr(y, x, "▐");  // casa 4
                        } else if (angle < 2.26) {
                            mvaddstr(y, x, "▞");  // casa 3
                        } else if (angle < 2.99) {
                            mvaddstr(y, x, "🙼");  // casa 2
                        } else if (angle <= 3.25) {
                            mvaddstr(y,   x, "▁▁"); // casa 1
                            mvaddstr(y+1, x, "▔▔");
                        } else if (angle <= 4.1) {
                            mvaddstr(y, x, "🙽");  // casa 12
                        } else {
                            mvaddstr(y, x, "▚"); // casa 11
                        }
                    }
                //mvprintw(y, x, "%.2f ", angle);
                }
            }
        }
    }
}

void draw_cusps_div_axis(int object_count, 
                double *cusps, int n, 
                int display_center_y, int display_center_x, 
                float current_scale, float aspect_ratio) {
    
    int asc = (int)cusps[1];
    (void)n;

    attron(A_BOLD);

    for (int r = 8; r < 20; r++) {    
        float radius = r * current_scale;
        
        int imin;
        int imax;
        if (fmod(cusps[1], 30) < 15 ) {
            imin = -75;
            imax = 285;
        }
        else {
            imin = -105;
            imax = 255; 
        }

        for (int i = imin; i < imax; i++) {
            float angle = i * PI / 180.0;
            int y = (int)(display_center_y + radius * sin(angle));
            int x = (int)(display_center_x + aspect_ratio * radius * cos(angle));
            
            for (int j = 1; j <= object_count; j += 3) {
                if (i == (180 - ((int)(cusps[j]) - asc) % 360) || 
                    i == (180 - ((int)(cusps[j]) - asc) % 360) + 360 ||
                    i == (180 - ((int)(cusps[j]) - asc) % 360) - 360) {
                                                
                    // Draw the appropriate text
                    if (j == 1 || j == 4 || j == 7 || j == 10) {            
                        if (angle <= -1.79) {
                            mvaddstr(y, x, "⧹");  // casa 11 // ⧹⧸
                        } else if (angle <= -1.35 || (angle >= 4.5 && angle < 4.93)) {
                            //mvaddstr(y, x, "▎"); // casa 10
                            mvaddstr(y, x-1, "▕▎");                            
                        } else if (angle <= -0.8 || angle >= 4.93) {
                            mvaddstr(y, x, "⧸");  // casa 9
                        } else if (angle <= -0.2) {
                            mvaddstr(y, x, "🙼");  // casa 8
                        } else if (angle <= 0.2) {
                            //mvaddstr(y, x, "▔");  // casa 7
                            mvaddstr(y-1, x, "▁▁");
                            mvaddstr(y,   x, "▔▔");
                        } else if (angle <= 0.95) {
                            mvaddstr(y, x, "🙽");  // casa 6
                        } else if (angle < 1.35) {
                            mvaddstr(y, x, "⧹"); // casa 5
                        } else if (angle < 1.79) {
                            //mvaddstr(y, x, "▕");  // casa 4
                            mvaddstr(y, x, "▕▎");                            
                        } else if (angle < 2.26) {
                            mvaddstr(y, x, "⧸");  // casa 3
                        } else if (angle < 2.99) {
                            mvaddstr(y, x, "🙼");  // casa 2
                        } else if (angle <= 3.25) {
                            //mvaddstr(y, x, "▁");  // casa 1
                            mvaddstr(y,   x, "▁▁");
                            mvaddstr(y+1, x, "▔▔");

                        } else if (angle <= 4.1) {
                            mvaddstr(y, x, "🙽");  // casa 12
                        } else {
                            mvaddstr(y, x, "⧹"); // casa 11
                        }
                    }
                }                
            }
        }
    }
    attroff(A_BOLD);     
}



void draw_day_hour_regents(int week_day, int planetary_hour, int display_center_y, int display_center_x, 
                           float current_scale, float aspect_ratio) {

    float radius = 4.0 * current_scale;

    float angle = -150 * PI / 180.0;
    int y = (int)(display_center_y + radius * sin(angle));
    int x = (int)(display_center_x + aspect_ratio * radius * cos(angle));

    int day_regent = get_hour_regent(week_day, (MAPA_DIURNO)?0:12);
    int hour_regent = get_hour_regent(week_day, planetary_hour);

    attron(COLOR_PAIR(19) | A_DIM);
    for (int j = 0; j < 6; j++) {
        if (day_regent == PH_SOL) {
            mvaddstr(y + j, x + 2, sol_ascii[j]);
        }
        else if (day_regent == PH_LUNA) {
            mvaddstr(y + j, x + 2, lua_ascii[j]);
        }
        else if (day_regent == PH_MERCURY) {
            mvaddstr(y + j, x + 2, mercury_ascii[j]);
        }
        else if (day_regent == PH_VENUS) {
            mvaddstr(y + j, x + 2, venus_ascii[j]);
        }
        else if (day_regent == PH_MARS) {
            mvaddstr(y + j, x + 2, marte_ascii[j]);
        }
        else if (day_regent == PH_JUPITER) {
            mvaddstr(y + j, x + 2, jupiter_ascii[j]);
        }
        else if (day_regent == PH_SATURN) {
            mvaddstr(y + j, x + 2, saturno_ascii[j]);
        }

        if (hour_regent == PH_SOL) {
            mvaddstr(y + j, x + 8, sol_ascii[j]);
        }
        else if (hour_regent == PH_LUNA) {
            mvaddstr(y + j, x + 8, lua_ascii[j]);
        }
        else if (hour_regent == PH_MERCURY) {
            mvaddstr(y + j, x + 8, mercury_ascii[j]);
        }
        else if (hour_regent == PH_VENUS) {
            mvaddstr(y + j, x + 8, venus_ascii[j]);
        }
        else if (hour_regent == PH_MARS) {
            mvaddstr(y + j, x + 8, marte_ascii[j]);
        }
        else if (hour_regent == PH_JUPITER) {
            mvaddstr(y + j, x + 8, jupiter_ascii[j]);
        }
        else if (hour_regent == PH_SATURN) {
            mvaddstr(y + j, x + 8, saturno_ascii[j]);
        }
    }
    attroff(COLOR_PAIR(19) | A_DIM);
}


// Helper function to draw zodiac signs
void draw_zodiac_signs(int display_center_y, int display_center_x, 
                      float current_scale, float aspect_ratio, int n, int asc) {
    
    
    float radius = 16.0 * current_scale;
    int offset = 0 * n;    
    
    for (int i = 75 + 270 + asc, k = 0; i < 435 + 270 + asc; i += 30, k++) {
        float angle = i * PI / 180.0;
        int y = (int)(display_center_y + radius * sin(angle));
        int x = (int)(display_center_x + aspect_ratio * radius * cos(angle));
        
        for (int j = 0; j < 4; j++) {
            if (y >= 0 && y < LINES && x >= 0 && x < COLS) {
                if ( (k + offset) % 12 == 0 ) {
                    
                    if (j == 0) continue;

                    attron(COLOR_PAIR(4) | A_NORMAL);
                    mvaddstr(y - 2 + j, x - 2, libra[j]);
                    attroff(COLOR_PAIR(4) | A_NORMAL);
                }
                else if ( (k + offset) % 12 == 1 ) {

                    if (j == 0) continue;

                    attron(COLOR_PAIR(3) | A_DIM);
                    mvaddstr(y - 2 + j, x - 2, virgo[j]);
                    attroff(COLOR_PAIR(3) | A_DIM);
                }
                else if ( (k + offset) % 12 == 2 ) {
                    attron(COLOR_PAIR(2) | A_BOLD);
                    mvaddstr(y - 2 + j, x - 2, leo[j]);
                    attroff(COLOR_PAIR(2) | A_BOLD);
                }
                else if ( (k + offset) % 12 == 3 ) {

                    if (j == 0) continue;

                    attron(COLOR_PAIR(5) | A_NORMAL);
                    mvaddstr(y - 2 + j, x - 2, cancer[j]);
                    attroff(COLOR_PAIR(5) | A_NORMAL);
                }
                else if ( (k + offset) % 12 == 4 ) {
                    attron(COLOR_PAIR(4) | A_NORMAL);
                    mvaddstr(y - 2 + j, x - 2, gemini[j]);
                    attroff(COLOR_PAIR(4) | A_NORMAL);
                }
                else if ( (k + offset) % 12 == 5 ) {
                    attron(COLOR_PAIR(3) | A_DIM);
                    mvaddstr(y - 2 + j, x - 2, taurus[j]);
                    attroff(COLOR_PAIR(3) | A_DIM);
                }
                else if ( (k + offset) % 12 == 6 ) {

                    if (j == 0) continue;

                    attron(COLOR_PAIR(2) | A_BOLD);
                    mvaddstr(y - 2 + j, x - 2, aries[j]);
                    attroff(COLOR_PAIR(2) | A_BOLD);
                }
                else if ( (k + offset) % 12 == 7 ) {
                    attron(COLOR_PAIR(5) | A_NORMAL);
                    mvaddstr(y - 2 + j, x - 2, pisces[j]);
                    attroff(COLOR_PAIR(5) | A_NORMAL);
                }
                else if ( (k + offset) % 12 == 8 ) {

                    if (j == 0) continue;

                    attron(COLOR_PAIR(4) | A_NORMAL);
                    mvaddstr(y - 2 + j, x - 2, aquarius[j]);
                    attroff(COLOR_PAIR(4) | A_NORMAL);
                }
                else if ( (k + offset) % 12 == 9 ) {

                    if (j == 0) continue;

                    attron(COLOR_PAIR(3) | A_DIM);
                    mvaddstr(y - 2 + j, x - 2, capricorn[j]);
                    attroff(COLOR_PAIR(3) | A_DIM);
                }
                else if ( (k + offset) % 12 == 10 ) {
                    attron(COLOR_PAIR(2) | A_BOLD);
                    mvaddstr(y - 2 + j, x - 2, sagittarius[j]);
                    attroff(COLOR_PAIR(2) | A_BOLD);
                }
                else if ( (k + offset) % 12 == 11 ) {

                    if (j == 0) continue;

                    attron(COLOR_PAIR(5) | A_NORMAL);
                    mvaddstr(y - 2 + j, x - 2, scorpio[j]);
                    attroff(COLOR_PAIR(5) | A_NORMAL);
                }
            }
        }
    }
}

void draw_decans(int display_center_y, int display_center_x, 
                 float current_scale, float aspect_ratio, int n, int asc) {
    
    float radius = 18.0 * current_scale;
    (void)n;    
    
    for (int i = -175 + asc, k = 0; i < 185 + asc; i += 10, k++) {
        
        float angle = i * PI / 180.0;
        int y = (int)(display_center_y + radius * sin(angle));
        int x = (int)(display_center_x + aspect_ratio * radius * cos(angle));
        
        
        if (y >= 0 && y < LINES && x >= 0 && x < COLS) {
            mvaddstr(y, x, planet_regent_symbols[decans_to_print[(k) % 36]]);
        }
        
    }
}


void draw_terms(int radius_multiplier, int object_count, 
                Termo t[12][5], 
                int display_center_y, int display_center_x, 
                float current_scale, float aspect_ratio, int asc) {
    
    
    float radius = radius_multiplier * current_scale;
    int steps = 360;

    int longitudes[object_count];
    int counter = 0;

    char text[object_count][10];

    for (int k = 0; k < 12; k++) {
        for (int l = 0; l < 5; l++) {
            longitudes[counter] = t[k][l].grau_limite;
            snprintf(text[counter], 10, "%s", planet_regent_symbols[t[k][l].regente]);
            counter++;
        }
    }

    for (int i = 0; i < steps; i++) {
        float angle = i * PI / 180.0;
        int y = (int)(display_center_y + radius * sin(angle));
        int x = (int)(display_center_x + aspect_ratio * radius * cos(angle));
        
        for (int j = 0; j < object_count; j++) {
            if (i == (180 - (longitudes[j] - asc) % 360) || 
                i == (180 - (longitudes[j] - asc) % 360) + 360|| 
                i == (180 - (longitudes[j] - asc) % 360) - 360) {
                
                mvaddstr(y, x , text[j]);
                                
            }
        }
    }
}



void draw_chart(float zoom_factor, float pan_x, float pan_y, 
                int n, struct tm *local_time, double lat, double lon, double elev, double tz_offset,
                PlotObject *plots, double *cusps, int sanYear, int sanMon, int sanDay, double sanHour, 
                char *sunrise_time, char *sunset_time, char *city, char *country, 
                double daytime_hour, double nighttime_hour, int week_day, int planetary_hour, 
                const char* phase, bool dark_mode, bool animated, int anim_interval, bool mapa_retorno,
                char *chart_name, char house_system, int gender_id, bool house_div, int last_hr, int last_min, double last_sec, bool show_dec, Termo terms[12][5], bool show_terms) {
    
    int object_diff = 0;
    if (show_modern_planets) {
        object_diff = 0;
    }
    else {
        object_diff = 3;
    }
    
    if (dark_mode) {
        init_pair(1, COLOR_WHITE, COLOR_BLACK);
        init_pair(2, COLOR_WHITE, COLOR_RED);
        init_pair(3, COLOR_WHITE, COLOR_GREEN);
        init_pair(4, COLOR_WHITE, COLOR_YELLOW);
        init_pair(5, COLOR_WHITE, COLOR_BLUE);
        init_pair(6, COLOR_WHITE, COLOR_BLACK);
        init_pair(7, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(8, COLOR_CYAN, COLOR_BLACK);
        init_pair(9, COLOR_CYAN, COLOR_CYAN);
        init_pair(10, COLOR_WHITE, COLOR_BLACK);

        init_pair(11, COLOR_RED, COLOR_BLACK);
        init_pair(12, COLOR_GREEN, COLOR_BLACK);
        init_pair(13, COLOR_WHITE, COLOR_BLACK);

        init_pair(14, COLOR_BLACK, COLOR_CYAN);
        init_pair(15, COLOR_BLUE, COLOR_YELLOW);
        init_pair(16, COLOR_WHITE, COLOR_BLACK);
        init_pair(17, COLOR_WHITE, COLOR_MAGENTA);
        init_pair(18, COLOR_WHITE, COLOR_BLUE);
        init_pair(19, COLOR_WHITE, COLOR_WHITE);
        init_pair(20, COLOR_CYAN, COLOR_BLACK);

        init_pair(21, COLOR_YELLOW, COLOR_BLUE);
        init_pair(22, COLOR_WHITE, COLOR_BLACK);
        init_pair(23, COLOR_RED, COLOR_WHITE);
        init_pair(24, COLOR_CYAN, COLOR_CYAN);
        init_pair(25, COLOR_YELLOW, COLOR_BLACK);
        init_pair(26, COLOR_WHITE, COLOR_BLACK);
        init_pair(27, COLOR_RED, COLOR_BLACK);
        init_pair(28, COLOR_WHITE, COLOR_MAGENTA);
        init_pair(29, COLOR_BLACK, COLOR_BLACK);
        init_pair(30, COLOR_CYAN, COLOR_MAGENTA);
        
        init_pair(31, COLOR_GREEN, COLOR_RED);
        init_pair(32, COLOR_MAGENTA, COLOR_GREEN);
        init_pair(33, COLOR_BLACK, COLOR_BLUE);
        init_pair(34, COLOR_WHITE, COLOR_CYAN);
    } 
    else {
        init_pair(1, COLOR_BLACK, COLOR_WHITE);
        init_pair(2, COLOR_RED, COLOR_WHITE);
        init_pair(3, COLOR_GREEN, COLOR_WHITE);
        init_pair(4, COLOR_YELLOW, COLOR_WHITE);
        init_pair(5, COLOR_BLUE, COLOR_WHITE);
        init_pair(6, COLOR_BLACK, COLOR_WHITE);
        init_pair(7, COLOR_MAGENTA, COLOR_WHITE);
        init_pair(8, COLOR_BLUE, COLOR_WHITE);
        init_pair(9, COLOR_BLACK, COLOR_BLACK);
        init_pair(10, COLOR_WHITE, COLOR_WHITE);

        init_pair(11, COLOR_RED, COLOR_WHITE);
        init_pair(12, COLOR_GREEN, COLOR_MAGENTA);
        init_pair(13, COLOR_BLACK, COLOR_WHITE);
        
        init_pair(14, COLOR_WHITE, COLOR_BLACK);
        init_pair(15, COLOR_YELLOW, COLOR_BLUE);
        init_pair(16, COLOR_BLACK, COLOR_WHITE);
        init_pair(17, COLOR_MAGENTA, COLOR_WHITE);
        init_pair(18, COLOR_BLUE, COLOR_WHITE);
        init_pair(19, COLOR_WHITE, COLOR_WHITE);
        init_pair(20, COLOR_WHITE, COLOR_BLUE);

        init_pair(21, COLOR_BLUE, COLOR_YELLOW);
        init_pair(22, COLOR_BLACK, COLOR_WHITE);
        init_pair(23, COLOR_RED, COLOR_WHITE);
        init_pair(24, COLOR_BLACK, COLOR_BLACK);
        init_pair(25, COLOR_YELLOW, COLOR_BLACK);
        init_pair(26, COLOR_BLACK, COLOR_CYAN);
        init_pair(27, COLOR_RED, COLOR_CYAN);
        init_pair(28, COLOR_MAGENTA, COLOR_WHITE);
        init_pair(29, COLOR_WHITE, COLOR_WHITE);
        init_pair(30, COLOR_MAGENTA, COLOR_CYAN);

        init_pair(31, COLOR_GREEN, COLOR_RED);
        init_pair(32, COLOR_MAGENTA, COLOR_GREEN);
        init_pair(33, COLOR_WHITE, COLOR_BLUE);
        init_pair(34, COLOR_CYAN, COLOR_WHITE);
    }
    
    int flags = 0;
    if (dark_mode) {
        flags |= A_DIM | A_REVERSE;
    }

    if (mapa_retorno) {
        if (!dark_mode) {
            bkgd(COLOR_PAIR(12) | flags);
        }
        else {
            bkgd(COLOR_PAIR(32) | flags); 
        }
    }
    else {
        bkgd(COLOR_PAIR(15) | flags);
    }
    
    clear();
    
    cbreak();
    noecho();
    curs_set(0); 
    keypad(stdscr, TRUE); // Enable keypad for special keys
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int center_y = max_y / 2;
    int center_x = max_x / 2;
    
    // Defined aspect ratio based on characters mono dimensions
    float aspect_ratio = 2.0;
    
    // Determine the maximum possible radius that fits in the terminal
    float max_r_y = (max_y / 2.0) - 0.0;
    float max_r_x = (max_x / 2.0) / aspect_ratio;
    
    float base_radius = (max_r_y < max_r_x) ? max_r_y : max_r_x;
    
    //Radius 20 as the reference for scaling
    float scale = base_radius / 20.0;
    
    // Apply zoom to the scale
    float current_scale = scale * zoom_factor;
    
    // Apply panning to center coordinates
    int display_center_y = center_y + (int)(pan_y * current_scale);
    int display_center_x = center_x + (int)(pan_x * current_scale);
    
    // Draw the outer circle filled
    attron(COLOR_PAIR(19) | flags);
    draw_circle_filled(display_center_y, display_center_x, 20, aspect_ratio, current_scale, L" ");
    attroff(COLOR_PAIR(19) | flags);
    
    attron(COLOR_PAIR(1) | flags);
    draw_circle_points(display_center_y, display_center_x, 20, aspect_ratio, current_scale, L"▓");    
    draw_circle_points(display_center_y, display_center_x, 7, aspect_ratio, current_scale, L"▒");

    //Draw the outer boundary using a light shade block
    int asc = (int)cusps[1];
    if (dark_mode) attron(COLOR_PAIR(19) | A_DIM | flags); else attron(COLOR_PAIR(34) | A_DIM | flags);
    for (float r = 8.0 * current_scale; r <= 19.5 * current_scale; r += current_scale) {
        for (int i = -60 + asc; i < 300 + asc; i += 30) {
            float angle = i * PI / 180.0;
            int y = (int)(display_center_y + r * sin(angle));
            int x = (int)(display_center_x + aspect_ratio * r * cos(angle));
            
            if (y >= 0 && y < LINES && x >= 0 && x < COLS) {                
                mvaddwstr(y, x, L"░");
            }
        }
    }
    if (dark_mode) attroff(COLOR_PAIR(19) | A_DIM | flags); else attroff(COLOR_PAIR(34) | A_DIM | flags);
    
    attron(COLOR_PAIR(1));    
    draw_cusps_div_axis(12, cusps, n, display_center_y, display_center_x, current_scale, aspect_ratio);
    attroff(COLOR_PAIR(1));
    

    if (house_div) {
        if (dark_mode) attron(COLOR_PAIR(1) | A_DIM); else attron(COLOR_PAIR(19) | A_DIM);

        draw_cusps_div(12, cusps, n, display_center_y, display_center_x, current_scale, aspect_ratio);
        
        if (dark_mode) attroff(COLOR_PAIR(1) | A_DIM); else attroff(COLOR_PAIR(19) | A_DIM);
    }

    // Draw house numbers
    draw_cusps(7, 12, cusps, n, display_center_y, display_center_x, current_scale, aspect_ratio);
    draw_cusps(20, 12, cusps, n, display_center_y, display_center_x, current_scale, aspect_ratio);
    
    if (zoom_factor <= 1.3) {
        attron(A_BOLD);
        for (int j = 1; j <= 12; j++) {
            int sign = (int)(cusps[j] / 30);
            int degree = (int)cusps[j] % 30 ;
            int min = (int)((cusps[j] - (int)cusps[j]) * 60);

            const char *sign_str = get_sign(sign);

            mvprintw(13 + j, 2, "%s %2d: %2d°%s%02d'", _("House"), j, degree, sign_str, min);
        }

        attroff(A_BOLD);

        int add_n = 0;
        if (mapa_retorno) add_n = 1;

        for (int i = 0; i < 6; i++) {
            if (MAPA_DIURNO) {
                mvprintw(LINES - (10 + add_n) + i, 7, "%s", sol_ascii[i]);
            } else {
                mvprintw(LINES - (10 + add_n) + i, 7, "%s", lua_ascii[i]);
            }
        }
        
    }

    // Draw zodiac signs
    draw_zodiac_signs(display_center_y, display_center_x, current_scale, aspect_ratio, n, (int)cusps[1]);
    
    if (show_dec) {
        attron(COLOR_PAIR(17) | flags | A_BOLD);
        draw_decans(display_center_y, display_center_x, current_scale, aspect_ratio, n, (int)cusps[1]);
        attroff(COLOR_PAIR(17) | flags | A_BOLD);
    }

    if (show_terms) {
        Termo t[12][5];
        get_terms_longitude_to_print(terms, t);
        attron(COLOR_PAIR(17) | flags | A_BOLD);
        draw_terms(18, 60, t, display_center_y, display_center_x, current_scale, aspect_ratio, asc);
        attroff(COLOR_PAIR(17) | flags | A_BOLD);
    }

    if (zoom_factor <= 1.3) {
        attron(A_BOLD);
        for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
            int sign = (int)(plots[i].longitude / 30);
            int degree = (int)plots[i].longitude % 30 ;
            int min = (int)((plots[i].longitude - (int)plots[i].longitude) * 60);

            const char *sign_str = get_sign(sign);
            
            if (get_visual_width(plots[i].object) == 1) {
                mvprintw(12 + i, COLS - 21, "%2s → %2d°%s%02d' %1s %s", plots[i].object, degree, sign_str, min, plots[i].retrograde, plots[i].house);
            }
            else if (get_visual_width(plots[i].object) == 2) {
                mvprintw(12 + i, COLS - 22, "%2s → %2d°%s%02d' %1s %s", plots[i].object, degree, sign_str, min, plots[i].retrograde, plots[i].house);
            }
            else {
                mvprintw(12 + i, COLS - 21, "%2s → %2d°%s%02d' %1s %s", plots[i].object, degree, sign_str, min, plots[i].retrograde, plots[i].house);
            }
        }
        attroff(A_BOLD);
    }

    draw_day_hour_regents(week_day - 1, planetary_hour - 1, display_center_y, display_center_x, current_scale, aspect_ratio);
    //refresh();
    
    // Draw all objects at different radii
    draw_objects_at_radius(14, NUM_OBJECTS - object_diff, plots, n, display_center_y, display_center_x, current_scale, aspect_ratio, asc, cusps);
    draw_objects_at_radius(12, NUM_OBJECTS - object_diff, plots, n, display_center_y, display_center_x, current_scale, aspect_ratio, asc, cusps);
    if (zoom_factor >= 1.4) {
        draw_objects_at_radius(11, NUM_OBJECTS - object_diff, plots, n, display_center_y, display_center_x, current_scale, aspect_ratio, asc, cusps);
    }
    draw_objects_at_radius(10, NUM_OBJECTS - object_diff, plots, n, display_center_y, display_center_x, current_scale, aspect_ratio, asc, cusps);
    //draw_objects_at_radius(9, NUM_OBJECTS - object_diff, plots, n, display_center_y, display_center_x, current_scale, aspect_ratio, asc, cusps);
    draw_objects_at_radius(8, NUM_OBJECTS - object_diff, plots, n, display_center_y, display_center_x, current_scale, aspect_ratio, asc, cusps);
    
    
    char *house_system_name;    
    get_house_system_name(house_system, &house_system_name);

    
    attron(A_BOLD | A_REVERSE);
    mvprintw(0, 1, "%s ", chart_name);
    attroff(A_BOLD);
    
    wprintw(stdscr, "(%s)", (gender_id == 1)?_("Masculine"):((gender_id == 2)?_("Feminine"):_("Neuter")));

    attroff(A_REVERSE);

    
    
    if (zoom_factor <= 1.2) {
        attron(A_BOLD);
        mvprintw(1, 1, "%s: %d/%d/%d %02d:%02d:%02d %s, %s", _("Time"), local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday, (local_time->tm_hour), local_time->tm_min, local_time->tm_sec, _("TZ"), str_dow(local_time->tm_wday));
        attroff(A_BOLD);

        mvprintw(3, 1, "%s / %s (%.4f)", city, country, tz_offset);
        mvprintw(4, 1, "Lat: %.4f / Lon: %.4f / Elev: %.1f", lat, lon, elev);

        attron(A_BOLD);
        mvprintw(6, 1, "%s: %s ", _("Sunrise"), sunrise_time);
        mvprintw(7, 1, "%s: %s ", _(" Sunset"), sunset_time);
        attroff(A_BOLD);

        Hora hd = get_fmt_hour(daytime_hour);
        Hora hn = get_fmt_hour(nighttime_hour);

        mvprintw(9, 1,  "%s: %.4f (%02d:%02d:%02d)", _("  Hour (Day)"), daytime_hour, hd.hora, hd.min, hd.sec);
        mvprintw(10, 1, "%s: %.4f (%02d:%02d:%02d)", _("Hour (Night)"), nighttime_hour, hn.hora, hn.min, hn.sec);
        mvprintw(12, 1, "%s: %s", _("Houses"), house_system_name);

        Hora hs = get_fmt_hour(sanHour);
        
        mvprintw(0, max_x - 39, "%s %s: %d/%d/%d %02d:%02d:%02d Local", _("Syzygy"), plots[P_SAN - object_diff].object, sanYear, sanMon, sanDay, hs.hora, hs.min, hs.sec);
        mvprintw(1, max_x - 40, "%s: %s", _("Current Moon Phase"), phase);
        mvprintw(3, max_x - 36, "%s %s(%d)/%s(%d): %s / %s", _("Planetary"), (MAPA_DIURNO)?_("Day"):_("Night"), week_day, _("Hour"), planetary_hour, planet_regent_symbols[get_hour_regent(week_day - 1, (MAPA_DIURNO)?0:12)], planet_regent_symbols[get_hour_regent(week_day - 1, planetary_hour - 1)]);
        
        mvprintw(5, max_x - 22, "%s: %02d:%02d:%02.0f", _("Sun Clock"), last_hr, last_min, last_sec);

        if (mapa_retorno) {
            mvprintw(LINES - 6, 1, _("Divisions = Zodiac Signs"));
            mvprintw(LINES - 4, 1, _("Radix Confrontation: C | Annual Transits: T "));
            mvprintw(LINES - 3, 1, _("Parts Radix Confrontation: P ")); 
        }
        else {
            mvprintw(LINES - 4, 1, _("Divisions = Zodiac Signs"));
        }
        mvprintw(LINES - 2, 1, "Zoom: + / -  | Pan: ←↓→↑  | Reset: R ");
        mvprintw(LINES - 1, 1, _("Animation: A | Speed: ]/[ | Quit: Q "));
        
        mvprintw(LINES - 3, max_x - 45, _(" Menu: M | Houses: H | Terms: B | Decans: D "));
        mvprintw(LINES - 2, max_x - 26, _(" Action: F1..F9, F12, 0-8 "));

        if (animated) {
            attron(A_BLINK);
        }
        mvprintw(LINES - 1, max_x - 37, "%s", (animated) ? _("▶️  Running") : _("⏸️  Stopped"));

        if (animated) {
            attroff(A_BLINK);
        }
    }
    else {
        attron(A_BOLD);
        mvprintw(LINES - 1, 1, "%s: %d/%d/%d %02d:%02d:%02d %s, %s", _("Time"), local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday, (local_time->tm_hour), local_time->tm_min, local_time->tm_sec, _("TZ"), str_dow(local_time->tm_wday));
        attroff(A_BOLD);
    }
    mvprintw(LINES - 1, max_x - 25, "%s: %5d s", _("Speed Animation"), anim_interval);

    refresh();
    free(house_system_name);
}


int get_opposite_sign(int sign) {
    switch(sign) {
        case 0: return 6;
        case 1: return 7;
        case 2: return 8;
        case 3: return 9;
        case 4: return 10;
        case 5: return 11;
        case 6: return 0;
        case 7: return 1;
        case 8: return 2;
        case 9: return 3;
        case 10: return 4;
        case 11: return 5;
    }
    return -1;  
}


int get_sign_antiscium(int sign) {
    switch(sign) {
        case 0: return 5;
        case 1: return 4;
        case 2: return 3;
        case 3: return 2;
        case 4: return 1;
        case 5: return 0;
        case 6: return 11;
        case 7: return 10;
        case 8: return 9;
        case 9: return 8;
        case 10: return 7;
        case 11: return 6;
    }
    return -1;  
}


double get_antiscium_degree(double degree) {
    return 30.0 - degree;
}


char *get_sign(int n) {
    switch(n) {
        case 0: return "♈";
        case 1: return "♉";
        case 2: return "♊";
        case 3: return "♋";
        case 4: return "♌";
        case 5: return "♍";
        case 6: return "♎";
        case 7: return "♏";
        case 8: return "♐";
        case 9: return "♑";
        case 10: return "♒";
        case 11: return "♓";
    }
    return "";
}

char *get_sign_name(int n) {
    switch(n) {
        case 0: return _("Aries");
        case 1: return _("Taurus");
        case 2: return _("Gemini");
        case 3: return _("Cancer");
        case 4: return _("Leo");
        case 5: return _("Virgo");
        case 6: return _("Libra");
        case 7: return _("Scorpio");
        case 8: return _("Sagittarius");
        case 9: return _("Capricorn");
        case 10: return _("Aquarius");
        case 11: return _("Pisces");
    }
    return "";
}


char *get_sign_element(int n) {
    switch(n) {
        case 0: return "🜂";
        case 1: return "🜃";
        case 2: return "🜁";
        case 3: return "🜄";
        case 4: return "🜂";
        case 5: return "🜃";
        case 6: return "🜁";
        case 7: return "🜄";
        case 8: return "🜂";
        case 9: return "🜃";
        case 10: return "🜁";
        case 11: return "🜄";
    }
    return "";
}


char *get_sign_element_name(int n) {
    switch(n) {
        case 0: return _("Fire");
        case 1: return _("Earth");
        case 2: return _("Air");
        case 3: return _("Water");
        case 4: return _("Fire");
        case 5: return _("Earth");
        case 6: return _("Air");
        case 7: return _("Water");
        case 8: return _("Fire");
        case 9: return _("Earth");
        case 10: return _("Air");
        case 11: return _("Water");
    }
    return "";
}


bool mapa_diurno() {
    return MAPA_DIURNO;
}

int diff_sign(int signA, int signB) {
    int diff = abs(signA - signB);

    if (diff > 6) {
        if (signB > signA) {
            diff = abs(signA + 12 - signB);
        }
        else {
            diff = abs(signB + 12 - signA);
        }
    }

    return diff;
}

double get_total_ponderado(double essential, double accidental) {
    double peso_essencial = 7.0;
    double peso_acidental = 3.0;

    double score_bruto = (essential * peso_essencial + accidental * peso_acidental) / (peso_essencial + peso_acidental);
    return score_bruto;
}

double get_total_normalized(double essential, double accidental) {
    
    double MIN_FIXO = -4.0;
    double MAX_FIXO = 10.0;

    double score_bruto = get_total_ponderado(essential, accidental);

    // Clampa os valores caso algum mapa extraordinário estoure os limites práticos
    if (score_bruto < MIN_FIXO) score_bruto = MIN_FIXO;
    if (score_bruto > MAX_FIXO) score_bruto = MAX_FIXO;

    // A régua de divisão agora tem SEMPRE o tamanho fixo de 14.0 pontos (10.0 - (-4.0))
    double nota_positiva = ((score_bruto - MIN_FIXO) / (MAX_FIXO - MIN_FIXO)) * 100.0;

    return (nota_positiva > 0.0) ? nota_positiva : 0.0;
}


double get_planet_force(int id, double total_normalized, double *weights) {
    return total_normalized * weights[id];
}



void calcular_forca_planetas(PlanetDignities *dig, int *resultado_strength, bool com_modernos) {
    int object_diff = com_modernos ? 0 : 3;

    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        // if ((com_modernos && (i >= 14 && i <= 17)) || 
        //     (!com_modernos && (i >= 13 && i <= 14))) {
        //     resultado_strength[i] = 0; 
        //     continue;
        // }

        // SALVA A NOTA PURA (0 a 100) DIRETO NO ARRAY!
        double total_normalized = get_total_normalized(dig[i].essential, dig[i].accidental);
        resultado_strength[i] = (int)round(total_normalized); // Guarda a porcentagem (ex: 84 para Mercúrio)
    }
}



// void calcular_forca_planetas(PlanetDignities *dig, int *resultado_strength, bool com_modernos) {
//     int object_diff = com_modernos ? 0 : 3;

//     /* RECALIBRADO PARA A REALIDADE PRÁTICA DO CÉU (Fim do esmagamento de notas) */
//     double min_value = -4.0;
//     double max_value = 10.0;

//     double weights[50];
//     get_weights(weights, com_modernos);

//     for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
//         if ((com_modernos && (i >= 14 && i <= 17)) || 
//             (!com_modernos && (i >= 13 && i <= 14))) {
//             resultado_strength[i] = 0; 
//             continue;
//         }

//         double total_normalized = get_total_normalized(dig[i].essential, dig[i].accidental, min_value, max_value);
//         double score_planet = get_planet_force(i, total_normalized, weights) / 10.0;
        
//         resultado_strength[i] = (int)ceil(score_planet);
//     }
// }




void display_planetary_energy_profile(PlotObject *plots, int *strength_planets) {
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 24;
    if (table_height > max_y - 2) table_height = max_y - 2; 
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = (max_x - table_width) / 2;

    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    // Renderização da sombra padrão
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13));

    // Cabeçalho Fixo do Formulário
    wattron(table_win, A_BOLD);
    const char *title = _(" Planetary Energy Profile ");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
    
    // Colunas reposicionadas: Nome ganhou mais espaço (coluna 4 a 22)
    mvwprintw(table_win, 2, 4, _("Planet               Bar Chart Representation                        Points"));
    mvwprintw(table_win, 3, 2, "────────────────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, A_BOLD);

    wrefresh(table_win);

    // 2. CRIAÇÃO DA PAD VIRTUAL DE ROLAGEM
    int max_linhas_dados_visiveis = table_height - 7; // Espaço útil físico na tela para as barras
    WINDOW *pad = newpad(40, table_width - 4); // Buffer abundante de 40 linhas verticais
    wbkgd(pad, COLOR_PAIR(13));

    int object_diff = show_modern_planets ? 0 : 3;
    int row_pad = 0;

    // 3. PREENCHIMENTO DO CONTEÚDO DENTRO DA PAD
    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        // if ((show_modern_planets && (i >= 14 && i <= 17)) || 
        //     (!show_modern_planets && (i >= 13 && i <= 14))) {
        //     continue;
        // }

        if (row_pad > 0) {
            wattron(pad, COLOR_PAIR(10) | A_DIM);
            mvwprintw(pad, row_pad, 2, "────────────────────────────────────────────────────────────────────────────────────────────"); 
            wattroff(pad, COLOR_PAIR(10) | A_DIM);        
            row_pad++;
        }

        // --- DENTRO DO LOOP FOR DA SUA FUNÇÃO DE BARRAS ---
        int aproveitamento_puro = strength_planets[i]; // Agora isso é a porcentagem pura (ex: 84)

        // 1. Definição de cores justa e democratizada (Mercúrio fica verde!)
        if (aproveitamento_puro >= 65)      wattron(pad, COLOR_PAIR(12) | A_BOLD); // Verde
        else if (aproveitamento_puro >= 35) wattron(pad, COLOR_PAIR(8) | A_BOLD);  // Azul
        else                                wattron(pad, COLOR_PAIR(11) | A_BOLD); // Vermelho

        if (aproveitamento_puro >= 65)      wattron(pad, COLOR_PAIR(12) | A_REVERSE);
        mvwprintw(pad, row_pad, 2, "  %s %-14s", plots[i].object, plots[i].object_name);
        if (aproveitamento_puro >= 65)      wattroff(pad, A_REVERSE);

        // 2. O tamanho da barra continua respeitando o peso arquetípico do planeta!
        double weights[100];
        get_weights(weights, show_modern_planets);
        
        // Multiplica a nota pelo peso e divide por 10.0 para achar a força ponderada
        double forca_ponderada = ((double)aproveitamento_puro * weights[i]) / 10.0;
        int strength_pontos = (int)ceil(forca_ponderada);

        // Mapeia o tamanho visual da barra de blocos na tela
        int num_blocos = (int)(forca_ponderada / 1.5);
        if (num_blocos > 40) num_blocos = 40;
        if (num_blocos < 1 && strength_pontos > 0) num_blocos = 1;

        wmove(pad, row_pad, 23);
        for (int b = 0; b < num_blocos; b++) wprintw(pad, "█");

        wattroff(pad, COLOR_PAIR(12) | COLOR_PAIR(8) | COLOR_PAIR(11) | A_BOLD);
        
        // Exibe os pontos ponderados reais na direita (Sol/Lua chegam a 50+, Mercúrio a 34)
        mvwprintw(pad, row_pad, 68, "[ %3d pts ]", strength_pontos);

        row_pad++;
    }

    // Rodapé fixo na janela principal
    mvwprintw(table_win, table_height - 1, 4, _("Press Q or ESC to return - Use [↓↑ / JK] to scroll"));
    wrefresh(table_win);

    // 5. MOTOR DE CONTROLE E TRAVAMENTO DE SCROLL AUTOMÁTICO
    int offset_y = 0;
    int max_scroll = row_pad - max_linhas_dados_visiveis;
    if (max_scroll < 0) max_scroll = 0;

    // Renderiza a primeira foto da PAD na tela
    prefresh(pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);

    keypad(pad, TRUE);
    int ch;
    while ((ch = wgetch(pad)) != 27 && ch != 'q' && ch != 'Q') {
        switch (ch) {
            case KEY_UP: 
            case 'k': 
            case 'K':
                if (offset_y > 0) offset_y -= 2;
                break;
                
            case KEY_DOWN: 
            case 'j': 
            case 'J':
                if (offset_y < max_scroll) offset_y += 2;
                break;
        }
        // Atualiza a janela de visualização do scroll a cada clique do usuário
        prefresh(pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);
    }

    delwin(pad);
    delwin(shadow_win);
    delwin(table_win);
}


void display_force(PlotObject *plots, PlanetDignities *dig, int *strength_planets) {
        
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 24;
    if (table_height > max_y - 2) table_height = max_y - 2; 
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = (max_x - table_width) / 2;

    int object_diff = show_modern_planets ? 0 : 3;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13));
    
    const char *title = _(" Planetary Strength ");
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);

    mvwprintw(table_win, 2, 4, _("Planet    Essential    Accidental        Score          Weight        Strength"));
    mvwprintw(table_win, 3, 2, "──────────────────────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, A_BOLD);

    double weights[100];
    get_weights(weights, show_modern_planets);

    int max_linhas_dados = table_height - 8;
    WINDOW *pad = newpad(40, table_width - 4);
    wbkgd(pad, COLOR_PAIR(13));

    int row_pad = 0;
    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        // if ((show_modern_planets && (i >= 14 && i <= 17)) || 
        //     (!show_modern_planets && (i >= 13 && i <= 14))) {
        //     continue;
        // }

        if (row_pad > 0) {
            wattron(pad, COLOR_PAIR(10) | A_DIM);
            mvwprintw(pad, row_pad, 2, "────────────────────────────────────────────────────────────────────────────────────────────────"); 
            wattroff(pad, COLOR_PAIR(10) | A_DIM);        
            row_pad++;
        }
        
        /* CORREÇÃO 1: Captura o aproveitamento (0 a 100) direto do seu array reformulado */
        int aproveitamento_puro = strength_planets[i];

        /* CORREÇÃO 2: Aplica os mesmos filtros de cores tripartidos das barras (65% e 35%) */
        if (aproveitamento_puro >= 65) {
            wattron(pad, COLOR_PAIR(12) | A_BOLD | A_REVERSE);  // VERDE: Excelente! (Mercúrio brilha aqui)
        } else if (aproveitamento_puro >= 35) {
            wattron(pad, COLOR_PAIR(8) | A_BOLD);   // AZUL: Moderado
        } else {
            wattron(pad, COLOR_PAIR(11) | A_BOLD);  // VERMELHO: Crítico
        }

        /* CORREÇÃO 3: Calcula a força ponderada final em pontos para exibir na última coluna */
        double forca_ponderada = ((double)aproveitamento_puro * weights[i]) / 10.0;
        int strength_pontos = (int)ceil(forca_ponderada);

        // Impressão limpa alinhada por colunas fixas na PAD
        mvwprintw(pad, row_pad, 2, "  %s", plots[i].object);
        mvwprintw(pad, row_pad, 14, "%3d", dig[i].essential);
        mvwprintw(pad, row_pad, 27, "%3d", dig[i].accidental);
        
        /* Exibe a porcentagem pura (ex: 83.00) coletada do array */
        mvwprintw(pad, row_pad, 41, "%8.2f", (double)aproveitamento_puro);
        
        mvwprintw(pad, row_pad, 56, "%8.2f", weights[i]);
        
        /* Exibe a força ponderada calculada localmente (ex: 34 pontos para Mercúrio, 54 para Lua) */
        mvwprintw(pad, row_pad, 72, "%3d points", strength_pontos);
        
        wattroff(pad, COLOR_PAIR(12) | COLOR_PAIR(8) | COLOR_PAIR(11) | A_BOLD | A_DIM | A_REVERSE);
        row_pad++;
    }

    mvwprintw(table_win, table_height - 1, 2, _("Press Q or ESC to return - [↓↑] to scroll"));
    wrefresh(table_win);

    int offset_y = 0;
    int max_scroll = row_pad - max_linhas_dados;
    if (max_scroll < 0) max_scroll = 0;

    prefresh(pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);

    keypad(pad, TRUE);
    int ch;
    while ((ch = wgetch(pad)) != 27 && ch != 'q' && ch != 'Q') {
        switch (ch) {
            case KEY_UP: case 'k': case 'K':
                if (offset_y > 0) offset_y -= 2;
                break;
            case KEY_DOWN: case 'j': case 'J':
                if (offset_y < max_scroll) offset_y += 2;
                break;
        }
        prefresh(pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);
    }
    
    delwin(pad);
    delwin(shadow_win);
    delwin(table_win);
}


void display_dignities(PlotObject *plots, PlanetDignities *dig, int *strength_planets) {   

    // Create a new window for the table
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Calculate window size and position
    int table_height = 24;
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = 2;

    int object_diff;
    if (show_modern_planets) {
        object_diff = 0;
    } else {
        object_diff = 3;
    }
    
    // Create window
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13));
    
    wattron(table_win, A_BOLD);
    const char *title = _("Accidental Dignities");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
    
    mvwprintw(table_win, 2, 2, _("Object"));
    mvwprintw(table_win, 2, 10, _("Position"));
    mvwprintw(table_win, 2, 26, _("Ess"));
    mvwprintw(table_win, 2, 31, _("Acc"));
    mvwprintw(table_win, 2, 36, _("Mov"));
    mvwprintw(table_win, 2, 41, _("Speed"));
    mvwprintw(table_win, 2, 48, _("House"));
    mvwprintw(table_win, 2, 55, _("State / Conditions"));
    mvwprintw(table_win, 2, 95, _("Aspects (B/M)"));
    mvwprintw(table_win, 2, 112, "Hayz");
    mvwprintw(table_win, 2, 120, _("Mut.Rec"));
    wattroff(table_win, A_BOLD);

    wattron(table_win, COLOR_PAIR(13));
    mvwprintw(table_win, 3, 2, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(13));

    wrefresh(table_win);

    // 1. CRIAÇÃO DA PAD VIRTUAL DE ROLAGEM
    // Definimos uma largura horizontal abundante (145 colunas) para acomodar os dados na horizontal
    int max_linhas_dados_visiveis = table_height - 6; 
    WINDOW *scroll_pad = newpad(40, 145); 
    wbkgd(scroll_pad, COLOR_PAIR(13));

    int row = 0;

    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        // if ((show_modern_planets && (i >= 14 && i <= 17)) || 
        //     (!show_modern_planets && (i >= 13 && i <= 14))) {
        //     continue;
        // }

        wattron(scroll_pad, COLOR_PAIR(10) | A_DIM);
        mvwprintw(scroll_pad, row - 1, 0, "──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(scroll_pad, COLOR_PAIR(10) | A_DIM);

        // 1. Objeto / Planeta
        wattron(scroll_pad, A_BOLD);
        mvwprintw(scroll_pad, row, 2, "%s", plots[i].object);
        wattroff(scroll_pad, A_BOLD);
        
        // 2. Coordenadas Básicas
        if (get_visual_width(plots[i].degree) == 2) {
            mvwprintw(scroll_pad, row, 9, "%s", plots[i].degree);
        }
        else { 
            mvwprintw(scroll_pad, row, 8, "%s", plots[i].degree);
        }
        mvwprintw(scroll_pad, row, 12, "%s", plots[i].sign);
        mvwprintw(scroll_pad, row, 15, "%s'", plots[i].min);
        
        // 3. Pontuações de Dignidade (Essencial e Acidental)
        if (dig[i].essential > 0) {
            wattron(scroll_pad, COLOR_PAIR(8));
        } else if (dig[i].essential < 0) {
            wattron(scroll_pad, COLOR_PAIR(11));
        }        
        mvwprintw(scroll_pad, row, 24, "%+2d", dig[i].essential);
        wattroff(scroll_pad, COLOR_PAIR(8) | COLOR_PAIR(11));

        if (dig[i].accidental > 0) {
            wattron(scroll_pad, COLOR_PAIR(8));
        } else if (dig[i].accidental < 0) {
            wattron(scroll_pad, COLOR_PAIR(11));
        } 
        mvwprintw(scroll_pad, row, 29, "%+3d", dig[i].accidental);
        wattroff(scroll_pad, COLOR_PAIR(8) | COLOR_PAIR(11));

        // 4. Movimento (1 = Direto, 0 = Estacionário, -1 = Retrógrado)
        if (i < 12 - object_diff) {
            if (dig[i].row.movement == -1) wattron(scroll_pad, COLOR_PAIR(11));
            else if (dig[i].row.movement == 1) wattron(scroll_pad, COLOR_PAIR(8));

            mvwprintw(scroll_pad, row, 34, (dig[i].row.movement == 1) ? "Dir" : (dig[i].row.movement == 0) ? "Est" : "Ret");
            wattroff(scroll_pad, COLOR_PAIR(8) | COLOR_PAIR(11));
        }
        
        // 5. Velocidade (1 = Fast, 0 = Mean, -1 = Slow)
        if (plots[i].id < P_FORTUNA - object_diff) {
            if (dig[i].row.fast == 1) wattron(scroll_pad, COLOR_PAIR(8));
            else if (dig[i].row.fast == -1) wattron(scroll_pad, COLOR_PAIR(11));

            mvwprintw(scroll_pad, row, 39, (dig[i].row.fast == 1) ? _("Fast") : (dig[i].row.fast == 0) ? _("Mean") : _("Slow"));
            wattroff(scroll_pad, COLOR_PAIR(8) | COLOR_PAIR(11));
        }
        
        
        
        // 6. Casa Astrológica
        if (strcmp(plots[i].house, "VI") == 0 || strcmp(plots[i].house, "VIII") == 0 || strcmp(plots[i].house, "XII") == 0) {
            wattron(scroll_pad, COLOR_PAIR(11));            
        }
        else if (strcmp(plots[i].house, "I") == 0 || strcmp(plots[i].house, "II") == 0 || strcmp(plots[i].house, "III") == 0 || 
                strcmp(plots[i].house, "V") == 0 || strcmp(plots[i].house, "IX") == 0 || strcmp(plots[i].house, "X") == 0 || strcmp(plots[i].house, "XI") == 0) {
                    wattron(scroll_pad, COLOR_PAIR(8));
        }
        else if (strcmp(plots[i].house, "IV") == 0 || strcmp(plots[i].house, "VII") == 0) {
            wattron(scroll_pad, COLOR_PAIR(7));
        }
        
        mvwprintw(scroll_pad, row, 47, "%s", plots[i].house);
        wattroff(scroll_pad, COLOR_PAIR(8) | COLOR_PAIR(11) | COLOR_PAIR(7));

        // 7. Condições Especiais
        int cond_x = 53;
        if (dig[i].row.cazimi) { 
            wattron(scroll_pad, COLOR_PAIR(8)); 
            mvwprintw(scroll_pad, row, cond_x, "Cazimi "); 
            cond_x += 7; 
            wattroff(scroll_pad, COLOR_PAIR(8));
        } else if (dig[i].row.combust) { 
            wattron(scroll_pad, COLOR_PAIR(11)); 
            mvwprintw(scroll_pad, row, cond_x, "Combust "); 
            cond_x += 8; 
            wattroff(scroll_pad, COLOR_PAIR(11));
        }
        
        if (dig[i].row.under_rays == 1) { 
            wattron(scroll_pad, COLOR_PAIR(11)); 
            mvwprintw(scroll_pad, row, cond_x, "SubRays "); 
            cond_x += 8; 
            wattroff(scroll_pad, COLOR_PAIR(11));
        } else if (dig[i].row.under_rays == -1) { 
            wattron(scroll_pad, COLOR_PAIR(8)); 
            mvwprintw(scroll_pad, row, cond_x, "FreeRays "); 
            cond_x += 9; 
            wattroff(scroll_pad, COLOR_PAIR(8));
        }
        
        if (dig[i].row.feral) { 
            wattron(scroll_pad, COLOR_PAIR(11)); 
            mvwprintw(scroll_pad, row, cond_x, "Feral ");
            wattroff(scroll_pad, COLOR_PAIR(11));
            cond_x += 6; 
        }
        if (dig[i].row.void_of_course) {
            wattron(scroll_pad, COLOR_PAIR(11)); 
            mvwprintw(scroll_pad, row, cond_x, _("VoC "));
            wattroff(scroll_pad, COLOR_PAIR(11));
            cond_x += 4; 
        }
        
        if (dig[i].row.under_siege) { 
            wattron(scroll_pad, COLOR_PAIR(11)); 
            mvwprintw(scroll_pad, row, cond_x, _("Siege ")); cond_x += 6; 
            wattroff(scroll_pad, COLOR_PAIR(11));
        }
        if (dig[i].row.under_assistance) { 
            wattron(scroll_pad, COLOR_PAIR(8)); 
            mvwprintw(scroll_pad, row, cond_x, "Assist "); cond_x += 7; 
            wattroff(scroll_pad, COLOR_PAIR(8));
        }
        if (i < 7) {
            if (dig[i].row.joy) {
                wattron(scroll_pad, COLOR_PAIR(8)); 
                mvwprintw(scroll_pad, row, cond_x, _("Joy ")); cond_x += 4; 
                wattroff(scroll_pad, COLOR_PAIR(8));
            }
            if (dig[i].row.sign_joy) {
                wattron(scroll_pad, COLOR_PAIR(8)); 
                mvwprintw(scroll_pad, row, cond_x, _("JoyS ")); cond_x += 5; 
                wattroff(scroll_pad, COLOR_PAIR(8));
            }
        }
        if (i < 7 && i > 0) {
            char *planet_orientality = " ";
            int found = get_planet_orientality(plots[i].object, &planet_orientality);
       
            if (dig[i].row.orientality) { 
                wattron(scroll_pad, COLOR_PAIR(8));
                if (strcmp(planet_orientality, "Oriental") == 0) {
                    mvwprintw(scroll_pad, row, cond_x, "Orient "); cond_x += 7; 
                }
                else {
                    mvwprintw(scroll_pad, row, cond_x, _("Occid ")); cond_x += 6; 
                }                
                wattroff(scroll_pad, COLOR_PAIR(8));
            }
            else {
                wattron(scroll_pad, COLOR_PAIR(11));
                if (strcmp(planet_orientality, "Oriental") == 0) {
                    mvwprintw(scroll_pad, row, cond_x, _("Occid ")); cond_x += 6; 
                }
                else {
                    mvwprintw(scroll_pad, row, cond_x, "Orient "); cond_x += 7; 
                }                
                wattroff(scroll_pad, COLOR_PAIR(11));
            }
            if (found) free(planet_orientality);
        }

        // 8. Aspectos com Benéficos / Maléficos e Nodos
        int asp_x = 93;
        wattron(scroll_pad, COLOR_PAIR(8));
        if (dig[i].row.asp_benef_conj)    { mvwprintw(scroll_pad, row, asp_x, "BN_☌ "); asp_x += 5; }
        if (dig[i].row.asp_benef_trine)   { mvwprintw(scroll_pad, row, asp_x, "BN_△ "); asp_x += 5; }
        if (dig[i].row.asp_benef_sextile) { mvwprintw(scroll_pad, row, asp_x, "BN_⚹ "); asp_x += 5; }
        if (dig[i].row.north_node_conj)   { mvwprintw(scroll_pad, row, asp_x, "☊_☌ "); asp_x += 4; }
        wattroff(scroll_pad, COLOR_PAIR(8));

        wattron(scroll_pad, COLOR_PAIR(11));
        if (dig[i].row.asp_malef_conj)    { mvwprintw(scroll_pad, row, asp_x, "ML_☌ "); asp_x += 5; }
        if (dig[i].row.asp_malef_opp)     { mvwprintw(scroll_pad, row, asp_x, "ML_☍ "); asp_x += 5; }
        if (dig[i].row.asp_malef_square)  { mvwprintw(scroll_pad, row, asp_x, "ML_□ "); asp_x += 5; }
        if (dig[i].row.south_node_conj)   { mvwprintw(scroll_pad, row, asp_x, "☋_☌ "); asp_x += 4; }
        wattroff(scroll_pad, COLOR_PAIR(11));

        // 9. Condição de Hayz / Haym
        if (dig[i].row.hayz == 1) {
            wattron(scroll_pad, COLOR_PAIR(8)); 
            mvwprintw(scroll_pad, row, 110, "Hayz  "); 
            wattroff(scroll_pad, COLOR_PAIR(8));
        } else if (dig[i].row.hayz == -1) {
            wattron(scroll_pad, COLOR_PAIR(11)); 
            mvwprintw(scroll_pad, row, 110, "AntiHz"); 
            wattroff(scroll_pad, COLOR_PAIR(11));
        } else if (dig[i].row.haym) {
            wattron(scroll_pad, COLOR_PAIR(8)); 
            mvwprintw(scroll_pad, row, 110, "Haym  "); 
            wattroff(scroll_pad, COLOR_PAIR(8));
        }

        // 10. Recepção Mútua
        if (dig[i].row.mut_reception) {
            wattron(scroll_pad, COLOR_PAIR(8));
            mvwprintw(scroll_pad, row, 118, "✅");
            if (dig[i].row.mut_reception_asp) {
                mvwprintw(scroll_pad, row, 121, "(Asp)");
            }
            wattroff(scroll_pad, COLOR_PAIR(8));
        }

        row += 2;
    }
    
    // Add instructions
    mvwprintw(table_win, table_height - 1, 2, _("Press ESC/Q to close - F3 Strength - F4 Energy Profile - [↓↑/JK] Scroll"));
    
    // Refresh the window
    wrefresh(table_win);

    // MOTOR DE CONTROLE, TRAVAMENTO E ROLAGEM VERTICAL
    int offset_y = 0;
    int max_scroll_y = row - max_linhas_dados_visiveis + 2;
    if (max_scroll_y < 0) max_scroll_y = 0;

    // Vincula o teclado à PAD virtual
    keypad(scroll_pad, TRUE);
    nodelay(scroll_pad, FALSE);

    // Renderiza a primeira foto da PAD na tela
    prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);

    int ch;
    while ((ch = wgetch(scroll_pad)) != 27 && ch != 'q' && ch != 'Q') {
        
        if (ch == KEY_F(3)) {
            display_force(plots, dig, strength_planets);
            
            touchwin(shadow_win); wrefresh(shadow_win);
            touchwin(table_win);  wrefresh(table_win);
            
            // REDESENHO CRÍTICO DA PAD
            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);
        }
        else if (ch == KEY_F(4)) {
            display_planetary_energy_profile(plots, strength_planets);

            touchwin(shadow_win); wrefresh(shadow_win);
            touchwin(table_win);  wrefresh(table_win);
            
            // REDESENHO CRÍTICO DA PAD
            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);
        }
        else {
            // Se não foi nenhuma tecla de função, processa a rolagem vertical do texto
            switch (ch) {
                case KEY_UP: 
                case 'k': 
                case 'K':
                    if (offset_y > 0) offset_y -= 2;
                    break;
                    
                case KEY_DOWN: 
                case 'j': 
                case 'J':
                    if (offset_y < max_scroll_y) offset_y += 2;
                    break;
            }
            // Atualiza os frames da PAD na tela após o movimento de subida/descida
            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);
        }
    }
    
    // CLEAN UP: Desaloca todas as janelas do escopo e devolve o controle para a stdscr limpa
    delwin(scroll_pad);
    delwin(shadow_win);
    delwin(table_win);
    
    touchwin(stdscr); 
    refresh();
}



void display_table_data(bool mapa_retorno, double jd, struct tm *local_time, double lat, double lon, double elev, PlotObject *plots, char *season,
    int sanYear, int sanMon, int sanDay, double sanHour, char *sunrise_time, char *sunset_time, char *next_sunrise_time,
    char *city, char *country, const char* phase, char *temperament,
    int last_hr, int last_min, double last_sec, char *chart_name, int gender_id) {
    
        // Create a new window for the table
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Calculate window size and position
    int table_height = 26;
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = 2;

    int object_diff = 0;
    if (show_modern_planets) {
        object_diff = 0;
    }
    else {
        object_diff = 3;
    }    
    
    // Create window
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    // Create a border around the table
    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13));

    
    
    // Add title
    wattron(table_win, A_BOLD);
    const char *title = _("Chart Data");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);

    mvwprintw(table_win, 5, 2, "%s", chart_name); 
    wattroff(table_win, A_BOLD);

    wattron(table_win, A_DIM);
    mvwprintw(table_win, 6, 2, "─────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, A_DIM);

    wattron(table_win, A_BOLD);

    if (mapa_retorno) {
        wattron(table_win, COLOR_PAIR(11));
        mvwprintw(table_win, 7, 2, _("Solar Revolution Chart"));
        wattroff(table_win, COLOR_PAIR(11));
    }
    else {
        wattroff(table_win, A_BOLD);
        mvwprintw(table_win, 7, 2, _("Radix Chart")); 
        wattron(table_win, A_BOLD);
    }
    mvwprintw(table_win, 10, 2, _("Location:")); 
    wattroff(table_win, A_BOLD);
    mvwprintw(table_win, 10, 12, "%s, %s", city, country);
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 14, 2, _("Time: "));
    wattroff(table_win, A_BOLD);
    mvwprintw(table_win, 14, 15, "%d/%d/%d %02d:%02d:%02d TZ, %s", local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday, (local_time->tm_hour), local_time->tm_min, local_time->tm_sec, str_dow(local_time->tm_wday));
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 15, 2, _("Julian Day: "));
    wattroff(table_win, A_BOLD);
    mvwprintw(table_win, 15, 15, "%f", jd);
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 11, 2, "Lat:");  
    mvwprintw(table_win, 11, 15, ", Lon:     ");
    mvwprintw(table_win, 11, 30, ", Elev:     ");
    wattroff(table_win, A_BOLD);
    mvwprintw(table_win, 11, 7, "%.4f", lat);
    mvwprintw(table_win, 11, 22, "%.4f", lon);
    mvwprintw(table_win, 11, 38, "%.4f", elev);
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 10, 55, _("Sunrise:"));
    wattroff(table_win, A_BOLD);
    mvwprintw(table_win, 10, 72, "%s", sunrise_time);
    wattroff(table_win, A_BOLD);
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 11, 55, _("Sunset:"));
    wattroff(table_win, A_BOLD);
    mvwprintw(table_win, 11, 72, "%s", sunset_time);
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 12, 55, _("Next Sunrise:"));
    wattroff(table_win, A_BOLD);
    mvwprintw(table_win, 12, 72, "%s", next_sunrise_time);
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 14, 55, _("Local Apparent Solar Time:"));
    wattroff(table_win, A_BOLD);
    mvwprintw(table_win, 14, 86, "%02d:%02d:%07.4f", last_hr, last_min, last_sec);
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 16, 55, _("Moon Phase:"));
    wattroff(table_win, A_BOLD);
    mvwprintw(table_win, 16, 68, "%s (%s)", phase, temperament);

    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 17, 55, _("Syzygy Ante-Nativitatem (SAN):"));
    wattroff(table_win, A_BOLD);

    int san_h = (int)sanHour;
    int san_min = (int)((sanHour - san_h) * 60.0);
    double san_sec_decimal = (((sanHour - san_h) * 60.0) - san_min) * 60.0;
    int san_seconds = (int)san_sec_decimal;
    mvwprintw(table_win, 17, 87, "%s %04d/%02d/%02d (%02d:%02d:%02d) Local", plots[P_SAN - object_diff].object, sanYear, sanMon, sanDay, san_h, san_min, san_seconds);

    if (strcmp(plots[P_SAN - object_diff].object, "🌑") == 0) {
        mvwprintw(table_win, 18, 55, _("Conjunctional pre-natal syzygy - New Moon"));
    }
    else {
        mvwprintw(table_win, 18, 55, _("Preventional pre-natal syzygy - Full Moon"));
    }

    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 20, 55, _("Astrological Season:"));
    wattroff(table_win, A_BOLD);
    mvwprintw(table_win, 20, 76, _("%s (North Hemisphere)"), season);


    //char *hsol = plots[0].house;
    wattron(table_win, A_BOLD);
    //if (mapa_diurno(hsol)) {
    if (MAPA_DIURNO) {
        mvwprintw(table_win, 7, 55, "%s", _("Diurnal chart"));    
    }
    else {
        mvwprintw(table_win, 7, 55, "%s", _("Nocturnal chart"));    
    }

    mvwprintw(table_win, 8, 55, "%s", (gender_id == 1)?_("Masculine chart"):((gender_id == 2)?_("Feminine chart"):_("Neuter gender chart")));    
    wattroff(table_win, A_BOLD);
    
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
    touchwin(stdscr); 
    refresh();
}

 

void display_table(PlotObject *plots, PlanetTableMatrix *matrix, PlanetDignities *dig, int *strength_planets) {

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Dimensionamento responsivo da janela física externa
    int table_height = 24;
    if (table_height > max_y - 2) table_height = max_y - 2;
    int table_width = max_x - 5;

    int start_y = (max_y - table_height) / 2;
    int start_x = 2;

    int object_diff = show_modern_planets ? 0 : 3;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13));

    // Cabeçalho Fixo na Janela de Borda (Não rola)
    wattron(table_win, A_BOLD);
    const char *title = _("Positions, Dignities & Rulership Table");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);
    
    mvwprintw(table_win, 2, 2, _("Object"));
    mvwprintw(table_win, 2, 10, _("Position"));
    mvwprintw(table_win, 2, 24, _("Decl"));
    mvwprintw(table_win, 2, 32, _("Speed"));
    mvwprintw(table_win, 2, 41, _("House"));
    mvwprintw(table_win, 2, 48, _("Mov"));
    mvwprintw(table_win, 2, 53, _("Dignity"));
    mvwprintw(table_win, 2, 77, _("Gen/Sect/Quad"));
    mvwprintw(table_win, 2, 92, _("Orient"));
    mvwprintw(table_win, 2, 100, _("Dec"));
    mvwprintw(table_win, 2, 104, _("Term"));
    mvwprintw(table_win, 2, 109, _("Trip"));
    mvwprintw(table_win, 2, 116, _("Dom/Ex"));
    mvwprintw(table_win, 2, 123, _("Mut.Rec"));
    wattroff(table_win, A_BOLD);

    wattron(table_win, COLOR_PAIR(13));
    mvwprintw(table_win, 3, 2, "────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, COLOR_PAIR(13));
    wrefresh(table_win);

    // 1. CRIAÇÃO DA PAD VIRTUAL DE ROLAGEM
    // Definimos uma largura horizontal abundante (145 colunas) para acomodar os dados na horizontal
    int max_linhas_dados_visiveis = table_height - 6; 
    WINDOW *scroll_pad = newpad(40, 145); 
    wbkgd(scroll_pad, COLOR_PAIR(13));

    int row_pad = 0;
    // Ajustamos as coordenadas horizontais para casar com a PAD a partir do zero
    int c_obj = 0, c_pos = 8, c_dec = 22, c_spd = 30, c_hse = 39, c_mov = 46, c_dig = 51;
    int c_sq = 75, c_ori = 90, c_dec_t = 97, c_trm = 101, c_tri = 105, c_rul = 113, c_mut = 119;

    for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        // if ((show_modern_planets && (i >= 14 && i <= 17)) || 
        //     (!show_modern_planets && (i >= 13 && i <= 14))) {
        //     continue;
        // }

        if (row_pad > 0) {
            wattron(scroll_pad, COLOR_PAIR(10) | A_DIM);
            mvwprintw(scroll_pad, row_pad - 1, c_obj, "────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────"); 
            wattroff(scroll_pad, COLOR_PAIR(10) | A_DIM);
        }

        PlanetRowData data = matrix->rows[i];

        // 1. Objeto / Planeta
        wattron(scroll_pad, A_BOLD);
        mvwprintw(scroll_pad, row_pad, c_obj + 2, "%s", plots[i].object);
        wattroff(scroll_pad, A_BOLD);
        
        // 2. Coordenadas Básicas
        if (get_visual_width(plots[i].degree) == 2) {
            mvwprintw(scroll_pad, row_pad, c_pos + 1, "%s", plots[i].degree);
        } else { 
            mvwprintw(scroll_pad, row_pad, c_pos, "%s", plots[i].degree);
        }
        mvwprintw(scroll_pad, row_pad, c_pos + 4, "%s", plots[i].sign);
        mvwprintw(scroll_pad, row_pad, c_pos + 7, "%s'", plots[i].min);
        
        if (isnan(plots[i].declination)) {
            mvwprintw(scroll_pad, row_pad, c_dec, " ");
        } else {
            mvwprintw(scroll_pad, row_pad, c_dec, "%s", data.decl_str);
        }
        
        // 3. Velocidade com cor condicional
        if (data.speed_color_pair > 0) {
            wattron(scroll_pad, COLOR_PAIR(data.speed_color_pair));
        }
        mvwprintw(scroll_pad, row_pad, c_spd, "%s", data.speed_str);
        if (data.speed_color_pair > 0) {
            wattroff(scroll_pad, COLOR_PAIR(data.speed_color_pair));
        }
        
        // 4. Casa com cor condicional
        if (data.house_color_pair > 0) {
            wattron(scroll_pad, COLOR_PAIR(data.house_color_pair));
        }
        mvwprintw(scroll_pad, row_pad, c_hse + 1, "%s", plots[i].house);
        if (data.house_color_pair > 0) {
            wattroff(scroll_pad, COLOR_PAIR(data.house_color_pair));
        }
        
        // 5. Movimento (Retrogrado)
        wattron(scroll_pad, COLOR_PAIR(11));
        mvwprintw(scroll_pad, row_pad, c_mov, "%s", plots[i].retrograde);
        wattroff(scroll_pad, COLOR_PAIR(11));
        
        // 6. Dignidades Essenciais
        wattron(scroll_pad, COLOR_PAIR(data.dignity_color_pair));
        mvwprintw(scroll_pad, row_pad, c_dig, "%s", data.dignity_str);
        wattroff(scroll_pad, COLOR_PAIR(data.dignity_color_pair));

        // 7. Conformidade de Gênero / Seita / Quadrante
        if (data.gender_match)   mvwprintw(scroll_pad, row_pad, c_sq, "✅");
        if (data.sect_match)     mvwprintw(scroll_pad, row_pad, c_sq + 4, "✅");
        if (data.quadrant_match) mvwprintw(scroll_pad, row_pad, c_sq + 8, "✅");

        // 8. Orientalidade
        if (i > 0 && i < 12 - object_diff) {
            wattron(scroll_pad, COLOR_PAIR(data.orientality_color_pair));
            mvwprintw(scroll_pad, row_pad, c_ori, "%s", data.orientality_str);
            wattroff(scroll_pad, COLOR_PAIR(data.orientality_color_pair));
        }

        // 9. Sub-Regências
        mvwprintw(scroll_pad, row_pad, c_dec_t + 1, "%s", data.decan);
        mvwprintw(scroll_pad, row_pad, c_trm + 2, "%s", data.term);
        mvwprintw(scroll_pad, row_pad, c_tri + 2, "%s", data.tri);
        mvwprintw(scroll_pad, row_pad, c_rul + 2, "%s", data.rulers_str);
        mvwprintw(scroll_pad, row_pad, c_mut + 1, "%s", data.mutual_reception);
        
        row_pad += 2;
    }

    // Adiciona as instruções fixas no rodapé da janela externa (table_win)
    mvwprintw(table_win, table_height - 1, 2, _("Press ESC/Q to close - F2 Dignities - F3 Strength - F4 Energy Profile - [↓↑/JK] Scroll"));
    wrefresh(table_win);

    // MOTOR DE CONTROLE, TRAVAMENTO E ROLAGEM VERTICAL
    int offset_y = 0;
    int max_scroll_y = row_pad - max_linhas_dados_visiveis + 2;
    if (max_scroll_y < 0) max_scroll_y = 0;

    // Vincula o teclado à PAD virtual
    keypad(scroll_pad, TRUE);
    nodelay(scroll_pad, FALSE);

    // Renderiza a primeira foto da PAD na tela
    prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);

    int ch;
    while ((ch = wgetch(scroll_pad)) != 27 && ch != 'q' && ch != 'Q') {
        
        if (ch == KEY_F(2)) {
            // Abre sua tela de detalhes das dignidades
            display_dignities(plots, dig, strength_planets);
            
            // Restaura as molduras fixas da tabela principal
            touchwin(shadow_win); wrefresh(shadow_win);
            touchwin(table_win);  wrefresh(table_win);
            
            // REDESENHO CRÍTICO DA PAD: Força o ncurses a recolocar as linhas da tabela na tela
            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);
        }
        else if (ch == KEY_F(3)) {
            // Abre sua tabela de forças limpa (passando as variáveis que vieram por parâmetro)
            display_force(plots, dig, strength_planets);
            
            touchwin(shadow_win); wrefresh(shadow_win);
            touchwin(table_win);  wrefresh(table_win);
            
            // REDESENHO CRÍTICO DA PAD
            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);
        }
        else if (ch == KEY_F(4)) {
            // Abre o novo perfil gráfico com barras horizontais (█)
            display_planetary_energy_profile(plots, strength_planets);

            touchwin(shadow_win); wrefresh(shadow_win);
            touchwin(table_win);  wrefresh(table_win);
            
            // REDESENHO CRÍTICO DA PAD
            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);
        }
        else {
            // Se não foi nenhuma tecla de função, processa a rolagem vertical do texto
            switch (ch) {
                case KEY_UP: 
                case 'k': 
                case 'K':
                    if (offset_y > 0) offset_y -= 2;
                    break;
                    
                case KEY_DOWN: 
                case 'j': 
                case 'J':
                    if (offset_y < max_scroll_y) offset_y += 2;
                    break;
            }
            // Atualiza os frames da PAD na tela após o movimento de subida/descida
            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 3, start_x + table_width - 3);
        }
    }
    
    // CLEAN UP: Desaloca todas as janelas do escopo e devolve o controle para a stdscr limpa
    delwin(scroll_pad);
    delwin(shadow_win);
    delwin(table_win);
    
    touchwin(stdscr); 
    refresh();
}




void display_houses(double *cusps, char pHouse[12][100], char **house_ruler, char *house_system) {
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 30;
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = 2;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
        
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(13));
    
    wattron(table_win, A_BOLD);
    const char *title = _("Houses Table");
    mvwprintw(table_win, 0, (table_width - get_visual_width(title)) / 2, title);

    mvwprintw(table_win, 1, 2, _("   House    Cusp         Rulers(Dom/Exalt)  Objects in the House                                House System:"));
    wattroff(table_win, A_BOLD);
    mvwprintw(table_win, 2, 98, "%s", house_system);

    int row = 3;
    for (int i = 1; i <= 12; i++) {

        wattron(table_win, COLOR_PAIR(10) | A_DIM);
        mvwprintw(table_win, row - 1, 2, "────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(10) | A_DIM);

        char house_num[4];
        snprintf(house_num, sizeof(house_num), "%02d", i);

        char sign_str[10];
        double sign_remainder = fmod(cusps[i], 30.0);
        int deg_cusp = (int)sign_remainder;        
        double min_cusp = (int)((sign_remainder - deg_cusp) * 60.0);

        snprintf(sign_str, sizeof(sign_str), "%s", get_sign((int)(cusps[i] / 30)));
        
        if (i == 1 || i == 4 || i == 7 || i == 10) {
            wattron(table_win, A_BOLD);
        }

        char angle[10] = "  ";
        switch(i) {
            case 1: 
                snprintf(angle, 10, "%s", "AC"); 
                break;
            case 4:
                snprintf(angle, 10, "%s", "IC"); 
                break;
            case 7: 
                snprintf(angle, 10, "%s", "DC"); 
                break;
            case 10: 
                snprintf(angle, 10, "%s", "MC"); 
                break;
            default: 
                snprintf(angle, 10, "%s", "  "); 
                break;
        }
                
        mvwprintw(table_win, row, 3, "%s %s      %02d° %s %02.0f'      %s\t\t%s", angle, house_num, deg_cusp, sign_str, min_cusp, house_ruler[i], pHouse[i - 1]);
        wattroff(table_win, A_BOLD);

        row += 2;

    }

    // Add instructions
    mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
    
    // Refresh the window
    wrefresh(table_win);

    keypad(table_win, TRUE);
    nodelay(table_win, FALSE);
    
    // Wait for ESC key press
    int ch;
    do {
        ch = wgetch(table_win);
    } while (ch != 27 && ch != 'q');
    
    // Clean up
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr); 
    refresh();
}


void display_hours(int week_day, double *hours, int planetary_hour, double daytime_hour, double nighttime_hour, int *strength_planets, PlanetDignities *dig) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 30;
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = 2;
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(6));

    wattron(table_win, A_BOLD);
    const char *title = _("Planetary Hours");
    mvwprintw(table_win, 0, (table_width - 17) / 2, title);
    
    int row = 4;
    
    mvwprintw(table_win, 2, 4, _("Day:"));
    mvwprintw(table_win, 2, 42, _("Night:"));
    wattroff(table_win, A_BOLD);

    void format_hour(char *buffer, size_t size, double total_hours) {
        
        int h = (int)total_hours;

        if (h < 0) {
            snprintf(buffer, size, "--:--:--");
            return; 
        }
        
        // Extrai os minutos da parte fracionária das horas
        double total_minutes = (total_hours - h) * 60.0;
        int m = (int)total_minutes;
        
        // Extrai os segundos da parte fracionária dos minutos
        // O +0.5 serve para arredondar corretamente em vez de truncar
        int s = (int)((total_minutes - m) * 60.0 + 0.5 / 3600000.0);
        
        // Ajuste opcional para o caso do arredondamento dar 60 segundos
        if (s >= 60) { s = 0; m++; }
        if (m >= 60) { m = 0; h++; }
    
        snprintf(buffer, size, "%02d:%02d:%02d", h, m, s);
    }

    for (int i = 0; i < 6; i++) {
        // Aumentado para 12 para caber "HH:MM:SS\0" com segurança
        char p_hour[12];
        char p_hour2[12];
        char p_hour3[12];
        char p_hour4[12];
        
        // Chamadas simples da função auxiliar
        
        format_hour(p_hour, sizeof(p_hour), hours[i]);
        format_hour(p_hour2, sizeof(p_hour2), hours[i+6]);
        format_hour(p_hour3, sizeof(p_hour3), hours[i+12]);
        format_hour(p_hour4, sizeof(p_hour4), hours[i+18]);
    
        wattron(table_win, COLOR_PAIR(15));
        if (i + 1 == planetary_hour) wattron(table_win, A_REVERSE);
        mvwprintw(table_win, row, 4, "%2d) %s  %s  ", i + 1, p_hour, planet_regent_symbols[get_hour_regent(week_day - 1, i)]);
        if (i + 1 == planetary_hour) wattroff(table_win, A_REVERSE);

        if (i + 1 + 6 == planetary_hour) wattron(table_win, A_REVERSE);
        mvwprintw(table_win, row, 24, "%2d) %s  %s  ", i + 1 + 6, p_hour2, planet_regent_symbols[get_hour_regent(week_day - 1, i + 6)]);
        if (i + 1 + 6 == planetary_hour) wattroff(table_win, A_REVERSE);
        wattroff(table_win, COLOR_PAIR(15) );

        wattron(table_win, COLOR_PAIR(12));
        if (i + 1 + 12 == planetary_hour) wattron(table_win, A_REVERSE);
        mvwprintw(table_win, row, 44, "%2d) %s  %s  ", i + 1 + 12, p_hour3, planet_regent_symbols[get_hour_regent(week_day - 1, i + 12)]);
        if (i + 1 + 12 == planetary_hour) wattroff(table_win, A_REVERSE);
        
        if (i + 1 + 18 == planetary_hour) wattron(table_win, A_REVERSE);
        mvwprintw(table_win, row, 64, "%2d) %s  %s  ", i + 1 + 18, p_hour4, planet_regent_symbols[get_hour_regent(week_day - 1, i + 18)]);
        if (i + 1 + 18 == planetary_hour) wattroff(table_win, A_REVERSE);
        wattroff(table_win, COLOR_PAIR(12));
        row += 2;
    }

    wattron(table_win, A_BOLD);
    mvwprintw(table_win, row + 1, 40, _("Next day (hour 1):"));
    
    char p_next_rise[12];
    format_hour(p_next_rise, sizeof(p_next_rise), hours[24]); // 25º elemento do array é o next sunrise    
    wattron(table_win, COLOR_PAIR(15));
    mvwprintw(table_win, row + 1, 64, "%2d) %s  %s  ", 1, p_next_rise, planet_regent_symbols[get_hour_regent((week_day % 7), 0)]);
    wattroff(table_win, COLOR_PAIR(15));

    char hour_len1[12];
    char hour_len2[12];

    format_hour(hour_len1, sizeof(hour_len1), daytime_hour);
    format_hour(hour_len2, sizeof(hour_len2), nighttime_hour);

    mvwprintw(table_win, row + 4, 4, "%s: %.4f (%s)", _("  Hour length (Day)"), daytime_hour, hour_len1);
    mvwprintw(table_win, row + 5, 4, "%s: %.4f (%s)", _("Hour length (Night)"), nighttime_hour, hour_len2);

    const char *regent_day_str = planet_regent_symbols[get_hour_regent(week_day - 1, (MAPA_DIURNO)?0:12)];
    const char *regent_hour_str = planet_regent_symbols[get_hour_regent(week_day - 1, planetary_hour - 1)];

    mvwprintw(table_win, row + 9, 4, "%s %s(%d)/%s(%d): %s / %s", _("Chart's Planetary"), (MAPA_DIURNO)?_("Day"):_("Night"), week_day, _("Hour"), planetary_hour, regent_day_str, regent_hour_str);
    
    
    const char **ascii_art = get_planet_ascii_by_gliph((char *)regent_day_str);
            
    if (MAPA_DIURNO ) {
       wattron(table_win, COLOR_PAIR(8));
    }
    else {
        wattron(table_win, COLOR_PAIR(7));
    } 

    mvwprintw(table_win, row + 4, 64, "%s", ascii_art[0]);
    mvwprintw(table_win, row + 5, 64, "%s", ascii_art[1]);
    mvwprintw(table_win, row + 6, 64, "%s", ascii_art[2]);
    mvwprintw(table_win, row + 7, 64, "%s", ascii_art[3]);
    mvwprintw(table_win, row + 8, 64, "%s", ascii_art[4]);
    mvwprintw(table_win, row + 9, 64, "%s", ascii_art[5]);
       
    const char **ascii_art1 = get_planet_ascii_by_gliph((char *)regent_hour_str);
            
    mvwprintw(table_win, row + 4, 72, "%s", ascii_art1[0]);
    mvwprintw(table_win, row + 5, 72, "%s", ascii_art1[1]);
    mvwprintw(table_win, row + 6, 72, "%s", ascii_art1[2]);
    mvwprintw(table_win, row + 7, 72, "%s", ascii_art1[3]);
    mvwprintw(table_win, row + 8, 72, "%s", ascii_art1[4]);
    mvwprintw(table_win, row + 9, 72, "%s", ascii_art1[5]);

    mvwprintw(table_win, row + 10, 63, "%s   %s", (MAPA_DIURNO)?_(" Day"):_("Night"), (MAPA_DIURNO)?_("  Hour"):_("Hour"));
       
    if (MAPA_DIURNO ) {
        wattroff(table_win, COLOR_PAIR(8) | A_BOLD);
     }
     else {
         wattroff(table_win, COLOR_PAIR(7) | A_BOLD);
     }
    
    mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart | [i] for interpretation."));
    
    // Refresh the window
    wrefresh(table_win);

    keypad(table_win, TRUE);
    nodelay(table_win, FALSE);
    
    // Wait for ESC key press
    int ch;
    do {
        ch = wgetch(table_win);
        
        /* GATILHO: Se pressionar 'i', abre o relatório corrido */
        if (ch == 'i' || ch == 'I') {
            int regente_dia = converter_codigo_planeta(get_hour_regent(week_day - 1, (MAPA_DIURNO)?0:12));
            int regente_hora = converter_codigo_planeta(get_hour_regent(week_day - 1, planetary_hour - 1));

            int strength_reg_day = strength_planets[regente_dia - 1];
            int strength_reg_hour = strength_planets[regente_hora - 1];

            int dig_reg_day = dig[regente_dia - 1].essential + dig[regente_dia - 1].accidental;
            int dig_reg_hour = dig[regente_hora - 1].essential + dig[regente_hora - 1].accidental;


            abrir_janela_interpretacao_horas(regente_dia, 
                                             regente_hora, 
                                             regent_day_str, 
                                             regent_hour_str,
                                             strength_reg_day,
                                             strength_reg_hour,
                                             dig_reg_day,
                                             dig_reg_hour);
            
            /* Ao fechar o relatório, redesenha a janela do painel para limpar resíduos */
            touchwin(table_win);
            wrefresh(table_win);
        }

    } while (ch != 27 && ch != 'q' && ch != 'Q');
    
    // Clean up
    delwin(shadow_win);
    delwin(table_win);
    touchwin(stdscr); 
    refresh();
}



void abrir_janela_interpretacao_horas(int regente_dia, int regente_hora, const char *regent_day_str, const char *regent_hour_str, int strength_reg_day, int strength_reg_hour, int dig_reg_day, int dig_reg_hour) {
    int p_max_y, p_max_x;
    getmaxyx(stdscr, p_max_y, p_max_x); 

    // 1. DIMENSIONAMENTO RESPONSIVO DA JANELA
    int i_height = p_max_y - 6;
    if (i_height > 24) i_height = 24; 
    int i_width = p_max_x - 12;
    if (i_width > 102) i_width = 102;   

    int i_start_y = (p_max_y - i_height) / 2;
    int i_start_x = (p_max_x - i_width) / 2;

    // 2. CRIAÇÃO E RENDERIZAÇÃO DA JANELA DE SOMBRA (FUNDO)
    WINDOW *shadow_win = newwin(i_height, i_width, i_start_y + 1, i_start_x + 1);
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9)); // Par de cor preta/escura para a sombra
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    // 3. CRIAÇÃO DA MOLDURA PRINCIPAL
    WINDOW *border_win = newwin(i_height, i_width, i_start_y, i_start_x);
    wbkgd(border_win, COLOR_PAIR(13));
    box(border_win, 0, 0);
    
    wattron(border_win, A_BOLD);
    const char *title = _(" Planetary Hours Analysis ");
    mvwprintw(border_win, 0, (i_width - get_visual_width(title)) / 2, title);
    wattroff(border_win, A_BOLD);
    
    mvwprintw(border_win, i_height - 1, (i_width - 44) / 2, _(" [↓↑|JK: Scroll | Q|ESC: Return] "));
    wrefresh(border_win);

    // 4. CRIAÇÃO DA PAD INTERNA COM MAIS ESPAÇO HORIZONTAL
    int pad_lines = 150; // Aumentado para suportar os novos espaços em branco
    int pad_cols = i_width - 6; // Margem lateral ligeiramente maior para o texto respirar
    WINDOW *pad = newpad(pad_lines, pad_cols);
    wbkgd(pad, COLOR_PAIR(13));
    keypad(pad, TRUE);
    idlok(pad, TRUE);
    scrollok(pad, TRUE);

    // 5. ESCRITA DOS TEXTOS NA PAD (COM ESPAÇAMENTO E MARGENS REFORÇADAS)
    wprintw(pad, "\n"); 

    wprintw(pad, "  ─────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattron(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, _("  1. REGENT OF THE %s: %s  \n"), (MAPA_DIURNO) ? _("DAY") : _("NIGHT"), regent_day_str);
    wattroff(pad, A_BOLD | COLOR_PAIR(15));
    wprintw(pad, "  ─────────────────────────────────────────────────────────────────────────────────────────────\n\n");
    
    switch(regente_dia) {
        case 1: // Sol
            wprintw(pad, _("Leadership and visibility."));
            break;
        case 2: // Lua
            wprintw(pad, _("A day marked by a certain restlessness and heightened sensitivity."));
            break;
        case 3: // Mercúrio
            wprintw(pad, _("A day for communications, studies, business, and partnerships."));
            break;
        case 4: // Vênus
            wprintw(pad, _("A day associated with pleasure and comfort."));
            break;
        case 5: // Marte
            wprintw(pad, _("Beginnings, action, competition, achievements, and discipline."));
            break;
        case 6: // Júpiter
            wprintw(pad, _("A favorable day for any undertaking."));
            break;
        case 7: // Saturno
            wprintw(pad, _("Discipline, patience, and deep reflection."));
            break;
    }
    wprintw(pad, "\n\n"); 

    wprintw(pad, "  ─────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattron(pad, A_BOLD | COLOR_PAIR(12));
    wprintw(pad, _("  2. REGENT OF THE HOUR: %s  \n"), regent_hour_str);
    wattroff(pad, A_BOLD | COLOR_PAIR(12));
    wprintw(pad, "  ─────────────────────────────────────────────────────────────────────────────────────────────\n\n");
    
    switch(regente_hora) {
        case 1: // Sol
            wprintw(pad, _("This is a favorable time for energetic activities or those involving leadership.\n"
                           "It is a suitable time for public actions and activities requiring visibility; therefore,\n"
                           "it is a good moment to speak with influential people.\n"
                           "It is a neutral time for business dealings, weddings, and construction projects.\n\n"));
            wprintw(pad, _("Time to shine. It favors activities requiring energy or matters related to strength, power,\n"
                           "and leadership. This is a moment when your energy will be in full swing. It is a good time\n"
                           "to deal with matters concerning money, prosperity, and business, as well as to look for a\n"
                           "job, make plans for the future, and buy new things.\n"));
            break;
        case 2: // Lua
            wprintw(pad, _("A favorable time for all domestic activities (especially buying food) and for anything\n"
                           "requiring imagination (ranging from useful inventions to activities that are not recommended,\n"
                           "such as fraud and acts of betrayal).\n"
                           "A beneficial time for tasks requiring rapid execution.\n"
                           "Unfavorable for tasks requiring stability.\n\n"));
            wprintw(pad, _("Ideal for routine tasks. It is a good time to review and re-evaluate your feelings and emotions.\n"
                           "Your sensitivity will be heightened, making you more emotionally volatile than usual. \n"
                           "It is a favorable time for making quick decisions, cleaning and organizing your home or business,\n"
                           "and traveling to visit relatives. However, it is not a good time to move house.\n"));
            break;
        case 3: // Mercúrio
            wprintw(pad, _("Mercury is suitable for communication, as well as for sending, signing, and renewing documents.\n"
                           "It is favorable for study, writing, teaching, and general learning activities, as well as for\n"
                           "business, commerce, and all forms of communication and partnerships.\n"
                           "It favors requests of all kinds (including prayers and marriage proposals).\n"
                           "It is a good time for medical treatments and travel, especially for business purposes.\n\n"));
            wprintw(pad, _("Time for communication. It is an excellent time for sending documents and signing contracts,\n"
                           "closing profitable deals, sending letters, and consulting with lawyers. \n"
                           "You will find success in signing paperwork during this period. \n"
                           "It is also a good time to obtain or renew official documents, engage in study-related \n"
                           "activities or teaching in general, and memorize texts.\n"));
            break;
        case 4: // Vênus
            wprintw(pad, _("A suitable time for harmony and beauty, and ideal for pleasure, social contacts, and \n"
                           "relationships.\n"
                           "A good moment to purchase ornaments and items related to beauty, as well as things associated\n"
                           "with pleasure and entertainment.\n"
                           "An excellent time for weddings and partnerships, as well as for speaking with superiors, \n"
                           "authorities, and women in general.\n"
                           "Given the playful and carefree nature of Venus, activities requiring great seriousness, \n"
                           "concentration, or effort are not recommended.\n\n"));
            wprintw(pad, _("A time for harmony and matters related to beauty. It is ideal for activities centered on \n"
                           "pleasure, as well as for intimate encounters, social interactions, and relationships. \n"
                           "It is an excellent time to boost your social life. \n"
                           "Do you want to buy something beautiful that will last a lifetime? This is the perfect moment to\n"
                           "purchase items for your home, refresh your wardrobe, or set a date for your engagement or a \n"
                           "happy wedding.\n"));
            break;
        case 5: // Marte
            wprintw(pad, _("A time for action, achievements, and new beginnings. Ideal for starting treatments and \n"
                           "medication. It favors any work involving fire.\n"
                           "A suitable time for assertive, competitive, and bold endeavors, though caution regarding \n"
                           "conflicts and disagreements is necessary.\n"
                           "Not a recommended time for negotiations, travel, construction activities, dealing with \n"
                           "superiors and authorities, or forming partnerships.\n\n"));
            wprintw(pad, _("A time for action, achievements, and picking up where you left off. \n"
                           "It is ideal for tasks requiring discipline, assertiveness, and a competitive spirit. \n"
                           "Caution is advised regarding arguments, accidents, and fires, given the powerful energy of \n"
                           "this hour.\n"));
            break;
        case 6: // Júpiter
            wprintw(pad, _("An auspicious time to launch any type of venture or project. An ideal moment for broadening \n"
                           "horizons and finding inspiration.\n"
                           "It is a balanced, tranquil period, favorable for changes and financial matters, as well as for\n"
                           "business, travel, medical treatments, and construction.\n"
                           "A good time to address matters of peace and harmony, friendship, and governance.\n\n"));
            wprintw(pad, _("It is time to expand toward new horizons and find happiness. It brings inspiration. \n"
                           "This is the ideal moment for shopping, making visits, and handling matters related to money, \n"
                           "abundance, and prosperity, as well as for expediting legal issues.\n"));
            break;
        case 7: // Saturno
            wprintw(pad, _("A suitable time for deep reflection, organizing ideas, and carrying out tasks that \n"
                           "require patience and discipline.\n"
                           "It may bring moments of mild depression due to the planet's melancholic nature; therefore, \n"
                           "one should be wary of thoughts centered on sadness.\n"
                           "A good time to devise strategies against adversaries.\n"
                           "Not a recommended time for medical treatments, taking medication, or speaking with authorities\n"
                           "and superiors. It is also ill-advised for construction activities or forming partnerships \n"
                           "(such as business ventures or marriages).\n\n"));
            wprintw(pad, _("It is time to resolve matters. This period calls for deep reflection, a restructuring of ideas,\n"
                           "and the execution of tasks requiring patience and discipline. It is a more tense time that \n"
                           "demands attention and care, as conditions tend to be unfavorable for almost everything. \n"
                           "It is a good time to attend to your health and pay close attention to any proposals made to you.\n"
                           "It is excellent for wrapping up affairs but terrible for starting new things.\n"));
            break;
    }
    
    wprintw(pad, "\n\n");


    // SYNTHESIS
    wprintw(pad, "  ─────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattron(pad, A_BOLD | COLOR_PAIR(32));
    wprintw(pad, _("  3. PLANETARY ALIGNMENT SYNTHESIS:  \n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(32));
    wprintw(pad, "  ─────────────────────────────────────────────────────────────────────────────────────────────\n\n");

    // To keep it readable, we check the Day first, then the Hour.
    wattron(pad, A_DIM);       
               
    switch(regente_dia) {
        case 1: // Sun
            switch(regente_hora) {
                case 1: wprintw(pad, _("A moment of peak vitality and visible leadership.")); break;
                case 2: wprintw(pad, _("Emotional sensitivity meets outward energy.")); break;
                case 3: wprintw(pad, _("Communicative leadership; a time to speak with authority.")); break;
                case 4: wprintw(pad, _("A beautiful moment for public displays of affection or art.")); break;
                case 5: wprintw(pad, _("Aggressive leadership; use your power with caution.")); break;
                case 6: wprintw(pad, _("Expanding your influence through visible action.")); break;
                case 7: wprintw(pad, _("Structured success; leading with discipline.")); break;
            }
            break;
        case 2: // Moon
            switch(regente_hora) {
                case 1: wprintw(pad, _("Action driven by intuition.")); break;
                case 2: wprintw(pad, _("A quiet, reflective moment of domestic peace.")); break;
                case 3: wprintw(pad, _("Quick changes in moods; perfect for writing or chatting.")); break;
                case 4: wprintw(pad, _("A day of aesthetic beauty and emotional comfort.")); break;
                case 5: wprintw(pad, _("A turbulent day; emotions may fuel physical conflicts.")); break;
                case 6: wprintw(pad, _("An expansive time for emotional growth and travel.")); break;
                case 7: wprintw(pad, _("A day of emotional boundaries and serious reflection.")); break;
            }
            break;
        case 3: // Mercury
            switch(regente_hora) {
                case 1: wprintw(pad, _("A sharp, communicative start to a powerful day.")); break;
                case 2: wprintw(pad, _("Fluid communication influenced by deep emotions.")); break;
                case 3: wprintw(pad, _("The peak of intellectual and commercial activity.")); break;
                case 4: wprintw(pad, _("Artistic expression through words and design.")); break;
                case 5: wprintw(pad, _("A sharp, potentially argumentative energy.")); break;
                case 6: wprintw(pad, _("A great time for business expansion and ideas.")); break;
                case 7: wprintw(pad, _("Logical, structured, and disciplined communication.")); break;
            }
            break;
        case 4: // Venus
            switch(regente_hora) {
                case 1: wprintw(pad, _("Radiant beauty meets solar visibility.")); break;
                case 2: wprintw(pad, _("A soft, romantic, and emotionally harmonious moment.")); break;
                case 3: wprintw(pad, _("Charming words and attractive social connections.")); break;
                case 4: wprintw(pad, _("Maximum pleasure, harmony, and aesthetic joy.")); break;
                case 5: wprintw(pad, _("Passionate energy meeting artistic grace.")); break;
                case 6: wprintw(pad, _("Luxurious and abundant social encounters.")); break;
                case 7: wprintw(pad, _("Classic, disciplined elegance and refined taste.")); break;
            }
            break;
        case 5: // Mars
            switch(regente_hora) {
                case 1: wprintw(pad, _("Heroic action and bold, visible leadership.")); break;
                case 2: wprintw(pad, _("Sudden, impulsive movements driven by passion.")); break;
                case 3: wprintw(pad, _("Decisive, sharp, and energetic communication.")); break;
                case 4: wprintw(pad, _("Intense passion and physical attraction.")); break;
                case 5: wprintw(pad, _("Unbridled energy and pure, competitive drive.")); break;
                case 6: wprintw(pad, _("Bold, expansive, and courageous new undertakings.")); break;
                case 7: wprintw(pad, _("Hard-fought victories through endurance and strength.")); break;
            }
    
            break;
        case 6: // Jupiter
            switch(regente_hora) {
                case 1: wprintw(pad, _("Great prosperity and public recognition.")); break;
                case 2: wprintw(pad, _("Expansive moods and fortunate travels.")); break;
                case 3: wprintw(pad, _("Profitable deals and wise negotiations.")); break;
                case 4: wprintw(pad, _("Joyful abundance in your social life.")); break;
                case 5: wprintw(pad, _("Large-scale, energetic, and prosperous projects.")); break;
                case 6: wprintw(pad, _("Maximum expansion and unparalleled luck.")); break;
                case 7: wprintw(pad, _("Wise, structured growth and long-term gains.")); break;
            }
            break;
        case 7: // Saturn
            switch(regente_hora) {
                case 1: wprintw(pad, _("Serious leadership and heavy responsibility.")); break;
                case 2: wprintw(pad, _("Somber reflection and emotional restraint.")); break;
                case 3: wprintw(pad, _("Careful, methodical, and slow communication.")); break;
                case 4: wprintw(pad, _("Tasteful restraint and classic, structured beauty.")); break;
                case 5: wprintw(pad, _("Intense discipline and defensive strength.")); break;
                case 6: wprintw(pad, _("Expanding through careful, structured planning.")); break;
                case 7: wprintw(pad, _("The peak of discipline, duty, and endurance.")); break;
            }
            break;
    }
    
    wprintw(pad, "\n\n");


    // SYNTHESIS
    wprintw(pad, "  ─────────────────────────────────────────────────────────────────────────────────────────────\n");
    wattron(pad, A_BOLD | COLOR_PAIR(27));
    wprintw(pad, _("  4. REGENTS' STRENGTH:  \n"));
    wattroff(pad, A_BOLD | COLOR_PAIR(27));
    wprintw(pad, "  ─────────────────────────────────────────────────────────────────────────────────────────────\n\n");
    
    wattroff(pad, A_DIM);


    /* Calcula os pontos ponderados reais apenas para a string informativa do texto */
    double weights[100];
    get_weights(weights, show_modern_planets);
    int pontos_finais_exibicao_day = (int)ceil(((double)strength_reg_day * weights[regente_dia]) / 10.0);

    wattron(pad, A_BOLD); 
    wprintw(pad, _("• REGENT OF %s: ( %s )\n\n"), (MAPA_DIURNO)?_("THE DAY"):_("THE NIGHT"), regent_day_str);
    wattroff(pad, A_BOLD); 

    wprintw(pad, _("    The base dignity score of the Regent of %s is: %d.\n"
                    "    Its relative cosmic efficiency is: %d%% (Resulting in %d Net Strength Points).\n\n"), 
            (MAPA_DIURNO)?_("the Day"):_("the Night"), dig_reg_day, strength_reg_day, pontos_finais_exibicao_day);
    
    wprintw(pad, _("    Structural Efficiency Verdict:\n"));

    // Julgamento por porcentagem pura e justa: Mercúrio com 83% fica verde!
    if (strength_reg_day >= 65) {
        wattron(pad, A_BOLD | A_REVERSE | COLOR_PAIR(12)); // Excelente / Verde
        wprintw(pad, _("    • HIGH OPERATIONAL CAPACITY (EXCELLENT CHAPTER):\n\n"));
        wattroff(pad, A_BOLD | A_REVERSE | COLOR_PAIR(12));
        wprintw(pad, _("       This planet commands %s with magnificent backing.\n"
                        "       Because its cosmic efficiency is highly abundant (%d%%), it acts as an honored\n"
                        "and powerful executive. The promises of this planet will manifest with clarity,\n"
                        "bringing structural progress, sudden expansion, and minimal friction.\n\n\n"), (MAPA_DIURNO)?_("the day"):_("the night"), strength_reg_day);
    } 
    else if (strength_reg_day >= 35) {
        wattron(pad, A_BOLD | COLOR_PAIR(8)); // Moderado / Azul
        wprintw(pad, _("    • MODERATE OPERATIONAL CAPACITY (BALANCED CHAPTER):\n\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(8));
        wprintw(pad, _("       This planet holds average, stable ground in your baseline blueprint (%d%%).\n"
                        "       It possesses the standard authority to execute its functions, but will demand steady\n"
                        "discipline and continuous focus from you. Events will unfold normally, tracking your\n"
                        "real-world daily effort without extraordinary windfalls or sudden structural collapses.\n\n\n"), strength_reg_day);
    } 
    else {
        wattron(pad, A_BOLD | COLOR_PAIR(11)); // Crítico / Vermelho
        wprintw(pad, _("    • CRITICAL CAPACITY DRAIN (MUTED OR IMPEDED CHAPTER):\n\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(11));
        wprintw(pad, _("       WARNING: The Lord of %s operates under extreme systemic debility (%d%%).\n"
                        "       Even though it governs this time stream, it lacks the raw vital\n"
                        "resources to fulfill its promises easily. The sectors it triggers this day will demand\n"
                        "intense adjustments, manifesting through chronic delays, heavy exhaustion,\n"
                        "administrative blocks, or the feeling of working against a locked door.\n\n\n"), (MAPA_DIURNO)?_("the Day"):_("the Night"), strength_reg_day);
    }

    wprintw(pad, "\n\n");

    int pontos_finais_exibicao_hour = (int)ceil(((double)strength_reg_hour * weights[regente_hora]) / 10.0);

    wattron(pad, A_BOLD); 
    wprintw(pad, _("• REGENT OF THE HOUR: ( %s )\n\n"), regent_hour_str);
    wattroff(pad, A_BOLD); 

    wprintw(pad, _("    The base dignity score of the Regent of the hour is: %d.\n"
                    "    Its relative cosmic efficiency is: %d%% (Resulting in %d Net Strength Points).\n\n"), 
            dig_reg_hour, strength_reg_hour, pontos_finais_exibicao_hour);
    
    wprintw(pad, _("    Structural Efficiency Verdict:\n"));

    // Julgamento por porcentagem pura e justa: Mercúrio com 83% fica verde!
    if (strength_reg_hour >= 65) {
        wattron(pad, A_BOLD | A_REVERSE | COLOR_PAIR(12)); // Excelente / Verde
        wprintw(pad, _("    • HIGH OPERATIONAL CAPACITY (EXCELLENT CHAPTER):\n\n"));
        wattroff(pad, A_BOLD | A_REVERSE | COLOR_PAIR(12));
        wprintw(pad, _("       This planet commands the hour with magnificent backing.\n"
                        "       Because its cosmic efficiency is highly abundant (%d%%), it acts as an honored\n"
                        "and powerful executive. The promises of this planet will manifest with clarity,\n"
                        "bringing structural progress, sudden expansion, and minimal friction.\n\n\n"), strength_reg_hour);
    } 
    else if (strength_reg_hour >= 35) {
        wattron(pad, A_BOLD | COLOR_PAIR(8)); // Moderado / Azul
        wprintw(pad, _("    • MODERATE OPERATIONAL CAPACITY (BALANCED CHAPTER):\n\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(8));
        wprintw(pad, _("       This planet holds average, stable ground in your baseline blueprint (%d%%).\n"
                        "       It possesses the standard authority to execute its functions, but will demand steady\n"
                        "discipline and continuous focus from you. Events will unfold normally, tracking your\n"
                        "real-world daily effort without extraordinary windfalls or sudden structural collapses.\n\n\n"), strength_reg_hour);
    } 
    else {
        wattron(pad, A_BOLD | COLOR_PAIR(11)); // Crítico / Vermelho
        wprintw(pad, _("    • CRITICAL CAPACITY DRAIN (MUTED OR IMPEDED CHAPTER):\n\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(11));
        wprintw(pad, _("       WARNING: The Lord of the hour operates under extreme systemic debility (%d%%).\n"
                        "       Even though it governs this time stream, it lacks the raw vital\n"
                        "resources to fulfill its promises easily. The sectors it triggers this day will demand\n"
                        "intense adjustments, manifesting through chronic delays, heavy exhaustion,\n"
                        "administrative blocks, or the feeling of working against a locked door.\n\n\n"), strength_reg_hour);
    }



    wprintw(pad, "\n\n");


    wattron(pad, A_DIM);
    wprintw(pad, "  ─────────────────────────────────────────────────────────────────────────────────────────────\n");
    wprintw(pad, _("  [NARRATIVE END] - Press 'Q' or ESC to return to the table.\n"));
    wattroff(pad, A_DIM);

    // 6. LOOP DE INTERAÇÃO E REDESENHO CONSTANTE DA PAD
    int pad_line_pos = 0;
    int max_scroll_y = 65; 
    int ch;

    prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);

    while ((ch = wgetch(pad)) != 27 && ch != 'q' && ch != 'Q') {
        switch (ch) {
            case KEY_UP:
            case 'k':
            case 'K':
                if (pad_line_pos > 0) pad_line_pos--;
                break;
            case KEY_DOWN:
            case 'j':
            case 'J':
                if (pad_line_pos < max_scroll_y) pad_line_pos++;
                break;
        }
        prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);
    }

    // 7. DESTRUIÇÃO E LIMPEZA
    delwin(pad);
    delwin(border_win);
    delwin(shadow_win);
}




void display_rising_times(PlotObject *plots, double tz_offset) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int table_height = 30;
    int table_width = max_x - 5;
    int start_y = (max_y - table_height) / 2;
    int start_x = 2;

    int object_diff = 0;
    if (show_modern_planets) {
        object_diff = 0;
    }
    else {
        object_diff = 3;
    }
    
    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);
    
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(6));

    wattron(table_win, A_BOLD);
    const char *title = _("Rising Times | Setting Times | Upper Culmination Times");
    mvwprintw(table_win, 0, (table_width - 56) / 2, title);

    mvwprintw(table_win, 2, 4, _("       Rising                    Setting                   Culmination                   Time zone: Local"));
    mvwprintw(table_win, 3, 3, "────────────────────────────────────────────────────────────────────────────────────"); 
    wattroff(table_win, A_BOLD);

    
    int row = 4;
    for (int i = 0; i < 12 - object_diff; i++) {
        
        double rise_hours = plots[i].rising_time + tz_offset;
        const char* rise_obs = "";
        if (rise_hours < 0.0) {
            rise_obs = " (-1d)"; // Indicador de dia anterior
            rise_hours += 24.0;
        } 
        else if (rise_hours >= 24.0) {
            rise_obs = " (+1d)"; // Indicador de dia seguinte
            rise_hours -= 24.0;
        }        

        Hora h_rise = get_fmt_hour(rise_hours);
        int rise_h = h_rise.hora;
        int rise_m = h_rise.min;
        int rise_s = h_rise.sec;

        
        double set_hours = plots[i].setting_time + tz_offset;
        const char* set_obs = "";
        if (set_hours < 0.0) {
            set_obs = " (-1d)"; // Indicador de dia anterior
            set_hours += 24.0;
        } 
        else if (set_hours >= 24.0) {
            set_obs = " (+1d)"; // Indicador de dia seguinte
            set_hours -= 24.0;
        }

        Hora h_set = get_fmt_hour(set_hours);
        int set_h = h_set.hora;
        int set_m = h_set.min;
        int set_s = h_set.sec;


        double mid_hours = plots[i].mid_time + tz_offset;
        const char* mid_obs = "";
        if (mid_hours < 0.0) {
            mid_obs = " (-1d)"; // Indicador de dia anterior
            mid_hours += 24.0;
        } 
        else if (mid_hours >= 24.0) {
            mid_obs = " (+1d)"; // Indicador de dia seguinte
            mid_hours -= 24.0;
        }

        Hora h_mid = get_fmt_hour(mid_hours);
        int mid_h = h_mid.hora;
        int mid_m = h_mid.min;
        int mid_s = h_mid.sec;


        mvwprintw(table_win, row, 4, "%s      %02d:%02d:%02d %s", plots[i].object, rise_h, rise_m, rise_s, rise_obs);
        mvwprintw(table_win, row, 37, "%02d:%02d:%02d %s", set_h, set_m, set_s, set_obs);
        mvwprintw(table_win, row, 63, "%02d:%02d:%02d %s", mid_h, mid_m, mid_s, mid_obs);

        wattron(table_win, COLOR_PAIR(10) | A_DIM);
        mvwprintw(table_win, row + 1, 3, "────────────────────────────────────────────────────────────────────────────────────"); 
        wattroff(table_win, COLOR_PAIR(10) | A_DIM);

        row += 2;
    }

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
    touchwin(stdscr); 
    refresh();
 
}


int extract_local_datetime_from_jd(double jd, double offset, int *y, int *m, int *d, double *h) {
    // Retorna um valor negativo sentinela caso o Dia Juliano seja inválido
    if (jd < 0) {
        return -1; 
    }
    
    // 1. Aplica o fuso horário diretamente no Dia Juliano antes da conversão
    double local_jd = jd + (offset / 24.0);
    
    // 2. A biblioteca extrai a hora decimal local perfeita (já lida com as viradas de dia)
    swe_revjul(local_jd, SE_GREG_CAL, y, m, d, h);
    
    // 3. Garante que o valor retornado esteja estritamente no intervalo de [0.0, 24.0)
    // Isso previne anomalias matemáticas flutuantes nas bordas da meia-noite
    if (*h >= 24.0) {
        *h -= 24.0;
    }
    if (*h < 0.0) {
        *h += 24.0;
    }
    
    return 0;
}





double get_hours_from_jd(double jd, double offset) {
    // Retorna um valor negativo sentinela caso o Dia Juliano seja inválido
    if (jd < 0) {
        return -1.0; 
    }
    
    int y, m, d;
    double h;
    
    // 1. Aplica o fuso horário diretamente no Dia Juliano antes da conversão
    double local_jd = jd + (offset / 24.0);
    
    // 2. A biblioteca extrai a hora decimal local perfeita (já lida com as viradas de dia)
    swe_revjul(local_jd, SE_GREG_CAL, &y, &m, &d, &h);
    
    // 3. Garante que o valor retornado esteja estritamente no intervalo de [0.0, 24.0)
    // Isso previne anomalias matemáticas flutuantes nas bordas da meia-noite
    if (h >= 24.0) {
        h -= 24.0;
    }
    if (h < 0.0) {
        h += 24.0;
    }
    
    return h;
}


void format_event_time(double jd, double offset, char *dest, size_t size_of_dest) {
    if (jd < 0) {
        snprintf(dest, size_of_dest, "--:--:--");
        return;
    }
    
    int y, m, d;
    double h;
    
    // 1. Aplica o fuso horário diretamente no Dia Juliano antes da conversão
    double local_jd = jd + (offset / 24.0);
    
    // 2. Converte para a data e hora local
    swe_revjul(local_jd, SE_GREG_CAL, &y, &m, &d, &h);
    
    // 3. Arredondamento matemático dos segundos (evita que 05:44:59.99 apareça como 05:44:59)
    // Adicionamos meio milissegundo (0.5 / 3600000.0) para estabilizar a exibição
    h += 0.5 / 3600000.0;
    if (h >= 24.0) {
        h -= 24.0;
    }

    // 4. Extração matemática de horas, minutos e segundos
    int hours = (int)h;
    int minutes = (int)((h - hours) * 60.0);
    double sec_decimal = (((h - hours) * 60.0) - minutes) * 60.0;
    int seconds = (int)sec_decimal;
    
    // 5. Formata a string de saída com segurança (Ex: "06:45:23")
    snprintf(dest, size_of_dest, "%02d:%02d:%02d", hours, minutes, seconds);
}


double calc_solar_time(double julian_day, double ut_hours, double lon) {
    
    // Descobrir o Delta T da data para converter UT em ET
    double delta_t = swe_deltat(julian_day);
    double julian_day_et = julian_day + delta_t; // Data em Ephemeris Time

    char serr[256];
    double teq;
    if (swe_time_equ(julian_day_et, &teq, serr) != 0) {
        fprintf(stderr, "Erro na EoT: %s\n", serr);
        return 1;
    }

    // Na Swiss Ephemeris teq = (Aparente - Médio) em frações de dia. Convertemos para horas.
    double eot_hours = teq * 24.0; 

    // Calcular o Local Apparent Solar Time (LAST) baseado no UTC real
    double lon_hours = lon / 15.0; // Assume convenção astronômica: Leste (+), Oeste (-)
    double lmt_hours = ut_hours + lon_hours;    // Tempo Médio Local exato
    
    // Aplica a EoT diretamente: Aparente = Médio + EoT
    double last_hours = lmt_hours + eot_hours;  

    // Normalizar LAST para o intervalo geométrico [0, 24)
    last_hours = fmod(last_hours, 24.0);
    if (last_hours < 0) last_hours += 24.0;

    return last_hours;
}



struct tm julian_day_para_struct_tm(double jd_retorno) {
    int ano, mes, dia, horas, minutos;
    double segundos_decimal;
    struct tm t_retorno;

    // SE_KEEP_GREG_CAL respeita a transição histórica de 1582
    swe_jdut1_to_utc(jd_retorno, SE_KEEP_GREG_CAL, &ano, &mes, &dia, &horas, &minutos, &segundos_decimal);

    // Arredonda os segundos com segurança
    int segundos = (int)(segundos_decimal + 0.5);

    // Tratamento de estouro de segundos (se der 60, vira 0 e soma 1 minuto)
    if (segundos >= 60) {
        segundos = 0;
        minutos++;
        if (minutos >= 60) {
            minutos = 0;
            horas++;
            // Nota: Se as horas estourarem (>=24), a função timegm() abaixo 
            // corrige os dias, meses e anos automaticamente para nós.
        }
    }

    /* Preenche a struct tm com as regras padrão do C */
    t_retorno.tm_year  = ano - 1900;  /* Anos desde 1900 */
    t_retorno.tm_mon   = mes - 1;     /* Meses de 0 a 11 */
    t_retorno.tm_mday  = dia;
    t_retorno.tm_hour  = horas;
    t_retorno.tm_min   = minutos;
    t_retorno.tm_sec   = segundos;
    t_retorno.tm_isdst = -1;          

    // Normaliza a struct (corrige possíveis estouros de horas/minutos causados pelo arredondamento)
    timegm(&t_retorno); 

    return t_retorno;
}


double calc_declination_mathematical_point(double jd, double longitude) {
    double xpin[3], xpout[3];
    double x_nut[6];
    char serr[256];
    
    // Calcular a Obliqüidade da Eclíptica para este momento (SE_ECL_NUT)
    if (swe_calc_ut(jd, SE_ECL_NUT, 0, x_nut, serr) < 0) {
        printf("Erro ao calcular obliqüidade: %s\n", serr);
        return NAN; // Retorna "Not a Number" em vez de 1.0 em caso de erro
    }
    
    // x_nut[0] contém a obliqüidade verdadeira da eclíptica (true obliquity)
    double eps = x_nut[0];
    
    // Preparar o vetor de entrada (Coordenadas Eclípticas)
    xpin[0] = longitude; 
    xpin[1] = 0.0;        // Latitude zero (ponto matemático na eclíptica)
    xpin[2] = 1.0;        // Distância arbitrária
    
    // Executar a transformação para Coordenadas Equatoriais
    // O sinal negativo (-eps) converte de ECLÍPTICA para EQUATORIAL
    swe_cotrans(xpin, xpout, -eps);
    
    return xpout[1]; // Retorna a declinação diretamente
}


// Exemplo de como calcular as coordenadas equatoriais de um limite de Termo
int obter_coordenadas_termo(double longitude_termo, double jd_natal, double *ra_out, double *dec_out) {
    double xpin[3], xpout[3];
    double x_nut[6];
    char serr[256];

    // 1. Obtém a obliquidade da eclíptica do nascimento
    if (swe_calc_ut(jd_natal, SE_ECL_NUT, 0, x_nut, serr) < 0) {
        return 0; // Erro
    }
    double eps = x_nut[0];

    // 2. Prepara o vetor com LATITUDE ZERO
    xpin[0] = longitude_termo; 
    xpin[1] = 0.0;             // CRÍTICO: Termos são pontos matemáticos da eclíptica
    xpin[2] = 1.0;

    // 3. Converte para Equatorial via Swiss Ephemeris
    swe_cotrans(xpin, xpout, -eps);

    *ra_out = xpout[0];  // Ascensão Reta do Termo
    *dec_out = xpout[1]; // Declinação do Termo (Pronta para o cálculo mundano!)
    return 1;
}


double get_longitude_term(int sign, int index, Termo tabela[12][5]) {
    if (index == 0) {
        return sign * 30.0;
    }

    return sign * 30.0 + tabela[sign][index - 1].grau_limite;
}



int chart(struct tm *local_time, double lat, double lon, double elev, double tz_offset, char *city, char *country, bool animated, int anim_interval, char *chart_name, char house_system, int gender_id, int darkmode, int mapa_retorno, int senhor_da_profeccao, int id_senhor_firdaria, int id_senhor_subfirdaria, double armc_natal, double lat_natal, PlanetDignities *dig_natal, char *nome_anareta_natal, char *nome_s8_natal, int tipo_h_natal, int idx_hyleg_natal, double *longitudes_natal, double jd_natal, int *strength_natal, double asc_natal, double *cusps_natal) {
    int n = 1;
    
    bool dark_mode = (darkmode)?true:false;
    
    char err[256];
    char serr[256];
    double x2[6];
    double cusps[13];
    double ascmc[10];
    double julian_day;
    double xx_equatorial[6];
    int flags_equatorial = SEFLG_SPEED | SEFLG_EQUATORIAL;

    
    int planets[11] = {
        SE_SUN, SE_MOON, SE_MERCURY, SE_VENUS, SE_MARS, 
        SE_JUPITER, SE_SATURN, SE_URANUS, SE_NEPTUNE, SE_PLUTO, SE_TRUE_NODE
    };
    double planet_longitudes[11];
    double planet_declinations[11];
    double planet_latitudes[11];
    double speed[11];

    double ascendant;
    double mc;

    planet_regent_names[0] = _("Sun");
    planet_regent_names[1] = _("Venus"); 
    planet_regent_names[2] = _("Mercury"); 
    planet_regent_names[3] = _("Moon"); 
    planet_regent_names[4] = _("Saturn"); 
    planet_regent_names[5] = _("Jupiter"); 
    planet_regent_names[6] = _("Mars");

    
    sqlite3 *db2 = open_database();
    if (db2) {
        sqlite3_stmt *stmt;
        const char *sql_select1 = "SELECT show_modern_planets FROM profiles WHERE profile = 'default';";
        if (sqlite3_prepare_v2(db2, sql_select1, -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int show_mod = sqlite3_column_int(stmt, 0);

                if (show_mod > 0) {
                    show_modern_planets = TRUE;
                }
                else {
                    show_modern_planets = FALSE;
                }
            }
            sqlite3_finalize(stmt);
        }
        close_database(db2);
    }
    int object_diff = 0;
    if (show_modern_planets) {
        object_diff = 0;
    }
    else {
        object_diff = 3;
    }


    
    // Carrega dados do banco (get_planet_orbis)
    int planet_ids[NUM_OBJECTS];
    double planet_orbis[NUM_OBJECTS];

    // define orbe mínimo de 1.0 para todos, antes de obter o orbe do planetas
    for (int i = 0; i < NUM_OBJECTS; i++, planet_orbis[i] = 1.0);
    char planet_symbols[NUM_OBJECTS][10];
    get_planet_orbis(planet_ids, planet_orbis, planet_symbols, 12);


    // Carrega dados do banco - parallel orbis
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



    // Carrega dados do banco - consider_modern_planets_rulling
    consider_modern_planets_rulling = 1;
    sqlite3 *db1 = open_database();
    if (db1) {
        sqlite3_stmt *stmt;
        const char *sql_select = "SELECT modern_planets_rulling FROM profiles WHERE profile = 'default';";
        if (sqlite3_prepare_v2(db1, sql_select, -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                consider_modern_planets_rulling = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        close_database(db1);
    }

    


    // Carrega dados do banco - get_default_terms_system
    terms_system = get_default_terms_system();




    swe_set_ephe_path(getenv("SE_EPHE_PATH"));

    
    if (!inicializar_swiss_ephemeris()) {
        fprintf(stderr, "Erro Fatal: Ephemerides não inicializada!\n");
        fprintf(stderr, "Abandono da execucao.\n");
        terminate_database();
        exit(1); 
    }



    // Initialize ncurses
    initscr();    
    start_color();



    // Zoom factor - start with 1.0 (normal size)
    float zoom_factor = 1.0;
                
    // Panning offsets - start at center
    float pan_x = 1.0;
    float pan_y = 0.0;

    
       
    // Main loop for handling input and redrawing
    int ch;
    bool house_div = false;
    bool show_dec = false;
    bool show_terms = false;
    bool running = true;

    bool backup_mapa_diurno = false;
    
    // Set a timeout for getch() - this adds the pause functionality
    // 1000ms = 1 second timeout
    timeout(1000); // This will make getch() wait for 1 second max
    
    while (running) {
        
        // =========================================================================
        // PREPARAÇÃO: CONVERSÃO SEGURA PARA UTC
        // =========================================================================
        
        // 1. Fazemos uma cópia de backup do horário local original do usuário
        struct tm backup_local = *local_time;

        // 2. timegm interpreta os dados locais como um timestamp Unix linear,
        // ignorando completamente a variável de ambiente 'TZ' do seu servidor Linux.
        time_t timestamp_puro = timegm(local_time); 

        // 3. Subtraímos o tz_offset do local para obter o UTC real.
        // Se tz_offset for -3.0 (Brasil), vira adição (- -3 = +3), gerando o UTC correto.
        time_t tempo_utc_timestamp = timestamp_puro - (time_t)(tz_offset * 3600.0);

        // 4. Converte o timestamp UTC de volta para a estrutura isolada astronômica
        struct tm *utc_time = gmtime(&tempo_utc_timestamp);

        // 5. Fração do dia em formato decimal UTC para a Swiss Ephemeris
        double hora_utc_decimal = utc_time->tm_hour + 
                                  (utc_time->tm_min / 60.0) + 
                                  (utc_time->tm_sec / 3600.0);

        // 6. Calcular o Julian Day correto em UT/UTC
        julian_day = swe_julday(utc_time->tm_year + 1900, 
                                utc_time->tm_mon + 1, 
                                utc_time->tm_mday, 
                                hora_utc_decimal, 
                                SE_GREG_CAL);



        
        // 1. Loop to calculate the 10 planetary positions

        for (int i = 0; i < 11; i++) {
            if (swe_calc_ut(julian_day, planets[i], SEFLG_SPEED, x2, err) < 0) {
                printf("Error calculating planet %d: %s\n", planets[i], err);
                return 1;
            }
            planet_longitudes[i] = x2[0];
            planet_latitudes[i] = x2[1];
            speed[i] = x2[3];
        }

        for (int i = 0; i < 11; i++) {
            if (swe_calc_ut(julian_day, planets[i], flags_equatorial, xx_equatorial, serr) < 0) {
                printf("Error calculating planet %d: %s\n", planets[i], err);
                return 1;
            }
            planet_declinations[i] = xx_equatorial[1];
        }

        // 2. Calculate Ascendant, MC and houses
        
        int result = swe_houses(julian_day, lat, lon, house_system, cusps, ascmc);
        
        if (result == ERR || ((house_system == 'P' || house_system == 'B' || house_system == 'K') && (lat > 66.5 || lat < -66.5))) {            
            house_system = 'O'; // fallback to Porphyry System
            
            result = swe_houses(julian_day, lat, lon, house_system, cusps, ascmc);

            if (result == ERR) {
                printf("Error calculating houses!");
                return 0;
            }
        }
        ascendant = ascmc[0]; // SE_ASC is always stored at index 0 of ascmc
        mc = ascmc[1];
        double vertex = ascmc[3];
        double armc = ascmc[2];
        //double east_point = ascmc[4];

        // Arrays para receber os resultados da conversão [longitude, latitude, distância]
        // Como o AC, o MC e Vertex estão exatamente na linha da eclíptica, a latitude eclíptica é SEMPRE 0.0
        double eclip_asc[3] = { ascendant, 0.0, 1.0 };
        double eclip_mc[3] = { mc, 0.0, 1.0 };
        double eclip_vertex[3] = { vertex, 0.0, 1.0 };
        
        double equat_asc[3];
        double equat_mc[3];
        double equat_vertex[3];

      
        // 3. Calcula a obliquidade da eclíptica para o julian_day atual
        double eps[6];

        if (swe_calc_ut(julian_day, SE_ECL_NUT, 0, eps, serr) < 0) {
            printf("Error calculating ecliptic obliquity: %s\n", serr);
            return 1;
        }
        double true_obliquity = eps[0]; // eps[0] contém a obliquidade verdadeira

        // 3. Converter de coordenadas Eclípticas para Equatoriais usando a Obliquidade (eps[0])
        // O resultado será gravado em equat_... onde:
        // [0] = Ascensão Reta, [1] = Declinação, [2] = Distância
        swe_cotrans(eclip_asc, equat_asc, -true_obliquity); // O sinal negativo converte de eclíptica para equatorial
        swe_cotrans(eclip_mc, equat_mc, -true_obliquity);
        swe_cotrans(eclip_vertex, equat_vertex, -true_obliquity);

        double ascendant_declination = equat_asc[1];
        double mc_declination = equat_mc[1];
        double vertex_declination = equat_vertex[1];


        

        // =========================================================================
        // 6. Calculate Solar Time
        // =========================================================================
        double last_hours = calc_solar_time(julian_day, utc_time->tm_hour + utc_time->tm_min / 60.0 + utc_time->tm_sec / 3600.0, lon);
        
        // Formatar saídas temporais do LAST
        int last_hr = (int)last_hours;
        int last_min = (int)((last_hours - last_hr) * 60.0);
        double last_sec = ((last_hours - last_hr) * 3600.0) - (last_min * 60.0);

        
        
        // 7. Calculate the sunrise and sunset

        double jd = swe_julday(utc_time->tm_year + 1900, utc_time->tm_mon + 1, utc_time->tm_mday, 0.0, SE_GREG_CAL);
        
        double sunrise_jd = find_sun_event(jd, lat, lon, elev, true); // true para sunrise
        double sunset_jd = find_sun_event(jd, lat, lon, elev, false); // false para sunset
        double next_sunrise_jd = find_sun_event(jd + 1.0, lat, lon, elev, true);
        double prev_sunset_jd = find_sun_event(jd - 1.0, lat, lon, elev, false);
        double prev_sunrise_jd = find_sun_event(jd - 1.0, lat, lon, elev, true);
        
        // strings para exibição
        char sunrise_time[16] = "--:--:--";
        char sunset_time[16] = "--:--:--";
        char next_sunrise_time[16] = "--:--:--";
        char prev_sunset_time[16] = "--:--:--";
        
        format_event_time(sunrise_jd, tz_offset, sunrise_time, sizeof(sunrise_time));
        format_event_time(sunset_jd, tz_offset, sunset_time, sizeof(sunset_time));
        format_event_time(next_sunrise_jd, tz_offset, next_sunrise_time, sizeof(next_sunrise_time));
        format_event_time(prev_sunset_jd, tz_offset, prev_sunset_time, sizeof(prev_sunset_time));
               
        // horas decimais
        double riseHour = get_hours_from_jd(sunrise_jd, tz_offset);
        double setHour = get_hours_from_jd(sunset_jd, tz_offset);

        double next_riseHour = get_hours_from_jd(next_sunrise_jd, tz_offset);
        double prev_setHour = get_hours_from_jd(prev_sunset_jd, tz_offset);
        double prev_riseHour = get_hours_from_jd(prev_sunrise_jd, tz_offset);



        // =========================================================================
        // TRATAMENTO PARA REGIÕES POLARES (ABORDAGEM ELEGANTE DE 2 HORAS POR HORA)
        // =========================================================================
        int y, m, d;
        double h;
        double last_rise, last_set;
        int REGIAO_POLAR = 0;
        int SOL_PERMANENTE = 0;

        if (sunrise_jd < 0 || sunset_jd < 0 || riseHour < 0 || setHour < 0) {
            REGIAO_POLAR = 1;
            
            // Verificação de Verão (Sol da Meia-Noite) vs Inverno (Noite Polar)
            if ((lat > 0 && utc_time->tm_mon >= 3 && utc_time->tm_mon <= 8) ||
                (lat < 0 && (utc_time->tm_mon <= 2 || utc_time->tm_mon >= 9))) {
                SOL_PERMANENTE = 1; 
            }

            // Se o Sol é permanente, o "Dia" dura 24h (de 0h às 24h)
            // Se for Noite Polar, o "Dia" tem duração 0 e a "Noite" dura 24h (de 0h às 24h)
            if (SOL_PERMANENTE) {
                riseHour = 0.0;
                setHour = 24.0;
                next_riseHour = 24.0;
            } else {
                riseHour = 0.0;
                setHour = 0.0;
                next_riseHour = 24.0;
            }

            // LASTs virtuais para consistência de interface
            last_rise = 0.0;
            last_set = SOL_PERMANENTE ? 24.0 : 0.0;
        } 
        else {
            // CÁLCULO PADRÃO PARA O RESTO DO MUNDO
            swe_revjul(sunrise_jd, SE_GREG_CAL, &y, &m, &d, &h);        
            last_rise = calc_solar_time(sunrise_jd, h, lon);
            
            swe_revjul(sunset_jd, SE_GREG_CAL, &y, &m, &d, &h);
            last_set = calc_solar_time(sunset_jd, h, lon);
        }

            
        backup_mapa_diurno = MAPA_DIURNO;

        // Definir MAPA_DIURNO com base no LAST ou na Solução Polar
        if (REGIAO_POLAR) {
            MAPA_DIURNO = (SOL_PERMANENTE) ? TRUE : FALSE;
        } else {
            MAPA_DIURNO = (last_hours >= last_rise && last_hours < last_set) ? TRUE : FALSE;
        }
        


        
        // 8. Planetary hour
        double current_rise = riseHour;
        double current_set = setHour;
        double next_rise = next_riseHour;

        *local_time = backup_local;

        double hour = (local_time->tm_hour * 60.0) + (local_time->tm_min) + (local_time->tm_sec / 60.0);
        hour = hour / 60.0;

        // IGNORA ajustes de madrugada se for polar, para não quebrar a baliza 0-24h fixa
        if (!REGIAO_POLAR) {
            if (!MAPA_DIURNO && hour < riseHour) {
                current_rise = prev_riseHour;
                current_set = prev_setHour;
                next_rise = riseHour;
            }

            if (current_set < current_rise) {
                current_set += 24.0;
            }

            while (next_rise < current_set) {
                next_rise += 24.0;
            }
        }

        // Cálculo dos intervalos (No polo, um dará 24.0 e o outro 0.0)
        double day_interval = current_set - current_rise;
        // Evita divisão por zero injetando 2.0 temporariamente para o laço não quebrar
        double daytime_hour = (day_interval > 0.0) ? (day_interval / 12.0) : 2.0; 

        double night_interval = next_rise - current_set;
        // Evita divisão por zero injetando 2.0 temporariamente para o laço não quebrar
        double nighttime_hour = (night_interval > 0.0) ? (night_interval / 12.0) : 2.0; 
            
        double hours[25];

        // =========================================================================
        // PREENCHIMENTO DO ARRAY CONTÍNUO COM TRATAMENTO DE HORAS INEXISTENTES (< 0)
        // =========================================================================
        if (REGIAO_POLAR) {
            if (SOL_PERMANENTE) {
                // Sol da Meia-Noite: As 12 horas diurnas ocupam as 24h do dia (2h cada)
                for (int i = 0; i < 12; i++) {
                    hours[i] = current_rise + (double)i * daytime_hour;
                }
                
                // As 12 horas noturnas NÃO EXISTEM.
                // Injetamos -1.0 para que sua função de UI imprima "--:--:--"
                for (int i = 0; i < 12; i++) {
                    hours[i + 12] = -1.0;
                }

                hours[24] = 24.0;

            } else {
                // Noite Polar: As 12 horas diurnas NÃO EXISTEM.
                // Injetamos -1.0 para que sua função de UI imprima "--:--:--"
                for (int i = 0; i < 12; i++) {
                    hours[i] = -1.0;
                }
                
                // As 12 horas noturnas ocupam as 24h do dia (2h cada)
                for (int i = 0; i < 12; i++) {
                    hours[i + 12] = current_set + (double)i * nighttime_hour;
                }

                hours[24] = -1.0;
            }
            
        } 
        else {
            // Preenchimento padrão para o resto do mundo
            for (int i = 0; i < 12; i++) {
                hours[i] = current_rise + (double)i * daytime_hour;
            }
            for (int i = 0; i < 12; i++) {
                hours[i + 12] = current_set + (double)i * nighttime_hour;
            }

            hours[24] = next_rise;
        }

        int week_day = local_time->tm_wday;
        int planetary_hour = -1;

        // Ajuste de dia astrológico na madrugada (Apenas fora dos polos)
        if (!REGIAO_POLAR) {
            if (hour < current_rise) {
                hour += 24.0; 
                week_day -= 1;
                if (week_day < 0) week_day = 6;
            }
            else if (hour < current_rise + 4.0 && (hour + 24.0) < next_rise) {
                if (hour < current_set - 24.0) {
                    hour += 24.0;
                }
            }
        }

        // Determinação da hora planetária vigente
        if (REGIAO_POLAR) {
            // Como as 12 horas válidas ocupam o dia todo de 2h em 2h:
            int indice_direto = (int)(hour / 2.0);
            if (indice_direto > 11) indice_direto = 11;

            if (SOL_PERMANENTE) {
                planetary_hour = indice_direto; // Horas Diurnas (0 a 11)
            } else {
                planetary_hour = 12 + indice_direto; // Horas Noturnas (12 a 23)
            }
        } 
        else {
            if (hour >= current_rise && hour < current_set) {
                planetary_hour = (int)((hour - current_rise) / daytime_hour);
                if (planetary_hour > 11) planetary_hour = 11; 
            } 
            else {
                planetary_hour = 12 + (int)((hour - current_set) / nighttime_hour);
                if (planetary_hour > 23) planetary_hour = 23; 
            }
        }

        // =========================================================================
        // NORMALIZAÇÃO FINAL PARA EXIBIÇÃO NA INTERFACE (UI)
        // =========================================================================
        if (REGIAO_POLAR) {
            if (SOL_PERMANENTE) {
                nighttime_hour = 0.0; 
                daytime_hour = 2.0;   
            } else {
                daytime_hour = 0.0;    
                nighttime_hour = 2.0;  
            }
        }

        // Se hours[i] for menor que 0, deixamos como -1.0
        for (int i = 0; i <= 24; i++) {
            if (hours[i] >= 0.0) { 
                hours[i] = fmod(hours[i], 24.0);
                if (hours[i] < 0.0) hours[i] += 24.0;
            }
        }



    
        // 9. Calculate SAN (Syzygy Ante Nativitatem)

        double last_conj = find_last_astrological_event(julian_day, false);
        double last_opp  = find_last_astrological_event(julian_day, true);

        double SAN, jdSAN;

        int sanYear, sanMon, sanDay;
        double sanHour;

        int tipo_san = 0;
        
        char san[10];
        
        if (last_conj > last_opp) {
            snprintf(san, sizeof(san), "%s", "🌑");
            jdSAN = last_conj;
            tipo_san = SAN_CONJUNCIONAL;
        } 
        else {
            snprintf(san, sizeof(san), "%s", "🌕");
            jdSAN = last_opp;
            tipo_san = SAN_PREVENCIONAL;
        }
        
        if (swe_calc_ut(jdSAN, SE_MOON, SEFLG_SPEED, x2, err) < 0) {
            printf("Error calculating planet %d: %s\n", SE_MOON, err);
            return 1;
        }
        SAN = x2[0]; // Armazena a longitude correta da SAN
        
        //swe_revjul(jdSAN, SE_GREG_CAL, &sanYear, &sanMon, &sanDay, &sanHour);
        extract_local_datetime_from_jd(jdSAN, tz_offset, &sanYear, &sanMon, &sanDay, &sanHour);
        
        // calcular declinação da SAN

        double SAN_declination = calc_declination_mathematical_point(jdSAN, SAN);

        
        
        // 10. Moon phase

        double angle = get_moon_phase_by_longitude(julian_day);

        int moon_phase_id = get_moon_phase_id(angle);
        const char* phase = get_moon_phase_name_by_id(moon_phase_id);

        int phase_id = get_moon_quarter(angle);

       
        // 11. Lot of Fortune

        //char *hsol = (char *)get_house_roman(planet_longitudes[0], cusps);

        double fortuna;
        double spirit;
        //if (mapa_diurno(hsol)) {
        if (MAPA_DIURNO) {
            fortuna = ascendant + planet_longitudes[1] - planet_longitudes[0];
            spirit = ascendant + planet_longitudes[0] - planet_longitudes[1];
        } else {
            fortuna = ascendant + planet_longitudes[0] - planet_longitudes[1];
            spirit = ascendant + planet_longitudes[1] - planet_longitudes[0];
        }
        
        if (fortuna < 0) fortuna = fortuna + 360;
        if (fortuna > 360) fortuna = fortuna - 360;

        if (spirit < 0) spirit = spirit + 360;
        if (spirit > 360) spirit = spirit - 360;

        double fortuna_declination = calc_declination_mathematical_point(julian_day, fortuna);


        // 12. Rising Times

        double rising_times[12];
        
        for (int i = 0; i < 11; i++) {
            double geopos[3];
            
            geopos[0] = lon;
            geopos[1] = lat;
            geopos[2] = elev;
    
            int epheflag = SEFLG_SWIEPH;
            int rsmi = SE_CALC_RISE;
    
            double tret;
            char serr[256];
    
            int result = swe_rise_trans(jd, planets[i], NULL, epheflag, rsmi, geopos, 0, 0, &tret, serr);
    
            if (result == ERR) {
                fprintf(stderr, "Erro no calculo: %s\n", serr);
                exit(1);
            }
                
            // O resultado 'tret' vem em Dia Juliano. Podemos converter para horas
            double jd_diff = tret - jd;
            double rise_hours = jd_diff * 24.0;
            
            rising_times[i] = rise_hours;
    
        }
        {
            double geopos_espelho[3] = {lon, -lat, elev}; // Latitude invertida
            double tret;
            char serr[256];
            // Calculamos usando o ID do Nodo Norte (planets[10]) na latitude invertida
            int result = swe_rise_trans(jd, planets[10], NULL, SEFLG_SWIEPH, SE_CALC_RISE, geopos_espelho, 0, 0, &tret, serr);
            if (result != ERR) {
                double rise_hours = (tret - jd) * 24.0 + 12.0; // Soma 12 horas
                if (rise_hours >= 24.0) rise_hours -= 24.0;    // Normaliza para o intervalo do dia
                rising_times[11] = rise_hours;
            }
        }

        // 13. Set Times

        double set_times[12];
        
        for (int i = 0; i < 11; i++) {
            double geopos[3];
            
            geopos[0] = lon;
            geopos[1] = lat;
            geopos[2] = elev;
    
            int epheflag = SEFLG_SWIEPH;
            int rsmi = SE_CALC_SET;
    
            double tret;
            char serr[256];
    
            int result = swe_rise_trans(jd, planets[i], NULL, epheflag, rsmi, geopos, 0, 0, &tret, serr);
    
            if (result == ERR) {
                fprintf(stderr, "Erro no calculo: %s\n", serr);
                exit(1);
            }
                
            // O resultado 'tret' vem em Dia Juliano. Podemos converter para horas
            double jd_diff = tret - jd;
            double set_hours = jd_diff * 24.0;
            
            set_times[i] = set_hours;
    
        }
        {
            double geopos_espelho[3] = {lon, -lat, elev}; // Latitude invertida
            double tret;
            char serr[256];
            // Calculamos usando o ID do Nodo Norte (planets[10]) na latitude invertida
            int result = swe_rise_trans(jd, planets[10], NULL, SEFLG_SWIEPH, SE_CALC_SET, geopos_espelho, 0, 0, &tret, serr);
            if (result != ERR) {
                double set_hours = (tret - jd) * 24.0 + 12.0; // Soma 12 horas
                if (set_hours >= 24.0) set_hours -= 24.0;     // Normaliza para o intervalo do dia
                set_times[11] = set_hours;
            }
        }


        // 14. Mid Upper Times

        double mid_times[12];
        
        for (int i = 0; i < 11; i++) {
            double geopos[3];
            
            geopos[0] = lon;
            geopos[1] = lat;
            geopos[2] = elev;
    
            int epheflag = SEFLG_SWIEPH;
            int rsmi = SE_CALC_MTRANSIT;
    
            double tret;
            char serr[256];
    
            int result = swe_rise_trans(jd, planets[i], NULL, epheflag, rsmi, geopos, 0, 0, &tret, serr);
    
            if (result == ERR) {
                fprintf(stderr, "Erro no calculo: %s\n", serr);
                exit(1);
            }
                
            // O resultado 'tret' vem em Dia Juliano. Podemos converter para horas
            double jd_diff = tret - jd;
            double mid_hours = jd_diff * 24.0;
            
            mid_times[i] = mid_hours;
    
        }
        {
            double geopos[3] = {lon, lat, elev}; // Latitude NORMAL
            double tret;
            char serr[256];
            // O Meio do Céu do Nodo Sul é o Nadir (SE_CALC_ITRANSIT) do Nodo Norte
            int result = swe_rise_trans(jd, planets[10], NULL, SEFLG_SWIEPH, SE_CALC_ITRANSIT, geopos, 0, 0, &tret, serr);
            if (result != ERR) {
                mid_times[11] = (tret - jd) * 24.0;
            }
        }
        
                
        // 15. Building the PlotObject

        double south_node = normalize360(planet_longitudes[10] - 180);
        double descendant = normalize360(ascendant - 180);
        double ic = normalize360(mc - 180);

        double south_node_declination = -planet_declinations[10];
        double descendant_declination = -ascendant_declination;
        double ic_declination = -mc_declination;
        
        char d0[12];
        snprintf(d0, sizeof(d0), "%d°", (int)planet_longitudes[0] % 30);
        char d1[12];
        snprintf(d1, sizeof(d1), "%d°", (int)planet_longitudes[1] % 30);
        char d2[12];
        snprintf(d2, sizeof(d2), "%d°", (int)planet_longitudes[2] % 30);
        char d3[12];
        snprintf(d3, sizeof(d3), "%d°", (int)planet_longitudes[3] % 30);
        char d4[12];
        snprintf(d4, sizeof(d4), "%d°", (int)planet_longitudes[4] % 30);
        char d5[12];
        snprintf(d5, sizeof(d5), "%d°", (int)planet_longitudes[5] % 30);
        char d6[12];
        snprintf(d6, sizeof(d6), "%d°", (int)planet_longitudes[6] % 30);
        char d7[12];
        snprintf(d7, sizeof(d7), "%d°", (int)planet_longitudes[7] % 30);
        char d8[12];
        snprintf(d8, sizeof(d8), "%d°", (int)planet_longitudes[8] % 30);
        char d9[12];
        snprintf(d9, sizeof(d9), "%d°", (int)planet_longitudes[9] % 30);
        char d10[12];
        snprintf(d10, sizeof(d10), "%d°", (int)planet_longitudes[10] % 30);
        
        char df[12];
        snprintf(df, sizeof(df), "%d°", (int)fortuna % 30);
        char da[12];
        snprintf(da, sizeof(da), "%d°", (int)ascendant % 30);
        char dm[12];
        snprintf(dm, sizeof(dm), "%d°", (int)mc % 30);
        char ds[12];
        snprintf(ds, sizeof(ds), "%d°", (int)SAN % 30);
        char dv[12];
        snprintf(dv, sizeof(dv), "%d°", (int)vertex % 30);

        char m0[12];
        snprintf(m0, sizeof(m0), "%02d", (int)((planet_longitudes[0] - ((int)planet_longitudes[0])) * 60.0));
        char m1[12];
        snprintf(m1, sizeof(m1), "%02d", (int)((planet_longitudes[1] - ((int)planet_longitudes[1])) * 60.0));
        char m2[12];
        snprintf(m2, sizeof(m2), "%02d", (int)((planet_longitudes[2] - ((int)planet_longitudes[2])) * 60.0));
        char m3[12];
        snprintf(m3, sizeof(m3), "%02d", (int)((planet_longitudes[3] - ((int)planet_longitudes[3])) * 60.0));
        char m4[12];
        snprintf(m4, sizeof(m4), "%02d", (int)((planet_longitudes[4] - ((int)planet_longitudes[4])) * 60.0));
        char m5[12];
        snprintf(m5, sizeof(m5), "%02d", (int)((planet_longitudes[5] - ((int)planet_longitudes[5])) * 60.0));
        char m6[12];
        snprintf(m6, sizeof(m6), "%02d", (int)((planet_longitudes[6] - ((int)planet_longitudes[6])) * 60.0));
        char m7[12];
        snprintf(m7, sizeof(m7), "%02d", (int)((planet_longitudes[7] - ((int)planet_longitudes[7])) * 60.0));
        char m8[12];
        snprintf(m8, sizeof(m8), "%02d", (int)((planet_longitudes[8] - ((int)planet_longitudes[8])) * 60.0));
        char m9[12];
        snprintf(m9, sizeof(m9), "%02d", (int)((planet_longitudes[9] - ((int)planet_longitudes[9])) * 60.0));
        char m10[12];
        snprintf(m10, sizeof(m10), "%02d", (int)((planet_longitudes[10] - ((int)planet_longitudes[10])) * 60.0));

        char mf[12];
        snprintf(mf, sizeof(mf), "%02d", (int)((fortuna - ((int)fortuna)) * 60.0));
        char ma[12];
        snprintf(ma, sizeof(ma), "%02d", (int)((ascendant - ((int)ascendant)) * 60.0));
        char mm[12];
        snprintf(mm, sizeof(mm), "%02d", (int)((mc - ((int)mc)) * 60.0));
        char ms[12];
        snprintf(ms, sizeof(ms), "%02d", (int)((SAN - ((int)SAN)) * 60.0));
        char mv[12];
        snprintf(mv, sizeof(mv), "%02d", (int)((vertex - ((int)vertex)) * 60.0));


        int sign[19] = {
            (int)planet_longitudes[0] / 30,
            (int)planet_longitudes[1] / 30,
            (int)planet_longitudes[2] / 30,
            (int)planet_longitudes[3] / 30,
            (int)planet_longitudes[4] / 30,
            (int)planet_longitudes[5] / 30,
            (int)planet_longitudes[6] / 30,
            (int)planet_longitudes[7] / 30,
            (int)planet_longitudes[8] / 30,
            (int)planet_longitudes[9] / 30,
            (int)planet_longitudes[10] / 30,
            (int)fortuna / 30,
            (int)SAN / 30,
            (int)ascendant / 30,
            (int)mc / 30,
            (int)south_node / 30,
            (int)descendant / 30,
            (int)ic / 30,
            (int)vertex / 30
        };

        
        PlotObject plotsA[] = {
            {0, planet_longitudes[0], "☉", _("Sol"), get_sign(sign[0]), d0, m0, (speed[0] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[0], cusps), speed[0], rising_times[0], set_times[0], mid_times[0], planet_declinations[0]},
            {1, planet_longitudes[1], "☽", _("Luna"), get_sign(sign[1]), d1, m1, (speed[1] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[1], cusps), speed[1], rising_times[1], set_times[1], mid_times[1], planet_declinations[1]},
            {2, planet_longitudes[2], "☿", _("Mercury"), get_sign(sign[2]), d2, m2, (speed[2] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[2], cusps), speed[2], rising_times[2], set_times[2], mid_times[2], planet_declinations[2]},
            {3, planet_longitudes[3], "♀", _("Venus"), get_sign(sign[3]), d3, m3, (speed[3] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[3], cusps), speed[3], rising_times[3], set_times[3], mid_times[3], planet_declinations[3]},
            {4, planet_longitudes[4], "♂", _("Mars"), get_sign(sign[4]), d4, m4, (speed[4] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[4], cusps), speed[4], rising_times[4], set_times[4], mid_times[4], planet_declinations[4]},
            {5, planet_longitudes[5], "♃", _("Jupiter"), get_sign(sign[5]), d5, m5, (speed[5] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[5], cusps), speed[5], rising_times[5], set_times[5], mid_times[5], planet_declinations[5]},
            {6, planet_longitudes[6], "♄", _("Saturn"), get_sign(sign[6]), d6, m6, (speed[6] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[6], cusps), speed[6], rising_times[6], set_times[6], mid_times[6], planet_declinations[6]},
            {7, planet_longitudes[7], "♅", _("Uranus"), get_sign(sign[7]), d7, m7, (speed[7] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[7], cusps), speed[7], rising_times[7], set_times[7], mid_times[7], planet_declinations[7]},
            {8, planet_longitudes[8], "♆", _("Neptune"), get_sign(sign[8]), d8, m8, (speed[8] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[8], cusps), speed[8], rising_times[8], set_times[8], mid_times[8], planet_declinations[8]},
            {9, planet_longitudes[9], "⯓", _("Pluto"), get_sign(sign[9]), d9, m9, (speed[9] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[9], cusps), speed[9], rising_times[9], set_times[9], mid_times[9], planet_declinations[9]},
            {10, planet_longitudes[10], "☊", _("North Node"), get_sign(sign[10]), d10, m10, (speed[10] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[10], cusps), speed[10], rising_times[10], set_times[10], mid_times[10], planet_declinations[10]},
            {11, south_node, "☋", _("South Node"), get_sign(sign[15]), d10, m10, (speed[10] >= 0 ? "": "℞"), (char *)get_house_roman(south_node, cusps), speed[10], rising_times[11], set_times[11], mid_times[11],south_node_declination},
            {12, fortuna, "🝴", _("Part of Fortune"), get_sign(sign[11]), df, mf, "", (char *)get_house_roman(fortuna, cusps), 0.0, 0.0, 0.0, 0.0, fortuna_declination},
            {13, SAN, san, _("SAN"), get_sign(sign[12]), ds, ms, "", (char *)get_house_roman(SAN, cusps), 0.0, 0.0, 0.0, 0.0, SAN_declination},
            {14, ascendant, "AC", _("Ascendant"), get_sign(sign[13]), da, ma, "", (char *)get_house_roman(ascendant, cusps), 0.0, 0.0, 0.0, 0.0, ascendant_declination},
            {15, mc, "MC", _("Midheaven"), get_sign(sign[14]), dm, mm, "", (char *)get_house_roman(mc, cusps), 0.0, 0.0, 0.0, 0.0, mc_declination},
            {16, descendant, "DC", _("Descendant"), get_sign(sign[16]), da, ma, "", (char *)get_house_roman(descendant, cusps), 0.0, 0.0, 0.0, 0.0, descendant_declination},
            {17, ic, "IC", _("Nadir"), get_sign(sign[17]), dm, mm, "", (char *)get_house_roman(ic, cusps), 0.0, 0.0, 0.0, 0.0, ic_declination},
            {18, vertex, "🜊", _("Vertex"), get_sign(sign[18]), dv, mv, "", (char *)get_house_roman(vertex, cusps), 0.0, 0.0, 0.0, 0.0, vertex_declination}
        };

        PlotObject plotsB[] = {
            {0, planet_longitudes[0], "☉", _("Sol"), get_sign(sign[0]), d0, m0, (speed[0] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[0], cusps), speed[0], rising_times[0], set_times[0], mid_times[0], planet_declinations[0]},
            {1, planet_longitudes[1], "☽", _("Luna"), get_sign(sign[1]), d1, m1, (speed[1] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[1], cusps), speed[1], rising_times[1], set_times[1], mid_times[1], planet_declinations[1]},
            {2, planet_longitudes[2], "☿", _("Mercury"), get_sign(sign[2]), d2, m2, (speed[2] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[2], cusps), speed[2], rising_times[2], set_times[2], mid_times[2], planet_declinations[2]},
            {3, planet_longitudes[3], "♀", _("Venus"), get_sign(sign[3]), d3, m3, (speed[3] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[3], cusps), speed[3], rising_times[3], set_times[3], mid_times[3], planet_declinations[3]},
            {4, planet_longitudes[4], "♂", _("Mars"), get_sign(sign[4]), d4, m4, (speed[4] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[4], cusps), speed[4], rising_times[4], set_times[4], mid_times[4], planet_declinations[4]},
            {5, planet_longitudes[5], "♃", _("Jupiter"), get_sign(sign[5]), d5, m5, (speed[5] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[5], cusps), speed[5], rising_times[5], set_times[5], mid_times[5], planet_declinations[5]},
            {6, planet_longitudes[6], "♄", _("Saturn"), get_sign(sign[6]), d6, m6, (speed[6] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[6], cusps), speed[6], rising_times[6], set_times[6], mid_times[6], planet_declinations[6]},
            {7, planet_longitudes[10], "☊", _("North Node"), get_sign(sign[10]), d10, m10, (speed[10] >= 0 ? "": "℞"), (char *)get_house_roman(planet_longitudes[10], cusps), speed[10], rising_times[10], set_times[10], mid_times[10], planet_declinations[10]},
            {8, south_node, "☋", _("South Node"), get_sign(sign[15]), d10, m10, (speed[10] >= 0 ? "": "℞"), (char *)get_house_roman(south_node, cusps), speed[10], rising_times[11], set_times[11], mid_times[11],south_node_declination},
            {9, fortuna, "🝴", _("Part of Fortune"), get_sign(sign[11]), df, mf, "", (char *)get_house_roman(fortuna, cusps), 0.0, 0.0, 0.0, 0.0, fortuna_declination},
            {10, SAN, san, _("SAN"), get_sign(sign[12]), ds, ms, "", (char *)get_house_roman(SAN, cusps), 0.0, 0.0, 0.0, 0.0, SAN_declination},
            {11, ascendant, "AC", _("Ascendant"), get_sign(sign[13]), da, ma, "", (char *)get_house_roman(ascendant, cusps), 0.0, 0.0, 0.0, 0.0, ascendant_declination},
            {12, mc, "MC", _("Midheaven"), get_sign(sign[14]), dm, mm, "", (char *)get_house_roman(mc, cusps), 0.0, 0.0, 0.0, 0.0, mc_declination},
            {13, descendant, "DC", _("Descendant"), get_sign(sign[16]), da, ma, "", (char *)get_house_roman(descendant, cusps), 0.0, 0.0, 0.0, 0.0, descendant_declination},
            {14, ic, "IC", _("Nadir"), get_sign(sign[17]), dm, mm, "", (char *)get_house_roman(ic, cusps), 0.0, 0.0, 0.0, 0.0, ic_declination},
            {15, vertex, "🜊", _("Vertex"), get_sign(sign[18]), dv, mv, "", (char *)get_house_roman(vertex, cusps), 0.0, 0.0, 0.0, 0.0, vertex_declination}
        };
        
        PlotObject *plots;
        if (!show_modern_planets) {
            plots = plotsB;  
        }
        else {
            plots = plotsA;
        }

        PlanetDignities dig[NUM_OBJECTS - object_diff];

        for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
            if (i < 7) {
                dig[i].id = i + 1;
            }
            else {
                dig[i].id = i + 1 + object_diff;
            }
            
            dig[i].essential = 0;
            dig[i].accidental = 0;
            dig[i].row.movement = 0;
            dig[i].row.fast = 0;
            dig[i].row.feral = 0;
            dig[i].row.void_of_course = 0;
            dig[i].row.orientality = 0;
            dig[i].row.combust = 0;
            dig[i].row.under_rays = 0;
            dig[i].row.cazimi = 0;
            dig[i].row.under_siege = 0;
            dig[i].row.under_assistance = 0;
            dig[i].row.asp_benef_conj = 0;
            dig[i].row.asp_benef_trine = 0;
            dig[i].row.asp_benef_sextile = 0;
            dig[i].row.asp_malef_conj = 0;
            dig[i].row.asp_malef_opp = 0;
            dig[i].row.asp_malef_square = 0;
            dig[i].row.north_node_conj = 0;
            dig[i].row.south_node_conj = 0;
            dig[i].row.haym = 0;
            dig[i].row.hayz = 0;
            dig[i].row.hayz_extra = 0;
            dig[i].row.mut_reception = 0;
            dig[i].row.mut_reception_asp = 0;
        }

        int retro[14 - object_diff];
        for (int i = 0; i < 14 - object_diff; i++) {
            retro[i] = (strcmp(plots[i].retrograde, "℞") == 0) ? 1 : 0;
        }

        
        for (int i = 0; i < 12 - object_diff; i++) {
            if (retro[i]) {
                dig[i].accidental -= 5;
                dig[i].row.movement = -1;
            } 
            else if (plots[i].speed > 0.0) {
                if (plots[i].id > 1) {
                    dig[i].accidental += 4;
                }
                dig[i].row.movement = 1;
            }
        }


        // 16. Calculate Aspects

        AspectMatrix matrix = {0}; // Inicializa tudo zerado/falso
        int feral[12 - object_diff];
        int vazio_de_curso[12 - object_diff];

        
        matrix = calculate_aspects(plots, planet_orbis, dig, feral, vazio_de_curso, retro);
     

        for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
            // Cerco
            if (plots[i].id != P_MARS && plots[i].id != P_SATURN) {
                if (is_under_siege(plots[i].id, &matrix)) {

                    dig[i].accidental -= 5;
                    dig[i].row.under_siege = 1;
                }
            }

            // Auxílio
            if (plots[i].id != P_VENUS && plots[i].id != P_JUPITER) {
                if (is_under_assistance(plots[i].id, &matrix)) {
                    
                    dig[i].accidental += 5;
                    dig[i].row.under_assistance = 1;
                }
            }


            // Aspectos com maléficos

            if (plots[i].id != P_MARS) {
                if (((matrix.grid[i][P_MARS].has_aspect || matrix.grid[P_MARS][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_MARS].symbol, "☌") || strstr(matrix.grid[P_MARS][i].symbol, "☌"))) {
                    
                    dig[i].accidental -= 5;
                    dig[i].row.asp_malef_conj += 1;
                }
            }
            if (plots[i].id != P_SATURN) {
                if (((matrix.grid[i][P_SATURN].has_aspect || matrix.grid[P_SATURN][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_SATURN].symbol, "☌") || strstr(matrix.grid[P_SATURN][i].symbol, "☌"))) {
                    
                    dig[i].accidental -= 5;
                    dig[i].row.asp_malef_conj += 1;
                }
            }

            if (plots[i].id != P_MARS) {
                if (((matrix.grid[i][P_MARS].has_aspect || matrix.grid[P_MARS][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_MARS].symbol, "☍") || strstr(matrix.grid[P_MARS][i].symbol, "☍"))) {
                    
                    dig[i].accidental -= 4;
                    dig[i].row.asp_malef_opp += 1;
                }
            }
            if (plots[i].id != P_SATURN) {
                if (((matrix.grid[i][P_SATURN].has_aspect || matrix.grid[P_SATURN][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_SATURN].symbol, "□") || strstr(matrix.grid[P_SATURN][i].symbol, "□"))) {
                    
                    dig[i].accidental -= 3;
                    dig[i].row.asp_malef_square += 1;
                }
            }
            if (plots[i].id != P_SATURN) {
                if (((matrix.grid[i][P_SATURN].has_aspect || matrix.grid[P_SATURN][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_SATURN].symbol, "☍") || strstr(matrix.grid[P_SATURN][i].symbol, "☍"))) {
                    
                    dig[i].accidental -= 4;
                    dig[i].row.asp_malef_opp += 1;
                }
            }
            if (plots[i].id != P_MARS) {
                if (((matrix.grid[i][P_MARS].has_aspect || matrix.grid[P_MARS][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_MARS].symbol, "□") || strstr(matrix.grid[P_MARS][i].symbol, "□"))) {
                    
                    dig[i].accidental -= 3;
                    dig[i].row.asp_malef_square += 1;
                }
            }


            // Aspectos com benéficos

            if (plots[i].id != P_VENUS) {
                if (((matrix.grid[i][P_VENUS].has_aspect || matrix.grid[P_VENUS][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_VENUS].symbol, "☌") || strstr(matrix.grid[P_VENUS][i].symbol, "☌"))) {
                    
                    dig[i].accidental += 5;
                    dig[i].row.asp_benef_conj += 1;
                }
            }
            if (plots[i].id != P_JUPITER) {
                if (((matrix.grid[i][P_JUPITER].has_aspect || matrix.grid[P_JUPITER][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_JUPITER].symbol, "△") || strstr(matrix.grid[P_JUPITER][i].symbol, "△"))) {
                    
                    dig[i].accidental += 4;
                    dig[i].row.asp_benef_trine += 1;
                }
            }

            if (plots[i].id != P_VENUS) {
                if (((matrix.grid[i][P_VENUS].has_aspect || matrix.grid[P_VENUS][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_VENUS].symbol, "⚹") || strstr(matrix.grid[P_VENUS][i].symbol, "⚹"))) {
                    
                    dig[i].accidental += 3;
                    dig[i].row.asp_benef_sextile += 1;
                }
            }

            if (plots[i].id != P_JUPITER) {
                if (((matrix.grid[i][P_JUPITER].has_aspect || matrix.grid[P_JUPITER][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_JUPITER].symbol, "☌") || strstr(matrix.grid[P_JUPITER][i].symbol, "☌"))) {
                    
                    dig[i].accidental += 5;
                    dig[i].row.asp_benef_conj += 1;
                }
            }
            if (plots[i].id != P_VENUS) {
                if (((matrix.grid[i][P_VENUS].has_aspect || matrix.grid[P_VENUS][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_VENUS].symbol, "△") || strstr(matrix.grid[P_VENUS][i].symbol, "△"))) {
                    
                    dig[i].accidental += 4;
                    dig[i].row.asp_benef_trine += 1;
                }
            }

            if (plots[i].id != P_JUPITER) {
                if (((matrix.grid[i][P_JUPITER].has_aspect || matrix.grid[P_JUPITER][i].has_aspect)) &&
                    (strstr(matrix.grid[i][P_JUPITER].symbol, "⚹") || strstr(matrix.grid[P_JUPITER][i].symbol, "⚹"))) {
                    
                    dig[i].accidental += 3;
                    dig[i].row.asp_benef_sextile += 1;
                }
            }


            // Conjunção com nodos

            if (plots[i].id != P_NORTH_NODE - object_diff) {
                if ((matrix.grid[i][P_NORTH_NODE - object_diff].has_aspect || matrix.grid[P_NORTH_NODE - object_diff][i].has_aspect) &&
                    (strstr(matrix.grid[i][P_NORTH_NODE - object_diff].symbol, "☌") || strstr(matrix.grid[P_NORTH_NODE - object_diff][i].symbol, "☌"))) {
                    
                    dig[i].accidental += 4;
                    dig[i].row.north_node_conj = 1;
                }
            }

            if (plots[i].id != P_SOUTH_NODE - object_diff) {
                if ((matrix.grid[i][P_SOUTH_NODE - object_diff].has_aspect || matrix.grid[P_SOUTH_NODE - object_diff][i].has_aspect) &&
                    (strstr(matrix.grid[i][P_SOUTH_NODE - object_diff].symbol, "☌") || strstr(matrix.grid[P_SOUTH_NODE - object_diff][i].symbol, "☌"))) {
                    
                    dig[i].accidental -= 4;
                    dig[i].row.south_node_conj = 1;
                }
            }
        }
        


        
        // 17. Declination Aspects

        DeclMatrix matrix_decl = {0}; // Inicializa toda a estrutura com falsos/zeros
        matrix_decl = calculate_declination_aspects(plots, decl_orbis);

        


        // 18. Table

        PlanetTableMatrix planet_matrix = {0};

        
        for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
            PlanetRowData *row = &planet_matrix.rows[i];

            double longitude_total = plots[i].longitude;

            // Decan
            int ndec = get_decan(longitude_total);
            int decan_ruler = get_decan_ruler(ndec);

            char rdec[10];
            snprintf(rdec, 10, "%s", planet_regent_symbols[decan_ruler]);
            snprintf(row->decan, 10, "%s", rdec);


            // Termo (Bounds)
            int regente_do_termo = get_term_ruler(longitude_total);            
            
            char rterm[10];
            snprintf(rterm, sizeof(rterm), "%s", planet_regent_symbols[regente_do_termo]);
            snprintf(row->term, 10, "%s", rterm);
            
            
            
            // Declinação
            snprintf(row->decl_str, 15, "%6.2f", plots[i].declination);

            // Velocidade e Cores
            double speed_abs = fabs(plots[i].speed);
            double mean_speed;
            
            get_planet_speed(plots[i].object, &mean_speed);
            // if (plots[i].id == 13 - object_diff) {
            //     get_planet_speed(plots[P_LUNA].object, &mean_speed);   
            // }

            int fast[14 - object_diff];

            row->speed_color_pair = 0;
            if (i < 12 - object_diff) {
                snprintf(row->speed_str, 15, "%8.4f", plots[i].speed);
                if (speed_abs > mean_speed) {
                    row->speed_color_pair = 8;

                    if (i < 12 - object_diff) {
                        fast[i] = 1;
                        dig[i].row.fast = 1;
                    }
                }
                else if (speed_abs < mean_speed) {
                    row->speed_color_pair = 11;
                    if (i < 12 - object_diff) {
                        fast[i] = -1;
                        dig[i].row.fast = -1;
                    }
                }
                else if (speed_abs == mean_speed) {
                    row->speed_color_pair = 7;
                    if (i < 12 - object_diff) {
                        fast[i] = 0;
                        dig[i].row.fast = 0;
                    }
                }
            } else {
                snprintf(row->speed_str, 15, " ");
            }



            if (i < 12 - object_diff) {
                if (fast[i]) {
                    dig[i].accidental +=2;
                }
                else if (fast[i] < 0) {
                    dig[i].accidental -=2;
                }
            }

           
           
            if (i < 12 - object_diff) {
                
                // se forem os nodos, não se aplica VoC nem Feral
                if (i >= 10 - object_diff) {
                    vazio_de_curso[i] = 0;
                    feral[i] = 0;
                }

                if (feral[i]) {
                    dig[i].accidental -= 3;
                    dig[i].row.feral = 1;
                }
                else if (vazio_de_curso[i]) {
                    dig[i].accidental -= 2;
                    dig[i].row.void_of_course = 1;
                }                
            }

            // Joy

            if (i < 7) {
                if (get_house(plots[i].longitude, cusps) == get_planetary_joy(plots[i].id + 1) ) {
                    dig[i].row.joy = 1;
                    dig[i].accidental += 2;
                }
                else {
                    dig[i].row.joy = 0;
                }

                if (get_sign_joy(plots[i].id + 1) == (int)(plots[i].longitude / 30.0) + 1) {
                    dig[i].row.sign_joy = 1;
                    dig[i].accidental += 3;
                }
                else {
                    dig[i].row.sign_joy = 0;
                }
            }

            // Classificação de Cores das Casas
            row->house_color_pair = 0;
            if (strcmp(plots[i].house, "VI") == 0 || strcmp(plots[i].house, "VIII") == 0 || strcmp(plots[i].house, "XII") == 0) {
                row->house_color_pair = 11;
                
            }
            else if (strcmp(plots[i].house, "I") == 0 || strcmp(plots[i].house, "II") == 0 || strcmp(plots[i].house, "III") == 0 || 
                    strcmp(plots[i].house, "V") == 0 || strcmp(plots[i].house, "IX") == 0 || strcmp(plots[i].house, "X") == 0 || strcmp(plots[i].house, "XI") == 0) {
                row->house_color_pair = 8;
            }
            else if (strcmp(plots[i].house, "IV") == 0 || strcmp(plots[i].house, "VII") == 0) {
                row->house_color_pair = 7;
            }

            

            if (i < NUM_OBJECTS - object_diff) {
                switch(get_house(plots[i].longitude, cusps)) {
                    case 1: dig[i].accidental += HOUSE_1_PT; break;
                    case 2: dig[i].accidental += HOUSE_2_PT; break;
                    case 3: dig[i].accidental += HOUSE_3_PT; break;
                    case 4: dig[i].accidental += HOUSE_4_PT; break;
                    case 5: dig[i].accidental += HOUSE_5_PT; break;
                    case 6: dig[i].accidental += HOUSE_6_PT; break;
                    case 7: dig[i].accidental += HOUSE_7_PT; break;
                    case 8: dig[i].accidental += HOUSE_8_PT; break;
                    case 9: dig[i].accidental += HOUSE_9_PT; break;
                    case 10: dig[i].accidental += HOUSE_10_PT; break;
                    case 11: dig[i].accidental += HOUSE_11_PT; break;
                    case 12: dig[i].accidental += HOUSE_12_PT; break;
                }
            }



            // Dignidades Essenciais Tradicionais
            snprintf(row->dignity_str, 70, " ");
            row->dignity_color_pair = 0;

            int pilgrim = 1;
            dig[i].major_dig = NO_MAJOR_DIGNITY;
            dig[i].row.pilgrim = 1;

            if (i < 12 - object_diff) {
                int n_ruler, n_exalted, n_exile, n_fall, r1, r2, r3;
                get_rulers(plots[i].sign, &n_ruler, &n_exalted, &n_exile, &n_fall, &r1, &r2, &r3);

                char dom_str[10] = "";
                char exalt_str[10] = "";
                char trip_str[10] = "";
                char term_str[10] = "";
                char dec_str[10] = "";
                char exile_str[10] = "";
                char fall_str[10] = "";
                char pilgrim_str[10] = "";

                int dom = 0;
                if (n_ruler == i + 1) {
                    row->dignity_color_pair = 8;
                    snprintf(dom_str, 10, "%s", _("Dom "));
                    if (i < 7) {
                        pilgrim = 0;
                        dig[i].row.pilgrim = 0;
                    }
                    if (i < 12 - object_diff) {
                        dig[i].essential += 5;
                        dig[i].major_dig += DOMICILE;
                    }
                    dom = 1;
                }

                int exalt = 0;
                if ((n_exalted <= 7 + (consider_modern_planets_rulling ? 3 : 0) && n_exalted == (i + 1)) ||
                    (n_exalted > 10 && n_exalted - object_diff == (i + 1))) {
                    row->dignity_color_pair = 8;
                    snprintf(exalt_str, 10, "%s", _("Exalt "));
                    if (i < 7) {
                        pilgrim = 0;
                        dig[i].row.pilgrim = 0;
                    }
                    if (i < 12 - object_diff) {
                        dig[i].essential += 4;
                        dig[i].major_dig += EXALTATION;
                    }
                    exalt = 1;
                }

                int trip = 0;
                if (i + 1 == r1 || i + 1 == r2 || i + 1 == r3) {
                    trip = 1;
                    row->dignity_color_pair = 8;
                    snprintf(trip_str, 10, "%s", _("Tri "));
                    if (i < 7) {
                        pilgrim = 0;
                        dig[i].row.pilgrim = 0;
                    }
                    if (i < 12 - object_diff) {
                        dig[i].essential += 3;
                    }
                }

                int dec = 0;
                if (strcmp(rdec, plots[i].object) == 0) {
                    dec = 1;
                    row->dignity_color_pair = 8;
                    snprintf(dec_str, 10, "%s", _("Dec "));
                    if (i < 7) {
                        pilgrim = 0;
                        dig[i].row.pilgrim = 0;
                    }
                    if (i < 12 - object_diff) {
                        dig[i].essential += 1;
                    }
                }

                int term = 0;
                if (strcmp(rterm, plots[i].object) == 0) {
                    term = 1; 
                    row->dignity_color_pair = 8;
                    snprintf(term_str, 10, "%s", _("Term "));
                    if (i < 7) {
                        pilgrim = 0;
                        dig[i].row.pilgrim = 0;
                    }
                    if (i < 12 - object_diff) {
                        dig[i].essential += 2;
                    }
                }

                int tem_debilidade = 0;

                if (n_exile == i + 1) {
                    tem_debilidade = 1;
                    pilgrim = 0; // Independente de ser tradicional ou não, se tem exílio não é peregrino
                    dig[i].row.pilgrim = 0;
                    snprintf(exile_str, 10, "%s", _("Exile "));
                    if (i < 12 - object_diff) {
                        dig[i].essential -= 5;
                        dig[i].major_dig += EXILE;
                    }
                }

                if ((n_fall <= 7 + (consider_modern_planets_rulling ? 3 : 0) && n_fall == i + 1) ||
                    (n_fall > 10 && n_fall - object_diff == i + 1)) {
                    tem_debilidade = 1;
                    pilgrim = 0; // Se tem fall, não é peregrino
                    dig[i].row.pilgrim = 0;
                    snprintf(fall_str, 10, "%s", _("Fall "));
                    if (i < 12 - object_diff) {
                        dig[i].essential -= 4;
                        dig[i].major_dig += FALL;
                    }
                }

                // Tem alguma dignidade calculada antes?
                int tem_dignidade = (dig[i].major_dig > 0) || dom || exalt || trip || dec || term;

                // Se houver dignidades menores, ele deixa de ser peregrino (independente de ser i < 7 ou não)
                if (tem_dignidade) {
                    pilgrim = 0;
                    dig[i].row.pilgrim = 0;
                }

                // ATRIBUIÇÃO FINAL DA COR (Hierarquia correta e sem sobreposição indesejada)
                if (pilgrim) {
                    row->dignity_color_pair = 11; // Peregrino recebe a cor de desfavorável
                    if (i < 7) {
                        snprintf(pilgrim_str, 10, "%s", _("pilgrim "));
                    }
                } 
                else if (tem_debilidade && tem_dignidade) {
                    row->dignity_color_pair = 7;  // Condição mista (Exílio/Queda com Tri/Term/Dec)
                } 
                else if (tem_debilidade) {
                    row->dignity_color_pair = 11; // Debilidade pura
                } 
                else if (tem_dignidade) {
                    row->dignity_color_pair = 8;  // Dignidade pura
                } 
                else {
                    row->dignity_color_pair = 0;  // Neutro absoluto
                }

                snprintf(row->dignity_str, 80, "%s%s%s%s%s%s%s%s", dom_str, exalt_str, exile_str, fall_str, trip_str, term_str, dec_str, pilgrim_str);
                
                if (i < 7) {
                    if (pilgrim) {
                        dig[i].accidental -= 5;
                    }
                }
            }


            // Recepção mútua
            if (i < 12 - object_diff) {    
                snprintf(row->mutual_reception, 32, " ");

                for (int j = 0; j < 12 - object_diff; j++) {
                    
                    if (i == j) continue;
                    if (strcmp(plots[i].sign, plots[j].sign) == 0) continue;
            
                    char *ruler_dom_de_i = NULL; // Quem rege o signo onde 'i' está hospedado
                    char *ruler_exal_de_i = NULL; // Quem exalta no signo onde 'i' está hospedado
                    
                    char *ruler_dom_de_j = NULL; // Quem rege o signo onde 'j' está hospedado
                    char *ruler_exal_de_j = NULL; // Quem exalta no signo onde 'j' está hospedado
                    
                    // 1. Descobre os donos da casa onde o Planeta I está pisando
                    get_sign_ruler_by_domicile(plots[i].sign, &ruler_dom_de_i);
                    get_sign_ruler_by_exaltation(plots[i].sign, &ruler_exal_de_i, consider_modern_planets_rulling);
                    
                    // 2. Descobre os donos da casa onde o Planeta J está pisando
                    get_sign_ruler_by_domicile(plots[j].sign, &ruler_dom_de_j);
                    get_sign_ruler_by_exaltation(plots[j].sign, &ruler_exal_de_j, consider_modern_planets_rulling);
                    
                    // 3. A VERDADEIRA RECEPÇÃO MÚTUA (CRUZADA):
                    // O Planeta J é dono do lugar onde o Planeta I está? 
                    int j_recebe_i = (ruler_dom_de_i && strcmp(plots[j].object, ruler_dom_de_i) == 0) || 
                                    (ruler_exal_de_i && strcmp(plots[j].object, ruler_exal_de_i) == 0);
            
                    // O Planeta I é dono do lugar onde o Planeta J está?
                    int i_recebe_j = (ruler_dom_de_j && strcmp(plots[i].object, ruler_dom_de_j) == 0) || 
                                    (ruler_exal_de_j && strcmp(plots[i].object, ruler_exal_de_j) == 0);
            
                    int ha_recepcao = j_recebe_i && i_recebe_j && strcmp(plots[i].sign, plots[j].sign) != 0;
            
                    if (ha_recepcao) {
                        int sign_i = (int)floor(plots[i].longitude / 30) + 1;
                        int sign_j = (int)floor(plots[j].longitude / 30) + 1;
                        int sign_diff = diff_sign(sign_i, sign_j);

                        int tem_aspecto = matrix.grid[i][j].has_aspect || matrix.grid[j][i].has_aspect || 
                                          (sign_diff != 1 && sign_diff != 5);
                        
                        char mut[14];
                        if (tem_aspecto) {
                            if (ruler_dom_de_i && strcmp(plots[j].object, ruler_dom_de_i) == 0) {
                                dig[i].accidental += 5;
                            }
                            else if (ruler_exal_de_i && strcmp(plots[j].object, ruler_exal_de_i) == 0) {
                                dig[i].accidental += 4;
                            }

                            snprintf(mut, 14, "%s / %s", plots[i].object, plots[j].object);
                            strncat(row->mutual_reception, mut, 32 - strlen(row->mutual_reception) - 1);

                            dig[i].row.mut_reception = 1;
                            dig[i].row.mut_reception_asp = 1;
                        } else {
                            dig[i].accidental += 1; 

                            snprintf(mut, 14, "%s / %s", plots[i].object, plots[j].object);
                            strncat(row->mutual_reception, mut, 32 - strlen(row->mutual_reception) - 1);

                            dig[i].row.mut_reception = 1;
                            dig[i].row.mut_reception_asp = 0;
                        }
                    } 
                    
            
                    // Liberação de memória segura dentro do laço
                    if (ruler_dom_de_i) free(ruler_dom_de_i);
                    if (ruler_exal_de_i) free(ruler_exal_de_i);
                    if (ruler_dom_de_j) free(ruler_dom_de_j);
                    if (ruler_exal_de_j) free(ruler_exal_de_j);
                }
            }


            // Lógica de Gênero
            int planet_gen = -1, sign_gen = -1, quadrant_gen = -1;
            get_planet_gender(plots[i].object, &planet_gen);
            get_sign_gender(plots[i].sign, &sign_gen);
            

            double sun_rising_time = plots[0].rising_time;
            double sun_rise_plus_12 = plots[0].rising_time + 12.0;
            if (sun_rise_plus_12 < 0.0) sun_rise_plus_12 += 24.0;
            if (sun_rise_plus_12 > 24.0) sun_rise_plus_12 -= 24.0;

            if (i == 2) { // Mercúrio Condicional Oriental/Ocidental
                if (plots[i].rising_time < sun_rising_time || plots[i].rising_time > sun_rise_plus_12) {
                    planet_gen = 1;
                }
                else {
                    planet_gen = 2;
                }
            }
            row->gender_match = (sign_gen == planet_gen);

        
            int house_s = get_house(plots[i].longitude, cusps);
            
            // Gender concerning to the quadrant
            get_quadrant_gender(house_s, &quadrant_gen);
            row->quadrant_match = (quadrant_gen == planet_gen);

            // Lógica de Seita (Sect / Hayz parcial)
            int planet_sect = -1;
            
            get_planet_sect(plots[i].object, &planet_sect);

            
            if (i == 2) { // Mercúrio Condicional Oriental/Ocidental
                if (plots[i].rising_time < sun_rising_time || plots[i].rising_time > sun_rise_plus_12) {
                    planet_sect = 1;
                }
                else {
                    planet_sect = 2;
                }
            }

            row->sect_match = false;
            if (MAPA_DIURNO) {
                if ((planet_sect == 1 && house_s > 6) || (planet_sect == 2 && house_s < 7 && i != 1) || i == 0) {
                    row->sect_match = true;
                }
            } else {
                if ((planet_sect == 1 && house_s < 7 && i != 0) || (planet_sect == 2 && house_s > 6) || i == 1) {
                    row->sect_match = true;
                }
            }


            int haym = 0;
            int hayz = 0;
            int hayz_extra = 0;

            if (i < 7) {            
                if (row->gender_match && row->sect_match) {
                    hayz = 1;
                    haym = 0;
                    if (row->quadrant_match) {
                        hayz_extra = 1;
                    }             
                }
                else if (!row->gender_match && !row->sect_match) {
                    hayz = -1;
                    haym = 0;
                }
                else if (row->sect_match) {
                    hayz = 0;
                    haym = 1;
                }

                if (hayz == 1) {
                    dig[i].accidental += 3;
                    dig[i].row.hayz = 1;
                    dig[i].row.haym = 0;

                    if (hayz_extra) {
                        dig[i].accidental += 2; 
                        dig[i].row.hayz_extra = 1;
                    }
                } 
                else if (hayz == -1) {
                    dig[i].accidental -= 2;
                    dig[i].row.hayz = -1;
                } 
                else if (hayz == 0) {
                    dig[i].row.hayz = 0;
                }

                if (haym) {
                    dig[i].accidental += 2;
                    dig[i].row.haym = 1;
                }
            }

            //DEBUG
            // terminate_database();
            // endwin();
            // for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
            //     fprintf(stderr, "%d %d %d\n", dig[i].id, dig[i].essential, dig[i].accidental);
            // }        
            // exit(1);
                
            
            // Orientality

            if (i < 12 - object_diff) {
                char *planet_orientality = " ";
                char orientality_str[11];

                int found = get_planet_orientality(plots[i].object, &planet_orientality);


                if (plots[i].rising_time < sun_rising_time || plots[i].rising_time > sun_rise_plus_12) {
                    snprintf(row->orientality_str, 30, "%s", _("Orient"));
                    snprintf(orientality_str, 11, "%s", "Oriental");
                }
                else {
                    snprintf(row->orientality_str, 30, "%s", _("Occid"));
                    snprintf(orientality_str, 11, "%s", "Occidental");
                }

                if (strcmp(orientality_str, planet_orientality) == 0) {
                    row->orientality_color_pair = 8;

                    dig[i].accidental += 2;
                    dig[i].row.orientality = 1;
                }
                else if (strcmp(" ", planet_orientality) != 0) {
                    row->orientality_color_pair = 11;

                    dig[i].accidental -= 2;
                    dig[i].row.orientality = 0;
                }

                if (found) free(planet_orientality);

            }
            


            // Busca e Formatação dos Regentes (Lidando com os Ponteiros e Free com segurança)
            char *ruler = NULL;
            char *ruler2 = NULL;
            get_sign_ruler_by_domicile(plots[i].sign, &ruler);
            get_sign_ruler_by_exaltation(plots[i].sign, &ruler2, consider_modern_planets_rulling);

            int rd, re, rex, rf, rt1, rt2, rt3;
            get_rulers(plots[i].sign, &rd, &re, &rex, &rf, &rt1, &rt2, &rt3);        
            
            snprintf(row->rulers_str, 30, "%s %s", ruler ? ruler : "", ruler2 ? ruler2 : "");
            snprintf(row->tri, 30, "%s %s %s", plots[rt1 - 1].object, plots[rt2 - 1].object, plots[rt3 - 1].object);

            if (ruler) free(ruler);
            if (ruler2) free(ruler2);
        }

        
        // Season and its temperament

        // char *season = " ";
        // int found_season = get_season(plots[0].sign, &season);

        // char *season_temperament = " ";
        // int found_temp = get_season_temperament(season, &season_temperament);

        int season_id = 0;
        get_season_id((int)(plots[0].longitude / 30) + 1, &season_id);

        char season_fmt[30];

        if (season_id == 1) {
            snprintf(season_fmt, 30, "%s (%s)", _("Spring"), _("Sanguine"));
        }
        else if (season_id == 2) {
            snprintf(season_fmt, 30, "%s (%s)", _("Summer"), _("Choleric"));
        }
        else if (season_id == 3) {
            snprintf(season_fmt, 30, "%s (%s)", _("Autumn"), _("Melancholic"));
        }
        else if (season_id == 4) {
            snprintf(season_fmt, 30, "%s (%s)", _("Winter"), _("Phlegmatic"));
        }

                      
        // Moon's temperament
        
        char moon_temperament[20];
        //get_moon_temperament_by_phase_id(moon_phase_id, &moon_temperament);

        switch(moon_phase_id) {
            case 1:
            case 2:
                snprintf(moon_temperament, 20, "%s", _("Sanguine"));
                break;
            case 3:
            case 4:
                snprintf(moon_temperament, 20, "%s", _("Choleric"));
                break;
            case 5:
            case 6:
                snprintf(moon_temperament, 20, "%s", _("Melancholic"));
                break;
            case 7:
            case 8:
                snprintf(moon_temperament, 20, "%s", _("Phlegmatic"));
                break;
        }



        // Strength of the Planets

        int strength_planets[NUM_OBJECTS - object_diff];
        calcular_forca_planetas(dig, strength_planets, show_modern_planets);

        

        // Cusps

        char pHouse[12][100];

        for (int i = 0; i < 12; i++) {
            pHouse[i][0] = '\0'; 
        }

        for (int i = 0; i < 12; i++) {
            for (int j = 0; j < NUM_OBJECTS - object_diff; j++) {
                if (get_house(plots[j].longitude, cusps) == i + 1) {
                    strncat(pHouse[i], plots[j].object, 100 - strlen(pHouse[i]) - 1);
                    strncat(pHouse[i], " ", 100 - strlen(pHouse[i]) - 1);
                }

            }
        }


        char **house_ruler_str = (char **)malloc(13 * sizeof(char *));
        int house_rulers[13] = {0};

        for (int i = 1; i <= 12; i++) {

            // Motivação Primária

            int sign = (int)(cusps[i] / 30);
            int n_ruler = obter_regente_tradicional(sign + 1);
            house_rulers[i] = n_ruler;

            
            // Houses
            
            house_ruler_str[i] = (char *)malloc(10 * sizeof(char));
    
            char sign_str[10];   
            snprintf(sign_str, sizeof(sign_str), "%s", get_sign(sign));
    
            char *house_ruler;
            get_sign_ruler_by_domicile(sign_str, &house_ruler);        
            
            char *house_ruler2;
            get_sign_ruler_by_exaltation(sign_str, &house_ruler2, consider_modern_planets_rulling);
            
            snprintf(house_ruler_str[i], 10, "%s %s", house_ruler, house_ruler2);
    
            free(house_ruler);
            free(house_ruler2);
        }
        char house_system_str[50];
        char *house_system_name;    
        get_house_system_name(house_system, &house_system_name);

        snprintf(house_system_str, 50, "%c - %s", house_system, house_system_name);
        free(house_system_name);

        
                

        

        // terminate_database();
        // endwin();
        // for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
        //     fprintf(stderr, "%d %d %d\n", dig[i].id, dig[i].essential, dig[i].accidental);
        // }        
        // exit(1);
        
        
        PontosHylegiacos pontos_calculados = {
            plots[P_SOL].longitude,
            plots[P_LUNA].longitude,
            plots[P_ASC - object_diff].longitude,
            plots[P_FORTUNA - object_diff].longitude,
            plots[P_SAN - object_diff].longitude
        };
        

        int id_almuten_ref = 0;
        ResultadoAlcochoden alco = {"None", "", 0, "None", 0};
        int regente_dia = converter_codigo_planeta(get_hour_regent(week_day, (MAPA_DIURNO)?0:12));
        int regente_hora = converter_codigo_planeta(get_hour_regent(week_day, planetary_hour));
        
        // 1. Antes do laço do teclado começar, calcule os dados base de expectativa vital:
        int tipo_h = -1;
        int idx_objeto_h = -1;

        if (!mapa_retorno) {
            tipo_h = get_hyleg(pontos_calculados, plots, &matrix, &id_almuten_ref, regente_dia, regente_hora, tipo_san, dig);
                    
            if (tipo_h == H_SOL) idx_objeto_h = 0;
            else if (tipo_h == H_LUNA) idx_objeto_h = 1;
            else if (tipo_h == H_SAN) idx_objeto_h = P_SAN - object_diff;
            else if (tipo_h == H_ALMUTEN) idx_objeto_h = id_almuten_ref - 1;
            else if (tipo_h == H_ALMUTEN_HYL) idx_objeto_h = id_almuten_ref - 1;
            else {
                for (int i = 0; i < NUM_OBJECTS - object_diff; i++) {
                    if (tipo_h == H_ASC && plots[i].id == P_ASC - object_diff) { idx_objeto_h = i; break; }
                    if (tipo_h == H_FORTUNA && plots[i].id == P_FORTUNA - object_diff) { idx_objeto_h = i; break; }
                }
            }
        }
        else {
            tipo_h = tipo_h_natal;
            idx_objeto_h = idx_hyleg_natal;
        }

        


        
        // Executa o cálculo e guarda na variável 'alco'        
        alco = calcular_alcochoden(tipo_h, idx_objeto_h + 1, &matrix, plots, dig, regente_dia, regente_hora, pontos_calculados);

              
        int signo_da_casa_8 = (int)(cusps[8] / 30) + 1;

        
        // para direções

        int id_senhor_da_casa8 = 0;
        get_ruler_dom_by_sign_id(signo_da_casa_8, &id_senhor_da_casa8);
        ResultadoAnareta anar = calcular_anareta(idx_objeto_h, &matrix, plots, dig, signo_da_casa_8);

        char *nome_senhor_da_casa8 = (mapa_retorno) ? nome_s8_natal : (char *)obter_nome_planeta_por_id(id_senhor_da_casa8);
        char nome_anareta[20];
        snprintf(nome_anareta, 20, (mapa_retorno) ? nome_anareta_natal : anar.name);





        // Constrói o ChartObject
        ChartObject obj[100] = {0};

        int total_objects = get_chart_objects(obj);

        for (int i = 0; i < total_objects; i++) {
            for (int j = 0; j < NUM_OBJECTS; j++) {
                if (obj[i].id == plotsA[j].id + 1 && obj[i].longitude == 0.0) {
                    obj[i].longitude = plotsA[j].longitude;
                    obj[i].house = get_house(plotsA[j].longitude, cusps);
                    break;
                }
            }            
        }
        for (int i = 0; i < total_objects; i++) {
            if (obj[i].type == OBJ_DISPOSITOR) {
                continue;
            }
            if (obj[i].type == OBJ_ZODIAC_DEGREE) {
                obj[i].house = get_house(obj[i].longitude, cusps);
            }
            if (obj[i].type == OBJ_CUSP) {
                for (int j = 1; j <= 12; j++) {
                    obj[i + j - 1].longitude = cusps[j];
                    obj[i + j - 1].house = j;
                }
            }
            if (strcmp(obj[i].object, "SPI") == 0) {
                obj[i].longitude = spirit;
                obj[i].house = get_house(spirit, cusps);
            }
        }
                
        for (int i = 0; i < total_objects; i++) {
            if (obj[i].type == OBJ_DISPOSITOR) {
                int ref = obj[i].object_ref;

                for (int j = 0; j < total_objects; j++) {
                    if (obj[j].id == ref) {
                        int sign = (int)(obj[j].longitude / 30.0) + 1;
                        int ruler = 0;

                        get_ruler_dom_by_sign_id(sign, &ruler);

                        for (int k = 0; k < 7; k++) {
                            if (obj[k].id == ruler) {
                                obj[i].longitude = obj[k].longitude;
                                obj[i].house = get_house(obj[k].longitude, cusps);
                                break;
                            }
                        }
                        break;
                    }
                }
            
            }
            
        }
        // DEBUG
        // bool zica = false;
        // for (int i = 0; i < total_objects; i++) {
        //     if (obj[i].longitude == 0.0) {
        //         zica = true;
        //         fprintf(stderr, "%d %f\n", obj[i].id, obj[i].longitude);
        //     }
        // }
        // if (zica) {
        //     terminate_database();
        //     endwin();

        //     for (int i = 0; i < total_objects; i++) {
        //         fprintf(stderr, "%d %f\n", obj[i].id, obj[i].longitude);
        //     }
        //     exit(1);
        // }
        
        // DEBUG
        // terminate_database();
        // endwin();
        // for (int i = 0; i < total_objects; i++) {
        //     fprintf(stderr, "%d %f\n", obj[i].id, obj[i].longitude);
        // }        
        // exit(1);

        // ArabicPartCalculada parts[MAX_PARTS];
        // memset(parts, 0, sizeof(parts));

        // int qtd_partes = load_and_calculate_arabic_parts(obj, total_objects, cusps, lista);



        DadosPlanetaMente mercurio = {3, dig[P_MERCURY].essential, dig[P_MERCURY].accidental, dig[P_MERCURY].row.combust};
        DadosPlanetaMente lua = {1, dig[P_LUNA].essential, dig[P_LUNA].accidental, dig[P_LUNA].row.combust};


        // Almuten da revolução
        int almuten_rev[12] = {0};
        int qtd_almuten_rev = 0;
        double almuten_lon = 0.0;
        int dig_almuten_natal = 0;
        double almuten_lat = 0.0;
        
        if (mapa_retorno) {
            qtd_almuten_rev = calcular_almuten_figuris(pontos_calculados, plots, &matrix, regente_dia, regente_hora, almuten_rev);
            if (qtd_almuten_rev > 0) {
                almuten_lon = plots[almuten_rev[0] - 1].longitude;
                almuten_lat = planet_latitudes[almuten_rev[0] - 1];
                dig_almuten_natal = dig_natal[almuten_rev[0] - 1].essential + dig_natal[almuten_rev[0] - 1].accidental;
            }
        }
        

        // criar Promissor objects array com as coordenadas dos 7 planetas, antissia, contrantissia e termos

        Promissor prom[100] = {0};

        for (int i = 0; i < 7; i++) {
            snprintf(prom[i].object, 10, "%s", plots[i].object);
            snprintf(prom[i].object_name, 30, "%s", plots[i].object_name);
            prom[i].id = plots[i].id;
            prom[i].longitude = plots[i].longitude;
            prom[i].latitude = planet_latitudes[i];
            prom[i].declination = plots[i].declination;
            prom[i].house = get_house(plots[i].longitude, cusps);
            prom[i].type = PROM_PLANET;
        }

        double longitudes_termos[60] = {0};
        double ra_termos[60] = {0};
        double decl_termos[60] = {0};

        char termos_regentes[60][10];
        
        int index = 0;
        for (int i = 0; i < 12; i++) {
            for (int j = 0; j < 5; j++, index++) {
                if (terms_system == 1) {
                    longitudes_termos[index] = get_longitude_term(i, j, tabela_termos_egipcios);
                    snprintf(termos_regentes[index], 10, "%s%s", get_sign(i), planet_regent_symbols[tabela_termos_egipcios[i][j].regente]);
                    snprintf(prom[index+7].object_name, 30, "%s", planet_regent_names[tabela_termos_egipcios[i][j].regente]);
                } 
                else {
                    longitudes_termos[index] = get_longitude_term(i, j, tabela_termos_ptolomeu);
                    snprintf(termos_regentes[index], 10, "%s%s", get_sign(i), planet_regent_symbols[tabela_termos_ptolomeu[i][j].regente]);
                    snprintf(prom[index+7].object_name, 30, "%s", planet_regent_names[tabela_termos_ptolomeu[i][j].regente]);          
                }
                snprintf(prom[index+7].object, 10, "%s", termos_regentes[index]);                
            }
        }

        for (int i = 0; i < 60; i++) {
            obter_coordenadas_termo(longitudes_termos[i], julian_day, &ra_termos[i], &decl_termos[i]);
            prom[i+7].id = i + 7;
            prom[i+7].longitude = longitudes_termos[i];
            prom[i+7].latitude = 0.0;
            prom[i+7].declination = decl_termos[i];
            prom[i+7].house = get_house(longitudes_termos[i], cusps);
            prom[i+7].type = PROM_TERM;
        }

        
        AntObject ants[14] = {0};
        
        
        double longitudes_ant[7] = {0};
        int sign_ant[7] = {0};

        for (int i = 0; i < 7; i++) {
            snprintf(prom[i+67].object, 10, "A%s", plots[i].object);
            snprintf(prom[i+67].object_name, 30, "%s", plots[i].object_name);
            prom[i+67].id = plots[i].id + 67;

            longitudes_ant[i] = get_antiscium_degree(fmod(plots[i].longitude, 30));
            sign_ant[i] = get_sign_antiscium((int)(plots[i].longitude / 30));

            prom[i+67].longitude = sign_ant[i] * 30.0 + longitudes_ant[i];
            
            prom[i+67].latitude = planet_latitudes[i];
            prom[i+67].declination = plots[i].declination;
            prom[i+67].house = get_house(prom[i+67].longitude, cusps);
            prom[i+67].type = PROM_ANTISCIUM;

            ants[i].id = i;
            ants[i].longitude = prom[i+67].longitude;
            ants[i].latitude = prom[i+67].latitude;
            ants[i].declination = prom[i+67].declination;
            ants[i].house = prom[i+67].house;
            snprintf(ants[i].object, 10, "A%s", plots[i].object);
            snprintf(ants[i].object_name, 30, "Antiscium %s", plots[i].object_name);
        }


        double longitudes_cant[7] = {0};
        int sign_cant[7] = {0};

        for (int i = 0; i < 7; i++) {
            snprintf(prom[i+74].object, 10, "CA%s", plots[i].object);
            snprintf(prom[i+74].object_name, 30, "%s", plots[i].object_name);
            prom[i+74].id = plots[i].id + 74;

            longitudes_cant[i] = longitudes_ant[i];
            sign_cant[i] = get_opposite_sign(sign_ant[i]);

            prom[i+74].longitude = sign_cant[i] * 30.0 + longitudes_cant[i];
            
            prom[i+74].latitude = -planet_latitudes[i];
            prom[i+74].declination = -plots[i].declination;
            prom[i+74].house = get_house(prom[i+74].longitude, cusps);
            prom[i+74].type = PROM_CONTRANTISCIUM;

            ants[i+7].id = i;
            ants[i+7].longitude = prom[i+74].longitude;
            ants[i+7].latitude = prom[i+74].latitude;
            ants[i+7].declination = prom[i+74].declination;
            ants[i+7].house = prom[i+74].house;
            snprintf(ants[i+7].object, 10, "CA%s", plots[i].object);
            snprintf(ants[i+7].object_name, 30, "Contrantiscium %s", plots[i].object_name);
        }



        
        // Aries offset to draw the chart
        n = mod_a(13 - ((int)ascendant / 30), 12);
             
        
        // Draw the chart
        draw_chart(zoom_factor, pan_x, pan_y, n - 1, local_time, lat, lon, elev, tz_offset, plots, cusps,
                   sanYear, sanMon, sanDay, sanHour, sunrise_time, sunset_time, city, country, 
                   daytime_hour, nighttime_hour, week_day + 1, planetary_hour + 1, phase, 
                   dark_mode, animated, anim_interval, mapa_retorno, chart_name, house_system, gender_id, 
                   house_div, last_hr, last_min, last_sec, show_dec, 
                   (terms_system == 1) ? tabela_termos_egipcios : tabela_termos_ptolomeu, show_terms);
        
        bool saiu_retorno = false;

        // Handle input with timeout
        
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);

        ch = getch();
        
        switch (ch) {
            case '+':
            case '=':
                zoom_factor += 0.1;
                if (zoom_factor > 4.0) zoom_factor = 4.0; // Cap at 4x zoom
                break;
            case '-':
            case '_':
                zoom_factor -= 0.1;
                if (zoom_factor < 0.3) zoom_factor = 0.3; // Cap at 0.3x zoom
                break;
            case 'q':
            case 'Q':
                running = false;
                break;
            case 'r':
            case 'R':
                // Reset panning and zoom
                pan_x = 0.0;
                pan_y = 0.0;
                zoom_factor = 1.0;
                break;
            case KEY_UP:
                pan_y -= 2.0; // Move up
                if (pan_y < -max_y) pan_y = -max_y;
                break;
            case KEY_DOWN:
                pan_y += 2.0; // Move down
                if (pan_y > max_y) pan_y = max_y;
                break;
            case KEY_LEFT:
                pan_x -= 2.0; // Move left
                if (pan_x < -max_x) pan_x = -max_x;
                break;
            case KEY_RIGHT:
                pan_x += 2.0; // Move right
                if (pan_x > max_x) pan_x = max_x;
                break;
            case '[':
                anim_interval -= 60;
                break;
            case ']':
                anim_interval += 60;
                break;
            case '{':
                anim_interval -= 1;
                break;
            case '}':
                anim_interval += 1;
                break;
 
            case 'a':
            case 'A':
            case ' ':
                animated = !animated;
                break;
            case 'b':
            case 'B':
                    show_terms = !show_terms;
                    if (show_terms) {
                        show_dec = false;
                    }
                    break;
            case 'c':
            case 'C':
                if (mapa_retorno) {                        
                    processar_confronto_natal_revolucao(
                        almuten_rev[0],
                        almuten_lon,
                        almuten_lat,
                        dig_almuten_natal,
                        lat_natal,
                        armc_natal,
                        senhor_da_profeccao,
                        id_senhor_firdaria,
                        id_senhor_subfirdaria,
                        ascendant,
                        strength_natal,
                        jd_natal,
                        julian_day,
                        armc,
                        lat,
                        asc_natal
                    );
                }
                break;
            case 'd':
            case 'D':
                    show_dec = !show_dec;
                    if (show_dec) {
                        show_terms = false;
                    }
                    break;
            case 'h':
            case 'H':
                    house_div = !house_div;
                    break;
            case 'm':
            case 'M':
                ContextoMenu ctx;

                ctx.julian_day = julian_day;
                ctx.local_time = local_time;
                ctx.lat = lat;
                ctx.lon = lon;
                ctx.elev = elev;
                ctx.plots = plots;
                ctx.season_fmt = season_fmt;
                ctx.sanYear = sanYear;
                ctx.sanMon = sanMon;
                ctx.sanDay = sanDay;
                ctx.sanHour = sanHour;
                ctx.sunrise_time = sunrise_time;
                ctx.sunset_time = sunset_time;
                ctx.next_sunrise_time = next_sunrise_time;
                ctx.city = city;
                ctx.country = country;
                ctx.phase = phase;
                ctx.moon_temperament = moon_temperament;
                ctx.dark_mode = dark_mode;
                ctx.last_hr = last_hr;
                ctx.last_min = last_min;
                ctx.last_sec = last_sec;
                ctx.chart_name = chart_name;
                ctx.gender_id = gender_id;
                
                ctx.planet_matrix = planet_matrix;
                ctx.dig = dig;
                
                ctx.matrix = matrix;
                ctx.matrix_decl = matrix_decl;
                
                ctx.week_day = week_day;
                ctx.hours = hours;
                ctx.planetary_hour = planetary_hour;
                ctx.daytime_hour = daytime_hour;
                ctx.nighttime_hour = nighttime_hour;

                ctx.cusps = cusps;
                ctx.pHouse = (char **)pHouse;
                ctx.house_ruler_str = (char **)house_ruler_str;
                ctx.house_system_str = house_system_str;

                ctx.pontos_calculados = pontos_calculados;

                ctx.phase_id = phase_id;
                ctx.season_id = season_id;
                
                ctx.anos_alcochoden = alco.anos_concedidos;

                ctx.signo_da_casa_8 = signo_da_casa_8;
                ctx.regente_dia = regente_dia;
                ctx.regente_hora = regente_hora;

                ctx.obj = obj;
                ctx.total_objects = total_objects;

                ctx.animated = animated;
                ctx.anim_interval = anim_interval;

                ctx.mercurio = mercurio;
                ctx.lua = lua;
                ctx.mercury_retro = retro[P_MERCURY];

                ctx.zoom_factor = zoom_factor;
                ctx.pan_x = pan_x;
                ctx.pan_y = pan_y;
                ctx.n = n;
                ctx.tz_offset = tz_offset;
                ctx.house_div = house_div;
                ctx.house_system = house_system;

                ctx.mapa_retorno = mapa_retorno;
                ctx.qtd_almuten_rev = qtd_almuten_rev;
                ctx.almuten_rev = almuten_rev;

                ctx.almuten_lon = almuten_lon;
                ctx.almuten_lat = almuten_lat;
                ctx.armc = armc;
                ctx.dig_almuten_natal = dig_almuten_natal;
                ctx.ascendant = ascendant;
                ctx.lat_natal = lat_natal;
                ctx.senhor_da_profeccao = senhor_da_profeccao;
                ctx.id_senhor_firdaria = id_senhor_firdaria;
                ctx.id_senhor_subfirdaria = id_senhor_subfirdaria;
                ctx.armc_natal = armc_natal;

                ctx.nome_anareta = nome_anareta;
                ctx.nome_senhor_da_casa8 = nome_senhor_da_casa8;
                ctx.tipo_h = tipo_h;
                ctx.idx_objeto_h = idx_objeto_h;
                ctx.planet_longitudes = planet_longitudes;
                ctx.jd_natal = jd_natal;
                ctx.planet_latitudes = planet_latitudes;

                ctx.strength_planets = strength_planets;

                ctx.strength_natal = strength_natal;

                ctx.longitudes_natal = longitudes_natal;
                ctx.tipo_h_natal = tipo_h_natal;
                ctx.idx_hyleg_natal = idx_hyleg_natal;

                ctx.asc_natal = asc_natal;

                ctx.cusps_natal = cusps_natal;

                ctx.tipo_san = tipo_san;

                ctx.prom = prom;

                ctx.ants = ants;

                ctx.house_rulers = house_rulers;
            
                open_menu_tables(&ctx);

                touchwin(stdscr);
                refresh();

                break;
            case 'p':
            case 'P':
                if (mapa_retorno) {
                    display_arabic_parts_solar_natal_confrontation(obj, cusps, total_objects, cusps_natal);
                }
                break;                
            case 't':
            case 'T':
                if (mapa_retorno) {
                    process_revolution_transits(jd_natal, planet_longitudes, planet_latitudes, armc_natal, lat_natal, house_system, tipo_h_natal, idx_hyleg_natal, longitudes_natal);
                }
                break;
            case KEY_F(1):
                display_table_data(
                    mapa_retorno, julian_day, local_time, lat, lon, elev, plots, season_fmt,
                    sanYear, sanMon, sanDay, sanHour, sunrise_time, sunset_time, next_sunrise_time, city, country, 
                    phase, moon_temperament, last_hr, last_min, last_sec, chart_name, gender_id
                );
                
                break;
            case KEY_F(2):
                display_table(plots, &planet_matrix, dig, strength_planets);
                break;    
            case 'x':
            case KEY_F(3):
                display_aspects(plots, &matrix, &matrix_decl, ants, 14);
                break;
            case KEY_F(4):
                display_hours(week_day + 1, hours, planetary_hour + 1, daytime_hour, nighttime_hour, strength_planets, dig);
                break;
            case KEY_F(5):
                display_rising_times(plots, tz_offset);
                break;
            case KEY_F(6):
                display_houses(cusps, pHouse, house_ruler_str, house_system_str);
                break;
            case KEY_F(7):
                display_almutens(pontos_calculados, plots, &matrix, week_day + 1, planetary_hour + 1, mapa_retorno);
                break;
            case KEY_F(8):
                if (!mapa_retorno) {
                    display_temperament(plots, &matrix, phase_id, season_id, week_day + 1, planetary_hour + 1);
                }
                break;
            case KEY_F(9):
                if (!mapa_retorno) {
                    display_profections(plots, alco.anos_concedidos);
                }
                break;
            case KEY_F(12):
                if (!mapa_retorno) {
                    display_firdaria(plots, &matrix, dig, pontos_calculados, signo_da_casa_8, regente_dia, regente_hora, tipo_san);
                }                
                break;
            case '1':
            case KEY_F(13):
                if (!mapa_retorno) {
                    display_life_givers(pontos_calculados, dig, plots, &matrix, week_day + 1, planetary_hour + 1, tipo_san);
                }
                break;            
            case '2':
            case KEY_F(14):
                if (!mapa_retorno) {
                    display_anareta(plots, &matrix, dig, pontos_calculados, signo_da_casa_8, week_day + 1, planetary_hour + 1, tipo_san);
                }
                break;
            case '3':
            case KEY_F(15):
                if (!mapa_retorno) {
                    display_primary_directions(plots, &matrix, pontos_calculados, regente_dia, regente_hora, nome_anareta, nome_senhor_da_casa8, tipo_h, idx_objeto_h, mapa_retorno, julian_day, planet_latitudes, tipo_san, dig, armc, lat, prom);
                }
                break;
            case '4':
            case KEY_F(16):
                if (!mapa_retorno) {
                    display_primary_directions_parts(prom, nome_anareta, nome_senhor_da_casa8, obj, total_objects, cusps, julian_day, planet_latitudes, armc, lat);
                }
                break;
            case '5':
            case KEY_F(17):
                display_arabic_parts(obj, cusps, total_objects);
                break;
            case '6':                   
            case KEY_F(18):
                if (!mapa_retorno) {
                    display_natal_mind_analysis(
                        mercurio,
                        lua,
                        retro[P_MERCURY],
                        phase_id,       // 1=Nova, 2=Crescente, 3=Cheia, 4=Minguante
                        &matrix,
                        pontos_calculados,
                        plots
                    );
                }
                break;
            case '7':
            case KEY_F(19):
                if (!mapa_retorno) {
                    disparar_revolucao_solar(julian_day, CHART_NAME, cusps, MAPA_DIURNO, lat, armc, dig, nome_anareta, nome_senhor_da_casa8, tipo_h, idx_objeto_h, planet_longitudes, strength_planets);
                    saiu_retorno = true;
                }                                
                break; 
            case '8':
            case KEY_F(20):
                display_planetary_energy_profile(plots, strength_planets);
                break;
            case '0':
                if (!mapa_retorno) {
                    display_motivation(plots, house_rulers);
                }
                break;
            case 27:
                running = false;
                break;
            case ERR: // No key pressed within timeout (1 second)
                // Do nothing - just continue the loop
                break;
        }
        
        if (saiu_retorno) {
            *local_time = julian_day_para_struct_tm(julian_day);

            local_time->tm_hour += tz_offset;
            timegm(local_time);
        }

        if (animated) {
            local_time->tm_sec += anim_interval;
            timegm(local_time);
        }

        
        
        for (int k = 1; k < 13; k++) {
            free(house_ruler_str[k]);
        }
        free(house_ruler_str);
        //free(moon_temperament);

    }

    MAPA_DIURNO = backup_mapa_diurno;

    swe_close();
    endwin();

    return 0;
}



void open_menu_tables(ContextoMenu *ctx) {
    // 1. Definição estática das diversas opções de funções
    const char *opcoes1[] = {
        _("01. Chart Data Panel"),
        _("02. Positions, Dignities & Rulerships"),
        _("03. Accidental Dignities"),
        _("04. Strength of the Planets"),
        _("05. Aspects"),
        _("06. Declination Aspects"),
        _("07. Planetary Hours"),
        _("08. Rising, Setting & Culmination Times"),
        _("09. Houses"),
        _("10. Hylegiacal & Figuris Almuten"),
        _("11. Temperament Analysis"),
        _("12. Annual Profections"),
        _("13. Firdaria"),
        _("14. Vital Chronocrators: Life Givers"),
        _("15. Vital Threats: The Anareta"),
        _("16. Primary Directions"),
        _("17. Prymary Directions to Arabic Parts"),
        _("18. Arabic Parts"),
        _("19. Mind Analysis"),
        _("20. Solar Revolution"),
        _("21. Planetary Energy Profile"),
        _("22. Primary Motivation"),
        _("23. Aspects by Sign"),
        _("24. Aspects to Antissia & Contrantissia")
        
    };
    // Calcula automaticamente o total de opções adicionadas ao array
    int total_opcoes1 = sizeof(opcoes1) / sizeof(opcoes1[0]);

    const char *opcoes2[] = {
        _("01. Chart Data Panel"),
        _("02. Positions, Dignities & Rulerships"),
        _("03. Accidental Dignities"),
        _("04. Strength of the Planets"),
        _("05. Aspects"),
        _("06. Declination Aspects"),
        _("07. Planetary Hours"),
        _("08. Rising, Setting & Culmination Times"),
        _("09. Houses"),
        _("10. Hylegiacal & Figuris Almuten"),
        _("11. Radix Confrontation: Solar Return Integration"),
        _("12. Annual Transits & Radical Projections"),
        _("13. Arabic Parts"),
        _("14. Planetary Energy Profile"),
        _("15. Arabic Parts Solar Return Radix Confrontation"),
        _("16. Aspects by Sign"),
        _("17. Aspects to Antissia & Contrantissia")
    };
    // Calcula automaticamente o total de opções adicionadas ao array
    int total_opcoes2 = sizeof(opcoes2) / sizeof(opcoes2[0]);

    const char **opcoes;
    int total_opcoes = 0;

    if (ctx->mapa_retorno) {
        opcoes = (const char **)opcoes2;
        total_opcoes = total_opcoes2;
    }
    else {
        opcoes = (const char **)opcoes1;
        total_opcoes = total_opcoes1;
    }

    int term_w, term_h;
    getmaxyx(stdscr, term_h, term_w);
    
    int menu_width = 54;
    int menu_height = 15;
    int menu_start_x = (term_w - menu_width) / 2;
    int menu_start_y = (term_h - menu_height) / 2;
    
    WINDOW *win = newwin(menu_height, menu_width, menu_start_y, menu_start_x);
    WINDOW *shadow = newwin(menu_height, menu_width, menu_start_y + 1, menu_start_x + 1);
    
    // Opcional: Se quiser que o gráfico atrás atualize, mude para o tempo desejado
    wtimeout(win, 1000); 
    keypad(win, TRUE);
    curs_set(0);
    
    // Variables for scrolling and selection
    int selected_index = 0;  
    int scroll_offset = 0;   
    int max_display_items = menu_height - 2;  
    int menu_selected = 0;
    int key;

    // Clear and draw shadow
    werase(shadow);
    wattron(shadow, COLOR_PAIR(24));
    box(shadow, 0, 0);
    wattroff(shadow, COLOR_PAIR(24));
    wrefresh(shadow);
      
    bool saiu_retorno = false;

    while (!menu_selected) {
        
        // Clear and redraw main menu
        werase(win);
        wattron(win, COLOR_PAIR(26) | A_DIM);
        box(win, 0, 0);
        
        wattron(win, A_BOLD);
        mvwprintw(win, 0, (menu_width - 17) / 2, " Select a Module ");
        wattroff(win, A_BOLD);

        wbkgd(win, COLOR_PAIR(26));
        wattroff(win, COLOR_PAIR(26) | A_DIM);
        
        // Draw options items with proper scrolling (idêntico ao seu loop do chart)
        for (int i = 0; i < max_display_items; i++) {
            int item_index = i + scroll_offset;
            if (item_index < total_opcoes) {
                int attr = (item_index == selected_index) ? (COLOR_PAIR(23) | A_REVERSE | A_BOLD) : COLOR_PAIR(26);
                wattron(win, attr);
                mvwprintw(win, i + 1, 1, " %s%*s ", opcoes[item_index], 50 - get_visual_width(opcoes[item_index]), " ");
                wattroff(win, attr);
            }
        }
        wrefresh(win);        
        
        key = wgetch(win);
        
        // Se houver timeout do loop principal do gráfico ao fundo (ch == ERR)
        if (key == ERR) {
            // Chame aqui a função que atualiza seu gráfico em tempo real, se necessário:
            // atualizar_chart(ctx->janela_chart);
            
            continue; 
        }
        
        // Handle letter jumping (Mudado o offset para 0 para ler do início da string)
        if (isalpha(key)) {
            int new_index = find_first_item_with_letter_offset((const char**)opcoes, total_opcoes, selected_index, key, 4);
            if (new_index != selected_index) {
                selected_index = new_index;
                if (selected_index < scroll_offset) {
                    scroll_offset = selected_index;
                } else if (selected_index >= scroll_offset + max_display_items) {
                    scroll_offset = selected_index - max_display_items + 1;
                }
                continue; 
            }
        }
        
        switch(key) {
            case KEY_UP:
                if (selected_index > 0) {
                    selected_index--;
                    if (selected_index < scroll_offset) {
                        scroll_offset = selected_index;
                    }
                }
                break;
            case KEY_DOWN:
                if (selected_index < total_opcoes - 1) {
                    selected_index++;
                    if (selected_index >= scroll_offset + max_display_items) {
                        scroll_offset = selected_index - max_display_items + 1;
                    }
                }
                break;
            case 10: // Enter
                menu_selected = 1;
                break;
            case 'q':
            case 'Q':
            case 27: // ESC
                delwin(win);
                delwin(shadow);
                return; // Aborta o menu limpo sem executar nada
        }
    }

    switch(selected_index) {
        case 0:
            display_table_data(
                ctx->mapa_retorno, ctx->julian_day, ctx->local_time, ctx->lat, ctx->lon, ctx->elev, ctx->plots, ctx->season_fmt,
                ctx->sanYear, ctx->sanMon, ctx->sanDay, ctx->sanHour, ctx->sunrise_time, ctx->sunset_time, ctx->next_sunrise_time, ctx->city, ctx->country, 
                ctx->phase, ctx->moon_temperament, ctx->last_hr, ctx->last_min, ctx->last_sec, ctx->chart_name, ctx->gender_id
            );
            break;
        case 1:
            display_table(ctx->plots, &ctx->planet_matrix, ctx->dig, ctx->strength_planets);
            break;
        case 2:
            display_dignities(ctx->plots, ctx->dig, ctx->strength_planets);
            break;
        case 3:
            display_force(ctx->plots, ctx->dig, ctx->strength_planets);
            break;
        case 4:
            display_aspects(ctx->plots, &ctx->matrix, &ctx->matrix_decl, ctx->ants, 14);
            break;
        case 5:
            display_declination_aspects(ctx->plots, &ctx->matrix_decl);
            break;
        case 6:
            display_hours(ctx->week_day + 1, ctx->hours, ctx->planetary_hour + 1, ctx->daytime_hour, ctx->nighttime_hour, ctx->strength_planets, ctx->dig);
            break;
        case 7:
            display_rising_times(ctx->plots, ctx->tz_offset);
            break;
        case 8:
            display_houses(ctx->cusps, (char (*)[100])ctx->pHouse, ctx->house_ruler_str, ctx->house_system_str);
            break;
        case 9:
            display_almutens(ctx->pontos_calculados, ctx->plots, &ctx->matrix, ctx->week_day + 1, ctx->planetary_hour + 1, ctx->mapa_retorno);
            break;
        case 10:
            if (!ctx->mapa_retorno) {
                display_temperament(ctx->plots, &ctx->matrix, ctx->phase_id, ctx->season_id, ctx->week_day + 1, ctx->planetary_hour + 1);
            }
            else {
                processar_confronto_natal_revolucao(
                    ctx->almuten_rev[0],
                    ctx->almuten_lon,
                    ctx->almuten_lat,
                    ctx->dig_almuten_natal,
                    ctx->lat_natal,
                    ctx->armc_natal,
                    ctx->senhor_da_profeccao,
                    ctx->id_senhor_firdaria,
                    ctx->id_senhor_subfirdaria,
                    ctx->ascendant,
                    ctx->strength_natal,
                    ctx->jd_natal,
                    ctx->julian_day,
                    ctx->armc,
                    ctx->lat,
                    ctx->asc_natal
                );
            }
            break;
        case 11:
            if (!ctx->mapa_retorno) {
                display_profections(ctx->plots, ctx->anos_alcochoden);
            }
            
            else {
                process_revolution_transits(ctx->jd_natal, ctx->planet_longitudes, ctx->planet_latitudes, ctx->armc_natal, ctx->lat_natal, ctx->house_system, ctx->tipo_h_natal, ctx->idx_hyleg_natal, ctx->longitudes_natal);
            }    
            break;
        case 12:
            if (!ctx->mapa_retorno) {
                display_firdaria(ctx->plots, &ctx->matrix, ctx->dig, ctx->pontos_calculados, ctx->signo_da_casa_8, ctx->regente_dia, ctx->regente_hora, ctx->tipo_san);
            }
            else {
                display_arabic_parts(ctx->obj, ctx->cusps, ctx->total_objects);
            }
            break;
        case 13:
            if (!ctx->mapa_retorno) {
                display_life_givers(ctx->pontos_calculados, ctx->dig, ctx->plots, &ctx->matrix, ctx->week_day + 1, ctx->planetary_hour + 1, ctx->tipo_san);
            }
            else {
                display_planetary_energy_profile(ctx->plots, ctx->strength_planets);
            }
            break;            
        case 14:
            if (!ctx->mapa_retorno) {
                display_anareta(ctx->plots, &ctx->matrix, ctx->dig, ctx->pontos_calculados, ctx->signo_da_casa_8, ctx->week_day + 1, ctx->planetary_hour + 1, ctx->tipo_san);
            }
            else {
                display_arabic_parts_solar_natal_confrontation(ctx->obj, ctx->cusps, ctx->total_objects, ctx->cusps_natal);
            }
            break;
        case 15:
            if (!ctx->mapa_retorno) {
                display_primary_directions(ctx->plots, &ctx->matrix, ctx->pontos_calculados, ctx->regente_dia, ctx->regente_hora, ctx->nome_anareta, ctx->nome_senhor_da_casa8, ctx->tipo_h, ctx->idx_objeto_h, ctx->mapa_retorno, ctx->julian_day, ctx->planet_latitudes, ctx->tipo_san, ctx->dig, ctx->armc, ctx->lat, ctx->prom);
            }
            else {
                AspectMatrix matrix_sign = {0};
                matrix_sign = calculate_aspects_by_sign(ctx->plots);
                display_aspects_by_sign(ctx->plots, &matrix_sign);
                break;
            }
            break;
        case 16:
            if (!ctx->mapa_retorno) {
                display_primary_directions_parts(ctx->prom, ctx->nome_anareta, ctx->nome_senhor_da_casa8, ctx->obj, ctx->total_objects, ctx->cusps, ctx->julian_day, ctx->planet_latitudes, ctx->armc, ctx->lat);
            }
            else {
                AspectMatrix matrix_ants = {0}; 
                matrix_ants = calculate_aspects_antiscium(ctx->plots, ctx->ants, 14);
                display_aspects_antissium(ctx->plots, ctx->ants, 14, &matrix_ants);  
            }           
            break;
        case 17:
            display_arabic_parts(ctx->obj, ctx->cusps, ctx->total_objects);
            break;
        case 18:
            display_natal_mind_analysis(ctx->mercurio, ctx->lua, ctx->mercury_retro, ctx->phase_id, &ctx->matrix, ctx->pontos_calculados, ctx->plots);
            break;
        case 19:
            if (!ctx->mapa_retorno) {    
                disparar_revolucao_solar(ctx->julian_day, ctx->chart_name, ctx->cusps, MAPA_DIURNO, ctx->lat, ctx->armc, ctx->dig, ctx->nome_anareta, ctx->nome_senhor_da_casa8, ctx->tipo_h, ctx->idx_objeto_h, ctx->planet_longitudes, ctx->strength_planets);
                saiu_retorno = true;
            }
            break;
        case 20:
            display_planetary_energy_profile(ctx->plots, ctx->strength_planets);
            break;
        case 21:
            if (!ctx->mapa_retorno) {    
                display_motivation(ctx->plots, ctx->house_rulers);
            }
            break;
        case 22:
            AspectMatrix matrix_sign = {0};
            matrix_sign = calculate_aspects_by_sign(ctx->plots);
            display_aspects_by_sign(ctx->plots, &matrix_sign);
            break;
        case 23:
            AspectMatrix matrix_ants = {0}; 
            matrix_ants = calculate_aspects_antiscium(ctx->plots, ctx->ants, 14);
            display_aspects_antissium(ctx->plots, ctx->ants, 14, &matrix_ants);
            break;
        default:
            break;
    }

    if (saiu_retorno) {
        *ctx->local_time = julian_day_para_struct_tm(ctx->julian_day);

        ctx->local_time->tm_hour += ctx->tz_offset;
        timegm(ctx->local_time);
    }

    delwin(win);
    delwin(shadow);
}