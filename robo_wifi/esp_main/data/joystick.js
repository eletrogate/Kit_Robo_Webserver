var connection = new WebSocket(`ws://${window.location.hostname}/ws`);
function send(n, o) {
    connection.send(n.toString().concat(" ", o.toString()));
}
(connection.onopen = function () {
    connection.send("Connect " + new Date());
}),
    (connection.onerror = function (n) {
        console.log("WebSocket Error ", n), alert("WebSocket Error ", n);
    }),
    (connection.onmessage = function (n) {
        console.log("Server: ", n.data);
    });

var canvas, ctx, width, height, radius, x_orig, y_orig, largura, altura;       // ************** TAMANHO E MARGEM DO JOYSTICK **************
function resize() {

    // mapeia as dimensões vertical e horizontal da janela do browser
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

// área útil celular (Redmi Note 8): 980x1793

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

function background() {         // ************** CORES DO JOYSTICK **************
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

}

function Draw(t) {
    if(paint)   {
        var e, n, i;
        ctx.clearRect(0, 0, canvas.width, canvas.height), background();
        var o = Math.atan2(coord.y - y_orig, coord.x - x_orig);
        (e = -1 == Math.sign(o) ? Math.round((180 * -o) / Math.PI) : Math.round(360 - (180 * o) / Math.PI)),
            is_it_in_the_circle() ? (joystick(coord.x, coord.y), (n = coord.x), (i = coord.y)) : joystick((n = radius * Math.cos(o) + x_orig), (i = radius * Math.sin(o) + y_orig)),
            getPosition(t);
        var c = Math.round((100 * Math.sqrt(Math.pow(n - x_orig, 2) + Math.pow(i - y_orig, 2))) / radius),
            a = Math.round(n - x_orig),
            r = Math.round(i - y_orig);
    }
    send(c, e);
}