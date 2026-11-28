#!/usr/bin/env node
/* sim_user2_v066.js - the USER's exact flow, instant-load edition.
 * LOAD "" -> boot (n, ev, 11, 11, 11, 11, SAFI, F) -> gate ENTER ->
 * RTBASE16 -> auto-start -> the engine. Instrumented for the EightyOne
 * crash: report-B sites ($0C6B/$0EAD/$1BA2), the RST 08 code decode,
 * ERR_NR, PPC, CH_ADD, and THE SP DEPTH AT THE ENGINE (if SP dips
 * below $7E19 the stack smashes the D-file = the frozen-picture bug).
 * Usage: node sim_user2_v066.js [frames] [year] [name] [gender]
 */
"use strict";
const fs = require("fs");
const BUILD = "/home/z/my-project/retro-tinyllm/simenv/build/zx81emulator";
const ZX81 = require(BUILD + "/machine/ZX81").default;
const Scanline = require(BUILD + "/display/Scanline").default;
const ROM = fs.readFileSync(process.env.SIMROM || "/home/z/my-project/retro-tinyllm/simenv/zx81.rom");
const TAPE = process.env.SIMTAPE ||
    "/home/z/my-project/retro-tinyllm/ports/zx81/16k_v072/GLM_v070_RT16_TOPICS.t81";

const FRAMES = +(process.argv[2] || 2400);
const YEAR = process.argv[3] || "2023";
const NAME = process.argv[4] || "SAFI";
const GEND = process.argv[5] || "F";

function parseTape(buf) {
    const blocks = [];
    let off = 4;
    while (off + 48 <= buf.length) {
        const name = buf.slice(off, off + 32).toString("latin1")
            .replace(/\0.*$/, "").trim();
        off += 32;
        const ln = parseInt(buf.slice(off, off + 16).toString("latin1")
            .replace(/\0.*$/, "").trim(), 10);
        off += 16;
        blocks.push({ name, data: buf.slice(off, off + ln) });
        off += ln;
    }
    return blocks;
}
function zxChar(c) {
    if (c === 0x00) return " ";
    if (c >= 0x1C && c <= 0x25) return String.fromCharCode(48 + c - 0x1C);
    if (c >= 0x26 && c <= 0x3F) return String.fromCharCode(65 + c - 0x26);
    const p = { 0x0B: '"', 0x0D: "$", 0x0E: ":", 0x0F: "?", 0x10: "(",
        0x11: ")", 0x12: ">", 0x13: "<", 0x14: "=", 0x15: "+", 0x16: "-",
        0x17: "*", 0x18: "/", 0x19: ";", 0x1A: ",", 0x1B: "." };
    return p[c] || "?";
}
const tape = parseTape(fs.readFileSync(TAPE));

class H extends ZX81 {
    constructor(tape) {
        super();
        this.tape = tape;
        for (let i = 0; i < ROM.length; i++) this.memory[i] = ROM[i];
        this.nmiGenerator = false;
        this.hsyncGenerator = false;
        this.z80.reset();
    }
    patch() {
        /* v0.7.2: a $02EC leader-search HALT mindig atugorva. A valodi
           gepen a HALT-ot a scanline NMI-k szakitjak meg (a generator
           a tanc utani walk alatt fegyverzett marad), az OUT $FD a
           $02ED-nel kikapcsolja a generatort es a betoltes megy tovabb.
           A szim NMI-modellje a HALT-fegyverzest nem hozza, ezert a
           $02EC-it vegtelen varakozas lenne - a gep ugyanolyan allapotban
           van, mint a valosag a megszakitasok utan: ugorjunk a $0356-ra. */
        if (this.z80.PC === 0x02EC && !this.nmiGenerator) {
            this.z80.PC = 0x0356;
        }
        if (this.z80.PC === 0x0356 && this.memory[0x0356] === 0x1f) {
            if (this.tape && this.tape.length) {
                let want = null;
                for (let a = 0x407D; a < 0x7F00; a++) {
                    if (this.memory[a] === 0xDE && this.memory[a + 1] === 0xEF &&
                        this.memory[a + 2] === 0x0B) {
                        let ok = true, s = "";
                        for (let k = 0; k < 8; k++) {
                            let c = this.memory[a + 3 + k];
                            const inv = (c & 0x80) !== 0;
                            c &= 0x7F;
                            if (c === 0x0B) break;
                            if (c >= 0x26 && c <= 0x3F)
                                s += String.fromCharCode(65 + c - 0x26);
                            else if (c >= 0x1C && c <= 0x25)
                                s += String.fromCharCode(48 + c - 0x1C);
                            else if (c === 0x16) s += "-";
                            else { ok = false; break; }
                            if (inv) break;
                        }
                        if (ok && s.length >= 4) { want = s; break; }
                    }
                }
                let blk = want ? this.tape.find(b => b.name === want) : null;
                if (!blk) blk = this.tape[0];
                let dd = blk.data, L = dd.length;
                while (L > 0 && dd[L - 1] !== 0x80) L--;
                let start = 0;
                /* v0.6.9: a blokk = [nev-rekord 8 B][kep a $4009-tol].
                   A nev-rekord: 7 display-kod + a 8. |0x80 (a ROM
                   IN-NAME egrezeset utanozva). A $4009-tol a KEP megy! */
                if (dd[0] !== 0x00 && dd[0] !== 0x01 && dd[0] !== 0xFF) {
                    while (start < L && (dd[start] & 0x80) === 0) start++;
                    start++;
                }
                for (let i = 0; i < L - start; i++)
                    this.memory[0x4009 + i] = dd[start + i];
                this.pop16();
                this.loadCount = (this.loadCount || 0) + 1;
                if (process.env.SIMDBG) {
                    let r = -1;
                    for (let a = 0x4090; a <= 0x7AFF; a++)
                        if (this.memory[a] === 0x52 && this.memory[a+1] === 0x54 &&
                            this.memory[a+2] === 0x4C && this.memory[a+3] === 0x4D)
                            { r = a; break; }
                    console.log("    [SIMDBG] load utan RTLM @$" +
                        (r < 0 ? -1 : r.toString(16)) + " D_FILE=$" +
                        ((this.memory[0x400C]) | (this.memory[0x400D] << 8)).toString(16));
                }
                console.log("  [load #" + this.loadCount + "] " + blk.name +
                    (want ? " (wanted " + want + ")" : " (any)") +
                    " " + (L - start) + " B");
                // v0.6.6 diag: a D-file state-sorok (rows 0-2) a betoltes UTAN
                if (this.loadCount >= 2) {
                    const st = [];
                    for (let r = 0; r < 3; r++) {
                        let h = "";
                        for (let c = 0; c < 32; c++)
                            h += this.memory[0x7B01 + r * 33 + c]
                                .toString(16).padStart(2, "0");
                        st.push(h);
                    }
                    console.log("  [state rows a load#" + this.loadCount +
                        " utan] " + st.join(" "));
                }
                return 0x0207;
            }
        }
        return this.z80.PC;
    }
    dfileText() {
        const d = this.memory[0x400C] | (this.memory[0x400D] << 8);
        const rows = [];
        for (let r = 0; r < 24; r++) {
            let line = "";
            const a = d + 1 + r * 33;
            for (let c = 0; c < 32; c++) line += zxChar(this.memory[a + c]);
            rows.push(line.replace(/\s+$/, ""));
        }
        return rows;
    }
}

