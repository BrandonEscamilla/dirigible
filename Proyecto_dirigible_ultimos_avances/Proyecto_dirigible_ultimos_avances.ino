#include <SFE_BMP180.h>
#include <Wire.h>
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>


#define BLYNK_PRINT Serial  // Comment this out to disable prints and save space
#define EspSerial Serial1
SFE_BMP180 bmp180;

ESP8266 wifi(&EspSerial);
BlynkTimer timerReadAlt;

// Attach virtual serial terminal to Virtual Pin V1
WidgetTerminal terminal(V2);
String mts;
int VA; 
int referencia;
int sensor; 
int AR;
int retro = VA; // VA sera el valor asignado para la diferencia de el error
const float refPin = AR;// referencia o set-point
int ActuadorPos=9;   // el pin 9 sera actuador motor
int ActuadorNeg=10;   // el pin 10 (salida pwm)se llamarÃ¡ actuador
const float Kp =0.5;//ganancia proporcional 
float error=0;
char x;
double Po; //presion del punto inicial para h=0;
char status;
double T,P,RA,A;

// You should get Auth Token in the Blynk App.
char auth[] = "5c2ebcc6a3ab4221983424df769cc3be";
#define ESP_SSID "aa4a92" // Your network name here
#define ESP_PASS "273065395" // Your network password here

void setup(){

pinMode(ActuadorPos, OUTPUT);
pinMode(ActuadorNeg, OUTPUT);

 Serial.begin(115200);  // Set console baud rate
 while (!Serial);
 EspSerial.begin(115200);  // Set ESP8266 baud rate
 while (!EspSerial);
 
 if (bmp180.begin())
  {
    Serial.println("Iniciando correctamente; Tomando medidas del punto de referncia del dirigible...");
    Serial.println("                                                                                            ");
    status = bmp180.startTemperature();//Inicio de lectura de temperatura
    if (status != 0)
    {   
      delay(status); //Pausa para que finalice la lectura
      status = bmp180.getTemperature(T);//Obtener la temperatura
      if (status != 0)
      {
        status = bmp180.startPressure(3);//Inicio lectura de presión
        if (status != 0)
        {        
          delay(status);//Pausa para que finalice la lectura        
          status = bmp180.getPressure(P,T);//Obtenemos la presión
          if (status != 0)
          {                  
            Po = P; //Asignamos el valor de presión como punto de referencia
            Serial.print("Presión inicial: ");
            Serial.println(Po);
            Serial.println("Envia una Alturaa entre 0 y 9 metros");
          }      
        }      
      }   
    }
    
  }
  else
  {
    Serial.println("Error al iniciar el BMP180");
    while(1); // bucle infinito
  }
  
 Blynk.begin(auth, wifi, ESP_SSID,ESP_PASS);
 timerReadAlt.setInterval(1000, readAlt);

 terminal.println(F("Blynk v" BLYNK_VERSION ": Device started"));
 terminal.println(F("-------------"));
 terminal.print("Presión inicial: ");
 terminal.println(Po);
 terminal.flush();
}

void readAlt(){
  status = bmp180.startTemperature();//Inicio de lectura de temperatura
  if (status != 0)
  {   
    delay(status); //Pausa para que finalice la lectura
    status = bmp180.getTemperature(T);//Obtener la temperatura
    if (status != 0)
    {
      Blynk.virtualWrite(V0,T);
      for(x=0;x<25;x++){
        status = bmp180.startPressure(3);//Inicio lectura de presión
        if (status != 0)
        {        
          delay(status);//Pausa para que finalice la lectura        
          status = bmp180.getPressure(P,T);//Obtenemos la presión
          if (status != 0)
          {                    
            //-------Calculamos la altura con respecto al punto de referencia--------
            Serial.print("+");
            A += bmp180.altitude(P,Po);            
          }      
        }
      }
      RA = (A/30);
      Serial.print("RA: ");
      Serial.println(RA);
      Blynk.virtualWrite(V6,P);
      Blynk.virtualWrite(V5,(RA));      
    }   
  } 
}

BLYNK_WRITE(V2)
{

  mts = param.asStr();
  Serial.println(mts);
  terminal.print("Sigues conectado:");
  terminal.write(param.getBuffer(), param.getLength());
  terminal.println(); 
  // Ensure everything is sent
  terminal.flush();
}

void loop(){
 Blynk.run();
 timerReadAlt.run(); // Initiates BlynkTimer

//A= AR;

int sensor=analogRead(retro);
int referencia=analogRead(refPin);
error=referencia-sensor;
int uP=Kp*error;
if (uP > 0) {
uP=constrain(uP,0,255);
analogWrite(ActuadorPos,uP);  // pin 9 arduino,  pin 10  PH     
analogWrite(ActuadorNeg,0); //  pin 10 arduino,   pin 15 PH 
     }
 if (uP <= 0){
uP=constrain(uP,-255,0);
analogWrite(ActuadorPos,0); //  pin 10 arduino,  pin 2  PH     
analogWrite(ActuadorNeg,abs(uP));   // pin 6 arduino,   pin 15 PH ;

 if (Serial.available()) {
         // si es asi, lo lee, debe ser un valor entre 0 y 9):
         VA = Serial.parseInt();
         // mapea el valor de entrada a un valor entre 0 y 255
         VA = map(VA, 0, 9, 0, 10);
         // limitamos el valor entre 0 y 255
         VA = constrain(VA, 0, 10);
         // envia por el serial el valor convertido a un valor entre 0 y 255
         Serial.println(VA);
 }
         // enciende el led con un valor entre 0 y 255
 }         //analogWrite(pinLed, brillo);

 
}

