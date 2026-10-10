# BETONG

Techno-sampler som VST3 och AU för macOS. Den hackar upp ljud och gör skeva, polyrytmiska beats. Allt ska låta som ett fuktigt betonggolv.

Ett plugin från **MASKINHALLEN**. Klicka på loggan uppe till vänster i pluginet för att läsa om båtvarvet Docklands, där allt började.

## Ladda ner

Ladda ner den senaste versionen här:
https://github.com/phunkshunz/Betong-VSTi/releases/latest/download/BETONG-macOS.zip

Dubbelklicka på zip-filen i Hämtade filer. Då får du en mapp som heter **BETONG**.

Ladda inte ner via den gröna knappen **Code**. Den ger bara källkoden, inte pluginet.

## Installera

Öppna Terminal och kör:

```
bash ~/Downloads/BETONG/installera.command
```

Hamnade mappen någon annanstans? Skriv `bash ` och dra in filen `installera.command` i Terminal-fönstret, och tryck sedan Enter.

Starta om Bitwig, eller tryck **Rescan** under Settings › Locations › Plug-ins. Sök på BETONG.

## Så funkar den

**Starta:** tryck Play i Bitwig. Sequencern följer DAW:ens tempo. Utan DAW, eller i den fristående appen, trycker du **SPELA** nere till höger.

**Uppe i fönstret**
- **Kit, Mönster och Effekter:** 12 kit, 50 mönster och 50 effekt-presets.
- **Tärningarna:** slumpar kit, effekter, mönster eller allt. **Galenskap** styr hur vilt det blir.
- **Ångra:** backar de senaste 20 slumpningarna.
- **Lås** på ett spår gör att slumpen låter det spåret vara.

**Sampling och pads**
- **Klick på en pad:** spelar den och väljer den.
- **Högerklick på en pad:** välj ett av de 103 inbyggda ljuden eller en egen fil.
- **Dra in filer** på en pad, eller dra flera filer på en gång.
- **Hacka till pads:** delar ljudet på vald pad i bitar och lägger dem på pad 1–16. Läget **Anslag** hittar slagen automatiskt.
- **Dra i de orange kanterna** i vågformen för att flytta start och slut.
- **Bibliotek:** alla inbyggda ljud plus **+ Lägg till mapp** för dina egna sample-mappar. Mapparna sparas till alla projekt.

**Sequencern (8 spår)**
- Varje spår har egen **pad**, **längd** (1–32 steg) och **takt** (1/16, 1/8, trioler, 1/32, 5-del och 7-del). Olika längder och takter glider mot varandra och blir polyrytmer.
- **E** = euklidiskt läge. Du väljer antal slag och rotation.
- **Klick** slår på och av ett steg, och du kan dra för att måla.
- **Shift-klick** = accent.
- **Alt-klick** = sannolikhet.
- **Cmd-klick** = ratchet.
- **Högerklick** = meny med allt.
- **M / S / Lås** = mute, solo och lås mot slump.

**Techno-filtret (valt spår)**
- LP 24, LP 12, HP, BP och Notch.
- Resonans som får dista, drive, envelope som öppnar filtret vid varje slag, och crush ner till 4 bit.
- Sändningar till delay och rumble.

**Effekter**
- **Dub-delay:** ping-pong med filter i ekot, tejp-wobble och **Freeze**.
- **Rumble:** reverb, distorsion och lowpass, duckad av kicken (spår 1).
- **Master:** DJ-svep (vänster = LP, höger = HP), grus, clipper och bas-mono under 120 Hz.
- **Stutter:** håll inne knappen för att loopa en bit av ljudet.

**MIDI:** C1 och uppåt (not 36–51) spelar pad 1–16.

## Ljuden

103 egna ljud i 24-bit, syntade från grunden i `tools/make_library.py`. De är fria att använda i egna låtar.

- Kickar, hats, clap, perc
- Bas, stabs, leads
- Texturer och drönare
- Röster och FX
- Skeva loopar i 5, 7 och 11 steg

Allt tonalt ligger i F-moll.

## Bygga själv

Pluginet byggs automatiskt av GitHub Actions och läggs upp som release. Det blir en universell binär för Intel och Apple Silicon och kräver macOS 11 Big Sur eller nyare.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