const m = new H(tape);
const events = [];
const keygap = 25, keyhold = 3;
function press(frame, code, shift) {
    if (shift) events.push({ frame, type: "kdown", code: 16, shift: false });
    events.push({ frame, type: "kdown", code, shift: !!shift });
    events.push({ frame: frame + keyhold, type: "kup", code, shift: !!shift });
    if (shift)
        events.push({ frame: frame + keyhold, type: "kup", code: 16, shift: false });
}

// ---- the USER's key schedule (as on EightyOne) ----
let f = 150;
press(f, 74, false);                 // J = LOAD
press(f + keygap, 80, true);         // SHIFT+P = "
press(f + 2 * keygap, 80, true);
press(f + 3 * keygap, 13, false);    // ENTER
f += 120;
press(f, 82, false);                 // R = RUN
press(f + keygap, 13, false);
let kf = 570;
const answers = "n\r" + YEAR + "\r11\r11\r11\r11\r" + NAME + "\r" + GEND;
for (const ch of answers) {
    if (ch === "\r") press(kf, 13, false);
    else if (/[0-9]/.test(ch)) press(kf, ch.charCodeAt(0), false);
    else press(kf, ch.toUpperCase().charCodeAt(0), false);
    kf += 30;
}
press(1750, 13, false);              // the ENTER gate
// v0.6.6: a chat line after the banner - the user's v3 symptom was
// "echo but no reply", so the reply generation MUST be verified too
const CHAT = process.env.CHATMSG || "SZIA";
let cf = +(process.env.CHATFRAME || 2250);
for (const ch of CHAT) {
    press(cf, ch.toUpperCase().charCodeAt(0), false);
    cf += 30;
}
press(cf + 30, 13, false);           // ENTER sends the chat line
// ---- a USER v067 flowja: /topic 1 -> RT-TUDOM -> begepeles ----
let tf = +(process.env.TOPICFRAME || 2900);
press(tf, 67, true);                 // SHIFT+C = '/' (sajat dekoder!)
press(tf + 30, 84);                  // T
press(tf + 60, 79);                  // O
press(tf + 90, 80);                  // P
press(tf + 120, 73);                 // I
press(tf + 150, 67);                 // C
press(tf + 180, 32);                 // SPACE
press(tf + 210, 49);                 // 1
press(tf + 240, 13);                 // ENTER -> return 1 -> THEN LOAD
// a topic betoltese utan a chat: CSILLAG
let qf = +(process.env.TOPICCHATFRAME || 4900);
for (const ch of "CSILLAG") {
    press(qf, ch.charCodeAt(0), false);
    qf += 30;
}
press(qf + 30, 13, false);
events.sort((a, b) => a.frame - b.frame);

