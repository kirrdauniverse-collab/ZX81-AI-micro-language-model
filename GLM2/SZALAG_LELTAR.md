# SZALAG-LELTÁR — GLM vonal kazettái (méret + ujjlenyomat + lelőhely)

Minden kazetta byte-hű azonosítása. A lelőhelyek:
- "repo": a repóban (a régi `GLM/reference/tapes/` és annak archív másolata
  `CHATGPT1/GLM/reference/tapes/` — a régi GLM törlése után is megmarad az
  archív másolat, amíg a CHATGPT1 könyvtár él)
- "ZIP": a GLM_retro_tinyllm_v071_atadas.zip csomagban (tape/ mappa)

## A kulcskazetták (a vizsgálat tárgyai)

| Fájl (a név minden szállításban ugyanaz!) | Verzió | MD5 | SHA256 (első 16) | Lelőhely |
|---|---|---|---|---|
| GLM_v070_RT16_TOPICS.t81 | v0.6.5/66 korszak | 1162414c907f7a0b13c5cc2b4999c0de | b053c7ddc2a43193 | repo (GLM/reference/tapes + CHATGPT1/GLM archív) |
| GLM_v070_RT16_TOPICS.t81 | v0.7.1 (motor 13242 B) | 740959e696fddc87f7c262c46022c421 | 6bf888dd34d57691 | v0.7.1 ZIP |
| GLM_v070_RT16_TOPICS.t81 | v0.7.2 (motor 13384 B) | cfe9b28143af344ef271d36db3a50561 | b781ca459bf4a785 | v0.7.2 munkakönyvtár letöltés |
| **GLM_v070_RT16_TOPICS.t81** | **v0.7.3 — AKTUÁLIS** | **48077afe25c2cd7ab2056fbe2cbb1f60** | **12fa403b54c2de51** | v0.7.3 munkakönyvtár letöltés |
| GLM_v070_RT16_TOPICS.tzx | v0.7.1 | edb8742d71784d1f8740b4f73cca772f | b338f6e787dd3a75 | v0.7.1 ZIP (TZX 1.20, 0x19 GENERAL blokkok) |

