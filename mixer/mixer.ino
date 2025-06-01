#include <DS18B20.h>
#include <AS5600.h>


///////////////////////////////////////////////////////////

#define PUMP_RELAY 7


void pumpPower(boolean pumpOn) {
    digitalWrite(PUMP_RELAY, !pumpOn); // relay on when pin=0
}


///////////////////////////////////////////////////////////

#define TEMP_SENSOR_PIN 11

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

#define FLOW_SENSOR_PIN A3

bool hasFlow() {
  int changes = 0;
  bool lastState = digitalRead(FLOW_SENSOR_PIN);
  long start = millis();
  while (millis() - start < 500) {
    if (lastState == digitalRead(FLOW_SENSOR_PIN)) continue;
    lastState = digitalRead(FLOW_SENSOR_PIN);
    changes++;
    if (changes > 15) return true;
  }

  return false;
}

///////////////////////////////////////////////////////////

AS5600 tankWaterLevelSensor;


int measureTankWaterLevelUnfiltered() {
  if (!tankWaterLevelSensor.isConnected()) {
    Serial.println("No tank water level sensor");
    return -1;
  }
  
  int angleRaw = tankWaterLevelSensor.readAngle(); // 12bit
  
  // Serial.print("Raw level sensor value: "); Serial.println(angleRaw);
  
  #define MAX_TANK_LEVEL_VAL 2450
  #define MIN_TANK_LEVEL_VAL 1650
  angleRaw = min(MAX_TANK_LEVEL_VAL, angleRaw);
  angleRaw = max(MIN_TANK_LEVEL_VAL, angleRaw);
  float angle = map(angleRaw, MIN_TANK_LEVEL_VAL, MAX_TANK_LEVEL_VAL, 0, 10000) / 10000.0;
  //return 100.0 * (1.0 - sin((PI / 2) * angle)); // level from 0 to 100
  return round(100.0 * (1.0 - cos(PI / 2.0 * angle))); // level from 0 to 100
}

int measureTankWaterLevel() {
  int8_t reads[3] = {-2, -2, -2};
  int8_t readI = 0;
  do {
    reads[readI++] = measureTankWaterLevelUnfiltered();
    readI %= 3;
  } while (reads[0] != reads[1] || reads[1] != reads[2] || reads[0] != reads[2]);

  return reads[0];
}

///////////////////////////////////////////////////////////


void setup() {
  Serial.begin(115200);
  delay(1000);
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

  pinMode(FLOW_SENSOR_PIN, INPUT);
  digitalWrite(FLOW_SENSOR_PIN, 1);

  Wire.begin();
  tankWaterLevelSensor.begin(); // set direction pin.
  tankWaterLevelSensor.setDirection(AS5600_CLOCK_WISE); // default, just be explicit.

  delay(2000);
}

///////////////////////////////////////////////////////////

void loop() {
  
  char serialCommand = Serial.read();

  if (serialCommand != 'R' and serialCommand != 'M') {
    delay(1000);
    return;
  }
  
  if (serialCommand == 'R') {
    Serial.print("STATUS Temperature=");
    Serial.print(temperatureRead());
    Serial.print(" TankLevel=");
    Serial.println(measureTankWaterLevel());
    
    return;
  }
  
  // else when command is M = mixing...
  
  pumpPower(true);
  delay(5000); // FIXME make longer
  bool isFlowing = hasFlow(); // FIXME GEORGI for debugging only
  if (!isFlowing) {
    Serial.println("ERROR No pump flow!");
    pumpPower(false);
    return;
  }
  delay(10000); // FIXME make longer
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
  Serial.print(" TankLevel=");
  Serial.println(measureTankWaterLevel());

  // TODO if needs PH correction...
  // run pump,
  // deliver PH correction solution
  // turn off pump
  
  delay(15000);
}