// ---- instrumentation ----
let evIdx = 0;
const origOp = m.z80.doOpcode.bind(m.z80);
const ring = [];
let lastErrNr = 0xFF;
let usrCount = 0, usrFrame = -1;
let reportHits = 0, bSeen = false;
let minSP = 0xFFFF, minSPframe = -1, minSPusr = 0;
let dfSmashLogged = false;
let escapeLogged = false;
const postUsrTrace = [];
m.z80.doOpcode = function () {
    const pc = m.z80.PC;
    /* v0.7.3 DANCETRACE: utasitasszintu kovetes a ROM $0207 tanc
       kornyeken (PC $0207-$0229) + a hivo oldalon (a SET/RES CDFLAG
       utasitasok). Kiirja: PC, CDFLAG, Carry - a lenyeg latszik:
       a tanc elszal-e a 17x OUT $FE fegyverzo sorozatig, vagy ter-e
       vissza azonnal a RET NC-n (a v0.7.2 fekete-kep hiba). */
    if (process.env.DANCETRACE && usrCount >= 2) {
        const cd = m.memory[0x403B];
        if ((pc >= 0x0207 && pc <= 0x0229) ||
            (pc >= 0x4C00 && pc <= 0x4CF0) ||
            (pc >= 0x7A40 && pc <= 0x7A62)) {
            const F = m.z80.regF !== undefined ? m.z80.regF :
                      (m.z80.F !== undefined ? m.z80.F : 0);
            const cf = (typeof F === "object" && F.get) ?
                       ((F.get() >> 0) & 1) : ((F & 1));
            console.log("  [DANCE] f" + (globalThis.simFrame||0) +
                " PC=$" + pc.toString(16) +
                " CDFLAG=$" + cd.toString(16).padStart(2,"0") +
                " C=" + cf +
                " nmiGen=" + m.nmiGenerator);
        }
    }
    if (process.env.NMITRACE) {
        if (globalThis.__nmiG === undefined) globalThis.__nmiG = m.nmiGenerator;
        if (m.nmiGenerator !== globalThis.__nmiG) {
            globalThis.__nmiG = m.nmiGenerator;
            if (usrCount >= 2)
                console.log("  [NMITRACE] f" + (globalThis.simFrame||0) +
                    " nmiGenerator -> " + m.nmiGenerator + " usr#" + usrCount +
                    " PC=$" + pc.toString(16));
        }
    }
    const fr = m.writeFrame || 0;
    ring.push(pc);
    if (ring.length > 60) ring.shift();
    // v0.6.6 diag: utasitasszintu nyomkovetes a generallas regioba
    // (GENTRACE=1): az elso PC $58E0-$5920-ben f>1500 utan
    const rg = (r) => (r && r.get) ? r.get() : r;
    if (pc >= 0x4082 && pc < 0x7AF5 && !(pc >= 0x7A40)) {
        globalThis.__engLast = { f: (globalThis.simFrame||0), pc: pc };
    }
    if (!globalThis.__stZero && (globalThis.simFrame||0) > 1700 &&
        m.memory[0x7B01] !== 0) {
        // a $7B01 meg nem nulla: figyeljuk a valtozasat
        globalThis.__stWatch = true;
    }
    if (globalThis.__stWatch && !globalThis.__stZero &&
        m.memory[0x7B01] === 0 && (globalThis.simFrame||0) > 1700) {
        globalThis.__stZero = true;
        console.log("  *** $7B01=0 LETT! f" + (globalThis.simFrame||0) +
            " - az utolso 25 PC ezelott:");
        console.log("    " + ring.slice(-25).map(x => "$" + x.toString(16)).join(" "));
    }
    if (!globalThis.__stZero && (globalThis.simFrame||0) > 1700 &&
        m.memory[0x7B01] === 0 && m.memory[0x7B02] === 0 &&
        m.memory[0x7B03] === 0 && m.memory[0x7B04] === 0) {
        globalThis.__stZero = true;
        console.log("  *** STATE NULLAZODOTT! f" + (globalThis.simFrame||0) +
            " PC=$" + pc.toString(16) +
            " engine utoljara: f" + ((globalThis.__engLast||{}).f) +
            " PC=$" + (((globalThis.__engLast||{}).pc)||0).toString(16));
    }
    if (pc >= 0x58E0 && pc < 0x5920) {
        globalThis.__inregion = (globalThis.__inregion || 0) + 1;
        if ((globalThis.simFrame || 0) > 2000)
            globalThis.__inregionLate = (globalThis.__inregionLate || 0) + 1;
    }
    if (process.env.GENTRACE && !globalThis.__gentrace &&
        (globalThis.simFrame || 0) >= +(process.env.GENTRACE_FRAME || 2350)) {
        globalThis.__gentrace = [];
        console.log("  [gentrace START] f" + (globalThis.simFrame||0) + " PC=$" + pc.toString(16));
    }
    if (globalThis.__gentrace && globalThis.__gentrace.length < 4000) {
        globalThis.__gentrace.push(
            "f" + fr + " PC=$" + pc.toString(16) +
            " SP=$" + rg(m.z80.SP).toString(16) +
            " HL=$" + rg(m.z80.HL).toString(16) +
            " DE=$" + rg(m.z80.DE).toString(16) +
            " BC=$" + rg(m.z80.BC).toString(16) +
            " A=$" + rg(m.z80.A).toString(16));
        if (globalThis.__gentrace.length === 6000)
            console.log("=== GENTRACE (4000 utasitas) ===\n" +
                globalThis.__gentrace.join("\n"));
    }
    // EVT: kritikus esemenyek (frame-idovel)
    if (pc === (process.env.MAINADDR ? parseInt(process.env.MAINADDR,16) : 0x4CB5)) {
        console.log("  [EVT] f" + (globalThis.simFrame||0) + " MAIN indul usr#" + usrCount);
        let found = 0;
        for (let a = 0x4090; a <= 0x7AFF; a++) {
            if (m.memory[a] === 0x52 && m.memory[a+1] === 0x54 &&
                m.memory[a+2] === 0x4C && m.memory[a+3] === 0x4D) {
                found++;
                if (found <= 2)
                    console.log("    [RTLM @ $" + a.toString(16) + " ver=$" +
                        (m.memory[a+4] | (m.memory[a+5] << 8)).toString(16) + " vocab=$" +
                        (m.memory[a+8] | (m.memory[a+9] << 8)).toString(16) + "]");
            }
        }
        console.log("    [RTLM darab: " + found + " a $4090-$7AFF zaban]");
    }
    if (pc === 0x40A5) {
        console.log("  [EVT] f" + (globalThis.simFrame||0) + " MAIN VISSZATERT -> cleanup ($40A5) usr#" + usrCount);
        const l1len = m.memory[0x407F] | (m.memory[0x4080] << 8);
        const l2 = 0x4085 + l1len;
        let hx = "";
        for (let i = 0; i < 40; i++) hx += m.memory[l2 + i].toString(16).padStart(2, "0") + " ";
        console.log("  [RET-kor line2 @$" + l2.toString(16) + "] " + hx);
    }
    if (pc === 0x40C4) {
        console.log("  [EVT] f" + (globalThis.simFrame||0) + " VEGSO RET a ROM-nak ($40B3) usr#" + usrCount +
            " HL=$" + rg(m.z80.HL).toString(16) + " BC=$" + rg(m.z80.BC).toString(16));
        if (!globalThis.__romtrace && usrCount >= 1) {
            globalThis.__romtrace = [];
            console.log("  [ROMTRACE START]");
        }
    }
    if (globalThis.__romtrace && globalThis.__romtrace.length < 2500 &&
        pc >= 0x207 && pc < 0x1f00 && !(pc >= 0x400 && pc < 0x500) &&
        !(pc >= 0x38 && pc <= 0x7c)) {
        globalThis.__romtrace.push((globalThis.simFrame||0) + " PC=$" + pc.toString(16) +
            " SP=$" + (m.z80.SP.get ? m.z80.SP.get() : m.z80.SP).toString(16) +
            " A=$" + rg(m.z80.A).toString(16) + " HL=$" + rg(m.z80.HL).toString(16));
        if (globalThis.__romtrace.length === 2500) {
            console.log("=== ROMTRACE (2500) ===");
            const t = globalThis.__romtrace;
            for (let i = 0; i < t.length; i += 8)
                console.log("  " + t.slice(i, i + 8).join(" | "));
            globalThis.__romtrace = null;
        }
    }
    if (pc === 0x0356 && m.memory[0x0356] === 0x1f) console.log("  [EVT] f" + fr + " ROM TAPE LOADER ($0356)!");
    if (pc === 0x04CF && !globalThis.__edSeen) { globalThis.__edSeen = 1;
        console.log("  [EVT] f" + fr + " ROM SZERKESZTO ($04CF) elso alkalommal usr#" + usrCount); }
    // ALLTRACE: a /topic ENTER utani lepesek
    if (process.env.ALLTRACE_FRAME &&
        (globalThis.simFrame||0) >= +process.env.ALLTRACE_FRAME &&
        globalThis.__alltrace === undefined) {
        globalThis.__alltrace = [];
        console.log("  [ALLTRACE START] f" + (globalThis.simFrame||0));
    }
    if (globalThis.__alltrace &&
        pc >= 0x4082 && pc < 0x7a40 &&
        !(pc >= 0x50b0 && pc <= 0x5170) &&         /* a getline idle hurok */
        !(pc >= 0x4a4f && pc <= 0x4a60) &&         /* az msleep */
        !(pc >= 0x41b9 && pc <= 0x41f1)) {         /* a regi msleep */        /* a szerkeszto fo ciklusa nem kell */
        globalThis.__alltrace.push(
            (globalThis.simFrame||0) + " PC=$" + pc.toString(16) +
            " SP=$" + (m.z80.SP.get ? m.z80.SP.get() : m.z80.SP).toString(16) +
            " IX=$" + ((m.z80.IX.get ? m.z80.IX.get() : m.z80.IX)).toString(16) +
            " A=$" + rg(m.z80.A).toString(16));
        if (globalThis.__alltrace.length === 6000) {
            console.log("=== ALLTRACE (6000) ===");
            const t = globalThis.__alltrace;
            for (let i = 0; i < t.length; i += 8)
                console.log("  " + t.slice(i, i + 8).join(" | "));
            globalThis.__alltrace = null;   /* kiiras utan leall */
        }
    }
    // a menekules-detektor: a PC kikerult a ROM+RAM-zonakbol
    if (!escapeLogged && pc >= 0x8000 && usrCount >= 2) {
        escapeLogged = true;
        console.log("  *** PC-ESCAPE! f" + fr + " PC=$" + pc.toString(16) +
            " SP=$" + (m.z80.SP.get ? m.z80.SP.get() : m.z80.SP).toString(16) +
            " usr#" + usrCount + "\n    az elozo 60 PC:\n    " +
            ring.map(x => "$" + x.toString(16)).join(" "));
    }
    const sp = m.z80.SP.get ? m.z80.SP.get() : m.z80.SP;
    if (pc >= 0x4082 && pc < 0x7B00) {   // engine code: track SP depth
        if (sp < minSP) { minSP = sp; minSPframe = fr; minSPusr = usrCount; }
        if (sp < 0x7E19 && !dfSmashLogged) {
            dfSmashLogged = true;
            console.log("  *** SP=$" + sp.toString(16) +
                " BELEPETT A D-FILE ZONABA ($7E19) f" + fr +
                " usr#" + usrCount + " prev:" +
                ring.slice(-12).map(x => "$" + x.toString(16)).join(" "));
        }
    }
    if (pc === 0x4082) {
        usrCount++;
        if (usrFrame < 0) usrFrame = fr;
        console.log("  [USR #" + usrCount + "] f" + fr +
            " SP=$" + sp.toString(16) +
            " ERR_NR=$" + m.memory[0x4000].toString(16));
    }
    if (usrCount >= 2 && postUsrTrace.length < 20000)
        postUsrTrace.push(pc);
    if (pc === 0x0C6B || pc === 0x0EAD || pc === 0x1BA2) {
        reportHits++;
        if (reportHits <= 8)
            console.log("  [REPORT-B SITE $" + pc.toString(16) + "] f" + fr +
                " usr#" + usrCount + " prev:" +
                ring.slice(-12).map(x => "$" + x.toString(16)).join(" "));
    }
    if (pc === 0x0008) {
        const ret = m.memory[sp] | (m.memory[sp + 1] << 8);
        const code = m.memory[ret] & 0xFF;
        if (code !== 0xFF)
            console.log("  [RST 08] f" + fr + " code=$" + code.toString(16) +
                " (report " + ((code + 1) & 0xFF).toString(16).toUpperCase() +
                ") ret=$" + ret.toString(16) + " usr#" + usrCount);
        if (code === 0x0A && !bSeen) {
            bSeen = true;
            console.log("  *** REPORT B! f" + fr + " usr#" + usrCount +
                " SP=$" + sp.toString(16) +
                " minSP=$" + minSP.toString(16) + " last 60 PC:\n    " +
                ring.map(x => "$" + x.toString(16)).join(" "));
        }
    }
    if (process.env.USR4TRACE && usrCount >= 3 &&
        pc === 0x4082 && globalThis.__u4t === undefined) {
        globalThis.__u4t = 0;
        globalThis.__u4n = 0;
        console.log("  [USR4TRACE START]");
    }
    if (globalThis.__u4t !== undefined && globalThis.__u4t < 1) {
        globalThis.__u4t = 1;
    }
    if (globalThis.__u4n !== undefined && globalThis.__u4n < 300) {
        globalThis.__u4n++;
        console.log("  u4 " + globalThis.__u4n + " PC=$" + pc.toString(16) +
            " SP=$" + sp.toString(16) +
            " HL=$" + rg(m.z80.HL).toString(16) +
            " IX=$" + rg(m.z80.IX).toString(16));
    }
    if (process.env.USR3TRACE && usrCount === 2 &&
        globalThis.__u3t === undefined) {
        globalThis.__u3t = 0;
        console.log("  [USR3TRACE START]");
    }
    if (globalThis.__u3t !== undefined && globalThis.__u3t < 2500) {
        globalThis.__u3t++;
        if (globalThis.__u3t <= 2500)
            console.log("  u3 " + globalThis.__u3t + " PC=$" + pc.toString(16) +
                " SP=$" + sp.toString(16) +
                " HL=$" + rg(m.z80.HL).toString(16) +
                " IX=$" + rg(m.z80.IX).toString(16));
    }
    const en = m.memory[0x4000];
    if (en !== lastErrNr) {
        if (usrFrame >= 0 || (en !== 0xFF && en !== 0x00))
            console.log("  [ERR_NR] f" + fr + " $" + lastErrNr.toString(16) +
                " -> $" + en.toString(16) + " usr#" + usrCount +
                " PPC=" + (m.memory[0x4007] | (m.memory[0x4008] << 8)));
        lastErrNr = en;
    }
    return origOp();
};

