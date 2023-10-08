@@ -0,0 +1,173 @@
//analog inouts
#define batt_in A0
#define hc_in A1
#define on_off 8 //pin that turns cycles the ss_relay acording to duty cycle

//current values
double Vout = 0.0;
double Current = 0;
double resADC = 0.0048828125;

//floats
float adcv0 = 0.0;
float batt_volts = 0.0;
float adcv1 = 0.0;
float hc_volts = 0.0;

//resistor values on V board
float R1 = 30000.0;
float R2 = 7500.0;

float ref_volts = 5.0;
int adci0 = 0;
int adci1 = 0;

//duty cycle (on) (0.1 to 1)
float duty_cycle = .5;

// thermistor stuff
float V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12;
float T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12;
float R20, R21, R22, R23, R24, R25, R26, R27, R28, R29, R210, R211, R212;
float K = 273.15;

//Given thermistor values from manufacturer     
float B = 3950;
float To = 25+K;
float Ro = 100000;
float r_inf = Ro*exp(-B/To);

//data file, SD card
#include <SPI.h>
#include<SD.h>
const int chipSelect = 53;

//rf24 module library
#include <RF24.h>
RF24 radio(9,10); //CE, CSN


//time
unsigned long runningTime = 0;
unsigned long dT = 0;

String csv = "";



void setup() {
  Serial.begin(9600);
  //Serial1.begin(9600);
  pinMode(on_off, OUTPUT);
  SD.begin(chipSelect);

  if (!SD.begin(chipSelect)) {
    Serial.println("Card not present");
  }

}

void loop() {

  if (Serial.available()) {
    char incoming_char = Serial.peek(); // Check the next character without removing it from the buffer
  if (incoming_char >= '0' && incoming_char <= '9' || incoming_char == '.' || incoming_char == '-') {
    // If the next character is a digit, a decimal point, or a minus sign, parse the float
    duty_cycle = float(Serial.parseFloat());
    Serial.println(duty_cycle);
  } else {
    // If the next character is not a digit, a decimal point, or a minus sign, discard it
    Serial.read(); // Remove the character from the buffer
  }
  }
  else {

  //Read analog inputs
  digitalWrite(on_off, HIGH);
  delay(10);
  adci0 = analogRead(batt_in);
  adci1 = analogRead(hc_in);

  //bit to volt at referance
  adcv0 = (adci0 * ref_volts) / 1024.0;
  adcv1 = (adci1 * ref_volts) / 1024.0;

  //calc voltage divider input
  batt_volts = adcv0 / (R2/(R1+R2));
  hc_volts = adcv1 / (R2/(R1+R2));

  for(int i = 0; i < 1000; i++){
    Vout = (Vout + (resADC * analogRead(A2)));
    delay(1);
  }
  Vout = Vout/1000;
  Current = (Vout - 2.49)/(.066*2); // This is set up for 4S, for 2S remove the "*2"


  //read and calc thermistor temps
    V0 = analogRead(A3);
    V0 = analogRead(A3); //A3 is intentially read twice to overcome an anomoly
    V1 = analogRead(A4);
    V2 = analogRead(A5);
    V3 = analogRead(A6);
    V4 = analogRead(A7);
    V5 = analogRead(A8);
    V6 = analogRead(A9);
    V7 = analogRead(A10);
    V8 = analogRead(A11);
    V9 = analogRead(A12);
    V10 = analogRead(A13);
    V11 = analogRead(A14);
    V12 = analogRead(A15);

    //Thermistor circuit//Voltage divider network//Series Circuit
    R20 = Ro*1023.0/V0-Ro;
    R21 = Ro*1023.0/V1-Ro;
    R22 = Ro*1023.0/V2-Ro;
    R23 = Ro*1023.0/V3-Ro;
    R24 = Ro*1023.0/V4-Ro;
    R25 = Ro*1023.0/V5-Ro;
    R26 = Ro*1023.0/V6-Ro;
    R27 = Ro*1023.0/V7-Ro;
    R28 = Ro*1023.0/V8-Ro;
    R29 = Ro*1023.0/V9-Ro;
    R210 = Ro*1023.0/V10-Ro;
    R211 = Ro*1023.0/V11-Ro;
    R212 = Ro*1023.0/V12-Ro;

    //Beta Parameter Equation
    T0 = B/log(R20/r_inf)-K; 
    T1 = B/log(R21/r_inf)-K;
    T2 = B/log(R22/r_inf)-K;
    T3 = B/log(R23/r_inf)-K;
    T4 = B/log(R24/r_inf)-K;
    T5 = B/log(R25/r_inf)-K;
    T6 = B/log(R26/r_inf)-K;
    T7 = B/log(R27/r_inf)-K;
    T8 = B/log(R28/r_inf)-K;
    T9 = B/log(R29/r_inf)-K;
    T10 = B/log(R210/r_inf)-K;
    T11 = B/log(R211/r_inf)-K;
    T12 = B/log(R212/r_inf)-K;

  dT = millis() - runningTime;
  runningTime = millis();
  

  csv = String(runningTime)+","+String(dT)+","+String(batt_volts, 2)+","+String(hc_volts, 2)+","+String(Current, 2)+","+String(duty_cycle)+","+String(T0)+","+String(T1)+","+String(T2)+","+String(T3)+","+String(T4)+","+String(T5)+","+String(T6)+","+String(T7)+","+String(T8)+","+String(T9)+","+String(T10)+","+String(T11)+","+String(T12);


  Serial.println(csv);
  
  digitalWrite(on_off, LOW);
  //script takes 1200ms to run plus added delays
  delay(1200/duty_cycle - 1200);

  File dataFile = SD.open("csvData.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println(csv);
    dataFile.close();
  }  
  }
  
}
