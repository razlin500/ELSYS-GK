#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YourNetworkSSID";
const char* password = "YourNetworkPassword";

WebServer server(80);

float particlesValue = 0.0;  // Replace with your actual particle data
float avParticlesValue = 0.0;  // Replace with your actual av_particles data

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Define the endpoints for "particles" and "av_particles"
  server.on("/particles", HTTP_GET, [](){
    server.send(200, "text/plain", String(particlesValue));
  });

  server.on("/av_particles", HTTP_GET, [](){
    server.send(200, "text/plain", String(avParticlesValue));
  });

  server.begin();
}

void loop() {
  server.handleClient();
  // Your sensor data retrieval code here (update particlesValue and avParticlesValue)
server.on("/", HTTP_GET, [](){
  File file = SPIFFS.open("/index.html", "r");
  if (file) {
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "File not found");
  }
});
}