let lastRows = "";
let chatPoked = false;

/* ---- v0.7.3b TV-RENDER (a Drawer.draw() logikajanak fejtelen masolata) ----
   TVW x TVH raszter, 0=fekete, 1=feher, 2=szurke (nem megrajzolt). A draw()
   a scanline pixeleit a raszterbe irja, a HSYNC utan sort leptet, a frame
   vegen (VSYNC) ujrafordul. A PNG mentes a SNAPFRAMES-ben megadott frameekkel
   tortenik (egyszeru PPM -> PNG konverzio nelkul: nyers PPM + Python
   atalakitas, mert nincs canvas node-ban). */
const TVW = 520, TVH = 380;
const HTOL = 405, HSYNC_MINLEN = 10;
const VSYNC_TOLLERANCEMIN = 283, VSYNC_TOLLERANCEMAX = 283 + 40, VSYNC_MINLEN = 350;
if (process.env.SNAPFRAMES) {
    globalThis.__tv = new Uint8Array(TVW * TVH);
    globalThis.__tvScanLen = 2 + m.tPerScanLine * 2;
    globalThis.__tvFrameNo = 0;
    console.log("  [TV-RENDER] bekapcsolva, SNAPFRAMES=" + process.env.SNAPFRAMES);
}
function __tvDraw(sl) {
    const tv = globalThis.__tv;
    let rasterX = globalThis.__tvRasterX, rasterY = globalThis.__tvRasterY;
    let dest = globalThis.__tvDest, frameNo = globalThis.__tvFrameNo;
    let bufferPos = dest + frameNo * TVW + rasterX;
    for (let i = 0; i < sl.getLength(); i++) {
        tv[bufferPos] = sl.getPixel(i) ? 0 : 1; /* Black=0, White=1 (invertalva) */
        rasterX++;
        bufferPos++;
        if (rasterX > globalThis.__tvScanLen) {
            rasterX = 0;
            dest += TVW;
            bufferPos = dest + frameNo * TVW + rasterX;
            rasterY++;
            if (rasterY >= TVH) i = sl.nextLine();
        }
    }
    if (sl.checkSyncLength(HSYNC_MINLEN)) {
        if (rasterX > HTOL) {
            rasterX = 0;
            rasterY++;
            dest += TVW;
        }
        if (rasterY >= TVH || rasterY >= VSYNC_TOLLERANCEMAX ||
            (sl.getSyncLength() > VSYNC_MINLEN && rasterY > VSYNC_TOLLERANCEMIN)) {
            /* completeFrame: a maradek szurke */
            let bp = dest + frameNo * TVW + rasterX;
            while (rasterY <= 32 + 240) {
                while (rasterX <= 42 + 320) {
                    tv[bp] = 2; rasterX++; bp++;
                }
                rasterX = 0; rasterY++; dest = rasterY * TVW;
                bp = dest + frameNo * TVW + rasterX;
            }
            rasterX = 0; rasterY = 0; dest = 0;
            frameNo = 0;
        }
    }
    globalThis.__tvRasterX = rasterX; globalThis.__tvRasterY = rasterY;
    globalThis.__tvDest = dest; globalThis.__tvFrameNo = frameNo;
}
function __tvSavePng(f) {
    const fs2 = require("fs");
    const tv = globalThis.__tv;
    /* PPM: minden sorban a feher/szurke/fekete blokkokbol
       fekete=0, szurke=128, feher=255 */
    const hdr = "P6\n" + TVW + " " + TVH + "\n255\n";
    const buf = Buffer.alloc(hdr.length + TVW * TVH * 3);
    buf.write(hdr, 0, "ascii");
    let o = hdr.length;
    for (let i = 0; i < TVW * TVH; i++) {
        /* tv[i]: 0 = pixel true = VILAGOS (a Drawer 'Black'=0xFFFFFFFF!), 
           1 = pixel false = SOTET (a szoveg!), 2 = szurke (nem megrajzolt) */
        const v = tv[i] === 0 ? 255 : tv[i] === 1 ? 0 : 128;
        buf[o++] = v; buf[o++] = v; buf[o++] = v;
    }
    const dir = process.env.SNAPDIR || "/tmp/tvsnap";
    try { fs2.mkdirSync(dir, { recursive: true }); } catch (e) {}
    fs2.writeFileSync(dir + "/tv_f" + f + ".ppm", buf);
    console.log("  [TV-SNAP] f" + f + " -> " + dir + "/tv_f" + f + ".ppm");
}
for (let f2 = 0; f2 < FRAMES; f2++) {
    globalThis.simFrame = f2;
    while (evIdx < events.length && events[evIdx].frame === f2) {
        const e = events[evIdx++];
        if (e.type === "kdown") m.keyDown(e.code, e.shift);
        else m.keyUp(e.code, e.shift);
    }
    // v0.6.6 diag: az igazi chat struct megtalalasa. A SZIA beerkezes
    // utan hist[0..3]=[0x14,0x1B,0x0A,0x02] (s,z,i,a charset indexek)
    // es hist_n=4 - ez egyedi alairas. CHATADDR=xxxx: max_new=72 irasa
    // mar a state_apply utan (a valasz-generalas tesztelese).
    if (usrCount >= 2 && !chatPoked && f2 % 25 === 0) {
        for (let a = 0x4082; a < 0x7F00 - 20; a++) {
            if (m.memory[a] === 0x14 && m.memory[a + 1] === 0x1B &&
                m.memory[a + 2] === 0x0A && m.memory[a + 3] === 0x02 &&
                m.memory[a + 8] === 4 && m.memory[a + 9] === 0) {
                console.log("  [chat struct] $%04X (hist='szia' hist_n=4"
                    .replace("%04X", a.toString(16)) +
                    " max_new=" + (m.memory[a + 18] | (m.memory[a + 19] << 8)) + ")");
                chatPoked = true;
                break;
            }
        }
        if (process.env.CHATADDR && !chatPoked) {
            const a = parseInt(process.env.CHATADDR, 16);
            if (m.memory[a + 18] === 0 && m.memory[a + 19] === 0) {
                m.memory[a + 18] = 72; m.memory[a + 19] = 0;
                console.log("  [chat poke] $%04X +18 := 72 (max_new)"
                    .replace("%04X", a.toString(16)));
                chatPoked = true;
            }
        }
    }
    let t = 0;
    /* v0.7.3b TV-RENDER: a scanline-okat nem dobjuk el - a Drawer.draw()
       logikajat masoljuk fejtelenul (rasterX/rasterY/dest + HSYNC kezeles)
       es a TVW x TVH rasztert feltoltjuk, majd a megadott frame-eknel
       PNG-be mentunk (SNAPFRAMES=2250,2520,...). A kep az IGAZSAG: a
       D-file mem-audit nem latja, mit mutat a valodi TV (sync-tores,
       osszeomlot sav, stb.). */
    let tvFrame = null;
    if (globalThis.__tv) {
        globalThis.__tv.fill(2); /* 2 = szurke (a Drawer Gray-ja) */
        globalThis.__tvRasterX = 0; globalThis.__tvRasterY = 0; globalThis.__tvDest = 0;
    }
    let nmiThisFrame = 0;
    const origNmi = m.z80.nmi.bind(m.z80);
    m.z80.nmi = function (len) {
        nmiThisFrame++;
        return origNmi(len);
    };
    while (t < m.tPerFrame) {
        const sl = new Scanline();
        t += m.doScanline(sl);
        if (globalThis.__tv) __tvDraw(sl);
    }
    m.z80.nmi = origNmi;
    /* v0.7.3b DFAUDIT: a D-file strukturalis egeszsege frame-enkent.
       A user hipotezise: 'a kep osszeomlott, kozepen fekete sav -
       kepernyomemoria-osszeomlas'. Ezt itt teszteljuk kozvetlenul:
       (a) minden sor 33. bajtja ($76 HALT sorzaro) ep-e,
       (b) van-e bit7-es bajt a 32 cellaban (inverz video = 'fekete sav'
           a valodi gepen!),
       (c) a D_FILE mutato (0x400C) valtozott-e,
       (d) a DF_CC (0x400E) a sorok kozott van-e (a ROM 'lower screen'
           es a D-file koze - ha kifut, a kep osszeomlik). */
    if (process.env.DFAUDIT && f2 > 1400) {
        const dfile = m.memory[0x400C] | (m.memory[0x400D] << 8);
        if (!globalThis.__dfBase) globalThis.__dfBase = dfile;
        let bad76 = -1, bit7row = -1, badcell = -1;
        for (let r = 0; r < 24 && bad76 < 0; r++) {
            const a = dfile + 1 + r * 33;
            if (m.memory[a + 32] !== 0x76) bad76 = r;
            for (let c = 0; c < 32; c++) {
                if (m.memory[a + c] & 0x80) { bit7row = r; badcell = c; break; }
            }
            if (bit7row >= 0 && bad76 < 0) bad76 = r; /* mindket hibat jelezzuk */
            if (bit7row >= 0) break;
        }
        const dfcc = m.memory[0x400E] | (m.memory[0x400F] << 8);
        const inRange = dfcc >= dfile && dfcc < dfile + 24 * 33;
        if ((bad76 >= 0 || bit7row >= 0 || dfile !== globalThis.__dfBase || !inRange) &&
            !globalThis.__dfErrLogged) {
            globalThis.__dfErrLogged = f2;
            console.log("  *** DFAUDIT SERULES! f" + f2 +
                " bad76row=" + bad76 + " bit7row=" + bit7row +
                " cell=" + badcell +
                " D_FILE=$" + dfile.toString(16) +
                " (alap $" + globalThis.__dfBase.toString(16) + ")" +
                " DF_CC=$" + dfcc.toString(16) +
                (inRange ? "" : " DF_CC KIVUL!"));
        }
        /* a scanline-k DB hibaja is 'fekete savot' ad: az auditorium */
        if (!globalThis.__dfErrLogged) globalThis.__dfOkFrames = (globalThis.__dfOkFrames || 0) + 1;
    }
    /* a TV frame lezarasa: ami nem lett megrajzolva, szurke marad */
    if (globalThis.__tv && process.env.SNAPFRAMES &&
        process.env.SNAPFRAMES.split(",").includes(String(f2))) {
        __tvSavePng(f2);
    }
    /* v0.7.3: a 'VIDEO HALOTT' mar nem az elso NMI=0 frame - az a generalas
       alatti SZANDERUNKOS FAST-allapot (~740 frame, v0.6.7 ota). Ami szamit:
       (a) a generalas UTAN a kep visszater-e (nmiGen ujra true), es
       (b) a vegallapot: SLOW (CDFLAG bit6) + fegyverzett generator.
       A VIDEO RECOVERY log az elso olyan frame amikor az NMI ujra elindul
       miutan mar halott volt (a generalas alatt), es a vegso riportban
       a VIDEO OK/HALOTT dontes. */
    if (nmiThisFrame === 0) {
        globalThis.__videoWasDead = true; /* a FAST idoszak es a halott video is */
    } else if (globalThis.__videoWasDead) {
        globalThis.__videoWasDead = false;
        globalThis.__videoRevived = f2;
        console.log("  [VIDEO RECOVERY] f" + f2 +
            " - a kep visszatert (NMI ujra fut, CDFLAG=$" +
            m.memory[0x403B].toString(16) + ")");
    }
    /* a VIDEO HALOTT csak akkor valos riasztas, ha SLOW-ban (CDFLAG bit6=1,
       a vegso blokk mar lefutott) az NMI 0 - az a tenyleges fekete-kep */
    if (!globalThis.__videoDead && f2 > 3200 && nmiThisFrame === 0 &&
        (m.memory[0x403B] & 0x40) !== 0) {
        globalThis.__videoDead = f2;
        console.log("  *** VIDEO HALOTT a generalas utan! f" + f2 +
            " (a kep nem terhet vissza - FEKETE KEP+BORDER!" +
            " elozo PC-k: " + ring.slice(-20).map(x => "$" + x.toString(16)).join(" ") + ")");
    }
    if (process.env.VIDEOTRACE && f2 > 1900 &&
        nmiThisFrame !== (globalThis.__nmiPrev ?? 312)) {
        console.log("  [VIDEOTRACE] f" + f2 + " NMI/frame: " +
            globalThis.__nmiPrev + " -> " + nmiThisFrame +
            " usr#" + usrCount + " PC=$" + m.z80.PC.toString(16) +
            " CDFLAG=$" + m.memory[0x403B].toString(16));
    }
    globalThis.__nmiPrev = nmiThisFrame;
    if (f2 % 250 === 0) {
        const rows = m.dfileText().filter(r => r).join("|");
        const sig = rows.slice(0, 60);
        if (sig !== lastRows) { lastRows = sig;
            console.log("  f" + f2 + " rows: " + rows.slice(0, 150)); }
    }
}

