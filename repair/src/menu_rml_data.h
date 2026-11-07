// Design of the embedded menu: RmlUi documents (HTML + RCSS).
// v5: two windows only — Home and Visuals. Visuals carries the 3D preview
// toggle, the effect master switches and every effect setting as collapsible
// sections. Module rows use vector icons built from RCSS shapes (boxes,
// circles, borders) and animate through CSS transitions; the C++ side only
// toggles an "anim" class on <body> a couple of times per second, so the
// icons animate without SVG or keyframes (RmlUi supports neither).
// The crown is gone from the interface on purpose.
// Kept as raw strings so the DLL stays a single file with no disk access.
#pragma once

static const char kMenuRcss[] = R"RCSS(
body {
    font-family: GAY;
    font-size: 20px;
    color: #f0f8f3;
    display: block;
    background-color: #0208059e;
}
.win {
    position: absolute;
    display: flex;
    flex-direction: column;
    background-color: #06110bf5;
    decorator: linear-gradient(165deg, #101e15f5, #050c08fa);
    border: 1px #ffffff17;
    border-radius: 14px;
    transition: border-color 0.15s;
}
.win.focus {
    decorator: linear-gradient(165deg, #132519f8, #061009fc);
    border-color: #6ef58c55;
}
.win-head {
    height: 44px;
    flex-basis: 44px;
    display: flex;
    flex-direction: row;
    align-items: center;
    padding: 0 8px 0 14px;
    border-bottom: 1px #ffffff0d;
    border-radius: 13px 13px 0 0;
    drag: drag;
}
.win.folded .win-head {
    border-bottom-width: 0px;
    border-radius: 13px;
}
.win-dot {
    width: 9px;
    height: 9px;
    border-radius: 5px;
    background-color: #3d4a43;
    margin-right: 10px;
    transition: background-color 0.15s;
}
.win.focus .win-dot { background-color: #6ef58c; }
.win-title {
    font-size: 15px;
    font-weight: bold;
    letter-spacing: 2px;
    color: #a5c5ae;
    transition: color 0.15s;
}
.win.focus .win-title { color: #e2ffe7; }
.win-sub {
    font-size: 12px;
    letter-spacing: 1px;
    color: #5f7a68;
    margin-left: 10px;
}
.win-flex { flex-grow: 1; }
.foldbtn {
    width: 30px;
    height: 30px;
    font-size: 16px;
    font-weight: bold;
    color: #89a792;
    background-color: #ffffff08;
    border: 1px #ffffff12;
    border-radius: 9px;
    text-align: center;
    line-height: 30px;
    cursor: pointer;
    transition: background-color 0.12s, color 0.12s;
}
.foldbtn:hover {
    background-color: #6ef58c24;
    color: #c9ffd7;
}
/* Fold glyphs are plain ASCII: - and +. U+25BE renders as tofu in Segoe UI. */
.f-plus, .f-minus { display: none; }
.win .f-minus, .sec .f-minus { display: inline; }
.win.folded .f-plus, .sec.folded .f-plus { display: inline; }
.win.folded .f-minus, .sec.folded .f-minus { display: none; }
.iconbtn {
    width: 30px;
    height: 30px;
    margin-left: 6px;
    font-size: 17px;
    color: #89a792;
    background-color: #ffffff08;
    border: 1px #ffffff12;
    border-radius: 9px;
    text-align: center;
    line-height: 30px;
    cursor: pointer;
    transition: background-color 0.12s, color 0.12s;
}
.iconbtn:hover {
    background-color: #ff5a5a24;
    color: #ff9c9c;
    border-color: #ff5a5a59;
}
.win-body {
    flex-basis: 0px;
    flex-grow: 1;
    display: block;
    overflow-y: auto;
    padding: 10px 12px 12px 12px;
}
.win.folded .win-body { display: none; }
.win-body scrollbarvertical { width: 10px; }
.win-body scrollbarvertical slidertrack {
    background-color: #ffffff0a;
    border-radius: 5px;
}
.win-body scrollbarvertical sliderbar {
    width: 8px;
    border-radius: 4px;
    background-color: #6ef58c73;
}
.brand { display: block; padding: 2px 2px 10px 2px; }
.brand-name { display: block; font-size: 25px; font-weight: bold; color: #f4fff6; }
.brand-accent { color: #6ef58c; }
.brand-sub {
    display: block;
    font-size: 12px;
    letter-spacing: 2px;
    color: #5f7a68;
    padding-top: 3px;
}
.module-row {
    height: 52px;
    display: flex;
    flex-direction: row;
    align-items: center;
    padding: 0 6px;
    border-radius: 10px;
    transition: background-color 0.12s;
}
.module-row:hover { background-color: #ffffff08; }
.m-text { display: block; flex-grow: 1; padding-left: 12px; }
.m-name { display: block; font-size: 18px; color: #e9f5ec; }
.m-desc { display: block; font-size: 13px; color: #89a792; margin-top: 1px; }

/* ------------------------------------------------------------------ */
/* Vector icons: RCSS shapes inside a 34x34 chip. Every part is an     */
/* absolutely positioned box; "body.anim ..." flips between two states */
/* and the transition property animates the change.                   */
/* ------------------------------------------------------------------ */
.m-ico {
    width: 34px;
    height: 34px;
    flex-basis: 34px;
    position: relative;
    border-radius: 9px;
    background-color: #6ef58c1a;
}
.ip { position: absolute; display: block; }

/* FPS: speedometer with a sweeping needle. */
.g-arc {
    left: 9px; top: 12px; width: 16px; height: 8px;
    border-top: 2px #6ef58c; border-left: 2px #6ef58c; border-right: 2px #6ef58c;
    border-radius: 8px 8px 0 0;
}
.g-needle {
    left: 16px; top: 18px; width: 2px; height: 7px;
    background-color: #c4ff62;
    transform-origin: 1px 7px;
    transform: rotate(-55deg);
    transition: transform 0.45s;
}
body.anim .g-needle { transform: rotate(55deg); }
.g-hub {
    left: 15px; top: 17px; width: 4px; height: 4px;
    border-radius: 2px;
    background-color: #6ef58c;
}
/* CPS: pulsing ripple. */
.r-dot {
    left: 14px; top: 14px; width: 6px; height: 6px;
    border-radius: 3px;
    background-color: #6ef58c;
}
.r-ring {
    left: 10px; top: 10px; width: 14px; height: 14px;
    border-radius: 7px;
    border: 2px #6ef58c;
    transition: transform 0.45s, opacity 0.45s;
}
body.anim .r-ring { transform: scale(1.7); opacity: 0; }
/* Clock: face with a rotating minute hand. */
.c-face {
    left: 9px; top: 9px; width: 16px; height: 16px;
    border-radius: 8px;
    border: 2px #a5c5ae;
}
.c-hour {
    left: 16px; top: 12px; width: 2px; height: 5px;
    background-color: #a5c5ae;
}
.c-min {
    left: 16px; top: 11px; width: 2px; height: 6px;
    background-color: #6ef58c;
    transform-origin: 1px 6px;
    transition: transform 0.45s;
}
body.anim .c-min { transform: rotate(180deg); }
/* Keystrokes: two 3D keycaps, the left one presses. */
.k-a {
    left: 8px; top: 10px; width: 9px; height: 9px;
    border: 1px #6ef58c8c; border-radius: 3px;
    background-color: #6ef58c1f;
    transition: transform 0.18s, background-color 0.18s;
}
body.anim .k-a { transform: translate(0px, 2px); background-color: #6ef58c55; }
.k-b {
    left: 18px; top: 14px; width: 10px; height: 9px;
    border: 1px #6ef58c4d; border-radius: 3px;
    background-color: #6ef58c14;
}
/* Watermark: shield badge with a pulse. */
.w-badge {
    left: 11px; top: 9px; width: 12px; height: 16px;
    border: 1px #6ef58c8c;
    border-radius: 6px 6px 6px 1px;
    background-color: #6ef58c14;
    transition: background-color 0.45s;
}
body.anim .w-badge { background-color: #6ef58c3d; }
.w-bar1 { left: 14px; top: 14px; width: 6px; height: 2px; background-color: #6ef58c; }
.w-bar2 { left: 16px; top: 18px; width: 4px; height: 2px; background-color: #6ef58c; }
/* Coordinates: rotating crosshair. */
.x-rot {
    position: absolute; display: block;
    left: 8px; top: 8px; width: 18px; height: 18px;
    transition: transform 0.6s;
}
body.anim .x-rot { transform: rotate(45deg); }
.x-ring {
    left: 2px; top: 2px; width: 14px; height: 14px;
    border-radius: 7px;
    border: 2px #6ef58c;
}
.x-dot { left: 8px; top: 8px; width: 2px; height: 2px; background-color: #6ef58c; }
.x-t1 { left: 8px; top: 0px; width: 2px; height: 4px; background-color: #6ef58c; }
.x-t2 { left: 8px; top: 14px; width: 2px; height: 4px; background-color: #6ef58c; }
.x-t3 { left: 0px; top: 8px; width: 4px; height: 2px; background-color: #6ef58c; }
.x-t4 { left: 14px; top: 8px; width: 4px; height: 2px; background-color: #6ef58c; }
/* Resolution: monitor with a lit screen. */
.v-scr {
    left: 8px; top: 9px; width: 18px; height: 12px;
    border: 2px #a5c5ae; border-radius: 2px;
    transition: background-color 0.45s;
}
body.anim .v-scr { background-color: #6ef58c33; }
.v-stand { left: 15px; top: 21px; width: 4px; height: 3px; background-color: #a5c5ae; }
.v-base { left: 12px; top: 24px; width: 10px; height: 2px; background-color: #a5c5ae; }
/* Performance: CPU die with pulsing substrate. */
.p-die {
    left: 10px; top: 10px; width: 14px; height: 14px;
    border: 1px #6ef58c8c; border-radius: 2px;
    background-color: #6ef58c24;
    transition: background-color 0.45s;
}
body.anim .p-die { background-color: #6ef58c4d; }
.p-core { left: 14px; top: 14px; width: 6px; height: 6px; background-color: #6ef58c; }
.p-l1 { left: 10px; top: 6px; width: 2px; height: 4px; background-color: #a5c5ae; }
.p-l2 { left: 16px; top: 6px; width: 2px; height: 4px; background-color: #a5c5ae; }
.p-l3 { left: 22px; top: 6px; width: 2px; height: 4px; background-color: #a5c5ae; }
.p-l4 { left: 10px; top: 24px; width: 2px; height: 4px; background-color: #a5c5ae; }
.p-l5 { left: 16px; top: 24px; width: 2px; height: 4px; background-color: #a5c5ae; }
.p-l6 { left: 22px; top: 24px; width: 2px; height: 4px; background-color: #a5c5ae; }
/* Module list: equalizer bars. */
.e-bar { width: 3px; background-color: #6ef58c; transition: height 0.45s; }
.e-b1 { left: 8px; top: 10px; height: 8px; }
.e-b2 { left: 12px; top: 10px; height: 15px; }
.e-b3 { left: 16px; top: 10px; height: 10px; }
.e-b4 { left: 20px; top: 10px; height: 13px; }
body.anim .e-b1 { height: 15px; }
body.anim .e-b2 { height: 8px; }
body.anim .e-b3 { height: 16px; }
body.anim .e-b4 { height: 9px; }
/* China hat master: stepped floating cone. */
.h-cwrap { position: absolute; display: block; left: 0px; top: 0px; width: 34px; height: 34px; }
.h-c1 { left: 10px; top: 10px; width: 14px; height: 4px; background-color: #6ef58c; }
.h-c2 { left: 13px; top: 14px; width: 8px; height: 4px; background-color: #6ef58c; }
.h-c3 { left: 15px; top: 18px; width: 4px; height: 4px; background-color: #6ef58c; }
.h-glint { left: 19px; top: 11px; width: 2px; height: 3px; background-color: #c4ff62; }
.h-cwrap { transition: transform 0.45s; }
body.anim .h-cwrap { transform: translate(0px, -2px); }
/* Trail master: three staggered dashes. */
.t-b1 { left: 8px; top: 12px; width: 8px; height: 2px; border-radius: 1px; background-color: #6ef58c; }
.t-b2 { left: 13px; top: 16px; width: 8px; height: 2px; border-radius: 1px; background-color: #6ef58c; }
.t-b3 { left: 18px; top: 20px; width: 8px; height: 2px; border-radius: 1px; background-color: #6ef58c; }
/* Jump rings master: two concentric ellipses. */
.j-r1 {
    left: 8px; top: 13px; width: 18px; height: 9px;
    border-radius: 4px;
    border: 1px #6ef58c;
    transition: transform 0.45s;
}
body.anim .j-r1 { transform: scale(1.15); }
.j-r2 {
    left: 12px; top: 15px; width: 10px; height: 5px;
    border-radius: 2px;
    border: 1px #6ef58c99;
}
/* Katana master: tilted blade. */
.kt-blade {
    left: 9px; top: 17px; width: 16px; height: 2px;
    background-color: #c4ff62;
    transform: rotate(-35deg);
    transform-origin: 2px 1px;
    transition: transform 0.45s;
}
body.anim .kt-blade { transform: rotate(-50deg); }
.kt-guard { left: 22px; top: 12px; width: 4px; height: 4px; border-radius: 2px; background-color: #6ef58c; }
/* 3D preview: offset cubes. */
.b-sq {
    left: 9px; top: 12px; width: 11px; height: 11px;
    border: 2px #a5c5ae; border-radius: 2px;
}
.b-top {
    left: 14px; top: 8px; width: 11px; height: 11px;
    border: 2px #6ef58c; border-radius: 2px;
    transition: transform 0.45s;
}
body.anim .b-top { transform: translate(1px, 1px); }

.switch {
    position: relative;
    width: 58px;
    height: 32px;
    border: 1px #ffffff14;
    border-radius: 16px;
    background-color: #182419;
    cursor: pointer;
    transition: background-color 0.16s, border-color 0.16s;
}
.switch .knob {
    position: absolute;
    left: 3px;
    top: 3px;
    width: 24px;
    height: 24px;
    border-radius: 12px;
    background-color: #5a6c60;
    transition: left 0.16s, background-color 0.16s;
}
.switch.on {
    background-color: #6ef58c3d;
    decorator: linear-gradient(90deg, #2fa960, #6ef58c);
    border-color: #6ef58c8c;
}
.switch.on .knob {
    left: 29px;
    background-color: #6ef58c;
}
.slider-row {
    display: flex;
    flex-direction: row;
    align-items: center;
    padding: 3px 6px;
    border-radius: 9px;
    transition: background-color 0.12s;
}
.slider-row:hover { background-color: #ffffff06; }
.slider-label { width: 156px; font-size: 15px; color: #cfe3d6; }
input[type="range"] {
    width: 150px;
    height: 26px;
}
input[type="range"] slidertrack {
    height: 6px;
    margin-top: 10px;
    background-color: #151f18;
    border-radius: 3px;
}
input[type="range"] sliderbar {
    width: 18px;
    height: 18px;
    top: -6px;
    border-radius: 9px;
    background-color: #6ef58c;
    transition: background-color 0.1s;
}
input[type="range"] sliderbar:hover { background-color: #c4ff62; }
input[type="range"] sliderarrowdec, input[type="range"] sliderarrowinc {
    width: 0px;
    height: 0px;
}
.slider-value {
    width: 54px;
    text-align: right;
    font-size: 15px;
    color: #a5c5ae;
}
.color-chip {
    width: 24px;
    height: 24px;
    margin-left: 8px;
    border-radius: 7px;
    border: 1px #ffffff2e;
}
.greenbtn {
    display: block;
    height: 44px;
    margin-top: 8px;
    font-size: 16px;
    font-weight: bold;
    color: #05230f;
    background-color: #6ef58c;
    decorator: linear-gradient(180deg, #c4ff62, #6ef58c 52%, #2fa960);
    border: 1px #e2ffc366;
    border-radius: 12px;
    text-align: center;
    cursor: pointer;
    transition: background-color 0.13s;
}
.greenbtn:hover { decorator: linear-gradient(180deg, #d9ff92, #83f5a5 52%, #3db870); }
.greenbtn:active { decorator: linear-gradient(180deg, #b9ea63, #64d681 52%, #2b9a57); }
.note {
    display: block;
    font-size: 13px;
    color: #89a792;
    padding: 6px 4px 8px 4px;
}
.foot {
    display: block;
    font-size: 12px;
    color: #5f7a68;
    padding: 8px 4px 2px 4px;
}
#btn-layout { color: #89a792; cursor: pointer; transition: color 0.12s; }
#btn-layout:hover { color: #6ef58c; }

/* Collapsible settings sections in the Visuals window. */
.sec {
    display: block;
    margin-top: 10px;
    border: 1px #ffffff0f;
    border-radius: 12px;
    background-color: #ffffff05;
}
.sec-head {
    height: 42px;
    display: flex;
    flex-direction: row;
    align-items: center;
    padding: 0 10px;
    border-radius: 11px;
    cursor: pointer;
    transition: background-color 0.12s;
}
.sec-head:hover { background-color: #6ef58c0d; }
.sec-dot {
    width: 8px;
    height: 8px;
    border-radius: 4px;
    background-color: #6ef58c;
    margin-right: 10px;
}
.sec-title {
    font-size: 16px;
    font-weight: bold;
    letter-spacing: 1px;
    color: #cfe3d6;
}
.sec-count {
    font-size: 12px;
    color: #5f7a68;
    margin-left: 8px;
}
.sec-fold {
    margin-left: auto;
    width: 26px;
    height: 26px;
    font-size: 15px;
    font-weight: bold;
    color: #89a792;
    background-color: #ffffff08;
    border: 1px #ffffff12;
    border-radius: 8px;
    text-align: center;
    line-height: 26px;
}
.sec-body { display: block; padding: 2px 8px 10px 8px; }
.sec.folded .sec-body { display: none; }
#tt-status {
    display: block;
    font-size: 14px;
    color: #6ef58c;
    padding: 8px 4px 2px 4px;
}
#tt-diag {
    display: block;
    font-size: 12px;
    color: #89a792;
    padding-bottom: 4px;
}
)RCSS";

static const char kMenuRml[] = R"RML(
<rml>
<head>
    <title>GAYClient</title>
    <style>
    </style>
</head>
<body id="doc-body">
<div class="win" id="win-home">
    <div class="win-head" id="head-home">
        <span class="win-dot"/>
        <span class="win-title">GAYCLIENT</span>
        <span class="win-sub">HOME</span>
        <span class="win-flex"/>
        <button class="foldbtn" id="fold-0"><span class="f-plus">+</span><span class="f-minus">–</span></button>
        <button class="iconbtn" id="btn-close">×</button>
    </div>
    <div class="win-body">
        <div class="brand">
            <span class="brand-name">GAY<span class="brand-accent">CLIENT</span></span>
            <span class="brand-sub">GAME MENU · BEDROCK 1.1.5</span>
        </div>
        <button class="greenbtn" id="btn-hudeditor">Настроить HUD</button>
        <div class="note">Окна тащатся за заголовок, колесо мыши прокручивает список, стрелка в углу сворачивает окно. «Настроить HUD» прячет меню: тащи элементы HUD мышкой, выход — Esc. <span id="btn-layout">Сбросить раскладку</span></div>
        <div class="module-row"><div class="m-ico"><span class="ip g-arc"/><span class="ip g-needle"/><span class="ip g-hub"/></div><div class="m-text"><div class="m-name">FPS</div><div class="m-desc">Счётчик кадров в секунду</div></div><button class="switch" id="sw-fps"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-ico"><span class="ip r-dot"/><span class="ip r-ring"/></div><div class="m-text"><div class="m-name">CPS</div><div class="m-desc">Клики в секунду</div></div><button class="switch" id="sw-cps"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-ico"><span class="ip c-face"/><span class="ip c-hour"/><span class="ip c-min"/></div><div class="m-text"><div class="m-name">Часы</div><div class="m-desc">Текущее время</div></div><button class="switch" id="sw-clock"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-ico"><span class="ip k-a"/><span class="ip k-b"/></div><div class="m-text"><div class="m-name">Клавиши</div><div class="m-desc">Отображение нажатых клавиш</div></div><button class="switch" id="sw-keys"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-ico"><span class="ip w-badge"/><span class="ip w-bar1"/><span class="ip w-bar2"/></div><div class="m-text"><div class="m-name">Водяной знак</div><div class="m-desc">Надпись GAYClient на экране</div></div><button class="switch" id="sw-watermark"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-ico"><span class="x-rot"><span class="ip x-ring"/><span class="ip x-dot"/><span class="ip x-t1"/><span class="ip x-t2"/><span class="ip x-t3"/><span class="ip x-t4"/></span></div><div class="m-text"><div class="m-name">Координаты</div><div class="m-desc">Позиция в мире</div></div><button class="switch" id="sw-coords"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-ico"><span class="ip v-scr"/><span class="ip v-stand"/><span class="ip v-base"/></div><div class="m-text"><div class="m-name">Разрешение</div><div class="m-desc">Размер окна игры</div></div><button class="switch" id="sw-res"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-ico"><span class="ip p-die"/><span class="ip p-core"/><span class="ip p-l1"/><span class="ip p-l2"/><span class="ip p-l3"/><span class="ip p-l4"/><span class="ip p-l5"/><span class="ip p-l6"/></div><div class="m-text"><div class="m-name">Производительность</div><div class="m-desc">Загрузка CPU и RAM</div></div><button class="switch" id="sw-perf"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-ico"><span class="ip e-bar e-b1"/><span class="ip e-bar e-b2"/><span class="ip e-bar e-b3"/><span class="ip e-bar e-b4"/></div><div class="m-text"><div class="m-name">Список модулей</div><div class="m-desc">Активные модули на экране</div></div><button class="switch" id="sw-modlist"><span class="knob"/></button></div>
        <div class="foot">v1.3 · RmlUi</div>
    </div>
</div>
<div class="win" id="win-visuals">
    <div class="win-head" id="head-visuals">
        <span class="win-dot"/>
        <span class="win-title">VISUALS</span>
        <span class="win-sub">ЭФФЕКТЫ</span>
        <span class="win-flex"/>
        <button class="foldbtn" id="fold-1"><span class="f-plus">+</span><span class="f-minus">–</span></button>
    </div>
    <div class="win-body">
        <div class="module-row"><div class="m-ico"><span class="ip b-sq"/><span class="ip b-top"/></div><div class="m-text"><div class="m-name">3D превью</div><div class="m-desc">Портрет игрока справа</div></div><button class="switch" id="sw-preview"><span class="knob"/></button></div>
        <div class="note">Главные выключатели. Тонкая настройка — в секциях ниже.</div>
        <div class="module-row"><div class="m-ico"><span class="h-cwrap"><span class="ip h-c1"/><span class="ip h-c2"/><span class="ip h-c3"/><span class="ip h-glint"/></span></div><div class="m-text"><div class="m-name">China Hat</div><div class="m-desc">Конус над головой</div></div><button class="switch" id="sw-chinahat"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-ico"><span class="ip t-b1"/><span class="ip t-b2"/><span class="ip t-b3"/></div><div class="m-text"><div class="m-name">След</div><div class="m-desc">Цветной след за игроком</div></div><button class="switch" id="sw-trail"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-ico"><span class="ip j-r1"/><span class="ip j-r2"/></div><div class="m-text"><div class="m-name">Кольца прыжка</div><div class="m-desc">Круги при прыжке</div></div><button class="switch" id="sw-jump"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-ico"><span class="ip kt-blade"/><span class="ip kt-guard"/></div><div class="m-text"><div class="m-name">Катана</div><div class="m-desc">Замена алмазного меча</div></div><button class="switch" id="sw-katana"><span class="knob"/></button></div>
        <div class="sec" id="sec-ch">
            <div class="sec-head">
                <span class="sec-dot"/>
                <span class="sec-title">CHINA HAT</span>
                <span class="sec-count">конус</span>
                <span class="sec-fold"><span class="f-plus">+</span><span class="f-minus">–</span></span>
            </div>
            <div class="sec-body">
                <div class="slider-row"><div class="slider-label">Включение эффекта</div><button class="switch" id="sw-gfx-chinahat"><span class="knob"/></button></div>
                <div class="slider-row"><div class="slider-label">Заливка конуса</div><button class="switch" id="sw-ch-fill"><span class="knob"/></button></div>
                <div class="slider-row"><div class="slider-label">Радужный режим</div><button class="switch" id="sw-ch-rainbow"><span class="knob"/></button></div>
                <div class="slider-row"><div class="slider-label">Скорость радуги</div><input type="range" id="sl-ch-rainbow" min="0" max="100" step="1" value="30"/><div class="slider-value" id="ro-ch-rainbow"/><div class="color-chip" id="chip-ch"/></div>
                <div class="slider-row"><div class="slider-label">Прозрачность</div><input type="range" id="sl-ch-opacity" min="0" max="100" step="1" value="70"/><div class="slider-value" id="ro-ch-opacity"/></div>
                <div class="slider-row"><div class="slider-label">Радиус</div><input type="range" id="sl-ch-radius" min="0.2" max="3" step="0.01" value="0.85"/><div class="slider-value" id="ro-ch-radius"/></div>
                <div class="slider-row"><div class="slider-label">Высота конуса</div><input type="range" id="sl-ch-height" min="0.2" max="3" step="0.01" value="0.6"/><div class="slider-value" id="ro-ch-height"/></div>
                <div class="slider-row"><div class="slider-label">Отступ от головы</div><input type="range" id="sl-ch-offset" min="0" max="2" step="0.01" value="0.1"/><div class="slider-value" id="ro-ch-offset"/></div>
                <div class="slider-row"><div class="slider-label">Амплитуда плавания</div><input type="range" id="sl-ch-amp" min="0" max="1" step="0.01" value="0.05"/><div class="slider-value" id="ro-ch-amp"/></div>
                <div class="slider-row"><div class="slider-label">Скорость плавания</div><input type="range" id="sl-ch-floatspeed" min="0" max="3" step="0.01" value="1"/><div class="slider-value" id="ro-ch-floatspeed"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: красный</div><input type="range" id="sl-ch-r" min="0" max="1" step="0.01" value="0.16"/><div class="slider-value" id="ro-ch-r"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: зелёный</div><input type="range" id="sl-ch-g" min="0" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-ch-g"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: синий</div><input type="range" id="sl-ch-b" min="0" max="1" step="0.01" value="0.21"/><div class="slider-value" id="ro-ch-b"/></div>
            </div>
        </div>
        <div class="sec" id="sec-tr">
            <div class="sec-head">
                <span class="sec-dot"/>
                <span class="sec-title">СЛЕД</span>
                <span class="sec-count">trail</span>
                <span class="sec-fold"><span class="f-plus">+</span><span class="f-minus">–</span></span>
            </div>
            <div class="sec-body">
                <div class="slider-row"><div class="slider-label">Включение эффекта</div><button class="switch" id="sw-gfx-trail"><span class="knob"/></button></div>
                <div class="slider-row"><div class="slider-label">Заливка</div><button class="switch" id="sw-tr-fill"><span class="knob"/></button></div>
                <div class="slider-row"><div class="slider-label">Радужный режим</div><button class="switch" id="sw-tr-rainbow"><span class="knob"/></button></div>
                <div class="slider-row"><div class="slider-label">Скорость радуги</div><input type="range" id="sl-tr-rainbow" min="0" max="100" step="1" value="30"/><div class="slider-value" id="ro-tr-rainbow"/><div class="color-chip" id="chip-tr"/></div>
                <div class="slider-row"><div class="slider-label">Прозрачность</div><input type="range" id="sl-tr-opacity" min="0" max="100" step="1" value="60"/><div class="slider-value" id="ro-tr-opacity"/></div>
                <div class="slider-row"><div class="slider-label">Высота следа</div><input type="range" id="sl-tr-height" min="0.2" max="3" step="0.01" value="1"/><div class="slider-value" id="ro-tr-height"/></div>
                <div class="slider-row"><div class="slider-label">Время жизни следа</div><input type="range" id="sl-tr-life" min="0.2" max="4" step="0.05" value="1.5"/><div class="slider-value" id="ro-tr-life"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: красный</div><input type="range" id="sl-tr-r" min="0" max="1" step="0.01" value="0.16"/><div class="slider-value" id="ro-tr-r"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: зелёный</div><input type="range" id="sl-tr-g" min="0" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-tr-g"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: синий</div><input type="range" id="sl-tr-b" min="0" max="1" step="0.01" value="0.21"/><div class="slider-value" id="ro-tr-b"/></div>
            </div>
        </div>
        <div class="sec" id="sec-jr">
            <div class="sec-head">
                <span class="sec-dot"/>
                <span class="sec-title">КОЛЬЦА ПРЫЖКА</span>
                <span class="sec-count">jump</span>
                <span class="sec-fold"><span class="f-plus">+</span><span class="f-minus">–</span></span>
            </div>
            <div class="sec-body">
                <div class="slider-row"><div class="slider-label">Включение эффекта</div><button class="switch" id="sw-gfx-jump"><span class="knob"/></button></div>
                <div class="slider-row"><div class="slider-label">Радужный режим</div><button class="switch" id="sw-jr-rainbow"><span class="knob"/></button></div>
                <div class="slider-row"><div class="slider-label">Скорость радуги</div><input type="range" id="sl-jr-rainbow" min="0" max="100" step="1" value="30"/><div class="slider-value" id="ro-jr-rainbow"/><div class="color-chip" id="chip-jr"/></div>
                <div class="slider-row"><div class="slider-label">Радиус</div><input type="range" id="sl-jr-radius" min="0.2" max="3" step="0.01" value="0.7"/><div class="slider-value" id="ro-jr-radius"/></div>
                <div class="slider-row"><div class="slider-label">Прозрачность</div><input type="range" id="sl-jr-opacity" min="0" max="100" step="1" value="60"/><div class="slider-value" id="ro-jr-opacity"/></div>
                <div class="slider-row"><div class="slider-label">Длительность</div><input type="range" id="sl-jr-duration" min="0.1" max="3" step="0.05" value="0.8"/><div class="slider-value" id="ro-jr-duration"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: красный</div><input type="range" id="sl-jr-r" min="0" max="1" step="0.01" value="0.16"/><div class="slider-value" id="ro-jr-r"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: зелёный</div><input type="range" id="sl-jr-g" min="0" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-jr-g"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: синий</div><input type="range" id="sl-jr-b" min="0" max="1" step="0.01" value="0.21"/><div class="slider-value" id="ro-jr-b"/></div>
            </div>
        </div>
        <div class="sec" id="sec-kt">
            <div class="sec-head">
                <span class="sec-dot"/>
                <span class="sec-title">КАТАНА</span>
                <span class="sec-count">katana</span>
                <span class="sec-fold"><span class="f-plus">+</span><span class="f-minus">–</span></span>
            </div>
            <div class="sec-body">
                <div class="slider-row"><div class="slider-label">Включить катану</div><button class="switch" id="sw-kt-master"><span class="knob"/></button></div>
                <div class="slider-row"><div class="slider-label">Размер</div><input type="range" id="sl-kt-scale" min="0.4" max="1" step="0.01" value="0.7"/><div class="slider-value" id="ro-kt-scale"/></div>
                <div class="slider-row"><div class="slider-label">Наклон клинка</div><input type="range" id="sl-kt-tilt" min="0.2" max="1.55" step="0.01" value="0.9"/><div class="slider-value" id="ro-kt-tilt"/></div>
                <div class="slider-row"><div class="slider-label">Смещение в сторону</div><input type="range" id="sl-kt-side" min="0" max="0.8" step="0.01" value="0.25"/><div class="slider-value" id="ro-kt-side"/></div>
                <div class="slider-row"><div class="slider-label">Высота в руке</div><input type="range" id="sl-kt-height" min="0.6" max="1.8" step="0.01" value="1.1"/><div class="slider-value" id="ro-kt-height"/></div>
                <div class="slider-row"><div class="slider-label">Смещение вперёд</div><input type="range" id="sl-kt-forward" min="-0.5" max="1" step="0.01" value="0.2"/><div class="slider-value" id="ro-kt-forward"/></div>
                <div class="slider-row"><div class="slider-label">Непрозрачность</div><input type="range" id="sl-kt-opacity" min="0.3" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-kt-opacity"/></div>
                <div class="slider-row"><div class="slider-label">Сторона (1-е лицо)</div><input type="range" id="sl-kt-fpside" min="0" max="0.8" step="0.01" value="0.3"/><div class="slider-value" id="ro-kt-fpside"/></div>
                <div class="slider-row"><div class="slider-label">Вперёд (1-е лицо)</div><input type="range" id="sl-kt-fpforward" min="-0.3" max="1" step="0.01" value="0.25"/><div class="slider-value" id="ro-kt-fpforward"/></div>
                <div id="tt-status">Katana: ожидание</div>
                <div id="tt-diag">—</div>
            </div>
        </div>
        <div class="foot">v1.3 · RmlUi</div>
    </div>
</div>
</body>
</rml>
)RML";
