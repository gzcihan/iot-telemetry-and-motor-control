#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <ClosedCube_HDC1080.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_BMP085.h>

// Wi-Fi Bilgileri
const char* ssid     = "CIHAN"; 
const char* password = "cihan1515";

ESP8266WebServer server(80);

// I2C Sensör Nesneleri
ClosedCube_HDC1080 hdc1080;
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
Adafruit_BMP085 bmp;

// GPS Modülü (D5 -> RX, D6 -> TX)
SoftwareSerial gpsSerial(14, 12); 
TinyGPSPlus gps;

// MQ Sensörü Pin Tanımlaması
const int mqPin = A0;

// HTML Arayüzünü Flash Hafızada (PROGMEM) saklıyoruz
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>IoT Kontrol Paneli — Cihan Gözcü</title>
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
<style>
:root {
  --bg-color: #030712;
  --panel-bg: rgba(13, 18, 30, 0.65);
  --border-color: rgba(255, 255, 255, 0.06);
  --text-main: #f3f4f6;
  --text-muted: #9ca3af;
  --primary: #ef4444;
}
* { margin:0; padding:0; box-sizing:border-box; font-family: 'Segoe UI', sans-serif; }
body {
  background: var(--bg-color);
  color: var(--text-main);
  min-height: 100vh;
  padding-bottom: 40px;
  position: relative;
  overflow-x: hidden;
}
#space-canvas {
  position: fixed;
  top: 0; left: 0;
  width: 100vw; height: 100vh;
  z-index: 1; pointer-events: none;
}
.content-wrapper { position: relative; z-index: 2; width: 100%; }
.header { padding: 40px 20px; text-align: center; }
.flag-img { width: 90px; height: auto; filter: drop-shadow(0 0 20px rgba(239, 68, 68, 0.6)); display: block; margin: 0 auto 16px; border-radius: 6px; }

