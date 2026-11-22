#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <libintl.h>
#include "swephexp.h"
#include "fixed_stars.h"
#include "number_helper.h"
#include "db-utils.h"
#include "helper.h"
#include "draw-chart.h"
#include "selections.h"
#include "planet_table.h"

#include "var.h"

// const TraditionalStar TRADITIONAL_STARS[] = {
//     // ÁRIES
//     {"ALB", "Algenib",       "Algenib",         "gaPeg", "Mar/Mer", 1.5, CAT_PTOLEMAIC},
//     {"ALF", "Alpheratz",     "Alpheratz",       "alAnd", "Jup/Ven", 1.5, CAT_PTOLEMAIC},
//     {"BKA", "Baten Kaitos",  "Baten Kaitos",    "zeCet", "Sat",     1.0, CAT_PTOLEMAIC},
//     {"MIR", "Mirach",        "Mirach",          "beAnd", "Ven",     1.5, CAT_PTOLEMAIC},
//     {"HAM", "Hamal",         "Hamal",           "alAri", "Mar/Sat", 1.5, CAT_PTOLEMAIC},
//     {"SCH", "Schedar",       "Schedar",         "alCas", "Sat/Ven", 1.5, CAT_PTOLEMAIC},
//     {"ALM", "Almach",        "Almach",          "gaAnd", "Ven",     1.5, CAT_PTOLEMAIC},

//     // TOURO
//     {"MKR", "Menkar",        "Menkar",          "alCet", "Sat",     1.5, CAT_PTOLEMAIC},
//     {"CPU", "Capulus",       "Capulus (M34)",   "M34",   "Mar/Mer", 1.0, CAT_NEBULA},
//     {"ALG", "Algol",         "Algol",           "bePer", "Sat/Jup", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC},
//     {"ALC", "Alcyone",       "Alcyone (Plêiades)", "etTau", "Moo/Mar", 1.5, CAT_BEHENIAN | CAT_PTOLEMAIC | CAT_NEBULA},
//     {"ALD", "Aldebaran",     "Aldebaran",       "alTau", "Mar",     2.0, CAT_ROYAL | CAT_BEHENIAN | CAT_PTOLEMAIC},

//     // GÊMEOS
//     {"RIG", "Rigel",         "Rigel",           "beOri", "Jup/Mar", 2.0, CAT_PTOLEMAIC},
//     {"BEL", "Bellatrix",     "Bellatrix",       "gaOri", "Mar/Mer", 1.5, CAT_PTOLEMAIC},
//     {"CAP", "Capella",       "Capella",         "alAur", "Mar/Mer", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC},
//     {"ALN", "Alnilam",       "Alnilam",         "epOri", "Jup/Sat", 1.5, CAT_PTOLEMAIC},
//     {"BTL", "Betelgeuse",    "Betelgeuse",      "alOri", "Mar/Mer", 2.0, CAT_PTOLEMAIC},
//     {"MEN", "Menkalinan",    "Menkalinan",      "beAur", "Mar/Mer", 1.5, CAT_PTOLEMAIC},

//     // CÂNCER
//     {"SIR", "Sirius",        "Sirius",          "alCMa", "Jup/Mar", 2.5, CAT_BEHENIAN | CAT_PTOLEMAIC},
//     {"CAN", "Canopus",       "Canopus",         "alCar", "Sat/Jup", 2.0, CAT_PTOLEMAIC},
//     {"CAS", "Castor",        "Castor",          "alGem", "Mer",     1.5, CAT_PTOLEMAIC},
//     {"POL", "Pollux",        "Pollux",          "beGem", "Mar",     2.0, CAT_PTOLEMAIC},
//     {"PRO", "Procyon",       "Procyon",         "alCMi", "Mer/Mar", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC},

//     // LEÃO
//     {"PRA", "Praesepe",      "Praesepe (M44)",  "M44",   "Mar/Moo", 1.0, CAT_NEBULA | CAT_PTOLEMAIC},
//     {"ALP", "Alphard",       "Alphard",         "alHya", "Sat/Ven", 1.5, CAT_PTOLEMAIC},
//     {"REG", "Regulus",       "Regulus",         "alLeo", "Mar/Jup", 2.0, CAT_ROYAL | CAT_BEHENIAN | CAT_PTOLEMAIC},

//     // VIRGEM
//     {"ZOS", "Zosma",         "Zosma",           "deLeo", "Sat/Ven", 1.5, CAT_PTOLEMAIC},
//     {"DEN", "Denebola",      "Denebola",        "beLeo", "Sat/Ven", 1.5, CAT_PTOLEMAIC},
//     {"ALK", "Alkaid",        "Alkaid (Benetnasch)", "etUMa", "Moo/Sat", 1.5, CAT_BEHENIAN | CAT_PTOLEMAIC},
//     {"VIX", "Vindemiatrix",  "Vindemiatrix",    "epVir", "Sat/Mer", 1.5, CAT_PTOLEMAIC},

//     // LIBRA
//     {"AGO", "Algorab",       "Algorab",         "deCrv", "Mar/Sat", 1.5, CAT_BEHENIAN | CAT_PTOLEMAIC},
//     {"SPI", "Spica",         "Spica",           "alVir", "Ven/Mar", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC},
//     {"ARC", "Arcturus",      "Arcturus",        "alBoo", "Jup/Mar", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC},

//     // ESCORPIÃO
//     {"ACA", "Alphecca",      "Alphecca (Gemma)","alCrB", "Ven/Mer", 1.5, CAT_BEHENIAN | CAT_PTOLEMAIC},
//     {"ZBI", "Zubenelgenubi", "Zubenelgenubi",   "alLib", "Sat/Mar", 1.5, CAT_PTOLEMAIC},
//     {"ZLI", "Zubeneschamali","Zubeneschamali",  "beLib", "Jup/Mer", 1.5, CAT_PTOLEMAIC},
//     {"UNU", "Unukalhai",     "Unukalhai",       "alSer", "Sat/Mar", 1.5, CAT_PTOLEMAIC},

//     // SAGITÁRIO
//     {"ANT", "Antares",       "Antares",         "alSco", "Mar/Jup", 2.0, CAT_ROYAL | CAT_BEHENIAN | CAT_PTOLEMAIC},
//     {"RAS", "Rasalhague",    "Rasalhague",      "alOph", "Sat/Ven", 1.5, CAT_PTOLEMAIC},
//     {"LES", "Lesath",        "Lesath",          "upSco", "Mer/Mar", 1.5, CAT_PTOLEMAIC},
//     {"ACS", "Aculeus",       "Aculeus (M6)",    "M6",    "Mar/Moo", 1.0, CAT_NEBULA},
//     {"ACU", "Acumen",        "Acumen (M7)",     "M7",    "Mar/Moo", 1.0, CAT_NEBULA},

//     // CAPRICÓRNIO
//     {"FAC", "Facies",        "Facies (M22)",    "M22",   "Sun/Mar", 1.0, CAT_NEBULA | CAT_PTOLEMAIC},
//     {"VEG", "Vega",          "Vega",            "alLyr", "Ven/Mer", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC},

//     // AQUÁRIO
//     {"ALT", "Altair",        "Altair",          "alAql", "Mar/Jup", 2.0, CAT_PTOLEMAIC},
//     {"DBA", "Deneb Algedi",  "Deneb Algedi",    "deCap", "Sat/Jup", 1.5, CAT_BEHENIAN | CAT_PTOLEMAIC},

//     // PEIXES
//     {"FOM", "Fomalhaut",     "Fomalhaut",       "alPsA", "Ven/Mer", 2.0, CAT_ROYAL | CAT_PTOLEMAIC},
//     {"DBD", "Deneb Adige",   "Deneb Adige",     "alCyg", "Ven/Mer", 2.0, CAT_PTOLEMAIC},
//     {"ACH", "Achernar",      "Achernar",        "alEri", "Jup",     2.0, CAT_PTOLEMAIC},
//     {"MKB", "Markab",        "Markab",          "alPeg", "Mar/Mer", 1.5, CAT_PTOLEMAIC},
//     {"SCT", "Scheat",        "Scheat",          "bePeg", "Mar/Mer", 1.5, CAT_PTOLEMAIC}
// };


