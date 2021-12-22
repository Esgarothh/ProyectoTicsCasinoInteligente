/*
   -------------------------------------------------------------------------------------
   HX711_ADC
   Arduino library for HX711 24-Bit Analog-to-Digital Converter for Weight Scales
   Olav Kallhovd sept2017
   -------------------------------------------------------------------------------------
*/

/*
   This example file shows how to calibrate the load cell and optionally store the calibration
   value in EEPROM, and also how to change the value manually.
   The result value can then later be included in your project sketch or fetched from EEPROM.

   To implement calibration in your project sketch the simplified procedure is as follow:
       LoadCell.tare();
       //place known mass
       LoadCell.refreshDataSet();
       float newCalibrationValue = LoadCell.getNewCalibration(known_mass);
*/

#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

//pins:
const int HX711_dout = 2; //mcu > HX711 dout pin
const int HX711_sck = 3; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;
float sumador = 0;
int cantidad = 0;
int promAnterior = 0;
int pesoSacado = 0;
int comidaActual = 0;
int comidaInicial = 0;
boolean llenar = false;
char incoming;
boolean procesoCompra = true; 
boolean solicitud = true;

int contador_luces = 0;

void setup() {
  
  pinMode(13, OUTPUT); //luz azul
  pinMode(7, OUTPUT); //luz roja 1
  pinMode(8, OUTPUT); //luz amarilla 1
  pinMode(9, OUTPUT); //luz amarilla 2
  pinMode(10, OUTPUT); //luz amarilla 3
  Serial.begin(19200); delay(10);
  Serial.println();
  Serial.println("Starting...");

  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  unsigned long stabilizingtime = 5000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(1.0); // user set calibration value (float), initial value 1.0 may be used for this sketch
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());
  calibrate(); //start calibration procedure
}


void loop() {

  boolean loopbool = false;
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity
  // check for new data/start next conversion:
   
  if(llenar == false){
  Serial.println("Coloque el alimento en la bandeja ('k' una vez listo):");
  while (loopbool == false) { //llenar bandeja
    if (Serial.available() > 0) {
      char aux = Serial.read(); 
      if (aux == 'k') {
        LoadCell.refreshDataSet();
        comidaActual = LoadCell.getData();
        comidaInicial = comidaActual;
        Serial.print("Peso ingresado: ");
        Serial.println(comidaInicial);
        loopbool = true;
        llenar = true;
      }
    }
  }
    }
  
  if(Serial.available() > 0) {
    //serial.println("leer algo -arduino");
    char incoming = Serial.read();
    Serial.println(incoming);
  
  if(incoming == 'a') {
    luz_led_compra(true);
    Serial.println("confirmacion");
      }
  else if(incoming == 'b'){
    Serial.println("confirmacion");
    luz_led_compra(false);
      }
  else{
    incoming='c';
  }
  
  }


  
   if (LoadCell.update()) newDataReady = true;
  bool h = false;
  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      
      sumador = sumador + i;
      cantidad = cantidad + 1;
      if (cantidad > 10){

        promAnterior = sumador/cantidad;
        sumador =0;
        cantidad = 0;
        if(solicitud){
        Serial.print("Load_cell output val: ");
        Serial.println(promAnterior);
        }
        h = true;
      }

        
        if(h)
        {                   
          int aux = comidaActual - promAnterior;
          if((aux)>=50){ //faltan 100
            pesoSacado += aux;
            comidaActual -= aux;
            contador_luces++;
          }
          else if(aux <= -50){ //se regreso
            pesoSacado -= aux;
            comidaActual += (aux*-1);
            contador_luces--;
          }
        
        luz_estados_compra(contador_luces);
        
        Serial.print("ComidaInicial: ");
        Serial.println(comidaInicial);
        Serial.print("promAnterior: ");
        Serial.println(promAnterior);
        Serial.print("pesoSacado: ");
        Serial.println(pesoSacado);
        Serial.print("comidaActual: ");
        Serial.println(comidaActual);
        h = false;
        }

        
      newDataReady = 0;
      t = millis();
    }
  }

  // receive command from serial terminal
 

}

void luz_led_compra(boolean a){
  if(a){
    digitalWrite(13, HIGH);    
        }
  else{
    digitalWrite(13, LOW);   
  }
}


void luz_estados_compra(int cant){
  
  if(cant == 3){
    digitalWrite(8, HIGH);
    digitalWrite(9, HIGH);
    digitalWrite(10, HIGH);    
        }
  if(cant == 2){
    digitalWrite(8, HIGH);
    digitalWrite(9, HIGH);
    digitalWrite(10, LOW);    
        }
  if(cant == 1){
    digitalWrite(8, HIGH);
    digitalWrite(9, LOW);
    digitalWrite(10, LOW);    
        }
  if(cant == 0){
    digitalWrite(8, LOW);
    digitalWrite(9, LOW);
    digitalWrite(10, LOW);    
        }
  
  }

void calibrate() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");

  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 't') LoadCell.tareNoDelay();
      }
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
  }

  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); //get the new calibration value

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;

      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }

  Serial.println("End calibration");
  Serial.println("***");
  Serial.println("FINAL");
}

void changeSavedCalFactor() {
  float oldCalibrationValue = LoadCell.getCalFactor();
  boolean _resume = false;
  Serial.println("***");
  Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor, i.e. 696.0");
  float newCalibrationValue;
  while (_resume == false) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        _resume = true;
      }
    }
  }
  _resume = false;
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }
  Serial.println("End change calibration value");
  Serial.println("***");
}
