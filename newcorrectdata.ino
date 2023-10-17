// Define all library
#include <SoftwareSerial.h>
#include <SPIMemory.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <MPU6050.h>

// serial communication between flash and esp32
SPIFlash flash(5, &SPI);
MPU6050 mpu; //define gyro

//serial communication between simmodule and esp32
SoftwareSerial gprsSerial(17, 16);

// define required gyro values
int16_t rawGyroX, rawGyroY, rawGyroZ;
float gyroX, gyroY, gyroZ;
int16_t rawAcceloX, rawAcceloY, rawAcceloZ;
float AcceloX, AcceloY, AcceloZ;
int16_t rawTemp;
float temp;
String api_Key1 = "QESOPC1EZE384OFJ";
String server1 = "GET https://api.thingspeak.com/update?api_key=";
// deine task name on different core
TaskHandle_t Task1;
TaskHandle_t  Task2;

const int STACK_SIZE = 100; // Adjust the stack size as needed
unsigned long addressStack[STACK_SIZE];
int stackIndex = 0;


void setup() {
  Serial.begin(9600);
  flash.begin();
  Wire.begin();
  mpu.initialize();
  gprsSerial.begin(9600);

  gprsSerial.println("AT+CPIN?");
  delay(1000);
 
  gprsSerial.println("AT+CREG?");
  delay(1000);
 
  gprsSerial.println("AT+CGATT?");
  delay(1000);
 
  gprsSerial.println("AT+CIPSHUT");
  delay(1000);
 
  gprsSerial.println("AT+CIPSTATUS");
  delay(2000);
 
  gprsSerial.println("AT+CIPMUX=0");
  delay(2000);
 
  //ShowSerialData();
  gprsSerial.println("AT+CSTT=\"airtelgprs.com\"");//start task and setting the APN,
  delay(1000);
 
  //ShowSerialData();
 
  gprsSerial.println("AT+CIICR");//bring up wireless connection
  delay(1000);
 
  //ShowSerialData();
 
  gprsSerial.println("AT+CIFSR");//get local IP adress
  delay(1000);
 
 // ShowSerialData();
 
  gprsSerial.println("AT+CIPSPRT=0");
  delay(1000);
 

  xTaskCreatePinnedToCore(send_real_time_data, "Task1", 10000, NULL, 1, &Task1, 0);                        
  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(send_stored_to_server, "Task2", 10000, NULL, 1, &Task2, 1);     


}

bool checkInternetConnectivity() {
  gprsSerial.println("AT+CGATT?");
  delay(1000);
  String response = "";

while (gprsSerial.available()) {
    char c = gprsSerial.read();
    response += c;
    Serial.print(c);
}
   return response.indexOf("+CGATT: 1") != -1;  // Returns true if GPRS is attached (internet is available)
}


void gyro_data(){
   mpu.getRotation(&rawGyroX, &rawGyroY, &rawGyroZ);
   mpu.getAcceleration(&rawAcceloX, &rawAcceloY, &rawAcceloZ);
   rawTemp = mpu.getTemperature();

   AcceloX = rawAcceloX / 16384.0;
   AcceloY = rawAcceloY / 16384.0;
   AcceloZ = rawAcceloZ / 16384.0;

   gyroX = rawGyroX / 131.0;
   gyroY = rawGyroY / 131.0;
   gyroZ = rawGyroZ / 131.0;

   temp = ((rawTemp) / 340.0) + 36.53;
}

void sendDataToThingsSpeak(String server, String api_Key, int field1, int field2, int field3, int field4){
  
   gprsSerial.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");//start up the connection
   delay(4000);
   
 
   gprsSerial.println("AT+CIPSEND");//begin send data to remote server
   delay(4000);
   

   String str = server + api_Key + "&field1=" +String (field1) + "&field2=" + String(field2) + "&field3=" +String(field3) + "&field4=" + String(field4);

   Serial.println(str);
   gprsSerial.println(str);
   
 
   gprsSerial.println((char)26);//sending
   delay(5000);//waitting for reply, important! the time is base on the condition of internet 
   gprsSerial.println();
 
}

void ShowSerialData()
{
  while(gprsSerial.available()!=0)
  Serial.write(gprsSerial.read());
  delay(5000); 
  
}

void send_real_time_data( void * pvParameters ){

  for(; ;){
   gyro_data();

  if (checkInternetConnectivity()) {
    Serial.println("Internet is available");
    sendDataToThingsSpeak("GET https://api.thingspeak.com/update?api_key=", "QESOPC1EZE384OFJ", AcceloX, AcceloY, AcceloZ, temp);
  } else {
    Serial.println("Internet is not available");
  }
}
}
void send_stored_to_server ( void * pvParameters ){
 for(; ;){
 Serial.print(F("Flash size: "));
 Serial.print((long)(flash.getCapacity() / 1000));
 Serial.println(F("Kb"));


  if (checkInternetConnectivity()) {

    Serial.println("Internet is available");
    sendDataToThingsSpeak("GET https://api.thingspeak.com/update?api_key=", "QESOPC1EZE384OFJ", AcceloX, AcceloY, AcceloZ, temp);

  } 
  else {
   Serial.println("Internet is not available");
   Serial.println("Data stored in flash memory");
   gyro_data();

   int strAddrSecondString = 0;

   Serial.println();
 
   DynamicJsonDocument gyroDoc(512);
   gyroDoc["gyroX"] = gyroX;
   gyroDoc["gyroY"] = gyroY;
   gyroDoc["gyroZ"] = gyroZ;
   gyroDoc["acceloX"] = AcceloX;
   gyroDoc["acceloY"] = AcceloY;
   gyroDoc["acceloZ"] = AcceloZ;
   gyroDoc["Temp"] = temp;

   strAddrSecondString = flash.getAddress(gyroDoc.size());
   Serial.println(F("Generate JSON file!"));

  // Serialize JSON to file
   String gyrobuf;
   serializeJson(gyroDoc, gyrobuf);

   if (flash.writeStr(strAddrSecondString, gyrobuf)) {
    Serial.print(F("OK, wrote on address "));
    Serial.println(strAddrSecondString);
   } else {
    Serial.println(F("KO"));
   }

   }










}

}






























































































void loop() {}