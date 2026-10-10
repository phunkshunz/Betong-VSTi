# BETONG

Techno-sampler som VST3 och AU för macOS. Den hackar upp ljud och gör skeva, polyrytmiska beats.

## Version 0.1, steg 1

Pluginet laddas i DAW:en och spelar ett ljud på alla MIDI-toner, med C3 som originaltonhöjd.

- En inbyggd 909-kick spelas direkt.
- Dra in en egen WAV, AIFF, MP3, FLAC eller OGG, eller tryck **Ladda sampling**.
- Tryck på pad:en för att testa ljudet utan keyboard.
- **+ Lägg till mapp** lägger till dina egna sample-mappar, inklusive undermappar. Ljuden visas i en lista där du klickar för att ladda. Mapparna sparas, så de finns kvar i alla projekt.

## Ladda ner

1. Gå till fliken **Actions** här på GitHub och öppna senaste gröna bygget.
2. Ladda ner **BETONG-macOS** längst ner på sidan.
3. Dubbelklicka på zip-filerna tills du har en mapp som heter **BETONG** i Hämtade filer.

## Installera

Öppna Terminal och kör:

```
bash ~/Downloads/BETONG/installera.command
```

Hamnade mappen någon annanstans? Skriv `bash ` och dra in filen `installera.command` i Terminal-fönstret, och tryck sedan Enter.

Skriptet gör två saker:

- Kopierar pluginet till `~/Library/Audio/Plug-Ins/VST3` och `Components`.
- Tar bort macOS-spärren, eftersom pluginet är osignerat.

## Bitwig

1. Starta om Bitwig, eller tryck **Rescan** under Settings › Locations › Plug-ins.
2. Sök på BETONG i browsern.

## Bygga själv

Pluginet byggs automatiskt av GitHub Actions på macOS 14. Det blir en universell binär för Intel och Apple Silicon och kräver macOS 11 Big Sur eller nyare.

Lokalt bygge:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