const TraditionalStar TRADITIONAL_STARS[] = {
    // ÁRIES
    {"ALB", "Algenib",       "Algenib",            "gaPeg", "Mar/Mer", 1.5, CAT_PTOLEMAIC,
     N_("Wing of Pegasus. Bestows keen intellect, eloquence, ambition, and quick perception, though warning against impulsiveness or sharp disputes.")},
    {"ALF", "Alpheratz",     "Alpheratz",          "alAnd", "Jup/Ven", 1.5, CAT_PTOLEMAIC,
     N_("Head of Andromeda. Confers independence, grace, honor, freedom of action, and harmony in public or social relationships.")},
    {"BKA", "Baten Kaitos",  "Baten Kaitos",       "zeCet", "Sat",     1.0, CAT_PTOLEMAIC,
     N_("Belly of the Sea Monster. Brings isolation, forced changes of residence, or trials through travel and sudden obstacles.")},
    {"MIR", "Mirach",        "Mirach",             "beAnd", "Ven",     1.5, CAT_PTOLEMAIC,
     N_("Girdle of Andromeda. Grants personal charm, artistic talent, devotion, marital happiness, and reconciliation.")},
    {"HAM", "Hamal",         "Hamal",              "alAri", "Mar/Sat", 1.5, CAT_PTOLEMAIC,
     N_("Head of the Ram. Confers fierce independence, pioneering courage, and leadership, but requires restraint against headstrong behavior.")},
    {"SCH", "Schedar",       "Schedar",            "alCas", "Sat/Ven", 1.5, CAT_PTOLEMAIC,
     N_("Breast of Cassiopeia. Bestows dignity, royal poise, quiet authority, and honor earned through gravitas and self-command.")},
    {"ALM", "Almach",        "Almach",             "gaAnd", "Ven",     1.5, CAT_PTOLEMAIC,
     N_("Foot of Andromeda. Brings artistic distinction, popularity, elegance, and favorable support from influential allies.")},

    // TOURO
    {"MKR", "Menkar",        "Menkar",             "alCet", "Sat",     1.5, CAT_PTOLEMAIC,
     N_("Jaw of the Whale. Indicates deep collective currents, endurance through trials, and sudden reversals if acting without foresight.")},
    {"CPU", "Capulus",       "Capulus (M34)",      "M34",   "Mar/Mer", 1.0, CAT_NEBULA,
     N_("Sword Hilt of Perseus. Concentrates sharp, penetrating action and decisiveness; traditionally linked to eye strain or acute physical risks.")},
    {"ALG", "Algol",         "Algol",              "bePer", "Sat/Jup", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("Head of Medusa (Caput Algol). The most intense star in classical astrology; channels raw, uncompromising power and fierce protection when mastered, or sudden crises if unchecked.")},
    {"ALC", "Alcyone",       "Alcyone (Plêiades)", "etTau", "Moo/Mar", 1.5, CAT_BEHENIAN | CAT_PTOLEMAIC | CAT_NEBULA,
     N_("Central star of the Pleiades. Confers mystical insight, deep emotional sensitivity, and inner vision, traditionally associated with ocular sensitivity or sorrow.")},
    {"ALD", "Aldebaran",     "Aldebaran",          "alTau", "Mar",     2.0, CAT_ROYAL | CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("Royal Star of the Bull (Watcher of the East). Grants high honor, courage, and public elevation, provided strict integrity is maintained without compromise.")},

    // GÊMEOS
    {"RIG", "Rigel",         "Rigel",              "beOri", "Jup/Mar", 2.0, CAT_PTOLEMAIC,
     N_("Foot of Orion. Bestows technical mastery, teaching ability, inventive brilliance, and lasting achievement through disciplined effort.")},
    {"BEL", "Bellatrix",     "Bellatrix",          "gaOri", "Mar/Mer", 1.5, CAT_PTOLEMAIC,
     N_("Left Shoulder of Orion. Confers rapid decision-making, strategic courage, and eloquence, though cautioning against recklessness after early victories.")},
    {"CAP", "Capella",       "Capella",            "alAur", "Mar/Mer", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("The Little She-Goat. Grants intellectual curiosity, love of learning, independence, and unconventional honors in scholarly or inventive fields.")},
    {"ALN", "Alnilam",       "Alnilam",            "epOri", "Jup/Sat", 1.5, CAT_PTOLEMAIC,
     N_("Belt of Orion. Brings fleeting public honors, organizational command, and strong ideals that require solid grounding to endure.")},
    {"BTL", "Betelgeuse",    "Betelgeuse",         "alOri", "Mar/Mer", 2.0, CAT_PTOLEMAIC,
     N_("Right Shoulder of Orion. One of the most fortunate martial stars, conferring unblocked advancement, executive command, and military or civic honors.")},
    {"MEN", "Menkalinan",    "Menkalinan",         "beAur", "Mar/Mer", 1.5, CAT_PTOLEMAIC,
     N_("Shoulder of the Charioteer. Gives dexterity, adaptability, and skill in managing complex systems, requiring balance to avoid overextension.")},

    // CÂNCER
    {"SIR", "Sirius",        "Sirius",             "alCMa", "Jup/Mar", 2.5, CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("The Dog Star, brightest in the heavens. Magnifies all it touches, bestowing brilliance, guardianship, renown, and extraordinary achievement through devotion.")},
    {"CAN", "Canopus",       "Canopus",            "alCar", "Sat/Jup", 2.0, CAT_PTOLEMAIC,
     N_("Helmsman of the Ship Argo. Grants broad philosophical vision, mastery over long journeys or complex enterprises, and conservative wisdom.")},
    {"CAS", "Castor",        "Castor",             "alGem", "Mer",     1.5, CAT_PTOLEMAIC,
     N_("The Mortal Twin. Bestows sharp wit, literary or intellectual distinction, and communicative skill, though marked by sudden shifts of fortune.")},
    {"POL", "Pollux",        "Pollux",             "beGem", "Mar",     2.0, CAT_PTOLEMAIC,
     N_("The Immortal Twin. Confers fearless tenacity, athletic or strategic strength, and the resilience to prevail after hard struggles.")},
    {"PRO", "Procyon",       "Procyon",            "alCMi", "Mer/Mar", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("The Lesser Dog. Brings quick wit, rapid rise, and decisive action, with the classical warning against haste and carelessness once success is reached.")},

    // LEÃO
    {"PRA", "Praesepe",      "Praesepe (M44)",     "M44",   "Mar/Moo", 1.0, CAT_NEBULA | CAT_PTOLEMAIC,
     N_("The Beehive Cluster in Cancer. Indicates inner retreat, sensitivity, and protective instincts; traditionally associated with eye strain or hidden vulnerabilities.")},
    {"ALP", "Alphard",       "Alphard",            "alHya", "Sat/Ven", 1.5, CAT_PTOLEMAIC,
     N_("Heart of the Hydra. Grants deep artistic passion, wisdom in human nature, and self-reliance, requiring moderation to avoid emotional excess.")},
    {"REG", "Regulus",       "Regulus",            "alLeo", "Mar/Jup", 2.0, CAT_ROYAL | CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("Royal Star of the Lion (Watcher of the North). Confers nobility, leadership, and high preferment, provided one avoids vindictiveness or thirst for revenge.")},

    // VIRGEM
    {"ZOS", "Zosma",         "Zosma",              "deLeo", "Sat/Ven", 1.5, CAT_PTOLEMAIC,
     N_("Back of the Lion. Gives keen discernment, prophetic sensitivity, and service to others, though occasionally carrying burdens on behalf of the collective.")},
    {"DEN", "Denebola",      "Denebola",           "beLeo", "Sat/Ven", 1.5, CAT_PTOLEMAIC,
     N_("Tail of the Lion. Bestows independent judgment, unconventional insight, and swift turns of fortune that reward swimming against the tide.")},
    {"ALK", "Alkaid",        "Alkaid (Benetnasch)","etUMa", "Moo/Sat", 1.5, CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("End of the Great Bear's Tail. Confers solemnity, endurance, and deep ancestral awareness; traditionally linked to mourning or profound life transitions.")},
    {"VIX", "Vindemiatrix",  "Vindemiatrix",       "epVir", "Sat/Mer", 1.5, CAT_PTOLEMAIC,
     N_("The Grape Gatherer. Grants analytical precision and gathering of resources, though traditionally connected to separations or widowhood in close bonds.")},

    // LIBRA
    {"AGO", "Algorab",       "Algorab",            "deCrv", "Mar/Sat", 1.5, CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("Wing of the Raven. Confers shrewdness, realism, and strategic scavenging in difficult environments, requiring ethical clarity.")},
    {"SPI", "Spica",         "Spica",              "alVir", "Ven/Mar", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("Ear of Wheat of the Virgin. One of the premier fortunate stars, bestowing artistic or scientific brilliance, protection, and enduring honors.")},
    {"ARC", "Arcturus",      "Arcturus",           "alBoo", "Jup/Mar", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("The Bear Guardian. Grants pioneering leadership, justice, pathfinding ability, and prosperity earned through self-determined action.")},

    // ESCORPIÃO
    {"ACA", "Alphecca",      "Alphecca (Gemma)",   "alCrB", "Ven/Mer", 1.5, CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("The Northern Crown. Bestows poetic gifts, grace, social distinction, and quiet dignity earned through artistic or intellectual merit.")},
    {"ZBI", "Zubenelgenubi", "Zubenelgenubi",      "alLib", "Sat/Mar", 1.5, CAT_PTOLEMAIC,
     N_("Southern Scale. Calls for rigorous fairness and social responsibility; warns against exacting penalties that rebound upon oneself.")},
    {"ZLI", "Zubeneschamali","Zubeneschamali",     "beLib", "Jup/Mer", 1.5, CAT_PTOLEMAIC,
     N_("Northern Scale. Confers civic honor, intellectual breadth, eloquence, and lasting advancement through constructive alliances.")},
    {"UNU", "Unukalhai",     "Unukalhai",          "alSer", "Sat/Mar", 1.5, CAT_PTOLEMAIC,
     N_("Neck of the Serpent. Grants deep psychological insight and healing wisdom, though testing emotional self-control in crises.")},

    // SAGITÁRIO
    {"ANT", "Antares",       "Antares",            "alSco", "Mar/Jup", 2.0, CAT_ROYAL | CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("Royal Star of the Scorpion (Watcher of the West). Bestows intensity, courage, and strategic brilliance, warning against obsession and extremism.")},
    {"RAS", "Rasalhague",    "Rasalhague",         "alOph", "Sat/Ven", 1.5, CAT_PTOLEMAIC,
     N_("Head of the Serpent Bearer. Confers therapeutic skill, diagnostic insight, and mastery over poisons, remedies, and hidden ailments.")},
    {"LES", "Lesath",        "Lesath",             "upSco", "Mer/Mar", 1.5, CAT_PTOLEMAIC,
     N_("Sting of the Scorpion. Grants razor-sharp mental penetration and boldness, requiring caution against corrosive speech or rashness.")},
    {"ACS", "Aculeus",       "Aculeus (M6)",       "M6",    "Mar/Moo", 1.0, CAT_NEBULA,
     N_("Upper Sting Cluster. Sharpens perception and resilience under verbal or mental attacks; traditionally linked to visual sensitivity.")},
    {"ACU", "Acumen",        "Acumen (M7)",        "M7",    "Mar/Moo", 1.0, CAT_NEBULA,
     N_("Lower Sting Cluster. Gives tenacity and endurance through adversity, though requiring vigilance regarding eyesight and nervous strain.")},

    // CAPRICÓRNIO
    {"FAC", "Facies",        "Facies (M22)",       "M22",   "Sun/Mar", 1.0, CAT_NEBULA | CAT_PTOLEMAIC,
     N_("Eye of the Archer. Projects intense, focused willpower and piercing vision, demanding discipline to avoid harshness or ocular strain.")},
    {"VEG", "Vega",          "Vega",               "alLyr", "Ven/Mer", 2.0, CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("Star of the Lyre. Bestows artistic charisma, musical or poetic refinement, idealism, and quiet magnetic authority.")},

    // AQUÁRIO
    {"ALT", "Altair",        "Altair",             "alAql", "Mar/Jup", 2.0, CAT_PTOLEMAIC,
     N_("The Flying Eagle. Confers boldness, swift ascent, generosity, and fearless action in moments demanding decisive leadership.")},
    {"DBA", "Deneb Algedi",  "Deneb Algedi",       "deCap", "Sat/Jup", 1.5, CAT_BEHENIAN | CAT_PTOLEMAIC,
     N_("Tail of the Sea-Goat. Grants judicial wisdom, integrity, and the ability to bring order, law, and fair counsel to communities.")},

    // PEIXES
    {"FOM", "Fomalhaut",     "Fomalhaut",          "alPsA", "Ven/Mer", 2.0, CAT_ROYAL | CAT_PTOLEMAIC,
     N_("Royal Star of the Southern Fish (Watcher of the South). Brings charisma, artistic or spiritual eminence, and high ideals requiring purity of motive.")},
    {"DBD", "Deneb Adige",   "Deneb Adige",        "alCyg", "Ven/Mer", 2.0, CAT_PTOLEMAIC,
     N_("Tail of the Swan. Bestows quick comprehension, artistic grace, idealism, and elevation through intellect or creative mastery.")},
    {"ACH", "Achernar",      "Achernar",           "alEri", "Jup",     2.0, CAT_PTOLEMAIC,
     N_("End of the River. Confers philosophical dignity, moral authority, public honor, and success in high office or spiritual leadership.")},
    {"MKB", "Markab",        "Markab",             "alPeg", "Mar/Mer", 1.5, CAT_PTOLEMAIC,
     N_("Saddle of Pegasus. Grants mental vigor, ambition, and literary or practical drive, though advising prudence against avoidable hazards.")},
    {"SCT", "Scheat",        "Scheat",             "bePeg", "Mar/Mer", 1.5, CAT_PTOLEMAIC,
     N_("Leg of Pegasus. Bestows original, avant-garde intellect and deep poetic or philosophical insight, warning against extreme risks.")}
};



const size_t TRADITIONAL_STARS_COUNT = sizeof(TRADITIONAL_STARS) / sizeof(TRADITIONAL_STARS[0]);


int calculate_traditional_stars(
    double tjd_ut,
    unsigned int active_categories, // Ex: CAT_BEHENIAN | CAT_ROYAL
    int extra_iflag,                // Ex: SEFLG_SWIEPH (ou 0) + SEFLG_SIDEREAL se desejado
    CalculatedStar *results,        // Array pré-alocado com tamanho TRADITIONAL_STARS_COUNT
    char *err_log                   // Buffer opcional para erros
) {
    int count = 0;

    if (!estrelas_fixas_disponiveis()) {
        show_alert_popup(
            _("Check if 'sefstars.txt' is inside your ephe folder."),
            _("Fixed Stars catalog not loaded!")
        );
        return 0;
    }
    
    char star_buf[SE_MAX_STNAME]; // Tamanho seguro da própria Swiss Ephemeris
    char serr[AS_MAXCH];
    double xx_ecl[6];
    double xx_equ[6];


    for (size_t i = 0; i < TRADITIONAL_STARS_COUNT; i++) {
        // 1. Filtra pela categoria selecionada pelo usuário
        if ((TRADITIONAL_STARS[i].categories & active_categories) == 0) {
            continue;
        }

        // 2. Copia o nome para o buffer mutável (obrigatório!)
        snprintf(star_buf, sizeof(star_buf), "%s", TRADITIONAL_STARS[i].swe_name);

        // 3. Calcula coordenadas eclípticas (Longitude e Latitude)
        int iflag_ecl = extra_iflag | SEFLG_SPEED | SEFLG_TOPOCTR;
        int ret = swe_fixstar2_ut(star_buf, tjd_ut, iflag_ecl, xx_ecl, serr);

        // Segunda tentativa: pela designação Bayer/Messier com vírgula na frente
        if (ret <= 0 && TRADITIONAL_STARS[i].bayer != NULL) {
            snprintf(star_buf, sizeof(star_buf), ",%s", TRADITIONAL_STARS[i].bayer);
            ret = swe_fixstar2_ut(star_buf, tjd_ut, iflag_ecl, xx_ecl, serr);
        }

        if (ret <= 0) {
            snprintf(err_log, AS_MAXCH * 2, "[DEBUG STARS] Falhou [%zu]: %s (bayer: %s) -> %s\n",
                    i, TRADITIONAL_STARS[i].swe_name, TRADITIONAL_STARS[i].bayer, serr);
            show_alert_popup(err_log, "");
            continue;
        }
        if (ret <= 0) {
            if (err_log) {                
                snprintf(err_log, AS_MAXCH * 2, "Erro na estrela %s: %s", TRADITIONAL_STARS[i].display_name, serr);
                show_alert_popup(err_log, "");
            }
            continue;
        }

        // 4. Calcula Declinação (coordenadas equatoriais) para aspectos de Paralelo
        // Reutiliza o star_buf já resolvido pela chamada anterior
        int iflag_equ = extra_iflag | SEFLG_EQUATORIAL | SEFLG_TOPOCTR;
        swe_fixstar2_ut(star_buf, tjd_ut, iflag_equ, xx_equ, serr);

        // 5. Obtém a magnitude aparente da estrela
        double mag = 0.0;
        swe_fixstar2_mag(star_buf, &mag, serr);

        // 6. Armazena o resultado
        results[count].meta        = &TRADITIONAL_STARS[i];
        results[count].longitude   = xx_ecl[0];
        results[count].latitude    = xx_ecl[1];
        results[count].ra          = xx_equ[0];
        results[count].declination = xx_equ[1]; // xx_equ[0] é Ascensão Reta, xx_equ[1] é Declinação
        results[count].magnitude   = mag;
        count++;

    }

    return count; // Retorna quantas estrelas foram calculadas com sucesso
}



// Função auxiliar para obter os 4 horários angulares de um planeta ou estrela
// Se star_name != NULL, calcula para a estrela; caso contrário, usa planet_id.
static void get_four_angles_ut(double tjd_start, int planet_id, const char *star_name, double geopos[3], int iflag, double angle_times[ANGLE_COUNT]) {
    int rsmi_flags[ANGLE_COUNT] = {
        SE_CALC_RISE     | SE_BIT_DISC_CENTER | SE_BIT_NO_REFRACTION,
        SE_CALC_MTRANSIT | SE_BIT_DISC_CENTER | SE_BIT_NO_REFRACTION,
        SE_CALC_SET      | SE_BIT_DISC_CENTER | SE_BIT_NO_REFRACTION,
        SE_CALC_ITRANSIT | SE_BIT_DISC_CENTER | SE_BIT_NO_REFRACTION
    };

    char star_buf[SE_MAX_STNAME];
    char serr[AS_MAXCH];

    for (int a = 0; a < ANGLE_COUNT; a++) {
        angle_times[a] = 0.0; // 0.0 indica que não ocorreu (ex: estrela circumpolar)

        if (star_name != NULL) {
            snprintf(star_buf, sizeof(star_buf), "%s", star_name);
            int ret = swe_rise_trans(
                tjd_start, SE_SUN, star_buf, iflag,
                rsmi_flags[a], geopos, 0.0, 0.0, &angle_times[a], serr
            );
            if (ret < 0) angle_times[a] = 0.0;
        } else {
            int ret = swe_rise_trans(
                tjd_start, planet_id, NULL, iflag,
                rsmi_flags[a], geopos, 0.0, 0.0, &angle_times[a], serr
            );
            if (ret < 0) angle_times[a] = 0.0;
        }
    }
}


