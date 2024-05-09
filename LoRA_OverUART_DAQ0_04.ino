//Analog inputs
#define batt_in A0
#define hc_in A1
#define on_off 8  //Pin that turns on/off the ss_relay according to the duty cycle
#define LED_PIN 13  //Define the pin where the LED is connected for SD card initialize success
#define LED_PIN1 12  //Define the pin where the LED is connected for LoRa initialize success
#define LED_PIN2 11  //Define the pin where the LED is connected when start command is received and DAQ begins recording data
#define LED_PIN3 10  //Define the pin where the LED is connected for detecting thermal load on thermistors
#define LED_PIN4 9  //Define the pin where the LED is connected for fail state; fail state are where an initialization step fails and 'return;' is used

//Current values
double Vout = 0.0;
double Current = 0;
double resADC = 0.0048828125;  //Resolution of ADC in volts per step

//Floats for voltage calculations
float adcv0 = 0.0;
float batt_volts = 0.0;
float adcv1 = 0.0;
float hc_volts = 0.0;

//LoRa settings using AT commands & RX/TX communication
#define loraSerial Serial3
bool startDAQ = false;  //Flag to start the DAQ process

//Resistor values on voltage divider
float R1 = 30000.0;
float R2 = 7500.0;

float ref_volts = 5.0;  //Reference voltage for ADC
int adci0 = 0;
int adci1 = 0;

//Duty cycle (on) (0.1 to 1)
float duty_cycle = 0.50;

//Thermistor parameters
float V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12;
float T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12;
float R20, R21, R22, R23, R24, R25, R26, R27, R28, R29, R210, R211, R212;
float K = 273.15;  //Conversion factor for Kelvin to Celsius

//Given thermistor values from the manufacturer
float B = 3950;  //Beta constant
float To = 25 + K;  //Reference temperature in Kelvin
float Ro = 100000;  //Reference resistance at To
float r_inf = Ro * exp(-B / To);  //Pre-calculated part of the thermistor resistance formula

//Data file, SD card
#include <SPI.h>
#include <SD.h>
const int chipSelect = 53;  //Chip select pin for the SD card

//Time management
unsigned long runningTime = 0;
unsigned long dT = 0;
unsigned long lastLoRaSendTime = 0;

String csv = "";  //CSV string for data accumulation
String currentFilename = "";  //Store the current file name

//Function to send an AT command and retry until +OK is received
void checkATCommand(String command) {
    bool successful = false;
    int maxRetries = 5;  //Set maximum retries if needed

    for (int attempt = 0; attempt < maxRetries && !successful; attempt++) {
        Serial.print("Sending to LoRa: ");
        Serial.println(command);
        loraSerial.println(command);

        long endTime = millis() + 5000;  //5 seconds timeout
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
            delay(1000);  //Wait before retrying
        }
    }

    if (!successful) {
        Serial.println("Command failed after maximum retries: " + command);
        digitalWrite(LED_PIN1, LOW); //Turn off LoRa success LED if initialization fails
        digitalWrite(LED_PIN4, HIGH);  //Indicate fail state
        return;  //Stop further execution if LoRa fails to initialize
    } else {
        digitalWrite(LED_PIN1, HIGH); //Indicate successful LoRa initialization
    }
}

//Initialize the LoRa module
void initializeLoRaModule() {
    checkATCommand("AT\r\n");
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
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_PIN1, OUTPUT);
    pinMode(LED_PIN2, OUTPUT);
    pinMode(LED_PIN3, OUTPUT);
    pinMode(LED_PIN4, OUTPUT);

    //Attempt to initialize the SD card
    if (!SD.begin(chipSelect)) {
        Serial.println("Error: SD card initialization failed!");
        digitalWrite(LED_PIN, LOW); //Indicate SD card initialization failure
        digitalWrite(LED_PIN4, HIGH); //Indicate critical fail state
        return; //Stop further execution if SD card fails to initialize
    } else {
        Serial.println("SD card initialized successfully.");
        digitalWrite(LED_PIN, HIGH); //Indicate SD card initialization success
    }

    loraSerial.begin(115200); //Initialize serial communication with LoRa module

    //Initialize LoRa module using AT commands
    initializeLoRaModule();
    if (!digitalRead(LED_PIN1)) { //Check if the LoRa initialization was successful
        Serial.println("LoRa initialization failed, stopping setup.");
        digitalWrite(LED_PIN4, HIGH); //Indicate critical fail state
        return; //Stop further execution if LoRa fails to initialize
    }
    Serial.println("LoRa module initialized. Awaiting commands...");
}

