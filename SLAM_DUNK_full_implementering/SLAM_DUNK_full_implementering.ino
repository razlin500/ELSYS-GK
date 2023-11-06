


#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

//Bibliotek for å styre URL
#include <ESPmDNS.h>

#include <WiFi.h>
#include "SPIFFS.h"


const char* ssid = "DESKTOP-MJ9LUCK";
const char* password = "87654321";

const char* mdnsHostname = "SLAMDUNK";

//Deklarerer funksjon for å regne ut gjennomsnittsverdi på array
int average_value(int inputArray[], int inputArraySize);


//GPIO tilhørende turbiditetssensor
const int turbiditet_pin = 17;

//GPIO tilhørende relé
const int rele_pin = 16;

//Definerer array med gjennomsnittsverdi på pumpeavlesninger. Alle er 8000 for at motoren ikke skal begynne.
int average_read[10] = {8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000};

int sensorValue;
const int threshold = 5000;

// Initialize the variable as false
bool activate_pump = false;  

AsyncWebServer server(80);

String readparticles() {
  // Read particles
  float p = analogRead(turbiditet_pin);
  
  if (isnan(p)) {    
    Serial.println("Failed to read particles!");
    return "";
  }
  else {
    return String(p);
  }
}

String readavparticles() {
  // Read particles
  float ap = average_value(average_read, 10);
  
  if (isnan(ap)) {    
    Serial.println("Failed to read array!");
    return "";
  }
  else {
    return String(ap);
  }
}

void setup() {
  //Starter Serial
  Serial.begin(115200);
  pinMode(turbiditet_pin, INPUT);
  pinMode(rele_pin, OUTPUT);

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialization failed.");
    return;
  }

  // Connect to the WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  if (!MDNS.begin(mdnsHostname)) {
    Serial.println("Error setting up mDNS.");
  } else {
    Serial.println("mDNS responder started");
    Serial.print("URL:");
    Serial.println(mdnsHostname);
  }
  
  //IP address of website
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  

  // Serve the HTML file from SPIFFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    File file = SPIFFS.open("/index.html", "r");
    if (file) {
      request->send(SPIFFS, "/index.html", "text/html");
      file.close();
      Serial.println("Web page served successfully.");
    } else {
      request->send(404, "text/plain", "File not found");
      Serial.println("Failed to serve web page.");
    }
  });

  // Route to load main.css file
  server.on("/main.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/main.css", "text/css");
  });

  // Route to load about.html file
  server.on("/about.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/about.html", "text/html");
  });
  
  // Route to load favicon file
  server.on("/SLAMDUNK_fav.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/SLAMDUNK_fav.png", "image/png");
  });

  // Route to load logo file
  server.on("/SLAMDUNK_logo", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/SLAMDUNK_logo.png", "image/png");
  });

  //Her kan vi legge inn et bilde av gruppa!
  // Route to load group picture
  server.on("/gruppebilde", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/gruppebilde.png", "image/png");
  });

  // Handle /particles request
  server.on("/particles", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readparticles().c_str());
  });

  // Handle /av_particles request
  server.on("/av_particles", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readavparticles().c_str());
  });

  // Handle /activate_pump request
  server.on("/activate_pump", HTTP_GET, [](AsyncWebServerRequest *request) {
    activate_pump = !activate_pump;
    Serial.println("GET request mottatt");
    request->send_P(200, "text/plain", "Successfull activation");
  });

  server.begin();
}
void loop() {
  // read the input on analog pin 17:
  int sensorValue = analogRead(turbiditet_pin);
  
  // print out the value you read:
  Serial.print(sensorValue);
 
  //Flytter alle verdier en plass til venstre for hver gang
  for(int i = 0; i < 9; i++) {
    average_read[i] = average_read[i+1];
  }
  //Endrer siste verdi i arrayet til sist leste verdi
  average_read[9] = sensorValue;

  //Sjekker om gjennomsnittsverdien i arrayet er under 5000, starter pumpe hvis sant
  if (activate_pump) {
    digitalWrite(rele_pin, HIGH);
    delay(5000);
    activate_pump = !activate_pump;
  }
  else if (average_value(average_read, 10) < threshold) {
    Serial.println("Pumpe på");
    digitalWrite(rele_pin, HIGH);
  }
  else{
    Serial.println("Pumpe av");
    digitalWrite(rele_pin, LOW);
  }
  Serial.print(" \n");
  delay(1000);
}

//Funksjon for å regne ut gjennomsnittsverdi
int average_value(int inputArray[], int inputArraySize) {
  int temp_avg = 0;
  for(int i = 0; i < inputArraySize; i++) {
    temp_avg += inputArray[i];
  }
  return temp_avg/10;
}
