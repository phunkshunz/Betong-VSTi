#pragma once

// Inbyggda kit, mönster och effekt-presets.
//
// Pad-roller i alla kit (1-baserat, så som mönstren skrivs):
//  1 Kick   2 Kick 2/rumble   3 Stängd hat   4 Öppen hat   5 Clap   6 Snare
//  7 Tom låg   8 Tom mitt   9 Perc A   10 Perc B   11 Rim   12 Shaker
//  13 Bas   14 Stab   15 Lead/textur   16 Röst/FX
namespace betong
{
struct KitPreset { const char* name; const char* pads[16]; };

constexpr KitPreset kKits[] = {
    { "Industriell 909", { "K06_Industrial_Crushed", "B07_Rumble_Tone_F", "H06_Industrial_6bit", "H03_Open", "C02_Clap_Gated", "C04_Snare_Noise_Industrial", "P01_Tom_Low", "P02_Tom_Mid", "P07_Woodblock_6bit", "P04_Metal_FM", "C05_Rim", "P08_Shaker", "B05_Reese_Dist_F", "S06_Crushed_Noise_Chord", "T02_Industrial_Noise", "X11_Metal_Slam" } },
    { "Detroit Morgon", { "K01_909_Punch", "K09_Sub_Boom_F", "H01_Closed", "H03_Open", "C01_Clap_909ish", "C03_Snare_Tight", "P01_Tom_Low", "P03_Tom_High", "P06_Wood", "P05_Bell_FM", "C05_Rim", "P08_Shaker", "B01_Sub_Roll_F", "S07_Detroit_Strings_Fm", "L03_FM_Pling_F", "X03_Crash" } },
    { "Plastik Minimal", { "K04_Minimal_Tight", "K10_Click_Thump", "H02_Closed_Short", "H03_Open", "C01_Clap_909ish", "C03_Snare_Tight", "P11_Conga_Low", "P12_Conga_High", "P09_Blip", "P10_Blip_Low", "P14_Clave", "P08_Shaker", "B03_Acid_Squelch_F", "S08_Minor_Ring_Fm", "T08_Drone_Dark_F", "X07_Laser" } },
    { "Svensk Loop", { "K01_909_Punch", "K08_Rumble_Combo", "H01_Closed", "H03_Open", "C01_Clap_909ish", "C03_Snare_Tight", "P02_Tom_Mid", "P03_Tom_High", "P06_Wood", "P04_Metal_FM", "C05_Rim", "P08_Shaker", "B02_Offbeat_F", "S04_Organ_Fm", "Lp01_Perc_Swedish_130", "V04_Formant_Chop" } },
    { "Dub Kanal", { "K02_Deep_Long", "K09_Sub_Boom_F", "H01_Closed", "H05_Closed_12bit", "C06_Clap_Layer_Verb", "C03_Snare_Tight", "P11_Conga_Low", "P12_Conga_High", "P06_Wood", "P05_Bell_FM", "C05_Rim", "P08_Shaker", "B01_Sub_Roll_F", "S02_Dub_Fm7_Echo", "S03_Dub_Bbm7_Echo", "T06_Tape_Hiss" } },
    { "Acid Lager", { "K03_Distorted", "K08_Rumble_Combo", "H01_Closed", "H03_Open", "C01_Clap_909ish", "C04_Snare_Noise_Industrial", "P01_Tom_Low", "P02_Tom_Mid", "P09_Blip", "P04_Metal_FM", "C05_Rim", "P08_Shaker", "B03_Acid_Squelch_F", "B04_Acid_Soft_F", "L08_Sync_Scream_F", "X05_Zap_Down" } },
    { "Rave 93", { "K07_Hard_Clipped", "K01_909_Punch", "H01_Closed", "H03_Open", "C01_Clap_909ish", "C03_Snare_Tight", "P02_Tom_Mid", "P03_Tom_High", "P13_Cowbell_Dark", "P05_Bell_FM", "C05_Rim", "P08_Shaker", "B08_Saw_Stab_Bass_F", "S05_Rave_Stab_Fm", "L01_Hoover_F", "X06_Siren" } },
    { "Tribal Mörker", { "K02_Deep_Long", "B07_Rumble_Tone_F", "H02_Closed_Short", "H08_Open_Long_Metal", "C02_Clap_Gated", "C03_Snare_Tight", "P01_Tom_Low", "P02_Tom_Mid", "P11_Conga_Low", "P12_Conga_High", "P14_Clave", "P08_Shaker", "B10_Square_Sub_F", "S09_Fifths_Stab_F", "T05_Metal_Drone_F", "V06_Whisper" } },
    { "Betongbunker", { "K07_Hard_Clipped", "K03_Distorted", "H05_Closed_12bit", "H03_Open", "C02_Clap_Gated", "C04_Snare_Noise_Industrial", "P15_Metal_Hit", "P01_Tom_Low", "P07_Woodblock_6bit", "P04_Metal_FM", "C05_Rim", "P08_Shaker", "B05_Reese_Dist_F", "S06_Crushed_Noise_Chord", "L08_Sync_Scream_F", "X11_Metal_Slam" } },
    { "Röstmaskin", { "K01_909_Punch", "K10_Click_Thump", "H01_Closed", "H03_Open", "C01_Clap_909ish", "C03_Snare_Tight", "P11_Conga_Low", "P12_Conga_High", "P06_Wood", "P10_Blip_Low", "C05_Rim", "P08_Shaker", "B06_FM_Bass_F", "V01_Ah_F", "V03_Vocoder_Ee_F", "V04_Formant_Chop" } },
    { "Glitch Labb", { "K05_Lofi_12bit", "K10_Click_Thump", "H05_Closed_12bit", "H06_Industrial_6bit", "C02_Clap_Gated", "C04_Snare_Noise_Industrial", "P07_Woodblock_6bit", "P10_Blip_Low", "P09_Blip", "P15_Metal_Hit", "P14_Clave", "P08_Shaker", "B09_Pluck_Bass_F", "S10_Skew_Detuned_Fm", "X09_Glitch_Granular", "X08_Radio_Scrape" } },
    { "Maskinrummet", { "K06_Industrial_Crushed", "K08_Rumble_Combo", "H06_Industrial_6bit", "H08_Open_Long_Metal", "C04_Snare_Noise_Industrial", "C03_Snare_Tight", "P15_Metal_Hit", "P16_Spring_Thud", "P07_Woodblock_6bit", "P04_Metal_FM", "C05_Rim", "P08_Shaker", "B07_Rumble_Tone_F", "S01_Dub_Fm7_Dry", "T10_Machine_Room", "X10_Tape_Stop" } },
};
constexpr int kNumKits = (int) (sizeof (kKits) / sizeof (kKits[0]));

// Mönster: upp till 8 spår, "pad:takt:steg". takt = 16, 8, 16T, 32, 5, 7.
// Steg: . av, x på, X accent, o 50 %, p 25 %, 2/3/4 ratchet. Eller e:träffar:längd:rotation.
struct PatternPreset { const char* name; float swing; const char* lanes[8]; };

constexpr PatternPreset kPatterns[] = {
    // Klassiskt 90-tal
    { "Detroit Morgon", 0.15f, { "1:16:X...x...x...x...", "4:16:..x...x...x...x.", "5:16:....x.......x...", "3:16:x.xxx.xxx.xxx.xx", "13:16:x..x..x...x..x..", "14:16:......x.......o.", nullptr } },
    { "Berlin Lager", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "3:16:xxXxxxXxxxXxxxXx", "11:16:...x..x....x..x.", "13:16:..x...x...x...x.", nullptr } },
    { "909 Grund", 0.1f, { "1:16:X...x...x...x...", "4:16:..x...x...x...x.", "5:16:....x.......x...", "3:16:x.x.x.x.x.x.x.x.", "8:16:.............................x3x", nullptr } },
    { "Swingig 909", 0.45f, { "1:16:X...x...x...x...", "4:16:..x...x...x...x.", "5:16:....x.......x...", "3:16:xxxxxxxxxxxxxxxx", "12:16:.x.x.x.x.x.x.x.x", nullptr } },
    { "Rave 93", 0.0f, { "1:16:X...x...x...x...", "4:16:..x...x...x...x.", "5:16:....x.......x..4", "14:16:x.....x.....x...", "15:16:x...............................", "13:16:x.xx.x.xx.x.xx.x", nullptr } },
    { "Ride Rusning", 0.05f, { "1:16:X...x...x...x...", "10:8:xxxxxxxx", "4:16:..x...x...x...x.", "5:16:....x.......x...", "13:16:..x...x...x..xx.", nullptr } },
    { "Tung Fyra", 0.0f, { "1:16:X...x...x...x...", "6:16:....X.......X...", "12:16:xxxxxxxxxxxxxxxx", "4:16:..x...x...x...x.", "2:16:x...x...x...x...", nullptr } },
    // Plastikman minimal
    { "Plast Klick", 0.08f, { "1:16:X...x...x...x...", "9:16:e:5:16:0", "13:16:x.x..x.xX..x.x.x", "3:12:..x..x..x..x", "10:16:e:3:16:2", nullptr } },
    { "Konsumerad", 0.0f, { "1:16:x.......x.......", "15:16:x...............................", "11:7:x..x...", "2:16:x...............", "10:16:e:2:16:5", nullptr } },
    { "Plast Ark", 0.0f, { "1:16:X...x...x...x...", "13:16:X.x2x.xxo.x.Xxx.", "3:12:..x..x..x..x", "11:16:...x.......x..o.", "14:16:..........x.....", nullptr } },
    { "Fem mot Tolv", 0.0f, { "1:16:X...x...x...x...", "9:5:x.x..", "7:12:x..x.x..x.x.", "3:16:..x...x...x...x.", "13:16:x...x..x..x.x...", nullptr } },
    { "Spritz", 0.12f, { "1:16:X...x...x...x...", "9:16:oo.o.po.o.op.o.p", "10:16:..3...p...2...o.", "3:16:..x...x...x...x.", "13:16:x..x....x..x....", nullptr } },
    { "Tom Konga Svep", 0.1f, { "1:16:X...x...x...x...", "7:16:x..x..x...x..x..", "8:16:..x...x.x...x..x", "9:16:.x...x.....x..x.", "3:16:..x...x...x...x.", nullptr } },
    { "Minimal Drift", 0.0f, { "1:16:X...x...x...x...", "9:15:x...x...x...x..", "3:16:..x...x...x...x.", "13:17:x..x..x..x..x..x.", "11:16:....x.......x...", nullptr } },
    // Svensk loop-techno
    { "Stockholm Loop", 0.05f, { "1:16:X...x...x...x...", "15:16:x...............", "3:16:..x...x...x...x.", "5:16:....x.......x...", "8:16:...x..x....x..x.", "12:16:xxxxxxxxxxxxxxxx", nullptr } },
    { "Södermalm Funk", 0.3f, { "1:16:X...x...x...x...", "7:16:x..x..x.x..x..x.", "5:16:..x...x...x...x.", "3:16:xxxxxxxxxxxxxxxx", "13:16:..x..x..x..x..x.", nullptr } },
    { "Kod Rull", 0.0f, { "1:16:X...x...x...x...", "7:16:xxxxxxxxxxxxxxxx", "8:16:..x...x...x..xx.", "4:16:..x...x...x...x.", "5:16:....x.......x...", nullptr } },
    { "Hammarby Hammare", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "15:16:x.......x.......", "4:16:..X...X...X...X.", "5:16:....x.......x...", "11:16:.x..x..x.x..x..x", nullptr } },
    { "Själsgroove", 0.4f, { "1:16:X...x...x...x...", "12:16:x.x.x.x.x.x.x.x.", "5:16:....x.......x...", "9:16:..x.....x.x.....", "13:16:..x...x...x...x.", nullptr } },
    { "Kalla Fötter", 0.0f, { "1:16:X...x...x...x...", "11:16T:x..x..x..x..x..x", "3:16:..x...x...x...x.", "15:16:x...............", "13:16:..x...x...x..x..", nullptr } },
    { "Loopkapning", 0.05f, { "1:16:X...x...x...x...", "15:16:xoxoxoxoxoxoxoxo", "16:16:...o......o..o..", "4:16:..x...x...x...x.", "5:16:....x.......x...", nullptr } },
    { "Göteborg Grus", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "15:8:x.x.x.x.", "6:16:....x.......x...", "10:16:.x..x...x..x..x.", nullptr } },
    // Dubfire tribal tech
    { "Tribal Mörker", 0.08f, { "1:16:X...x...x...x...", "7:16:x..x..x...x..x..", "8:12:..x..x.x..x.", "9:7:x.x.x..", "3:16:..x...x...x...x.", "13:16:..x...x...x...x.", nullptr } },
    { "Rullande Bas", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "13:16:.xxx.xxx.xxx.xxx", "9:16:..x...x...x...x.", "3:16:xxxxxxxxxxxxxxxx", nullptr } },
    { "Djungelgolv", 0.15f, { "1:16:X...x...x...x...", "9:16:e:7:16:0", "10:16:e:5:16:3", "12:16:xxxxxxxxxxxxxxxx", "4:16:..x...x...x...x.", nullptr } },
    { "Mörk Tech", 0.0f, { "1:16:X...x...x...x...", "11:16:e:3:16:1", "3:16:..x...x...x...x.", "13:16:..x...x...x...x.", "14:16:...........x....", nullptr } },
    { "Spänningsbygge", 0.0f, { "1:16:X...x...x...x...", "9:16:x.x.x.x.x2x2x3x4", "3:16:..x...x...x...x.", "5:16:....x.......x...", "16:16:x...............................", nullptr } },
    { "Ritual", 0.05f, { "1:16:X...x...x...x...", "7:5:x..x.", "8:7:x.x..x.", "9:9:x..x.x...", "3:16:..x...x...x...x.", nullptr } },
    { "Klubbens Kärna", 0.1f, { "1:16:X...x...x...x...", "5:16:....x.......x...", "9:16:oxoxoxoxoxoxoxox", "3:16:..x...x...x...x.", "13:16:..x...x...x...x.", nullptr } },
    // Industrial
    { "Industriell 909", 0.2f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "7:16:...x..x....x..x.", "9:16:.....x.......x..", "12:16:......x.......x.", "6:16:....x.......x...", nullptr } },
    { "Fabriksgolv", 0.0f, { "1:16:X...x...x...x...", "6:16:....X.......X...", "3:16:xxXxxxXxxxXxxxXx", "2:16:x...x...x...x...", "16:16:........................x.......", nullptr } },
    { "Stålhall", 0.0f, { "1:16:X...x...x...x...", "10:8:e:5:8:0", "3:16:..x...x...x...x.", "15:16:x...............", nullptr } },
    { "Rost", 0.05f, { "1:16:X...x...x...x...", "7:16:..x..x....x..x..", "5:16:....x.......x...", "9:16:e:5:16:2", "3:16:..x...x...x...x.", nullptr } },
    { "Maskinrummet", 0.0f, { "1:16:X...x...x...x...", "3:16:oooooooooooooooo", "9:16:xoxoxoxoxoxoxoxo", "10:16:o.o.o.o.o.o.o.o.", "7:16:o..o..o..o..o..o", "15:16:x...............................", nullptr } },
    { "Betongpuls", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "3:16T:..x..x..x..x..x.", "6:16:....x.......x...", "13:16:..x...x...x...x.", nullptr } },
    // Dub techno
    { "Kanal Ett", 0.0f, { "1:16:X...x...x...x...", "14:16:..x.....x..x....", "3:16:..x...x...x...x.", "12:16:.x.x.x.x.x.x.x.x", "13:16:x.......x.......", nullptr } },
    { "Dimbank", 0.0f, { "1:16:X...x...x...x...", "14:16:...x.......x....", "11:7:x..x...", "3:16:..x...x...x...x.", "16:16:x...............................", nullptr } },
    { "Djup Dub", 0.05f, { "1:16:x...x...x...x...", "14:16:..x.............................", "3:16:..x...x...x...x.", "15:16:x...............................", "13:16:x...........x...", nullptr } },
    { "Ekolod", 0.0f, { "1:16:X...x...x...x...", "11:16T:x...x...x...x...", "14:16:...x.....x......", "3:16:..x...x...x...x.", nullptr } },
    { "Morgongrått", 0.0f, { "1:16:x...x...x...x...", "14:16:o.....o.....o...", "3:16:..p...x...p...x.", "15:16:x...............................", nullptr } },
    // Hårt och snabbt
    { "Peak Time", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "3:16:xxXxxxXxxxXxxxXx", "5:16:....x.......x...", "13:16:..x...x...x...x.", nullptr } },
    { "Hård Industri", 0.0f, { "1:16:X...x...x...x..x", "5:16:....x.......x..4", "3:16:..x...x...x...x.", "9:16:x..x..x..x..x..x", nullptr } },
    { "Rave Larm", 0.0f, { "1:16:X...x...x...x...", "13:16:X.x2x.xxX.x.Xxx.", "16:16:x...............................", "4:16:..x...x...x...x.", "5:16:....x.......x...", nullptr } },
    { "Bunkerdörr", 0.0f, { "1:16:X...x...x...x...", "9:16:2.2.2.2.2.2.2.3.", "3:16:..x...x...x...x.", "6:16:....x.......x...", nullptr } },
    { "Snabb Rumble", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "4:16:..x...x...x...x.", "12:16:xxxxxxxxxxxxxxxx", nullptr } },
    // Polyrytm och experiment
    { "Euklid 3-5-7", 0.0f, { "1:16:X...x...x...x...", "7:8:e:3:8:0", "9:13:e:5:13:0", "10:16:e:7:16:0", "3:16:..x...x...x...x.", nullptr } },
    { "Kvintol-svep", 0.0f, { "1:16:X...x...x...x...", "3:5:xxxxxxxxxx", "13:16:..x...x...x...x.", "5:16:....x.......x...", nullptr } },
    { "Septol-rost", 0.0f, { "1:16:X...x...x...x...", "9:7:x.xx.x.x.xx.x", "3:16:..x...x...x...x.", "5:16:....x.......x...", nullptr } },
    { "Fasförskjutning", 0.0f, { "1:16:X...x...x...x...", "9:12:x..x.xx..x.x", "10:13:x..x.xx..x.x.", "3:16:..x...x...x...x.", nullptr } },
    { "Kaos Kontrollerat", 0.2f, { "1:16:X...x...x...x...", "9:11:oxo.o.x.op.", "10:9:p.o.x.o.p", "7:7:o..x.o.", "13:13:x.o.x..o.x.p.", "3:16:o.x.o.x.o.x.o.x.", nullptr } },
};
constexpr int kNumPatterns = (int) (sizeof (kPatterns) / sizeof (kPatterns[0]));

