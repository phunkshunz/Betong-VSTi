# BETONG

Techno-sampler som VST3 och AU för macOS. Den hackar upp ljud och gör skeva, polyrytmiska beats. Allt ska låta som ett fuktigt betonggolv.

Zippen innehåller två maskiner som bygger på samma motor:

- **BETONG**, techno-samplern. Öppnar med det rena grundkitet och mönstret Grund 130.
- **SMIDET**, den euklidiska smedjan. En helt egen maskin utan pads och vågform: ringarna är huvudfönstret, ljuden är städ, hammare, kedjor och klockor, och två makron (HAMMARE och GLÖD) styr allt. Röd i stället för orange.

Båda har egna inställningar och ligger under tillverkaren Phunkshunz i Bitwig. Installationsskriptet installerar båda.

Ett plugin från **MASKINHALLEN**. Klicka på loggan uppe till vänster i pluginet för att läsa om båtvarvet Docklands, där allt började.

## SMIDET, den euklidiska smedjan

- **Ringarna** fyller vänstra halvan. Varje spår är en ring. Dra runt för att rotera, dra upp/ner eller rulla för antal slag, dubbelklicka för euklid på/av. Klicka på en ring för att välja spår.
- **M, S och +** bredvid varje spår i ringlistan: mute, solo och ladda egen sampling. Mutade spår blir mörka i ringen.
- **Egna samplingar:** dra in ljudfiler på en ring eller ett spårnamn. Flera filer på en gång hamnar på spåret och spåren under. Knappen **Ladda egen sampling...** i smedjan gör samma sak för valt spår.
- **Högerklick** på en ring eller ett spårnamn: mute, solo, lås, euklid, ljud ur biblioteket, egen sampling, provlyssna, avmuta alla och stäng av all solo.
- **Smedjan** (panelen till höger) visar valt spår: pad, takt, längd, euklid, mute, solo och lås, plus knappen **Ljud** som väljer ljud direkt ur biblioteket (Smedja först).
- **HAMMARE:** tätare, hårdare och fler skurar. Vrider upp densitet, glitch, drive och clipper på en gång.
- **GLÖD:** resonans, drive, mer filter-envelope, mer delay och mer rumble. Smedjan börjar glöda.
- **Ljuden:** 16 smedjeljud i kategorin Smedja (städ stämda i F-moll, stålhammare, slagg, plåt, kedja, fjäder, härdning, blåsbälg, gjutklocka, rör, skrot och gnistor) plus städkick, smideshats och hammarknack.
- **Kiten:** Smedjan, Järnverket, Klockgjuteriet, Valsverk Euklid, Rost och Gnistor och Kall Härdning.
- **Mönstren:** mappen **Smide** har 14 helt euklidiska, polymetriska mönster. Öppnar med Smedjans Puls och effekten Smedjeglöd.
- Allt annat (scener, fill, svaj, glitch, euklid-fliken, filter, effekter) delar SMIDET med BETONG.

## Ladda ner

Ladda ner den senaste versionen här:
https://github.com/phunkshunz/Betong-VSTi/releases/latest/download/BETONG-macOS.zip

Dubbelklicka på zip-filen i Hämtade filer. Då får du en mapp som heter **BETONG**.

Ladda inte ner via den gröna knappen **Code**. Den ger bara källkoden, inte pluginet.

## Installera på Mac

Öppna Terminal och kör:

```
bash ~/Downloads/BETONG/installera.command
```

Hamnade mappen någon annanstans? Skriv `bash ` och dra in filen `installera.command` i Terminal-fönstret, och tryck sedan Enter.

Starta om Bitwig, eller tryck **Rescan** under Settings › Locations › Plug-ins. Sök på BETONG.

## Windows

Ladda ner Windows-versionen här:
https://github.com/phunkshunz/Betong-VSTi/releases/latest/download/BETONG-Windows.zip

1. Högerklicka på zip-filen och välj **Extrahera alla**. Du får en mapp som heter **BETONG**.
2. Dubbelklicka på **installera.bat** i mappen. Svara **Ja** när Windows frågar om administratör.
3. Om Windows säger "Windows skyddade datorn": klicka **Mer info** och sedan **Kör ändå**. Det beror på att pluginet inte är signerat än.
4. Starta om din DAW, eller kör en ny plugin-sökning. BETONG ligger i `C:\Program Files\Common Files\VST3`.

I mappen finns också **BETONG.exe**, som är den fristående appen. Den kan du starta direkt utan DAW.

Kräver 64-bitars Windows 10 eller 11.

## Manual

Den engelska manualen ligger i zippen som **BETONG-Manual.pdf**. Källan finns i `manual/manual.html`. GitHub gör PDF:en automatiskt vid varje bygge.

## Så funkar den

**Starta:** tryck Play i Bitwig. Sequencern följer DAW:ens tempo. Utan DAW, eller i den fristående appen, trycker du **SPELA** nere till höger.

**Uppe i fönstret**
- **Kit, Mönster och Effekter:** 44 kit, 183 mönster och 55 effekt-presets, sorterade i genremappar (Techno, Peak time, Hardgroove, Tech house, Tribal, Minimal, Dub, Polyrytm, Euklid-magi, Smide med flera). Börja med kiten under **Rena grundkit**.
- **Tärningarna:** slumpar kit, effekter, mönster eller allt. **Galenskap** styr hur vilt det blir.
- **Ångra:** backar de senaste 20 slumpningarna.
- **Lås** på ett spår gör att slumpen låter det spåret vara.

