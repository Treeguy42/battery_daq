// Analog inputs
#define batt_in A0
#define hc_in A1
#define on_off 8  // Pin that turns on/off the ss_relay according to the duty cycle

// Current values
double Vout = 0.0;
double Current = 0;
double resADC = 0.0048828125;

// Floats
float adcv0 = 0.0;
float batt_volts = 0.0;
float adcv1 = 0.0;
float hc_volts = 0.0;

// LoRa settings using AT commands & RX/TX communication
#define loraSerial Serial3
bool startDAQ = false;  // Flag to start the DAQ process

// Resistor values on V board
float R1 = 30000.0;
float R2 = 7500.0;

float ref_volts = 5.0;
int adci0 = 0;
int adci1 = 0;

// Duty cycle (on) (0.1 to 1)
float duty_cycle = 0.50;

// Thermistor parameters
float V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12;
float T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12;
float R20, R21, R22, R23, R24, R25, R26, R27, R28, R29, R210, R211, R212;
float K = 273.15;

// Given thermistor values from the manufacturer
float B = 3950;
float To = 25 + K;
float Ro = 100000;
float r_inf = Ro * exp(-B / To);

// Data file, SD card
#include <SPI.h>
#include <SD.h>
const int chipSelect = 53;

// Time
unsigned long runningTime = 0;
unsigned long dT = 0;
unsigned long lastLoRaSendTime = 0;

String csv = "";
String currentFilename = "";

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
    checkATCommand("AT+ADDRESS=111\r\n");
    checkATCommand("AT+NETWORKID=7\r\n");
    checkATCommand("AT+BAND=915200000\r\n");
    checkATCommand("AT+PARAMETER=12,7,1,4\r\n");
    checkATCommand("AT+IPR=115200\r\n");
    checkATCommand("AT+CPIN=FAC8A8BA66BF894F0FDF50187831A4D1");
}

void setup() {
    Serial.begin(9600);
    pinMode(on_off, OUTPUT);
    if (!SD.begin(chipSelect)) {
        Serial.println("Error: SD card initialization failed!");
    } else {
        Serial.println("SD card initialized successfully.");
    }

    loraSerial.begin(115200);  // Start LoRa communication

    // Initialize LoRa module using AT commands
    initializeLoRaModule();
    Serial.println("LoRa module initialized.");
}

String generateUniqueFilename() {
    String baseName = "csvDAQ";
    String extension = ".txt";
    if (!SD.exists(baseName + extension)) {
        Serial.println("Creating new base file: " + baseName + extension);
        return baseName + extension;
    }

    for (int i = 1; i < 1000; i++) {
        String newFilename = baseName + String(i) + extension;
        if (!SD.exists(newFilename)) {
            Serial.println("Creating new file: " + newFilename);
            return newFilename;
        }
    }

    Serial.println("Error: Unique filename could not be created.");
    return "file_error.txt";  // Return this if a unique name can't be found.
}

void loop() {
    signed long currentMillis = millis();
    // Send LoRa data every 30 seconds
    if (currentMillis - lastLoRaSendTime >= 30000) {
        String dataToSend = "Current Readings: " + String(Current) + ", CSV Name: " + currentFilename;
        loraSerial.print("AT+SEND=0," + String(dataToSend.length()) + "," + dataToSend + "\r\n");
        lastLoRaSendTime = currentMillis;
    }

    if (!startDAQ) {
        return;  // Exit loop if DAQ is not started
    }

    if (loraSerial.available()) {
        String command = loraSerial.readStringUntil('\n');
        if (command.startsWith("+OK")) {
            Serial.println(command);  // Handle OK response
        } else if (command.startsWith("START")) {
            startDAQ = true;
            Serial.println("DAQ started.");
        } else if (command.startsWith("STOP")) {
            startDAQ = false;
            Serial.println("DAQ stopped.");
        } else if (command.startsWith("DUTY_CYCLE:")) {
            duty_cycle = command.substring(11).toFloat();
            Serial.println("Duty cycle set to: " + String(duty_cycle));
        }
    }

    if (Serial.available()) {
        char incoming_char = Serial.peek();  // Check the next character without removing it from the buffer
        if (incoming_char >= '0' && incoming_char <= '9' || incoming_char == '.' || incoming_char == '-') {
            // If the next character is a digit, a decimal point, or a minus sign, parse the float
            duty_cycle = float(Serial.parseFloat());
            Serial.println("Duty cycle adjusted to: " + String(duty_cycle));
        } else {
            // If the next character is not a digit, a decimal point, or a minus sign, discard it
            Serial.read();  // Remove the character from the buffer
        }
    }

    // Analog reading and calculations
    digitalWrite(on_off, HIGH);
    delay(10);
    adci0 = analogRead(batt_in);
    adci1 = analogRead(hc_in);

    // Bit to volt at reference
    adcv0 = (adci0 * ref_volts) / 1024.0;
    adcv1 = (adci1 * ref_volts) / 1024.0;

    // Calculate voltage divider input
    batt_volts = adcv0 / (R2 / (R1 + R2));
    hc_volts = adcv1 / (R2 / (R1 + R2));

    for (int i = 0; i < 1000; i++) {
        Vout = (Vout + (resADC * analogRead(A2)));
        delay(1);
    }
    Vout = Vout / 1000;
    Current = (Vout - 2.49) / (.066 * 2);  // Adjust for 4S, remove "*2" for 2S

    // Read and calculate thermistor temperatures
    V0 = analogRead(A3);
    V0 = analogRead(A3);  // Read twice to overcome an anomaly
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

    // Thermistor circuit (Voltage divider network / Series Circuit)
    R20 = Ro * 1023.0 / V0 - Ro;
    R21 = Ro * 1023.0 / V1 - Ro;
    R22 = Ro * 1023.0 / V2 - Ro;
    R23 = Ro * 1023.0 / V3 - Ro;
    R24 = Ro * 1023.0 / V4 - Ro;
    R25 = Ro * 1023.0 / V5 - Ro;
    R26 = Ro * 1023.0 / V6 - Ro;
    R27 = Ro * 1023.0 / V7 - Ro;
    R28 = Ro * 1023.0 / V8 - Ro;
    R29 = Ro * 1023.0 / V9 - Ro;
    R210 = Ro * 1023.0 / V10 - Ro;
    R211 = Ro * 1023.0 / V11 - Ro;
    R212 = Ro * 1023.0 / V12 - Ro;

    // Kelvin conversion for logging/csv creation
    T0 = (B / log(R20 / r_inf)) - K;
    T1 = (B / log(R21 / r_inf)) - K;
    T2 = (B / log(R22 / r_inf)) - K;
    T3 = (B / log(R23 / r_inf)) - K;
    T4 = (B / log(R24 / r_inf)) - K;
    T5 = (B / log(R25 / r_inf)) - K;
    T6 = (B / log(R26 / r_inf)) - K;
    T7 = (B / log(R27 / r_inf)) - K;
    T8 = (B / log(R28 / r_inf)) - K;
    T9 = (B / log(R29 / r_inf)) - K;
    T10 = (B / log(R210 / r_inf)) - K;
    T11 = (B / log(R211 / r_inf)) - K;
    T12 = (B / log(R212 / r_inf)) - K;
}