// Effekt-presets. Siffersträngar = ett värde per spår (0..9). Spår 1 är nästan alltid kicken.
// delay: 0=1/8 1=1/8 punkt 2=3/16 3=1/8T 4=5/16 5=1/4
struct FxPreset
{
    const char* name;
    int delayTime; float delayFb, delayTone, delayWobble, delayReturn;
    float rumbleSize, rumbleDrive, rumbleCut, rumbleDuck, rumbleReturn;
    float clip, crushMaster;
    const char* sendA; const char* sendB; const char* crush; const char* drive; const char* reso;
};

constexpr FxPreset kFx[] = {
    // Rumble
    { "Betonggolv Rumble", 2, 0.45f, 0.4f, 0.1f, 0.25f, 0.85f, 0.5f, 100.0f, 0.9f, 0.85f, 0.35f, 0, "00000000", "90000000", "00000000", "30000000", "00000000" },
    { "Källar-Rumble", 2, 0.4f, 0.3f, 0.0f, 0.2f, 0.6f, 0.7f, 80.0f, 0.95f, 0.8f, 0.5f, 0, "00000000", "90000000", "00000000", "20000000", "00000000" },
    { "Rumble Pump Hård", 1, 0.4f, 0.5f, 0.0f, 0.2f, 0.75f, 0.9f, 120.0f, 1.0f, 0.9f, 0.55f, 0, "00000000", "99000000", "00000000", "50000000", "00000000" },
    { "Rumble Mjuk Sub", 2, 0.35f, 0.3f, 0.0f, 0.15f, 0.5f, 0.2f, 60.0f, 0.7f, 0.7f, 0.2f, 0, "00000000", "70000000", "00000000", "00000000", "00000000" },
    { "Tunnelbana", 4, 0.55f, 0.25f, 0.3f, 0.3f, 1.0f, 0.6f, 110.0f, 0.85f, 0.85f, 0.4f, 0.3f, "00050000", "90000000", "00300000", "20000000", "00000000" },
    { "Rumble Rattle", 2, 0.4f, 0.5f, 0.0f, 0.2f, 0.7f, 0.85f, 150.0f, 0.9f, 0.75f, 0.45f, 0, "00000000", "90000000", "00000000", "40000000", "00000000" },
    { "Lagerlokal", 5, 0.5f, 0.35f, 0.1f, 0.25f, 1.0f, 0.3f, 130.0f, 0.6f, 0.7f, 0.3f, 0, "00300300", "95005000", "00000000", "10000000", "00000000" },
    // Dub
    { "Kanalbas-eko", 2, 0.68f, 0.45f, 0.25f, 0.6f, 0.6f, 0.3f, 90.0f, 0.8f, 0.4f, 0.25f, 0, "00000900", "60000000", "00000000", "00000000", "00000000" },
    { "Punkterad Dimma", 1, 0.62f, 0.3f, 0.2f, 0.55f, 0.6f, 0.3f, 90.0f, 0.8f, 0.35f, 0.2f, 0, "00300600", "50000000", "00000000", "00000000", "00000000" },
    { "Fem Sextondelar", 4, 0.6f, 0.5f, 0.15f, 0.55f, 0.5f, 0.3f, 90.0f, 0.8f, 0.3f, 0.2f, 0, "00009900", "40000000", "00000000", "00000000", "00000000" },
    { "Triol-eko", 3, 0.55f, 0.55f, 0.1f, 0.5f, 0.5f, 0.3f, 90.0f, 0.8f, 0.3f, 0.25f, 0, "00099000", "40000000", "00000000", "00000000", "00000000" },
    { "Tape-Dub", 2, 0.65f, 0.35f, 0.8f, 0.6f, 0.6f, 0.4f, 90.0f, 0.8f, 0.35f, 0.3f, 0.1f, "00000990", "50000000", "00000000", "00000000", "00000000" },
    { "Freeze Moln", 5, 0.85f, 0.3f, 0.5f, 0.65f, 0.9f, 0.3f, 100.0f, 0.5f, 0.4f, 0.2f, 0, "00009990", "60000000", "00000000", "00000000", "00000000" },
    { "Dubbelt Eko", 0, 0.6f, 0.6f, 0.1f, 0.55f, 0.5f, 0.3f, 90.0f, 0.8f, 0.3f, 0.25f, 0, "00900000", "40000000", "00000000", "00000000", "00000000" },
    { "Ekokammare", 1, 0.5f, 0.85f, 0.2f, 0.45f, 0.5f, 0.3f, 90.0f, 0.8f, 0.3f, 0.2f, 0, "09000090", "40000000", "00000000", "00000000", "00000000" },
    // Distorsion
    { "Mixern i Rött", 2, 0.35f, 0.4f, 0.0f, 0.15f, 0.6f, 0.5f, 100.0f, 0.8f, 0.5f, 0.6f, 0, "00000000", "70000000", "00000000", "70000000", "00000000" },
    { "Fabrikshall", 2, 0.4f, 0.4f, 0.0f, 0.2f, 0.7f, 0.6f, 100.0f, 0.85f, 0.6f, 0.85f, 0, "00000000", "80000000", "00000000", "53333333", "00000000" },
    { "Krossad Bunker", 1, 0.45f, 0.5f, 0.0f, 0.2f, 0.7f, 0.8f, 110.0f, 0.9f, 0.7f, 0.9f, 0.15f, "00000000", "90000000", "02222222", "66666666", "00000000" },
    { "Acid Skrik", 2, 0.55f, 0.6f, 0.1f, 0.35f, 0.6f, 0.5f, 100.0f, 0.8f, 0.5f, 0.5f, 0, "00009000", "70000000", "00000000", "20009000", "00009000" },
    { "Rostig Tom", 2, 0.45f, 0.4f, 0.0f, 0.25f, 0.6f, 0.5f, 100.0f, 0.8f, 0.5f, 0.45f, 0, "00900000", "70000000", "00700000", "00700000", "00000000" },
    { "Smutsig Parallell", 2, 0.4f, 0.4f, 0.0f, 0.2f, 0.6f, 0.6f, 100.0f, 0.8f, 0.6f, 0.75f, 0, "00000000", "80000000", "00000000", "44444444", "00000000" },
    { "Varm Tejp", 2, 0.5f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f, 90.0f, 0.8f, 0.5f, 0.35f, 0.05f, "00300300", "70000000", "00000000", "22222222", "00000000" },
    { "Överstyrd 909", 2, 0.4f, 0.4f, 0.0f, 0.2f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.6f, 0, "00000000", "70000000", "00000000", "90000000", "00000000" },
    // Lo-fi
    { "SP-12 Grus", 2, 0.4f, 0.4f, 0.1f, 0.25f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0.25f, "00000000", "70000000", "05555555", "00000000", "00000000" },
    { "Sexbitars Trä", 2, 0.45f, 0.4f, 0.1f, 0.3f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0, "00000000", "70000000", "00009900", "00000000", "00000000" },
    { "MPC Varm", 2, 0.4f, 0.3f, 0.2f, 0.25f, 0.6f, 0.3f, 90.0f, 0.8f, 0.5f, 0.35f, 0.15f, "00000000", "70000000", "03333333", "11111111", "00000000" },
    { "Kassett", 2, 0.5f, 0.25f, 0.9f, 0.35f, 0.6f, 0.3f, 90.0f, 0.7f, 0.5f, 0.3f, 0.2f, "00300330", "60000000", "02222222", "11111111", "00000000" },
    { "Vinylgolv", 1, 0.5f, 0.3f, 0.4f, 0.35f, 0.6f, 0.3f, 90.0f, 0.7f, 0.5f, 0.3f, 0.1f, "00000660", "60000000", "00000000", "00000000", "00000000" },
    { "Trasig Sampler", 3, 0.5f, 0.5f, 0.3f, 0.4f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.45f, 0.5f, "00990000", "70000000", "09999999", "00000000", "00000000" },
    // Filter
    { "Acid Svep", 2, 0.55f, 0.6f, 0.1f, 0.35f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.45f, 0, "00009000", "70000000", "00000000", "00006000", "00008000" },
    { "Självsvängning", 2, 0.5f, 0.5f, 0.1f, 0.3f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0, "00990000", "70000000", "00000000", "00000000", "00999999" },
    { "Bandpass Dimma", 1, 0.6f, 0.4f, 0.2f, 0.45f, 0.6f, 0.3f, 90.0f, 0.8f, 0.4f, 0.3f, 0, "00000990", "60000000", "00000000", "00000000", "00000660" },
    { "Pump-filter", 2, 0.45f, 0.5f, 0.0f, 0.25f, 0.7f, 0.5f, 100.0f, 1.0f, 0.6f, 0.45f, 0, "00000000", "90000000", "00000000", "00000000", "05555555" },
    { "Triol-svep", 3, 0.55f, 0.6f, 0.1f, 0.4f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0, "00900000", "70000000", "00000000", "00000000", "00700000" },
    { "Notch-fas", 2, 0.45f, 0.5f, 0.3f, 0.3f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "00000000", "70000000", "00000000", "00000000", "05555555" },
    { "Comb-ringning", 4, 0.75f, 0.8f, 0.0f, 0.45f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0, "00099000", "70000000", "00000000", "00000000", "00990000" },
    { "Master-uppbyggnad", 2, 0.5f, 0.6f, 0.1f, 0.35f, 0.7f, 0.5f, 110.0f, 0.8f, 0.6f, 0.5f, 0, "00300300", "80000000", "00000000", "20000000", "00000000" },
    // Rymd
    { "Gated Clap", 0, 0.35f, 0.6f, 0.0f, 0.3f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "00009000", "70000000", "00000000", "00000000", "00000000" },
    { "Plåtregn", 0, 0.4f, 0.9f, 0.0f, 0.35f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "09900009", "70000000", "00000000", "00000000", "00000000" },
    { "Mörk Katedral", 5, 0.75f, 0.2f, 0.3f, 0.55f, 1.0f, 0.3f, 90.0f, 0.6f, 0.6f, 0.3f, 0, "00000999", "80000000", "00000000", "00000000", "00000000" },
    { "Betongrum", 0, 0.3f, 0.5f, 0.0f, 0.3f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "00000000", "70000000", "00000000", "00000000", "00000000" },
    { "Flanger-Jet", 3, 0.7f, 0.8f, 1.0f, 0.45f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "09900000", "70000000", "00000000", "00000000", "00000000" },
    { "Stereospridning", 0, 0.45f, 0.6f, 0.2f, 0.4f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "00999990", "70000000", "00000000", "00000000", "00000000" },
    { "Ride i Rök", 5, 0.6f, 0.7f, 0.2f, 0.45f, 0.8f, 0.3f, 100.0f, 0.8f, 0.5f, 0.3f, 0, "09000000", "70000000", "00000000", "00000000", "00000000" },
    // Performance
    { "Stutter-redo", 2, 0.4f, 0.5f, 0.0f, 0.25f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.45f, 0, "00000000", "70000000", "00000000", "20000000", "00000000" },
    { "Tejpstopp-känsla", 4, 0.6f, 0.3f, 1.0f, 0.4f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0.1f, "00300900", "70000000", "00000000", "10000000", "00000000" },
    { "Mute-cykel Dub", 2, 0.65f, 0.4f, 0.3f, 0.6f, 0.6f, 0.3f, 90.0f, 0.8f, 0.4f, 0.3f, 0, "00999999", "60000000", "00000000", "00000000", "00000000" },
    { "Ren Klubb", 2, 0.4f, 0.5f, 0.0f, 0.2f, 0.6f, 0.3f, 100.0f, 0.8f, 0.45f, 0.25f, 0, "00000000", "60000000", "00000000", "00000000", "00000000" },
    { "Total Kollaps", 4, 0.9f, 0.4f, 0.7f, 0.6f, 1.0f, 1.0f, 140.0f, 1.0f, 0.8f, 0.95f, 0.45f, "09999999", "99000000", "05555555", "77777777", "03333333" },
    { "Fuktigt Betonggolv", 2, 0.62f, 0.35f, 0.35f, 0.45f, 0.9f, 0.6f, 105.0f, 0.9f, 0.8f, 0.25f, 0.08f, "00300960", "95000000", "00200000", "40000000", "00000000" },
};
constexpr int kNumFx = (int) (sizeof (kFx) / sizeof (kFx[0]));
} // namespace betong
