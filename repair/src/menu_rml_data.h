// Design of the embedded menu: RmlUi documents (HTML + RCSS).
// v6: two windows (Home + Visuals). Switches are square checkboxes (empty
// square when off, check mark when on). No decorative icon chips: module rows
// are clean text rows. Sections animate open/closed through a max-height
// transition, the document fades in/out, and windows fold through a short
// C++ height interpolation. Typography is plain ASCII: hyphens only, no
// middle-dot separators (they read as generated text).
// Kept as raw strings so the DLL stays a single file with no disk access.
#pragma once

static const char kMenuRcss[] = R"RCSS(
body {
    font-family: GAY;
    font-size: 20px;
    color: #f0f8f3;
    display: block;
    background-color: #0208059e;
    transition: opacity 0.16s cubic-out;
}
/* Driven by the C++ side through SetClass: inline SetProperty does not run
   CSS transitions in RmlUi, class flips do. */
body.faded { opacity: 0; }
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
.win.folded .win-body { display: none; }
/* While the C++ side interpolates the window height the body must stay
   visible (it is only hidden once the fold settles); equal specificity,
   so this rule has to come after the one above. */
.win.folding .win-body {
    display: block;
    overflow-y: hidden;
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
/* Fold glyphs are plain ASCII: - and + (U+25BE is tofu in Segoe UI). */
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
.m-text { display: block; flex-grow: 1; }
.m-name { display: block; font-size: 18px; color: #e9f5ec; }
.m-desc { display: block; font-size: 13px; color: #89a792; margin-top: 1px; }

/* Checkbox: an empty square when off, a check mark when on. */
.check {
    position: relative;
    width: 26px;
    height: 26px;
    flex-basis: 26px;
    border: 1px #ffffff26;
    border-radius: 7px;
    background-color: #16211a;
    cursor: pointer;
    transition: background-color 0.12s, border-color 0.12s;
}
.check:hover { border-color: #6ef58c66; }
.check .ck {
    position: absolute;
    display: block;
    left: 6px;
    top: 6px;
    width: 11px;
    height: 6px;
    border-left: 3px #05230f;
    border-bottom: 3px #05230f;
    border-radius: 2px;
    transform: rotate(-45deg);
    opacity: 0;
    transition: opacity 0.1s;
}
.check.on {
    background-color: #6ef58c3d;
    decorator: linear-gradient(180deg, #c4ff62, #6ef58c 55%, #2fa960);
    border-color: #6ef58c8c;
}
.check.on .ck { opacity: 1; }

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
    height: 28px;
}
/* The widget positions the track through margin-top and the knob through
   the knob's own margin-top (a CSS top: would be ignored), so the two
   margins are what centers the knob on the track. */
input[type="range"] slidertrack {
    height: 8px;
    margin-top: 9px;
    background-color: #101a13;
    border: 1px #ffffff0f;
    border-radius: 4px;
}
/* Value fill: the widget sizes sliderprogress up to the knob. */
input[type="range"] sliderprogress {
    display: block;
    background-color: #6ef58c99;
    border-radius: 4px;
}
input[type="range"] sliderbar {
    width: 16px;
    height: 16px;
    margin-top: 6px;
    border-radius: 8px;
    border: 1px #e2ffc366;
    background-color: #6ef58c;
    decorator: linear-gradient(180deg, #c4ff62, #6ef58c 52%, #2fa960);
}
input[type="range"] sliderbar:hover {
    decorator: linear-gradient(180deg, #d9ff92, #83f5a5 52%, #3db870);
}
input[type="range"] sliderbar:active {
    decorator: linear-gradient(180deg, #b9ea63, #64d681 52%, #2b9a57);
}
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

/* Collapsible settings sections: the body animates its max-height. */
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
.sec-title {
    font-size: 16px;
    font-weight: bold;
    letter-spacing: 1px;
    color: #cfe3d6;
}
.sec-flex { flex-grow: 1; }
.sec-head .check { margin-right: 8px; }
.sec-fold {
    margin-left: 8px;
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
.sec-body {
    display: block;
    padding: 2px 8px 10px 8px;
    overflow: hidden;
    max-height: 560px;
    opacity: 1;
    transition: max-height 0.16s cubic-out, opacity 0.12s cubic-out, padding 0.16s cubic-out;
}
.sec.folded .sec-body {
    max-height: 0px;
    opacity: 0;
    padding-top: 0px;
    padding-bottom: 0px;
}
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
        <span class="win-flex"/>
        <button class="foldbtn" id="fold-0"><span class="f-plus">+</span><span class="f-minus">-</span></button>
        <button class="iconbtn" id="btn-close">×</button>
    </div>
    <div class="win-body">
        <div class="brand">
            <span class="brand-name">GAY<span class="brand-accent">CLIENT</span></span>
            <span class="brand-sub">BEDROCK 1.1.5</span>
        </div>
        <button class="greenbtn" id="btn-hudeditor">Настроить HUD</button>
        <div class="note">Окна тащатся за заголовок, колесо мыши прокручивает список, кнопка в углу сворачивает окно. «Настроить HUD» прячет меню: тащи элементы мышкой, выход - Esc. <span id="btn-layout">Сбросить раскладку</span></div>
        <div class="module-row"><div class="m-text"><div class="m-name">FPS</div><div class="m-desc">Счётчик кадров в секунду</div></div><button class="check" id="sw-fps"><span class="ck"/></button></div>
        <div class="module-row"><div class="m-text"><div class="m-name">CPS</div><div class="m-desc">Клики в секунду</div></div><button class="check" id="sw-cps"><span class="ck"/></button></div>
        <div class="module-row"><div class="m-text"><div class="m-name">Часы</div><div class="m-desc">Текущее время</div></div><button class="check" id="sw-clock"><span class="ck"/></button></div>
        <div class="module-row"><div class="m-text"><div class="m-name">Клавиши</div><div class="m-desc">Отображение нажатых клавиш</div></div><button class="check" id="sw-keys"><span class="ck"/></button></div>
        <div class="module-row"><div class="m-text"><div class="m-name">Водяной знак</div><div class="m-desc">Надпись GAYClient на экране</div></div><button class="check" id="sw-watermark"><span class="ck"/></button></div>
        <div class="module-row"><div class="m-text"><div class="m-name">Координаты</div><div class="m-desc">Позиция в мире</div></div><button class="check" id="sw-coords"><span class="ck"/></button></div>
        <div class="module-row"><div class="m-text"><div class="m-name">Разрешение</div><div class="m-desc">Размер окна игры</div></div><button class="check" id="sw-res"><span class="ck"/></button></div>
        <div class="module-row"><div class="m-text"><div class="m-name">Производительность</div><div class="m-desc">Загрузка CPU и RAM</div></div><button class="check" id="sw-perf"><span class="ck"/></button></div>
        <div class="module-row"><div class="m-text"><div class="m-name">Список модулей</div><div class="m-desc">Активные модули на экране</div></div><button class="check" id="sw-modlist"><span class="ck"/></button></div>
        <div class="foot">v1.4</div>
    </div>
</div>
<div class="win" id="win-visuals">
    <div class="win-head" id="head-visuals">
        <span class="win-dot"/>
        <span class="win-title">VISUALS</span>
        <span class="win-flex"/>
        <button class="foldbtn" id="fold-1"><span class="f-plus">+</span><span class="f-minus">-</span></button>
    </div>
    <div class="win-body">
        <div class="module-row"><div class="m-text"><div class="m-name">3D превью</div><div class="m-desc">Портрет игрока справа</div></div><button class="check" id="sw-preview"><span class="ck"/></button></div>
        <div class="sec" id="sec-ch">
            <div class="sec-head">
                <span class="sec-title">CHINA HAT</span>
                <span class="sec-flex"/>
                <button class="check" id="sw-gfx-chinahat"><span class="ck"/></button>
                <span class="sec-fold"><span class="f-plus">+</span><span class="f-minus">-</span></span>
            </div>
            <div class="sec-body">
                                <div class="slider-row"><div class="slider-label">Заливка конуса</div><button class="check" id="sw-ch-fill"><span class="ck"/></button></div>
                <div class="slider-row"><div class="slider-label">Радужный режим</div><button class="check" id="sw-ch-rainbow"><span class="ck"/></button></div>
                <div class="slider-row"><div class="slider-label">Скорость радуги</div><input type="range" id="sl-ch-rainbow" min="0" max="100" step="1" value="30"/><div class="slider-value" id="ro-ch-rainbow"/><div class="color-chip" id="chip-ch"/></div>
                <div class="slider-row"><div class="slider-label">Прозрачность</div><input type="range" id="sl-ch-opacity" min="0" max="100" step="1" value="70"/><div class="slider-value" id="ro-ch-opacity"/></div>
                <div class="slider-row"><div class="slider-label">Радиус</div><input type="range" id="sl-ch-radius" min="0.3" max="1" step="0.01" value="0.58"/><div class="slider-value" id="ro-ch-radius"/></div>
                <div class="slider-row"><div class="slider-label">Высота конуса</div><input type="range" id="sl-ch-height" min="0.12" max="0.8" step="0.01" value="0.34"/><div class="slider-value" id="ro-ch-height"/></div>
                <div class="slider-row"><div class="slider-label">Отступ от головы</div><input type="range" id="sl-ch-offset" min="-0.2" max="0.4" step="0.01" value="0"/><div class="slider-value" id="ro-ch-offset"/></div>
                <div class="slider-row"><div class="slider-label">Амплитуда плавания</div><input type="range" id="sl-ch-amp" min="0" max="0.2" step="0.002" value="0.07"/><div class="slider-value" id="ro-ch-amp"/></div>
                <div class="slider-row"><div class="slider-label">Скорость плавания</div><input type="range" id="sl-ch-floatspeed" min="0.1" max="1" step="0.01" value="0.31"/><div class="slider-value" id="ro-ch-floatspeed"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: красный</div><input type="range" id="sl-ch-r" min="0" max="1" step="0.01" value="0.2"/><div class="slider-value" id="ro-ch-r"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: зелёный</div><input type="range" id="sl-ch-g" min="0" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-ch-g"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: синий</div><input type="range" id="sl-ch-b" min="0" max="1" step="0.01" value="0.42"/><div class="slider-value" id="ro-ch-b"/></div>
            </div>
        </div>
        <div class="sec" id="sec-tr">
            <div class="sec-head">
                <span class="sec-title">СЛЕД</span>
                <span class="sec-flex"/>
                <button class="check" id="sw-gfx-trail"><span class="ck"/></button>
                <span class="sec-fold"><span class="f-plus">+</span><span class="f-minus">-</span></span>
            </div>
            <div class="sec-body">
                                <div class="slider-row"><div class="slider-label">Заливка</div><button class="check" id="sw-tr-fill"><span class="ck"/></button></div>
                <div class="slider-row"><div class="slider-label">Радужный режим</div><button class="check" id="sw-tr-rainbow"><span class="ck"/></button></div>
                <div class="slider-row"><div class="slider-label">Скорость радуги</div><input type="range" id="sl-tr-rainbow" min="0" max="100" step="1" value="30"/><div class="slider-value" id="ro-tr-rainbow"/><div class="color-chip" id="chip-tr"/></div>
                <div class="slider-row"><div class="slider-label">Прозрачность</div><input type="range" id="sl-tr-opacity" min="0" max="100" step="1" value="60"/><div class="slider-value" id="ro-tr-opacity"/></div>
                <div class="slider-row"><div class="slider-label">Высота следа</div><input type="range" id="sl-tr-height" min="0.3" max="1" step="0.01" value="0.8"/><div class="slider-value" id="ro-tr-height"/></div>
                <div class="slider-row"><div class="slider-label">Время жизни следа</div><input type="range" id="sl-tr-life" min="1" max="6" step="0.05" value="3"/><div class="slider-value" id="ro-tr-life"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: красный</div><input type="range" id="sl-tr-r" min="0" max="1" step="0.01" value="0.08"/><div class="slider-value" id="ro-tr-r"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: зелёный</div><input type="range" id="sl-tr-g" min="0" max="1" step="0.01" value="0.86"/><div class="slider-value" id="ro-tr-g"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: синий</div><input type="range" id="sl-tr-b" min="0" max="1" step="0.01" value="0.21"/><div class="slider-value" id="ro-tr-b"/></div>
            </div>
        </div>
        <div class="sec" id="sec-jr">
            <div class="sec-head">
                <span class="sec-title">КОЛЬЦА ПРЫЖКА</span>
                <span class="sec-flex"/>
                <button class="check" id="sw-gfx-jump"><span class="ck"/></button>
                <span class="sec-fold"><span class="f-plus">+</span><span class="f-minus">-</span></span>
            </div>
            <div class="sec-body">
                                <div class="slider-row"><div class="slider-label">Радужный режим</div><button class="check" id="sw-jr-rainbow"><span class="ck"/></button></div>
                <div class="slider-row"><div class="slider-label">Скорость радуги</div><input type="range" id="sl-jr-rainbow" min="0" max="100" step="1" value="30"/><div class="slider-value" id="ro-jr-rainbow"/><div class="color-chip" id="chip-jr"/></div>
                <div class="slider-row"><div class="slider-label">Радиус</div><input type="range" id="sl-jr-radius" min="0.4" max="3" step="0.02" value="1.4"/><div class="slider-value" id="ro-jr-radius"/></div>
                <div class="slider-row"><div class="slider-label">Прозрачность</div><input type="range" id="sl-jr-opacity" min="0" max="100" step="1" value="60"/><div class="slider-value" id="ro-jr-opacity"/></div>
                <div class="slider-row"><div class="slider-label">Длительность</div><input type="range" id="sl-jr-duration" min="0.3" max="3" step="0.02" value="1"/><div class="slider-value" id="ro-jr-duration"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: красный</div><input type="range" id="sl-jr-r" min="0" max="1" step="0.01" value="0.2"/><div class="slider-value" id="ro-jr-r"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: зелёный</div><input type="range" id="sl-jr-g" min="0" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-jr-g"/></div>
                <div class="slider-row"><div class="slider-label">Цвет: синий</div><input type="range" id="sl-jr-b" min="0" max="1" step="0.01" value="0.42"/><div class="slider-value" id="ro-jr-b"/></div>
            </div>
        </div>
        <div class="sec" id="sec-kt">
            <div class="sec-head">
                <span class="sec-title">КАТАНА</span>
                <span class="sec-flex"/>
                <button class="check" id="sw-kt-master"><span class="ck"/></button>
                <span class="sec-fold"><span class="f-plus">+</span><span class="f-minus">-</span></span>
            </div>
            <div class="sec-body">
                                <div class="slider-row"><div class="slider-label">Размер</div><input type="range" id="sl-kt-scale" min="0.02" max="0.1" step="0.001" value="0.052"/><div class="slider-value" id="ro-kt-scale"/></div>
                <div class="slider-row"><div class="slider-label">Наклон клинка</div><input type="range" id="sl-kt-tilt" min="0.2" max="1.55" step="0.01" value="1"/><div class="slider-value" id="ro-kt-tilt"/></div>
                <div class="slider-row"><div class="slider-label">Смещение в сторону</div><input type="range" id="sl-kt-side" min="0" max="0.8" step="0.01" value="0.32"/><div class="slider-value" id="ro-kt-side"/></div>
                <div class="slider-row"><div class="slider-label">Высота в руке</div><input type="range" id="sl-kt-height" min="0.6" max="1.8" step="0.01" value="1.3"/><div class="slider-value" id="ro-kt-height"/></div>
                <div class="slider-row"><div class="slider-label">Смещение вперёд</div><input type="range" id="sl-kt-forward" min="-0.5" max="1" step="0.01" value="0.1"/><div class="slider-value" id="ro-kt-forward"/></div>
                <div class="slider-row"><div class="slider-label">Непрозрачность</div><input type="range" id="sl-kt-opacity" min="30" max="100" step="1" value="100"/><div class="slider-value" id="ro-kt-opacity"/></div>
                <div class="slider-row"><div class="slider-label">Сторона (1-е лицо)</div><input type="range" id="sl-kt-fpside" min="0" max="0.8" step="0.01" value="0.34"/><div class="slider-value" id="ro-kt-fpside"/></div>
                <div class="slider-row"><div class="slider-label">Вперёд (1-е лицо)</div><input type="range" id="sl-kt-fpforward" min="-0.3" max="1" step="0.01" value="0.3"/><div class="slider-value" id="ro-kt-fpforward"/></div>
                <div id="tt-status">Katana: ожидание</div>
                <div id="tt-diag"> </div>
            </div>
        </div>
        <div class="foot">v1.4</div>
    </div>
</div>
</body>
</rml>
)RML";