console.log("  [engine utoljara lathato: f" + ((globalThis.__engLast||{}).f) +
    " PC=$" + (((globalThis.__engLast||{}).pc)||0).toString(16) + "]");
console.log("  [VEGSO RING - az utolso 60 tenyleges PC]:");
console.log("    " + ring.map(x => "$" + x.toString(16)).join(" "));
console.log("  [diag] GENTRACE env='" + (process.env.GENTRACE||"") +
    "' inregion=" + (globalThis.__inregion||0) +
    " inregionLate=" + (globalThis.__inregionLate||0) +
    " simFrame=" + (globalThis.simFrame||0));
console.log("== VEGEREDMENY (ev=" + YEAR + " nev=" + NAME + " nem=" + GEND + ") ==");
// v0.6.6 diag: a state-sorok a futas vegen (a motor pack_state-ja latja-e)
{
    const st = [];
    for (let r = 0; r < 3; r++) {
        let h = "";
        for (let c = 0; c < 32; c++)
            h += m.memory[0x7B01 + r * 33 + c].toString(16).padStart(2, "0");
        st.push(h);
    }
    console.log("  [state rows a vegén] " + st.join(" "));
}
console.log("  USR hivasok: " + usrCount + " (elso: f" + usrFrame + ")");
/* v0.6.8 diag: a line 2 es a namtab a /topic utan */
{
    const l1len = m.memory[0x407F] | (m.memory[0x4080] << 8);
    const l2 = 0x4085 + l1len;
    let hx = "";
    for (let i = 0; i < 40; i++) hx += m.memory[l2 + i].toString(16).padStart(2, "0") + " ";
    console.log("  [line2 @$" + l2.toString(16) + " l1len=" + l1len + "] " + hx);
    /* a namtab: a modell utan - az _namtab C valtozo keresese helyett
       a memoriat szkenneljuk a $A1 + 14 mintara */
    for (let a = 0x7000; a < 0x7B00; a++) {
        if (m.memory[a] === 0xA1 && m.memory[a+1] === 14) {
            let t = "namtab @$" + a.toString(16) + ": ";
            for (let k = 0; k < 16; k++) {
                let nm = "";
                for (let j = 0; j < 8; j++) {
                    const c = m.memory[a + 2 + k*8 + j];
                    nm += (c >= 0x26 && c <= 0x3F) ? String.fromCharCode(65 + c - 0x26) :
                          (c >= 0x1C && c <= 0x25) ? String.fromCharCode(48 + c - 0x1C) :
                          (c === 0x16) ? "-" : (c === 0) ? "" : "?";
                }
                t += nm + " ";
            }
            console.log("  [" + t + "]");
            break;
        }
    }
}
console.log("  REPORT-B site talalatok: " + reportHits +
    (bSeen ? "  *** B RIPORT TORTENT ***" : " (nem tortent B)"));
