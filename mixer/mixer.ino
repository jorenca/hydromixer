#include <DS18B20.h>


///////////////////////////////////////////////////////////

#define PUMP_RELAY 7

void pumpPower(boolean pumpOn) {
    digitalWrite(PUMP_RELAY, !pumpOn); // relay on when pin=0
}


///////////////////////////////////////////////////////////
#define TEMP_SENSOR_PIN 12

DS18B20 ds(TEMP_SENSOR_PIN);

float temperatureRead() {
  return ds.getTempC();
}
///////////////////////////////////////////////////////////


#define TDS_RELAY 2
#define TDS_INPUT A0

int tdsRead(float temperature) {
  digitalWrite(TDS_RELAY, 0); // sensor on
  delay(3000);
  int rawVal = analogRead(TDS_INPUT);
  digitalWrite(TDS_RELAY, 1); // sensor off
  
  float volts = (rawVal / 1023.0) * 5.0;
  float tds = 66.1*volts*volts*volts - 127.9*volts*volts + 428.7*volts;
  float tempCorrection = 1.0 + 0.02*(temperature-25.0); 
  return tds * tempCorrection;
}
///////////////////////////////////////////////////////////

#define PH_SENSOR_RELAY 3
#define PH_SENSOR_INPUT A1

float phRead() {
  digitalWrite(PH_SENSOR_RELAY, 0); // sensor on
  delay(10000);
  int val = analogRead(PH_SENSOR_INPUT);
  digitalWrite(PH_SENSOR_RELAY, 1); // sensor off
  return (val / 1023.0) * 14.0;
}
///////////////////////////////////////////////////////////

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("Hydromixer v0.1");
  Serial.println("Send M to run a mixing cycle");

  ds.selectNext();

  digitalWrite(PUMP_RELAY, 1);
  pinMode(PUMP_RELAY, OUTPUT);
  
  digitalWrite(TDS_RELAY, 1);
  pinMode(TDS_RELAY, OUTPUT);
  pinMode(TDS_INPUT, INPUT);
  
  digitalWrite(PH_SENSOR_RELAY, 1);
  pinMode(PH_SENSOR_RELAY, OUTPUT);
  pinMode(PH_SENSOR_INPUT, INPUT);

  delay(2000);
}


void loop() {
  char serialCommand = Serial.read();

  if (serialCommand != 'R' and serialCommand != 'M') {
    delay(1000);
    return;
  }
  
  if (serialCommand == 'R') {

    Serial.print("STATUS Temperature=");
    Serial.println(temperatureRead());
    
    return;
  }
  
  
  pumpPower(true);
  delay(2000); // FIXME make longer
  pumpPower(false);
  delay(3000);
  
  float temperature = temperatureRead();
  int tds = tdsRead(temperature);
  delay(1000);
  float ph = phRead();
  
  Serial.print("STATUS TDS=");
  Serial.print(tds);
  Serial.print(" PH=");
  Serial.print(ph);
  Serial.print(" Temperature=");
  Serial.println(temperature);

  // TODO if needs PH correction...
  // run pump,
  // deliver PH correction solution
  // turn off pump
  
  delay(15000);
}
