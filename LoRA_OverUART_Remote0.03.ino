// Function to send an AT command and retry until +OK is received
void checkATCommand(String command) {
    bool successful = false;
    int maxRetries = 5;  // Set maximum retries if needed

    for (int attempt = 0; attempt < maxRetries && !successful; attempt++) {
        Serial.print("Sending to LoRa: ");
        Serial.println(command);
        Serial3.println(command);

        long endTime = millis() + 5000; // 5 seconds timeout
        String response = "";

        while (millis() < endTime && !response.endsWith("\n")) {
            if (Serial3.available()) {
                char c = Serial3.read();
                response += c;
            }
        }

        response.trim();
        Serial.println("Response from LoRa: " + response);

        if (response.startsWith("+OK")) {
            Serial.println("Command successful: " + command);
            successful = true;
        } else {
            Serial.println("Error in command: " + command + ". Retrying in 1 second...");
            delay(1000);  // Wait before retrying
        }
    }

    if (!successful) {
        Serial.println("Command failed after maximum retries: " + command);
    }
}

// Initialize the LoRa module
void initializeLoRaModule() {
    checkATCommand("AT\r\n");
    checkATCommand("AT+RESET\r\n");
    checkATCommand("AT+MODE=0\r\n");
    checkATCommand("AT+ADDRESS=222\r\n");
    checkATCommand("AT+NETWORKID=7\r\n");
    checkATCommand("AT+BAND=915200000\r\n");
    checkATCommand("AT+PARAMETER=12,7,1,4\r\n");
    checkATCommand("AT+IPR=115200\r\n");
    checkATCommand("AT+CPIN=FAC8A8BA66BF894F0FDF50187831A4D1");
}

void setup() {
    Serial.begin(9600);  // USB communication
    Serial3.begin(115200);  // LoRa communication
    delay(1000);
    Serial.println("Setup started...");
    delay(1000);

    // Initialize LoRa module using AT commands
    initializeLoRaModule();
    Serial.println("LoRa module initialized.");
    Serial.println("Type in the duty cycle value (e.g., 0.50) to transmit. Type 'start' or 'stop' to send commands.");
}

void loop() {
    // Check if any commands were received from the LoRa module
    if (Serial3.available()) {
        String line = Serial3.readStringUntil('\n');
        Serial.println("From LoRa: " + line);
    }

    // Handle user commands input via USB serial
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input.equalsIgnoreCase("start")) {
            Serial3.println("AT+SEND=0,5,START\r\n");  // Send the "START" command via LoRa
            Serial.println("Activation code sent via LoRa.");
        } else if (input.equalsIgnoreCase("stop")) {
            Serial3.println("AT+SEND=0,4,STOP\r\n");  // Send the "STOP" command via LoRa
            Serial.println("Termination code sent via LoRa.");
        } else if (input.startsWith("set duty ")) {
            float duty_cycle = input.substring(9).toFloat();
            if (duty_cycle >= 0.01 && duty_cycle <= 1.00) {
                String dutyMsg = "DUTY_CYCLE:" + String(duty_cycle);
                Serial3.println("AT+SEND=0," + String(dutyMsg.length()) + "," + dutyMsg + "\r\n");  // Send the duty cycle via LoRa
                Serial.println("Duty cycle " + String(duty_cycle) + " sent via LoRa.");
            } else {
                Serial.println("Invalid duty cycle. Please specify a value between 0.01 and 1.0.");
            }
        } else {
            Serial.println("Unrecognized input. Please enter 'start', 'stop', or 'set duty X' where X is between 0.01 and 1.0.");
        }
    }
}
