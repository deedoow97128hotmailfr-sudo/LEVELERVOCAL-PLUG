#pragma once
#include <array>

namespace gparams
{
enum Index { P_IN = 0, P_HPF, P_BODY, P_BOXY, P_PRES, P_AIR, P_DSTHR, P_DSFREQ, P_RIDER, P_RIDERTGT,
             P_CTHR, P_CRATIO, P_CATK, P_CREL, P_CKNEE, P_CMK, P_SATD, P_SATM, P_WIDTH, P_BASSMONO,
             P_CEIL, P_LIMREL, P_OUT, P_COUNT };

struct ParamDef { const char* id; const char* name; float mn, mx, def, skew; const char* unit; int dec; bool isBool; };

static const ParamDef kParams[P_COUNT] = {
    { "inGain",   "Entrée",       -12.f,  12.f,   0.f, 1.0f, " dB", 1, false },
    { "hpf",      "Passe-haut",    20.f, 300.f,  80.f, 0.5f, " Hz", 0, false },
    { "body",     "Corps 120Hz",   -6.f,   6.f,   0.f, 1.0f, " dB", 1, false },
    { "boxy",     "Boîte 320Hz",   -8.f,   3.f,   0.f, 1.0f, " dB", 1, false },
    { "presence", "Présence 3k",   -6.f,   6.f,   0.f, 1.0f, " dB", 1, false },
    { "air",      "Air 11k",       -6.f,   6.f,   0.f, 1.0f, " dB", 1, false },
    { "dsThr",    "De-ess seuil", -40.f,   0.f,   0.f, 1.0f, " dB", 1, false },
    { "dsFreq",   "De-ess fréq", 4000.f,10000.f,6500.f,1.0f, " Hz", 0, false },
    { "rider",    "Niveau auto",    0.f, 100.f,   0.f, 1.0f, " %",  0, false },
    { "riderTgt", "Cible auto",   -30.f, -12.f, -20.f, 1.0f, " dB", 1, false },
    { "cThr",     "Comp seuil",   -40.f,   0.f, -18.f, 1.0f, " dB", 1, false },
    { "cRatio",   "Ratio",          1.f,  10.f,   2.f, 0.6f, ":1",  1, false },
    { "cAtk",     "Attaque",       0.1f, 100.f,  10.f, 0.4f, " ms", 1, false },
    { "cRel",     "Relâche",       20.f,1000.f, 120.f, 0.5f, " ms", 0, false },
    { "cKnee",    "Coude",          0.f,  12.f,   6.f, 1.0f, " dB", 1, false },
    { "cMk",      "Gain comp",      0.f,  12.f,   0.f, 1.0f, " dB", 1, false },
    { "satD",     "Saturation",     0.f, 100.f,   0.f, 1.0f, " %",  0, false },
    { "satM",     "Mix sat",        0.f, 100.f,  30.f, 1.0f, " %",  0, false },
    { "width",    "Largeur",        0.f, 150.f, 100.f, 1.0f, " %",  0, false },
    { "bassMono", "Basses mono",    0.f,   1.f,   0.f, 1.0f, "",    0, true  },
    { "ceil",     "Plafond",       -3.f,   0.f,  -1.f, 1.0f, " dB", 1, false },
    { "limRel",   "Relâche lim",   10.f, 500.f, 100.f, 0.5f, " ms", 0, false },
    { "outGain",  "Sortie",       -12.f,  12.f,   0.f, 1.0f, " dB", 1, false },
};

enum Mode { MODE_VOICE = 0, MODE_INSTRU = 1, MODE_MASTER = 2 };

struct Preset { const char* name; int mode; std::array<float, P_COUNT> v; };

// ordre : in, hpf, body, boxy, pres, air, dsThr, dsFreq, rider, riderTgt, cThr, ratio, atk, rel, knee, mk,
//         satD, satM, width, bassMono, ceil, limRel, out
static const Preset kPresets[] = {
    // ---------------- VOIX ----------------
    { "Voix Rap / Trap",              MODE_VOICE,  {  0, 90, 1.0f,-2.5f, 2.0f, 2.5f,-26, 6500, 60,-20,-22, 4.0f,  5,  80,  6, 4.0f, 20, 35, 100, 0,-1.0f,  80, 0 } },
    { "Voix Chant Pop",               MODE_VOICE,  {  0, 80, 0.5f,-2.0f, 1.5f, 3.0f,-28, 7000, 50,-21,-20, 3.0f, 12, 120,  8, 3.0f, 12, 25, 100, 0,-1.0f, 100, 0 } },
    { "Voix Afro / Dancehall",        MODE_VOICE,  {  0, 85, 1.0f,-2.0f, 2.5f, 2.0f,-27, 6500, 60,-20,-21, 3.5f,  8, 100,  6, 3.5f, 18, 30, 100, 0,-1.0f,  80, 0 } },
    { "Voix Naturelle (léger)",       MODE_VOICE,  {  0, 70, 0.0f,-1.0f, 1.0f, 1.5f,-30, 6500, 35,-22,-18, 2.0f, 15, 150, 10, 2.0f,  5, 20, 100, 0,-1.0f, 100, 0 } },
    { "Voix très dynamique",          MODE_VOICE,  {  0, 90, 0.5f,-2.5f, 2.0f, 2.0f,-26, 6500, 90,-20,-26, 5.0f,  4,  90,  6, 5.0f, 15, 30, 100, 0,-1.0f,  80, 0 } },
    // ---------------- BUS INSTRUMENTAL ----------------
    { "Instru - Place pour la voix",  MODE_INSTRU, {  0, 25, 0.0f,-1.5f,-2.0f, 0.0f,  0, 6500,  0,-20,-18, 1.8f, 30, 150,  8, 1.0f, 10, 20, 110, 1,-1.5f, 100,-1.5f } },
    { "Instru Trap / Drill",          MODE_INSTRU, {  0, 20, 1.0f,-1.5f,-2.5f, 1.0f,  0, 6500,  0,-20,-16, 2.0f, 25, 120,  6, 1.5f, 15, 25, 100, 1,-1.5f, 100,-1.5f } },
    { "Instru Afro / Dancehall",      MODE_INSTRU, {  0, 25, 0.5f,-1.0f,-2.0f, 1.5f,  0, 6500,  0,-20,-17, 1.8f, 30, 140,  8, 1.5f, 12, 25, 105, 1,-1.5f, 100,-1.5f } },
    { "Instru Pop / Acoustique",      MODE_INSTRU, {  0, 30, 0.0f,-1.0f,-1.5f, 1.5f,  0, 6500,  0,-20,-19, 1.6f, 35, 160, 10, 1.0f,  8, 20, 120, 1,-1.5f, 100,-1.5f } },
    // ---------------- BUS MASTER ----------------
    { "Master Streaming -14 LUFS",    MODE_MASTER, {  0, 20, 0.5f,-0.5f, 0.5f, 1.0f,-24, 7500,  0,-20,-14, 1.8f, 30, 200,  8, 2.0f, 10, 20, 100, 1,-1.0f, 150, 0 } },
    { "Master Club Punchy",           MODE_MASTER, {  2, 20, 1.0f,-0.5f, 0.5f, 1.0f,-24, 7500,  0,-20,-12, 2.2f, 20, 120,  6, 3.0f, 15, 25, 100, 1,-1.0f, 100, 0 } },
    { "Master Doux Multi-écoute",     MODE_MASTER, {  0, 25, 0.5f,-1.0f, 1.0f, 1.5f,-24, 7500,  0,-20,-15, 1.6f, 35, 220, 10, 1.5f, 10, 20,  95, 1,-1.0f, 150, 0 } },
    { "Master Loud (~ -9 LUFS)",      MODE_MASTER, {  4, 20, 1.0f,-0.5f, 0.5f, 1.0f,-24, 7500,  0,-20,-11, 2.5f, 15, 100,  6, 3.0f, 20, 30, 100, 1,-1.0f,  80, 0 } },
};
static const int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
} // namespace gparams