**FONTOS (a v0.7.3b lecke): a fájlnév minden szállításban UGYANAZ volt — a
verziót csak a banner („retro-tinyllm 16k v0.7.3" a betöltés után) vagy az
MD5 árulja el!** A v0.7.3-as kazetta mind a 15 blokkjában a „v0.7.3"
banner van (ellenőrizve); a v0.7.3 előtti kazetták V0.6.2 banner-t
írnak — ha a TV-n „V0.6.2" áll, RÉGI szalag van betöltve.

## A v0.7.1 csomag többi kazettája (mind a ZIP tape/ mappájában)

| Fájl | Méret (B) | MD5 |
|---|---:|---|
| GLM_v070_rt16_base.t81 / .p / .p81 | 15945 / 15889 / 15897 | 811e1d2c546e3fa40d28e21f12feefb4 / ff786905a1a78d106a38b410f2e88849 / 2506d61afa8a7a3e77c4119fa9cd2b9b |
| GLM_v070_rt16_science.t81 / .p / .p81 | 15945 / 15889 / 15897 | e1df4e32d6e7a157e99944536c690feb / 1c921db8e862d8887f3f1cc1e3d90654 / 461b69e0d62aac9fe57b6758393669a8 |
| GLM_v070_rt16_magyar.t81 / .p / .p81 | 15945 / 15889 / 15897 | e10823f10cccf34394537ef048c76618 / 4432a06eec371e5cf9253dc74bf0ba80 / 227cde0a3a62d891f4883a195069d584 |
| GLM_v070_rt16_hobby.t81 / .p / .p81 | 15945 / 15889 / 15897 | 679d2e1dc03731e50247f7282464772f / 33b945fafb8077744ffe4216a3d4c547 / 729685ec6c0088ea68736525cebd9627 |
| GLM_v070_rt16_health.t81 / .p / .p81 | 15945 / 15889 / 15897 | bed92d2c46d7d2de55dae1c15d16e1e0 / 956ef4db953277cdda223640085db57f / 5ead3b8e6927826263b84e6199497739 |
| GLM_v070_rt16_work.t81 / .p / .p81 | 15945 / 15889 / 15897 | 2101a953ca344e16afbec8551856c3c2 / a017a68e70bb020b9c33dfe702e1b4c1 / 3f23403daf7f13c87106b15ce0a0ba6e |
| GLM_v070_rt16_art.t81 / .p / .p81 | 15945 / 15889 / 15897 | e271d0c97af15fd76ce8738906aea897 / 5a0ab32388a569fd3a27bd939bd501bf / 003e191f49ea03a7dfa285f1ef2bbf9a |
| GLM_v070_rt16_learning.t81 / .p / .p81 | 15945 / 15889 / 15897 | 4ac9616a567ce0217fca4663db45a2b6 / d4208dbe93dc212d2e171a15a9ce179b / 8cf797cf3c729a1b434a150644d25743 |
| GLM_v070_rt16_literature.t81 / .p / .p81 | 15945 / 15889 / 15897 | 59886e6842977b2cae6777e67a16361b / cbccd38cdedce421c649e42f8a3e31a0 / 557334b3c2a59320b9a85ee5553e8679 |
| GLM_v070_rt16_movies.t81 / .p / .p81 | 15945 / 15889 / 15897 | 551a3910ec699b74bf827bb42ab92cdb / 84c4f70103ef0a6b9119ab9bd0ea4893 / b71cd3e0f463d022e79e1c3070a7761f |
| GLM_v070_rt16_programming.t81 / .p / .p81 | 15945 / 15889 / 15897 | d27192db63daea3adc237e5ce40313e6 / 70abaa73d27eaf9065c608ba2363f00e / 1e994b25a7511edfa9ac53cd9b0fa6d5 |
| GLM_v070_rt16_astronomy.t81 / .p / .p81 | 15945 / 15889 / 15897 | af50e2216c277ef41b82e8d2e2940792 / 5d937491537728edbd91dd117b9912e4 / 232001d40fe8823085c27ef2c76feefa |
| GLM_v070_rt16_asciiart.t81 / .p / .p81 | 15945 / 15889 / 15897 | 2ccba110cd7d324e4c8fe32c68c55027 / ce16940be61b59491444e1f21b940aba / b52a619ac23923c2e03e8b76e576c68a |
| GLM_v070_rt16_verse.t81 / .p / .p81 | 15945 / 15889 / 15897 | fab16777ee7e75339782a793724252f9 / 792fdeafdf23652e661b35ec6b560cb1 / 47b5dbe0c905ff4dae3a0e2f9706aca8 |

Témakódok: base, art, asciiart, astronomy, health, hobby, learning, literature,
magyar, movies, programming, science, verse, work (RTBASE16 + 13 téma).

## Referencia-kazetták (más modellek korábbi kísérletei — nem a GLM vonal)

| Fájl | Méret (B) | MD5 | Lelőhely | Mi |
|---|---:|---|---|---|
| Claude_ZXAI.t81 | 105919 | 1c04622e7b3596d1c723e0a601d17001 | repo (GLM/reference/tapes + CHATGPT1 archív) | Claude korábbi kísérlete |
| Mistral_zx81ai16.t81 | 15437 | 324361dc0b83f2418afd47ae161ef270 | repo (ugyanott) + CHATGPT1/RETRO_TINYLLM/ports/zx81/ | Mistral AI16 — a ChatGPT vonal ismert-jó alapja |
| RETRO-TINYAI_BOOT_UI_v2_stripes_bug.t81 | 15943 | ca174cb5b28cbb6484eacae71c79a16a | repo (ugyanott) | másik kísérlet "csíkos hibás" boot UI — a névben hordozott tünet |

A v0.7.2 és v0.7.3 munkakönyvtárak teljes szalagkészletei (TOPICS + 14×t81/.p/.p81)
a letöltésekben élnek (GLM_v072_workdir / GLM_v073_workdir — a user gépén);
byte-hű másolatuk e munkamenetben is elérhető. A v0.7.3 szalag MD5-jét l. fent.

## Egyéb byte-hű archívum a GLM2-ben

- tools/altint_v066.bin (181 B, MD5 2d346cd1eb0b0a1ab30899ff7a90ef48,
  SHA256 91418781d219ab248eb501789653be676ac06bf9c2f4e0e110eaa6bf2db43588)
  → GLM2/tools/altint_v066.bin.b64 formában.

## Visszaállítás / ellenőrzés

A v065 és a v070/v071 kulcskazetta byte-hű másolata e munkamenet letölthető
fájljaiként is átadásra került (MD5-ük fent). A szalagok éles forrása a
v0.7.1-es ZIP csomag; a repóban a v065 él a CHATGPT1/GLM archívumban.
Az altint-v066 blob visszaállítása:

```sh
base64 -d GLM2/tools/altint_v066.bin.b64 > altint_v066.bin
md5sum altint_v066.bin   # elvárt: 2d346cd1eb0b0a1ab30899ff7a90ef48
```
