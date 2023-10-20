#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

void setup() {
  Serial.begin(9600);
  mySwitch.enableTransmit(10);  //Example pin number
  
  Serial.println("Type in the duty cycle value (e.g., 0.5) to transmit. Type 'start' to send the activation code or 'stop' to send deactivation code.");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');  //Read the input until a newline character is detected

    if (input == "start") {
      mySwitch.send(999999, 32);  //Sending the activation code
      Serial.println("Activation code sent!");
    } else if (input == "stop") {
      mySwitch.send(888888, 32);  //Sending the termination code
      Serial.println("Termination code sent!");
    } else {
      float duty_cycle = input.toFloat();
      if (duty_cycle > 0 && duty_cycle <= 1) {
        mySwitch.send(floatToIntBits(duty_cycle), 32);  //Sending the duty_cycle
        Serial.println("Duty cycle " + String(duty_cycle) + " sent!");
      } else {
        Serial.println("Invalid input. Please enter a value between 0.1 and 1, type 'start', or type 'stop'.");
      }
    }
  }
}

unsigned long floatToIntBits(float value) {
    union {
        float f;
        unsigned long l;
    } data;
    data.f = value;
    return data.l;
}
