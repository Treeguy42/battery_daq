// Initialization and settings for LoRa communication
#define loraSerial Serial3

// Time tracking for last command sent and last command verification
unsigned long lastCommandTime = 0;
String lastCommandSent = "";  // Store the last command sent for verification

// Function to send an AT command and retry until +OK is received
void checkATCommand(String command) {
    bool successful = false;
    int maxRetries = 5;  // Set maximum retries if needed

    for (int attempt = 0; attempt < maxRetries && !successful; attempt++) {
        Serial.print("Sending to LoRa: ");
        Serial.println(command);
        loraSerial.println(command);

        long endTime = millis() + 5000;  // 5 seconds timeout
        String response = "";

        while (millis() < endTime && !response.endsWith("\n")) {
            if (loraSerial.available()) {
                char c = loraSerial.read();
                response += c;
            }
        }

        response.trim();
        Serial.println("Response from LoRa: " + response);

        if (response.startsWith("+OK")) {
            Serial.println("Command successful: " + command);
            successful = true;
        } else if (response.startsWith("+ERR")) {
            Serial.println("Error in command: " + command + ". Retrying in 1 second...");
            delay(1000);  // Wait before retrying
        }
    }

    if (!successful) {
        Serial.println("Command failed after maximum retries: " + command);
        return;  // Stop further execution if LoRa fails to initialize
    }
}

// Initialize the LoRa module
void initializeLoRaModule() {
    checkATCommand("AT\r\n");
    //checkATCommand("AT+RESET\r\n");
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
    loraSerial.begin(115200);  // LoRa communication
    delay(1000);
    Serial.println("Setup started...");
    delay(1000);

    // Initialize LoRa module using AT commands
    initializeLoRaModule();
    Serial.println("LoRa module initialized.");
    Serial.println("Ready to receive data and send commands. Please enter 'start', 'stop', or 'set duty X' where X is between 0.01 and 1.0.");
}

void sendCommandWithVerification(String command) {
    lastCommandSent = "AT+SEND=111," + String(command.length()) + "," + command; // Prepare the full command string for echo comparison
    loraSerial.println(lastCommandSent);  // Send the command
    Serial.println("Command sent: " + lastCommandSent);
    Serial.println("Awaiting confirmation and echo for verification...");
}

void loop() {
    // Check if any data or commands were received from the DAQ
    if (loraSerial.available()) {
        String line = loraSerial.readStringUntil('\n');
        line.trim();  // Clean up the input

        if (line.startsWith("+OK")) {
            Serial.println("Command processed by LoRa module: " + line);
            // Now wait for the echo
        } else if (line.startsWith("+RCV=")) {
        Serial.println("Received line: " + line); // Debugging output

        // Extract command from received message
        int firstCommaIndex = line.indexOf(',');
        int secondCommaIndex = line.indexOf(',', firstCommaIndex + 1);
        int thirdCommaIndex = line.indexOf(',', secondCommaIndex + 1);
        int startIdx = thirdCommaIndex + 1;
        int endIdx = line.indexOf(',', startIdx);
        String command = line.substring(startIdx, endIdx);

        //Serial.println("Extracted command: " + command); // Debugging output

        // Handling specific commands
        if (command.equalsIgnoreCase("START")) {
            Serial.println("Starting DAQ as commanded.");
        } else if (command.equalsIgnoreCase("STOP")) {
            Serial.println("Stopping DAQ as commanded.");
        } else if (command.startsWith("DUTY_CYCLE:")) {
            float newDuty = command.substring(11).toFloat();
            Serial.println("Setting new duty cycle to: " + String(newDuty));
        } else {
            // Handle other data (split the payload into individual data fields)
            String payload = line.substring(thirdCommaIndex + 1); // Get the payload data after "+RCV=...,"

            int idx = 0;
            while (idx != -1) {
                int nextIdx = payload.indexOf(',', idx);
                String dataField = nextIdx == -1 ? payload.substring(idx) : payload.substring(idx, nextIdx);
                //Serial.println("Data field: " + dataField); // Debug output for each field
                idx = nextIdx != -1 ? nextIdx + 1 : -1; // Update idx to the start of the next data field
            }
        }
        } else if (line.startsWith("+ERR")) {
            Serial.println("Error response received: " + line);
        } else {
            Serial.println("Other message received: " + line);
        }
    }

    // Handle user input to send commands
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase(); // Convert input to uppercase for consistent command handling

        if (input.equalsIgnoreCase("START") || input.equalsIgnoreCase("STOP")) {
            sendCommandWithVerification(input);
        } else if (input.startsWith("SET DUTY ")) {
            float duty_cycle = input.substring(9).toFloat();
            if (duty_cycle >= 0.01 && duty_cycle <= 1.00) {
                sendCommandWithVerification("DUTY_CYCLE:" + String(duty_cycle, 2)); // Ensure the command includes precise float formatting
            } else {
                Serial.println("Invalid duty cycle. Please specify a value between 0.01 and 1.0.");
            }
        } else {
            Serial.println("Unrecognized input. Please enter 'start', 'stop', or 'set duty X' where X is between 0.01 and 1.0.");
        }
    }
}
