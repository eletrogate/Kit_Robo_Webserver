var connection = new WebSocket(`ws://${window.location.hostname}/ws`);

function send(velocidade, angulo_graus) {
    vel_pI = parseInt(velocidade);
    ang_pI = parseInt(angulo_graus);
    connection.send(vel_pI.toString().concat(" ", ang_pI.toString()));
}

(connection.onopen = function () { connection.send("Connect " + new Date()); }),
(connection.onerror = function (n) { console.log("WebSocket Error ", n), alert("WebSocket Error ", n); }),
(connection.onmessage = function (n) { console.log("Server: ", n.data); });

var canvas, ctx, width, height, radius, x_orig, y_orig, largura, altura;
function resize() {

    largura = window.innerWidth
        || document.documentElement.clientWidth
        || document.body.clientWidth;
    altura = window.innerHeight
        || document.documentElement.clientHeight
        || document.body.clientHeight;

    if (largura > altura) {
        width = largura * 0.3646;
        radius = width * 0.2;
        height = altura * 0.65;
    }

    if (altura > largura) {
        width = largura;
        radius = width * 0.23
        height = largura;
    }

    ctx.canvas.width = width;
    ctx.canvas.height = height;
    background();
    joystick(width / 2, height / 2);
}

function background() {
    x_orig = width / 2;
    y_orig = height / 2;
    ctx.beginPath();
    ctx.arc(x_orig, y_orig, radius + 20, 0, Math.PI * 2, true);
    ctx.fillStyle = '#0e37cd';
    ctx.fill();
}

function joystick(width, height) {
    ctx.beginPath();
    ctx.arc(width, height, radius, 0, Math.PI * 2, true);
    ctx.fillStyle = '#ffffff';
    ctx.fill();
    ctx.strokeStyle = '#4464d9';
    ctx.lineWidth = 8;
    ctx.stroke();
}

window.addEventListener("load", () => {
    (canvas = document.getElementById("canvas")),
    (ctx = canvas.getContext("2d")),
    resize(),
    document.addEventListener("mousedown", startDrawing),
    document.addEventListener("mouseup", stopDrawing),
    document.addEventListener("mousemove", Draw),
    document.addEventListener("touchstart", startDrawing),
    document.addEventListener("touchend", stopDrawing),
    document.addEventListener("touchcancel", stopDrawing),
    document.addEventListener("touchmove", Draw),
    window.addEventListener("resize", resize);
});

let coord = { x: 0, y: 0 };
let paint = !1;

function getPosition(event) {
    e = window.event || e;
    var mouse_x = e.clientX || e.touches[0].clientX;
    var mouse_y = e.clientY || e.touches[0].clientY;
    coord.x = mouse_x - canvas.offsetLeft;
    coord.y = mouse_y - canvas.offsetTop;
}

function is_it_in_the_circle() {
    var current_radius = Math.sqrt(Math.pow(coord.x - x_orig, 2) + Math.pow(coord.y - y_orig, 2));
    if (radius >= current_radius) return true
    else return false
}

function startDrawing(event) {
    paint = true;
    getPosition(event);
    if (is_it_in_the_circle()) {
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        background();
        joystick(coord.x, coord.y);
        Draw();
    }
}

function stopDrawing() {
    paint = false;
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    background();
    joystick(width / 2, height / 2);
    for(let i = 0; i < 15; i ++)
      send(0, 0)
}

function Draw(t) {
    if (paint) {
        var velocidade, angulo_graus, angulo_rad, x_abs, y_abs;
        ctx.clearRect(0, 0, canvas.width, canvas.height), background();
        angulo_rad = Math.atan2(coord.y - y_orig, coord.x - x_orig);

        (angulo_graus = -1 == Math.sign(angulo_rad) ? Math.round((180 * -angulo_rad) / Math.PI)
            : Math.round(360 - (180 * angulo_rad) / Math.PI)),
        is_it_in_the_circle() ? (joystick(coord.x, coord.y), (x_abs = coord.x), (y_abs = coord.y))
            : joystick((x_abs = radius * Math.cos(angulo_rad) + x_orig), (y_abs = radius * Math.sin(angulo_rad) + y_orig)),
        getPosition(t);

        velocidade = Math.round((100 * Math.sqrt(Math.pow(x_abs - x_orig, 2) + Math.pow(y_abs - y_orig, 2))) / radius),
        send(velocidade, angulo_graus);
    }
}