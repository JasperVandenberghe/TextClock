#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <FastLED.h>
#include <time.h>
#include <vector>
#include <ArduinoOTA.h>

// Dim lights between 22 andd 8
#define BRIGHTNESS_DAY 90
#define BRIGHTNESS_NIGHT 40
#define HOUR_DAY  8
#define HOUR_NIGHT 22
#define BRIGHTNESS_STEPS 10

#define NUM_LEDS 94
#define LED_PIN 5
#define COLOR_INTERPOLATE_STEPS 40
#define COLOR_INTERPOLATE_MILLIS 100
#define LED_UPDATE_SECONDS 5

/* Configuration of NTP */
#define MY_NTP_SERVER "be.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   

#define TEXT_HET_IS       0
#define TEXT_HOUR_EEN     1
#define TEXT_HOUR_TWEE    2
#define TEXT_HOUR_DRIE    3
#define TEXT_HOUR_VIER    4
#define TEXT_HOUR_VIJF    5
#define TEXT_HOUR_ZES     6
#define TEXT_HOUR_ZEVEN   7
#define TEXT_HOUR_ACHT    8
#define TEXT_HOUR_NEGEN   9
#define TEXT_HOUR_TIEN    10
#define TEXT_HOUR_ELF     11
#define TEXT_HOUR_TWAALF  12
#define TEXT_OVER         13
#define TEXT_VOOR         14
#define TEXT_MINUTE_VIJF  15
#define TEXT_MINUTE_TIEN  16
#define TEXT_MINUTE_KWART 17
#define TEXT_MINUTE_HALF  18
#define TEXT_UUR          19

CRGB leds[NUM_LEDS];              // Array containing the LEDs
bool ledsActive[NUM_LEDS];        // Array containing active/disabled status of all LEDs
uint16_t timerColor = 50;         // Update color every 50ms, 40 steps -> 2s to change color
uint8_t colorInterpolateStep = 1; // Variable holding the current step for colour interpolation
uint8_t startRed, startGreen, startBlue;
uint8_t targetRed, targetGreen, targetBlue;
uint8_t currentRed = 125, currentGreen = 125, currentBlue = 0;
bool updateColors = false;

time_t now;                         // this is the epoch
tm tm;                              // the structure tm holds time information in a more convient way

std::vector<std::vector<int>> ledsByWord = {
  {84, 85, 86, 89, 90},     // Het is
  {4, 5, 6},                // uur Een
  {47, 48, 49, 50},         // uur Twee
  {40, 41, 42, 43},         // uur Drie
  {20, 21, 22, 23},         // uur Vier
  {7, 8, 9, 10},            // uur Vijf
  {51, 52, 53},             // uur Zes
  {25, 26, 27, 28, 29},     // uur Zeven
  {44, 45, 46, 47},         // uur Acht
  {29, 30, 31, 32, 33},     // uur Negen
  {34, 35, 36, 37},         // uur Tien
  {38, 39, 40},             // uur Elf
  {14, 15, 16, 17, 18, 19}, // uur Twaalf
  {70, 71, 72, 73},         // over
  {60, 61, 62, 63},         // voor
  {79, 80, 81, 82},         // minuten Vijf
  {75, 76, 77, 78},         // minuten Tien
  {64, 65, 66, 67, 68},     // minuten Kwart
  {55, 56, 57, 58},         // minuten Half
  {11, 12, 13}              // uur
};

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