**Sampling och pads**
- **Klick på en pad:** spelar den och väljer den.
- **Högerklick på en pad:** välj ett av de 235 inbyggda ljuden eller en egen fil.
- **Dra in filer** på en pad, eller dra flera filer på en gång.
- **Hacka till pads:** delar ljudet på vald pad i bitar och lägger dem på pad 1–16. Läget **Anslag** hittar slagen automatiskt.
- **Dra i de orange kanterna** i vågformen för att flytta start och slut.
- **Bibliotek:** alla inbyggda ljud plus **+ Lägg till mapp** för dina egna sample-mappar. Mapparna sparas till alla projekt.

**Sequencern (8 spår)**
- Varje spår har egen **pad**, **längd** (1–32 steg) och **takt** (1/16, 1/8, trioler, 1/32, 5-del och 7-del). Olika längder och takter glider mot varandra och blir polyrytmer.
- **E** = euklidiskt läge. Du väljer antal slag och rotation.
- **Klick** slår på och av ett steg, och du kan dra för att måla.
- **Shift-klick** = stark, svag (ghost) eller normal.
- **Rulla** över ett steg = ton i halvtoner (för basgångar). Högerklick har också en Ton-meny.
- **Alt-klick** = sannolikhet.
- **Cmd-klick** (Ctrl-klick på Windows) = ratchet.
- **Högerklick** = meny med allt.
- **M / S / Lås** = mute, solo och lås mot slump.
- **Scener A–D:** fyra mönster att växla mellan live. Bytet sker på nästa takt. **Kopiera till...** kopierar scenen.
- **FILL:** håll inne för en virvel på clap, snare och toms.
- **SVAJ:** slagen släpar lite, styrkan varierar och tonen glider, som en gammal maskin som driver.
- **GLITCH:** slumpvisa skurar, baklängesslag, tonhopp och tappade slag. Varken svaj eller glitch ändrar mönstret.
- **Swing per spår** i panelen till höger, till exempel shuffle på shakern medan kicken går rak.
- **DENSITET:** vrid upp för fler slag, ner för glesare. Euklidiska spår fyller på eller glesar ut jämnt fördelat.
- **RING:** visar spåren som euklidiska ringar. Dra runt en ring för att rotera, dra upp/ner eller rulla för antal slag, dubbelklicka för euklid på/av.

**Euklid-magi (fliken EUKLID för valt spår)**
- **Rotera själv:** mönstret flyttar sig ett steg var 1:a–16:e takt och glider hypnotiskt runt.
- **Andas:** antalet slag sveper upp och ner över 4–32 takter, så uppbyggnader och breaks kommer av sig själva.
- **Accent-, ratchet- och tonlager:** egna euklidiska mönster med egen längd ovanpå slagen. De landar olika varje varv, så basgångar trasslar ihop sig och reder ut sig igen.
- **Fyll luckorna i:** spåret spelar bara där ett annat spår är tyst, för låsta afrikanska mönster.
- Mönstermappen **Euklid-magi** har 14 färdiga exempel. Inget av detta ändrar själva mönstret.

**MOTOR (beatet lever av sig självt)**
- Tryck **MOTOR** i sequencern eller öppna fliken **MOTOR**. Ditt mönster ändras aldrig. Motorn räknar fram en levande kopia medan det spelar.
- **Formen** följer låtens position i DAW:en: intro, bygg, topp, break, lyft, topp 2 och utrull över 64, 128 eller 256 takter. **Fri** ger ändlösa vågor med break ibland. **Bara liv** låter allt spela och gör bara små förändringar.
- **Energikurvan** visar var i låten du är. Spår kommer in och går ut på frasgränser när energin stiger och sjunker. Kicken går ut i breaken och kommer tillbaka på droppen.
- **RÖRELSE:** hur ofta saker ändras. **DJUP:** hur stora ändringarna blir. Ett slag läggs till, tas bort, flyttas eller byter styrka per fras. Efter fyra steg kommer grundmönstret tillbaka, och ibland kommer ett gammalt motiv tillbaka lite förändrat.
- **KAOS:** drar i allt på en gång.
- **FILTER:** filtren andas, stängs inför skiften och öppnar när det nya kommer in. Resonanstoppar när det är hett, mörkare master i breaken och ett HP-svep i lyftet.
- **DUB:** ett slag kastas in i delayen på slutet av frasen. **PUMP:** sidechain från kicken och drive som växer med energin. **SVÄNG:** velocity-vågor, mikrotiming och swing som driver långsamt.
- **Seed:** samma seed låter alltid likadant. **NY SEED** ger en ny utveckling av samma mönster.
- I sequencern syns motorns ändringar live: kall ram = tillagt slag, överstruket = borttaget, prick = ändrad styrka eller ton. Spår som vilar blir mörka.
- **Lås** på ett spår gör att motorn låter det vara.

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

235 egna ljud i 24-bit, syntade från grunden i `tools/make_library.py`. Grunden är en kärna av rena, feta trummaskinstrummor (kickar, hats, clap, snare, rim, toms och ride) som alla ligger på jämna nivåer. De är fria att använda i egna låtar.

- Kickar, hats, clap, perc
- Tech house och tribal: bongos, congas, djembe, timbales, tamburin, log drums, agogo, orgelbas och marimba
- Rå och hård ångvälts-techno: distade kickar, rumble, smutsiga hats och smällande clap. Kiten Ångvälten, Hardgroove 97, Rostfabriken, Valsverk 140, Lagerlokal och Stålpressen.
- Bas, stabs, leads
- Texturer och drönare
- Röster och FX
- Skeva loopar i 5, 7 och 11 steg
- Smedjan: städ, hammare, kedjor, klockor, rör och gnistor

Allt tonalt ligger i F-moll.

## Bygga själv

Pluginet byggs automatiskt av GitHub Actions och läggs upp som release. Det blir en universell binär för Intel och Apple Silicon och kräver macOS 11 Big Sur eller nyare. Windows-versionen byggs bara när man startar bygget för hand under Actions.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
