// Design of the embedded menu: RmlUi documents (HTML + RCSS), launcher-style.
// Kept as raw strings so the DLL stays a single file with no disk access.
// Colors follow the web launcher: dark background, bright green accent.
#pragma once

static const char kMenuRcss[] = R"RCSS(
body {
    font-family: GAY;
    font-size: 20px;
    color: #e6f2ea;
    background-color: rgba(4, 8, 6, 0.42);
    display: flex;
    align-items: center;
    justify-content: center;
}
#shell {
    width: 1140px;
    height: 716px;
    display: flex;
    flex-direction: column;
    background-color: rgba(10, 16, 13, 0.97);
    border: 1px rgba(58, 232, 130, 0.26);
    border-radius: 18px;
}
#titlebar {
    height: 66px;
    display: flex;
    flex-direction: row;
    align-items: center;
    padding: 0 22px;
    border-bottom: 1px rgba(255, 255, 255, 0.05);
}
#brand {
    font-size: 27px;
    font-weight: bold;
    letter-spacing: 1px;
    color: #f2fbf5;
}
#brand-accent {
    font-size: 27px;
    font-weight: bold;
    letter-spacing: 1px;
    color: #3ae882;
}
#brand-sub {
    margin-left: 14px;
    margin-top: 6px;
    font-size: 13px;
    letter-spacing: 2px;
    color: #7e9889;
}
#title-flex { flex-grow: 1; }
.iconbtn {
    width: 36px;
    height: 36px;
    font-size: 20px;
    color: #7e9889;
    background-color: rgba(255, 255, 255, 0.03);
    border: 1px rgba(255, 255, 255, 0.07);
    border-radius: 10px;
    text-align: center;
    vertical-align: middle;
    line-height: 36px;
    cursor: pointer;
    transition: background-color 0.12s, color 0.12s;
}
.iconbtn:hover {
    background-color: rgba(255, 90, 90, 0.14);
    color: #ff9c9c;
    border-color: rgba(255, 90, 90, 0.35);
}
#layout {
    flex-grow: 1;
    display: flex;
    flex-direction: row;
}
#nav {
    width: 216px;
    display: flex;
    flex-direction: column;
    padding: 16px 12px;
    border-right: 1px rgba(255, 255, 255, 0.05);
}
.nav-label {
    font-size: 11px;
    letter-spacing: 2px;
    color: #69816f;
    padding: 6px 12px;
}
.nav-item {
    position: relative;
    height: 46px;
    display: flex;
    flex-direction: row;
    align-items: center;
    padding: 0 14px;
    margin-bottom: 4px;
    border-radius: 11px;
    font-size: 19px;
    color: #8fa697;
    cursor: pointer;
    transition: background-color 0.13s, color 0.13s;
}
.nav-item:hover {
    background-color: rgba(255, 255, 255, 0.045);
    color: #d7e8dd;
}
.nav-item.active {
    background-color: rgba(58, 232, 130, 0.10);
    color: #eafff1;
}
.nav-bar {
    position: absolute;
    left: 0px;
    top: 11px;
    width: 3px;
    height: 24px;
    border-radius: 2px;
    background-color: #3ae882;
    opacity: 0;
    transition: opacity 0.15s;
}
.nav-item.active .nav-bar { opacity: 1; }
.nav-dot {
    width: 8px;
    height: 8px;
    margin-right: 12px;
    border-radius: 4px;
    background-color: #3d4a43;
    transition: background-color 0.15s;
}
.nav-item.active .nav-dot { background-color: #3ae882; }
#nav-flex { flex-grow: 1; }
.greenbtn {
    height: 48px;
    font-size: 18px;
    font-weight: bold;
    color: #06240f;
    background-color: #3ae882;
    border: 1px rgba(255, 255, 255, 0.22);
    border-radius: 12px;
    text-align: center;
    cursor: pointer;
    transition: background-color 0.13s;
}
.greenbtn:hover { background-color: #63f59d; }
.greenbtn:active { background-color: #2cc06c; }
#content {
    flex-grow: 1;
    display: flex;
    flex-direction: column;
    padding: 20px 22px;
    overflow-y: auto;
}
#content scrollbarvertical { width: 10px; }
#content scrollbarvertical slidertrack {
    background-color: rgba(255, 255, 255, 0.04);
    border-radius: 5px;
}
#content scrollbarvertical sliderbar {
    width: 8px;
    border-radius: 4px;
    background-color: rgba(58, 232, 130, 0.45);
}
.panel { display: none; }
.panel.selected { display: block; }
.card {
    padding: 8px 18px 14px 18px;
    margin-bottom: 14px;
    background-color: rgba(255, 255, 255, 0.028);
    border: 1px rgba(255, 255, 255, 0.06);
    border-radius: 14px;
}
.card-title {
    font-size: 22px;
    font-weight: bold;
    color: #f0f8f3;
    padding: 12px 0 2px 0;
}
.card-sub {
    font-size: 14px;
    color: #7e9889;
    padding-bottom: 8px;
}
.section-head {
    height: 46px;
    display: flex;
    flex-direction: row;
    align-items: center;
    cursor: pointer;
    border-radius: 9px;
    padding: 0 6px;
    transition: background-color 0.12s;
}
.section-head:hover { background-color: rgba(255, 255, 255, 0.035); }
.section-title {
    font-size: 15px;
    font-weight: bold;
    letter-spacing: 1.5px;
    color: #9fb8a9;
}
.caret {
    font-size: 13px;
    color: #69816f;
    margin-left: 8px;
    transition: transform 0.15s;
}
.section.collapsed .caret { transform: rotate(-90deg); }
.section-body { padding: 2px 0 6px 0; }
.section.collapsed .section-body { display: none; }
.module-row {
    height: 60px;
    display: flex;
    flex-direction: row;
    align-items: center;
    padding: 0 8px;
    border-radius: 11px;
    transition: background-color 0.12s;
}
.module-row:hover { background-color: rgba(255, 255, 255, 0.03); }
.m-chip {
    width: 40px;
    height: 40px;
    border-radius: 11px;
    font-size: 20px;
    font-weight: bold;
    color: #3ae882;
    background-color: rgba(58, 232, 130, 0.10);
    text-align: center;
    line-height: 40px;
}
.m-text { flex-grow: 1; padding-left: 14px; }
.m-name { font-size: 20px; color: #e9f4ed; }
.m-desc { font-size: 14px; color: #7e9889; margin-top: 1px; }
.switch {
    position: relative;
    width: 58px;
    height: 32px;
    border: 1px rgba(255, 255, 255, 0.08);
    border-radius: 16px;
    background-color: #20281f;
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
    background-color: rgba(58, 232, 130, 0.24);
    border-color: rgba(58, 232, 130, 0.55);
}
.switch.on .knob {
    left: 29px;
    background-color: #3ae882;
}
.slider-row {
    height: 44px;
    display: flex;
    flex-direction: row;
    align-items: center;
    padding: 0 8px;
    border-radius: 9px;
    transition: background-color 0.12s;
}
.slider-row:hover { background-color: rgba(255, 255, 255, 0.025); }
.slider-label { width: 300px; font-size: 18px; color: #cfe3d6; }
.slider-label small { font-size: 13px; color: #7e9889; }
input[type="range"] {
    width: 260px;
    height: 26px;
}
input[type="range"] slidertrack {
    height: 6px;
    margin-top: 10px;
    background-color: #1d221f;
    border-radius: 3px;
}
input[type="range"] sliderbar {
    width: 18px;
    height: 18px;
    top: -6px;
    border-radius: 9px;
    background-color: #3ae882;
    transition: background-color 0.1s;
}
input[type="range"] sliderbar:hover { background-color: #7bf5a8; }
input[type="range"] sliderarrowdec, input[type="range"] sliderarrowinc {
    width: 0px;
    height: 0px;
}
.slider-value {
    width: 86px;
    text-align: right;
    font-size: 17px;
    color: #9fb8a9;
}
.color-chip {
    width: 26px;
    height: 26px;
    margin-left: 10px;
    border-radius: 7px;
    border: 1px rgba(255, 255, 255, 0.18);
}
.note {
    font-size: 15px;
    color: #7e9889;
    padding: 6px 4px 10px 4px;
}
.tag {
    display: inline-block;
    font-size: 12px;
    letter-spacing: 1px;
    color: #3ae882;
    background-color: rgba(58, 232, 130, 0.10);
    border: 1px rgba(58, 232, 130, 0.25);
    border-radius: 7px;
    padding: 2px 8px;
    margin-left: 10px;
}
.info-row {
    display: flex;
    flex-direction: row;
    align-items: center;
    height: 34px;
    font-size: 18px;
}
.info-key { width: 320px; color: #8fa697; }
.info-val { color: #e9f4ed; }
.danger { color: #ff8a8a; }
.ok { color: #3ae882; }
)RCSS";

static const char kMenuRml[] = R"RML(
<rml>
<head>
    <title>GAYClient</title>
    <style>
    </style>
</head>
<body>
<div id="shell">
    <div id="titlebar">
        <div id="brand">GAY<span id="brand-accent">CLIENT</span></div>
        <div id="brand-sub">GAME MENU · BEDROCK 1.1.5</div>
        <div id="title-flex"/>
        <div id="brand-sub">v1.2</div>
        <button class="iconbtn" id="btn-close">&#215;</button>
    </div>
    <div id="layout">
        <div id="nav">
            <div class="nav-label">НАВИГАЦИЯ</div>
            <div class="nav-item active" id="tab-modules"><span class="nav-bar"/><span class="nav-dot"/>Модули</div>
            <div class="nav-item" id="tab-graphics"><span class="nav-bar"/><span class="nav-dot"/>Графика</div>
            <div class="nav-item" id="tab-hud"><span class="nav-bar"/><span class="nav-dot"/>HUD</div>
            <div class="nav-item" id="tab-info"><span class="nav-bar"/><span class="nav-dot"/>Инфо</div>
            <div id="nav-flex"/>
            <button class="greenbtn" id="btn-hudeditor">Настроить HUD</button>
        </div>
        <div id="content">

            <div class="panel selected" id="panel-modules">
                <div class="card">
                    <div class="card-title">Игровые модули<span class="tag">9 модулей</span></div>
                    <div class="card-sub">Стандартные HUD-модули клиента</div>
                    <div class="section" id="sec-hud">
                        <div class="section-head" id="head-sec-hud"><span class="section-title">HUD</span><span class="caret">&#9662;</span></div>
                        <div class="section-body">
                            <div class="module-row"><div class="m-chip">F</div><div class="m-text"><div class="m-name">FPS</div><div class="m-desc">Счётчик кадров в секунду</div></div><button class="switch" id="sw-fps"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">C</div><div class="m-text"><div class="m-name">CPS</div><div class="m-desc">Клики в секунду</div></div><button class="switch" id="sw-cps"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">&#9201;</div><div class="m-text"><div class="m-name">Часы</div><div class="m-desc">Текущее время</div></div><button class="switch" id="sw-clock"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">K</div><div class="m-text"><div class="m-name">Клавиши</div><div class="m-desc">Отображение нажатых клавиш</div></div><button class="switch" id="sw-keys"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">G</div><div class="m-text"><div class="m-name">Водяной знак</div><div class="m-desc">Надпись GAYClient на экране</div></div><button class="switch" id="sw-watermark"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">XY</div><div class="m-text"><div class="m-name">Координаты</div><div class="m-desc">Позиция в мире</div></div><button class="switch" id="sw-coords"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">R</div><div class="m-text"><div class="m-name">Разрешение</div><div class="m-desc">Размер окна игры</div></div><button class="switch" id="sw-res"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">%</div><div class="m-text"><div class="m-name">Производительность</div><div class="m-desc">Загрузка CPU и RAM</div></div><button class="switch" id="sw-perf"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">&#9776;</div><div class="m-text"><div class="m-name">Список модулей</div><div class="m-desc">Активные модули на экране</div></div><button class="switch" id="sw-modlist"><span class="knob"/></button></div>
                        </div>
                    </div>
                </div>
                <div class="card">
                    <div class="card-title">Эффекты<span class="tag">испытания</span></div>
                    <div class="card-sub">Визуальные эффекты персонажа</div>
                    <div class="section" id="sec-effects">
                        <div class="section-head" id="head-sec-effects"><span class="section-title">ЭФФЕКТЫ</span><span class="caret">&#9662;</span></div>
                        <div class="section-body">
                            <div class="module-row"><div class="m-chip">&#9660;</div><div class="m-text"><div class="m-name">China Hat</div><div class="m-desc">Конус над головой</div></div><button class="switch" id="sw-chinahat"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">&#8776;</div><div class="m-text"><div class="m-name">След</div><div class="m-desc">Цветной след за игроком</div></div><button class="switch" id="sw-trail"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">O</div><div class="m-text"><div class="m-name">Кольца прыжка</div><div class="m-desc">Круги при прыжке</div></div><button class="switch" id="sw-jump"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">&#9876;</div><div class="m-text"><div class="m-name">Катана</div><div class="m-desc">Замена алмазного меча</div></div><button class="switch" id="sw-katana"><span class="knob"/></button></div>
                            <div class="module-row"><div class="m-chip">&#9733;</div><div class="m-text"><div class="m-name">Корона</div><div class="m-desc">3D-корона над головой</div></div><button class="switch" id="sw-crown"><span class="knob"/></button></div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="panel" id="panel-graphics">
                <div class="card">
                    <div class="card-title">Графика<span class="tag">живые настройки</span></div>
                    <div class="card-sub">Параметры эффектов применяются сразу</div>

                    <div class="section" id="sec-g-ch">
                        <div class="section-head" id="head-sec-g-ch"><span class="section-title">CHINA HAT</span><span class="caret">&#9662;</span></div>
                        <div class="section-body">
                            <div class="slider-row"><div class="slider-label">Включение эффекта</div><button class="switch" id="sw-gfx-chinahat"><span class="knob"/></button></div>
                            <div class="slider-row"><div class="slider-label">Заливка конуса</div><button class="switch" id="sw-ch-fill"><span class="knob"/></button></div>
                            <div class="slider-row"><div class="slider-label">Радужный режим</div><button class="switch" id="sw-ch-rainbow"><span class="knob"/></button></div>
                            <div class="slider-row"><div class="slider-label">Скорость радуги</div><input type="range" id="sl-ch-rainbow" min="0" max="100" step="1" value="30"/><div class="slider-value" id="ro-ch-rainbow"/><div class="color-chip" id="chip-ch"/></div>
                            <div class="slider-row"><div class="slider-label">Прозрачность</div><input type="range" id="sl-ch-opacity" min="0" max="100" step="1" value="70"/><div class="slider-value" id="ro-ch-opacity"/></div>
                            <div class="slider-row"><div class="slider-label">Радиус</div><input type="range" id="sl-ch-radius" min="0.2" max="3" step="0.01" value="0.85"/><div class="slider-value" id="ro-ch-radius"/></div>
                            <div class="slider-row"><div class="slider-label">Высота конуса</div><input type="range" id="sl-ch-height" min="0.2" max="3" step="0.01" value="0.6"/><div class="slider-value" id="ro-ch-height"/></div>
                            <div class="slider-row"><div class="slider-label">Смещение над головой</div><input type="range" id="sl-ch-offset" min="0" max="2" step="0.01" value="0.1"/><div class="slider-value" id="ro-ch-offset"/></div>
                            <div class="slider-row"><div class="slider-label">Амплитуда плавания</div><input type="range" id="sl-ch-amp" min="0" max="1" step="0.01" value="0.05"/><div class="slider-value" id="ro-ch-amp"/></div>
                            <div class="slider-row"><div class="slider-label">Скорость плавания</div><input type="range" id="sl-ch-floatspeed" min="0" max="3" step="0.01" value="1"/><div class="slider-value" id="ro-ch-floatspeed"/></div>
                            <div class="slider-row"><div class="slider-label">Цвет: красный</div><input type="range" id="sl-ch-r" min="0" max="1" step="0.01" value="0.16"/><div class="slider-value" id="ro-ch-r"/></div>
                            <div class="slider-row"><div class="slider-label">Цвет: зелёный</div><input type="range" id="sl-ch-g" min="0" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-ch-g"/></div>
                            <div class="slider-row"><div class="slider-label">Цвет: синий</div><input type="range" id="sl-ch-b" min="0" max="1" step="0.01" value="0.21"/><div class="slider-value" id="ro-ch-b"/></div>
                        </div>
                    </div>

                    <div class="section" id="sec-g-tr">
                        <div class="section-head" id="head-sec-g-tr"><span class="section-title">СЛЕД</span><span class="caret">&#9662;</span></div>
                        <div class="section-body">
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

                    <div class="section" id="sec-g-jr">
                        <div class="section-head" id="head-sec-g-jr"><span class="section-title">КОЛЬЦА ПРЫЖКА</span><span class="caret">&#9662;</span></div>
                        <div class="section-body">
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

                    <div class="section" id="sec-g-kt">
                        <div class="section-head" id="head-sec-g-kt"><span class="section-title">КАТАНА</span><span class="caret">&#9662;</span></div>
                        <div class="section-body">
                            <div class="slider-row"><div class="slider-label">Размер</div><input type="range" id="sl-kt-scale" min="0.4" max="1" step="0.01" value="0.7"/><div class="slider-value" id="ro-kt-scale"/></div>
                            <div class="slider-row"><div class="slider-label">Наклон клинка</div><input type="range" id="sl-kt-tilt" min="0.2" max="1.55" step="0.01" value="0.9"/><div class="slider-value" id="ro-kt-tilt"/></div>
                            <div class="slider-row"><div class="slider-label">Смещение в сторону</div><input type="range" id="sl-kt-side" min="0" max="0.8" step="0.01" value="0.25"/><div class="slider-value" id="ro-kt-side"/></div>
                            <div class="slider-row"><div class="slider-label">Высота в руке</div><input type="range" id="sl-kt-height" min="0.6" max="1.8" step="0.01" value="1.1"/><div class="slider-value" id="ro-kt-height"/></div>
                            <div class="slider-row"><div class="slider-label">Смещение вперёд</div><input type="range" id="sl-kt-forward" min="-0.5" max="1" step="0.01" value="0.2"/><div class="slider-value" id="ro-kt-forward"/></div>
                            <div class="slider-row"><div class="slider-label">Непрозрачность</div><input type="range" id="sl-kt-opacity" min="0.3" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-kt-opacity"/></div>
                            <div class="slider-row"><div class="slider-label">Сторона (1-е лицо)</div><input type="range" id="sl-kt-fpside" min="0" max="0.8" step="0.01" value="0.3"/><div class="slider-value" id="ro-kt-fpside"/></div>
                            <div class="slider-row"><div class="slider-label">Вперёд (1-е лицо)</div><input type="range" id="sl-kt-fpforward" min="-0.3" max="1" step="0.01" value="0.25"/><div class="slider-value" id="ro-kt-fpforward"/></div>
                        </div>
                    </div>

                    <div class="section" id="sec-g-cr">
                        <div class="section-head" id="head-sec-g-cr"><span class="section-title">КОРОНА</span><span class="caret">&#9662;</span></div>
                        <div class="section-body">
                            <div class="slider-row"><div class="slider-label">Размер</div><input type="range" id="sl-cr-scale" min="0" max="2" step="0.01" value="1"/><div class="slider-value" id="ro-cr-scale"/></div>
                            <div class="slider-row"><div class="slider-label">Смещение</div><input type="range" id="sl-cr-offset" min="0" max="2" step="0.01" value="0.5"/><div class="slider-value" id="ro-cr-offset"/></div>
                            <div class="slider-row"><div class="slider-label">Непрозрачность</div><input type="range" id="sl-cr-opacity" min="0" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-cr-opacity"/></div>
                            <div class="slider-row"><div class="slider-label">Цвет: красный</div><input type="range" id="sl-cr-r" min="0" max="1" step="0.01" value="1"/><div class="slider-value" id="ro-cr-r"/></div>
                            <div class="slider-row"><div class="slider-label">Цвет: зелёный</div><input type="range" id="sl-cr-g" min="0" max="1" step="0.01" value="0.8"/><div class="slider-value" id="ro-cr-g"/></div>
                            <div class="slider-row"><div class="slider-label">Цвет: синий</div><input type="range" id="sl-cr-b" min="0" max="1" step="0.01" value="0.2"/><div class="slider-value" id="ro-cr-b"/></div>
                        </div>
                    </div>
                </div>
            </div>

            <div class="panel" id="panel-hud">
                <div class="card">
                    <div class="card-title">HUD<span class="tag">быстрые переключатели</span></div>
                    <div class="card-sub">Элементы на экране во время игры</div>
                    <div class="module-row"><div class="m-chip">F</div><div class="m-text"><div class="m-name">FPS</div></div><button class="switch" id="hud-sw-fps"><span class="knob"/></button></div>
                    <div class="module-row"><div class="m-chip">C</div><div class="m-text"><div class="m-name">CPS</div></div><button class="switch" id="hud-sw-cps"><span class="knob"/></button></div>
                    <div class="module-row"><div class="m-chip">&#9201;</div><div class="m-text"><div class="m-name">Часы</div></div><button class="switch" id="hud-sw-clock"><span class="knob"/></button></div>
                    <div class="module-row"><div class="m-chip">K</div><div class="m-text"><div class="m-name">Клавиши</div></div><button class="switch" id="hud-sw-keys"><span class="knob"/></button></div>
                    <div class="module-row"><div class="m-chip">G</div><div class="m-text"><div class="m-name">Водяной знак</div></div><button class="switch" id="hud-sw-watermark"><span class="knob"/></button></div>
                    <div class="module-row"><div class="m-chip">XY</div><div class="m-text"><div class="m-name">Координаты</div></div><button class="switch" id="hud-sw-coords"><span class="knob"/></button></div>
                    <div class="module-row"><div class="m-chip">%</div><div class="m-text"><div class="m-name">Производительность</div></div><button class="switch" id="hud-sw-perf"><span class="knob"/></button></div>
                    <div class="module-row"><div class="m-chip">&#9776;</div><div class="m-text"><div class="m-name">Список модулей</div></div><button class="switch" id="hud-sw-modlist"><span class="knob"/></button></div>
                </div>
                <div class="card">
                    <div class="card-title">Редактор</div>
                    <div class="card-sub">Перетаскивание элементов HUD мышью</div>
                    <button class="greenbtn" id="btn-hudeditor2">Настроить HUD</button>
                    <div class="note">Все панели скроются — останется только HUD. Перетаскивай элементы, чтобы изменить их позицию. Esc или кнопка меню вернёт обычный режим.</div>
                </div>
            </div>

            <div class="panel" id="panel-info">
                <div class="card">
                    <div class="card-title">О клиенте</div>
                    <div class="info-row"><div class="info-key">Клиент</div><div class="info-val">GAYClient v1.2</div></div>
                    <div class="info-row"><div class="info-key">Игра</div><div class="info-val">Minecraft Bedrock 1.1.5 (Windows)</div></div>
                    <div class="info-row"><div class="info-key">Интерфейс</div><div class="info-val ok">RmlUi · HTML/CSS</div></div>
                    <div class="info-row"><div class="info-key">Сборка</div><div class="info-val" id="info-build">—</div></div>
                </div>
                <div class="card">
                    <div class="card-title">Статус</div>
                    <div class="info-row"><div class="info-key">Соединение с ядром</div><div class="info-val ok" id="info-conn">подключено</div></div>
                    <div class="info-row"><div class="info-key">Профили</div><div class="info-val danger">пока не подключены</div></div>
                    <div class="info-row"><div class="info-key">Звук</div><div class="info-val danger">пока не подключён</div></div>
                    <div class="info-row"><div class="info-key">Статистика</div><div class="info-val danger">пока не подключена</div></div>
                </div>
                <div class="card">
                    <div class="card-title">Управление</div>
                    <div class="info-row"><div class="info-key">Открыть меню</div><div class="info-val">клавиша меню игры</div></div>
                    <div class="info-row"><div class="info-key">Выключить оверлей</div><div class="info-val danger">Delete (полное отключение)</div></div>
                </div>
            </div>

        </div>
    </div>
</div>
</body>
</rml>
)RML";

static const char kTestToolsRcss[] = R"RCSS(
body {
    font-family: GAY;
    font-size: 17px;
    color: #e6f2ea;
}
#tt {
    position: absolute;
    left: 18px;
    top: 240px;
    width: 360px;
    background-color: rgba(10, 16, 13, 0.96);
    border: 1px rgba(58, 232, 130, 0.30);
    border-radius: 14px;
    drag: drag;
}
#tt-head {
    height: 40px;
    display: flex;
    flex-direction: row;
    align-items: center;
    padding: 0 14px;
    border-bottom: 1px rgba(255, 255, 255, 0.06);
    font-size: 15px;
    font-weight: bold;
    letter-spacing: 1px;
    color: #9fb8a9;
    cursor: move;
}
#tt-body { padding: 10px 14px 14px 14px; }
#tt-status { font-size: 17px; color: #3ae882; padding-bottom: 4px; }
#tt-diag { font-size: 13px; color: #7e9889; padding-bottom: 8px; }
.tt-row {
    height: 38px;
    display: flex;
    flex-direction: row;
    align-items: center;
}
.tt-label { width: 150px; font-size: 16px; color: #cfe3d6; }
input[type="range"] { width: 130px; height: 24px; }
input[type="range"] slidertrack {
    height: 5px;
    margin-top: 9px;
    background-color: #1d221f;
    border-radius: 3px;
}
input[type="range"] sliderbar {
    width: 15px;
    height: 15px;
    top: -5px;
    border-radius: 8px;
    background-color: #3ae882;
}
input[type="range"] sliderarrowdec, input[type="range"] sliderarrowinc {
    width: 0px;
    height: 0px;
}
.tt-val { width: 62px; text-align: right; font-size: 15px; color: #9fb8a9; }
.tt-head-label { font-size: 13px; letter-spacing: 1.5px; color: #69816f; padding-top: 6px; }
)RCSS";

static const char kTestToolsRml[] = R"RML(
<rml>
<head>
    <title>Test tools</title>
    <style>
    </style>
</head>
<body>
<div id="tt">
    <div id="tt-head">TEST TOOLS (ADAPTER)</div>
    <div id="tt-body">
        <div id="tt-status">Katana: ожидание</div>
        <div id="tt-diag">—</div>
        <div class="tt-head-label">КАТАНА</div>
        <div class="tt-row"><div class="tt-label">Размер</div><input type="range" id="sl-kt2-scale" min="0.4" max="1" step="0.01" value="0.7"/><div class="tt-val" id="ro-kt2-scale"/></div>
        <div class="tt-row"><div class="tt-label">Наклон</div><input type="range" id="sl-kt2-tilt" min="0.2" max="1.55" step="0.01" value="0.9"/><div class="tt-val" id="ro-kt2-tilt"/></div>
        <div class="tt-row"><div class="tt-label">Сторона</div><input type="range" id="sl-kt2-side" min="0" max="0.8" step="0.01" value="0.25"/><div class="tt-val" id="ro-kt2-side"/></div>
        <div class="tt-row"><div class="tt-label">Высота</div><input type="range" id="sl-kt2-height" min="0.6" max="1.8" step="0.01" value="1.1"/><div class="tt-val" id="ro-kt2-height"/></div>
        <div class="tt-row"><div class="tt-label">Вперёд</div><input type="range" id="sl-kt2-forward" min="-0.5" max="1" step="0.01" value="0.2"/><div class="tt-val" id="ro-kt2-forward"/></div>
        <div class="tt-row"><div class="tt-label">Непрозр.</div><input type="range" id="sl-kt2-opacity" min="0.3" max="1" step="0.01" value="1"/><div class="tt-val" id="ro-kt2-opacity"/></div>
        <div class="tt-head-label">КОРОНА</div>
        <div class="tt-row"><div class="tt-label">Размер</div><input type="range" id="sl-cr2-scale" min="0" max="2" step="0.01" value="1"/><div class="tt-val" id="ro-cr2-scale"/></div>
        <div class="tt-row"><div class="tt-label">Смещение</div><input type="range" id="sl-cr2-offset" min="0" max="2" step="0.01" value="0.5"/><div class="tt-val" id="ro-cr2-offset"/></div>
        <div class="tt-row"><div class="tt-label">Непрозр.</div><input type="range" id="sl-cr2-opacity" min="0" max="1" step="0.01" value="1"/><div class="tt-val" id="ro-cr2-opacity"/></div>
        <div class="tt-row"><div class="tt-label">Красный</div><input type="range" id="sl-cr2-r" min="0" max="1" step="0.01" value="1"/><div class="tt-val" id="ro-cr2-r"/></div>
        <div class="tt-row"><div class="tt-label">Зелёный</div><input type="range" id="sl-cr2-g" min="0" max="1" step="0.01" value="0.8"/><div class="tt-val" id="ro-cr2-g"/></div>
        <div class="tt-row"><div class="tt-label">Синий</div><input type="range" id="sl-cr2-b" min="0" max="1" step="0.01" value="0.2"/><div class="tt-val" id="ro-cr2-b"/></div>
    </div>
</div>
</body>
</rml>
)RML";