void setup() {
  Serial.begin(115200);

  // Initialize LEDs
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  // Set first LED in the left to red show sign of life
  leds[3] = CRGB::Red;
  FastLED.show();

  configTime(MY_TZ, MY_NTP_SERVER); // --> Here is the IMPORTANT ONE LINER needed in your sketch!

  // Determine time to set initial brightness
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
  if (tm.tm_hour >= HOUR_DAY && tm.tm_hour < HOUR_NIGHT) {
    FastLED.setBrightness(BRIGHTNESS_DAY);
  } else {
    FastLED.setBrightness(BRIGHTNESS_NIGHT);
  }

  // Second LED on the left to red
  leds[2] = CRGB::Red;
  FastLED.show();

  // Initialize wifi connection & web server
  // WiFiManager
  WiFiManager wifiManager;
  wifiManager.autoConnect("TextClock");

  // Third LED on the left to red
  leds[1] = CRGB::Red;
  FastLED.show();

  // Once connected
  Serial.println("Connected.");
  server.begin();

  // All bottom LEDs to red
  leds[0] = CRGB::Red;
  FastLED.show();

  // Setup for OTA
  ArduinoOTA.setHostname("textclock");
  ArduinoOTA.onStart([](){
    Serial.println("Arduino OTA Start");
    // Set LED 4 till 0 to colors to indicate update
    for(int i = 4; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;  
    }
    for(int i = 0; i < 4; i++) {
      leds[i] = CRGB::Blue;  
    }

    FastLED.show();
  });
  ArduinoOTA.onEnd([](){
    Serial.println("Arduino OTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void loop() {
  // Power efficiency
  delay(2); // https://hackaday.com/2022/10/28/esp8266-web-server-saves-60-power-with-a-1-ms-delay/
  
  ArduinoOTA.handle(); 
  
  // Listen for incoming clients
  WiFiClient client = server.available();

  // ********************** Start of everything client related **********************
  if (client) {
    // Client connected
    Serial.println("New client connected.");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        // Check if there's data to read from client
        char c = client.read(); // Read a byte
        Serial.write(c);        // Print data
        header += c;

        if (c == '\n') {        // If byte is a newline char
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\">");
            client.println("<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js\"></script>");
            client.println("</head><body><div class=\"container\"><div class=\"row\"><h1>ESP Color Picker</h1></div>");
            client.println("<a class=\"btn btn-primary btn-lg\" href=\"#\" id=\"change_color\" role=\"button\">Change Color</a> ");
            client.println("<input class=\"jscolor {onFineChange:'update(this)'}\" id=\"rgb\"></div>");
            client.println("<script>function update(picker) {document.getElementById('rgb').innerHTML = Math.round(picker.rgb[0]) + ', ' +  Math.round(picker.rgb[1]) + ', ' + Math.round(picker.rgb[2]);");
            client.println("document.getElementById(\"change_color\").href=\"?r\" + Math.round(picker.rgb[0]) + \"g\" +  Math.round(picker.rgb[1]) + \"b\" + Math.round(picker.rgb[2]) + \"&\";}</script></body></html>");
            // The HTTP response ends with another blank line
            client.println();

            // Request sample: /?r201g32b255&
            // Red = 201 | Green = 32 | Blue = 255
            if (header.indexOf("GET /?r") >= 0) {
              int pos1 = header.indexOf('r');
              int pos2 = header.indexOf('g');
              int pos3 = header.indexOf('b');
              int pos4 = header.indexOf('&');
              targetRed = header.substring(pos1 + 1, pos2).toInt();
              targetGreen = header.substring(pos2 + 1, pos3).toInt();
              targetBlue = header.substring(pos3 + 1, pos4).toInt();

              // Print colors
              Serial.print("Red: ");
              Serial.println(targetRed);
              Serial.print("Green: ");
              Serial.println(targetGreen);
              Serial.print("Blue: ");
              Serial.println(targetBlue);

              // Check if actual changes are selected
              if (targetRed != currentRed || targetGreen != currentGreen || targetBlue != currentBlue) {
                // Update the colors           
                startRed = currentRed;
                startGreen = currentGreen;
                startBlue = currentBlue;
                colorInterpolateStep = 1;
                updateColors = true;
              }
            }

            // Break out of the while loop
            break;
          } else {    // If you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') { // If you got anything else than a carriage return
          currentLine += c;     // add it to the end of the currentLine
        }
      }
    }

    // Clear header variable
    header = "";

    // Close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
  // ********************** End of everything client related **********************

  // ********************** Start of everything LED related **********************

  // Update the output leds every 5 seconds 
 
  EVERY_N_SECONDS(LED_UPDATE_SECONDS) { 
    updateActiveLeds();
    showLeds();
  }


  // Update brightness every minute (time is updated every 5s)
  EVERY_N_MINUTES(1) {
    checkBrightness();
  }

  if(updateColors) {
    // Interpolate from start color to target color
    EVERY_N_MILLISECONDS(COLOR_INTERPOLATE_MILLIS){
      interpolateColors();
      showLeds();
    }
  }
  // End of everything LED related
}

void checkBrightness(){
  if (tm.tm_hour == HOUR_DAY && tm.tm_min < BRIGHTNESS_STEPS) {
      // Day -> increase brightness
      // In BRIGHTNESS_STEPS minutes, go from BRIGHTNESS_NIGHT to BRIGHTNESS_DAY
      FastLED.setBrightness(map(tm.tm_min, 0, BRIGHTNESS_STEPS - 1, BRIGHTNESS_NIGHT, BRIGHTNESS_DAY));
    } else if (tm.tm_hour == HOUR_NIGHT && tm.tm_min < BRIGHTNESS_STEPS) {
      // Night -> decrease brightness
      FastLED.setBrightness(map(tm.tm_min, 0, BRIGHTNESS_STEPS - 1, BRIGHTNESS_DAY, BRIGHTNESS_NIGHT));
    }
}

void interpolateColors() {
  // Interpolate the color
  currentRed = map(colorInterpolateStep, 1, COLOR_INTERPOLATE_STEPS, startRed, targetRed);
  currentGreen = map(colorInterpolateStep, 1, COLOR_INTERPOLATE_STEPS, startGreen, targetGreen);
  currentBlue = map(colorInterpolateStep, 1, COLOR_INTERPOLATE_STEPS, startBlue, targetBlue);

  Serial.println(currentRed);
  Serial.println(currentGreen);
  Serial.println(currentBlue);

  // If we are at step 40, interpolation has been completed
  if(colorInterpolateStep == COLOR_INTERPOLATE_STEPS) {
    updateColors = false;
    colorInterpolateStep = 1;
  } else {
    colorInterpolateStep++;
  }
}

void updateActiveLeds() {
    time(&now);                       // read the current time
    localtime_r(&now, &tm);           // update the structure tm with the current time

    // Hours since midnight
    // Vanaf "kwart na" springt de tekst naar het volgende uur -> "10 voor half 2"
    // --> Als minuten >= 20 --> gebruik het VOLGENDE uur (%12)
    uint8_t hour = tm.tm_hour % 12;
    
    if (tm.tm_min >= 20)
      hour = (hour + 1) % 12;
    if(hour == 0)
      hour = 12;

    for(int i = 0; i < NUM_LEDS; i++) {
      ledsActive[i] = false;  
    }

    // Set hour LEDs
    for(int i: ledsByWord[hour]){ledsActive[i] = true;}      

    for(int i: ledsByWord[TEXT_HET_IS]){ledsActive[i] = true;}
    // Minuten per 5
    switch(tm.tm_min / 5) {
      case 0: // Minute 0 - 4
        for(int i: ledsByWord[TEXT_UUR]){ledsActive[i] = true;}
        break;
      case 1: // Minute 5 - 9
        for(int i: ledsByWord[TEXT_MINUTE_VIJF]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_OVER]){ledsActive[i] = true;}
        break;
      case 2: // Minute 10 - 14
        for(int i: ledsByWord[TEXT_MINUTE_TIEN]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_OVER]){ledsActive[i] = true;}
        break;
      case 3: // Minute 15 - 19
        for(int i: ledsByWord[TEXT_MINUTE_KWART]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_OVER]){ledsActive[i] = true;}
        break;
      case 4: // Minute 20 - 24
        // " tien voor half xx "
        for(int i: ledsByWord[TEXT_MINUTE_TIEN]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_VOOR]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_MINUTE_HALF]){ledsActive[i] = true;}
        break;
      case 5: // Minute 25 - 29
        // " vijf voor half xx "
        for(int i: ledsByWord[TEXT_MINUTE_VIJF]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_VOOR]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_MINUTE_HALF]){ledsActive[i] = true;}
        break;
      case 6: // Minute 30 - 34
        for(int i: ledsByWord[TEXT_MINUTE_HALF]){ledsActive[i] = true;}
        break;
      case 7: // Minute 35 - 39
        // " vijf over half xx "
        for(int i: ledsByWord[TEXT_MINUTE_VIJF]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_OVER]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_MINUTE_HALF]){ledsActive[i] = true;}
        break;
      case 8: // Minute 40 - 44
        // " tien over half xx "
        for(int i: ledsByWord[TEXT_MINUTE_TIEN]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_OVER]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_MINUTE_HALF]){ledsActive[i] = true;}
        break;
      case 9: // Minute 45 - 49
        for(int i: ledsByWord[TEXT_MINUTE_KWART]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_VOOR]){ledsActive[i] = true;}
        break;
      case 10: // Minute 50 - 54
        for(int i: ledsByWord[TEXT_MINUTE_TIEN]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_VOOR]){ledsActive[i] = true;}
        break;
      case 11: // Minute 55 - 59
        for(int i: ledsByWord[TEXT_MINUTE_VIJF]){ledsActive[i] = true;}
        for(int i: ledsByWord[TEXT_VOOR]){ledsActive[i] = true;}
        break;
    }

    // Update the 4 minute counters
    for (int i = 0; i < (tm.tm_min % 5); i++) {
      ledsActive[3 - i] = true;  
    }
}


void showLeds() { 
  for(int i = 0; i < NUM_LEDS; i++) {
    if(ledsActive[i]) {
      //leds[i] = CRGB::Blue;
      leds[i] = CRGB(currentRed, currentGreen, currentBlue);  
    } else {
      leds[i] = CRGB::Black;
    }   
  }
  
  FastLED.show();  
}