// Função principal para detectar Parans no dia natal
int calculate_parans(double tjd_ut, double geo_lon, double geo_lat, double geo_alt, int iflag, CalculatedStar *stars, int star_count, const int *planets, int planet_count, double max_orb_minutes, ParanResult *out_parans, int max_out_parans) {
    double geopos[3] = {geo_lon, geo_lat, geo_alt};
    // Inicia a busca 12 horas (0.5 dias) antes do nascimento para cobrir as 24h do dia natal
    double tjd_window_start = tjd_ut - 0.5;

    // 1. Pré-calcula os 4 ângulos para todos os planetas (evita recálculos no loop interno)
    double planet_angles[15][ANGLE_COUNT]; // Suporta até 15 corpos/planetas
    for (int p = 0; p < planet_count; p++) {
        get_four_angles_ut(tjd_window_start, planets[p], NULL, geopos, iflag, planet_angles[p]);
    }

    int found = 0;

    // 2. Para cada estrela, calcula seus 4 ângulos e compara com todos os planetas
    for (int s = 0; s < star_count; s++) {
        double star_angles[ANGLE_COUNT];
        get_four_angles_ut(
            tjd_window_start, 0, stars[s].meta->swe_name, geopos, iflag, star_angles
        );

        for (int sa = 0; sa < ANGLE_COUNT; sa++) {
            if (star_angles[sa] <= 0.0) continue; // Estrela circumpolar (nunca nasce/põe nesta latitude)

            for (int p = 0; p < planet_count; p++) {
                for (int pa = 0; pa < ANGLE_COUNT; pa++) {
                    if (planet_angles[p][pa] <= 0.0) continue;

                    // Diferença em dias convertida para minutos (1 dia = 1440 minutos)
                    double diff_mins = fabs(star_angles[sa] - planet_angles[p][pa]) * 1440.0;

                    if (diff_mins <= max_orb_minutes) {

                        if (!stars[s].heliacal_calculated) {
                            HeliacalInfo hinfo = get_star_heliacal_info(
                                tjd_ut, geo_lon, geo_lat, geo_alt, &stars[s]
                            );
                            stars[s].heliacal_state = hinfo.state;
                            stars[s].heliacal_calculated = true;
                        }
                
                        
                        if (found < max_out_parans) {
                            out_parans[found].star_index     = s;
                            out_parans[found].planet_id      = planets[p];
                            out_parans[found].star_angle     = (ParanAngle)sa;
                            out_parans[found].planet_angle   = (ParanAngle)pa;
                            out_parans[found].time_diff_mins = diff_mins;
                            out_parans[found].stars = &stars[s];
                            found++;
                        }
                    }
                }
            }
        }
    }

    return found;
}





// Calcula diferença circular entre duas horas (0..24h) retornando em minutos
static double diff_hours_circular_mins(double h1, double h2) {
    double diff = fabs(h1 - h2);
    if (diff > 12.0) {
        diff = 24.0 - diff; // Trata virada de meia-noite (ex: 23:55 e 00:05 = 10 min)
    }
    return diff * 60.0;
}

// Converte TJD (Julian Day UT) para horas decimais do dia (0.0 a 24.0)
static double tjd_to_ut_hours(double tjd) {
    int y, m, d;
    double h;
    swe_revjul(tjd, SE_GREG_CAL, &y, &m, &d, &h);
    return h;
}

int compute_parans(double tjd_ut, double geo_lon, double geo_lat, double geo_alt, int iflag, int *planets, int planet_count, CalculatedStar *stars, int star_count, double max_orb_minutes, ParanEntry *out_list, int max_entries) {

    if (!estrelas_fixas_disponiveis()) {
        show_alert_popup(
            _("Check if 'sefstars.txt' is inside your ephe folder."),
            _("Fixed Stars catalog not loaded!")
        );
        return 0;
    }

    double geopos[3] = {geo_lon, geo_lat, geo_alt};
    double tjd_start = tjd_ut - 0.5; // Janela de 24h centrada no nascimento

    int rsmi_list[4] = {
        SE_CALC_RISE     | SE_BIT_DISC_CENTER | SE_BIT_NO_REFRACTION,
        SE_CALC_SET      | SE_BIT_DISC_CENTER | SE_BIT_NO_REFRACTION,
        SE_CALC_MTRANSIT | SE_BIT_DISC_CENTER | SE_BIT_NO_REFRACTION,
        SE_CALC_ITRANSIT | SE_BIT_DISC_CENTER | SE_BIT_NO_REFRACTION
    };

    const char *angle_labels[4] = {
        _("Rising"),
        _("Setting"),
        _("Culminating"),
        _("Anti-Culminating")
    };

    char star_buf[SE_MAX_STNAME];
    char serr[AS_MAXCH];

    // 1. Pré-calcula os 4 ângulos geométricos dos planetas UMA ÚNICA VEZ (fora do loop das estrelas)
    double planet_hours[15][4];
    static char planet_names[15][32]; // Buffer estático seguro para apontar em ParanEntry.planet_name
    static char *planet_glyphs[15][10];
    static const int brady_map[4] = {0, 2, 1, 3}; // Mapeia índice pa (Rise, Set, MC, IC) para Brady

    if (planet_count > 15) planet_count = 15; // Proteção de limite do array

    for (int p = 0; p < planet_count; p++) {
        int ipl = planets[p];
        swe_get_planet_name(ipl, planet_names[p]);
        get_planet_symbol(ipl + 1, planet_glyphs[p]);

        for (int pa = 0; pa < 4; pa++) {
            planet_hours[p][pa] = -1.0;
            double tret = 0.0;
            int ret = swe_rise_trans(
                tjd_start, ipl, NULL, iflag,
                rsmi_list[pa], geopos, 0.0, 0.0, &tret, serr
            );
            if (ret >= 0 && tret > 0.0) {
                planet_hours[p][pa] = tjd_to_ut_hours(tret);
            }
        }
    }

    int count = 0;

    // 2. Calcula os 4 ângulos de cada estrela e compara com todos os planetas
    for (int s = 0; s < star_count; s++) {
        double star_hours[4] = {-1.0, -1.0, -1.0, -1.0};

        for (int sa = 0; sa < 4; sa++) {
            double tret = 0.0;
            snprintf(star_buf, sizeof(star_buf), "%s", stars[s].meta->swe_name);
            int ret = swe_rise_trans(
                tjd_start, SE_SUN, star_buf, iflag,
                rsmi_list[sa], geopos, 0.0, 0.0, &tret, serr
            );

            // Fallback usando designação Bayer (ex: ",M34") se o nome principal falhar
            if (ret < 0 && stars[s].meta->bayer != NULL) {
                snprintf(star_buf, sizeof(star_buf), ",%s", stars[s].meta->bayer);
                ret = swe_rise_trans(
                    tjd_start, SE_SUN, star_buf, iflag,
                    rsmi_list[sa], geopos, 0.0, 0.0, &tret, serr
                );
            }

            if (ret >= 0 && tret > 0.0) {
                star_hours[sa] = tjd_to_ut_hours(tret);
            }
        }
        

        // 3. Cruzamento: compara cada ângulo da estrela com cada ângulo de cada planeta
        for (int p = 0; p < planet_count; p++) {
            for (int sa = 0; sa < 4; sa++) {
                if (star_hours[sa] < 0.0) continue; // Estrela circumpolar (não nasce/põe nesta latitude)

                for (int pa = 0; pa < 4; pa++) {
                    if (planet_hours[p][pa] < 0.0) continue;

                    double diff_mins = diff_hours_circular_mins(star_hours[sa], planet_hours[p][pa]);

                    if (diff_mins <= max_orb_minutes && count < max_entries) {
                        // Calcula a fase helíaca uma única vez por estrela ativa
                        if (!stars[s].heliacal_calculated) {
                            HeliacalInfo hinfo = get_star_heliacal_info(
                                tjd_ut, geo_lon, geo_lat, geo_alt, &stars[s]
                            );
                            stars[s].heliacal_state = hinfo.state;
                            stars[s].heliacal_calculated = true;
                        }

                        out_list[count].star_index      = s;
                        out_list[count].object          = *planet_glyphs[p];
                        out_list[count].planet_name     = _(planet_names[p]);
                        out_list[count].planet_angle    = angle_labels[pa];
                        out_list[count].star_name       = stars[s].meta->display_name;
                        out_list[count].star_nature     = stars[s].meta->nature;
                        out_list[count].star_angle      = angle_labels[sa];
                        out_list[count].star_time_hours = star_hours[sa];
                        out_list[count].diff_minutes    = diff_mins;
                        out_list[count].planet_order    = p;
                        out_list[count].brady_phase     = brady_map[pa];
                        out_list[count].angle_planet    = pa;
                        out_list[count].stars           = &stars[s]; // Aponta para a estrela 's', não para o início do array!

                        count++;
                    }
                }
            }
        }
    }

    return count;
}
    



