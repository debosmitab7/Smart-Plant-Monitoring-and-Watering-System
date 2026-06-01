#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_TOKEN"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

//OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#include <FluxGarage_RoboEyes.h>
RoboEyes<Adafruit_SSD1306> roboEyes(display); 
//DHT11
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
//Soil Sensor
#define SOIL_PIN 34
//Relay
#define RELAY_PIN 15
//Ultrasonic Sensor
#define TRIG_PIN 5
#define ECHO_PIN 18
//Buzzer
#define BUZZER_PIN 19
//IR sensor
#define IR_PIN 27
//Touch sensor
#define TOUCH_PIN 14
// Threshold for dry soil
int threshold = 2000;
float tankHeight = 20.0;

// Global variables
int soilValue;
float temperature;
float humidity;
float distance;
float waterLevel;

int expression = 0;

bool touchState = false;
bool showEyes = false;

unsigned long eyesStartTime = 0;

const unsigned long eyesDuration = 5000;

bool lastIRState = false;

char ssid[] = "YOUR_WIFI";
char pass[] = "YOUR_PASSWORD";
BlynkTimer timer;

// Function to read soil moisture
void readSoilMoisture()
{
  soilValue = analogRead(SOIL_PIN);
}

// Function to read DHT11
void readDHTSensor() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity))
  {
    Serial.println("DHT Error");
    return;
  }
}

// Function to read Ultrasonic Sensor
void readWaterLevel()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);

  distance = duration * 0.034 / 2;

  waterLevel = tankHeight - distance;

}

// Function to control pump
void controlPump() {
  if (waterLevel <= 14.70)
  {
    digitalWrite(RELAY_PIN, HIGH); // Pump OFF
    Serial.println("Tank LOW");
    // Buzzer ON
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else if (soilValue > threshold)
  {
    digitalWrite(RELAY_PIN, LOW); // Pump ON
    digitalWrite(BUZZER_PIN, LOW);
  }
  else
  {
    digitalWrite(RELAY_PIN, HIGH); // Pump OFF
    digitalWrite(BUZZER_PIN, LOW);
  }
}


// Function to display data on OLED
void displayData()
{
  bool personDetected = digitalRead(IR_PIN) == LOW;
  // IR DETECTION
  if (personDetected && !lastIRState)
  {
    showEyes = true;
    eyesStartTime = millis();
  }
  lastIRState = personDetected;
  // TOUCH SENSOR
  if (digitalRead(TOUCH_PIN) == HIGH && !touchState)
  {
    touchState = true;
    showEyes = true;
    eyesStartTime = millis();
    expression++;
    if (expression > 3)
    {
      expression = 0;
    }
    switch(expression)
    {
      case 0:
        roboEyes.setMood(DEFAULT);
        break;

      case 1:
        roboEyes.setMood(HAPPY);
        roboEyes.anim_laugh();
        break;

      case 2:
        roboEyes.setMood(ANGRY);
        roboEyes.anim_confused();
        break;

      case 3:
        roboEyes.setMood(TIRED);
        break;
    }
  }

  if (digitalRead(TOUCH_PIN) == LOW)
  {
    touchState = false;
  }
  // SHOW EYES
  if (showEyes)
  {
    roboEyes.update();
    if (millis() - eyesStartTime >= eyesDuration)
    {
      showEyes = false;
    }
  }
  // SHOW SENSOR READINGS
  else
  {
    display.clearDisplay();

    display.setTextSize(1);

    display.setTextColor(WHITE);

    display.setCursor(0,0);
    display.print("Soil: ");
    display.println(soilValue);

    display.setCursor(0,12);
    display.print("Temp: ");
    display.print(temperature);
    display.println(" C");

    display.setCursor(0,24);
    display.print("Humidity: ");
    display.print(humidity);
    display.println("%");

    display.setCursor(0,36);
    display.print("Water: ");
    display.print(waterLevel);
    display.println(" cm");

    // Pump status
    display.setCursor(0,48);
    if (soilValue > threshold)
    {
      display.println("Pump: ON");
    }
    else
    {
      display.println("Pump: OFF");
    }
    display.display();
  }
}

void sendSensorData()
{
  Blynk.virtualWrite(V0, soilValue);

  Blynk.virtualWrite(V1, temperature);

  Blynk.virtualWrite(V2, humidity);

  Blynk.virtualWrite(V3, waterLevel);
  // Pump Status
  if (soilValue > threshold)
  {
    Blynk.virtualWrite(V4, "ON");
  }
  else
  {
    Blynk.virtualWrite(V4, "OFF");
  }
}

BLYNK_WRITE(V5)
{
  int value = param.asInt();

  if (value == 1)
  {
    digitalWrite(RELAY_PIN, LOW);
  }
  else
  {
    digitalWrite(RELAY_PIN, HIGH);
  }
}

// Setup
void setup() {
  //Initialise Serial Print
  Serial.begin(115200);

  //Initialise Relay
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  // Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // IR Sensor
  pinMode(IR_PIN, INPUT);

  // Touch Sensor
  pinMode(TOUCH_PIN, INPUT);

  //Initialise DHT
  dht.begin();

  Wire.begin();
  //Initialise OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED not found");
    while (1);
  }
  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 100);

  roboEyes.setPosition(DEFAULT);

  roboEyes.setMood(DEFAULT);

  roboEyes.open();
  Serial.print("Connecting to WiFi");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(2000L, sendSensorData);
  Serial.println("");

  Serial.println("WiFi Connected");

  Serial.print("IP Address: ");

  Serial.println(WiFi.localIP());

  Serial.println("System Started");
}

// Loop
void loop()
{
  Blynk.run();

  timer.run();
  readSoilMoisture();
  
  readDHTSensor();

  readWaterLevel();

  controlPump();
  
  displayData();

 delay(20);
}