#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
//Bibliotek for å styre URL
#include <ESPmDNS.h>
#include <WiFi.h>
#include "SPIFFS.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCLpins)
//Skjermen er som standard koblet til GPIO 9, GPIO 8
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//Funksjonsdeklarasjon
void standard_display();
void status();
void wifi_led();
int average_value(int inputArray[], int inputArraySize);
String readparticles();
String readavparticles();

//Variabeldeklarasjon
//LED_diode til WiFi-status
const int LED_diode = 2;


const char* ssid = "DESKTOP-MJ9LUCK";
const char* password = "87654321";

const char* mdnsHostname = "SLAMDUNK";

//GPIO tilhørende turbiditetssensor
const int turbiditet_pin = 17;

//GPIO tilhørende relé
const int rele_pin = 16;

//Definerer array med gjennomsnittsverdi på pumpeavlesninger. Alle er 8000 for at motoren ikke skal begynne.
int average_read[10] = {8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000, 8000};

//Maksimumsverdi for aktivering av pumpen
const int threshold = 5000;

int sensorValue;

// Initialize the variable as false
bool activate_pump = false;  

AsyncWebServer server(80);


void setup() {
  //Starter Serial
  Serial.begin(115200);
  //Starter opp skjermen
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  for(;;);
  }
  //Initialiserer pins
  pinMode(turbiditet_pin, INPUT);
  pinMode(rele_pin, OUTPUT);
  pinMode(LED_diode, OUTPUT);

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialization failed.");
    return;
  }

  // Connect to the WiFi network
  WiFi.begin(ssid, password);
  standard_display();
  display.print("Connection to WiFi")
  while (WiFi.status() != WL_CONNECTED) {
    wifi_led();
    delay(100);
    display.print(".");
    Serial.println("Connection to WiFi...");
  }
  standard_display();
  display.print("Connected to WiFi");
  display.println();
  delay(1000);
  
  /*while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");*/

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
  display.print("Server has been initiated");
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
    digitalWrite(rele_pin, LOW);
    status();
    delay(5000);
    activate_pump = !activate_pump;
  }
  else if (average_value(average_read, 10) < threshold) {
    Serial.println("Pumpe på");
    digitalWrite(rele_pin, LOW);
  }
  else{
    Serial.println("Pumpe av");
    digitalWrite(rele_pin, HIGH);
  }
  Serial.print(" \n");
  wifi_led();
  status();
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


//Nullstiller displayet for å skrive ny tekst
void standard_display() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 8);
}

//Led slåes av/på basert på om WiFi er koblet til eller ikke
void wifi_led() {
  if(WiFi.status == WL_CONNECTED) {
    digitalWrite(LED_diode, HIGH)
  }
  else {
    digitalWrite(LED_diode, LOW)
  }
}


//Printer statusen til systemet på OLED-skjermen
void status() {
  standard_display()
  display.print("Pumpen er: ");
  if(activate_pump) {
    display.print("på");
  }
  display.print("av");
  display.println();
  display.print("Gjennomsnittsmåling: ")
  display.print(average_value(average_read, 10));
  display.display();
}

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
