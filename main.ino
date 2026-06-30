#include <SDI12.h>

#define SDI12_DATA_PIN 7   // blue wire -> D7 (any digital pin except D0/D1)

uint32_t serialBaud    = 115200;
int8_t   dataPin       = SDI12_DATA_PIN;
char     sensorAddress = '0';   // HydraProbe factory default address

SDI12 mySDI12(dataPin);
String myCommand   = "";
String serialInput = "";
String values_names[9] = {"WFV", "Bulk EC (TC)", "Temp (C)", "Temp (F)", "Bulk EC", "Eps_r", "Eps_i", "Pore EC", "Loss tan"};

int parseSDI12Values(String response, float* out, int idx, int maxVals) {
  int i = 0;
  int n = response.length();

  // skip address + any junk before the first value's sign
  while (i < n && response[i] != '+' && response[i] != '-') i++;

  while (i < n && idx < maxVals) {
    int start = i;           // points at a '+' or '-'
    i++;                     // step past the sign
    while (i < n && response[i] != '+' && response[i] != '-') i++;
    out[idx++] = response.substring(start, i).toFloat();
  }
  return idx;
}

String issueCommand(String cmd) {
  static String sdiResponse;
  sdiResponse = "";
  Serial.println(cmd);
  mySDI12.sendCommand(cmd);
  delay(100);

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
  String commandResult = issueCommand(cmd);
  printCommandResult(commandResult);
  return commandResult;
}

void getMeasurement(char sensorAddress) {

  // 1) Start a measurement
  myCommand = String(sensorAddress) + "M!";
  handleCommand(myCommand);
  delay(3000);          // give the sensor time to finish the measurement
  String measurementResult = "";
  String measurements[9];

  // 2) Request the data from that measurement
  float values[9];
  int count = 0;

  count = parseSDI12Values(handleCommand("0D0!"), values, count, 9);
  count = parseSDI12Values(handleCommand("0D1!"), values, count, 9);
  count = parseSDI12Values(handleCommand("0D2!"), values, count, 9);

  for (int i = 0; i < count; i++) {
    if (i == 3) continue; // Skip Farenheit Measurement
    Serial.print(values_names[i]);
    Serial.print(" = ");
    Serial.println(values[i], 3);   // 3 decimal places
  }

  // 3) Print measurement results
 
}

void getInfo(char sensorAddress) {
  handleCommand((String)sensorAddress + "I!");
  delay(1000);
}

void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L);

  Serial.println("Opening SDI-12 bus...");
  mySDI12.begin();
  delay(500);  // allow things to settle
  Serial.println("SDI-12 bus Ready!");
  // No power-pin block: the HydraProbe is powered directly from the 12V adapter.
  getInfo(sensorAddress);
}

void loop() {
  //Serial.write("1) Get Sensor Information\n2) Get Measurements\n\r\n");
  do {  // wait for the user to hit enter in the serial monitor
    delay(30);
  } while (!Serial.available());
  if (Serial.available()) {
    String serialInput = Serial.readStringUntil('\n');  // read until newline
    serialInput.trim();                                 // strip trailing \r and spaces
    getMeasurement(sensorAddress);
    /*switch (int(serialInput[0])-int('0')) {
      case 1:
        getInfo(sensorAddress);
        break;
      case 2:
        getMeasurement(sensorAddress);
        break;
    }*/
  }

}