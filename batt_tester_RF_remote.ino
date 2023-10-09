#include <RF24.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_I2C_ADDRESS 0x3C  //Typically 0x3C or 0x3D
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

RF24 radio(9, 10);  //CE, CSN
const byte pins[] = {2, 3, 4, 5, 6, 7, 8};  //(Digital)D-Pin assignments for the buttons
float duty_cycle = 0.5;

void setup() {
  Serial.begin(115200);

  //Initialize button pins with internal pull-up
  for (int i = 0; i < 7; i++) {
    pinMode(pins[i], INPUT_PULLUP);  
  }

  //RF radio setup
  radio.begin();
  radio.setAutoAck(false);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.openWritingPipe(0xF0F0F0F0E1LL);
  radio.stopListening();

  //OLED setup
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);  //loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
    //Check each button for changes in duty cycle
    if (digitalRead(pins[0]) == LOW) { duty_cycle -= 0.25; delay(200); }
    if (digitalRead(pins[1]) == LOW) { duty_cycle += 0.25; delay(200); }
    if (digitalRead(pins[2]) == LOW) { duty_cycle -= 0.2; delay(200); }
    if (digitalRead(pins[3]) == LOW) { duty_cycle += 0.2; delay(200); }
    if (digitalRead(pins[4]) == LOW) { duty_cycle -= 0.1; delay(200); }
    if (digitalRead(pins[5]) == LOW) { duty_cycle += 0.1; delay(200); }

    //Check the "send" button D8
    if (digitalRead(pins[6]) == LOW) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Sending updated duty cycle:");
        display.setCursor(0, 20);
        display.print(String(duty_cycle));
        display.display();

        radio.stopListening();
        radio.write(&duty_cycle, sizeof(duty_cycle));

        char incoming_msg[15] = "";
        radio.startListening();
        unsigned long start_time = millis();
        bool receivedConfirmation = false;
        while (millis() - start_time < 2000) {
            if (radio.available()) {
                radio.read(&incoming_msg, sizeof(incoming_msg));
                if (strcmp(incoming_msg, "Received") == 0) {
                    receivedConfirmation = true;
                    break;
                }
            }
        }

        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Duty Cycle: ");
        display.print(String(duty_cycle));
        display.setCursor(0, 20);
        if (receivedConfirmation) {
            display.print("Confirmation: Go!");
        } else {
            display.print("Confirmation: No-Go");
        }
        display.display();
    }

    //Continuously show current duty cycle without sending
    else {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Duty Cycle: ");
        display.print(String(duty_cycle));
        display.display();
    }

    //Limit duty_cycle within [0.1, 1.0]
    duty_cycle = constrain(duty_cycle, 0.1, 1.0);
    delay(10);
}