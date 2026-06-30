#include <SDI12.h>

#define SDI12_DATA_PIN 7   // blue wire -> D7 (any digital pin except D0/D1)

uint32_t serialBaud    = 115200;
int8_t   dataPin       = SDI12_DATA_PIN;
char     sensorAddress = '0';   // HydraProbe factory default address

SDI12 mySDI12(dataPin);
String myCommand   = "";
String serialInput = "";
String values_names[9] = {"WFV", "Bulk EC (TC)", "Temp (C)", 
						          "Temp (F)", "Bulk EC", "Eps_r", 
						          "Eps_i", "Pore EC", "Loss tan"};

int parseSDI12Values(String response, float* out, int idx, int maxVals) {

  /**
  * Parse the numeric values out of an SDI-12 "D" response into a float array.
  * Responses look like "0+77.9+0.000+1.462": a leading address, then values that
  * are each prefixed by a '+' or '-' sign. We tokenize on those signs, which also
  * lets us skip the address by ignoring everything before the first one.
  *
  * Appends rather than overwrites, so it can be called for D0!, D1!, D2!... to
  * accumulate into one array. Pass the return value back as `idx` to chain calls.
  * Returns the total number of values stored.
  */

  int i = 0;
  int n = response.length();

  // skip the address: advance to the first sign character
  while (i < n && response[i] != '+' && response[i] != '-') i++;

  while (i < n && idx < maxVals) {
    int start = i;
    i++;  // step past this value's sign so the scan below stops at the *next* one
    while (i < n && response[i] != '+' && response[i] != '-') i++;
    out[idx++] = response.substring(start, i).toFloat();
  }

  return idx;
}

String issueCommand(String cmd) {

  static String sdiResponse;

  // Send command through SDI-12
  Serial.println(cmd);
  mySDI12.sendCommand(cmd);
  delay(100);

  // Receive results
  sdiResponse = "";
  while (mySDI12.available()) {
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse += c;
      delay(10);
    }
  }

  return sdiResponse;

} 

void printCommandResult(String sdiResponse) {

  if (sdiResponse.length() > 1) Serial.println(sdiResponse);

  mySDI12.clearBuffer();

}

String handleCommand(String cmd) {
  
  // Primary function for interfacing with the sensors
  
  Serial.print("$ ");

  String commandResult = issueCommand(cmd);

  printCommandResult(commandResult);

  delay(100);

  return commandResult;

}

void getMeasurement(char sensorAddress) {

  // Start a measurement
  String aMresult = handleCommand(String(sensorAddress) + "M!");
  delay(aMresult.substring(1,4).toInt() * 1000);          // give the sensor time to finish the measurement

  // Request the data from that measurement
  float values[9];
  int count = 0;

  count = parseSDI12Values(handleCommand((String)sensorAddress + "D0!"), values, count, 9);
  count = parseSDI12Values(handleCommand((String)sensorAddress + "D1!"), values, count, 9);
  count = parseSDI12Values(handleCommand((String)sensorAddress + "D2!"), values, count, 9);

  // Print measurement results
  Serial.println("\n----------- RESULTS OF MEASUREMENT ------------");
  for (int i = 0; i < count; i++) {
    if (i == 3) continue; // Skip temperature Farenheit measurement
    Serial.print("\n\t    " + values_names[i] + " = ");
    Serial.println(values[i], 3);   // 3 decimal places
  }
  
}

void getInfo(char sensorAddress) {

  handleCommand((String)sensorAddress + "I!");

}

void setup() {

  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L);

  Serial.println("Opening SDI-12 bus...");
  mySDI12.begin();
  delay(500);  // allow things to settle
  Serial.println("SDI-12 bus Ready!");

  getInfo(sensorAddress);

}

void loop() {

  Serial.println("----- Press ENTER to acquire measurement -----");

  do {  // wait for the user to hit enter in the serial monitor
    delay(30);
  } while (!Serial.available());

  if (Serial.available()) {
    String serialInput = Serial.readStringUntil('\n');  // read until newline
    getMeasurement(sensorAddress);
  }

  Serial.println("\n----------------------------------------------");

}
