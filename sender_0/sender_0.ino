/*
 * para hacer: ver si funciona el voltaje de referencia
 * long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

 *
 *2- si batería <3.65 y panel <4 no mandar

*/
#include "LoRaWan_APP.h"
#include "Arduino.h"

//Sensor de temperatura
#include <OneWire.h>
#include <DallasTemperature.h>

#include <CayenneLPP.h>


#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1790        /* Time ESP32 will go to sleep (in seconds) */
RTC_DATA_ATTR int bootCount = 1;



CayenneLPP lpp(51);

//Pines de entrada
#define TEMP_SENSOR_PIN 13
#define VOLT_SOLAR_PIN 32
#define VOLT_BAT_PIN 33


//Inicio del modulo de temperatura
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensor(&oneWire);



#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             10       // dBm

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

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];



bool lora_idle=true;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );

void setup() {
    Serial.begin(115200);
    Mcu.begin();
	

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

    
    //Inicializacion del modulo de temperatura
    sensor.begin();
    bootCount = (bootCount+1)%100;
    
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    
   }



void loop()
{
	if(lora_idle == true)
	{
    delay(5000);
		
    lpp.reset();
    ///Leer temperatura
    sensor.requestTemperatures();
    float temperature = sensor.getTempCByIndex(0);
    lpp.addTemperature(1, temperature);
    ///Lectura de la bateria
    float volt_bat = 2*3.3*analogRead(VOLT_BAT_PIN)/4096;
    lpp.addAnalogInput(2, volt_bat);
    ///Lectura del panel
    float volt_solar = 2*3.3*analogRead(VOLT_SOLAR_PIN)/4096;
    lpp.addAnalogInput(3, volt_solar);
    ///Print en serial de las lecturas
    Serial.print("Sending packet: ");
    Serial.println(temperature);
    Serial.print("Voltage bateria: ");
    Serial.println(volt_bat);
    Serial.print("Voltage panel solar: ");
    Serial.println(volt_solar);
    Serial.print("Paquete número: ");
    Serial.println(bootCount, DEC);
    lpp.addDigitalInput(4, bootCount);   //

    memcpy(txpacket, lpp.getBuffer(), lpp.getSize());
    Serial.print(" | ");  //https://community.mydevices.com/t/read-lora-cayennelpp-message-and-decode/16273/18
    for (int i = 0; i < lpp.getSize(); i++)
    {
      Serial.print(txpacket[i] >> 4, HEX);
      Serial.print(txpacket[i] & 0xF, HEX);
      Serial.print(" ");
    }
    Serial.print("|");
    
    Radio.Send( (uint8_t *)lpp.getBuffer(), lpp.getSize() ); //send the package out 
    //delay(5000);// no le estaba dando tiempo!
    //esp_deep_sleep_start();
    
        
    lora_idle = false;
    
	}
  Radio.IrqProcess( );   // ? pa qué sirve
}

void OnTxDone( void )
{
	Serial.println("TX done......");
	lora_idle = true;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    Serial.println("TX Timeout......");
    lora_idle = true;
}
