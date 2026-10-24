#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

const char web_html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8"/>
<meta content="width=device-width, initial-scale=1.0" name="viewport"/>
<title>wifeBoxColon3 control</title>
<style>
:root {
  --bg: #111;
  --fg: #eee;
  --border: #444;
  --highlight: #fff;
  --accent: #222;
}
* {
  box-sizing: border-box;
  font-family: 'Courier New', Courier, monospace;
  text-transform: lowercase;
  border-radius: 0 !important;
}
body {
  background-color: var(--bg);
  color: var(--fg);
  margin: 0;
  padding: 20px;
  display: flex;
  flex-direction: column;
  align-items: center;
}
h1, h2 {
  font-weight: normal;
  border-bottom: 1px solid var(--border);
  padding-bottom: 5px;
  margin-top: 0;
}
h1 {
  font-size: 1.5rem;
  margin-bottom: 30px;
}
h2 {
  font-size: 1.1rem;
  margin-bottom: 15px;
}
.container {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 20px;
  width: 100%;
  max-width: 1200px;
}
.card {
  border: 1px solid var(--border);
  background: var(--accent);
  padding: 20px;
}
.input-group {
  display: flex;
  flex-direction: column;
  gap: 10px;
  margin-bottom: 15px;
}
label {
  font-size: 0.9rem;
  color: #aaa;
}
input[type="text"], input[type="datetime-local"], select {
  width: 100%;
  padding: 10px;
  background: var(--bg);
  color: var(--fg);
  border: 1px solid var(--border);
  outline: none;
}
input:focus, select:focus {
  border-color: var(--highlight);
}
input[type="range"] {
  width: 100%;
  accent-color: var(--fg);
}
button {
  width: 100%;
  padding: 12px;
  background: var(--bg);
  color: var(--fg);
  border: 1px solid var(--border);
  cursor: pointer;
  font-weight: bold;
  transition: background 0.2s;
}
button:hover {
  background: var(--fg);
  color: var(--bg);
}
.btn-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}
.status, .notification {
  text-align: center;
  min-height: 20px;
  margin-top: 10px;
  font-size: 0.9rem;
}
.asleep-mode {
  background-color: #000;
}
#sleep-banner, .ack-box {
  background: var(--bg);
  color: var(--fg);
  border: 1px solid var(--border);
  padding: 10px;
  margin-bottom: 20px;
  text-align: center;
  width: 100%;
  max-width: 1200px;
}
.form-group {
  margin-bottom: 15px;
}
canvas {
  background: var(--bg);
  border: 1px solid var(--border) !important;
}
</style>
</head>
<body>
<h1> wifeBoxColon3 control </h1>
<div class="container">
<div class="card">
<div class="form-group">
<label for="brightness">oled brightness</label>
<input id="brightness" max="255" min="0" onchange="updateBrightness(this.value)" type="range" value="128"/>
</div>
</div>
<div class="card">
<div class="form-group">
<label for="message">send a note</label>
<input id="message" placeholder="type something sweet..." type="text"/>
</div>
<button onclick="sendMessage()">send message</button>
<div class="notification" id="msg-status"></div>
</div>
<div class="card">
<button onclick="playBirthday()"> play happy birthday</button>
<div class="notification" id="bday-status"></div>
</div>
<div class="card">
<div class="form-group">
<label for="jukeboxSelect">buzzer jukebox</label>
<select id="jukeboxSelect">
<option value="0">tem shop (undertale)</option>
<option value="1">undertale theme (undertale)</option>
<option value="2">snowdin town (undertale)</option>
<option value="3">memory (undertale)</option>
<option value="4">home (undertale)</option>
<option value="5">an ending (undertale)</option>
<option value="6">sans. (undertale)</option>
<option value="7">once upon a time (undertale)</option>
<option value="8">bergentruckung (undertale)</option>
<option value="9">asgore (undertale)</option>
<option value="10">good morning (omori)</option>
<option value="11">by your side (omori)</option>
<option value="12">white space (omori)</option>
<option value="13">see you tomorrow (omori)</option>
<option value="14">trees (omori)</option>
<option value="15">title (omori)</option>
</select>
</div>
<div class="btn-grid">
<button onclick="playJukebox()">play track</button>
<button onclick="stopJukebox()">stop music</button>
</div>
<div class="notification" id="jukebox-status"></div>
</div>
<div class="card">
<h2>screen power</h2>
<div class="btn-grid">
<button onclick="sleepScreen()">sleep screen</button>
<button onclick="wakeScreen()">wake screen</button>
</div>
<div class="notification" id="power-status"></div>
</div>
<div class="card">
<h2>physical button simulator</h2>
<div class="btn-grid">
<button onclick="simBtn('btn1')">btn 1 (scroll)</button>
<button onclick="simBtn('btn2_short')">btn 2 (play/pause)</button>
<button onclick="simBtn('btn2_long')">btn 2 (next song)</button>
<button onclick="simBtn('btn3')">btn 3 (miss u webhook)</button>
</div>
<div class="notification" id="btn-status"></div>
</div>
<div class="card">
<h2>tamagotchi kitty</h2>
<div class="btn-grid">
<button onclick="kittyCmd('open')">open kitty</button>
<button onclick="kittyCmd('feed')">feed kitty</button>
<button onclick="kittyCmd('pet')">pet kitty</button>
<button onclick="kittyCmd('sleep')">toggle sleep</button>
<button onclick="kittyCmd('exit')">exit kitty</button>
</div>
<div class="notification" id="kitty-status"></div>
</div>
<div class="card">
<h2>screen switcher</h2>
<div class="btn-grid">
<button onclick="switchScreen('clock')">clock &amp; weather</button>
<button onclick="switchScreen('spotify')">spotify</button>
<button onclick="switchScreen('spotify_recent')">spotify recent</button>
<button onclick="switchScreen('music')">buzzer jukebox</button>
<button onclick="switchScreen('birthday')">birthday</button>
<button onclick="switchScreen('doodle')">doodle art</button>
<button onclick="switchScreen('photo')">wife photo</button>
<button onclick="switchScreen('kitty')">tamagotchi kitty</button>
</div>
<div class="notification" id="screen-status"></div>
</div>
</div>
<script>
        function sleepScreen() {
            const status = document.getElementById('power-status');
            fetch('/sleep', { method: 'POST' })
            .then(response => {
                status.innerText = "sleeping!";
                setTimeout(() => status.innerText = "", 3000);
            })
            .catch(err => {
                status.innerText = "error!";
                setTimeout(() => status.innerText = "", 3000);
            });
        }
        function wakeScreen() {
            const status = document.getElementById('power-status');
            fetch('/wake', { method: 'POST' })
            .then(response => {
                status.innerText = "waking up!";
                setTimeout(() => status.innerText = "", 3000);
            })
            .catch(err => {
                status.innerText = "error!";
                setTimeout(() => status.innerText = "", 3000);
            });
        }

        function simBtn(btnName) {
            fetch('/btn', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'id=' + btnName.replace('btn', '').replace('_short', 's').replace('_long', 'l')
            })
            .then(r => r.text())
            .then(t => {
                const s = document.getElementById('btn-status');
                s.innerText = 'simulated: ' + btnName;
                s.classList.add('show');
                setTimeout(() => s.classList.remove('show'), 3000);
            });
        }
        function updateBrightness(val) {
            fetch('/brightness?value=' + val)
                .catch(err => console.error(err));
        }

        function sendMessage() {
            const msg = document.getElementById('message').value;
            if(!msg) return;
            const btn = document.querySelector('button[onclick="sendMessage()"]');
            const status = document.getElementById('msg-status');
            btn.disabled = true;
            fetch('/send_note', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'message=' + encodeURIComponent(msg)
            })
            .then(response => {
                status.innerText = "sent successfully!";
                setTimeout(() => status.innerText = "", 3000);
            })
            .catch(err => {
                status.innerText = "error sending!";
                status.style.color = "red";
                setTimeout(() => { status.innerText = ""; status.style.color = "#4CAF50"; }, 3000);
            })
            .finally(() => btn.disabled = false);
        }

        function playBirthday() {
            const status = document.getElementById('bday-status');
            fetch('/birthday', { method: 'POST' })
            .then(response => {
                status.innerText = "playing!";
                setTimeout(() => status.innerText = "", 3000);
            })
            .catch(err => {
                status.innerText = "error!";
                status.style.color = "red";
                setTimeout(() => { status.innerText = ""; status.style.color = "#4CAF50"; }, 3000);
            });
        }

                function stopJukebox() {
            const status = document.getElementById('jukebox-status');
            fetch('/stop', { method: 'POST' })
            .then(response => {
                status.innerText = "music stopped!";
                setTimeout(() => status.innerText = "", 3000);
            })
            .catch(err => {
                status.innerText = "error!";
                setTimeout(() => { status.innerText = ""; }, 3000);
            });
        }

        function playJukebox() {
            const val = document.getElementById('jukeboxSelect').value;
            const status = document.getElementById('jukebox-status');
            fetch('/play', { 
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'song=' + encodeURIComponent(val)
            })
            .then(response => {
                status.innerText = "playing track!";
                setTimeout(() => status.innerText = "", 3000);
            })
            .catch(err => {
                status.innerText = "error!";
                status.style.color = "red";
                setTimeout(() => { status.innerText = ""; status.style.color = "#4CAF50"; }, 3000);
            });
        }

        function kittyCmd(act) {
            const status = document.getElementById('kitty-status');
            fetch('/kitty', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'action=' + encodeURIComponent(act)
            })
            .then(response => {
                status.innerText = "kitty: " + act + "!";
                setTimeout(() => status.innerText = "", 3000);
            })
            .catch(err => {
                status.innerText = "error!";
                status.style.color = "red";
                setTimeout(() => { status.innerText = ""; status.style.color = "#4CAF50"; }, 3000);
            });
        }

        function switchScreen(id) {
            const status = document.getElementById('screen-status');
            fetch('/screen', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'id=' + encodeURIComponent(id)
            })
            .then(response => {
                status.innerText = "screen: " + id;
                setTimeout(() => status.innerText = "", 3000);
            })
            .catch(err => {
                status.innerText = "error!";
                status.style.color = "red";
                setTimeout(() => { status.innerText = ""; status.style.color = "#4CAF50"; }, 3000);
            });
        }
    </script>
</body>
</html>

)rawliteral";

#endif
