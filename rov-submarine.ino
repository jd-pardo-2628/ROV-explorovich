#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ===========================================
// CONFIGURACIÓN DE PINES TB6612FNG
// ===========================================

// TB6612FNG #1 - Motores Derecha e Izquierda
#define TB1_AIN1 32  // Motor Derecha - Dirección 1
#define TB1_AIN2 33  // Motor Derecha - Dirección 2
#define TB1_PWMA 25  // Motor Derecha - PWM
#define TB1_BIN1 26  // Motor Izquierda - Dirección 1
#define TB1_BIN2 27  // Motor Izquierda - Dirección 2
#define TB1_PWMB 4  // Motor Izquierda - PWM
#define TB1_STBY 23  // Standby TB6612FNG #1

// TB6612FNG #2 - Motor Adelante/Atrás
#define TB2_AIN1 19  // Motor Adelante/Atrás - Dirección 1
#define TB2_AIN2 18  // Motor Adelante/Atrás - Dirección 2
#define TB2_PWMA 5   // Motor Adelante/Atrás - PWM
#define TB2_STBY 21  // Standby TB6612FNG #2

// ===========================================
// CONFIGURACIÓN WiFi
// ===========================================
const char* ssid = "ROVSUBMARINE";
const char* password = "1234567890";
WebServer server(80);

// ===========================================
// VARIABLES DE CONTROL
// ===========================================
int motorDerecha = 0;   // -255 a 255
int motorIzquierda = 0; // -255 a 255
int motorAdelante = 0;  // -255 a 255

// ===========================================
// FUNCIONES DE CONTROL DE MOTORES
// ===========================================

void setupMotors() {
  // Configurar pines como salidas
  pinMode(TB1_AIN1, OUTPUT);
  pinMode(TB1_AIN2, OUTPUT);
  pinMode(TB1_PWMA, OUTPUT);
  pinMode(TB1_BIN1, OUTPUT);
  pinMode(TB1_BIN2, OUTPUT);
  pinMode(TB1_PWMB, OUTPUT);
  pinMode(TB1_STBY, OUTPUT);
  pinMode(TB2_AIN1, OUTPUT);
  pinMode(TB2_AIN2, OUTPUT);
  pinMode(TB2_PWMA, OUTPUT);
  pinMode(TB2_STBY, OUTPUT);

  // Activar controladores
  digitalWrite(TB1_STBY, HIGH);
  digitalWrite(TB2_STBY, HIGH);

  // Detener todos los motores al inicio
  stopAllMotors();
}

void controlMotorDerecha(int velocidad) {
  motorDerecha = constrain(velocidad, -255, 255);

  if (velocidad > 0) {
    digitalWrite(TB1_AIN1, HIGH);
    digitalWrite(TB1_AIN2, LOW);
    analogWrite(TB1_PWMA, velocidad);
  } else if (velocidad < 0) {
    digitalWrite(TB1_AIN1, LOW);
    digitalWrite(TB1_AIN2, HIGH);
    analogWrite(TB1_PWMA, abs(velocidad));
  } else {
    digitalWrite(TB1_AIN1, LOW);
    digitalWrite(TB1_AIN2, LOW);
    analogWrite(TB1_PWMA, 0);
  }
}

void controlMotorIzquierda(int velocidad) {
  motorIzquierda = constrain(velocidad, -255, 255);

  if (velocidad > 0) {
    digitalWrite(TB1_BIN1, HIGH);
    digitalWrite(TB1_BIN2, LOW);
    analogWrite(TB1_PWMB, velocidad);
  } else if (velocidad < 0) {
    digitalWrite(TB1_BIN1, LOW);
    digitalWrite(TB1_BIN2, HIGH);
    analogWrite(TB1_PWMB, abs(velocidad));
  } else {
    digitalWrite(TB1_BIN1, LOW);
    digitalWrite(TB1_BIN2, LOW);
    analogWrite(TB1_PWMB, 0);
  }
}

void controlMotorAdelante(int velocidad) {
  motorAdelante = constrain(velocidad, -255, 255);

  if (velocidad > 0) {
    digitalWrite(TB2_AIN1, HIGH);
    digitalWrite(TB2_AIN2, LOW);
    analogWrite(TB2_PWMA, velocidad);
  } else if (velocidad < 0) {
    digitalWrite(TB2_AIN1, LOW);
    digitalWrite(TB2_AIN2, HIGH);
    analogWrite(TB2_PWMA, abs(velocidad));
  } else {
    digitalWrite(TB2_AIN1, LOW);
    digitalWrite(TB2_AIN2, LOW);
    analogWrite(TB2_PWMA, 0);
  }
}

void stopAllMotors() {
  controlMotorDerecha(0);
  controlMotorIzquierda(0);
  controlMotorAdelante(0);
}

