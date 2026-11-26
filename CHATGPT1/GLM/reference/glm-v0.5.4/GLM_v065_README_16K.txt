v0.6.5 - A /TOPIC VALTAS ES A CHAT-TORLES MUKODIK
  - A /TOPIC VALTAS (pl. /TOPIC 2 = magyar tema): harom retegben
    hibazott a regota. (1) A nevtabla valasztasa egyel csuszott (a
    0. tabla elem a BASE, a temak szama 1-13 az 1-13 elemre mutat).
    (2) A nev-iro a NYITO IDEZOJELT torolte a sorbol - a LOAD mar
    nem latott ervenyes szoveget. (3) A ROM SET-FAST jeben a HALT
    varakozas kifagyott: a motor a chat alatt KI KAPcsolja az NMI
    generatorat, a HALT viszont eppen azt varja. A /topic most:
    atirja a LOAD nevet a sorban, bekapcsolja az NMI-t, a calculator
    vermet a D-file folott allitja - aztan "TOLTOM A TEMAT...", a
    ROM sajat LOAD-ja nevre keresi a blokkot, es az UJ TEMA betolt
    es elindul. Szimulacional vegigbizonyitva: /topic 2 utan a
    "RT-MAGYR" blokk betolt, a magyar modell chat-el, es a /topic
    tovabbra is mukodik.
  - A TORLES (SHIFT+0) A CHAT-BEN: a regi motor a 18-as billentyu-
    kodra varakozott (a valodi rubout a 8). Javitva: a 8-as kod a
    sorveg roviditesere es a kepernyo tiszta torlesere fut (a
    karakter utan feltorolve). Bizonyitva: "L", "P", torles, "O"
    - a beviteli puffer "LO", a kepernyo "YOU> LO", a kurzor a
    szoveg vegen.
  - A BOOT kerdesekben a torles mar a v0.6.4 ota mukodik.
  - HASZNALAT: toltsd be a GLM_v065_RT16_TOPICS.t81 kazettat az
    emulatoredba (tape/kazetta kent!), majd LOAD "" es RUN. A
    kovetkezo blokkok (RTBASE16, a temak) nevre keresese a szalagon
    megy - a .p fajlok kozvetlen gyorsbetoltesre valnak. Chat
    kozben: /TOPIC 1..13 = tema valtas (tudomany, magyar, hobbi,
    egeszseg, munka, muveszet, tanulas, iroda, film, irodalom,
    programozas, csillagaszat, vers), SHIFT+0 = torles.
  - Tudott hatar: a /quit meg nem allitja le a ciklust (a 3. sor
    ujra meghivja a motort - a chat egyszeruen folytatodik), es a
    valodi vaszon (nem emulator) tesztje meg var.