console.log("  minSP a motorban: $" + minSP.toString(16) +
    " (f" + minSPframe + ", usr#" + minSPusr + ")" +
    (dfSmashLogged ? "  *** D-FILE ZONA MEGSZORULT ***" : " - D-file intact"));
const sys = (a) => "$" + (m.memory[a] | (m.memory[a + 1] << 8)).toString(16);
console.log("  ERR_NR=$" + m.memory[0x4000].toString(16) +
    " PPC=" + (m.memory[0x4007] | (m.memory[0x4008] << 8)) +
    " D_FILE=" + sys(0x400C) + " VARS=" + sys(0x4010) +
    " E_LINE=" + sys(0x4014) + " CH_ADD=" + sys(0x4016) +
    " NXTLIN=" + sys(0x4029));
console.log('  MEM $7560-$75E0 a futas utan:');
console.log('    ' + Array.from({length:0x81},(_,i)=>m.memory[0x7560+i].toString(16).padStart(2,'0')).join(' '));
const d = m.dfileText();
d.forEach((r, i) => { if (r) console.log("  r" + i + ": |" + r + "|"); });
if (postUsrTrace.length) {
    console.log("  az USR utan az utolso 60 PC:");
    console.log("    " + postUsrTrace.slice(-60)
        .map(x => "$" + x.toString(16)).join(" "));
}
