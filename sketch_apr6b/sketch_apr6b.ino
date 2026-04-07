
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "esp_netif.h"

const byte DNS_PORT = 53;
DNSServer dnsServer;
bool staConnected = false; // Track if STA has internet

const char* ssid = "Traffic_Controller";
const char* password = "password123"; 

WebServer server(80);
Preferences preferences;

// Store saved sequence names in a comma-separated list
String savedSequenceNames = ""; 

// Function to handle saving a sequence
void saveSequence(String name, String cmd) {
    preferences.putString(("s_" + name).c_str(), cmd);
    
    // Add to names list if not already there
    if (savedSequenceNames.indexOf(name + ",") == -1) {
        savedSequenceNames += name + ",";
        preferences.putString("seqList", savedSequenceNames);
    }
}

// Function to delete a sequence
void deleteSequence(String name) {
    preferences.remove(("s_" + name).c_str());
    savedSequenceNames.replace(name + ",", "");
    preferences.putString("seqList", savedSequenceNames);
}

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Traffic Controller Pro</title>
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600;800&family=Share+Tech+Mono&display=swap');

        * { box-sizing: border-box; }

        body {
            margin: 0;
            padding: 0;
            background-color: #0f172a;
            color: #f8fafc;
            font-family: 'Inter', sans-serif;
            display: flex;
            height: 100vh;
            overflow: hidden;
        }

        /* Mobile Responsive Design */
        @media (max-width: 768px) {
            body {
                flex-direction: column;
                overflow-y: auto;
            }
            #sidebar {
                width: 100% !important;
                border-right: none !important;
                border-bottom: 2px solid #334155;
                flex-shrink: 0;
            }
            #simulation {
                width: 100% !important;
            }
        }

        /* LEFT PANEL */
        #sidebar {
            width: 380px;
            background: #1e293b;
            border-right: 1px solid #334155;
            padding: 24px;
            display: flex;
            flex-direction: column;
            gap: 20px;
            z-index: 10;
            box-shadow: 5px 0 15px rgba(0,0,0,0.5);
        }

        h1 {
            font-size: 24px;
            margin: 0;
            font-weight: 800;
            color: #38bdf8;
            text-shadow: 0 2px 10px rgba(56, 189, 248, 0.2);
        }

        .select-box {
            width: 100%;
            padding: 12px;
            background: #334155;
            color: white;
            border: 1px solid #475569;
            border-radius: 8px;
            font-family: 'Inter', sans-serif;
            font-size: 15px;
            outline: none;
            cursor: pointer;
        }

        .btn {
            background: #0ea5e9;
            color: white;
            border: none;
            padding: 14px;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: 0.2s;
            display: flex;
            justify-content: center;
            align-items: center;
        }
        .btn:hover { background: #0284c7; }
        .btn:disabled { background: #475569; color: #94a3b8; cursor: not-allowed; }
        .btn.connect { background: #10b981; font-size: 18px; }
        .btn.connect:hover { background: #059669; }

        .presets {
            display: flex;
            flex-direction: column;
            gap: 12px;
            margin-top: 5px;
        }
        
        .presets h3 {
            margin: 0;
            font-size: 13px;
            color: #94a3b8;
            text-transform: uppercase;
            letter-spacing: 1px;
        }

        .preset-btn {
            background: #334155;
            font-size: 14px;
            padding: 12px;
        }
        .preset-btn:hover { background: #475569; }

        .console-container {
            flex-grow: 1;
            display: flex;
            flex-direction: column;
            background: #000;
            border-radius: 8px;
            border: 1px solid #334155;
            overflow: hidden;
            margin-top: 10px;
        }

        #log {
            flex-grow: 1;
            padding: 15px;
            font-family: 'Share Tech Mono', monospace;
            font-size: 13px;
            color: #a3e635;
            overflow-y: auto;
            max-height: calc(100vh - 450px);
        }
        
        .user-msg { color: #38bdf8; font-weight: bold; }
        .esp-msg { color: #facc15; }

        .console-input {
            display: flex;
            border-top: 1px solid #334155;
            background: #0f172a;
        }

        .console-input input {
            flex-grow: 1;
            background: transparent;
            border: none;
            padding: 14px;
            color: #38bdf8;
            font-family: monospace;
            font-size: 15px;
            outline: none;
        }
        
        .console-input button {
            background: #38bdf8;
            border: none;
            color: #0f172a;
            padding: 0 15px;
            font-weight: bold;
            font-size: 18px;
            cursor: pointer;
            transition: 0.2s;
        }
        .console-input button:hover { background: #0ea5e9; }
        .console-input button:disabled { background: #334155; color: #64748b; cursor: not-allowed;}
        
        #saveBtn { background: #fbbf24; color: #0f172a; border-left: 1px solid #334155;}
        #saveBtn:hover { background: #f59e0b; }

        .saved-sequences {
            margin-top: 10px;
            display: flex;
            flex-direction: column;
            gap: 8px;
            max-height: 200px;
            overflow-y: auto;
            padding-right: 5px;
        }

        .saved-item {
            background: #334155;
            padding: 10px;
            border-radius: 6px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            font-size: 13px;
        }

        .saved-item span { cursor: pointer; flex-grow: 1; }
        .saved-item span:hover { color: #38bdf8; }
        .saved-item .del-btn { color: #ef4444; cursor: pointer; font-weight: bold; margin-left: 10px; padding: 0 5px;}
        .saved-item .del-btn:hover { color: #f87171; }

        .wifi-config {
            margin-top: auto;
            padding-top: 15px;
            border-top: 1px solid #334155;
        }

        #wifiModal {
            display: none;
            position: fixed;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background: #1e293b;
            padding: 30px;
            border-radius: 12px;
            box-shadow: 0 0 50px rgba(0,0,0,0.8);
            z-index: 1000;
            width: 350px;
            border: 1px solid #334155;
        }

        #wifiModal h2 { margin: 0 0 20px 0; font-size: 18px; color: #38bdf8; }
        #wifiModal input { width: 100%; border: 1px solid #334155; background: #0f172a; padding: 12px; color: white; border-radius: 6px; margin-bottom: 15px; }
        #modalOverlay { display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.7); z-index: 999; }

        #main-area {
            flex-grow: 1;
            position: relative;
            display: flex;
            justify-content: center;
            align-items: center;
            background: radial-gradient(circle, #334155 0%, #0f172a 100%);
        }

        canvas {
            border-radius: 12px;
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.8);
            background-color: #1e293b;
            max-height: 95vh;
            max-width: 95vw;
            object-fit: contain;
        }
    </style>
</head>
<body>

    <!-- SIDEBAR -->
    <div id="sidebar">
        <h1>Simulation</h1>
        <button id="connectBtn" class="btn connect">Connect ESP32</button>

        <select id="mapType" class="select-box">
            <option value="tmap" selected>Map: T-Junction</option>
            <option value="intersection">Map: 4-Way Intersection</option>
            <option value="twoway">Map: Two-Way Highway</option>
        </select>

        <div class="presets">
            <h3>Manual Controls</h3>
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 8px;">
                <button class="btn preset-btn" style="border-left: 5px solid #ef4444;" onclick="sendCmd('R')">Red</button>
                <button class="btn preset-btn" style="border-left: 5px solid #facc15;" onclick="sendCmd('Y')">Yellow</button>
                <button class="btn preset-btn" style="border-left: 5px solid #22c55e;" onclick="sendCmd('G')">Green</button>
                <button class="btn preset-btn" onclick="sendCmd('X')">OFF</button>
            </div>
        </div>

        <div class="presets">
            <h3>Saved Sequences</h3>
            <div id="savedList" class="saved-sequences">
                <!-- Loaded from ESP32 or LocalStorage -->
                <div style="color: #64748b; font-style: italic; font-size: 12px; padding: 10px;">No saved sequences yet.</div>
            </div>
        </div>

        <div class="console-container">
            <div id="log">Awaiting connection...<br></div>
            <div class="console-input">
                <input type="text" id="cmdInput" placeholder="Enter sequence (e.g. G10,Y5,R10 L)" disabled>
                <button id="sendBtn" disabled title="Send to ESP32">&#10148;</button>
                <button id="saveBtn" disabled title="Save to Database" onclick="saveCurrentSequence()">&#128190;</button>
            </div>
        </div>

        <div class="wifi-config">
            <button class="btn" style="width: 100%; background: #475569; font-size: 13px;" onclick="openWifiModal()">Wi-Fi Station Settings</button>
        </div>
    </div>

    <!-- MODAL -->
    <div id="modalOverlay"></div>
    <div id="wifiModal">
        <h2>Connect to Internet (STA)</h2>
        <input type="text" id="staSsid" placeholder="Home Wi-Fi Name">
        <input type="password" id="staPass" placeholder="Home Wi-Fi Password">
        <div style="display: flex; gap: 10px;">
            <button class="btn" style="flex: 1; background: #10b981;" onclick="saveWifi()">Save & Reboot</button>
            <button class="btn" style="flex: 1; background: #ef4444;" onclick="closeWifiModal()">Cancel</button>
        </div>
    </div>

    <!-- MAIN AREA -->
    <div id="main-area">
        <canvas id="simCanvas" width="800" height="800"></canvas>
    </div>

    <script>
        const connectBtn = document.getElementById('connectBtn');
        const cmdInput = document.getElementById('cmdInput');
        const sendBtn = document.getElementById('sendBtn');
        const logDiv = document.getElementById('log');

        const mapSelect = document.getElementById('mapType');

        let port;
        let reader;
        let writer;
        let currentMap = mapSelect.value;
        let blinkTimeout;
        let currentState = 'red'; 
        let countdownValue = 0;
        let countdownInterval;

        mapSelect.addEventListener('change', (e) => {
            currentMap = e.target.value;
            resetSimulation();
        });

        function startTimer(seconds) {
            clearInterval(countdownInterval);
            countdownValue = seconds;
            
            countdownInterval = setInterval(() => {
                if (countdownValue > 0) {
                    countdownValue--;
                } else {
                    clearInterval(countdownInterval);
                }
            }, 1000);
        }

        function log(message, type = "") {
            if (type === "esp") logDiv.innerHTML += "<span class='esp-msg'>&gt; " + message + "</span><br>";
            else if (type === "user") logDiv.innerHTML += "<span class='user-msg'>" + message + "</span><br>";
            else logDiv.innerHTML += message + "<br>";
            logDiv.scrollTop = logDiv.scrollHeight;
        }

        function updateLights(text) {
            let previousState = currentState;
            currentState = 'none';
            if (text.includes("Red")) currentState = 'red';
            if (text.includes("Yellow")) currentState = 'yellow';
            if (text.includes("Green")) currentState = 'green';
            if (currentState === 'none' && (text.includes("OFF") || text.includes("STOPPED"))) currentState = 'red';

            let timeMatch = text.match(/(?:T:|TIME:|S)\s*(\d+)/i);
            if (timeMatch && parseInt(timeMatch[1]) > 0) {
                startTimer(parseInt(timeMatch[1]));
            } else if (currentState !== previousState) {
                clearInterval(countdownInterval);
                countdownValue = 0; 
            }
        }

        // ==========================================
        // CANVAS SIMULATION
        // ==========================================
        const canvas = document.getElementById('simCanvas');
        const ctx = canvas.getContext('2d');

        class Car {
            constructor(axis, dir, lanePos, color, speed, canTurn = false) {
                this.originalAxis = axis;
                this.originalDir = dir;
                this.originalLanePos = lanePos;
                this.canTurn = canTurn;
                this.axis = axis; 
                this.dir = dir; 
                this.lanePos = lanePos;
                this.color = color;
                this.baseSpeed = speed + Math.random();
                this.speed = this.baseSpeed;
                this.turnDir = 0;
                this.reset(true);
            }

            reset(initial = false) {
                let spawnDist = (initial ? Math.random() * 800 : 800) + Math.random() * 400; 
                
                // Reset turning variables
                this.axis = this.originalAxis;
                this.dir = this.originalDir;
                this.lanePos = this.originalLanePos;
                this.turnDir = 0;
                this.width = this.axis === 'H' ? 65 : 30;
                this.height = this.axis === 'H' ? 30 : 65;

                if (this.axis === 'H') {
                    this.x = this.dir === 1 ? -spawnDist : 800 + spawnDist;
                    this.y = this.lanePos;
                } else {
                    this.x = this.lanePos;
                    this.y = this.dir === 1 ? -spawnDist : 800 + spawnDist;
                }
                this.speed = this.baseSpeed;
            }

            update(cars) {
                let stopLine = this.dir === 1 ? 270 : 530; 
                let targetSpeed = this.baseSpeed;

                let stop = false;
                if (currentMap === 'intersection' || currentMap === 'tmap') {
                    if (this.axis === 'H') {
                        if (currentState === 'red' || currentState === 'yellow' || currentState === 'none') stop = true;
                    } else {
                        if (currentState === 'green' || currentState === 'yellow' || currentState === 'none') stop = true;
                    }
                } else {
                    if (currentState === 'red' || currentState === 'yellow' || currentState === 'none') stop = true;
                }

                let pos = this.axis === 'H' ? this.x : this.y;
                let carSize = this.axis === 'H' ? this.width : this.height;

                if (stop) {
                    let distToStop = (stopLine - (pos + (this.dir === 1 ? carSize : 0))) * this.dir;
                    if (distToStop > 0 && distToStop < 160) {
                        targetSpeed = Math.max(0, this.baseSpeed * (distToStop / 100));
                        if(distToStop < 7) targetSpeed = 0;
                    }
                }

                cars.forEach(other => {
                    if (this !== other && this.axis === other.axis && this.dir === other.dir &&
                        Math.abs(this.lanePos - other.lanePos) < 15) { 
                        let otherPos = this.axis === 'H' ? other.x : other.y;
                        let dist = (otherPos - pos) * this.dir;
                        if (dist > 0 && dist < 120) {
                            if (other.speed < targetSpeed) targetSpeed = other.speed;
                            if (dist < 80) targetSpeed = 0;
                        }
                    }
                });

                if (this.speed < targetSpeed) this.speed += 0.05;
                else if (this.speed > targetSpeed) this.speed -= 0.1;
                if (Math.abs(this.speed) < 0.05) this.speed = 0;

                // T-MAP TURNING LOGIC
                if (currentMap === 'tmap' && this.canTurn) {
                    if (this.originalAxis === 'V' && this.axis === 'V') {
                        if (this.turnDir === 0) {
                            this.turnDir = (this.lanePos > 420) ? 1 : -1; 
                            let options = this.turnDir === 1 ? [410, 445] : [330, 365];
                            this.targetY = options[Math.floor(Math.random() * options.length)];
                        }
                        if (this.y <= this.targetY && this.turnDir !== -10) {
                            this.axis = 'H';
                            this.dir = this.turnDir;
                            this.y = this.targetY; // snap into lane
                            this.lanePos = this.targetY;
                            this.width = 65;
                            this.height = 30;
                            this.turnDir = -10; // flag completed
                        }
                    } else if (this.originalAxis === 'H' && this.axis === 'H') {
                        if (this.turnDir === 0) {
                            this.turnDir = 1; 
                            let options = [330, 365];
                            this.targetX = options[Math.floor(Math.random() * options.length)];
                        }
                        let hasReached = (this.originalDir === 1) ? (this.x >= this.targetX) : (this.x <= this.targetX);
                        if (hasReached && this.turnDir !== -10) {
                            this.axis = 'V';
                            this.dir = 1;
                            this.x = this.targetX; 
                            this.lanePos = this.targetX;
                            this.width = 30;
                            this.height = 65;
                            this.turnDir = -10; 
                        }
                    }
                }

                if (this.axis === 'H') this.x += this.speed * this.dir;
                else this.y += this.speed * this.dir;

                pos = this.axis === 'H' ? this.x : this.y;
                if (this.dir === 1 && pos > 900) this.reset();
                if (this.dir === -1 && pos < -100) this.reset();
            }

            draw() {
                ctx.fillStyle = this.color;
                ctx.beginPath();
                ctx.roundRect(this.x, this.y, this.width, this.height, 6);
                ctx.fill();
                
                ctx.fillStyle = '#1e293b'; // windshield
                let hL = '#facc15'; // headlight colors
                if(this.axis === 'H') {
                    if (this.dir === 1) { // Eastbound (Right)
                        ctx.fillRect(this.x + this.width - 15, this.y + 4, 10, this.height - 8); 
                        ctx.fillStyle = hL; ctx.fillRect(this.x + this.width - 4, this.y + 4, 4, 6); ctx.fillRect(this.x + this.width - 4, this.y + this.height - 10, 4, 6);
                    } else { // Westbound (Left)
                        ctx.fillRect(this.x + 5, this.y + 4, 10, this.height - 8);
                        ctx.fillStyle = hL; ctx.fillRect(this.x, this.y + 4, 4, 6); ctx.fillRect(this.x, this.y + this.height - 10, 4, 6);
                    }
                } else {
                    if (this.dir === 1) { // Southbound (Down)
                        ctx.fillRect(this.x + 4, this.y + this.height - 15, this.width - 8, 10);
                        ctx.fillStyle = hL; ctx.fillRect(this.x + 4, this.y + this.height - 4, 6, 4); ctx.fillRect(this.x + this.width - 10, this.y + this.height - 4, 6, 4);
                    } else { // Northbound (Up)
                        ctx.fillRect(this.x + 4, this.y + 5, this.width - 8, 10);
                        ctx.fillStyle = hL; ctx.fillRect(this.x + 4, this.y, 6, 4); ctx.fillRect(this.x + this.width - 10, this.y, 6, 4);
                    }
                }
            }
        }

        class Pedestrian {
            constructor() {
                this.reset();
            }
            reset() {
                this.active = false;
                setTimeout(() => this.spawn(), Math.random() * 8000 + 2000);
            }
            spawn() {
                let possible = [];
                if (currentMap === 'intersection') possible = [0, 1, 2, 3];
                if (currentMap === 'tmap') possible = [0, 1, 3]; // No top crosswalk (2 is top)
                if (currentMap === 'twoway') possible = [0, 1]; // Only cross the horizontal road

                this.type = possible[Math.floor(Math.random() * possible.length)];
                if (this.type === 0) { this.x = 280; this.y = 300; this.dir = 1; this.axis = 'V'; } 
                if (this.type === 1) { this.x = 520; this.y = 500; this.dir = -1; this.axis = 'V'; } 
                if (this.type === 2) { this.x = 300; this.y = 280; this.dir = 1; this.axis = 'H'; } 
                if (this.type === 3) { this.x = 500; this.y = 520; this.dir = -1; this.axis = 'H'; } 
                this.traveled = 0;
                this.active = true;
            }
            update() {
                if (!this.active) return;
                
                let isLightSafe = false;
                if (currentMap === 'intersection' || currentMap === 'tmap') {
                    if (this.axis === 'V' && currentState === 'red') isLightSafe = true;
                    if (this.axis === 'H' && currentState === 'green') isLightSafe = true;
                } else {
                    if (currentState === 'red') isLightSafe = true;
                }

                // If they are waiting to cross and the light is not safe, don't walk.
                // But if they ALREADY started crossing (traveled > 0), they must finish!
                if (this.traveled === 0 && !isLightSafe) {
                    return; 
                }

                // Walk speed (speed up if caught in an unsafe light!)
                let walkSpeed = (!isLightSafe && this.traveled > 0) ? 2.5 : 1.2;

                if (this.axis === 'H') this.x += this.dir * walkSpeed;
                if (this.axis === 'V') this.y += this.dir * walkSpeed;
                
                this.traveled += walkSpeed;

                if (this.traveled > 210) {
                    this.reset();
                }
            }
            draw() {
                if (!this.active) return;
                
                // Shoulders
                ctx.fillStyle = '#64748b';
                ctx.beginPath();
                ctx.ellipse(this.x, this.y, 
                            this.axis === 'H' ? 6 : 10, 
                            this.axis === 'V' ? 6 : 10, 
                            0, 0, Math.PI*2);
                ctx.fill();
                
                // Head
                ctx.fillStyle = '#f8fafc';
                ctx.beginPath();
                ctx.arc(this.x, this.y, 5, 0, Math.PI * 2);
                ctx.fill();
            }
        }

        let cars = [];
        let pedestrians = [];

        function resetSimulation() {
            cars = [];
            pedestrians = [];
            
            function spawnCar(axis, dir, lanes, count, canTurn=false) {
                for(let i=0; i<count; i++) {
                    let lane = lanes[Math.floor(Math.random() * lanes.length)];
                    let speed = 1.8 + Math.random() * 1.5; // Random unique speeds
                    let hue = Math.floor(Math.random() * 360);
                    let color = `hsl(${hue}, 70%, 60%)`; // Beautiful bright paint
                    cars.push(new Car(axis, dir, lane, color, speed, canTurn));
                }
            }

            if (currentMap === 'intersection') {
                spawnCar('H', 1, [410, 445], 5);
                spawnCar('H', -1, [330, 365], 5);
                spawnCar('V', 1, [330, 365], 5);
                spawnCar('V', -1, [410, 445], 5);
                for(let i=0; i<6; i++) pedestrians.push(new Pedestrian());
            } else if (currentMap === 'tmap') {
                spawnCar('H', 1, [410], 4);
                spawnCar('H', 1, [445], 4, true); 
                spawnCar('H', -1, [330, 365], 8); 
                spawnCar('V', -1, [410], 3, true); 
                spawnCar('V', -1, [445], 3, true); 
                for(let i=0; i<4; i++) pedestrians.push(new Pedestrian());
            } else if (currentMap === 'twoway') {
                spawnCar('H', 1, [410, 445], 8);
                spawnCar('H', -1, [330, 365], 8);
                for(let i=0; i<3; i++) pedestrians.push(new Pedestrian());
            }
        }
        
        resetSimulation();

        function drawMap() {
            ctx.fillStyle = '#1e293b'; 
            ctx.fillRect(0, 0, canvas.width, canvas.height);

            ctx.fillStyle = '#334155';
            
            // Reusable dashed lane divider
            function drawDashedLines(axis, start, end, pos1, pos2) {
                ctx.strokeStyle = '#64748b'; ctx.lineWidth = 2; ctx.setLineDash([10, 20]);
                if (axis === 'H') {
                    ctx.beginPath(); ctx.moveTo(start, pos1); ctx.lineTo(end, pos1); ctx.stroke();
                    ctx.beginPath(); ctx.moveTo(start, pos2); ctx.lineTo(end, pos2); ctx.stroke();
                } else {
                    ctx.beginPath(); ctx.moveTo(pos1, start); ctx.lineTo(pos1, end); ctx.stroke();
                    ctx.beginPath(); ctx.moveTo(pos2, start); ctx.lineTo(pos2, end); ctx.stroke();
                }
                ctx.setLineDash([]);
            }

            if (currentMap === 'intersection') {
                ctx.fillRect(0, 320, 800, 160);
                ctx.fillRect(320, 0, 160, 800);

                ctx.strokeStyle = '#94a3b8'; ctx.lineWidth = 3; ctx.setLineDash([15, 15]);
                ctx.beginPath(); ctx.moveTo(0, 400); ctx.lineTo(320, 400); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(480, 400); ctx.lineTo(800, 400); ctx.stroke();
                ctx.strokeStyle = '#facc15';
                ctx.beginPath(); ctx.moveTo(400, 0); ctx.lineTo(400, 320); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(400, 480); ctx.lineTo(400, 800); ctx.stroke();
                ctx.setLineDash([]);

                drawDashedLines('H', 0, 800, 360, 440);
                drawDashedLines('V', 0, 800, 360, 440);

                ctx.strokeStyle = '#cbd5e1'; ctx.lineWidth = 6;
                ctx.beginPath(); ctx.moveTo(320, 400); ctx.lineTo(320, 480); ctx.stroke(); 
                ctx.beginPath(); ctx.moveTo(480, 320); ctx.lineTo(480, 400); ctx.stroke(); 
                ctx.beginPath(); ctx.moveTo(320, 320); ctx.lineTo(400, 320); ctx.stroke(); 
                ctx.beginPath(); ctx.moveTo(400, 480); ctx.lineTo(480, 480); ctx.stroke(); 

                ctx.fillStyle = '#f1f5f9';
                for (let i = 325; i < 475; i += 20) {
                    ctx.fillRect(i, 270, 10, 40); 
                    ctx.fillRect(i, 490, 10, 40); 
                    ctx.fillRect(270, i, 40, 10); 
                    ctx.fillRect(490, i, 40, 10); 
                }
            } else if (currentMap === 'tmap') {
                ctx.fillRect(0, 320, 800, 160); // Horizontal
                ctx.fillRect(320, 480, 160, 320); // Bottom Vertical

                ctx.strokeStyle = '#94a3b8'; ctx.lineWidth = 3; ctx.setLineDash([15, 15]);
                ctx.beginPath(); ctx.moveTo(0, 400); ctx.lineTo(320, 400); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(480, 400); ctx.lineTo(800, 400); ctx.stroke();
                ctx.strokeStyle = '#facc15';
                ctx.beginPath(); ctx.moveTo(400, 480); ctx.lineTo(400, 800); ctx.stroke();
                ctx.setLineDash([]);
                
                drawDashedLines('H', 0, 800, 360, 440);
                drawDashedLines('V', 480, 800, 360, 440);

                ctx.strokeStyle = '#cbd5e1'; ctx.lineWidth = 6;
                ctx.beginPath(); ctx.moveTo(320, 400); ctx.lineTo(320, 480); ctx.stroke(); 
                ctx.beginPath(); ctx.moveTo(480, 320); ctx.lineTo(480, 400); ctx.stroke(); 
                ctx.beginPath(); ctx.moveTo(400, 480); ctx.lineTo(480, 480); ctx.stroke(); 

                ctx.fillStyle = '#f1f5f9';
                for (let i = 325; i < 475; i += 20) {
                    ctx.fillRect(270, i, 40, 10); // left
                    ctx.fillRect(490, i, 40, 10); // right
                    ctx.fillRect(i, 490, 10, 40); // bottom
                }

            } else if (currentMap === 'twoway') {
                ctx.fillRect(0, 320, 800, 160);
                ctx.strokeStyle = '#facc15'; ctx.lineWidth = 3; ctx.setLineDash([15, 15]);
                ctx.beginPath(); ctx.moveTo(0, 400); ctx.lineTo(800, 400); ctx.stroke();
                ctx.setLineDash([]);
                
                drawDashedLines('H', 0, 800, 360, 440);

                ctx.strokeStyle = '#cbd5e1'; ctx.lineWidth = 6;
                ctx.beginPath(); ctx.moveTo(320, 400); ctx.lineTo(320, 480); ctx.stroke(); 
                ctx.beginPath(); ctx.moveTo(480, 320); ctx.lineTo(480, 400); ctx.stroke(); 

                ctx.fillStyle = '#f1f5f9';
                for (let i = 325; i < 475; i += 20) {
                    ctx.fillRect(270, i, 40, 10); 
                    ctx.fillRect(490, i, 40, 10); 
                }
            }
        }

        function drawTrafficLight(x, y, rotation, stateAxis) {
            ctx.save();
            ctx.translate(x, y);
            ctx.rotate(rotation);
            
            // Box casing
            ctx.fillStyle = '#111';
            ctx.fillRect(-15, -40, 30, 80);
            ctx.strokeStyle = '#333';
            ctx.lineWidth = 2;
            ctx.strokeRect(-15, -40, 30, 80);
            
            let rColor = '#333', yColor = '#333', gColor = '#333';
            
            if (stateAxis === 'H') {
                if (currentState === 'red') rColor = '#ef4444';
                else if (currentState === 'yellow') yColor = '#facc15';
                else if (currentState === 'green') gColor = '#22c55e';
            } else {
                if (currentState === 'red') gColor = '#22c55e';
                else if (currentState === 'yellow') rColor = '#ef4444'; // V stays red during H yellow
                else if (currentState === 'green') rColor = '#ef4444';
            }

            // Draw Bulbs
            ctx.fillStyle = rColor;
            ctx.beginPath(); ctx.arc(0, -20, 10, 0, Math.PI*2); ctx.fill();
            if(rColor !== '#333') { ctx.shadowBlur = 15; ctx.shadowColor = rColor; ctx.fill(); ctx.shadowBlur = 0; }
            
            ctx.fillStyle = yColor;
            ctx.beginPath(); ctx.arc(0, 0, 10, 0, Math.PI*2); ctx.fill();
            if(yColor !== '#333') { ctx.shadowBlur = 15; ctx.shadowColor = yColor; ctx.fill(); ctx.shadowBlur = 0; }

            ctx.fillStyle = gColor;
            ctx.beginPath(); ctx.arc(0, 20, 10, 0, Math.PI*2); ctx.fill();
            if(gColor !== '#333') { ctx.shadowBlur = 15; ctx.shadowColor = gColor; ctx.fill(); ctx.shadowBlur = 0; }

            ctx.restore();
        }

        function drawCenterTimer() {
            ctx.fillStyle = 'rgba(15, 23, 42, 0.8)';
            ctx.beginPath();
            ctx.roundRect(360, 360, 80, 80, 12);
            ctx.fill();

            let tColor = '#ffffff';
            if(currentState === 'red') tColor = '#ef4444';
            if(currentState === 'yellow') tColor = '#facc15';
            if(currentState === 'green') tColor = '#22c55e';

            ctx.fillStyle = tColor;
            ctx.font = 'bold 36px "Share Tech Mono"';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            let s = countdownValue > 0 ? countdownValue.toString().padStart(2, '0') : '--';
            ctx.shadowBlur = countdownValue > 0 ? 15 : 0;
            ctx.shadowColor = tColor;
            ctx.fillText(s, 400, 400);
            ctx.shadowBlur = 0;
            ctx.strokeStyle = '#334155';
            ctx.lineWidth = 2;
            ctx.strokeRect(360, 360, 80, 80);
        }

        function animate() {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            drawMap();

            cars.forEach(car => { car.update(cars); car.draw(); });
            pedestrians.forEach(ped => { ped.update(); ped.draw(); });

            // Render Light Boxes
            if (currentMap === 'intersection') {
                drawTrafficLight(400, 300, Math.PI, 'V'); // Top (faces Up)
                drawTrafficLight(400, 500, 0, 'V'); // Bottom (faces Down)
                drawTrafficLight(290, 400, -Math.PI/2, 'H'); // Left (faces Left)
                drawTrafficLight(510, 400, Math.PI/2, 'H'); // Right (faces Right)
            } else if (currentMap === 'tmap') {
                drawTrafficLight(400, 500, 0, 'V'); // Bottom (faces Down)
                drawTrafficLight(290, 400, -Math.PI/2, 'H'); // Left (faces Left)
                drawTrafficLight(510, 400, Math.PI/2, 'H'); // Right (faces Right)
            } else if (currentMap === 'twoway') {
                drawTrafficLight(290, 400, -Math.PI/2, 'H'); // Left (faces Left)
                drawTrafficLight(510, 400, Math.PI/2, 'H'); // Right (faces Right)
            }
            
            drawCenterTimer();

            requestAnimationFrame(animate);
        }

        animate();

        // ==========================================
        // HYBRID NETWORKING (WIFI & USB SERIAL)
        // ==========================================
        let isHttp = window.location.protocol === 'http:' && window.location.hostname !== '' && window.location.hostname !== 'localhost' && window.location.hostname !== '127.0.0.1';

        if (isHttp) {
            // Running purely via Wi-Fi from the ESP32!
            connectBtn.innerText = "Wi-Fi Connected";
            connectBtn.style.background = "#059669";
            connectBtn.style.cursor = "default";
            
            cmdInput.disabled = false;
            sendBtn.disabled = false;
            document.getElementById('saveBtn').disabled = false;
            log("<b>SUCCESS: Connected via ESP32 Wi-Fi Server!</b>");

            loadSavedSequences();

            // Poll for status every 500ms
            let lastStatusText = '';
            setInterval(async () => {
                try {
                    let res = await fetch('/status');
                    if (res.ok) {
                        let text = await res.text();
                        if (text.trim() !== '' && text !== lastStatusText) {
                            lastStatusText = text;
                            updateLights(text);
                            if (!text.includes(" T:") && !text.includes("OFF")) log(text, "esp");
                        }
                    }
                } catch(e) {}
            }, 500);

            async function loadSavedSequences() {
                try {
                    let res = await fetch('/list_seqs');
                    if (res.ok) {
                        let text = await res.text();
                        let names = text.split(',').filter(n => n.length > 0);
                        renderSavedList(names);
                    }
                } catch(e) {}
            }

            function renderSavedList(names) {
                let container = document.getElementById('savedList');
                if (!container) return;
                if (names.length === 0) {
                    container.innerHTML = '<div style="color: #64748b; font-style: italic; font-size: 12px; padding: 10px;">No saved sequences.</div>';
                    return;
                }
                container.innerHTML = '';
                names.forEach(name => {
                    let div = document.createElement('div');
                    div.className = 'saved-item';
                    div.innerHTML = `
                        <span onclick="playSequence('${name}')">${name}</span>
                        <div class="del-btn" onclick="deleteSeq('${name}')">&times;</div>
                    `;
                    container.appendChild(div);
                });
            }

            window.saveCurrentSequence = async function() {
                let cmd = cmdInput.value.trim();
                if (cmd.length < 1) { alert("Enter a sequence first!"); return; }
                let name = prompt("Enter a name for this sequence:");
                if (!name) return;

                try {
                    let res = await fetch(`/save_seq?name=${encodeURIComponent(name)}&cmd=${encodeURIComponent(cmd)}`);
                    if (res.ok) {
                        log(`Sequence [${name}] saved to database.`);
                        loadSavedSequences();
                    }
                } catch(e) { log("Error saving."); }
            };

            window.playSequence = async function(name) {
                try {
                    let res = await fetch(`/get_seq?name=${encodeURIComponent(name)}`);
                    if (res.ok) {
                        let cmd = await res.text();
                        cmdInput.value = cmd;
                        window.sendCmd(cmd);
                    }
                } catch(e) {}
            };

            window.deleteSeq = async function(name) {
                if (!confirm(`Delete sequence "${name}"?`)) return;
                try {
                    let res = await fetch(`/del_seq?name=${encodeURIComponent(name)}`);
                    if (res.ok) {
                        log(`Sequence [${name}] deleted.`);
                        loadSavedSequences();
                    }
                } catch(e) {}
            };

            window.openWifiModal = function() {
                document.getElementById('modalOverlay').style.display = 'block';
                document.getElementById('wifiModal').style.display = 'block';
            };

            window.closeWifiModal = function() {
                document.getElementById('modalOverlay').style.display = 'none';
                document.getElementById('wifiModal').style.display = 'none';
            };

            window.saveWifi = async function() {
                let ssid = document.getElementById('staSsid').value;
                let pass = document.getElementById('staPass').value;
                if (!ssid) return alert("Enter SSID");
                log("Saving Wi-Fi... ESP32 will reboot.");
                await fetch(`/set_wifi?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`);
                setTimeout(() => window.location.reload(), 5000);
            };

            window.sendCmd = async function(cmdStr) {
                const command = cmdStr || cmdInput.value.trim();
                if (command) {
                    log(command, "user");
                    fetch('/cmd?q=' + encodeURIComponent(command));
                    if(!cmdStr) cmdInput.value = "";
                    cmdInput.focus();
                }
            };
            
            cmdInput.addEventListener("keypress", function (event) {
                if (event.key === "Enter") { event.preventDefault(); window.sendCmd(); }
            });
            sendBtn.addEventListener('click', () => window.sendCmd());

        } else {
            // Running Locally via USB Serial Cable
            window.sendCmd = async function(cmdStr) {
                if (!writer) return;
                const command = cmdStr || cmdInput.value.trim();
                if (command) {
                    log(command, "user");
                    await writer.write(command + '\n');
                    if(!cmdStr) cmdInput.value = "";
                    cmdInput.focus();
                }
            };

            cmdInput.addEventListener("keypress", function (event) {
                if (event.key === "Enter") { event.preventDefault(); window.sendCmd(); }
            });
            sendBtn.addEventListener('click', () => window.sendCmd());

            async function connect() {
                try {
                    port = await navigator.serial.requestPort();
                    await port.open({ baudRate: 115200 });

                    log("<b>SUCCESS: Local USB Connected!</b>");
                    connectBtn.innerText = "Connected";
                    connectBtn.style.background = "#059669";
                    
                    cmdInput.disabled = false;
                    sendBtn.disabled = false;
                    cmdInput.focus();

                    const textEncoder = new TextEncoderStream();
                    textEncoder.readable.pipeTo(port.writable);
                    writer = textEncoder.writable.getWriter();

                    const textDecoder = new TextDecoderStream();
                    port.readable.pipeTo(textDecoder.writable);
                    reader = textDecoder.readable.getReader();

                    let buffer = "";

                    while (true) {
                        const { value, done } = await reader.read();
                        if (done) break;

                        buffer += value;
                        let lines = buffer.split('\n');
                        for (let i = 0; i < lines.length - 1; i++) {
                            let line = lines[i].trim();
                            if (line) {
                                log(line, "esp");

                                if (line.includes("->")) {
                                    updateLights(line);
                                } else if (line.includes("All OFF") || line.includes("STOPPED")) {
                                    updateLights("");
                                }
                            }
                        }
                        buffer = lines[lines.length - 1];
                    }
                } catch (error) {
                    log("<span style='color:#ef4444;'>Error: " + error.message + "</span>");
                }
            }
            connectBtn.addEventListener('click', connect);
        }
    </script>
</body>
</html>
)rawliteral";

int redPin = 25;    
int yellowPin = 26; 
int greenPin = 27;  

struct Step {
  bool r;
  bool y;
  bool g;
  unsigned long duration; 
  bool blink; 
  bool rapid; 
};

Step sequence[20]; 
int stepCount = 0;
bool isLooping = false;

int currentStep = -1; 
unsigned long stepStartTime = 0;
bool isNewStep = false;
String currentStatus = "-> All OFF";

void turnOffAll() {
  digitalWrite(redPin, LOW);
  digitalWrite(yellowPin, LOW);
  digitalWrite(greenPin, LOW);
}

void parseAndStart(String input) {
  input.toUpperCase();      
  input.replace(",", " ");  
  input.trim();

  if (input.length() > 0) {
    if (input.indexOf('X') >= 0) {
      stepCount = 0;
      currentStep = -1;
      turnOffAll();
      currentStatus = "-> All OFF";
      Serial.println("Sequence STOPPED.");
      return; 
    }

    isLooping = (input.indexOf('L') >= 0);
    stepCount = 0; 

    for (int i = 0; i < input.length(); i++) {
      if (i <= input.length() - 5 && input.substring(i, i+5) == "RAPID") {
        if (stepCount > 0) sequence[stepCount - 1].rapid = true;
        i += 4; 
        continue; 
      }

      char c = input[i];

      if (c == 'R' || c == 'Y' || c == 'G' || c == 'S') {
        sequence[stepCount].r = false;
        sequence[stepCount].y = false;
        sequence[stepCount].g = false;
        sequence[stepCount].duration = 0; 
        sequence[stepCount].blink = false; 
        sequence[stepCount].rapid = false; 

        int j = i;
        while (j < input.length()) {
          char temp = input[j];
          if (temp == 'R') sequence[stepCount].r = true;
          else if (temp == 'Y') sequence[stepCount].y = true;
          else if (temp == 'G') sequence[stepCount].g = true;
          else if (temp == 'S') { 
            sequence[stepCount].r = true; 
            sequence[stepCount].y = true; 
            sequence[stepCount].g = true; 
          }
          else break; 
          j++;
        }

        String numStr = "";
        while (j < input.length() && input[j] == ' ') j++; 
        while (j < input.length() && isDigit(input[j])) {
          numStr += input[j]; 
          j++;
        }

        if (numStr.length() > 0) {
          sequence[stepCount].duration = numStr.toInt() * 1000UL;
        }
        
        stepCount++; 
        i = j - 1;   
      }
      
      if (c == 'B') {
        if (stepCount > 0) sequence[stepCount - 1].blink = true;
      }
    }

    if (stepCount > 0) {
      currentStep = 0;
      isNewStep = true;
      Serial.print("Started sequence with ");
      Serial.print(stepCount);
      Serial.println(" steps.");
    }
  }
}

void setup() {
  Serial.begin(115200); 
  
  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);

  Serial.println("--- Advanced Traffic Controller v6 (Wi-Fi Edition) ---");
  
  preferences.begin("traffic", false); 
  String savedCommand = preferences.getString("defaultCmd", "");
  
  if (savedCommand.length() > 0) {
    Serial.println("Found default command! Booting: " + savedCommand);
    parseAndStart(savedCommand); 
  }

  // Load saved sequences list
  savedSequenceNames = preferences.getString("seqList", "");

  // --- Wi-Fi Setup (AP + STA) ---
  String staSSID = preferences.getString("staSSID", "");
  String staPass = preferences.getString("staPass", "");

  WiFi.mode(WIFI_AP_STA);

  Serial.print("Starting Wi-Fi Hotspot: ");
  Serial.println(ssid);
  WiFi.softAP(ssid, password);
  
  if (staSSID.length() > 0) {
      Serial.print("Connecting to STA Wi-Fi: ");
      Serial.println(staSSID);
      WiFi.begin(staSSID.c_str(), staPass.c_str());
      
      // Wait for STA connection (max 15 seconds)
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 30) {
          delay(500);
          Serial.print(".");
          attempts++;
      }
      Serial.println();
      
      if (WiFi.status() == WL_CONNECTED) {
          staConnected = true;
          Serial.println("STA Connected! Enabling internet bridge (NAPT)...");
          
          // Enable NAPT to bridge internet from STA to AP clients
          esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
          if (ap_netif) {
              esp_netif_napt_enable(ap_netif);
              Serial.println("NAPT enabled! AP clients now have internet.");
          } else {
              Serial.println("WARNING: Could not get AP netif for NAPT.");
          }
      } else {
          Serial.println("STA connection failed. Hotspot will work without internet.");
      }
  }
  
  Serial.println("");
  Serial.println("=====================================");
  Serial.print("Hotspot Active: "); Serial.println(ssid);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  if (staConnected) {
      Serial.print("STA IP: "); Serial.println(WiFi.localIP());
      Serial.println("Internet Bridge: ACTIVE");
  }
  Serial.println("=====================================");

  // Start DNS Server for Captive Portal ONLY if NOT bridging internet
  // (Captive portal DNS intercepts ALL queries -> breaks real DNS resolution)
  if (!staConnected) {
      dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
      Serial.println("Captive Portal DNS started.");
  } else {
      Serial.println("Captive Portal DNS skipped (internet bridge active).");
  }

  // --- Web Server Setup ---
  server.on("/", HTTP_GET, []() {
      server.sendHeader("Content-Encoding", "identity");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/html", HTML_PAGE);
  });
  
  // Catch-all to redirect any other URL to the main page (Captive Portal trick)
  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  });

  server.on("/status", HTTP_GET, []() {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", currentStatus);
  });

  server.on("/cmd", HTTP_GET, []() {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      if (server.hasArg("q")) {
          String input = server.arg("q");
          input.trim();
          parseAndStart(input);
          server.send(200, "text/plain", "OK");
      } else {
          server.send(400, "text/plain", "Missing q param");
      }
  });

  server.on("/save_seq", HTTP_GET, []() {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      String name = server.arg("name");
      String cmd = server.arg("cmd");
      if (name.length() > 0 && cmd.length() > 0) {
          saveSequence(name, cmd);
          server.send(200, "text/plain", "SAVED");
      } else {
          server.send(400, "text/plain", "Invalid params");
      }
  });

  server.on("/list_seqs", HTTP_GET, []() {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", savedSequenceNames);
  });

  server.on("/get_seq", HTTP_GET, []() {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      String name = server.arg("name");
      String cmd = preferences.getString(("s_" + name).c_str(), "");
      server.send(200, "text/plain", cmd);
  });

  server.on("/del_seq", HTTP_GET, []() {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      String name = server.arg("name");
      deleteSequence(name);
      server.send(200, "text/plain", "DELETED");
  });

  server.on("/set_wifi", HTTP_GET, []() {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      String s = server.arg("ssid");
      String p = server.arg("pass");
      preferences.putString("staSSID", s);
      preferences.putString("staPass", p);
      server.send(200, "text/plain", "REBOOTING...");
      delay(1000);
      ESP.restart();
  });

  server.begin();
  Serial.println("HTTP Server Started");
}

void loop() {
  if (!staConnected) dnsServer.processNextRequest(); // Only run captive portal when no internet
  server.handleClient(); // Handle all HTTP requests

  // Allow USB Serial as fallback control!
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.endsWith(" D") || input == "D" || input.indexOf(" D ") >= 0) {
      preferences.putString("defaultCmd", input);
      Serial.println(">>> SAVED AS POWER-ON DEFAULT! <<<");
    }
    parseAndStart(input);
  }

  if (currentStep >= 0 && currentStep < stepCount) {
    
    if (isNewStep) {
      turnOffAll();
      
      String active = "";
      if (sequence[currentStep].r) active += "Red ";
      if (sequence[currentStep].y) active += "Yellow ";
      if (sequence[currentStep].g) active += "Green ";
      
      String mode = "";
      if (sequence[currentStep].duration > 0) {
        mode += " T:" + String(sequence[currentStep].duration / 1000);
      }
      if (sequence[currentStep].rapid) mode += " (RAPID)";
      else if (sequence[currentStep].blink) mode += " (BLINK:" + String(sequence[currentStep].duration) + ")";
      
      currentStatus = "-> " + active + "ON" + mode;
      Serial.println(currentStatus);
      
      if (!sequence[currentStep].rapid) {
        if (sequence[currentStep].r) digitalWrite(redPin, HIGH);
        if (sequence[currentStep].y) digitalWrite(yellowPin, HIGH);
        if (sequence[currentStep].g) digitalWrite(greenPin, HIGH);
      }
      
      stepStartTime = millis(); 
      isNewStep = false;
    }

    if (sequence[currentStep].rapid) {
      bool ledState = (millis() / 50) % 2 == 0; 
      if (sequence[currentStep].r) digitalWrite(redPin, ledState ? HIGH : LOW);
      if (sequence[currentStep].y) digitalWrite(yellowPin, ledState ? HIGH : LOW);
      if (sequence[currentStep].g) digitalWrite(greenPin, ledState ? HIGH : LOW);
    }
    
    else if (sequence[currentStep].duration > 0) {
      unsigned long timePassed = millis() - stepStartTime;
      
      if (sequence[currentStep].blink) {
        unsigned long blinkStart = (sequence[currentStep].duration > 2000) ? sequence[currentStep].duration - 2000 : 0;
        if (timePassed >= blinkStart) {
          unsigned long timeInBlinkZone = timePassed - blinkStart;
          bool ledState = (timeInBlinkZone / 250) % 2 == 0; 
          
          if (sequence[currentStep].r) digitalWrite(redPin, ledState ? HIGH : LOW);
          if (sequence[currentStep].y) digitalWrite(yellowPin, ledState ? HIGH : LOW);
          if (sequence[currentStep].g) digitalWrite(greenPin, ledState ? HIGH : LOW);
        }
      }

      if (timePassed >= sequence[currentStep].duration) {
        currentStep++; 
        isNewStep = true;
        if (currentStep >= stepCount) {
          if (isLooping) {
            currentStep = 0; 
            Serial.println("--- Looping ---");
          } else {
            currentStep = -1; 
            turnOffAll();
            currentStatus = "-> All OFF";
            Serial.println("Sequence complete. All OFF.");
          }
        }
      }
    }
  }
}