String generateUniqueFilename() {
    String baseName = "csvDAQ";
    String extension = ".txt";
    if (!SD.exists(baseName + extension)) {
        return baseName + extension;
    }

    for (int i = 1; i < 1000; i++) {
        String newFilename = baseName + String(i) + extension;
        if (!SD.exists(newFilename)) {
            return newFilename;
        }
    }

    return "file_error.txt";  //Return this if a unique name can't be found (unlikely with a limit of 1000 iterations).
}

//Global variable to keep track of the open file
File dataFile;

void loop() {
    unsigned long currentMillis = millis();

    //Handle incoming LoRa messages
    if (loraSerial.available()) {
        String line = loraSerial.readStringUntil('\n');
        line.trim();  //Clean up the input

        if (line.startsWith("+RCV=")) {
            int firstCommaIndex = line.indexOf(',');
            int secondCommaIndex = line.indexOf(',', firstCommaIndex + 1);
            int thirdCommaIndex = line.indexOf(',', secondCommaIndex + 1);
            String data = line.substring(secondCommaIndex + 1, thirdCommaIndex);
            data.trim();  //Trim any leading or trailing whitespace

            //Handle commands based on the data received
            if (data.startsWith("DUTY_CYCLE:")) {
                float newDuty = data.substring(11).toFloat();
                if (newDuty >= 0.1 && newDuty <= 1.0) {
                    duty_cycle = newDuty;
                    Serial.println("Duty cycle updated to: " + String(duty_cycle));
                } else {
                    Serial.println("Received invalid duty cycle value.");
                }
            } else if (data.equalsIgnoreCase("START")) {
                if (!startDAQ) {
                    startDAQ = true;
                    digitalWrite(LED_PIN2, HIGH);  //DAQ started
                    currentFilename = generateUniqueFilename();
                    dataFile = SD.open(currentFilename, FILE_WRITE);
                    if (dataFile) {
                        Serial.println("Logging started in file: " + currentFilename);
                    } else {
                        Serial.println("Failed to create file.");
                        digitalWrite(LED_PIN4, HIGH); //Indicate file creation failure
                        startDAQ = false;  //Prevent data acquisition if file cannot be created
                    }
                }
            } else if (data.equalsIgnoreCase("STOP")) {
                if (startDAQ) {
                    startDAQ = false;
                    digitalWrite(LED_PIN2, LOW);  //DAQ stopped
                    if (dataFile) {
                        dataFile.close();
                        dataFile = File();  //Reset the file handle
                    }
                    digitalWrite(LED_PIN4, HIGH);  //Indicate successful STOP command
                    Serial.println("DAQ stopped via remote command.");
                }
            }
        }
    }

    //Data Acquisition and Logging
    if (startDAQ) {
        //Read analog inputs
        digitalWrite(on_off, HIGH);
        delay(10);
        adci0 = analogRead(batt_in);
        adci1 = analogRead(hc_in);

        //Convert ADC values to voltages
        adcv0 = (adci0 * ref_volts) / 1024.0;
        adcv1 = (adci1 * ref_volts) / 1024.0;

        //Calculate voltage divider input
        batt_volts = adcv0 / (R2 / (R1 + R2));
        hc_volts = adcv1 / (R2 / (R1 + R2));

        //Read and calculate current
        Vout = 0;
        for (int i = 0; i < 1000; i++) {
            Vout += (resADC * analogRead(A2));
            delay(1);
        }
        Vout /= 1000;
        Current = (Vout - 2.49) / (.066 * 2); //This is set up for 4S, for 2S remove the "*2"

        //Read and calculate thermistor temperatures
        V0 = analogRead(A3);
        V0 = analogRead(A3); //A3 is intentionally read twice to overcome an anomaly
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

        //Thermistor circuit //Voltage divider network //Series Circuit
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

        //Beta Parameter Equation
        T0 = B / log(R20 / r_inf) - K;
        T1 = B / log(R21 / r_inf) - K;
        T2 = B / log(R22 / r_inf) - K;
        T3 = B / log(R23 / r_inf) - K;
        T4 = B / log(R24 / r_inf) - K;
        T5 = B / log(R25 / r_inf) - K;
        T6 = B / log(R26 / r_inf) - K;
        T7 = B / log(R27 / r_inf) - K;
        T8 = B / log(R28 / r_inf) - K;
        T9 = B / log(R29 / r_inf) - K;
        T10 = B / log(R210 / r_inf) - K;
        T11 = B / log(R211 / r_inf) - K;
        T12 = B / log(R212 / r_inf) - K;

        //Detect thermal load on thermistors
        if (T0 > 294.15 || T1 > 294.15) {
            digitalWrite(LED_PIN3, HIGH);
        } else {
            digitalWrite(LED_PIN3, LOW);
        }

        //Prepare the data string
        csv = String(runningTime)+","+String(dT)+","+String(batt_volts, 2)+","+String(hc_volts, 2)+","+String(Current, 2)+","+String(duty_cycle)+","+String(T0)+","+String(T1)+","+String(T2)+","+String(T3)+","+String(T4)+","+String(T5)+","+String(T6)+","+String(T7)+","+String(T8)+","+String(T9)+","+String(T10)+","+String(T11)+","+String(T12);
        //Check each thermistor temperature and replace -273.15 with 'Off'
//     String T0_csv = (T0 == -273.15) ? "Off" : String(T0);
        String T1_csv = (T1 == -273.15) ? "Off" : String(T1);
        String T2_csv = (T2 == -273.15) ? "Off" : String(T2);
        String T3_csv = (T3 == -273.15) ? "Off" : String(T3);
        String T4_csv = (T4 == -273.15) ? "Off" : String(T4);
        String T5_csv = (T5 == -273.15) ? "Off" : String(T5);
        String T6_csv = (T6 == -273.15) ? "Off" : String(T6);
        String T7_csv = (T7 == -273.15) ? "Off" : String(T7);
        String T8_csv = (T8 == -273.15) ? "Off" : String(T8);
        String T9_csv = (T9 == -273.15) ? "Off" : String(T9);
        String T10_csv = (T10 == -273.15) ? "Off" : String(T10);
        String T11_csv = (T11 == -273.15) ? "Off" : String(T11);
        String T12_csv = (T12 == -273.15) ? "Off" : String(T12);

        csv += "," + T0_csv + "," + T1_csv + "," + T2_csv + "," + T3_csv + "," + T4_csv + "," + T5_csv + "," + T6_csv + "," + T7_csv + "," + T8_csv + "," + T9_csv + "," + T10_csv + "," + T11_csv + "," + T12_csv;

        //Log to Serial
        Serial.println(csv);

        //Write data to the SD card
        if (dataFile) {
            dataFile.println(csv);
    // } else {
    //     Serial.println("Failed to write to file.");
            digitalWrite(LED_PIN4, HIGH); //Indicate SD card write failure
    // }
//
        //Update time variables
        dT = millis() - runningTime;
        runningTime = millis();

        digitalWrite(on_off, LOW);
        delay(1200 / duty_cycle - 1200);
    }

    //Periodic LoRa Update
if (startDAQ && (currentMillis - lastLoRaSendTime >= 30000)) {
    lastLoRaSendTime = currentMillis;
    //Send the latest data point over LoRa
    String latestData = String(runningTime) + "," + String(batt_volts, 2) + "," + String(hc_volts, 2) + "," + String(Current, 2) + "," + String(duty_cycle);
    
    //Check each thermistor temperature and replace -273.15 with 'Off'
    String T0_str = (T0 == -273.15) ? "Off" : String(T0);
    String T1_str = (T1 == -273.15) ? "Off" : String(T1);
    String T2_str = (T2 == -273.15) ? "Off" : String(T2);
    String T3_str = (T3 == -273.15) ? "Off" : String(T3);
    
    latestData += "," + T0_str + "," + T1_str + "," + T2_str + "," + T3_str + "," + currentFilename;
    loraSerial.print("AT+SEND=222," + String(latestData.length()) + "," + latestData + "\r\n");
}

void stopDataAcquisition() {
    if (startDAQ) {  //Only try to stop if DAQ is currently running
        if (dataFile) {
            dataFile.close();  //Close the SD file to save all data properly
            dataFile = File();  //Reset the file handle to ensure it's no longer referenced
        }
        startDAQ = false;  //Set the DAQ flag to false to stop data acquisition
        digitalWrite(LED_PIN2, LOW); //Turn off the DAQ activity LED to indicate it's stopped
        digitalWrite(LED_PIN4, HIGH);  //Optionally, turn on an error LED to indicate manual stop
        Serial.println("Data acquisition manually stopped and file closed.");
    }
}

float intBitsToFloat(unsigned long value) {
    //Convert integer bits representation back to float
    union {
        float f;
        unsigned long l;
    } data;
    data.l = value;
    return data.f;
}