#include <RF24.h>

RF24 radio(9, 10);  // CE, VSN/CSN
const byte pins[] = {2, 3, 4, 5, 6, 7, 8};  //Example pin assignments for the buttons
float duty_cycle = 0.5;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 7; i++) {
    pinMode(pins[i], INPUT_PULLUP);  //Initialize button pins with internal pull-up
  }

  radio.begin();
  radio.setChannel(90);
  radio.setDataRate(RF24_1MBPS);
  radio.openWritingPipe(0xF0F0F0F0E1LL);
  radio.setPALevel(RF24_PA_HIGH);
}

void loop() {
  // Check each button
  if (digitalRead(pins[0]) == LOW) { duty_cycle += 0.1; delay(200); }
  if (digitalRead(pins[1]) == LOW) { duty_cycle -= 0.1; delay(200); }
  if (digitalRead(pins[2]) == LOW) { duty_cycle += 0.2; delay(200); }
  if (digitalRead(pins[3]) == LOW) { duty_cycle -= 0.2; delay(200); }
  if (digitalRead(pins[4]) == LOW) { duty_cycle += 0.25; delay(200); }
  if (digitalRead(pins[5]) == LOW) { duty_cycle -= 0.25; delay(200); }
  
  // Check the "send" button
  if (digitalRead(pins[6]) == LOW) {
    Serial.println("Sending updated duty cycle: " + String(duty_cycle));
    radio.stopListening();
    radio.write(&duty_cycle, sizeof(duty_cycle));
    radio.startListening();
    delay(200);  // Simple debounce
  }

  // Limit duty_cycle within [0.1, 1.0]
  duty_cycle = constrain(duty_cycle, 0.1, 1.0);
  delay(10);
}
