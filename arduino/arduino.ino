
//#include "ThingSpeak.h"
//#include "secrets.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <math.h>
#include <Wire.h>
#include "DHT.h"
#include "SparkFunCCS811.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <Arduino_JSON.h>


#define DHTPIN 2         // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11    // DHT 11
#define CCS811_ADDR 0x5A //Default I2C Address

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET LED_BUILTIN

char ssid[] = "DIGI-2367";  // your network SSID (name) 
char pass[] = "";   // your network password

WiFiClient  client;

unsigned long myChannelNumber = 1049952;
const char * myWriteAPIKey = "0HKBHZ7CLZTMUPD3";

//initializare componente
CCS811 ccs811(CCS811_ADDR);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
void setup()
{
    Serial.begin(115200);
    Wire.begin(); //Inialize I2C Hardware
    //initializare display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    { // Address 0x3C for 128x32
        Serial.println(F("SSD1306 allocation failed"));
        while (1)
            ; // Don't proceed, loop forever
    }

    display.clearDisplay(); //curatare buffer display

    //intitializare senzor DHT11
    dht.begin();

    //initializare senzor CSS811
    delay(1000);
    if (ccs811.begin() == false)
    {
        Serial.println("CCS811 error. Please check wiring. Freezing...");
        while (1)
            ;
    }
    WiFi.mode(WIFI_STA); 
//    ThingSpeak.begin(client);  // Initialize ThingSpeak
     if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.forceSleepWake();
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }
}

typedef struct
{
    float total;
    float count;
} avg;

void loop() {
  
    delay(1000);
    
    static avg tvoc_avg = {0, 0};
    static avg co2_avg = {0, 0};
    static avg hum_avg = {0, 0};
    static avg temp_avg = {0, 0};

    float tvoc_final;
    float co2_final;
    float hum_final;
    float temp_final;

    float h=0;
    float t=0;
    
    int tvoc_level;
    int co2_level;
   
   // Connect or reconnect to WiFi
  
    if(dht.readHumidity() && dht.readTemperature()){
      float h = dht.readHumidity(); 
      float t = dht.readTemperature();  
  
      hum_avg.total += h;
      temp_avg.total +=t;
      hum_avg.count++;
      temp_avg.count++;
    }
    if (ccs811.dataAvailable())
    {
        ccs811.readAlgorithmResults();
        if(t!=0 &&h!=0)
          ccs811.setEnvironmentalData(h, t); // calibram senzorul CCS811 in fucntie de temperatura si umidiate
        float tvoc = ccs811.getTVOC();
        float co2 = ccs811.getCO2();
        tvoc_avg.total += tvoc;
        co2_avg.total += co2;
        tvoc_avg.count++;
        co2_avg.count++;
    }
    static long timp_es = 0;
    static long timp_propus = 15;         //dorim media masuratorii realizate in 10 secunde
    unsigned long timp = millis() / 1000; //timpul de la start up in secunde
    if (timp-timp_es> timp_propus)
    {
        //timp_es = timp;//pentru rularea cu alimentrea continua, fara DeepSleep
        //if (tvoc_avg.count > 0 && co2_avg.count > 0)
        {
            float tvoc_final = tvoc_avg.total / tvoc_avg.count;
            float co2_final = co2_avg.total / co2_avg.count;
            float hum_final = hum_avg.total / hum_avg.count;
            float temp_final = temp_avg.total / temp_avg.count;

            //nivele tvoc
            if(tvoc_final>=0 && tvoc_final<65)
              tvoc_level = 1;
            if(tvoc_final>=65 && tvoc_final<220)
              tvoc_level = 2;
            if(tvoc_final>=220 && tvoc_final<660)
              tvoc_level = 3;
            if(tvoc_final>=660 && tvoc_final<2200)
              tvoc_level = 4;
            if(tvoc_final>=2200 && tvoc_final<5500)
              tvoc_level = 5;

          if(co2_final>=350 && co2_final<450)
            co2_level = 1;
          if(co2_final>=450 && co2_final<600)
            co2_level = 2;
          if(co2_final>=600 && co2_final<1000)
            co2_level = 3;
          if(co2_final>1000 && co2_final<2500)
            co2_level = 4;
          if(co2_final>=2500 && co2_final<5000)
            co2_level = 5;
            
            
              
            Serial.print("Tvoc: ");
            Serial.print(tvoc_final);
            Serial.println();
            
            Serial.print("CO2 :");
            Serial.print(co2_final);
            Serial.println();

            Serial.print("Hum :");
            Serial.print(hum_final);
            Serial.println();

            Serial.print("Temp :");
            Serial.print(temp_final);
            Serial.println();
            
            tvoc_avg.total = 0;
            tvoc_avg.count = 0;
            co2_avg.total = 0;
            co2_avg.count = 0;
            hum_avg.total = 0;
            hum_avg.count = 0;
            temp_avg.total = 0;
            temp_avg.count = 0;
            
            display.clearDisplay();
            scriereDHT11(temp_final, hum_final);
            scriereCCS811(co2_final, tvoc_final,tvoc_level,co2_level);
            if(temp_final!=0 && hum_final!=0 && co2_final!=0 && tvoc_final!=0)
              sendDHT(temp_final, hum_final, co2_final, tvoc_final);
            ESP.deepSleep(3000);
//            ESP.deepSleep(0); 
        }
       
    }    
}

