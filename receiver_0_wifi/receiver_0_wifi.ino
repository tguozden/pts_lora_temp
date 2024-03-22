#include "LoRaWan_APP.h"
#include "Arduino.h"

#include "ArduinoJson.h"
#include <WiFi.h>
WiFiClient client;

#define ssid "bc811a"
#define password "nachiketas"
#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             14        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       12         // [SF7..SF12]
#define LORA_CODINGRATE                             4         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here

#include <ArduinoJson.h>
//Cayenne
#include <CayenneLPP.h>

//Inicio del objeto de cayenne
CayenneLPP lpp(LORAWAN_APP_DATA_MAX_SIZE);

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

int16_t txNumber;

int16_t rssi;

bool lora_idle = true;

void setup() {
    Serial.begin(115200);
    Mcu.begin();
    
// We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());


    
   
    txNumber=0;
    rssi=0;
 
    RadioEvents.RxDone = OnRxDone;
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                               LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                               LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                               0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
                                  
}



void loop()
{
  if(lora_idle)
  {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }
  Radio.IrqProcess( );
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{

    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';
    Radio.Sleep( );

    // convert String to bytes array
    // https://community.mydevices.com/t/read-lora-cayennelpp-message-and-decode/16273/4
    //uint8_t *buffer = payload;
   
    //StaticJsonDocument<512> jsonBuffer;
    //JsonArray root = jsonBuffer.to<JsonArray>();
    //lpp.decode((uint8_t *)payload, size, root);
   //
    //serializeJsonPretty(root, Serial);
    //Serial.println();
   
    DynamicJsonDocument jsonBuffer(512);
    JsonObject root = jsonBuffer.to<JsonObject>();

    

    lpp.decodeTTN((uint8_t *) payload, size, root);
    
    root["rssi"]= rssi;
//    serializeJsonPretty(root, Serial);
    Serial.println();
  //https://la.mathworks.com/help/thingspeak/continuously-collect-data-and-bulk-update-a-thingspeak-channel-using-an-arduino-mkr1000-board-or-an-esp8266-board.html     
    //Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n",rxpacket,,rxSize);
    lora_idle = true;

      char aux[100];
      if (client.connect("api.thingspeak.com",80)) {  //   "184.106.153.149" or api.thingspeak.com
        String postStr = "HB61WTMZM5G2J3MT";
        postStr +="&temperature_1=";
        sprintf(aux,"%f", root["temperature_1"]); 
        postStr +=aux;
        postStr +="&analog_in_3=";
        postStr +=aux;
        postStr += "&digital_in_4=";
        postStr +=aux;
        postStr += "&rssi=";
        postStr +=aux;
       Serial.println(postStr);
 
       client.print("POST /update HTTP/1.1\n"); 
       client.print("Host: api.thingspeak.com\n"); 
       client.print("Connection: close\n"); 
       client.print("X-THINGSPEAKAPIKEY: HB61WTMZM5G2J3MT\n"); 
       client.print("Content-Type: application/x-www-form-urlencoded\n"); 
       client.print("Content-Length: "); 
       client.print(postStr.length()); 
       client.print("\n\n"); 
       client.print(postStr);
      }
    
}