// Janela ncurses seguindo exatamente a identidade visual do seu software
void display_parans_table(ParanEntry *parans, int paran_count, CalculatedStar *stars, double tz_offset, double max_orb_minutes) {

    (void)stars;
    //const char *brady_comp[] = {_("Youth"), _("Prime"), _("Later"), _("Hidden")};

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int table_height = 24;
    if (table_height > max_y - 2) table_height = max_y - 2;
    int table_width = max_x - 5;

    int start_y = (max_y - table_height) / 2;
    int start_x = (max_x - table_width) / 2;

    WINDOW *table_win = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);

    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(6) | FLAGS);

    // Título centralizado
    wattron(table_win, A_BOLD);
    char title[128];
    snprintf(title, sizeof(title), _(" Fixed Stars Parans (Max Orb: %.0f min | Total: %d) "),
             max_orb_minutes, paran_count);
    int title_x = (table_width - (int)strlen(title)) / 2;
    if (title_x < 2) title_x = 2;
    mvwprintw(table_win, 0, title_x, "%s", title);

    // Cabeçalho das colunas (ajuste na linha 2 da table_win antes do while)
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 2, 7,   "%-6s",  _("Planet"));
    mvwprintw(table_win, 2, 18,  "%-20s", _("Planet Angle"));
    mvwprintw(table_win, 2, 37,  "%-20s", _("Fixed Star"));
    mvwprintw(table_win, 2, 59,  "%-10s", _("Nature"));
    mvwprintw(table_win, 2, 71,  "%-14s", _("Star Angle"));
    mvwprintw(table_win, 2, 90,  "%-10s", _("Time"));
    mvwprintw(table_win, 2, 105, "%-8s",  _("Orb"));
    mvwprintw(table_win, 2, 115, "%-12s", _("Visibility")); // Nova coluna helíaca
    mvwhline(table_win, 3, 3, ACS_HLINE, table_width - 6);
    wattroff(table_win, A_BOLD);

    // Botão [✖] no canto superior direito
    int col_fechar = getmaxx(table_win) - 4;
    wattron(table_win, COLOR_PAIR(13));
    mvwprintw(table_win, 0, col_fechar, "[");
    mvwprintw(table_win, 0, col_fechar + 2, "]");
    wattroff(table_win, COLOR_PAIR(13));

    wattron(table_win, COLOR_PAIR(13) | A_BOLD);
    mvwprintw(table_win, 0, col_fechar + 1, "✖");
    wattroff(table_win, COLOR_PAIR(13) | A_BOLD);

    
    // Área visível para o pad: de (start_y + 4) até (start_y + table_height - 5)
    // Altura total da viewport = (table_height - 5) - 4 + 1 = table_height - 8
    int max_linhas_dados_visiveis = table_height - 8;
    if (max_linhas_dados_visiveis < 2) max_linhas_dados_visiveis = 2;

    // Cada item ocupa 2 linhas (dados + separador) dentro do scroll_pad
    int total_pad_rows = (paran_count > 0) ? (paran_count * 2) : max_linhas_dados_visiveis;
    if (total_pad_rows < max_linhas_dados_visiveis) {
        total_pad_rows = max_linhas_dados_visiveis;
    }

    int pad_width = table_width - 5;
    WINDOW *scroll_pad = newpad(total_pad_rows, pad_width);
    wbkgd(scroll_pad, COLOR_PAIR(8) | FLAGS);
    
    int sort_mode = 0; // 0: Planet | 1: Orb | 2: Life Phase (Brady)
    const char *sort_label = _("Planet");;

    int running = 1;

    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(100);

    drag_and_drop_on();

    while (running) {
        if (sort_mode == 0) {
            qsort(parans, paran_count, sizeof(ParanEntry), compare_parans_by_planet);
            sort_label = _("Planet");
        } else if (sort_mode == 1) {
            qsort(parans, paran_count, sizeof(ParanEntry), compare_parans_by_orb);
            sort_label = _("Orb");
        } else {
            qsort(parans, paran_count, sizeof(ParanEntry), compare_parans_by_brady);
            sort_label = _("Life Phase (Brady)");
        }

        werase(table_win);
        box(table_win, 0, 0);

        wattron(table_win, COLOR_PAIR(13));
        mvwprintw(table_win, 0, col_fechar, "[");
        mvwprintw(table_win, 0, col_fechar + 2, "]");
        wattroff(table_win, COLOR_PAIR(13));

        wattron(table_win, COLOR_PAIR(13) | A_BOLD);
        mvwprintw(table_win, 0, col_fechar + 1, "✖");
        wattroff(table_win, COLOR_PAIR(13) | A_BOLD);

        snprintf(title, sizeof(title),
                 _(" Fixed Stars Parans (Max Orb: %.0f min | Total: %d | Sorted by: %s) "),
                 max_orb_minutes, paran_count, sort_label);

        int title_x = (table_width - (int)strlen(title)) / 2;
        if (title_x < 2) title_x = 2;

        wattron(table_win, A_BOLD);
        mvwprintw(table_win, 0, title_x, "%s", title);
        wattroff(table_win, A_BOLD);

        // Cabeçalho das colunas (ajuste na linha 2 da table_win antes do while)
        wattron(table_win, A_BOLD);
        mvwprintw(table_win, 2, 7,   "%-6s",  _("Planet"));
        mvwprintw(table_win, 2, 18,  "%-20s", _("Planet Angle"));
        mvwprintw(table_win, 2, 37,  "%-20s", _("Fixed Star"));
        mvwprintw(table_win, 2, 59,  "%-10s", _("Nature"));
        mvwprintw(table_win, 2, 71,  "%-14s", _("Star Angle"));
        mvwprintw(table_win, 2, 90,  "%-10s", _("Time"));
        mvwprintw(table_win, 2, 105, "%-8s",  _("Orb"));
        mvwprintw(table_win, 2, 115, "%-12s", _("Visibility")); // Nova coluna helíaca
        mvwhline(table_win, 3, 3, ACS_HLINE, table_width - 6);
        wattroff(table_win, A_BOLD);
        
        // 2. Limpa o pad antes de preencher com a nova ordem
        werase(scroll_pad);

        int row_pad = 0;
        if (paran_count == 0) {
            mvwprintw(scroll_pad, 1, 2, _("No Parans found within %.0f minutes orb."), max_orb_minutes);
            row_pad = 2;
        } else {

            for (int idx = 0; idx < paran_count; idx++) {
                double local_hours = parans[idx].star_time_hours + tz_offset;
                if (local_hours < 0.0) local_hours += 24.0;
                else if (local_hours >= 24.0) local_hours -= 24.0;

                Hora h_ev = get_fmt_hour(local_hours);
                int orb_min = (int)parans[idx].diff_minutes;
                int orb_sec = (int)round((parans[idx].diff_minutes - orb_min) * 60.0);
                if (orb_sec == 60) { orb_min++; orb_sec = 0; }

                               
                //int s_idx = parans[idx].star_index;
                //HeliacalState h_state = parans[idx].stars[s_idx].heliacal_state;
                HeliacalState h_state = parans[idx].stars->heliacal_state;

                const char *vis_str = _("Visible");
                if (h_state == HELIACAL_HIDDEN) {
                    vis_str = _("Under Beams");
                } else if (h_state == HELIACAL_CIRCUMPOLAR) {
                    vis_str = _("Circumpolar");
                }

                // Se a estrela está oculta sob os raios, aplica A_DIM; se orbe <= 5m, aplica A_BOLD
                if (h_state == HELIACAL_HIDDEN) {
                    wattron(scroll_pad, A_DIM);
                } else if (parans[idx].diff_minutes <= 5.0) {
                    wattron(scroll_pad, A_BOLD);
                }

                mvwprintw(scroll_pad, row_pad, 5,   "%-4s",  parans[idx].object);
                mvwprintw(scroll_pad, row_pad, 16,  "%-20s", parans[idx].planet_angle);
                mvwprintw(scroll_pad, row_pad, 35,  "%-20s", parans[idx].star_name);
                mvwprintw(scroll_pad, row_pad, 57,  "%-10s", parans[idx].star_nature);
                mvwprintw(scroll_pad, row_pad, 69,  "%-14s", parans[idx].star_angle);
                mvwprintw(scroll_pad, row_pad, 88,  "%02d:%02d:%02d", h_ev.hora, h_ev.min, h_ev.sec);
                mvwprintw(scroll_pad, row_pad, 103, "%02dm %02ds", orb_min, orb_sec);
                mvwprintw(scroll_pad, row_pad, 113, "%-12s", vis_str);

                wattroff(scroll_pad, A_BOLD | A_DIM);

                wattron(scroll_pad, COLOR_PAIR(10) | A_DIM);
                mvwhline(scroll_pad, row_pad + 1, 1, ACS_HLINE, pad_width - 2);
                wattroff(scroll_pad, COLOR_PAIR(10) | A_DIM);

                row_pad += 2;
            }
            
        }

        

        int offset_y = 0;
        int max_scroll_y = row_pad - max_linhas_dados_visiveis;
        if (max_scroll_y < 0) max_scroll_y = 0;

        keypad(table_win, TRUE);
        nodelay(table_win, FALSE);

        int flag = 0;
        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
        wattron(table_win, COLOR_PAIR(28) | flag);
        desenhar_scrollbar(table_win, offset_y, row_pad, max_linhas_dados_visiveis - 1, 4);
        wattroff(table_win, COLOR_PAIR(28) | flag);
        wnoutrefresh(table_win);

        mvwaddstr(table_win, 4, table_width - 2, "▲");
        mvwaddstr(table_win, 4 + max_linhas_dados_visiveis - 1, table_width - 2, "▼");
        
        int itens_visiveis = max_linhas_dados_visiveis / 2;

        if (paran_count > itens_visiveis) {
            int primeiro_item = (offset_y / 2) + 1;
            int ultimo_item   = (offset_y / 2) + itens_visiveis;
            if (ultimo_item > paran_count) {
                ultimo_item = paran_count;
            }

            // Limpa a linha da borda inferior antes de reescrever (evita sobras de dígitos antigos)
            mvwhline(table_win, table_height - 1, 1, ACS_HLINE, table_width - 2);
            mvwprintw(table_win, table_height - 1, 2,
                    _("ESC: Return | ↓/↑: Scroll | [S] Toggle Sort by Planet/Orb (%d-%d of %d)"),
                    primeiro_item, ultimo_item, paran_count);
        } else {
            mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
        }

        wnoutrefresh(table_win);
        prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 5, start_x + table_width - 3);
        doupdate();

        int ch;
        int need_resort = 0; // Flag para sair do loop interno e reordenar

        while (running && !need_resort) {
            ch = wgetch(table_win);

            if (ch == 27 || ch == 'q' || ch == 'Q') {
                running = 0; // Garante que sairá também do while (running) externo!
                break;
            }    

            switch (ch) {
                case 's':
                case 'S':
                    sort_mode = (sort_mode + 1) % 3;
                    need_resort = 1;
                    break;
                case KEY_UP:
                case 'k':
                case 'K':
                    if (offset_y > 0) offset_y -= 2;
                    if (paran_count > itens_visiveis) {
                        int primeiro_item = (offset_y / 2) + 1;
                        int ultimo_item   = (offset_y / 2) + itens_visiveis;
                        if (ultimo_item > paran_count) {
                            ultimo_item = paran_count;
                        }
                
                        // Limpa a linha da borda inferior antes de reescrever (evita sobras de dígitos antigos)
                        mvwhline(table_win, table_height - 1, 1, ACS_HLINE, table_width - 2);
                        mvwprintw(table_win, table_height - 1, 2,
                            _("ESC: Return | ↓/↑: Scroll | [S] Toggle Sort by Planet/Orb (%d-%d of %d)"),
                                primeiro_item, ultimo_item, paran_count);
                    } else {
                        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
                    }
                
                    wnoutrefresh(table_win);
                    break;

                case KEY_DOWN:
                case 'j':
                case 'J':
                    if (offset_y < max_scroll_y) offset_y += 2;
                    if (paran_count > itens_visiveis) {
                        int primeiro_item = (offset_y / 2) + 1;
                        int ultimo_item   = (offset_y / 2) + itens_visiveis;
                        if (ultimo_item > paran_count) {
                            ultimo_item = paran_count;
                        }
                
                        // Limpa a linha da borda inferior antes de reescrever (evita sobras de dígitos antigos)
                        mvwhline(table_win, table_height - 1, 1, ACS_HLINE, table_width - 2);
                        mvwprintw(table_win, table_height - 1, 2,
                            _("ESC: Return | ↓/↑: Scroll | [S] Toggle Sort by Planet/Orb (%d-%d of %d)"),
                                primeiro_item, ultimo_item, paran_count);
                    } else {
                        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
                    }
                
                    wnoutrefresh(table_win);
                    break;

                case KEY_PPAGE:
                    offset_y -= (max_linhas_dados_visiveis / 2) * 2;
                    if (offset_y < 0) offset_y = 0;
                    if (paran_count > itens_visiveis) {
                        int primeiro_item = (offset_y / 2) + 1;
                        int ultimo_item   = (offset_y / 2) + itens_visiveis;
                        if (ultimo_item > paran_count) {
                            ultimo_item = paran_count;
                        }
                
                        // Limpa a linha da borda inferior antes de reescrever (evita sobras de dígitos antigos)
                        mvwhline(table_win, table_height - 1, 1, ACS_HLINE, table_width - 2);
                        mvwprintw(table_win, table_height - 1, 2,
                            _("ESC: Return | ↓/↑: Scroll | [S] Toggle Sort by Planet/Orb (%d-%d of %d)"),
                                primeiro_item, ultimo_item, paran_count);
                    } else {
                        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
                    }
                
                    wnoutrefresh(table_win);
                    break;

                case KEY_NPAGE:
                    offset_y += (max_linhas_dados_visiveis / 2) * 2;
                    if (offset_y > max_scroll_y) offset_y = max_scroll_y;
                    if (paran_count > itens_visiveis) {
                        int primeiro_item = (offset_y / 2) + 1;
                        int ultimo_item   = (offset_y / 2) + itens_visiveis;
                        if (ultimo_item > paran_count) {
                            ultimo_item = paran_count;
                        }
                
                        // Limpa a linha da borda inferior antes de reescrever (evita sobras de dígitos antigos)
                        mvwhline(table_win, table_height - 1, 1, ACS_HLINE, table_width - 2);
                        mvwprintw(table_win, table_height - 1, 2,
                            _("ESC: Return | ↓/↑: Scroll | [S] Toggle Sort by Planet/Orb (%d-%d of %d)"),
                                primeiro_item, ultimo_item, paran_count);
                    } else {
                        mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
                    }
                
                    wnoutrefresh(table_win);
                    break;

                case KEY_MOUSE: {
                    MEVENT event;
                    if (getmouse(&event) == OK) {
                        int linha_clique_janela = event.y - getbegy(table_win);
                        int col_clique_janela = event.x - getbegx(table_win);

                        int col_inicio_fechar = getmaxx(table_win) - 4;
                        int col_fim_fechar = col_inicio_fechar + 3;

                        if (linha_clique_janela == 0 && col_clique_janela >= col_inicio_fechar && col_clique_janela < col_fim_fechar) {
                            if (event.bstate & (BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED)) {
                                running = 0;
                                break;
                            }
                        }

                        int col_scrollbar_absoluta = getbegx(table_win) + (getmaxx(table_win) - 2);

                        if (event.x == col_scrollbar_absoluta) {
                            int linha_seta_cima = 4;
                            int offset_inicio_barra = 4;
                            int linha_seta_baixo = offset_inicio_barra + max_linhas_dados_visiveis - 1;

                            int max_scroll_y = row_pad - max_linhas_dados_visiveis;
                            if (max_scroll_y < 0) max_scroll_y = 0;

                            if (linha_clique_janela == linha_seta_cima) {
                                if (event.bstate & BUTTON1_PRESSED) {
                                    nodelay(table_win, TRUE);
                                    while (1) {
                                        if (offset_y > 1) {
                                            offset_y -= 2;

                                            int flag = 0;
                                            if (DARK_MODE) flag |= A_DIM | A_REVERSE;
                                            wattron(table_win, COLOR_PAIR(28) | flag);
                                            desenhar_scrollbar(table_win, offset_y, row_pad, max_linhas_dados_visiveis - 1, 4);
                                            wattroff(table_win, COLOR_PAIR(28) | flag);
                                            mvwaddstr(table_win, 4 + max_linhas_dados_visiveis - 1, table_width - 2, "▼");
                                            if (paran_count > itens_visiveis) {
                                                int primeiro_item = (offset_y / 2) + 1;
                                                int ultimo_item   = (offset_y / 2) + itens_visiveis;
                                                if (ultimo_item > paran_count) {
                                                    ultimo_item = paran_count;
                                                }
                                        
                                                // Limpa a linha da borda inferior antes de reescrever (evita sobras de dígitos antigos)
                                                mvwhline(table_win, table_height - 1, 1, ACS_HLINE, table_width - 2);
                                                mvwprintw(table_win, table_height - 1, 2,
                                                    _("ESC: Return | ↓/↑: Scroll | [S] Toggle Sort by Planet/Orb (%d-%d of %d)"),
                                                        primeiro_item, ultimo_item, paran_count);
                                            } else {
                                                mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
                                            }
                                        
                                            wnoutrefresh(table_win);

                                            wnoutrefresh(table_win);
                                            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 5, start_x + table_width - 3);
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
                            } else if (linha_clique_janela == linha_seta_baixo) {
                                if (event.bstate & BUTTON1_PRESSED) {
                                    nodelay(table_win, TRUE);

                                    while (1) {
                                        if (offset_y < max_scroll_y - 1) {

                                            offset_y += 2;
                                            int flag = 0;

                                            if (DARK_MODE) flag |= A_DIM | A_REVERSE;

                                            if (paran_count > itens_visiveis) {
                                                int primeiro_item = (offset_y / 2) + 1;
                                                int ultimo_item   = (offset_y / 2) + itens_visiveis;
                                                if (ultimo_item > paran_count) {
                                                    ultimo_item = paran_count;
                                                }
                                        
                                                // Limpa a linha da borda inferior antes de reescrever (evita sobras de dígitos antigos)
                                                mvwhline(table_win, table_height - 1, 1, ACS_HLINE, table_width - 2);
                                                mvwprintw(table_win, table_height - 1, 2,
                                                    _("ESC: Return | ↓/↑: Scroll | [S] Toggle Sort by Planet/Orb (%d-%d of %d)"),
                                                        primeiro_item, ultimo_item, paran_count);
                                            } else {
                                                mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
                                            }
                                        
                                            wnoutrefresh(table_win);

                                            wattron(table_win, COLOR_PAIR(28) | flag);
                                            desenhar_scrollbar(table_win, offset_y, row_pad, max_linhas_dados_visiveis - 1, 4);
                                            mvwaddstr(table_win, 4 + max_linhas_dados_visiveis - 1, table_width - 2, "▼");
                                            wattroff(table_win, COLOR_PAIR(28) | flag);
                                            wnoutrefresh(table_win);
                                            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 5, start_x + table_width - 3);
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
                            } else if (event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED | REPORT_MOUSE_POSITION)) {
                                nodelay(table_win, TRUE);
                                int linha_trilho_atual = 0;
                                if (max_scroll_y > 0 && (max_linhas_dados_visiveis - 1) > 0) {
                                    linha_trilho_atual = (offset_y * (max_linhas_dados_visiveis - 1)) / max_scroll_y;
                                }
                                int linha_trilho_janela = linha_trilho_atual + offset_inicio_barra;
                                int modo_arrasto_trilho = (linha_clique_janela == linha_trilho_janela);
                                int linha_inicial_mouse = linha_clique_janela;
                                int pad_line_pos_original = offset_y;
                                while (1) {
                                    int novo_offset = offset_y;
                                    int linha_clique_barra = linha_clique_janela - offset_inicio_barra;
                                    if (modo_arrasto_trilho) {
                                        int delta_mouse = linha_clique_janela - linha_inicial_mouse;
                                        if ((max_linhas_dados_visiveis - 1) > 0 && max_scroll_y > 0) {
                                            int delta_scroll = (delta_mouse * max_scroll_y) / (max_linhas_dados_visiveis - 1);
                                            novo_offset = pad_line_pos_original + delta_scroll;
                                        }
                                    } else {
                                        if (linha_clique_barra >= 0 && linha_clique_barra < max_linhas_dados_visiveis) {
                                            if ((max_linhas_dados_visiveis - 1) > 0 && max_scroll_y > 0) {
                                                novo_offset = (linha_clique_barra * max_scroll_y) / (max_linhas_dados_visiveis - 1);
                                            }
                                        }
                                    }
                                    novo_offset = (novo_offset / 2) * 2;
                                    if (novo_offset < 0) novo_offset = 0;
                                    if (novo_offset > max_scroll_y) novo_offset = max_scroll_y;
                                    if (offset_y != novo_offset) {
                                        offset_y = novo_offset;
                                        int flag = 0;
                                        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
                                        wattron(table_win, COLOR_PAIR(28) | flag);
                                        desenhar_scrollbar(table_win, offset_y, row_pad, max_linhas_dados_visiveis - 1, 4);
                                        wattroff(table_win, COLOR_PAIR(28) | flag);
                                        mvwaddstr(table_win, 4 + max_linhas_dados_visiveis - 1, table_width - 2, "▼");

                                        if (paran_count > itens_visiveis) {
                                            int primeiro_item = (offset_y / 2) + 1;
                                            int ultimo_item   = (offset_y / 2) + itens_visiveis;
                                            if (ultimo_item > paran_count) {
                                                ultimo_item = paran_count;
                                            }
                                    
                                            // Limpa a linha da borda inferior antes de reescrever (evita sobras de dígitos antigos)
                                            mvwhline(table_win, table_height - 1, 1, ACS_HLINE, table_width - 2);
                                            mvwprintw(table_win, table_height - 1, 2,
                                                _("ESC: Return | ↓/↑: Scroll | [S] Toggle Sort by Planet/Orb (%d-%d of %d)"),
                                                    primeiro_item, ultimo_item, paran_count);
                                        } else {
                                            mvwprintw(table_win, table_height - 1, 2, _("Press ESC to return to chart"));
                                        }
                                    
                                        wnoutrefresh(table_win);
                                        prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 5, start_x + table_width - 3);
                                        doupdate();
                                    }
                                    if ((event.bstate & BUTTON1_CLICKED) && !modo_arrasto_trilho) {
                                        break;
                                    }
                                    napms(10);
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
            if (running && !need_resort) {
                int flag = 0;
                if (DARK_MODE) flag |= A_DIM | A_REVERSE;
                wattron(table_win, COLOR_PAIR(28) | flag);
                desenhar_scrollbar(table_win, offset_y, row_pad, max_linhas_dados_visiveis - 1, 4);
                wattroff(table_win, COLOR_PAIR(28) | flag);
                mvwaddstr(table_win, 4 + max_linhas_dados_visiveis - 1, table_width - 2, "▼");
                wnoutrefresh(table_win);
                prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 5, start_x + table_width - 3);
                doupdate();
            }
        }
    }
    delwin(scroll_pad);
    delwin(shadow_win);
    delwin(table_win);
    drag_and_drop_off();
    touchwin(stdscr);
    refresh();
}



HeliacalInfo get_star_heliacal_info(double tjd_ut, double geo_lon, double geo_lat, double geo_alt, const CalculatedStar *star) {
    HeliacalInfo info = {HELIACAL_UNKNOWN, 0.0, 0.0, 0.0};

    // Nebulosas/aglomerados não têm modelo pontual na swe_heliacal_ut (evita varredura de 1 ano)
    // if (star->meta->categories & CAT_NEBULA) {
    //     info.state = HELIACAL_UNKNOWN;
    //     return info;
    // }

    double geopos[3] = {geo_lon, geo_lat, geo_alt};
    double datm[4]   = {1013.25, 15.0, 40.0, 0.0};
    double dobs[6]   = {36.0, 1.0, 0.0, 0.0, 0.0, 0.0};

    double dret[50];
    double xx_sun[6];
    char star_buf[SE_MAX_STNAME];
    char serr[AS_MAXCH];
    int helflag = SE_HELFLAG_AV;

    // 1. Calcula a longitude atual do Sol (instantâneo) para estimar o ponto exato do ciclo anual
    if (swe_calc_ut(tjd_ut, SE_SUN, SEFLG_SWIEPH | SEFLG_TOPOCTR, xx_sun, serr) < 0) {
        return info;
    }
    double sun_lon = xx_sun[0];

    // Diferença angular positiva (0° a 360°): quantos graus o Sol já andou à frente da estrela
    double deg_after_conj = swe_degnorm(sun_lon - star->longitude);
    double days_since_conj = deg_after_conj / 0.985647; // Velocidade média solar (~0.9856°/dia)

    // Data aproximada da última conjunção Sol-Estrela
    double tjd_last_conj = tjd_ut - days_since_conj;

    // Prepara o nome da estrela (usa o nome principal; se falhar, usa Bayer)
    snprintf(star_buf, sizeof(star_buf), "%s", star->meta->swe_name);

    // 2. Busca o ÚLTIMO Nascer Helíaco (ocorre logo após a conjunção: iniciamos 5 dias antes da conjunção)
    double search_rise_start = tjd_last_conj - 5.0;
    int ret_rise = swe_heliacal_ut(
        search_rise_start, geopos, datm, dobs, star_buf,
        SE_HELIACAL_RISING, helflag, dret, serr
    );

    if (ret_rise < 0 && star->meta->bayer != NULL) {
        snprintf(star_buf, sizeof(star_buf), ",%s", star->meta->bayer);
        ret_rise = swe_heliacal_ut(
            search_rise_start, geopos, datm, dobs, star_buf,
            SE_HELIACAL_RISING, helflag, dret, serr
        );
    }

    if (ret_rise < 0) {
        // Estrela circumpolar nesta latitude (nunca sofre ocultação solar)
        info.state = HELIACAL_CIRCUMPOLAR;
        return info;
    }

    // Se o Nascer Helíaco encontrado ainda está no futuro (ou seja, o Sol acabou de passar pela estrela
    // e ela ainda não reapareceu este ano), recuamos 1 ciclo anual (365.25 dias) para pegar o anterior
    if (dret[0] > tjd_ut) {
        info.next_rising_tjd = dret[0];
        if (swe_heliacal_ut(search_rise_start - 365.25, geopos, datm, dobs, star_buf,
                            SE_HELIACAL_RISING, helflag, dret, serr) >= 0) {
            info.last_rising_tjd = dret[0];
        }
    } else {
        info.last_rising_tjd = dret[0];
        // Calcula o próximo Nascer Helíaco (~1 ano depois) para exibir na coluna Next Rising
        if (swe_heliacal_ut(dret[0] + 330.0, geopos, datm, dobs, star_buf,
                            SE_HELIACAL_RISING, helflag, dret, serr) >= 0) {
            info.next_rising_tjd = dret[0];
        }
    }

    // 3. Busca o ÚLTIMO Pôr Helíaco (ocorre antes da conjunção: iniciamos 65 dias antes da conjunção)
    double search_set_start = tjd_last_conj - 65.0;
    int ret_set = swe_heliacal_ut(
        search_set_start, geopos, datm, dobs, star_buf,
        SE_HELIACAL_SETTING, helflag, dret, serr
    );

    if (ret_set >= 0) {
        // Se o Pôr Helíaco encontrado está no futuro (caso raro em altas latitudes), recua 1 ano
        if (dret[0] > tjd_ut) {
            if (swe_heliacal_ut(search_set_start - 365.25, geopos, datm, dobs, star_buf,
                                SE_HELIACAL_SETTING, helflag, dret, serr) >= 0) {
                info.last_setting_tjd = dret[0];
            }
        } else {
            info.last_setting_tjd = dret[0];
        }
    }

    // 4. Determina o estado comparando qual evento ocorreu por último antes de tjd_ut
    if (info.last_rising_tjd > 0.0 && info.last_setting_tjd > 0.0) {
        if (info.last_rising_tjd > info.last_setting_tjd) {
            info.state = HELIACAL_VISIBLE;
        } else {
            info.state = HELIACAL_HIDDEN;
        }
    }

    return info;
}




// Calcula o estado helíaco de uma estrela fixa na data natal tjd_ut
HeliacalInfo get_star_heliacal_info_original(double tjd_ut, double geo_lon, double geo_lat, double geo_alt, const TraditionalStar *star_meta) {
    HeliacalInfo info = {HELIACAL_UNKNOWN, 0.0, 0.0, 0.0};

    double geopos[3] = {geo_lon, geo_lat, geo_alt};
    // Atmosfera padrão: 1013.25 mbar, 15°C, 40% umidade, 0 (usa coeficiente padrão)
    double datm[4] = {1013.25, 15.0, 40.0, 0.0};
    // Observador padrão: idade 36, acuidade 1.0, demais zeros para usar default da Swiss Ephemeris
    double dobs[6] = {36.0, 1.0, 0.0, 0.0, 0.0, 0.0};

    double dret[50];
    char star_buf[SE_MAX_STNAME];
    char serr[AS_MAXCH];

    
    // Inicia a busca 1 ano (366 dias) antes do nascimento para capturar o ciclo anual completo
    double tjd_search_start = tjd_ut - 366.0;
    int32 helflag = SE_HELFLAG_AV; // Usa parâmetros médios padrão

    // 1. Busca o Nascer Helíaco no ano anterior ao nascimento
    snprintf(star_buf, sizeof(star_buf), "%s", star_meta->swe_name);
    int ret_rise = swe_heliacal_ut(
        tjd_search_start, geopos, datm, dobs, star_buf,
        SE_HELIACAL_RISING, helflag, dret, serr
    );

    // Fallback Bayer se necessário
    if (ret_rise < 0 && star_meta->bayer != NULL) {
        snprintf(star_buf, sizeof(star_buf), ",%s", star_meta->bayer);
        ret_rise = swe_heliacal_ut(
            tjd_search_start, geopos, datm, dobs, star_buf,
            SE_HELIACAL_RISING, helflag, dret, serr
        );
    }

    if (ret_rise < 0) {
        // Na Swiss Ephemeris, erro em estrela brilhante geralmente indica estrela circumpolar
        // (que nunca sofre ocultação helíaca nesta latitude) ou estrela abaixo do horizonte local
        info.state = HELIACAL_CIRCUMPOLAR;
        return info;
    }

    double rise1 = dret[0];
    // Verifica se houve outro Nascer Helíaco mais próximo antes do nascimento
    int ret_rise2 = swe_heliacal_ut(
        rise1 + 10.0, geopos, datm, dobs, star_buf,
        SE_HELIACAL_RISING, helflag, dret, serr
    );
    if (ret_rise2 >= 0 && dret[0] <= tjd_ut) {
        info.last_rising_tjd = dret[0];
        // Busca o próximo após o nascimento
        if (swe_heliacal_ut(dret[0] + 10.0, geopos, datm, dobs, star_buf,
                            SE_HELIACAL_RISING, helflag, dret, serr) >= 0) {
            info.next_rising_tjd = dret[0];
        }
    } else {
        info.last_rising_tjd = rise1;
        info.next_rising_tjd = (ret_rise2 >= 0) ? dret[0] : 0.0;
    }

    // 2. Busca o Pôr Helíaco no ano anterior ao nascimento
    int ret_set = swe_heliacal_ut(
        tjd_search_start, geopos, datm, dobs, star_buf,
        SE_HELIACAL_SETTING, helflag, dret, serr
    );
    if (ret_set >= 0) {
        double set1 = dret[0];
        int ret_set2 = swe_heliacal_ut(
            set1 + 10.0, geopos, datm, dobs, star_buf,
            SE_HELIACAL_SETTING, helflag, dret, serr
        );
        if (ret_set2 >= 0 && dret[0] <= tjd_ut) {
            info.last_setting_tjd = dret[0];
        } else {
            info.last_setting_tjd = set1;
        }
    }

    // 3. Compara qual evento ocorreu por último antes do nascimento
    if (info.last_rising_tjd > 0.0 && info.last_setting_tjd > 0.0) {
        if (info.last_rising_tjd > info.last_setting_tjd) {
            info.state = HELIACAL_VISIBLE;
        } else {
            info.state = HELIACAL_HIDDEN;
        }
    }

    return info;
}





// Formata TJD (UT + tz_offset) em string "DD/MM/YYYY" (ou "-" se não aplicável)
static void format_tjd_date(double tjd_ut, double tz_offset, char *buf, size_t buf_size) {
    if (tjd_ut <= 0.0) {
        snprintf(buf, buf_size, "%s", "-");
        return;
    }
    int y, m, d;
    double h;
    swe_revjul(tjd_ut + (tz_offset / 24.0), SE_GREG_CAL, &y, &m, &d, &h);
    snprintf(buf, buf_size, "%02d/%02d/%04d", d, m, y);
}

// Comparador 1: Ordena pela proximidade do último Nascer Helíaco (Estrela Helíaca Natal no topo!)
static int compare_heliacal_by_recent_rising(const void *a, const void *b) {
    const HeliacalRow *ha = (const HeliacalRow *)a;
    const HeliacalRow *hb = (const HeliacalRow *)b;

    // Prioriza estrelas visíveis com Nascer Helíaco recente no topo
    int a_valid = (ha->info.state == HELIACAL_VISIBLE && ha->days_since_rising >= 0.0);
    int b_valid = (hb->info.state == HELIACAL_VISIBLE && hb->days_since_rising >= 0.0);

    if (a_valid && !b_valid) return -1;
    if (!a_valid && b_valid) return 1;

    if (a_valid && b_valid) {
        if (ha->days_since_rising < hb->days_since_rising) return -1;
        if (ha->days_since_rising > hb->days_since_rising) return 1;
    }

    // Demais estrelas (Sob os Raios / Circumpolares) ordenadas pela longitude zodiacal
    if (ha->longitude < hb->longitude) return -1;
    if (ha->longitude > hb->longitude) return 1;
    return 0;
}

// Comparador 2: Ordena pela Longitude Eclíptica (ordem zodiacal padrão)
static int compare_heliacal_by_longitude(const void *a, const void *b) {
    const HeliacalRow *ha = (const HeliacalRow *)a;
    const HeliacalRow *hb = (const HeliacalRow *)b;

    if (ha->longitude < hb->longitude) return -1;
    if (ha->longitude > hb->longitude) return 1;
    return 0;
}

static void draw_heliacal_header(WINDOW *table_win, int table_width, int total_stars, const char *sort_label) {
    mvwhline(table_win, 0, 1, ACS_HLINE, table_width - 2);

    int col_fechar = table_width - 4;
    wattron(table_win, COLOR_PAIR(13));
    mvwprintw(table_win, 0, col_fechar, "[");
    mvwprintw(table_win, 0, col_fechar + 2, "]");
    wattroff(table_win, COLOR_PAIR(13));

    wattron(table_win, COLOR_PAIR(13) | A_BOLD);
    mvwprintw(table_win, 0, col_fechar + 1, "✖");
    wattroff(table_win, COLOR_PAIR(13) | A_BOLD);

    char title[160];
    snprintf(title, sizeof(title),
             _(" Heliacal Phases & Visibility (Stars: %d | Sorted by: %s) "),
             total_stars, sort_label);

    int title_x = (table_width - (int)strlen(title)) / 2;
    if (title_x < 2) title_x = 2;

    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 0, title_x, "%s", title);
    wattroff(table_win, A_BOLD);
}

static void draw_heliacal_footer(WINDOW *table_win, int table_height, int table_width,
                                 int offset_y, int itens_visiveis, int total_rows) {
    mvwhline(table_win, table_height - 1, 1, ACS_HLINE, table_width - 2);

    if (total_rows > itens_visiveis) {
        int primeiro = (offset_y / 2) + 1;
        int ultimo   = (offset_y / 2) + itens_visiveis;
        if (ultimo > total_rows) ultimo = total_rows;

        mvwprintw(table_win, table_height - 1, 2,
                  _("ESC: Return | ↓/↑: Scroll | [S] Sort criteria (%d-%d of %d)"),
                  primeiro, ultimo, total_rows);
    } else {
        mvwprintw(table_win, table_height - 1, 2, _("ESC: Return | [S] Sort criteria"));
    }
}



void display_heliacal_phases_table(double tjd_ut, double geo_lon, double geo_lat, double geo_alt, double tz_offset, const CalculatedStar *stars, int star_count) {
    // 1. Prepara os dados helíacos das estrelas pontuais (exclui nebulosas difusas)
    HeliacalRow rows[TRADITIONAL_STARS_COUNT];
    int row_count = 0;

    for (int i = 0; i < star_count && row_count < (int)TRADITIONAL_STARS_COUNT; i++) {
        // if (stars[i].meta->categories & CAT_NEBULA) {
        //     continue; // Nebulosas não têm fase helíaca pontual
        // }

        HeliacalInfo hinfo = get_star_heliacal_info(
            tjd_ut, geo_lon, geo_lat, geo_alt, &stars[i]
        );

        rows[row_count].meta      = stars[i].meta;
        rows[row_count].longitude = stars[i].longitude;
        rows[row_count].magnitude = stars[i].magnitude;
        rows[row_count].info      = hinfo;

        if (hinfo.last_rising_tjd > 0.0 && tjd_ut >= hinfo.last_rising_tjd) {
            rows[row_count].days_since_rising = tjd_ut - hinfo.last_rising_tjd;
        } else {
            rows[row_count].days_since_rising = -1.0;
        }

        row_count++;
    }

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int table_height = 24;
    if (table_height > max_y - 2) table_height = max_y - 2;
    int table_width = max_x - 5;

    int start_y = (max_y - table_height) / 2;
    int start_x = (max_x - table_width) / 2;

    WINDOW *table_win  = newwin(table_height, table_width, start_y, start_x);
    WINDOW *shadow_win = newwin(table_height, table_width, start_y + 1, start_x + 1);

    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);

    box(table_win, 0, 0);
    wbkgd(table_win, COLOR_PAIR(6) | FLAGS);

    // Cabeçalho fixo das colunas
    wattron(table_win, A_BOLD);
    mvwprintw(table_win, 2, 4,   "%-20s", _("Fixed Star"));
    mvwprintw(table_win, 2, 26,  "%-10s", _("Position"));
    mvwprintw(table_win, 2, 39,  "%-10s", _("Nature"));
    mvwprintw(table_win, 2, 55,  "%-14s", _("Status"));
    mvwprintw(table_win, 2, 71,  "%-14s", _("Last Rising"));
    mvwprintw(table_win, 2, 87,  "%-14s", _("Last Setting"));
    mvwprintw(table_win, 2, 103,  "%-14s", _("Next Rising"));
    mvwprintw(table_win, 2, 119, "%-14s", _("Days Active"));
    mvwhline(table_win, 3, 3, ACS_HLINE, table_width - 6);
    wattroff(table_win, A_BOLD);

    int max_linhas_dados_visiveis = table_height - 8;
    if (max_linhas_dados_visiveis < 2) max_linhas_dados_visiveis = 2;

    int total_pad_rows = (row_count > 0) ? (row_count * 2) : max_linhas_dados_visiveis;
    if (total_pad_rows < max_linhas_dados_visiveis) {
        total_pad_rows = max_linhas_dados_visiveis;
    }

    int pad_width = table_width - 5;
    WINDOW *scroll_pad = newpad(total_pad_rows, pad_width);
    wbkgd(scroll_pad, COLOR_PAIR(8) | FLAGS);

    int sort_mode = 0; // 0: Heliacal Proximity (Natal Rising Star first) | 1: Zodiacal Longitude
    int running = 1;

    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(100);
    drag_and_drop_on();

    while (running) {
        const char *sort_label = "";
        if (sort_mode == 0) {
            qsort(rows, row_count, sizeof(HeliacalRow), compare_heliacal_by_recent_rising);
            sort_label = _("Heliacal Rising");
        } else {
            qsort(rows, row_count, sizeof(HeliacalRow), compare_heliacal_by_longitude);
            sort_label = _("Longitude");
        }

        draw_heliacal_header(table_win, table_width, row_count, sort_label);
        werase(scroll_pad);

        int row_pad = 0;
        for (int idx = 0; idx < row_count; idx++) {
            char str_last_rise[16], str_last_set[16], str_next_rise[16], str_days[20];

            format_tjd_date(rows[idx].info.last_rising_tjd,  tz_offset, str_last_rise, sizeof(str_last_rise));
            format_tjd_date(rows[idx].info.last_setting_tjd, tz_offset, str_last_set,  sizeof(str_last_set));
            format_tjd_date(rows[idx].info.next_rising_tjd,  tz_offset, str_next_rise, sizeof(str_next_rise));

            const char *status_label = _("Visible");
            if (rows[idx].info.state == HELIACAL_HIDDEN) {
                status_label = _("Under Beams");
            } else if (rows[idx].info.state == HELIACAL_CIRCUMPOLAR) {
                status_label = _("Circumpolar");
            }

            if (rows[idx].info.state == HELIACAL_VISIBLE && rows[idx].days_since_rising >= 0.0) {
                snprintf(str_days, sizeof(str_days), _("%d days"), (int)round(rows[idx].days_since_rising));
            } else {
                snprintf(str_days, sizeof(str_days), "%s", "-");
            }

            // Destaca a Estrela do Nascer Helíaco Natal (idx == 0 no modo 0, ou nascida há <= 15 dias)
            int is_natal_rising = (rows[idx].info.state == HELIACAL_VISIBLE &&
                                   rows[idx].days_since_rising >= 0.0 &&
                                   rows[idx].days_since_rising <= 15.0);

            if (rows[idx].info.state == HELIACAL_HIDDEN) {
                wattron(scroll_pad, A_DIM);
            } else if (is_natal_rising) {
                wattron(scroll_pad, A_BOLD);
            }


            int degree = (int)fmod(rows[idx].longitude, 30.0);

            // 1. Pega apenas a fração decimal do grau
            double graus_decimais = rows[idx].longitude;
            double fracao_grau = graus_decimais - (int)graus_decimais;
    
            // 2. Calcula minutos decimais e extrai os segundos com arredondamento
            double minutos_decimais = fracao_grau * 60.0;
            int min = (int)minutos_decimais;
            
            double segundos_decimais = (minutos_decimais - min) * 60.0;
            int seg = (int)round(segundos_decimais);
    
            // 3. TRATAMENTO CRÍTICO DE ESTOURO (O Efeito Cascata de Arredondamento)
            if (seg >= 60) {
                seg = 0;
                min++;
                if (min >= 60) {
                    min = 0;
                    degree++;
                    
                    // TRAVA EXTRA DE BORDA DO SIGNO:
                    // Se o arredondamento jogou o planeta para 30°, nós seguramos ele em 29°59'59"
                    // para não exibir "30°" na tela e não dessincronizar com a string do signo natal.
                    if (degree >= 30) {
                        degree = 29;
                        min = 59;
                        seg = 59;
                    }
                }
            }

            mvwprintw(scroll_pad, row_pad, 2,   "%-20s", rows[idx].meta->display_name);
            mvwprintw(scroll_pad, row_pad, 24,  "%2d°%s%02d'%02d\"", degree, get_sign(rows[idx].longitude / 30), min, seg);

            mvwprintw(scroll_pad, row_pad, 37,  "%-10s", rows[idx].meta->nature);
            mvwprintw(scroll_pad, row_pad, 53,  "%-14s", status_label);
            mvwprintw(scroll_pad, row_pad, 69,  "%-14s", str_last_rise);
            mvwprintw(scroll_pad, row_pad, 85,  "%-14s", str_last_set);
            mvwprintw(scroll_pad, row_pad, 101, "%-14s", str_next_rise);
            mvwprintw(scroll_pad, row_pad, 117, "%-14s", str_days);

            wattroff(scroll_pad, A_BOLD | A_DIM);

            wattron(scroll_pad, COLOR_PAIR(10) | A_DIM);
            mvwhline(scroll_pad, row_pad + 1, 1, ACS_HLINE, pad_width - 2);
            wattroff(scroll_pad, COLOR_PAIR(10) | A_DIM);

            row_pad += 2;
        }

        int row_count = row_pad / 2;

        int offset_y = 0;
        int max_scroll_y = row_pad - max_linhas_dados_visiveis;
        if (max_scroll_y < 0) max_scroll_y = 0;
        int itens_visiveis = max_linhas_dados_visiveis / 2;

        keypad(table_win, TRUE);
        nodelay(table_win, FALSE);

        mvwaddstr(table_win, 4, table_width - 2, "▲");
        mvwaddstr(table_win, 4 + max_linhas_dados_visiveis - 1, table_width - 2, "▼");

        int ch;
        int need_resort = 0;

        while (running && !need_resort) {
            int flag = 0;
            if (DARK_MODE) flag |= A_DIM | A_REVERSE;
            wattron(table_win, COLOR_PAIR(28) | flag);
            desenhar_scrollbar(table_win, offset_y, row_pad, max_linhas_dados_visiveis - 1, 4);
            wattroff(table_win, COLOR_PAIR(28) | flag);

            mvwaddstr(table_win, 4 + max_linhas_dados_visiveis - 1, table_width - 2, "▼");

            draw_heliacal_footer(table_win, table_height, table_width, offset_y, itens_visiveis, row_count);

            wnoutrefresh(table_win);
            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 5, start_x + table_width - 3);
            doupdate();

            ch = wgetch(table_win);

            if (ch == 27 || ch == 'q' || ch == 'Q') {
                running = 0;
                break;
            }

            switch (ch) {
                case 's':
                case 'S':
                    sort_mode = (sort_mode + 1) % 2;
                    need_resort = 1;
                    break;

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

                case KEY_PPAGE:
                    offset_y -= itens_visiveis * 2;
                    if (offset_y < 0) offset_y = 0;
                    break;

                case KEY_NPAGE:
                    offset_y += itens_visiveis * 2;
                    if (offset_y > max_scroll_y) offset_y = max_scroll_y;
                    break;

                case KEY_MOUSE: {
                    MEVENT event;
                    if (getmouse(&event) == OK) {
                        int linha_clique_janela = event.y - getbegy(table_win);
                        int col_clique_janela = event.x - getbegx(table_win);

                        int col_inicio_fechar = getmaxx(table_win) - 4;
                        int col_fim_fechar = col_inicio_fechar + 3;

                        if (linha_clique_janela == 0 && col_clique_janela >= col_inicio_fechar && col_clique_janela < col_fim_fechar) {
                            if (event.bstate & (BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED)) {
                                running = 0;
                                break;
                            }
                        }

                        int col_scrollbar_absoluta = getbegx(table_win) + (getmaxx(table_win) - 2);

                        if (event.x == col_scrollbar_absoluta) {
                            int linha_seta_cima = 4;
                            int offset_inicio_barra = 4;
                            int linha_seta_baixo = offset_inicio_barra + max_linhas_dados_visiveis - 1;

                            int max_scroll_y = row_pad - max_linhas_dados_visiveis;
                            if (max_scroll_y < 0) max_scroll_y = 0;

                            if (linha_clique_janela == linha_seta_cima) {
                                if (event.bstate & BUTTON1_PRESSED) {
                                    nodelay(table_win, TRUE);
                                    while (1) {
                                        if (offset_y > 1) {
                                            offset_y -= 2;

                                            int flag = 0;
                                            if (DARK_MODE) flag |= A_DIM | A_REVERSE;
                                            wattron(table_win, COLOR_PAIR(28) | flag);
                                            desenhar_scrollbar(table_win, offset_y, row_pad, max_linhas_dados_visiveis - 1, 4);
                                            wattroff(table_win, COLOR_PAIR(28) | flag);
                                            mvwaddstr(table_win, 4 + max_linhas_dados_visiveis - 1, table_width - 2, "▼");

                                            draw_heliacal_footer(table_win, table_height, table_width, offset_y, itens_visiveis, row_count);
                                        
                                            wnoutrefresh(table_win);

                                            wnoutrefresh(table_win);
                                            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 5, start_x + table_width - 3);
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
                            } else if (linha_clique_janela == linha_seta_baixo) {
                                if (event.bstate & BUTTON1_PRESSED) {
                                    nodelay(table_win, TRUE);

                                    while (1) {
                                        if (offset_y < max_scroll_y - 1) {

                                            offset_y += 2;
                                            int flag = 0;

                                            if (DARK_MODE) flag |= A_DIM | A_REVERSE;

                                            draw_heliacal_footer(table_win, table_height, table_width, offset_y, itens_visiveis, row_count);
                                        
                                            wnoutrefresh(table_win);

                                            wattron(table_win, COLOR_PAIR(28) | flag);
                                            desenhar_scrollbar(table_win, offset_y, row_pad, max_linhas_dados_visiveis - 1, 4);
                                            mvwaddstr(table_win, 4 + max_linhas_dados_visiveis - 1, table_width - 2, "▼");
                                            wattroff(table_win, COLOR_PAIR(28) | flag);
                                            wnoutrefresh(table_win);
                                            prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 5, start_x + table_width - 3);
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
                            } else if (event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED | REPORT_MOUSE_POSITION)) {
                                nodelay(table_win, TRUE);
                                int linha_trilho_atual = 0;
                                if (max_scroll_y > 0 && (max_linhas_dados_visiveis - 1) > 0) {
                                    linha_trilho_atual = (offset_y * (max_linhas_dados_visiveis - 1)) / max_scroll_y;
                                }
                                int linha_trilho_janela = linha_trilho_atual + offset_inicio_barra;
                                int modo_arrasto_trilho = (linha_clique_janela == linha_trilho_janela);
                                int linha_inicial_mouse = linha_clique_janela;
                                int pad_line_pos_original = offset_y;
                                while (1) {
                                    int novo_offset = offset_y;
                                    int linha_clique_barra = linha_clique_janela - offset_inicio_barra;
                                    if (modo_arrasto_trilho) {
                                        int delta_mouse = linha_clique_janela - linha_inicial_mouse;
                                        if ((max_linhas_dados_visiveis - 1) > 0 && max_scroll_y > 0) {
                                            int delta_scroll = (delta_mouse * max_scroll_y) / (max_linhas_dados_visiveis - 1);
                                            novo_offset = pad_line_pos_original + delta_scroll;
                                        }
                                    } else {
                                        if (linha_clique_barra >= 0 && linha_clique_barra < max_linhas_dados_visiveis) {
                                            if ((max_linhas_dados_visiveis - 1) > 0 && max_scroll_y > 0) {
                                                novo_offset = (linha_clique_barra * max_scroll_y) / (max_linhas_dados_visiveis - 1);
                                            }
                                        }
                                    }
                                    novo_offset = (novo_offset / 2) * 2;
                                    if (novo_offset < 0) novo_offset = 0;
                                    if (novo_offset > max_scroll_y) novo_offset = max_scroll_y;
                                    if (offset_y != novo_offset) {
                                        offset_y = novo_offset;
                                        int flag = 0;
                                        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
                                        wattron(table_win, COLOR_PAIR(28) | flag);
                                        desenhar_scrollbar(table_win, offset_y, row_pad, max_linhas_dados_visiveis - 1, 4);
                                        wattroff(table_win, COLOR_PAIR(28) | flag);
                                        mvwaddstr(table_win, 4 + max_linhas_dados_visiveis - 1, table_width - 2, "▼");

                                        draw_heliacal_footer(table_win, table_height, table_width, offset_y, itens_visiveis, row_count);

                                        wnoutrefresh(table_win);
                                        prefresh(scroll_pad, offset_y, 0, start_y + 4, start_x + 2, start_y + table_height - 5, start_x + table_width - 3);
                                        doupdate();
                                    }
                                    if ((event.bstate & BUTTON1_CLICKED) && !modo_arrasto_trilho) {
                                        break;
                                    }
                                    napms(10);
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
    }

    delwin(scroll_pad);
    delwin(shadow_win);
    delwin(table_win);

    drag_and_drop_off();
    touchwin(stdscr);
    refresh();
}






static const char *get_temperament_meaning(const char *nature) {
    if (strcmp(nature, "Mar/Jup") == 0 || strcmp(nature, "Jup/Mar") == 0) {
        return _("Combines martial courage and initiative with Jupiterian nobility, leadership, and expansion.");
    }
    if (strcmp(nature, "Sat/Jup") == 0 || strcmp(nature, "Jup/Sat") == 0) {
        return _("Blends Saturnian endurance, gravity, and structure with Jupiterian authority and prudence.");
    }
    if (strcmp(nature, "Mar/Mer") == 0 || strcmp(nature, "Mer/Mar") == 0) {
        return _("Unites sharp intellect, speed, and incisive speech with boldness, strategy, and executive drive.");
    }
    if (strcmp(nature, "Ven/Mer") == 0 || strcmp(nature, "Mer/Ven") == 0) {
        return _("Harmonizes artistic grace, charm, and social refinement with eloquence, wit, and versatility.");
    }
    if (strcmp(nature, "Ven/Mar") == 0 || strcmp(nature, "Mar/Ven") == 0) {
        return _("Fuses aesthetic magnetism and desire with passion, creative drive, and strong interpersonal energy.");
    }
    if (strcmp(nature, "Sat/Ven") == 0 || strcmp(nature, "Ven/Sat") == 0) {
        return _("Tempers affection and artistic taste with loyalty, restraint, realism, and lasting commitment.");
    }
    if (strcmp(nature, "Sat/Mar") == 0 || strcmp(nature, "Mar/Sat") == 0) {
        return _("Concentrates disciplined force, resilience under pressure, and austerity; requires patience to avoid harshness.");
    }
    if (strcmp(nature, "Sat/Mer") == 0 || strcmp(nature, "Mer/Sat") == 0) {
        return _("Gives deep concentration, analytical rigor, skepticism, and serious scholarly or technical focus.");
    }
    if (strcmp(nature, "Moo/Mar") == 0 || strcmp(nature, "Mar/Moo") == 0) {
        return _("Heightens emotional intensity, instinctual courage, and sensitivity, requiring emotional balance.");
    }
    if (strcmp(nature, "Moo/Sat") == 0 || strcmp(nature, "Sat/Moo") == 0) {
        return _("Brings emotional sobriety, historical memory, endurance, and introspective depth.");
    }
    if (strcmp(nature, "Sun/Mar") == 0) {
        return _("Radiates solar vitality, command, and fierce willpower, demanding self-control to avoid excess.");
    }
    if (strcmp(nature, "Mar") == 0) {
        return _("Pure martial temperament: courage, decisiveness, conquest, and direct action.");
    }
    if (strcmp(nature, "Ven") == 0) {
        return _("Pure Venusian temperament: harmony, grace, beauty, and relational attraction.");
    }
    if (strcmp(nature, "Mer") == 0) {
        return _("Pure Mercurial temperament: agility of mind, communication, skill, and adaptability.");
    }
    if (strcmp(nature, "Jup") == 0) {
        return _("Pure Jupiterian temperament: wisdom, generosity, protection, and dignified elevation.");
    }
    if (strcmp(nature, "Sat") == 0) {
        return _("Pure Saturnian temperament: gravity, perseverance, detachment, and structural mastery.");
    }
    return _("Classical planetary temperament governing how the star channels its influence.");
}



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
) {
    int p_max_y, p_max_x;
    getmaxyx(stdscr, p_max_y, p_max_x);

    int i_height = p_max_y - 6;
    if (i_height > 26) i_height = 26;
    int i_width = p_max_x - 12;
    if (i_width > 102) i_width = 102;

    int i_start_y = (p_max_y - i_height) / 2;
    int i_start_x = (p_max_x - i_width) / 2;

    WINDOW *shadow_win = newwin(i_height, i_width, i_start_y + 1, i_start_x + 1);
    werase(shadow_win);
    wattron(shadow_win, COLOR_PAIR(9));
    box(shadow_win, 0, 0);
    wattroff(shadow_win, COLOR_PAIR(9));
    wnoutrefresh(shadow_win);

    WINDOW *border_win = newwin(i_height, i_width, i_start_y, i_start_x);
    wbkgd(border_win, COLOR_PAIR(13) | FLAGS);
    box(border_win, 0, 0);
    wattron(border_win, A_BOLD);
    const char *title = is_solar_return
    ? _(" Solar Return Fixed Stars & Delineation ")
    : _(" Radical Fixed Stars & Delineation ");
    mvwprintw(border_win, 0, (i_width - get_visual_width(title)) / 2, "%s", title);


    // Botão [✖] no canto superior direito
    int col_fechar = getmaxx(border_win) - 4;
    wattron(border_win, COLOR_PAIR(13));
    mvwprintw(border_win, 0, col_fechar, "[");
    mvwprintw(border_win, 0, col_fechar + 2, "]");
    wattroff(border_win, COLOR_PAIR(13));

    wattron(border_win, COLOR_PAIR(13) | A_BOLD);
    mvwprintw(border_win, 0, col_fechar + 1, "✖");
    wattroff(border_win, COLOR_PAIR(13) | A_BOLD);

    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(100);
    drag_and_drop_on();

    mvwprintw(border_win, i_height - 1, (i_width - 44) / 2, _(" [↓↑|JK: Scroll | Q|ESC: Return to Chart] "));
    wnoutrefresh(border_win);
    doupdate();

    int pad_cols = i_width - 6;

    // 1. Conta quantas estrelas estão ativas neste mapa
    int active_stars_count = 0;
    for (int s = 0; s < star_count; s++) {
        if (star_active[s]) {
            active_stars_count++;
        }
    }

    // 2. Calcula a altura exata necessária com folga de segurança:
    // - 45 linhas fixas para cabeçalho inicial, introdução, Monitor Crítico do Hyleg e rodapé
    // - 30 linhas por estrela ativa (média real ~21 linhas + margem de segurança para múltiplos contatos)
    int pad_lines = 45 + (active_stars_count * 30);

    // Garante que o pad tenha no mínimo a altura da janela visível
    if (pad_lines < i_height) {
        pad_lines = i_height;
    }

    WINDOW *pad = newpad(pad_lines, pad_cols);

    wbkgd(pad, COLOR_PAIR(13) | FLAGS);
    keypad(pad, TRUE);
    idlok(pad, TRUE);
    scrollok(pad, TRUE);

    int line_count = 0;
    char str[512] = "";

    wprintw(pad, "\n");
    wattron(pad, A_BOLD | COLOR_PAIR(15));
    if (is_solar_return) {
        wprintw(pad, _("FIXED STARS ACTIVATED IN THE SOLAR RETURN\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(15));
        wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
        line_count += 2;
        line_count += print_split_lines(
            pad,
            _("Below is the classical delineation of the fixed stars anchored by conjunction, parallel, "
              "or local paran in your Solar Return chart, governing prominent themes for this annual cycle.\n\n"),
            MAX_LINE_WIDTH
        );
    } else {
        wprintw(pad, _("FIXED STARS ACTIVATED IN THE RADIX BLUEPRINT\n"));
        wattroff(pad, A_BOLD | COLOR_PAIR(15));
        wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
        line_count += 2;
        line_count += print_split_lines(
            pad,
            _("In traditional astrology, fixed stars do not cast rays like wandering planets; they act through "
              "bodily conjunction, equatorial parallel, or local horizon/meridian parans. Below is the "
              "delineation of the stars anchored in your natal chart.\n\n"),
            MAX_LINE_WIDTH
        );
    }
    

    /* ======================================================================
       SEÇÃO 1: MONITOR CRÍTICO — ESTRELAS FIXAS SOBRE O HYLEG NATAL
       ====================================================================== */
    if (!is_solar_return) {
       if (lon_hyleg_natal >= 0.0 && nome_hyleg_natal != NULL) {
            wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
            wattron(pad, A_BOLD | COLOR_PAIR(11) | A_REVERSE);
            line_count++;

            snprintf(str, sizeof(str), _("[CRITICAL MONITOR] FIXED STARS CONJUNCT NATAL HYLEG (%s %s)\n"),
                    glifo_hyleg_natal ? glifo_hyleg_natal : "", nome_hyleg_natal);
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

            wattroff(pad, A_BOLD | COLOR_PAIR(11) | A_REVERSE);
            wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
            line_count++;

            bool hyleg_star_found = false;
            for (int s = 0; s < star_count; s++) {
                double diff = fabs(swe_difdeg2n(stars[s].longitude, lon_hyleg_natal));
                if (diff <= stars[s].meta->default_orb + DBL_EPSILON) {
                    hyleg_star_found = true;

                    if ((stars[s].meta->categories & CAT_NEBULA) ||
                        strcmp(stars[s].meta->swe_name, "Algol") == 0 ||
                        strcmp(stars[s].meta->swe_name, "Scheat") == 0) {
                        wattron(pad, A_BOLD | COLOR_PAIR(11));
                    } else {
                        wattron(pad, A_BOLD | COLOR_PAIR(32));
                    }

                    snprintf(str, sizeof(str), _("[*] %s (%s) IS CONJUNCT YOUR NATAL HYLEG (Orb: %.2f°)\n"),
                            stars[s].meta->display_name, stars[s].meta->nature, diff);
                    line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);
                    wattroff(pad, A_BOLD | COLOR_PAIR(11) | COLOR_PAIR(32));

                    if (stars[s].meta->delineation != NULL) {
                        line_count += print_split_lines(pad, _(stars[s].meta->delineation), MAX_LINE_WIDTH);
                        wprintw(pad, "\n");
                        line_count++;
                    }
                }
            }

            if (!hyleg_star_found) {
                line_count += print_split_lines(
                    pad,
                    _("No fixed stars from the classical catalog fall in direct bodily conjunction with your Natal Hyleg.\n\n"),
                    MAX_LINE_WIDTH
                );
            }
        }
    }

    /* ======================================================================
       SEÇÃO 2: DOSSIÊ INDIVIDUAL DAS ESTRELAS ATIVADAS NO MAPA RADICAL
       ====================================================================== */
    int active_found = 0;

    for (int s = 0; s < star_count; s++) {
        if (!star_active[s]) {
            continue;
        }
        active_found++;

        if (!stars[s].heliacal_calculated) {
            HeliacalInfo hinfo = get_star_heliacal_info(
                tjd_ut, lon, lat, elev, &stars[s]
            );
            stars[s].heliacal_state = hinfo.state;
            stars[s].heliacal_calculated = true;
        }

        // Cabeçalho reverso idêntico ao padrão dos planetas
        int header_color = COLOR_PAIR(32);
        const char *badge = _("CLASSICAL");
        if (stars[s].meta->categories & CAT_ROYAL) {
            header_color = COLOR_PAIR(25);
            badge = _("ROYAL STAR");
        } else if (strcmp(stars[s].meta->swe_name, "Algol") == 0 || 
                   (stars[s].meta->categories & CAT_NEBULA)) {
            header_color = COLOR_PAIR(11);
            badge = _("CRITICAL / NEBULA");
        } else if (stars[s].meta->categories & CAT_BEHENIAN) {
            badge = _("BEHENIAN");
        }

        wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
        wattron(pad, A_BOLD | header_color | A_REVERSE);
        char str_aux[128];
        snprintf(str_aux, sizeof(str_aux), _("    ★  %s (%s) — %s | Nature: %s | Mag: %.2f"),
                 stars[s].meta->display_name,
                 stars[s].meta->object,
                 badge,
                 stars[s].meta->nature,
                 stars[s].magnitude);
        wprintw(pad, "%s%*.s\n", str_aux, pad_cols - get_visual_width(str_aux) - 1, " ");
        wattroff(pad, A_BOLD | header_color | A_REVERSE);
        wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
        line_count += 4;

        // Condição Helíaca
        const char *vis_label = _("Visible in Sky (Direct, public, and manifest expression)");
        if (stars[s].heliacal_state == HELIACAL_HIDDEN) {
            vis_label = _("Under the Sun's Beams (Latent, private, or internal expression)");
        } else if (stars[s].heliacal_state == HELIACAL_CIRCUMPOLAR) {
            vis_label = _("Circumpolar (Continuously above the local horizon)");
        }

        wattron(pad, A_BOLD);
        wprintw(pad, _(" • Heliacal Condition: %s\n"), vis_label);
        wattroff(pad, A_BOLD);
        wprintw(pad, "\n");
        line_count += 2;

        // Contatos Natais Ativos (Conjunções, Paralelos e Parans estreitos)
        wattron(pad, A_BOLD);
        wprintw(pad, is_solar_return ? _(" • Active Solar Return Connections:\n") : _(" • Active Radical Connections:\n"));
        wattroff(pad, A_BOLD);
        line_count++;

        for (int p = 0; p < plot_count; p++) {
            double lon_diff = fabs(swe_difdeg2n(stars[s].longitude, plots[p].longitude));
            if (lon_diff <= stars[s].meta->default_orb + DBL_EPSILON) {
                wattron(pad, A_BOLD | COLOR_PAIR(25));
                snprintf(str, sizeof(str), _("   [*] BODILY CONJUNCTION WITH %s %s (Orb: %.2f°)\n"),
                         plots[p].object, plots[p].object_name, lon_diff);
                line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);
                wattroff(pad, A_BOLD | COLOR_PAIR(25));
            }

            double par_diff = fabs(stars[s].declination - plots[p].declination);
            double cpar_diff = fabs(stars[s].declination + plots[p].declination);
            if (par_diff <= 1.0 + DBL_EPSILON) {
                snprintf(str, sizeof(str), _("   [*] PARALLEL OF DECLINATION WITH %s %s (Orb: %.2f°)\n"),
                         plots[p].object, plots[p].object_name, par_diff);
                line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);
            } else if (cpar_diff <= 1.0 + DBL_EPSILON) {
                snprintf(str, sizeof(str), _("   [*] CONTRA-PARALLEL OF DECLINATION WITH %s %s (Orb: %.2f°)\n"),
                         plots[p].object, plots[p].object_name, cpar_diff);
                line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);
            }
        }

        for (int pr = 0; pr < paran_count; pr++) {
            if (parans[pr].star_index == s && parans[pr].diff_minutes <= 6.0) {
                wattron(pad, A_BOLD | COLOR_PAIR(32));
                const char *paran_scope = is_solar_return ? _("SOLAR RETURN PARAN") : _("NATAL PARAN");
                snprintf(str, sizeof(str), _("   [*] %s: Star %s while %s %s is %s (Orb: %.1f min)\n"),
                        paran_scope,
                        parans[pr].star_angle,
                        parans[pr].object,
                        parans[pr].planet_name,
                        parans[pr].planet_angle,
                        parans[pr].diff_minutes);
                line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);
                wattroff(pad, A_BOLD | COLOR_PAIR(32));
            }
        }

        wprintw(pad, "\n");
        line_count++;

        // Temperamento Ptolomaico
        wattron(pad, A_BOLD);
        wprintw(pad, _(" • Planetary Temperament (%s):\n"), stars[s].meta->nature);
        wattroff(pad, A_BOLD);
        line_count++;

        snprintf(str, sizeof(str), "%s\n\n", get_temperament_meaning(stars[s].meta->nature));
        line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);

        // Delineamento Tradicional da Estrela
        if (stars[s].meta->delineation != NULL) {
            wattron(pad, A_BOLD);
            wprintw(pad, _(" • Traditional Delineation:\n"));
            wattroff(pad, A_BOLD);
            line_count++;

            snprintf(str, sizeof(str), "%s\n\n", _(stars[s].meta->delineation));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);
        }

        wprintw(pad, "\n");
        line_count++;

        if (is_solar_return && radix_star_active != NULL && radix_star_active[s]) {
            wattron(pad, A_BOLD | COLOR_PAIR(25) | A_REVERSE);
            snprintf(str, sizeof(str), _("   [★ RADIX REINFORCEMENT] This star is also anchored in your Natal Chart!\n"));
            line_count += print_split_lines(pad, str, MAX_LINE_WIDTH);
            wattroff(pad, A_BOLD | COLOR_PAIR(25) | A_REVERSE);
        }
        
    }

    if (active_found == 0) {
        line_count += print_split_lines(
            pad,
            _("No fixed stars form tight conjunctions, parallels, or narrow-orb parans in this natal chart.\n\n"),
            MAX_LINE_WIDTH
        );
    }

    wattron(pad, A_DIM);
    wprintw(pad, "───────────────────────────────────────────────────────────────────────────────────────────────\n");
    wprintw(pad, _("  [FIXED STARS END] - Press 'Q' or ESC to return to the Chart interface.\n"));
    wattroff(pad, A_DIM);
    line_count += 3;

    /* ======================================================================
    LOOP DE ROLAGEM E MOUSE (Exatamente idêntico ao seu padrão)
    ====================================================================== */
    int pad_line_pos = 0;
    int ch;
    int visible_height = (i_start_y + i_height - 4) - (i_start_y + 1) + 1;

    keypad(border_win, TRUE);

    int running = 1;
    while (running) {
        int flag = 0;
        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
        wattron(border_win, COLOR_PAIR(28) | flag);
        desenhar_scrollbar(border_win, pad_line_pos, line_count, visible_height, 1);
        wattroff(border_win, COLOR_PAIR(28) | flag);

        mvwaddstr(border_win, 1, i_width - 2, "▲");
        mvwaddstr(border_win, 2 + visible_height, i_width - 2, "▼");

        wnoutrefresh(border_win);
        prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);
        doupdate();

        ch = wgetch(border_win);
        if (ch == 27 || ch == 'q' || ch == 'Q') {
            break;
        }

        switch (ch) {
            case KEY_UP: case 'k': case 'K':
                if (pad_line_pos > 0) pad_line_pos--;
                break;
            case KEY_DOWN: case 'j': case 'J':
                if (pad_line_pos < (line_count - visible_height)) pad_line_pos++;
                break;
            case KEY_PPAGE:
                pad_line_pos -= visible_height;
                if (pad_line_pos < 0) pad_line_pos = 0;
                break;
            case KEY_NPAGE:
                pad_line_pos += visible_height;
                if (pad_line_pos > (line_count - visible_height)) {
                    pad_line_pos = (line_count > visible_height) ? (line_count - visible_height) : 0;
                }
                break;

            case KEY_MOUSE: {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    int linha_clique_janela = event.y - getbegy(border_win);
                    int col_clique_janela = event.x - getbegx(border_win);

                    int col_inicio_fechar = getmaxx(border_win) - 4;
                    int col_fim_fechar = col_inicio_fechar + 3;

                    if (linha_clique_janela == 0 && col_clique_janela >= col_inicio_fechar && col_clique_janela < col_fim_fechar) {
                        if (event.bstate & (BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED)) {
                            running = 0;
                            break;
                        }
                    }

                    int col_scrollbar_absoluta = getbegx(border_win) + (getmaxx(border_win) - 2);
                    if (event.x == col_scrollbar_absoluta) {
                        int linha_seta_cima = 1;
                        int offset_inicio_barra = 2;
                        int linha_seta_baixo = offset_inicio_barra + visible_height;

                        int max_scroll_y = line_count - visible_height;
                        if (max_scroll_y < 0) max_scroll_y = 0;

                        if (linha_clique_janela == linha_seta_cima) {
                            if (event.bstate & BUTTON1_PRESSED) {
                                nodelay(border_win, TRUE);
                                while (1) {
                                    if (pad_line_pos > 0) {
                                        pad_line_pos--;
                                        int flag = 0;
                                        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
                                        wattron(border_win, COLOR_PAIR(28) | flag);
                                        desenhar_scrollbar(border_win, pad_line_pos, line_count, visible_height, 1);
                                        wattroff(border_win, COLOR_PAIR(28) | flag);

                                        wnoutrefresh(border_win);
                                        prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);
                                        doupdate();
                                    } else {
                                        break;
                                    }
                                    napms(80);
                                    int ch_check = wgetch(border_win);
                                    if (ch_check == KEY_MOUSE) {
                                        MEVENT check_event;
                                        if (getmouse(&check_event) == OK && (check_event.bstate & BUTTON1_RELEASED)) {
                                            break;
                                        }
                                    }
                                }
                                nodelay(border_win, FALSE);
                            }
                        } else if (linha_clique_janela == linha_seta_baixo) {
                            if (event.bstate & BUTTON1_PRESSED) {
                                nodelay(border_win, TRUE);
                                while (1) {
                                    if (pad_line_pos < max_scroll_y) {
                                        pad_line_pos++;
                                        int flag = 0;
                                        if (DARK_MODE) flag |= A_DIM | A_REVERSE;
                                        wattron(border_win, COLOR_PAIR(28) | flag);
                                        desenhar_scrollbar(border_win, pad_line_pos, line_count, visible_height, 1);
                                        wattroff(border_win, COLOR_PAIR(28) | flag);

                                        wnoutrefresh(border_win);
                                        prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);
                                        doupdate();
                                    } else {
                                        break;
                                    }
                                    napms(80);
                                    int ch_check = wgetch(border_win);
                                    if (ch_check == KEY_MOUSE) {
                                        MEVENT check_event;
                                        if (getmouse(&check_event) == OK && (check_event.bstate & BUTTON1_RELEASED)) {
                                            break;
                                        }
                                    }
                                }
                                nodelay(border_win, FALSE);
                            }
                        } else if (event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED | REPORT_MOUSE_POSITION)) {
                            nodelay(border_win, TRUE);
                            int linha_trilho_atual = 0;
                            if (max_scroll_y > 0 && (visible_height - 1) > 0) {
                                linha_trilho_atual = (pad_line_pos * (visible_height - 1)) / max_scroll_y;
                            }
                            int linha_trilho_janela = linha_trilho_atual + offset_inicio_barra;
                            int modo_arrasto_trilho = (linha_clique_janela == linha_trilho_janela);
                            int linha_inicial_mouse = linha_clique_janela;
                            int pad_line_pos_original = pad_line_pos;

                            while (1) {
                                int novo_offset = pad_line_pos;
                                int linha_clique_barra = linha_clique_janela - offset_inicio_barra;

                                if (modo_arrasto_trilho) {
                                    int delta_mouse = linha_clique_janela - linha_inicial_mouse;
                                    if ((visible_height - 1) > 0 && max_scroll_y > 0) {
                                        int delta_scroll = (delta_mouse * max_scroll_y) / (visible_height - 1);
                                        novo_offset = pad_line_pos_original + delta_scroll;
                                    }
                                } else {
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
                                    int flag = 0;
                                    if (DARK_MODE) flag |= A_DIM | A_REVERSE;
                                    wattron(border_win, COLOR_PAIR(28) | flag);
                                    desenhar_scrollbar(border_win, pad_line_pos, line_count, visible_height, 1);
                                    wattroff(border_win, COLOR_PAIR(28) | flag);

                                    wnoutrefresh(border_win);
                                    prefresh(pad, pad_line_pos, 0, i_start_y + 1, i_start_x + 3, i_start_y + i_height - 2, i_start_x + i_width - 4);
                                    doupdate();
                                }

                                if ((event.bstate & BUTTON1_CLICKED) && !modo_arrasto_trilho) {
                                    break;
                                }
                                napms(10);
                                int ch_check = wgetch(border_win);
                                if (ch_check == KEY_MOUSE) {
                                    MEVENT check_event;
                                    if (getmouse(&check_event) == OK) {
                                        if (check_event.bstate & BUTTON1_RELEASED) {
                                            break;
                                        }
                                        linha_clique_janela = check_event.y - getbegy(border_win);
                                    }
                                } else if (ch_check != ERR) {
                                    ungetch(ch_check);
                                    break;
                                }
                            }
                            nodelay(border_win, FALSE);
                        }
                    }
                }
                break;
            }
        }
    }

    delwin(pad);
    delwin(shadow_win);
    delwin(border_win);

    drag_and_drop_off();
    touchwin(stdscr);
    refresh();
}