void sendDHT(float temperatureData, float humidityData, float co2Data, float tvocData){
   char server[] = "iotproject616.000webhostapp.com";
   
    if (client.connect(server, 80)) {
    Serial.println("connected");
    // Make a HTTP request:
    Serial.print("GET /dht_con.php?humidity=");
    client.print("GET /dht_con.php?humidity=");     //YOUR URL
    Serial.println(humidityData);
    client.print(humidityData);
    client.print("&temperature=");
    Serial.println("&temperature=");
    client.print(temperatureData);
    Serial.println(temperatureData);
    client.print("&co2=");
    Serial.println("&co=");
    client.print(co2Data);
    Serial.println(co2Data);
    client.print("&tvoc=");
    Serial.println("&tvoc=");
    client.print(tvocData);
    Serial.println(tvocData);   
    client.print(" ");      //SPACE BEFORE HTTP/1.1
    client.print("HTTP/1.1");
    client.println();
    client.println("Host: iotproject616.000webhostapp.com");
    client.println("Connection: close");
    client.println();
    delay(300);
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
}
#define HOST "iotproject616.000webhostapp.com"          // Enter HOST URL without "http:// "  and "/" at the end of URL
void insertSQL( float temp, float hum){
  
  int val = 1;
  int val2 = 99;
  
  String sendval, sendval2, postData;
  
  HTTPClient http;    // http object of clas HTTPClient

  // Convert integer variables to string
  sendval = String(val);  
  sendval2 = String(val2);  

  postData = "hum=" + sendval + "&temp=" + sendval2;

//  http.begin("https://iotproject616.000webhostapp.com/dbwrite.php");              // Connect to host where MySQL databse is hosted
//  http.addHeader("Content-Type", "application/x-www-form-urlencoded");            //Specify content-type header
  
  // We can post values to PHP files as  example.com/dbwrite.php?name1=val1&name2=val2&name3=val3  
   
  int httpCode = http.POST(postData);   // Send POST request to php file and store server response code in variable named httpCode
  Serial.println("Values are, hum = " + sendval + " and temp = "+sendval2 );
  
  // if connection eatablished then do this
  if (httpCode == 200) { Serial.println("Values uploaded successfully."); Serial.println(httpCode); 
  String webpage = http.getString();    // Get html webpage output and store it in a string
  Serial.println(webpage + "\n"); 
}

// if failed to connect then return and restart

else { 
  Serial.println(httpCode); 
  Serial.println("Failed to upload values. \n"); 
  http.end(); 
  return; }

}

void scriereDHT11(float t, float h) {
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.print("Umiditate : ");
  display.print(h);
  display.println();
  display.print("Temperatura : ");
  display.print(t); 
  display.display();
}

void scriereCCS811(float CO2, float TVOC, int co2_values ,int tvoc_levels) {
 
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println();
  display.println();
 
  display.print("CO2 : ");
  display.print(CO2);
  switch(co2_values){
    case 1 : display.print(" Excelent");break;
    case 2 : display.print(" Good");break;
    case 3 : display.print(" Moderate");break;
    case 4 : display.print(" Poor");break;
    case 5 : display.print(" Danger");break;
  }
  display.println();
  display.print("TVOC : ");
  display.print(TVOC);
  switch(tvoc_levels){
    case 1 : display.print(" Excelent");break;
    case 2 : display.print(" Good");break;
    case 3 : display.print(" Moderate");break;
    case 4 : display.print(" Poor");break;
    case 5 : display.print(" Danger");break;
  }
  display.display();
}