// ===========================================
// MANEJO DE REQUESTS HTTP
// ===========================================

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Control de Motores ESP32</title>
  <script src="https://cdn.tailwindcss.com"></script>
  <style>
    body {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      font-family: 'Arial', sans-serif;
    }
    .joystick-container {
      width: 150px;
      height: 150px;
      background: #2d3748;
      border-radius: 50%;
      position: relative;
      margin: 20px auto;
      border: 4px solid #4a5568;
      box-shadow: inset 0 0 20px rgba(0,0,0,0.3);
    }
    .joystick-knob {
      width: 60px;
      height: 60px;
      background: linear-gradient(145deg, #4fd1c7, #38b2ac);
      border-radius: 50%;
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      cursor: pointer;
      transition: all 0.1s;
      box-shadow: 0 4px 15px rgba(0,0,0,0.3);
    }
    .motor-button {
      transition: all 0.2s;
      box-shadow: 0 4px 15px rgba(0,0,0,0.2);
    }
    .motor-button:active {
      transform: scale(0.95);
    }
    .speed-slider {
      background: linear-gradient(90deg, #e53e3e, #38a169);
      height: 8px;
      border-radius: 4px;
    }
  </style>
</head>
<body class="min-h-screen p-4">
  <div class="max-w-md mx-auto bg-white rounded-3xl shadow-2xl overflow-hidden">
    <!-- Header -->
    <div class="bg-gradient-to-r from-purple-600 to-blue-600 p-6 text-white text-center">
      <h1 class="text-2xl font-bold mb-2">🤖 Control de Motores</h1>
      <div class="flex items-center justify-center space-x-2">
        <div class="w-3 h-3 bg-green-400 rounded-full animate-pulse"></div>
        <span class="text-sm">ESP32 Conectado</span>
      </div>
    </div>

    <div class="p-6 space-y-6">
      <!-- Control Joystick Derecha/Izquierda -->
      <div class="bg-gray-50 rounded-2xl p-4">
        <h3 class="text-lg font-semibold text-center mb-4 text-gray-700">↔️ Derecha / Izquierda</h3>
        <div class="joystick-container mx-auto" id="joystick-horizontal">
          <div class="joystick-knob" id="knob-horizontal"></div>
        </div>
        <div class="text-center text-sm text-gray-600 mt-2">
          Velocidad: <span id="speed-horizontal">0</span>
        </div>
      </div>

      <!-- Control Joystick Adelante/Atrás -->
      <div class="bg-gray-50 rounded-2xl p-4">
        <h3 class="text-lg font-semibold text-center mb-4 text-gray-700">↕️ Adelante / Atrás</h3>
        <div class="joystick-container mx-auto" id="joystick-vertical">
          <div class="joystick-knob" id="knob-vertical"></div>
        </div>
        <div class="text-center text-sm text-gray-600 mt-2">
          Velocidad: <span id="speed-vertical">0</span>
        </div>
      </div>

      <!-- Controles Rápidos -->
      <div class="bg-gray-50 rounded-2xl p-4">
        <h3 class="text-lg font-semibold text-center mb-4 text-gray-700">⚡ Controles Rápidos</h3>
        <div class="grid grid-cols-2 gap-3">
          <button onclick="emergencyStop()" class="motor-button bg-red-500 hover:bg-red-600 text-white font-bold py-3 px-4 rounded-xl">🛑 STOP</button>
          <button onclick="testMotors()" class="motor-button bg-blue-500 hover:bg-blue-600 text-white font-bold py-3 px-4 rounded-xl">🔧 Test</button>
        </div>
      </div>

      <!-- Estado de Motores -->
      <div class="bg-gray-50 rounded-2xl p-4">
        <h3 class="text-lg font-semibold text-center mb-4 text-gray-700">📊 Estado Actual</h3>
        <div class="space-y-2 text-sm">
          <div class="flex justify-between">
            <span>Motor Derecha:</span>
            <span id="status-right" class="font-mono">0</span>
          </div>
          <div class="flex justify-between">
            <span>Motor Izquierda:</span>
            <span id="status-left" class="font-mono">0</span>
          </div>
          <div class="flex justify-between">
            <span>Motor Adelante/Atrás:</span>
            <span id="status-forward" class="font-mono">0</span>
          </div>
        </div>
      </div>
    </div>
  </div>

  <script>
    // Variables globales
    let motorValues = { right: 0, left: 0, forward: 0 };

    // Configuración de joysticks
    function setupJoystick(containerId, knobId, onMove) {
      const container = document.getElementById(containerId);
      const knob = document.getElementById(knobId);
      let isDragging = false;

      function startDrag(e) {
        isDragging = true;
        e.preventDefault();
      }

      function stopDrag() {
        isDragging = false;
        knob.style.transform = 'translate(-50%, -50%)';
        onMove(0, 0);
      }

      function drag(e) {
        if (!isDragging) return;
        const rect = container.getBoundingClientRect();
        const centerX = rect.left + rect.width / 2;
        const centerY = rect.top + rect.height / 2;

        let clientX, clientY;
        if (e.touches) {
          clientX = e.touches[0].clientX;
          clientY = e.touches[0].clientY;
        } else {
          clientX = e.clientX;
          clientY = e.clientY;
        }

        const deltaX = clientX - centerX;
        const deltaY = clientY - centerY;
        const maxDistance = rect.width / 2 - 30;
        const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);

        if (distance <= maxDistance) {
          knob.style.transform = `translate(calc(-50% + ${deltaX}px), calc(-50% + ${deltaY}px))`;
          onMove(deltaX / maxDistance, deltaY / maxDistance);
        }
      }

      // Mouse & touch events
      knob.addEventListener('mousedown', startDrag);
      document.addEventListener('mousemove', drag);
      document.addEventListener('mouseup', stopDrag);
      knob.addEventListener('touchstart', startDrag);
      document.addEventListener('touchmove', drag);
      document.addEventListener('touchend', stopDrag);
    }

    // Joystick horizontal (derecha/izquierda)
    setupJoystick('joystick-horizontal', 'knob-horizontal', (x, y) => {
      const speed = Math.round(x * 255);
      motorValues.right = speed;
      motorValues.left = -speed;
      document.getElementById('speed-horizontal').textContent = Math.abs(speed);
      updateMotorStatus();
      sendMotorCommands();
    });

    // Joystick vertical (adelante/atrás)
    setupJoystick('joystick-vertical', 'knob-vertical', (x, y) => {
      const speed = Math.round(-y * 255);
      motorValues.forward = speed;
      document.getElementById('speed-vertical').textContent = Math.abs(speed);
      updateMotorStatus();
      sendMotorCommands();
    });

    // Actualizar estado visual
    function updateMotorStatus() {
      document.getElementById('status-right').textContent = motorValues.right;
      document.getElementById('status-left').textContent = motorValues.left;
      document.getElementById('status-forward').textContent = motorValues.forward;
    }

    // Enviar comandos a ESP32
    async function sendMotorCommands() {
      try {
        await fetch('/control', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(motorValues)
        });
      } catch (error) {
        console.error('Error enviando comando:', error);
      }
    }

    // Parada de emergencia
    function emergencyStop() {
      motorValues = { right: 0, left: 0, forward: 0 };
      updateMotorStatus();
      sendMotorCommands();
      document.getElementById('knob-horizontal').style.transform = 'translate(-50%, -50%)';
      document.getElementById('knob-vertical').style.transform = 'translate(-50%, -50%)';
      document.getElementById('speed-horizontal').textContent = '0';
      document.getElementById('speed-vertical').textContent = '0';
    }

    // Test de motores
    async function testMotors() {
      const testSequence = [
        { right: 100, left: 0, forward: 0 },
        { right: 0, left: 100, forward: 0 },
        { right: 0, left: 0, forward: 100 },
        { right: 0, left: 0, forward: -100 },
        { right: 0, left: 0, forward: 0 }
      ];

      for (const values of testSequence) {
        motorValues = values;
        updateMotorStatus();
        await sendMotorCommands();
        await new Promise(resolve => setTimeout(resolve, 1000));
      }
    }

    // Inicialización
    updateMotorStatus();
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleControl() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    int derecha = doc["right"];
    int izquierda = doc["left"];
    int adelante = doc["forward"];

    controlMotorDerecha(derecha);
    controlMotorIzquierda(izquierda);
    controlMotorAdelante(adelante);

    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
  }
}

void handleStatus() {
  String status = "{";
  status += "\"derecha\":" + String(motorDerecha) + ",";
  status += "\"izquierda\":" + String(motorIzquierda) + ",";
  status += "\"adelante\":" + String(motorAdelante);
  status += "}";
  server.send(200, "application/json", status);
}

// ===========================================
// SETUP Y LOOP PRINCIPAL
// ===========================================
void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando ESP32 Control de Motores...");

  // Configurar motores
  setupMotors();

  // Conectar a WiFi
WiFi.begin("ROVSUBMARINE","1234567890" );
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Configurar servidor web
  server.on("/", handleRoot);
  server.on("/control", HTTP_POST, handleControl);
  server.on("/status", handleStatus);
  server.begin();

  Serial.println("Servidor web iniciado");
  Serial.println("Abre http://" + WiFi.localIP().toString() + " en tu navegador");
}

void loop() {
  server.handleClient();
  delay(10);
}


