#pragma once

// Inbyggda kit, mönster och effekt-presets.
//
// Pad-roller i alla kit (1-baserat, så som mönstren skrivs):
//  1 Kick   2 Kick 2/rumble   3 Stängd hat   4 Öppen hat   5 Clap   6 Snare
//  7 Tom låg   8 Tom mitt   9 Perc A   10 Perc B   11 Rim   12 Shaker
//  13 Bas   14 Stab   15 Lead/textur   16 Röst/FX
namespace betong
{
struct KitPreset { const char* name; const char* pads[16]; const char* group = nullptr; };

constexpr KitPreset kKits[] = {
    { "Betong Grund", { "K22_Grund_Techno_F", "K24_Berlin_Deep_F", "H17_Closed_Clean", "H19_Open_Clean", "C15_Clap_Clean", "C17_Snare_909_Clean", "P43_Tom_909_Low", "P44_Tom_909_Mid", "P41_Woodblock_Tribal", "P47_Clave_Clean", "C19_Rim_909", "H22_Shaker_Clean", "B21_Techno_Bas_F", "S01_Dub_Fm7_Dry", "H20_Ride_Clean", "X14_Crash_Clean" }, "Rena grundkit" },
    { "Peak Time Ren", { "K23_Peak_Punch", "K28_Lang_Svans_F", "H17_Closed_Clean", "H19_Open_Clean", "C16_Clap_Wide_Room", "C18_Snare_Fat", "P43_Tom_909_Low", "P45_Tom_909_High", "P46_Cowbell_808", "P25_Conga_Slap", "C19_Rim_909", "H22_Shaker_Clean", "B22_Offbeat_Ren_F", "S16_Chord_Stab_Tech_Fm9", "H20_Ride_Clean", "X01_Noise_Riser" }, "Rena grundkit" },
    { "Berlin Djup", { "K24_Berlin_Deep_F", "B07_Rumble_Tone_F", "H18_Closed_Tight", "H19_Open_Clean", "C15_Clap_Clean", "C17_Snare_909_Clean", "P43_Tom_909_Low", "P44_Tom_909_Mid", "P29_Djembe_Tone", "P47_Clave_Clean", "C19_Rim_909", "H22_Shaker_Clean", "B20_Sub_Sine_F", "S02_Dub_Fm7_Echo", "T08_Drone_Dark_F", "X14_Crash_Clean" }, "Rena grundkit" },
    { "Hardgroove 909", { "K25_Hardgroove_Tight", "K27_909_Classic", "H17_Closed_Clean", "H19_Open_Clean", "C16_Clap_Wide_Room", "C18_Snare_Fat", "P44_Tom_909_Mid", "P45_Tom_909_High", "P46_Cowbell_808", "P25_Conga_Slap", "C19_Rim_909", "H22_Shaker_Clean", "B22_Offbeat_Ren_F", "S15_Organ_Stab_House_Fm", "H20_Ride_Clean", "X12_Impact_Steel" }, "Rena grundkit" },
    { "Tech House Ren", { "K19_Tech_House", "K26_Rund_Minimal", "H14_Hat_Closed_Crisp", "H15_Hat_Open_House", "C11_Clap_House", "C12_Snap", "P24_Bongo_Low", "P23_Bongo_High", "P25_Conga_Slap", "P33_Tambourine", "C19_Rim_909", "P40_Shaker_Soft", "B17_Organ_Bass_F", "S15_Organ_Stab_House_Fm", "S17_Marimba_F", "V07_Hey_Shout" }, "Rena grundkit" },
    { "Tribal Ren", { "K21_Tribal_Tom_Kick", "K22_Grund_Techno_F", "H18_Closed_Tight", "H19_Open_Clean", "C15_Clap_Clean", "C17_Snare_909_Clean", "P26_Conga_Open", "P28_Djembe_Bass", "P29_Djembe_Tone", "P30_Djembe_Slap", "P41_Woodblock_Tribal", "P36_Cabasa", "B21_Techno_Bas_F", "S09_Fifths_Stab_F", "P34_Log_Drum_F", "V08_Chant_Oh_F" }, "Rena grundkit" },
    { "Detroit 909", { "K27_909_Classic", "K26_Rund_Minimal", "H17_Closed_Clean", "H19_Open_Clean", "C15_Clap_Clean", "C17_Snare_909_Clean", "P43_Tom_909_Low", "P45_Tom_909_High", "P05_Bell_FM", "P47_Clave_Clean", "C19_Rim_909", "H22_Shaker_Clean", "B01_Sub_Roll_F", "S07_Detroit_Strings_Fm", "H20_Ride_Clean", "X14_Crash_Clean" }, "Rena grundkit" },
    { "Minimal Ren", { "K26_Rund_Minimal", "K10_Click_Thump", "H18_Closed_Tight", "H21_Hat_Pedal", "C12_Snap", "C17_Snare_909_Clean", "P27_Conga_Mute", "P23_Bongo_High", "P09_Blip", "P47_Clave_Clean", "C19_Rim_909", "H22_Shaker_Clean", "B18_Tech_Pluck_F", "S08_Minor_Ring_Fm", "T12_Jungle_Night", "X07_Laser" }, "Rena grundkit" },
    { "Dub Ren", { "K24_Berlin_Deep_F", "K28_Lang_Svans_F", "H17_Closed_Clean", "H21_Hat_Pedal", "C16_Clap_Wide_Room", "C17_Snare_909_Clean", "P43_Tom_909_Low", "P44_Tom_909_Mid", "P27_Conga_Mute", "P47_Clave_Clean", "C19_Rim_909", "H22_Shaker_Clean", "B20_Sub_Sine_F", "S02_Dub_Fm7_Echo", "S03_Dub_Bbm7_Echo", "T06_Tape_Hiss" }, "Rena grundkit" },
    { "Afro Ren", { "K19_Tech_House", "K20_Round_Deep_F", "H14_Hat_Closed_Crisp", "H15_Hat_Open_House", "C11_Clap_House", "C12_Snap", "P28_Djembe_Bass", "P26_Conga_Open", "P38_Agogo_High", "P39_Agogo_Low", "P31_Timbale_High", "P40_Shaker_Soft", "B19_Rubber_Bass_F", "S17_Marimba_F", "P34_Log_Drum_F", "P42_Talking_Drum" }, "Rena grundkit" },
    { "Acid Ren", { "K23_Peak_Punch", "K24_Berlin_Deep_F", "H17_Closed_Clean", "H19_Open_Clean", "C15_Clap_Clean", "C17_Snare_909_Clean", "P43_Tom_909_Low", "P44_Tom_909_Mid", "P46_Cowbell_808", "P47_Clave_Clean", "C19_Rim_909", "H22_Shaker_Clean", "B03_Acid_Squelch_F", "B04_Acid_Soft_F", "L02_Saw_Seq_F_130", "X14_Crash_Clean" }, "Rena grundkit" },
    { "Industri Ren", { "K25_Hardgroove_Tight", "K28_Lang_Svans_F", "H17_Closed_Clean", "H19_Open_Clean", "C16_Clap_Wide_Room", "C18_Snare_Fat", "P43_Tom_909_Low", "P45_Tom_909_High", "P17_Metal_Pipe", "P20_Hammer_Clank", "C19_Rim_909", "H22_Shaker_Clean", "B13_Growl_Bass_F", "S12_Industrial_Stab_Fm", "T10_Machine_Room", "X12_Impact_Steel" }, "Rena grundkit" },
    { "Industriell 909", { "K06_Industrial_Crushed", "B07_Rumble_Tone_F", "H06_Industrial_6bit", "H03_Open", "C02_Clap_Gated", "C04_Snare_Noise_Industrial", "P01_Tom_Low", "P02_Tom_Mid", "P07_Woodblock_6bit", "P04_Metal_FM", "C05_Rim", "P08_Shaker", "B05_Reese_Dist_F", "S06_Crushed_Noise_Chord", "T02_Industrial_Noise", "X11_Metal_Slam" }, "Klassiska kit" },
    { "Detroit Morgon", { "K01_909_Punch", "K09_Sub_Boom_F", "H01_Closed", "H03_Open", "C01_Clap_909ish", "C03_Snare_Tight", "P01_Tom_Low", "P03_Tom_High", "P06_Wood", "P05_Bell_FM", "C05_Rim", "P08_Shaker", "B01_Sub_Roll_F", "S07_Detroit_Strings_Fm", "L03_FM_Pling_F", "X03_Crash" }, "Klassiska kit" },
    { "Plastik Minimal", { "K04_Minimal_Tight", "K10_Click_Thump", "H02_Closed_Short", "H03_Open", "C01_Clap_909ish", "C03_Snare_Tight", "P11_Conga_Low", "P12_Conga_High", "P09_Blip", "P10_Blip_Low", "P14_Clave", "P08_Shaker", "B03_Acid_Squelch_F", "S08_Minor_Ring_Fm", "T08_Drone_Dark_F", "X07_Laser" }, "Klassiska kit" },
    { "Svensk Loop", { "K01_909_Punch", "K08_Rumble_Combo", "H01_Closed", "H03_Open", "C01_Clap_909ish", "C03_Snare_Tight", "P02_Tom_Mid", "P03_Tom_High", "P06_Wood", "P04_Metal_FM", "C05_Rim", "P08_Shaker", "B02_Offbeat_F", "S04_Organ_Fm", "Lp01_Perc_Swedish_130", "V04_Formant_Chop" }, "Klassiska kit" },
    { "Dub Kanal", { "K02_Deep_Long", "K09_Sub_Boom_F", "H01_Closed", "H05_Closed_12bit", "C06_Clap_Layer_Verb", "C03_Snare_Tight", "P11_Conga_Low", "P12_Conga_High", "P06_Wood", "P05_Bell_FM", "C05_Rim", "P08_Shaker", "B01_Sub_Roll_F", "S02_Dub_Fm7_Echo", "S03_Dub_Bbm7_Echo", "T06_Tape_Hiss" }, "Klassiska kit" },
    { "Acid Lager", { "K03_Distorted", "K08_Rumble_Combo", "H01_Closed", "H03_Open", "C01_Clap_909ish", "C04_Snare_Noise_Industrial", "P01_Tom_Low", "P02_Tom_Mid", "P09_Blip", "P04_Metal_FM", "C05_Rim", "P08_Shaker", "B03_Acid_Squelch_F", "B04_Acid_Soft_F", "L08_Sync_Scream_F", "X05_Zap_Down" }, "Klassiska kit" },
    { "Rave 93", { "K07_Hard_Clipped", "K01_909_Punch", "H01_Closed", "H03_Open", "C01_Clap_909ish", "C03_Snare_Tight", "P02_Tom_Mid", "P03_Tom_High", "P13_Cowbell_Dark", "P05_Bell_FM", "C05_Rim", "P08_Shaker", "B08_Saw_Stab_Bass_F", "S05_Rave_Stab_Fm", "L01_Hoover_F", "X06_Siren" }, "Klassiska kit" },
    { "Tribal Mörker", { "K02_Deep_Long", "B07_Rumble_Tone_F", "H02_Closed_Short", "H08_Open_Long_Metal", "C02_Clap_Gated", "C03_Snare_Tight", "P01_Tom_Low", "P02_Tom_Mid", "P11_Conga_Low", "P12_Conga_High", "P14_Clave", "P08_Shaker", "B10_Square_Sub_F", "S09_Fifths_Stab_F", "T05_Metal_Drone_F", "V06_Whisper" }, "Klassiska kit" },
    { "Betongbunker", { "K07_Hard_Clipped", "K03_Distorted", "H05_Closed_12bit", "H03_Open", "C02_Clap_Gated", "C04_Snare_Noise_Industrial", "P15_Metal_Hit", "P01_Tom_Low", "P07_Woodblock_6bit", "P04_Metal_FM", "C05_Rim", "P08_Shaker", "B05_Reese_Dist_F", "S06_Crushed_Noise_Chord", "L08_Sync_Scream_F", "X11_Metal_Slam" }, "Klassiska kit" },
    { "Röstmaskin", { "K01_909_Punch", "K10_Click_Thump", "H01_Closed", "H03_Open", "C01_Clap_909ish", "C03_Snare_Tight", "P11_Conga_Low", "P12_Conga_High", "P06_Wood", "P10_Blip_Low", "C05_Rim", "P08_Shaker", "B06_FM_Bass_F", "V01_Ah_F", "V03_Vocoder_Ee_F", "V04_Formant_Chop" }, "Klassiska kit" },
    { "Glitch Labb", { "K05_Lofi_12bit", "K10_Click_Thump", "H05_Closed_12bit", "H06_Industrial_6bit", "C02_Clap_Gated", "C04_Snare_Noise_Industrial", "P07_Woodblock_6bit", "P10_Blip_Low", "P09_Blip", "P15_Metal_Hit", "P14_Clave", "P08_Shaker", "B09_Pluck_Bass_F", "S10_Skew_Detuned_Fm", "X09_Glitch_Granular", "X08_Radio_Scrape" }, "Klassiska kit" },
    { "Maskinrummet", { "K06_Industrial_Crushed", "K08_Rumble_Combo", "H06_Industrial_6bit", "H08_Open_Long_Metal", "C04_Snare_Noise_Industrial", "C03_Snare_Tight", "P15_Metal_Hit", "P16_Spring_Thud", "P07_Woodblock_6bit", "P04_Metal_FM", "C05_Rim", "P08_Shaker", "B07_Rumble_Tone_F", "S01_Dub_Fm7_Dry", "T10_Machine_Room", "X10_Tape_Stop" }, "Klassiska kit" },
    // Techno: hypnotiskt, industriellt och dub
    { "Rullande Tunnel", { "K02_Deep_Long", "K08_Rumble_Combo", "H02_Closed_Short", "H04_Ride", "C02_Clap_Gated", "C03_Snare_Tight", "P01_Tom_Low", "P02_Tom_Mid", "P11_Conga_Low", "P06_Wood", "C05_Rim", "P08_Shaker", "B01_Sub_Roll_F", "S08_Minor_Ring_Fm", "T04_Subway_Hum", "V06_Whisper" }, "Techno: hypnotiskt, industriellt och dub" },
    { "Hypnos Minimal", { "K04_Minimal_Tight", "K10_Click_Thump", "H02_Closed_Short", "H03_Open", "C01_Clap_909ish", "C03_Snare_Tight", "P11_Conga_Low", "P12_Conga_High", "P09_Blip", "P10_Blip_Low", "P14_Clave", "P08_Shaker", "B10_Square_Sub_F", "S10_Skew_Detuned_Fm", "T08_Drone_Dark_F", "X07_Laser" }, "Techno: hypnotiskt, industriellt och dub" },
    { "Varvet", { "K15_Hardgroove_909", "B12_Rumble_Breath_F", "H12_Hat_Tape_909", "H11_Ride_Crunch", "C07_Clap_Smash", "C09_Snare_Rost", "P21_Tom_Dist_Mid", "P19_Tom_Dist_Low", "P17_Metal_Pipe", "P13_Cowbell_Dark", "P22_Rim_Crack", "H13_Hat_Shuffle_Fat", "B16_Hoover_Bass_F", "S13_Hoover_Stab_Fm", "T05_Metal_Drone_F", "X12_Impact_Steel" }, "Techno: hypnotiskt, industriellt och dub" },
    { "Stålverk", { "K16_Anghammare", "K12_Rumble_Tunnel", "H09_Hard_Closed_Dist", "H08_Open_Long_Metal", "C09_Snare_Rost", "C07_Clap_Smash", "P20_Hammer_Clank", "P16_Spring_Thud", "P17_Metal_Pipe", "P04_Metal_FM", "P22_Rim_Crack", "H05_Closed_12bit", "B15_Grind_FM_Bass_F", "S12_Industrial_Stab_Fm", "T02_Industrial_Noise", "X12_Impact_Steel" }, "Techno: hypnotiskt, industriellt och dub" },
    { "Bunker 140", { "K13_Stampen", "B12_Rumble_Breath_F", "H09_Hard_Closed_Dist", "H10_Open_Trash", "C10_Clap_Stack_Hard", "C09_Snare_Rost", "P19_Tom_Dist_Low", "P15_Metal_Hit", "P13_Cowbell_Dark", "P04_Metal_FM", "P22_Rim_Crack", "H13_Hat_Shuffle_Fat", "B14_Dist_Saw_Bass_F", "S11_Rave_Stab_Dist_Fm", "L08_Sync_Scream_F", "X06_Siren" }, "Techno: hypnotiskt, industriellt och dub" },
    { "Ångvalsen", { "K11_Angvalt", "B12_Rumble_Breath_F", "H13_Hat_Shuffle_Fat", "H10_Open_Trash", "C07_Clap_Smash", "C09_Snare_Rost", "P19_Tom_Dist_Low", "P17_Metal_Pipe", "P20_Hammer_Clank", "P18_Anvil", "P22_Rim_Crack", "P08_Shaker", "B13_Growl_Bass_F", "S14_Dub_Stab_Dirty_Fm", "T11_Steamroller_138", "T10_Machine_Room" }, "Techno: hypnotiskt, industriellt och dub" },
    { "Dimhamn", { "K02_Deep_Long", "K09_Sub_Boom_F", "H05_Closed_12bit", "H07_Ride_Pitched_Verb", "C06_Clap_Layer_Verb", "C03_Snare_Tight", "P11_Conga_Low", "P12_Conga_High", "P06_Wood", "P05_Bell_FM", "C05_Rim", "P08_Shaker", "B10_Square_Sub_F", "S02_Dub_Fm7_Echo", "T01_Dark_Pad_Fm", "T06_Tape_Hiss" }, "Techno: hypnotiskt, industriellt och dub" },
    { "Djuphavet", { "K09_Sub_Boom_F", "K02_Deep_Long", "H02_Closed_Short", "H07_Ride_Pitched_Verb", "C06_Clap_Layer_Verb", "C03_Snare_Tight", "P11_Conga_Low", "P01_Tom_Low", "P06_Wood", "P10_Blip_Low", "P14_Clave", "P08_Shaker", "B01_Sub_Roll_F", "S03_Dub_Bbm7_Echo", "T08_Drone_Dark_F", "T09_Wind_Tunnel" }, "Techno: hypnotiskt, industriellt och dub" },
    // Rå och hård: ångvälts-techno
    { "Ångvälten", { "K11_Angvalt", "B12_Rumble_Breath_F", "H13_Hat_Shuffle_Fat", "H10_Open_Trash", "C07_Clap_Smash", "C09_Snare_Rost", "P19_Tom_Dist_Low", "P21_Tom_Dist_Mid", "P17_Metal_Pipe", "P20_Hammer_Clank", "P22_Rim_Crack", "P08_Shaker", "B13_Growl_Bass_F", "S11_Rave_Stab_Dist_Fm", "T11_Steamroller_138", "X12_Impact_Steel" }, "Rå och hård: ångvälts-techno" },
    { "Hardgroove 97", { "K15_Hardgroove_909", "B12_Rumble_Breath_F", "H12_Hat_Tape_909", "H10_Open_Trash", "C10_Clap_Stack_Hard", "C03_Snare_Tight", "P21_Tom_Dist_Mid", "P19_Tom_Dist_Low", "P13_Cowbell_Dark", "P17_Metal_Pipe", "P22_Rim_Crack", "H13_Hat_Shuffle_Fat", "B14_Dist_Saw_Bass_F", "S13_Hoover_Stab_Fm", "Lp09_Hardgroove_Perc_138", "X13_Riser_Grind_138" }, "Rå och hård: ångvälts-techno" },
    { "Rostfabriken", { "K14_Rost_Kick_F", "K12_Rumble_Tunnel", "H09_Hard_Closed_Dist", "H10_Open_Trash", "C09_Snare_Rost", "C04_Snare_Noise_Industrial", "P19_Tom_Dist_Low", "P15_Metal_Hit", "P20_Hammer_Clank", "P18_Anvil", "P22_Rim_Crack", "H06_Industrial_6bit", "B15_Grind_FM_Bass_F", "S12_Industrial_Stab_Fm", "L11_Alarm", "X12_Impact_Steel" }, "Rå och hård: ångvälts-techno" },
    { "Valsverk 140", { "K18_Tunga_Vals", "B12_Rumble_Breath_F", "H09_Hard_Closed_Dist", "H11_Ride_Crunch", "C07_Clap_Smash", "C10_Clap_Stack_Hard", "P19_Tom_Dist_Low", "P21_Tom_Dist_Mid", "P17_Metal_Pipe", "P04_Metal_FM", "P22_Rim_Crack", "H13_Hat_Shuffle_Fat", "B16_Hoover_Bass_F", "S13_Hoover_Stab_Fm", "L09_Acid_Dist_F", "X13_Riser_Grind_138" }, "Rå och hård: ångvälts-techno" },
    { "Lagerlokal", { "K17_Bunker_Long_F", "K12_Rumble_Tunnel", "H12_Hat_Tape_909", "H11_Ride_Crunch", "C08_Clap_Warehouse", "C09_Snare_Rost", "P01_Tom_Low", "P21_Tom_Dist_Mid", "P18_Anvil", "P13_Cowbell_Dark", "P22_Rim_Crack", "P08_Shaker", "B13_Growl_Bass_F", "S14_Dub_Stab_Dirty_Fm", "T11_Steamroller_138", "X12_Impact_Steel" }, "Rå och hård: ångvälts-techno" },
    { "Stålpressen", { "K13_Stampen", "K16_Anghammare", "H09_Hard_Closed_Dist", "H10_Open_Trash", "C10_Clap_Stack_Hard", "C09_Snare_Rost", "P20_Hammer_Clank", "P17_Metal_Pipe", "P18_Anvil", "P15_Metal_Hit", "P22_Rim_Crack", "H13_Hat_Shuffle_Fat", "B14_Dist_Saw_Bass_F", "S12_Industrial_Stab_Fm", "L10_Screech_F", "X12_Impact_Steel" }, "Rå och hård: ångvälts-techno" },
};
constexpr int kNumKits = (int) (sizeof (kKits) / sizeof (kKits[0]));

// Mönster: upp till 8 spår, "pad:takt:steg". takt = 16, 8, 16T, 32, 5, 7.
// Steg: . av, x på, X accent, o 50 %, p 25 %, 2/3/4 ratchet. Eller e:träffar:längd:rotation.
struct PatternPreset { const char* name; float swing; const char* lanes[8]; const char* group = nullptr; };

constexpr PatternPreset kPatterns[] = {
    { "Grund 130", 0.05f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "5:16:....x.......x...", "12:16:xgxgxgxgxgxgxgxg", "11:16:...x..x....x..x.", "13:16:..x...x...x...x.:n0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", nullptr }, "Techno" },
    { "Grund Rullande", 0.05f, { "1:16:X...x...x...x...", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "13:16:.xxx.xxx.xxx.xxx:n0,0,0,0,0,0,12,0,0,0,0,0,0,0,10,0", nullptr }, "Techno" },
    { "Lagerlokal", 0.0f, { "1:16:X...x...x...x...", "3:16:gxXggxXxgxXggxXx", "4:16:..X...X...X...o.", "5:16:....x.......x...", "11:16:..x..x..x..x..x.", "9:16:....x..x......x.", nullptr }, "Techno" },
    { "Betongpelare", 0.0f, { "1:16:X...x...x..gx...", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "6:16:....X.......X...", "12:16:xgxgxgxgxgxgxgxg", "14:16:......x.......x.", "13:16:..x...x...x...x.:n0,0,3,0,0,0,0,0,0,0,5,0,0,0,3,0", nullptr }, "Techno" },
    { "Nattskift", 0.08f, { "1:16:X...x...x...x...", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "5:16:....x.......x..g", "11:16:.x.....x..x.....", "9:16:x..x..x...x..x..", "13:16:..x...x...x..xx.:n0,0,0,0,0,0,7,0,0,0,0,0,0,10,12,0", nullptr }, "Techno" },
    { "Kolgruva", 0.0f, { "1:16:X...x...x...x...", "3:16:x.x.x.x.x.x.x.x.", "4:16:..X...X...X...X.", "6:16:....X.......X.2.", "9:16:x..x..x..x..x..x", "13:16:xx.xxx.xxx.xxx.x:n0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,-2", nullptr }, "Techno" },
    { "Hypnos", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "5:16:....x.......x...", "9:16:e:3:8:1", "10:16:e:5:13:0", "13:16:..x...x...x...x.", nullptr }, "Techno" },
    { "Detroit Grund", 0.15f, { "1:16:X...x...x...x...", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "10:16:x...............", "14:16:x.....x.....x...", "13:16:x..x..x...x..x..:n0,0,0,3,0,0,7,0,0,0,5,0,0,3,0,0", nullptr }, "Techno" },
    { "Ståldrift", 0.0f, { "1:16:X...x...x...x...", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...o.", "5:16:....x.......x...", "11:16T:..x..x..x..x..x.", "13:16:.xxx.xxx.xxx.xxx:n0,0,0,0,0,0,0,0,3,3,3,0,5,5,5,0", nullptr }, "Techno" },
    { "Fabriksgolv Ren", 0.0f, { "1:16:X...x...x...x..g", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "6:16:....X.......X...", "9:16:...x...x...x.x.x", "12:16:xgxgxgxgxgxgxgxg", "13:16:..x...x...x...x.:n0,0,0,0,0,0,-2,0,0,0,0,0,0,0,3,0", nullptr }, "Techno" },
    { "Mörkt Valv", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "5:16:....x.......x...", "15:32:x...............x...............", "13:16:x.x.x.x.x.x.x.x.:n0,0,12,0,0,0,12,0,0,0,10,0,0,0,7,0", nullptr }, "Techno" },
    { "Pump 130", 0.0f, { "1:16:X...x...x...x.x.", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "5:16:....x.......x.g.", "11:16:...x..x....x..x.", "12:16:xgxgxgxgxgxgxgxg", "13:16:..x...x...x...x.:n0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0", nullptr }, "Techno" },
    { "Tunnelljus", 0.05f, { "1:16:X...x...x...x...", "3:16:gxXggxXxgxXggxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "14:16:......x.........", "16:32:x...............x...............", "13:16:..x...x...x...x.", nullptr }, "Techno" },
    { "Peak Grund", 0.0f, { "1:16:X...x...x...xx..", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "6:16:....X.......X.2.", "13:16:.xxx.xxx.xxx.xxx:n0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", nullptr }, "Peak time" },
    { "Klubbens Topp", 0.0f, { "1:16:X...x...x...x...", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "12:16:xgxgxgxgxgxgxgxg", "9:16:x.x..x.x..x.x..x", "13:16:.xxx.xxx.xxx.xxx:n0,0,0,0,0,0,0,0,0,3,3,3,0,5,5,5", nullptr }, "Peak time" },
    { "Lyftet", 0.0f, { "1:16:X...x...x...x...", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "5:16:....x.......x.xx", "6:16:............2222", "13:16:.xxx.xxx.xxx.xxx", nullptr }, "Peak time" },
    { "Stroboskop", 0.0f, { "1:16:X...x...x...x...", "3:32:xxxxxxxxxxxxxxxx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "15:8:x...x...x...x...", "13:16:.xxx.xxx.xxx.xxx:n0,12,0,0,0,12,0,0,0,12,0,0,0,10,0,0", nullptr }, "Peak time" },
    { "Händerna Upp", 0.0f, { "1:16:X...x...x...x...", "3:16:gxXggxXxgxXggxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "16:16:x...............................", "14:16:x.....x.....x...", "13:16:..x...x...x...x.", nullptr }, "Peak time" },
    { "Maskinen Går", 0.0f, { "1:16:X...x...x...xx..", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...o.", "6:16:....X.......X...", "9:16:x..x..x..x..x...", "10:16:..x..x..x..x..x.", "13:16:.xx..xx..xx..xx.:n0,0,0,0,0,3,3,0,0,0,0,0,0,-2,-2,0", nullptr }, "Peak time" },
    { "Sista Timmen", 0.0f, { "1:16:X...x...x...x.x.", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "12:16:xgxgxgxgxgxgxgxg", "14:16:..........x.....", "13:16:.xxx.xxx.xxx.xxx:n0,0,0,0,0,0,0,0,7,7,7,0,5,5,5,0", nullptr }, "Peak time" },
    { "Gnistor", 0.0f, { "1:16:X...x...x...x...", "3:16:xxXxxxXxxxXxxxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "11:16T:x.x.x.x.x.x.x.x.", "13:16:..x...x...x...x.", nullptr }, "Peak time" },
    { "Hardgroove Grund", 0.18f, { "1:16:X...x...x...x..x", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "6:16:....X.......X...", "7:16:...x..x....x..x.", "8:16:.x.....x..x.....", "11:16:..x..x..x..x..x.", "12:16:xgxgxgxgxgxgxgxg" }, "Hardgroove" },
    { "Groove 97", 0.22f, { "1:16:X...x...x...x..x", "3:16:gxXggxXxgxXggxXx", "4:16:..X...X...X...o.", "5:16:....x.......x...", "7:16:..x..x....x..x..", "9:16:x...x..x..x...x.", "13:16:x..x..x...x..x..:n0,0,0,0,0,0,3,0,0,0,0,0,0,5,0,0", nullptr }, "Hardgroove" },
    { "Tom Tom Fabrik", 0.2f, { "1:16:X...x...x...x...", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "7:16:x..x..x..x..x..x", "8:16:..x..x..x..x..x.", "6:16:....X.......X...", "11:16:...x..x....x..x.", nullptr }, "Hardgroove" },
    { "Svettig Vägg", 0.25f, { "1:16:X...x...x...x..g", "3:16:gxgxgxgxgxgxgxgx", "4:16:..X...X...X...X.", "5:16:....x.......x..g", "9:16:..xx..x...xx..x.", "10:16:x....x....x....x", "13:16:.x.x.x.x.x.x.x.x", nullptr }, "Hardgroove" },
    { "Loopkross", 0.2f, { "1:16:X...x...x...x..x", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "6:16:......x.......x.", "9:16:x.xx.x.xx.x.xx.x", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Hardgroove" },
    { "Cowbell Krig", 0.15f, { "1:16:X...x...x...x...", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "9:16:x..x..x...x..x..", "11:16:..x..x..x..x..x.", "13:16:..x...x...x...x.", nullptr }, "Hardgroove" },
    { "Rostig Swing", 0.3f, { "1:16:X...x...x...x..x", "3:16:gxgxgxgxgxgxgxgx", "4:16:..X...X...X...o.", "6:16:....X.......X.2.", "7:16:...x.......x....", "8:16:.......x.......x", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Hardgroove" },
    { "Snurr 909", 0.18f, { "1:16:X...x...x...x...", "3:16:gxXggxXxgxXggxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "7:32:x.x.x.x.........", "8:32:........x.x.x.x.", "11:16:.x.....x..x.....", nullptr }, "Hardgroove" },
    { "Svensk Hammare", 0.12f, { "1:16:X...x...x...x..x", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "5:16:....x.......x...", "9:16:xx.x.xx.xx.x.xx.", "13:16:.xxx.xxx.xxx.xxx", nullptr }, "Hardgroove" },
    { "Tech House Grund", 0.1f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "5:16:....x.......x...", "12:16:xgxgxgxgxgxgxgxg:s32", "9:16:...x......x..x..:s32", "13:16:..x..x....x..x..:n0,0,0,0,0,3,0,0,0,0,0,0,0,5,0,0:s32", nullptr }, "Tech house" },
    { "Shuffle Söndag", 0.0f, { "1:16:X...x...x...x...", "3:16:gxgxgxgxgxgxgxgx:s40", "4:16:..X...X...X...X.", "5:16:....x.......x...", "9:16:..x....x..x.....:s40", "13:16:..x.x...x.x...xx:n0,0,0,0,3,0,0,0,0,0,5,0,0,0,7,5:s40", nullptr }, "Tech house" },
    { "Bongo Groove", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "5:16:....x.......x...", "9:16:x..x..x...x.x...:s32", "10:16:..x..x..x.....x.:s32", "12:16:xgxgxgxgxgxgxgxg:s32", "13:16:..x...x...x...x." }, "Tech house" },
    { "Orgelbas", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "5:16:....x.......x...", "12:16:xgxgxgxgxgxgxgxg:s32", "14:16:...x.......x....:s32", "13:16:x..x..x...x..x..:n0,0,0,12,0,0,10,0,0,0,7,0,0,5,0,0:s32", nullptr }, "Tech house" },
    { "Fingerknäpp", 0.0f, { "1:16:X...x...x...x...", "3:16:gxXxgxXxgxXxgxXx:s32", "4:16:..X...X...X...X.", "5:16:....x.......x...", "11:16:..x..x..x..x.x..:s32", "13:16:..x...xx..x...x.:n0,0,0,0,0,0,3,5,0,0,0,0,0,0,7,0", nullptr }, "Tech house" },
    { "Takterrass", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "5:16:....x.......x..g", "9:16:x...x.x...x...x.:s36", "10:16:...x...x...x..x.:s36", "12:16:xgxgxgxgxgxgxgxg:s36", "13:16:..x...x...x...x.:n0,0,0,0,0,0,3,0,0,0,0,0,0,0,-2,0" }, "Tech house" },
    { "Hey Hey", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "5:16:....x.......x...", "16:16:..............x.", "12:16:xgxgxgxgxgxgxgxg:s32", "13:16:x.xx..x.x.xx..x.:n0,0,0,3,0,0,0,0,0,0,5,3,0,0,0,0:s32", nullptr }, "Tech house" },
    { "Tamburin Natt", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "5:16:....x.......x...", "10:16:..x...x...x...x.", "9:16:x..x..x..x..x..x:s32", "13:16:..x...x...x..xx.:n0,0,0,0,0,0,0,0,0,0,0,0,0,3,5,0", nullptr }, "Tech house" },
    { "Gummibas", 0.0f, { "1:16:X...x...x...x...", "3:16:gxXxgxXxgxXxgxXx:s32", "4:16:..X...X...X...X.", "5:16:....x.......x...", "12:16:xgxgxgxgxgxgxgxg:s32", "13:16:x..xx..x..x.x...:n0,0,0,0,12,0,0,10,0,0,7,0,5,0,0,0:s32", nullptr }, "Tech house" },
    { "Marimba Sol", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "5:16:....x.......x...", "15:16:x..x..x...x..x..:n0,0,0,3,0,0,7,0,0,0,10,0,0,7,0,0:s32", "12:16:xgxgxgxgxgxgxgxg:s32", "13:16:..x...x...x...x.", nullptr }, "Tech house" },
    { "Sen Kväll", 0.0f, { "1:16:X...x...x...x...", "3:16:gxgxgxgxgxgxgxgx:s28", "4:16:..X...X...X...o.", "5:16:....x.......x...", "9:16:...x..x......x..:s28", "13:16:..x...x...x...x.:n0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0", nullptr }, "Tech house" },
    { "Klapp Klapp", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "5:16:....x..x....x..x", "12:16:xgxgxgxgxgxgxgxg:s32", "13:16:.x.x.x.x.x.x.x.x:n0,0,0,0,0,0,0,0,3,0,3,0,5,0,5,0", nullptr }, "Tech house" },
    { "Tribal Grund", 0.08f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "5:16:....x.......x...", "7:16:x..x..x...x..x..", "8:16:..x..x....x.x...", "9:16:x..x.x..x..x.x..", "12:16:xgxgxgxgxgxgxgxg", "13:16:..x...x...x...x." }, "Tribal" },
    { "Djembe Cirkel", 0.1f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "7:16:x.....x...x.....", "8:16:..x.x...x...x.x.", "9:16:...x..x....x..x.", "10:16:x.x.....x.x.....", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Tribal" },
    { "Trummor i Natten", 0.05f, { "1:16:X...x...x...x...", "7:16:x..x..x..x..x...", "8:16:..x..x..x..x..x.", "9:16:x..", "10:16:x....", "3:16:..x...x...x...x.", "5:16:....x.......x...", nullptr }, "Tribal" },
    { "Tre mot Fyra", 0.0f, { "1:16:X...x...x...x...", "9:16:x..x..x..x..", "10:16:x...x...x...", "7:16:x.....x.....", "3:16:..x...x...x...x.", "12:16:xgxgxgxgxgxgxgxg", "5:16:....x.......x...", nullptr }, "Tribal" },
    { "Kongadans", 0.12f, { "1:16:X...x...x...x...", "9:16:x..x..x...x.x...", "10:16:..x..x..x.....x.", "7:16:...x.......x....", "3:16:..x...x...x...x.", "4:16:..X...X...X...X.", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Tribal" },
    { "Afro 3-3-2", 0.05f, { "1:16:X...x...x...x...", "15:16:x..x..x.x..x..x.", "9:16:..x..x....x..x..", "10:16T:x.x.x.x.x.x.x.x.", "3:16:..x...x...x...x.", "5:16:....x.......x...", "13:16:x..x..x.x..x..x.:n0,0,0,3,0,0,7,0,5,0,0,3,0,0,0,0", nullptr }, "Tribal" },
    { "Talande Trumman", 0.0f, { "1:16:X...x...x...x...", "16:16:...x.......x....", "9:16:x..x..x...x..x..", "7:16:..x...x...x...x.", "3:16:..x...x...x...x.", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Tribal" },
    { "Logtrummor", 0.0f, { "1:16:X...x...x...x...", "15:16:x..x..x..x..x.x.:n0,0,0,3,0,0,7,0,0,5,0,0,3,0,0,0", "9:16:...x..x....x..x.", "3:16:..x...x...x...x.", "5:16:....x.......x...", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Tribal" },
    { "Femtakt Ritual", 0.0f, { "1:16:X...x...x...x...", "7:16:e:3:5:0", "8:16:e:2:5:2", "9:16:e:4:7:0", "3:16:..x...x...x...x.", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Tribal" },
    { "Sju mot Fyra", 0.0f, { "1:16:X...x...x...x...", "9:16:e:3:7:0", "10:16:e:4:7:3", "7:16:x...x...x...x...", "3:16:..x...x...x...x.", "5:16:....x.......x...", nullptr }, "Tribal" },
    { "Dubfire Djup", 0.1f, { "1:16:X...x...x...x...", "7:16:x..x..x...x..x..", "8:16:...x..x.x.....x.", "11:16:...x..x....x..x.", "3:16:gxXxgxXxgxXxgxXx", "4:16:..X...X...X...X.", "13:16:.x.x.x.x.x.x.x.x", nullptr }, "Tribal" },
    { "Djungelgolv Ren", 0.15f, { "1:16:X...x...x...x...", "9:16:e:7:16:0", "10:16:e:5:16:3", "12:16:xgxgxgxgxgxgxgxg", "3:16:..x...x...x...x.", "5:16:....x.......x...", "13:16:..x...x...x...x.", nullptr }, "Tribal" },
    { "Agogo Gata", 0.08f, { "1:16:X...x...x...x...", "9:16:x.x.xx.x.x.xx.x.", "10:16:..x....x..x....x", "3:16:..x...x...x...x.", "5:16:....x.......x...", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Tribal" },
    { "Minimal Grund", 0.08f, { "1:16:X...x...x...x...", "3:16:..x...x...x...xg", "11:16:.....x.......x..", "9:16:e:3:16:2", "13:16:x.......x..x....:n0,0,0,0,0,0,0,0,3,0,0,5,0,0,0,0", nullptr }, "Minimal" },
    { "Klick Klock", 0.1f, { "1:16:X...x...x...x...", "9:16:...x..x...x..x..", "10:16:..x.....x.....x.", "3:16:..g...x...g...x.", "16:16:...............x", nullptr }, "Minimal" },
    { "Tyst Rum", 0.05f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "11:16:......x......x..", "14:16:........x.......", "13:16:x..........x....:n0,0,0,0,0,0,0,0,0,0,0,-2,0,0,0,0", nullptr }, "Minimal" },
    { "Mikro", 0.0f, { "1:16:X...x...x...x...", "3:32:xxxxxxxxxxxxxxxx", "9:16:e:5:13:0", "10:16:e:3:11:2", "5:16:............x...", nullptr }, "Minimal" },
    { "Spöke", 0.12f, { "1:16:X...x...x...x...", "3:16:gxgxgxgxgxgxgxgx", "6:16:....g.......g...", "9:16:...o...o..o...o.", "13:16:..x.....x.x.....:n0,0,0,0,0,0,0,0,3,0,-2,0,0,0,0,0", nullptr }, "Minimal" },
    { "Plastik Grund", 0.0f, { "1:16:X...x...x...x...", "9:16:x..x..x...x..x..", "3:16:..x..x..x..x", "13:16:X.x2x.xxo.x.Xxx.:n0,0,12,0,0,0,10,0,0,0,7,0,0,0,12,0", nullptr }, "Minimal" },
    { "Kontakt", 0.0f, { "1:16:X...x...x...x...", "11:16:.x.....x..x.....", "9:32:x...............", "3:16:..x...x...x...x.", "14:16:..............x.", nullptr }, "Minimal" },
    { "Blindskär", 0.06f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "9:16:e:4:15:0", "10:16:e:3:16:5", "13:16:x...x.....x.....:n0,0,0,0,7,0,0,0,0,0,5,0,0,0,0,0", nullptr }, "Minimal" },
    { "Dub Grund", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "12:16:xgxgxgxgxgxgxgxg", "14:16:..x.....x..x....", "11:16:...x.......x....", "13:16:x.......x.......:n0,0,0,0,0,0,0,0,-2,0,0,0,0,0,0,0", nullptr }, "Dub" },
    { "Ekokammare Ren", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "14:16:...x.......x....", "15:16:x...............x...............", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Dub" },
    { "Dimma 122", 0.05f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "5:16:............x...", "14:16:...x..x....x....", "13:16:x...........x...:n0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0", nullptr }, "Dub" },
    { "Kanal Djup", 0.0f, { "1:16:X...x...x...x...", "3:16:gxXxgxXxgxXxgxXx", "14:16:..x.....x..x....", "11:16:.x.....x..x.....", "13:16:x.......x.......:n0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0", nullptr }, "Dub" },
    { "Morgondimma", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "14:16:o.....o.....o...", "16:16:x...............................", "13:16:x...............:n0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", nullptr }, "Dub" },
    { "Basic Ren", 0.0f, { "1:16:X...x...x...x...", "3:16:gxXggxXxgxXggxXx", "4:16:..X...X...X...X.", "14:16:...x..x...x..x..", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Dub" },
    { "Fem mot Sju", 0.0f, { "1:16:X...x...x...x...", "9:16:e:3:5:0", "10:16:e:4:7:0", "3:16:..x...x...x...x.", "5:16:....x.......x...", "13:16:x..x.x..x..x.x..:n0,0,0,3,0,5,0,0,0,0,0,3,0,0,0,0", nullptr }, "Polyrytm" },
    { "Glidande Hats", 0.0f, { "1:16:X...x...x...x...", "3:16:x.xx.x.xx.xxx", "4:16:..x..x..x..x..x", "5:16:....x.......x...", "13:16:..x...x...x...x.", nullptr }, "Polyrytm" },
    { "Kvintoler", 0.0f, { "1:16:X...x...x...x...", "3:5:gxXgx", "5:16:....x.......x...", "9:16:x..x..x.", "13:16:..x...x...x...x.", nullptr }, "Polyrytm" },
    { "Septoler", 0.0f, { "1:16:X...x...x...x...", "3:7:gxgXgxg", "5:16:....x.......x...", "4:16:..X...X...X...X.", "13:16:..x...x...x...x.", nullptr }, "Polyrytm" },
    { "Trasslet", 0.0f, { "1:16:X...x...x...x...", "13:16:x.xx.x.xx.x:n0,0,3,0,0,7,0,0,5,0,3", "15:16:x..x.x.:n12,0,0,10,0,7,0", "3:16:..x...x...x...x.", "5:16:....x.......x...", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Polyrytm" },
    { "Euklid Väv", 0.0f, { "1:16:X...x...x...x...", "7:16:e:3:8:0", "8:16:e:5:13:2", "9:16:e:7:16:1", "10:16:e:4:11:3", "3:16:..x...x...x...x.", "5:16:....x.......x...", nullptr }, "Polyrytm" },
    { "Glitch Grund", 0.0f, { "1:16:X...x...x...x...", "3:16:xx3x.xx2xx.x4xx.", "5:16:....x.......x...", "9:16:o.o.p.o.o.p.o.p.", "13:16:..x...x...x...x.", nullptr }, "Polyrytm" },
    { "Svajig Maskin", 0.0f, { "1:16:X...x...x...x...", "3:16:oxoXoxoXoxoXoxoX", "4:16:..o...o...o...o.", "5:16:....x.......x...", "9:16:p.p.o.p.p.o.p.p.", "13:16:..x...x...x...x.", nullptr }, "Polyrytm" },
    { "Fasglidning", 0.0f, { "1:16:X...x...x...x...", "9:16:x..x.xx..x.x", "10:16:x..x.xx..x.x.", "3:16:..x...x...x...x.", "5:16:....x.......x...", nullptr }, "Polyrytm" },
    { "Elva Steg", 0.0f, { "1:16:X...x...x...x...", "13:16:x..x.x..x.x:n0,0,0,3,0,7,0,0,5,0,10", "3:16:..x...x...x...x.", "5:16:....x.......x...", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Polyrytm" },
    { "Ratchet Regn", 0.0f, { "1:16:X...x...x...x...", "3:16:x2x3x2x4x2x3x2x4", "5:16:....x.......x...", "11:16:....2.......3...", "13:16:..x...x...x...x.", nullptr }, "Polyrytm" },
    { "Triolbas", 0.0f, { "1:16:X...x...x...x...", "13:16T:x.xx.xx.xx.x:n0,0,0,3,0,0,5,0,0,7,0,0", "3:16:..x...x...x...x.", "5:16:....x.......x...", "12:16:xgxgxgxgxgxgxgxg", nullptr }, "Polyrytm" },
    // Klassiskt 90-tal
    { "Detroit Morgon", 0.15f, { "1:16:X...x...x...x...", "4:16:..x...x...x...x.", "5:16:....x.......x...", "3:16:x.xxx.xxx.xxx.xx", "13:16:x..x..x...x..x..", "14:16:......x.......o.", nullptr }, "Klassiskt 90-tal" },
    { "Berlin Lager", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "3:16:xxXxxxXxxxXxxxXx", "11:16:...x..x....x..x.", "13:16:..x...x...x...x.", nullptr }, "Klassiskt 90-tal" },
    { "909 Grund", 0.1f, { "1:16:X...x...x...x...", "4:16:..x...x...x...x.", "5:16:....x.......x...", "3:16:x.x.x.x.x.x.x.x.", "8:16:.............................x3x", nullptr }, "Klassiskt 90-tal" },
    { "Swingig 909", 0.45f, { "1:16:X...x...x...x...", "4:16:..x...x...x...x.", "5:16:....x.......x...", "3:16:xxxxxxxxxxxxxxxx", "12:16:.x.x.x.x.x.x.x.x", nullptr }, "Klassiskt 90-tal" },
    { "Rave 93", 0.0f, { "1:16:X...x...x...x...", "4:16:..x...x...x...x.", "5:16:....x.......x..4", "14:16:x.....x.....x...", "15:16:x...............................", "13:16:x.xx.x.xx.x.xx.x", nullptr }, "Klassiskt 90-tal" },
    { "Ride Rusning", 0.05f, { "1:16:X...x...x...x...", "10:8:xxxxxxxx", "4:16:..x...x...x...x.", "5:16:....x.......x...", "13:16:..x...x...x..xx.", nullptr }, "Klassiskt 90-tal" },
    { "Tung Fyra", 0.0f, { "1:16:X...x...x...x...", "6:16:....X.......X...", "12:16:xxxxxxxxxxxxxxxx", "4:16:..x...x...x...x.", "2:16:x...x...x...x...", nullptr }, "Klassiskt 90-tal" },
    // Plastikman minimal
    { "Plast Klick", 0.08f, { "1:16:X...x...x...x...", "9:16:e:5:16:0", "13:16:x.x..x.xX..x.x.x", "3:12:..x..x..x..x", "10:16:e:3:16:2", nullptr }, "Plastikman minimal" },
    { "Konsumerad", 0.0f, { "1:16:x.......x.......", "15:16:x...............................", "11:7:x..x...", "2:16:x...............", "10:16:e:2:16:5", nullptr }, "Plastikman minimal" },
    { "Plast Ark", 0.0f, { "1:16:X...x...x...x...", "13:16:X.x2x.xxo.x.Xxx.", "3:12:..x..x..x..x", "11:16:...x.......x..o.", "14:16:..........x.....", nullptr }, "Plastikman minimal" },
    { "Fem mot Tolv", 0.0f, { "1:16:X...x...x...x...", "9:5:x.x..", "7:12:x..x.x..x.x.", "3:16:..x...x...x...x.", "13:16:x...x..x..x.x...", nullptr }, "Plastikman minimal" },
    { "Spritz", 0.12f, { "1:16:X...x...x...x...", "9:16:oo.o.po.o.op.o.p", "10:16:..3...p...2...o.", "3:16:..x...x...x...x.", "13:16:x..x....x..x....", nullptr }, "Plastikman minimal" },
    { "Tom Konga Svep", 0.1f, { "1:16:X...x...x...x...", "7:16:x..x..x...x..x..", "8:16:..x...x.x...x..x", "9:16:.x...x.....x..x.", "3:16:..x...x...x...x.", nullptr }, "Plastikman minimal" },
    { "Minimal Drift", 0.0f, { "1:16:X...x...x...x...", "9:15:x...x...x...x..", "3:16:..x...x...x...x.", "13:17:x..x..x..x..x..x.", "11:16:....x.......x...", nullptr }, "Plastikman minimal" },
    // Svensk loop-techno
    { "Stockholm Loop", 0.05f, { "1:16:X...x...x...x...", "15:16:x...............", "3:16:..x...x...x...x.", "5:16:....x.......x...", "8:16:...x..x....x..x.", "12:16:xxxxxxxxxxxxxxxx", nullptr }, "Svensk loop-techno" },
    { "Södermalm Funk", 0.3f, { "1:16:X...x...x...x...", "7:16:x..x..x.x..x..x.", "5:16:..x...x...x...x.", "3:16:xxxxxxxxxxxxxxxx", "13:16:..x..x..x..x..x.", nullptr }, "Svensk loop-techno" },
    { "Kod Rull", 0.0f, { "1:16:X...x...x...x...", "7:16:xxxxxxxxxxxxxxxx", "8:16:..x...x...x..xx.", "4:16:..x...x...x...x.", "5:16:....x.......x...", nullptr }, "Svensk loop-techno" },
    { "Hammarby Hammare", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "15:16:x.......x.......", "4:16:..X...X...X...X.", "5:16:....x.......x...", "11:16:.x..x..x.x..x..x", nullptr }, "Svensk loop-techno" },
    { "Själsgroove", 0.4f, { "1:16:X...x...x...x...", "12:16:x.x.x.x.x.x.x.x.", "5:16:....x.......x...", "9:16:..x.....x.x.....", "13:16:..x...x...x...x.", nullptr }, "Svensk loop-techno" },
    { "Kalla Fötter", 0.0f, { "1:16:X...x...x...x...", "11:16T:x..x..x..x..x..x", "3:16:..x...x...x...x.", "15:16:x...............", "13:16:..x...x...x..x..", nullptr }, "Svensk loop-techno" },
    { "Loopkapning", 0.05f, { "1:16:X...x...x...x...", "15:16:xoxoxoxoxoxoxoxo", "16:16:...o......o..o..", "4:16:..x...x...x...x.", "5:16:....x.......x...", nullptr }, "Svensk loop-techno" },
    { "Göteborg Grus", 0.0f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "15:8:x.x.x.x.", "6:16:....x.......x...", "10:16:.x..x...x..x..x.", nullptr }, "Svensk loop-techno" },
    // Dubfire tribal tech
    { "Tribal Mörker", 0.08f, { "1:16:X...x...x...x...", "7:16:x..x..x...x..x..", "8:12:..x..x.x..x.", "9:7:x.x.x..", "3:16:..x...x...x...x.", "13:16:..x...x...x...x.", nullptr }, "Dubfire tribal tech" },
    { "Rullande Bas", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "13:16:.xxx.xxx.xxx.xxx", "9:16:..x...x...x...x.", "3:16:xxxxxxxxxxxxxxxx", nullptr }, "Dubfire tribal tech" },
    { "Djungelgolv", 0.15f, { "1:16:X...x...x...x...", "9:16:e:7:16:0", "10:16:e:5:16:3", "12:16:xxxxxxxxxxxxxxxx", "4:16:..x...x...x...x.", nullptr }, "Dubfire tribal tech" },
    { "Mörk Tech", 0.0f, { "1:16:X...x...x...x...", "11:16:e:3:16:1", "3:16:..x...x...x...x.", "13:16:..x...x...x...x.", "14:16:...........x....", nullptr }, "Dubfire tribal tech" },
    { "Spänningsbygge", 0.0f, { "1:16:X...x...x...x...", "9:16:x.x.x.x.x2x2x3x4", "3:16:..x...x...x...x.", "5:16:....x.......x...", "16:16:x...............................", nullptr }, "Dubfire tribal tech" },
    { "Ritual", 0.05f, { "1:16:X...x...x...x...", "7:5:x..x.", "8:7:x.x..x.", "9:9:x..x.x...", "3:16:..x...x...x...x.", nullptr }, "Dubfire tribal tech" },
    { "Klubbens Kärna", 0.1f, { "1:16:X...x...x...x...", "5:16:....x.......x...", "9:16:oxoxoxoxoxoxoxox", "3:16:..x...x...x...x.", "13:16:..x...x...x...x.", nullptr }, "Dubfire tribal tech" },
    // Industrial
    { "Industriell 909", 0.2f, { "1:16:X...x...x...x...", "3:16:..x...x...x...x.", "7:16:...x..x....x..x.", "9:16:.....x.......x..", "12:16:......x.......x.", "6:16:....x.......x...", nullptr }, "Industrial" },
    { "Fabriksgolv", 0.0f, { "1:16:X...x...x...x...", "6:16:....X.......X...", "3:16:xxXxxxXxxxXxxxXx", "2:16:x...x...x...x...", "16:16:........................x.......", nullptr }, "Industrial" },
    { "Stålhall", 0.0f, { "1:16:X...x...x...x...", "10:8:e:5:8:0", "3:16:..x...x...x...x.", "15:16:x...............", nullptr }, "Industrial" },
    { "Rost", 0.05f, { "1:16:X...x...x...x...", "7:16:..x..x....x..x..", "5:16:....x.......x...", "9:16:e:5:16:2", "3:16:..x...x...x...x.", nullptr }, "Industrial" },
    { "Maskinrummet", 0.0f, { "1:16:X...x...x...x...", "3:16:oooooooooooooooo", "9:16:xoxoxoxoxoxoxoxo", "10:16:o.o.o.o.o.o.o.o.", "7:16:o..o..o..o..o..o", "15:16:x...............................", nullptr }, "Industrial" },
    { "Betongpuls", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "3:16T:..x..x..x..x..x.", "6:16:....x.......x...", "13:16:..x...x...x...x.", nullptr }, "Industrial" },
    // Dub techno
    { "Kanal Ett", 0.0f, { "1:16:X...x...x...x...", "14:16:..x.....x..x....", "3:16:..x...x...x...x.", "12:16:.x.x.x.x.x.x.x.x", "13:16:x.......x.......", nullptr }, "Dub techno" },
    { "Dimbank", 0.0f, { "1:16:X...x...x...x...", "14:16:...x.......x....", "11:7:x..x...", "3:16:..x...x...x...x.", "16:16:x...............................", nullptr }, "Dub techno" },
    { "Djup Dub", 0.05f, { "1:16:x...x...x...x...", "14:16:..x.............................", "3:16:..x...x...x...x.", "15:16:x...............................", "13:16:x...........x...", nullptr }, "Dub techno" },
    { "Ekolod", 0.0f, { "1:16:X...x...x...x...", "11:16T:x...x...x...x...", "14:16:...x.....x......", "3:16:..x...x...x...x.", nullptr }, "Dub techno" },
    { "Morgongrått", 0.0f, { "1:16:x...x...x...x...", "14:16:o.....o.....o...", "3:16:..p...x...p...x.", "15:16:x...............................", nullptr }, "Dub techno" },
    // Hårt och snabbt
    { "Peak Time", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "3:16:xxXxxxXxxxXxxxXx", "5:16:....x.......x...", "13:16:..x...x...x...x.", nullptr }, "Hårt och snabbt" },
    { "Hård Industri", 0.0f, { "1:16:X...x...x...x..x", "5:16:....x.......x..4", "3:16:..x...x...x...x.", "9:16:x..x..x..x..x..x", nullptr }, "Hårt och snabbt" },
    { "Rave Larm", 0.0f, { "1:16:X...x...x...x...", "13:16:X.x2x.xxX.x.Xxx.", "16:16:x...............................", "4:16:..x...x...x...x.", "5:16:....x.......x...", nullptr }, "Hårt och snabbt" },
    { "Bunkerdörr", 0.0f, { "1:16:X...x...x...x...", "9:16:2.2.2.2.2.2.2.3.", "3:16:..x...x...x...x.", "6:16:....x.......x...", nullptr }, "Hårt och snabbt" },
    { "Snabb Rumble", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "4:16:..x...x...x...x.", "12:16:xxxxxxxxxxxxxxxx", nullptr }, "Hårt och snabbt" },
    // Polyrytm och experiment
    { "Euklid 3-5-7", 0.0f, { "1:16:X...x...x...x...", "7:8:e:3:8:0", "9:13:e:5:13:0", "10:16:e:7:16:0", "3:16:..x...x...x...x.", nullptr }, "Polyrytm och experiment" },
    { "Kvintol-svep", 0.0f, { "1:16:X...x...x...x...", "3:5:xxxxxxxxxx", "13:16:..x...x...x...x.", "5:16:....x.......x...", nullptr }, "Polyrytm och experiment" },
    { "Septol-rost", 0.0f, { "1:16:X...x...x...x...", "9:7:x.xx.x.x.xx.x", "3:16:..x...x...x...x.", "5:16:....x.......x...", nullptr }, "Polyrytm och experiment" },
    { "Fasförskjutning", 0.0f, { "1:16:X...x...x...x...", "9:12:x..x.xx..x.x", "10:13:x..x.xx..x.x.", "3:16:..x...x...x...x.", nullptr }, "Polyrytm och experiment" },
    { "Kaos Kontrollerat", 0.2f, { "1:16:X...x...x...x...", "9:11:oxo.o.x.op.", "10:9:p.o.x.o.p", "7:7:o..x.o.", "13:13:x.o.x..o.x.p.", "3:16:o.x.o.x.o.x.o.x.", nullptr }, "Polyrytm och experiment" },
    // Hypnotiskt och rullande
    { "Rullande Tunnel", 0.05f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "3:16:xxXxxxXxxxXxxxXx", "4:16:..x...x...x...x.", "13:16:.xxx.xxx.xxx.xxx", "10:16:e:5:16:3", "12:16:.x.x.x.x.x.x.x.x", "15:16:x..............................." }, "Hypnotiskt och rullande" },
    { "Hypnos 128", 0.1f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "3:16:..x...x...x...x.", "4:16:..x...........x.", "9:8:e:3:8:1", "13:16:..x...x...x...xx", "11:16:...x.......x..o.", "15:16:x..............................." }, "Hypnotiskt och rullande" },
    { "Tunnelseende", 0.0f, { "1:16:X...x...x...x...", "13:16:.x.x.x.x.x.x.x.x", "3:16:xxxxxxxxxxxxxxxx", "11:16:e:3:16:2", "4:16:..x...x...x...x.", "14:16:..............................x.", nullptr }, "Hypnotiskt och rullande" },
    { "Spiral 7/13", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "9:7:x..x.x.", "10:16:e:9:16:1", "13:13:x..x..x.x..x.", "3:16:..x...x...x...x.", nullptr }, "Hypnotiskt och rullande" },
    { "Snurr", 0.08f, { "1:16:X...x...x...x...", "3:16T:x.xx.xx.xx.xx.xx", "4:16:..x...x...x...x.", "7:16:..x...x...x..x..", "13:16:.x..x.x..x..x.x.", "12:16:oxoxoxoxoxoxoxox", nullptr }, "Hypnotiskt och rullande" },
    { "Ändlös Ride", 0.0f, { "1:16:X...x...x...x...", "4:8:xxxxxxxx", "3:16:..x...x...x...x.", "2:16:x...x...x...x...", "13:16:..x...x...x...x.", "11:16:e:5:16:0", nullptr }, "Hypnotiskt och rullande" },
    // Hårt och industriellt
    { "Stålverk 140", 0.0f, { "1:16:X...x...x...x...", "2:16:x.x.x.x.x.x.x.x.", "3:16:x.xXx.xXx.xXx.xX", "4:16:..x...x...x...x.", "6:16:....X.......X...", "9:16:e:5:16:1", "15:16:x......................x........" }, "Hårt och industriellt" },
    { "Hammarslag", 0.0f, { "1:16:X...x...x...x.xx", "5:16:....x.......x...", "3:16:xxxxxxxxxxxxxxxx", "10:16:x..x..x..x..x..x", "13:16:..x...x...x...x.", "16:16:..............................x.", nullptr }, "Hårt och industriellt" },
    { "Valsverk", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "12:16:xxxxxxxxxxxxxxxx", "7:16:x..x..x...x..x..", "8:16:..x...x.x...x..x", "15:16:x...............................", "6:16:....x.......x...", nullptr }, "Hårt och industriellt" },
    { "Bunkerrave", 0.0f, { "1:16:X...x...x...x...", "6:16:....x.......x..4", "3:16:x.xXx.xXx.xXx.xX", "4:16:..x...x...x...x.", "14:16:x.....x.....x...", "16:16:x...............................", "13:16:..x...x...x...x.", nullptr }, "Hårt och industriellt" },
    { "Rostlåda", 0.0f, { "1:16:X...x...x...x...", "9:16:e:7:16:2", "10:16:e:5:16:0", "7:12:x..x.x..x.x.", "3:16:..x...x...x...x.", "6:16:....x.......x...", nullptr }, "Hårt och industriellt" },
    { "Nitpistol", 0.0f, { "1:16:X...x...x...x...", "9:16:x3x3x3x3x3x3x3x4", "3:16:..x...x...x...x.", "5:16:....x.......x...", "13:16:..x...x...x...x.", nullptr }, "Hårt och industriellt" },
    // Dub och djup
    { "Dimhamn", 0.05f, { "1:16:x...x...x...x...", "14:16:..x.......x.....", "3:16:..x...x...x...x.", "12:16:oxoxoxoxoxoxoxox", "13:16:x.......x.......", "11:16T:..x..x..x..x..x.", nullptr }, "Dub och djup" },
    { "Kanal Två", 0.0f, { "1:16:X...x...x...x...", "14:16:...x..x....x....", "11:16T:x..x..x..x..x..x", "3:16:..x...x...x...x.", "15:16:x...............................", "13:16:..x...x...x...xx", nullptr }, "Dub och djup" },
    { "Djupvatten", 0.0f, { "1:16:x...x...x...x...", "14:16:x...........x...", "4:16:..x...x...x...x.", "13:16:x..x............", "9:16:e:3:16:5", nullptr }, "Dub och djup" },
    { "Ekobassäng", 0.0f, { "1:16:X...x...x...x...", "14:16:..x.....x..x....", "10:16:e:3:16:2", "3:16:..x...x...x...x.", "2:16:x...x...x...x...", "16:16:..........................x.....", nullptr }, "Dub och djup" },
    { "Kvällsdimma", 0.1f, { "1:16:x...x...x...x...", "14:16:x.....x.....x...", "12:16:.x.x.x.x.x.x.x.x", "11:7:x..x...", "15:16:x...............................", nullptr }, "Dub och djup" },
    { "Varvet 1994", 0.05f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "3:16:xxXxxxXxxxXxxxXx", "4:16:..x...x...x...x.", "13:16:.xxx.xX..xxx.x2.", "14:16:..x.....x.......", "9:16:e:5:16:3", "16:16:..............................x." }, "Dub och djup" },
    // Rå och hård: ångvälts-techno
    { "Ångvält 138", 0.06f, { "1:16:X...x...x...x...", "2:16:.x...x...x...x..", "3:16:xxXxxxXxxxXxxxXx", "4:16:..x...x...x...x.", "5:16:....x.......x...", "7:16:...x..x...x....x", "8:16:.......x......x.", "11:16:......x.......x." }, "Rå och hård: ångvälts-techno" },
    { "Hardgroove Rull", 0.12f, { "1:16:X...x...x...x...", "2:16:.x...x...x...x..", "12:16:xoxxxoxxxoxxxoxx", "4:16:..x...x...x...x.", "5:16:....x.......x...", "7:16:x..x..x.x..x..x.", "8:16:..x...x...x..xx.", "15:16:x..............................." }, "Rå och hård: ångvälts-techno" },
    { "Rostfabriken", 0.0f, { "1:16:X...x...x...x...", "2:16:.x...x...x...x..", "3:16:x.xxx.xxx.xxx.xx", "6:16:....x.......x..x", "9:16:e:5:16:2", "10:16:e:3:16:7", "13:16:..x...x...x..xx.", "16:16:...............................x" }, "Rå och hård: ångvälts-techno" },
    { "Stålpressen", 0.0f, { "1:16:X...x...x...x...", "2:16:x...x...x...x...", "3:16:..x...x...x...x.", "5:16:....X.......X...", "7:16:...x..x....x..x.", "9:16:.x...x...x...x2.", "13:16:x.x..x.x..x..x.x", "14:16:......x.......x." }, "Rå och hård: ångvälts-techno" },
    { "Valsverk 140", 0.04f, { "1:16:X...x...x...x...", "2:16:.x...x...x...x..", "3:16:xxXxxxXxxxXxxxXx", "4:16:..x...x...x...x.", "10:16:e:7:16:1", "11:16:...x.......x....", "13:16:.xx..xx..xx..xx.", "15:16:x..............................." }, "Rå och hård: ångvälts-techno" },
    { "Lagerlokal", 0.18f, { "1:16:X...x...x...x...", "2:16:.x...x...x...x..", "4:8:.x.x.x.x", "5:16:....x.......x...", "7:12:x..x.x..x.x.", "9:16:..x.....x..x....", "12:16:xxxxxxxxxxxxxxxx", "14:16:...x.......x...." }, "Rå och hård: ångvälts-techno" },
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
    const char* group = nullptr;
};

constexpr FxPreset kFx[] = {
    // Rumble
    { "Betonggolv Rumble", 2, 0.45f, 0.4f, 0.1f, 0.25f, 0.85f, 0.5f, 100.0f, 0.9f, 0.85f, 0.35f, 0, "00000000", "90000000", "00000000", "30000000", "00000000", "Rumble" },
    { "Källar-Rumble", 2, 0.4f, 0.3f, 0.0f, 0.2f, 0.6f, 0.7f, 80.0f, 0.95f, 0.8f, 0.5f, 0, "00000000", "90000000", "00000000", "20000000", "00000000", "Rumble" },
    { "Rumble Pump Hård", 1, 0.4f, 0.5f, 0.0f, 0.2f, 0.75f, 0.9f, 120.0f, 1.0f, 0.9f, 0.55f, 0, "00000000", "99000000", "00000000", "50000000", "00000000", "Rumble" },
    { "Rumble Mjuk Sub", 2, 0.35f, 0.3f, 0.0f, 0.15f, 0.5f, 0.2f, 60.0f, 0.7f, 0.7f, 0.2f, 0, "00000000", "70000000", "00000000", "00000000", "00000000", "Rumble" },
    { "Tunnelbana", 4, 0.55f, 0.25f, 0.3f, 0.3f, 1.0f, 0.6f, 110.0f, 0.85f, 0.85f, 0.4f, 0.3f, "00050000", "90000000", "00300000", "20000000", "00000000", "Rumble" },
    { "Rumble Rattle", 2, 0.4f, 0.5f, 0.0f, 0.2f, 0.7f, 0.85f, 150.0f, 0.9f, 0.75f, 0.45f, 0, "00000000", "90000000", "00000000", "40000000", "00000000", "Rumble" },
    { "Lagerlokal", 5, 0.5f, 0.35f, 0.1f, 0.25f, 1.0f, 0.3f, 130.0f, 0.6f, 0.7f, 0.3f, 0, "00300300", "95005000", "00000000", "10000000", "00000000", "Rumble" },
    // Dub
    { "Kanalbas-eko", 2, 0.68f, 0.45f, 0.25f, 0.6f, 0.6f, 0.3f, 90.0f, 0.8f, 0.4f, 0.25f, 0, "00000900", "60000000", "00000000", "00000000", "00000000", "Dub" },
    { "Punkterad Dimma", 1, 0.62f, 0.3f, 0.2f, 0.55f, 0.6f, 0.3f, 90.0f, 0.8f, 0.35f, 0.2f, 0, "00300600", "50000000", "00000000", "00000000", "00000000", "Dub" },
    { "Fem Sextondelar", 4, 0.6f, 0.5f, 0.15f, 0.55f, 0.5f, 0.3f, 90.0f, 0.8f, 0.3f, 0.2f, 0, "00009900", "40000000", "00000000", "00000000", "00000000", "Dub" },
    { "Triol-eko", 3, 0.55f, 0.55f, 0.1f, 0.5f, 0.5f, 0.3f, 90.0f, 0.8f, 0.3f, 0.25f, 0, "00099000", "40000000", "00000000", "00000000", "00000000", "Dub" },
    { "Tape-Dub", 2, 0.65f, 0.35f, 0.8f, 0.6f, 0.6f, 0.4f, 90.0f, 0.8f, 0.35f, 0.3f, 0.1f, "00000990", "50000000", "00000000", "00000000", "00000000", "Dub" },
    { "Freeze Moln", 5, 0.85f, 0.3f, 0.5f, 0.65f, 0.9f, 0.3f, 100.0f, 0.5f, 0.4f, 0.2f, 0, "00009990", "60000000", "00000000", "00000000", "00000000", "Dub" },
    { "Dubbelt Eko", 0, 0.6f, 0.6f, 0.1f, 0.55f, 0.5f, 0.3f, 90.0f, 0.8f, 0.3f, 0.25f, 0, "00900000", "40000000", "00000000", "00000000", "00000000", "Dub" },
    { "Ekokammare", 1, 0.5f, 0.85f, 0.2f, 0.45f, 0.5f, 0.3f, 90.0f, 0.8f, 0.3f, 0.2f, 0, "09000090", "40000000", "00000000", "00000000", "00000000", "Dub" },
    // Distorsion
    { "Mixern i Rött", 2, 0.35f, 0.4f, 0.0f, 0.15f, 0.6f, 0.5f, 100.0f, 0.8f, 0.5f, 0.6f, 0, "00000000", "70000000", "00000000", "70000000", "00000000", "Distorsion" },
    { "Fabrikshall", 2, 0.4f, 0.4f, 0.0f, 0.2f, 0.7f, 0.6f, 100.0f, 0.85f, 0.6f, 0.85f, 0, "00000000", "80000000", "00000000", "53333333", "00000000", "Distorsion" },
    { "Krossad Bunker", 1, 0.45f, 0.5f, 0.0f, 0.2f, 0.7f, 0.8f, 110.0f, 0.9f, 0.7f, 0.9f, 0.15f, "00000000", "90000000", "02222222", "66666666", "00000000", "Distorsion" },
    { "Acid Skrik", 2, 0.55f, 0.6f, 0.1f, 0.35f, 0.6f, 0.5f, 100.0f, 0.8f, 0.5f, 0.5f, 0, "00009000", "70000000", "00000000", "20009000", "00009000", "Distorsion" },
    { "Rostig Tom", 2, 0.45f, 0.4f, 0.0f, 0.25f, 0.6f, 0.5f, 100.0f, 0.8f, 0.5f, 0.45f, 0, "00900000", "70000000", "00700000", "00700000", "00000000", "Distorsion" },
    { "Smutsig Parallell", 2, 0.4f, 0.4f, 0.0f, 0.2f, 0.6f, 0.6f, 100.0f, 0.8f, 0.6f, 0.75f, 0, "00000000", "80000000", "00000000", "44444444", "00000000", "Distorsion" },
    { "Varm Tejp", 2, 0.5f, 0.3f, 0.6f, 0.3f, 0.6f, 0.3f, 90.0f, 0.8f, 0.5f, 0.35f, 0.05f, "00300300", "70000000", "00000000", "22222222", "00000000", "Distorsion" },
    { "Överstyrd 909", 2, 0.4f, 0.4f, 0.0f, 0.2f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.6f, 0, "00000000", "70000000", "00000000", "90000000", "00000000", "Distorsion" },
    // Lo-fi
    { "SP-12 Grus", 2, 0.4f, 0.4f, 0.1f, 0.25f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0.25f, "00000000", "70000000", "05555555", "00000000", "00000000", "Lo-fi" },
    { "Sexbitars Trä", 2, 0.45f, 0.4f, 0.1f, 0.3f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0, "00000000", "70000000", "00009900", "00000000", "00000000", "Lo-fi" },
    { "MPC Varm", 2, 0.4f, 0.3f, 0.2f, 0.25f, 0.6f, 0.3f, 90.0f, 0.8f, 0.5f, 0.35f, 0.15f, "00000000", "70000000", "03333333", "11111111", "00000000", "Lo-fi" },
    { "Kassett", 2, 0.5f, 0.25f, 0.9f, 0.35f, 0.6f, 0.3f, 90.0f, 0.7f, 0.5f, 0.3f, 0.2f, "00300330", "60000000", "02222222", "11111111", "00000000", "Lo-fi" },
    { "Vinylgolv", 1, 0.5f, 0.3f, 0.4f, 0.35f, 0.6f, 0.3f, 90.0f, 0.7f, 0.5f, 0.3f, 0.1f, "00000660", "60000000", "00000000", "00000000", "00000000", "Lo-fi" },
    { "Trasig Sampler", 3, 0.5f, 0.5f, 0.3f, 0.4f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.45f, 0.5f, "00990000", "70000000", "09999999", "00000000", "00000000", "Lo-fi" },
    // Filter
    { "Acid Svep", 2, 0.55f, 0.6f, 0.1f, 0.35f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.45f, 0, "00009000", "70000000", "00000000", "00006000", "00008000", "Filter" },
    { "Självsvängning", 2, 0.5f, 0.5f, 0.1f, 0.3f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0, "00990000", "70000000", "00000000", "00000000", "00999999", "Filter" },
    { "Bandpass Dimma", 1, 0.6f, 0.4f, 0.2f, 0.45f, 0.6f, 0.3f, 90.0f, 0.8f, 0.4f, 0.3f, 0, "00000990", "60000000", "00000000", "00000000", "00000660", "Filter" },
    { "Pump-filter", 2, 0.45f, 0.5f, 0.0f, 0.25f, 0.7f, 0.5f, 100.0f, 1.0f, 0.6f, 0.45f, 0, "00000000", "90000000", "00000000", "00000000", "05555555", "Filter" },
    { "Triol-svep", 3, 0.55f, 0.6f, 0.1f, 0.4f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0, "00900000", "70000000", "00000000", "00000000", "00700000", "Filter" },
    { "Notch-fas", 2, 0.45f, 0.5f, 0.3f, 0.3f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "00000000", "70000000", "00000000", "00000000", "05555555", "Filter" },
    { "Comb-ringning", 4, 0.75f, 0.8f, 0.0f, 0.45f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0, "00099000", "70000000", "00000000", "00000000", "00990000", "Filter" },
    { "Master-uppbyggnad", 2, 0.5f, 0.6f, 0.1f, 0.35f, 0.7f, 0.5f, 110.0f, 0.8f, 0.6f, 0.5f, 0, "00300300", "80000000", "00000000", "20000000", "00000000", "Filter" },
    // Rymd
    { "Gated Clap", 0, 0.35f, 0.6f, 0.0f, 0.3f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "00009000", "70000000", "00000000", "00000000", "00000000", "Rymd" },
    { "Plåtregn", 0, 0.4f, 0.9f, 0.0f, 0.35f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "09900009", "70000000", "00000000", "00000000", "00000000", "Rymd" },
    { "Mörk Katedral", 5, 0.75f, 0.2f, 0.3f, 0.55f, 1.0f, 0.3f, 90.0f, 0.6f, 0.6f, 0.3f, 0, "00000999", "80000000", "00000000", "00000000", "00000000", "Rymd" },
    { "Betongrum", 0, 0.3f, 0.5f, 0.0f, 0.3f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "00000000", "70000000", "00000000", "00000000", "00000000", "Rymd" },
    { "Flanger-Jet", 3, 0.7f, 0.8f, 1.0f, 0.45f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "09900000", "70000000", "00000000", "00000000", "00000000", "Rymd" },
    { "Stereospridning", 0, 0.45f, 0.6f, 0.2f, 0.4f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.35f, 0, "00999990", "70000000", "00000000", "00000000", "00000000", "Rymd" },
    { "Ride i Rök", 5, 0.6f, 0.7f, 0.2f, 0.45f, 0.8f, 0.3f, 100.0f, 0.8f, 0.5f, 0.3f, 0, "09000000", "70000000", "00000000", "00000000", "00000000", "Rymd" },
    // Performance
    { "Stutter-redo", 2, 0.4f, 0.5f, 0.0f, 0.25f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.45f, 0, "00000000", "70000000", "00000000", "20000000", "00000000", "Performance" },
    { "Tejpstopp-känsla", 4, 0.6f, 0.3f, 1.0f, 0.4f, 0.6f, 0.4f, 100.0f, 0.8f, 0.5f, 0.4f, 0.1f, "00300900", "70000000", "00000000", "10000000", "00000000", "Performance" },
    { "Mute-cykel Dub", 2, 0.65f, 0.4f, 0.3f, 0.6f, 0.6f, 0.3f, 90.0f, 0.8f, 0.4f, 0.3f, 0, "00999999", "60000000", "00000000", "00000000", "00000000", "Performance" },
    { "Ren Klubb", 2, 0.4f, 0.5f, 0.0f, 0.2f, 0.6f, 0.3f, 100.0f, 0.8f, 0.45f, 0.25f, 0, "00000000", "60000000", "00000000", "00000000", "00000000", "Performance" },
    { "Total Kollaps", 4, 0.9f, 0.4f, 0.7f, 0.6f, 1.0f, 1.0f, 140.0f, 1.0f, 0.8f, 0.95f, 0.45f, "09999999", "99000000", "05555555", "77777777", "03333333", "Performance" },
    { "Fuktigt Betonggolv", 2, 0.62f, 0.35f, 0.35f, 0.45f, 0.9f, 0.6f, 105.0f, 0.9f, 0.8f, 0.25f, 0.08f, "00300960", "95000000", "00200000", "40000000", "00000000", "Performance" },
    // Techno
    { "Tunnelpump", 2, 0.55f, 0.35f, 0.15f, 0.3f, 0.9f, 0.6f, 100.0f, 1.0f, 0.85f, 0.35f, 0, "00000000", "95000000", "00000000", "30000000", "00000000", "Techno" },
    { "Stålverk", 1, 0.45f, 0.6f, 0.0f, 0.25f, 0.7f, 0.85f, 120.0f, 0.95f, 0.7f, 0.75f, 0.1f, "00090000", "90000000", "00330000", "55555555", "00000000", "Techno" },
    { "Dimhamn", 2, 0.75f, 0.3f, 0.45f, 0.65f, 0.8f, 0.3f, 90.0f, 0.8f, 0.6f, 0.2f, 0, "00009900", "80000000", "00000000", "00000000", "00000000", "Techno" },
    { "Djuphavet", 5, 0.8f, 0.25f, 0.35f, 0.6f, 1.0f, 0.35f, 80.0f, 0.7f, 0.7f, 0.2f, 0, "00009990", "90000000", "00000000", "00000000", "00000000", "Techno" },
};
constexpr int kNumFx = (int) (sizeof (kFx) / sizeof (kFx[0]));
} // namespace betong