/* İstediğiniz Başlık Güncellemesi: Gözcü büyük ve beyaz renk */
.brand-title { font-size: 2.2rem; font-weight: 800; color: #ffffff; }
.brand-title span { color: #ffffff; -webkit-text-fill-color: #ffffff; }

.sub-title { font-size: 0.85rem; color: #64748b; text-transform: uppercase; letter-spacing: 0.15em; margin-top: 6px; font-weight: 600; }
.tags-container { margin-top: 14px; display: flex; justify-content: center; gap: 10px; flex-wrap: wrap; }
.tag { padding: 6px 16px; border-radius: 30px; font-size: 0.75rem; font-weight: 600; backdrop-filter: blur(8px); border: 1px solid rgba(255,255,255,0.05); }
.tag-dept { background: rgba(239, 68, 68, 0.1); border-color: rgba(239, 68, 68, 0.2); color: #f87171; }
.tag-status { background: rgba(16, 185, 129, 0.1); border-color: rgba(16, 185, 129, 0.2); color: #34d399; }
.container { max-width: 1100px; margin: 0 auto; padding: 0 20px; }
.project-meta { background: var(--panel-bg); backdrop-filter: blur(12px); border: 1px solid var(--border-color); border-left: 4px solid var(--primary); border-radius: 16px; padding: 20px; display: flex; align-items: center; gap: 20px; margin-bottom: 30px; }
.project-icon-wrapper { width: 50px; height: 50px; background: rgba(255,255,255,0.02); border-radius: 12px; display: flex; align-items: center; justify-content: center; font-size: 1.5rem; color: var(--primary); border: 1px solid rgba(255,255,255,0.05); }
.project-text h2 { font-size: 1.1rem; font-weight: 700; margin-bottom: 4px; }
.project-text p { font-size: 0.85rem; color: var(--text-muted); }
.grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(240px, 1fr)); gap: 20px; margin-bottom: 25px; }
.card { background: var(--panel-bg); backdrop-filter: blur(12px); border: 1px solid var(--border-color); border-radius: 20px; padding: 22px; position: relative; overflow: hidden; transition: transform 0.3s, border-color 0.3s; box-shadow: 0 8px 25px rgba(0,0,0,0.3); }
.card:hover { transform: translateY(-4px); border-color: var(--c); box-shadow: 0 15px 30px -10px var(--shadow); }
.card-header-area { display: flex; justify-content: space-between; align-items: center; margin-bottom: 14px; }
.icon-box { width: 42px; height: 42px; background: var(--bg-alpha); border-radius: 12px; display: flex; align-items: center; justify-content: center; font-size: 1.2rem; color: var(--c); border: 1px solid var(--border-alpha); }
.card-label { font-size: 0.8rem; font-weight: 600; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.05em; }
.card-value { font-size: 1.7rem; font-weight: 700; color: #fff; display: flex; align-items: baseline; }
.card-unit { font-size: 0.85rem; color: var(--text-muted); margin-left: 4px; font-weight: 500; }
.progress-container { margin-top: 14px; }
.progress-bg { height: 5px; background: rgba(255,255,255,0.03); border-radius: 10px; overflow: hidden; }
.progress-bar { height: 100%; background: var(--c); width: 0%; border-radius: 10px; transition: width 0.6s; box-shadow: 0 0 10px var(--c); }
.card-footer-status { margin-top: 10px; font-size: 0.75rem; font-weight: 600; color: var(--c); display: flex; align-items: center; gap: 6px; }
.axis-container { display: flex; flex-direction: column; gap: 8px; margin-top: 10px; }
.axis-row { display: flex; align-items: center; justify-content: space-between; font-size: 0.85rem; }
.axis-lbl { font-weight: 700; color: var(--text-muted); }
.meta-bottom { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 20px; margin-bottom: 40px; }
.info-card { background: var(--panel-bg); backdrop-filter: blur(12px); border: 1px solid var(--border-color); padding: 18px 24px; border-radius: 16px; display: flex; align-items: center; gap: 16px; }
.info-card i { font-size: 1.4rem; color: #64748b; }
.info-card-text { display: flex; flex-direction: column; }
.info-card-label { font-size: 0.75rem; color: var(--text-muted); text-transform: uppercase; }
.info-card-value { font-size: 1.1rem; font-weight: 700; color: #fff; margin-top: 2px; }

footer { text-align: center; font-size: 0.8rem; color: #475569; padding: 25px 0; border-top: 1px solid rgba(255,255,255,0.02); }
@media(max-width: 640px){ .grid { grid-template-columns: 1fr; } }
</style>
</head>
<body>
  <canvas id="space-canvas"></canvas>
  <div class="content-wrapper">
    <div class="header">
      <img class="flag-img" src="https://upload.wikimedia.org/wikipedia/commons/b/b4/Flag_of_Turkey.svg" alt="Türk Bayrağı">
      <div class="brand-title">Cihan <span>Gözcü</span></div><div class="sub-title">Sakarya Üniversitesi</div>
      <div class="tags-container">
        <div class="tag tag-dept"><i class="fa-solid fa-bolt"></i> Elektrik-Elektronik Mühendisliği</div>
        <div class="tag tag-status" id="durum"><i class="fa-solid fa-circle-notch fa-spin"></i> BAĞLANIYOR...</div>
      </div>
    </div>
    
    <div class="container">
      <div class="project-meta">
        <div class="project-icon-wrapper"><i class="fa-solid fa-server"></i></div>
        <div class="project-text">
          <h2>Tam Donanımlı Telemetri ve Çevresel Analiz İstasyonu</h2>
          <p>I2C bus altyapısı ve analog/seri veri hatları üzerinden toplanan modül verileri anlık işlenmektedir.</p>
        </div>
      </div>
      
      <div class="grid">
        <div class="card" style="--c:#06b6d4; --shadow:rgba(6,182,212,0.15); --bg-alpha:rgba(6,182,212,0.05); --border-alpha:rgba(6,182,212,0.15);">
          <div class="card-header-area"><span class="card-label">Sıcaklık (HDC)</span><div class="icon-box"><i class="fa-solid fa-thermometer-half"></i></div></div>
          <div class="card-value" id="v-sicaklik">--<span class="card-unit">°C</span></div>
          <div class="progress-container"><div class="progress-bg"><div class="progress-bar" id="b-sicaklik"></div></div></div>
          <div class="card-footer-status" id="s-sicaklik">—</div>
        </div>
        <div class="card" style="--c:#8b5cf6; --shadow:rgba(139,92,246,0.15); --bg-alpha:rgba(139,92,246,0.05); --border-alpha:rgba(139,92,246,0.15);">
          <div class="card-header-area"><span class="card-label">Nem Oranı (HDC)</span><div class="icon-box"><i class="fa-solid fa-droplet"></i></div></div>
          <div class="card-value" id="v-nem">--<span class="card-unit">%</span></div>
          <div class="progress-container"><div class="progress-bg"><div class="progress-bar" id="b-nem"></div></div></div>
          <div class="card-footer-status" id="s-nem">—</div>
        </div>
        <div class="card" style="--c:#3b82f6; --shadow:rgba(59,130,246,0.15); --bg-alpha:rgba(59,130,246,0.05); --border-alpha:rgba(59,130,246,0.15);">
          <div class="card-header-area"><span class="card-label">Basınç (BMP180)</span><div class="icon-box"><i class="fa-solid fa-cloud-arrow-down"></i></div></div>
          <div class="card-value" id="v-basinc">--<span class="card-unit">hPa</span></div>
          <div class="progress-container"><div class="progress-bg"><div class="progress-bar" id="b-basinc"></div></div></div>
          <div class="card-footer-status" id="s-basinc">—</div>
        </div>
        <div class="card" style="--c:#ec4899; --shadow:rgba(236,72,153,0.15); --bg-alpha:rgba(236,72,153,0.05); --border-alpha:rgba(236,72,153,0.15);">
          <div class="card-header-area"><span class="card-label">Barometrik Rakım</span><div class="icon-box"><i class="fa-solid fa-mountain"></i></div></div>
          <div class="card-value" id="v-rakim">--<span class="card-unit">m</span></div>
          <div class="progress-container"><div class="progress-bg"><div class="progress-bar" id="b-rakim"></div></div></div>
          <div class="card-footer-status" id="s-rakim">—</div>
        </div>
        <div class="card" style="--c:#f59e0b; --shadow:rgba(245,158,11,0.15); --bg-alpha:rgba(245,158,11,0.05); --border-alpha:rgba(245,158,11,0.15);">
          <div class="card-header-area"><span class="card-label">Hava Kalitesi / Gaz</span><div class="icon-box"><i class="fa-solid fa-wind"></i></div></div>
          <div class="card-value" id="v-gaz">--</div>
          <div class="progress-container"><div class="progress-bg"><div class="progress-bar" id="b-gaz"></div></div></div>
          <div class="card-footer-status" id="s-gaz">—</div>
        </div>
        <div class="card" style="--c:#10b981; --shadow:rgba(16,185,129,0.15); --bg-alpha:rgba(16,185,129,0.05); --border-alpha:rgba(16,185,129,0.15);">
          <div class="card-header-area"><span class="card-label">İvmeölçer (ADXL)</span><div class="icon-box"><i class="fa-solid fa-compass"></i></div></div>
          <div class="axis-container">
            <div class="axis-row"><span class="axis-lbl">X:</span><span id="v-ax">0.00</span> m/s²</div>
            <div class="axis-row"><span class="axis-lbl">Y:</span><span id="v-ay">0.00</span> m/s²</div>
            <div class="axis-row"><span class="axis-lbl">Z:</span><span id="v-az">0.00</span> m/s²</div>
          </div>
          <div class="card-footer-status" id="s-ivme">✅ Aktif</div>
        </div>
        <div class="card" style="--c:#6366f1; --shadow:rgba(99,102,241,0.15); --bg-alpha:rgba(99,102,241,0.05); --border-alpha:rgba(99,102,241,0.15);">
          <div class="card-header-area"><span class="card-label">GPS Enlem</span><div class="icon-box"><i class="fa-solid fa-location-crosshairs"></i></div></div>
          <div class="card-value" id="v-enlem" style="font-size:1.3rem;">--</div>
          <div class="card-footer-status" id="s-enlem">—</div>
        </div>
        <div class="card" style="--c:#14b8a6; --shadow:rgba(20,184,166,0.15); --bg-alpha:rgba(20,184,166,0.05); --border-alpha:rgba(20,184,166,0.15);">
          <div class="card-header-area"><span class="card-label">GPS Boylam</span><div class="icon-box"><i class="fa-solid fa-map-marked-alt"></i></div></div>
          <div class="card-value" id="v-boylam" style="font-size:1.3rem;">--</div>
          <div class="card-footer-status" id="s-boylam">—</div>
        </div>
      </div>
      
      <div class="meta-bottom">
        <div class="info-card"><i class="fa-solid fa-satellite"></i><div class="info-card-text"><div class="info-card-label">Bağlı Uydu Sayısı</div><div class="info-card-value" id="v-uydu">0</div></div></div>
        <div class="info-card"><i class="fa-solid fa-gauge-high"></i><div class="info-card-text"><div class="info-card-label">GPS Anlık Hız</div><div class="info-card-value" id="v-hiz">0.0 km/h</div></div></div>
        <div class="info-card"><i class="fa-solid fa-layer-group"></i><div class="info-card-text"><div class="info-card-label">GPS İrtifa (Yükseklik)</div><div class="info-card-value" id="v-gps-rakim">0.0 m</div></div></div>
      </div>
      <footer>© 2026 · Sakarya Üniversitesi · Cihan Gözcü · Tüm Hakları Saklıdır.</footer>
    </div>
  </div>

<script>
var canvas = document.getElementById('space-canvas');
var ctx = canvas.getContext('2d');
var stars = [];
var meteors = [];

var resizeCanvas = function() {
  canvas.width = window.innerWidth;
  canvas.height = window.innerHeight;
};
window.addEventListener('resize', resizeCanvas);
resizeCanvas();

for(var i=0; i<100; i++) {
  stars.push({
    x: Math.random() * canvas.width,
    y: Math.random() * canvas.height,
    size: Math.random() * 1.2,
    alpha: Math.random(),
    speed: 0.01 + Math.random() * 0.02
  });
}

var createMeteor = function() {
  if (meteors.length < 2 && Math.random() < 0.008) {
    meteors.push({
      x: Math.random() * (canvas.width * 0.8),
      y: Math.random() * (canvas.height / 3),
      len: 30 + Math.random() * 30,
      speed: 7 + Math.random() * 5,
      size: 1
    });
  }
};

var animateSpace = function() {
  ctx.fillStyle = '#050811';
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  
  var gradient = ctx.createRadialGradient(canvas.width/2, canvas.height/2, 50, canvas.width/2, canvas.height/2, canvas.width);
  gradient.addColorStop(0, '#0f1626');
  gradient.addColorStop(1, '#050811');
  ctx.fillStyle = gradient;
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  ctx.fillStyle = '#ffffff';
  stars.forEach(function(star) {
    star.alpha += star.speed;
    if (star.alpha > 1 || star.alpha < 0) star.speed = -star.speed;
    ctx.globalAlpha = Math.abs(star.alpha);
    ctx.fillRect(star.x, star.y, star.size, star.size);
  });
  ctx.globalAlpha = 1.0;

  createMeteor();
  meteors.forEach(function(m, idx) {
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.4)';
    ctx.lineWidth = m.size;
    ctx.beginPath();
    ctx.moveTo(m.x, m.y);
    ctx.lineTo(m.x - m.len, m.y + m.len);
    ctx.stroke();

    m.x += m.speed; m.y += m.speed;
    if (m.x > canvas.width || m.y > canvas.height) meteors.splice(idx, 1);
  });

  requestAnimationFrame(animateSpace);
};
animateSpace();

var bar = function(id,val,min,max){ var p=Math.min(100,Math.max(0,(val-min)/(max-min)*100)); document.getElementById(id).style.width=p+'%'; };
var set = function(id,html){document.getElementById(id).innerHTML=html;};

var guncelle = async function(){
  try{
    var r=await fetch('/api/data'); var d=await r.json();
    set('v-sicaklik',d.sicaklik.toFixed(1)+'<span class="card-unit">°C</span>');
    set('v-nem',     d.nem.toFixed(1)+'<span class="card-unit">%</span>');
    set('v-basinc',  d.basinc.toFixed(1)+'<span class="card-unit">hPa</span>');
    set('v-rakim',   d.rakim.toFixed(0)+'<span class="card-unit">m</span>');
    set('v-gaz',     d.gaz);
    set('v-ax',      d.ax.toFixed(2)); set('v-ay', d.ay.toFixed(2)); set('v-az', d.az.toFixed(2));
    set('v-enlem',   d.enlem === 0 ? "Sinyal Aranıyor..." : d.enlem.toFixed(6));
    set('v-boylam',  d.boylam === 0 ? "Sinyal Aranıyor..." : d.boylam.toFixed(6));
    set('v-uydu',    d.uydu);
    set('v-hiz',     d.hiz.toFixed(1)+' km/h');
    set('v-gps-rakim', d.enlem === 0 ? "Kilit Yok" : d.gps_rakim.toFixed(1)+' m');
    
    bar('b-sicaklik',d.sicaklik,-10,50); bar('b-nem',d.nem,0,100); bar('b-basinc',d.basinc,950,1050); bar('b-rakim',d.rakim,0,500); bar('b-gaz',d.gaz,0,1023);
    document.getElementById('s-sicaklik').textContent = d.sicaklik > 32 ? "⚠️ Yüksek" : "✅ Normal";
    document.getElementById('s-nem').textContent = "✅ Kararlı";
    document.getElementById('s-basinc').textContent = "✅ Ölçülüyor";
    document.getElementById('s-rakim').textContent = "⛰️ Kalibre Edildi";
    document.getElementById('s-gaz').textContent = d.gaz > 400 ? "⚠️ Gaz Algılandı!" : "✅ Hava Temiz";
    document.getElementById('s-enlem').textContent = d.enlem === 0 ? "❌ Kilit Yok" : "🛰️ Konum Aktif";
    document.getElementById('s-boylam').textContent = d.boylam === 0 ? "❌ Kilit Yok" : "🛰️ Konum Aktif";
    
    var el=document.getElementById('durum'); el.innerHTML='<i class="fa-solid fa-circle" style="font-size:0.6rem; animation: pulse 1.5s infinite;"></i> CANLI'; el.style.color='#10b981'; el.style.borderColor='rgba(16,185,129,0.3)'; el.style.background='rgba(16,185,129,0.1)';
  } catch(e){ var el=document.getElementById('durum'); el.innerHTML='<i class="fa-solid fa-triangle-exclamation"></i> BAĞLANTI KESİLDİ'; el.style.color='#ef4444'; }
};
var style = document.createElement('style'); style.innerHTML = '@keyframes pulse { 0% { opacity: 0.4; } 50% { opacity: 1; } 100% { opacity: 0.4; } }'; document.head.appendChild(style);
guncelle(); setInterval(guncelle,2000);
</script>
</body>
</html>
)rawliteral";

// Web Sunucu Ana Sayfa Rotası
void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

// Web Arayüzü İçin JSON API Çıktısı
void handleData() {
  String json = "{";
  
  float t = hdc1080.readTemperature();
  float h = hdc1080.readHumidity();
  if (isnan(t) || t > 120 || t < -40) t = 0.0;
  if (isnan(h) || h > 100 || h < 0) h = 0.0;
  json += "\"sicaklik\":" + String(t, 1) + ",";
  json += "\"nem\":" + String(h, 1) + ",";
  
  float basinc = bmp.readPressure() / 100.0; 
  float rakim = bmp.readAltitude(103350); 
  if (isnan(basinc)) basinc = 0.0;
  if (isnan(rakim)) rakim = 0.0;
  json += "\"basinc\":" + String(basinc, 1) + ",";
  json += "\"rakim\":" + String(rakim, 0) + ",";
  
  long gaz_toplam = 0;
  for(int i = 0; i < 3; i++) {
    gaz_toplam += analogRead(mqPin);
    delayMicroseconds(50);
  }
  int gas = gaz_toplam / 3;
  json += "\"gaz\":" + String(gas) + ",";
  
  sensors_event_t event;
  accel.getEvent(&event);
  json += "\"ax\":" + String(event.acceleration.x, 2) + ",";
  json += "\"ay\":" + String(event.acceleration.y, 2) + ",";
  json += "\"az\":" + String(event.acceleration.z, 2) + ",";
  
  float gps_rakim = 0.0;
  if (gps.location.isValid()) {
    json += "\"enlem\":" + String(gps.location.lat(), 6) + ",";
    json += "\"boylam\":" + String(gps.location.lng(), 6) + ",";
    if (gps.altitude.isValid()) {
      gps_rakim = gps.altitude.meters();
    }
  } else {
    json += "\"enlem\":0.0,";
    json += "\"boylam\":0.0,";
  }
  json += "\"gps_rakim\":" + String(gps_rakim, 1) + ",";
  json += "\"hiz\":" + String(gps.speed.kmph(), 1) + ",";
  json += "\"uydu\":" + String(gps.satellites.value()) + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";
  
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  
  Wire.begin(); 
  hdc1080.begin(0x40); 
  accel.begin();
  accel.setRange(ADXL345_RANGE_4_G);
  bmp.begin();
  
  delay(500);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Wi-Fi Bağlantısı Kuruluyor");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\n✅ Wi-Fi Bağlantısı Başarılı!");
  Serial.print("İstasyon IP Adresi: ");
  Serial.println(WiFi.localIP()); 
  
  server.on("/",         handleRoot);
  server.on("/api/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
  
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
}