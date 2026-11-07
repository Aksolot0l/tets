// Design of the embedded menu: RmlUi documents (HTML + RCSS).
// The menu is a set of independent windows ("okoshki") in the launcher
// design system: dark glass cards with emerald accents, dragged by their
// title bars, foldable, scrolling inside so a large number of functions
// never makes windows grow into each other.
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
    font-size: 15px;
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
.foldcaret {
    font-size: 13px;
    transition: transform 0.15s;
}
.win.folded .foldcaret { transform: rotate(-90deg); }
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
.m-chip {
    width: 34px;
    height: 34px;
    border-radius: 9px;
    font-size: 17px;
    font-weight: bold;
    color: #6ef58c;
    background-color: #6ef58c1a;
    text-align: center;
    line-height: 34px;
}
.m-text { display: block; flex-grow: 1; padding-left: 12px; }
.m-name { display: block; font-size: 18px; color: #e9f5ec; }
.m-desc { display: block; font-size: 13px; color: #89a792; margin-top: 1px; }
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
        <button class="foldbtn" id="fold-0"><span class="foldcaret">▾</span></button>
        <button class="iconbtn" id="btn-close">×</button>
    </div>
    <div class="win-body">
        <div class="brand">
            <span class="brand-name">GAY<span class="brand-accent">CLIENT</span></span>
            <span class="brand-sub">GAME MENU · BEDROCK 1.1.5</span>
        </div>
        <button class="greenbtn" id="btn-hudeditor">Настроить HUD</button>
        <div class="note">Окна тащатся за заголовок, колесо мыши прокручивает список, стрелка в углу сворачивает окно. <span id="btn-layout">Сбросить раскладку</span></div>
        <div class="module-row"><div class="m-chip">F</div><div class="m-text"><div class="m-name">FPS</div><div class="m-desc">Счётчик кадров в секунду</div></div><button class="switch" id="sw-fps"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">C</div><div class="m-text"><div class="m-name">CPS</div><div class="m-desc">Клики в секунду</div></div><button class="switch" id="sw-cps"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">⏱</div><div class="m-text"><div class="m-name">Часы</div><div class="m-desc">Текущее время</div></div><button class="switch" id="sw-clock"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">K</div><div class="m-text"><div class="m-name">Клавиши</div><div class="m-desc">Отображение нажатых клавиш</div></div><button class="switch" id="sw-keys"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">G</div><div class="m-text"><div class="m-name">Водяной знак</div><div class="m-desc">Надпись GAYClient на экране</div></div><button class="switch" id="sw-watermark"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">XY</div><div class="m-text"><div class="m-name">Координаты</div><div class="m-desc">Позиция в мире</div></div><button class="switch" id="sw-coords"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">R</div><div class="m-text"><div class="m-name">Разрешение</div><div class="m-desc">Размер окна игры</div></div><button class="switch" id="sw-res"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">%</div><div class="m-text"><div class="m-name">Производительность</div><div class="m-desc">Загрузка CPU и RAM</div></div><button class="switch" id="sw-perf"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">☰</div><div class="m-text"><div class="m-name">Список модулей</div><div class="m-desc">Активные модули на экране</div></div><button class="switch" id="sw-modlist"><span class="knob"/></button></div>
        <div class="foot">v1.2 · RmlUi</div>
    </div>
</div>
<div class="win" id="win-visuals">
    <div class="win-head" id="head-visuals">
        <span class="win-dot"/>
        <span class="win-title">VISUALS</span>
        <span class="win-sub">ЭФФЕКТЫ</span>
        <span class="win-flex"/>
        <button class="foldbtn" id="fold-1"><span class="foldcaret">▾</span></button>
    </div>
    <div class="win-body">
        <div class="module-row"><div class="m-chip">▼</div><div class="m-text"><div class="m-name">China Hat</div><div class="m-desc">Конус над головой</div></div><button class="switch" id="sw-chinahat"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">≈</div><div class="m-text"><div class="m-name">След</div><div class="m-desc">Цветной след за игроком</div></div><button class="switch" id="sw-trail"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">O</div><div class="m-text"><div class="m-name">Кольца прыжка</div><div class="m-desc">Круги при прыжке</div></div><button class="switch" id="sw-jump"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">⚔</div><div class="m-text"><div class="m-name">Катана</div><div class="m-desc">Замена алмазного меча</div></div><button class="switch" id="sw-katana"><span class="knob"/></button></div>
        <div class="module-row"><div class="m-chip">★</div><div class="m-text"><div class="m-name">Корона</div><div class="m-desc">3D-корона над головой</div></div><button class="switch" id="sw-crown"><span class="knob"/></button></div>
        <div class="note">Главные выключатели. Тонкая настройка каждого эффекта — в его собственном окне.</div>
    </div>
</div>
<div class="win" id="win-ch">
    <div class="win-head" id="head-ch">
        <span class="win-dot"/>
        <span class="win-title">CHINA HAT</span>
        <span class="win-flex"/>
        <button class="foldbtn" id="fold-2"><span class="foldcaret">▾</span></button>
    </div>
    <div class="win-body">
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
<div class="win" id="win-tr">
    <div class="win-head" id="head-tr">
        <span class="win-dot"/>
        <span class="win-title">СЛЕД</span>
        <span class="win-flex"/>
        <button class="foldbtn" id="fold-3"><span class="foldcaret">▾</span></button>
    </div>
    <div class="win-body">
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
<div class="win" id="win-jr">
    <div class="win-head" id="head-jr">
        <span class="win-dot"/>
        <span class="win-title">КОЛЬЦА</span>
        <span class="win-sub">ПРЫЖОК</span>
        <span class="win-flex"/>
        <button class="foldbtn" id="fold-4"><span class="foldcaret">▾</span></button>
    </div>
    <div class="win-body">
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
<div class="win" id="win-kt">
    <div class="win-head" id="head-kt">
        <span class="win-dot"/>
        <span class="win-title">КАТАНА</span>
        <span class="win-flex"/>
        <button class="foldbtn" id="fold-5"><span class="foldcaret">▾</span></button>
    </div>
    <div class="win-body">
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
<div class="win" id="win-cr">
    <div class="win-head" id="head-cr">
        <span class="win-dot"/>
        <span class="win-title">КОРОНА</span>
        <span class="win-flex"/>
        <button class="foldbtn" id="fold-6"><span class="foldcaret">▾</span></button>
    </div>
    <div class="win-body">
        <div class="slider-row"><div class="slider-label">Включить корону</div><button class="switch" id="sw-cr-master"><span class="knob"/></button></div>
        <div class="slider-row"><div class="slider-label">Размер</div><input type="range" id="sl-cr-scale" min="0" max="2" step="0.01" value="1"/><div class="slider-value" id="ro-cr-scale"/></div>
        <div class="slider-row"><div class="slider-label">Смещение</div><input type="range" id="sl-cr-offset" min="0" max="2" step="0.01" value="0.5"/><div class="slider-value" id="ro-cr-offset"/></div>
        <div class="slider-row"><div class="slider-label">Непрозрачность</div><input type="range" id="sl-cr-opacity" min="0" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-cr-opacity"/></div>
        <div class="slider-row"><div class="slider-label">Цвет: красный</div><input type="range" id="sl-cr-r" min="0" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-cr-r"/></div>
        <div class="slider-row"><div class="slider-label">Цвет: зелёный</div><input type="range" id="sl-cr-g" min="0" max="1" step="0.01" value="0.8"/><div class="slider-value" id="ro-cr-g"/></div>
        <div class="slider-row"><div class="slider-label">Цвет: синий</div><input type="range" id="sl-cr-b" min="0" max="1" step="0.01" value="0.2"/><div class="slider-value" id="ro-cr-b"/></div>
    </div>
</div>
</body>
</rml>
)RML";
