#include <SFE_BMP180.h>
#include <Wire.h>
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
//Configuracion de salida de Blynk al serial habilitado para debug
#define BLYNK_PRINT Serial  // Comment this out to disable prints and save space
//Asignación de serial al modulo de wifi
#define EspSerial Serial1
//Crea intancia de BMP180
SFE_BMP180 bmp180;
ESP8266 wifi(&EspSerial);

// Asignación de pin virtual V2 a la terminal de Blynk
WidgetTerminal terminal(V2);
BlynkTimer timerSendAlt;

// Variables Globales
unsigned char referenciaAltura = 0;
double Po, T, P; //presion del punto inicial para h=0;
unsigned char status;
short altitud;

//Variables globales de PID
const float Kp =.5;//ganancia proporcional como una constante qÂ´puede tener decimales
const float Ki =.03;//ganancia integral
const unsigned char Td = 1;//ganancia derivativa
unsigned long lastTime;//variable de tiempo auxiliar para muestreos uniformes
float errorActual = 0;// el error del sistema
float errorAcum = 0;
float ultimoError = 0;
unsigned char Tm = 170; // periodo de muestreo T deseado en milisegundos
const unsigned char numReadings = 5;
short readings[numReadings]; // the readings from the analog input
unsigned char index = 0;          // the index of the current reading
unsigned char total = 0;         // the running total
double average = 0;   // the average

//Configuración de salida a motor
// pin 9(A) - pin 10(B)
//   0           0     -- Motor Apagado
//   0           1     -- Gira motor sentido horario
//   1           0     -- Gira motor sentido anti-horario
//   1           1     -- NS

const unsigned char A = 9;
const unsigned char B = 10;
const unsigned char EN = 11;

// Configuración de wifi
char auth[] = "5c2ebcc6a3ab4221983424df769cc3be";
#define ESP_SSID "ARRIS-BD92" // Your network name here
#define ESP_PASS "5793D2ECE1F316DC" // Your network password here

void setup(){
    for (unsigned char thisReading = 0; thisReading < numReadings; thisReading++){
        readings[thisReading] = 0;  
    }

    pinMode(A, OUTPUT);
    pinMode(B, OUTPUT);
    //pinMode(EN, OUTPUT);
    //analogWrite(A , 255);//escribir la acciÃ³n de control uP en el pin 9 del actuador
    digitalWrite(B , 0);//escribir la acciÃ³n de control uP en el pin 9 del actuador

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
    
    Serial.println("Iniciando Blynk");
    Blynk.begin(auth, wifi, ESP_SSID,ESP_PASS);
    Serial.println("Blynk Iniciado");
    timerSendAlt.setInterval(500, sendAlt);

    terminal.println(F("Blynk v" BLYNK_VERSION ": Device started"));
    terminal.println(F("-------------"));
    terminal.print("Presión inicial: ");
    terminal.println(Po);
    terminal.flush();
}

void sendAlt(){
    //Serial.print("altitud: ");
    //Serial.println(altitud);
    Blynk.virtualWrite(V0,T);
    Blynk.virtualWrite(V6,P);
    Blynk.virtualWrite(V5,(altitud));
}

BLYNK_WRITE(V2){
    referenciaAltura = param.asInt();
    Serial.println(referenciaAltura);
    terminal.print("Sigues conectado:");
    terminal.write(param.getBuffer(), param.getLength());
    terminal.println(); 
    // Ensure everything is sent
    terminal.flush();
}

short getAltitud(){
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
                    //-------Calculamos la altura con respecto al punto de referencia--------
                    altitud = bmp180.altitude(P,Po);            
                }      
            }
        }   
    }

    return altitud;
}


void loop(){
    Blynk.run();
    //timerSendAlt.run(); // Initiates BlynkTimer
    //subtract the last reading:
    total -= readings[index];         
    //read from the alturaActual:  Blocking for 34ms aprox
    readings[index] = getAltitud();
    //add the reading to the total:
    total += readings[index];       
    //advance to the next position in the array:  
    index++;                    

    // if we're at the end of the array...
    if (index >= numReadings){
        index = 0;                           
    }
    // calculate the average:
    average = total / numReadings;         
    short alturaActual = average;
    unsigned long now = millis(); //now sera el tiempo transcurrido desde el arranque
    short timeChange = now - lastTime; // tiempo transcurrido
    
    if(timeChange >= Tm){   //hasta que el tiempo transcurrido sea >T se calcula el resto 
        errorActual = referenciaAltura - alturaActual; // calculo del error
        int uP = Kp*errorActual;
        if (uP >= 0) {
            analogWrite(A,0);  // pin 9 arduino,  pin 10  PH     
            analogWrite(B,0); //  pin 10 arduino,   pin 15 PH 
        }
        
        if (uP < 0){
            analogWrite(A,255); //  pin 10 arduino,  pin 2  PH     
            analogWrite(B,0);
        }
        // errorAcum += errorActual;//+ultimoError;
        // float difError = errorActual - ultimoError;
        // int uP = (errorActual)*Kp+(errorAcum)*Ki+(difError)*Td;//calculo de ley de controll proporcional al error,a la sumatoria del error (integral) y al cambio del error;
        // uP = constrain(uP,0,255);//limitar el valor de uP a lim inf 0 y lim sup 255
        // analogWrite(A , uP);//escribir la acciÃ³n de control uP en el pin 9 del actuador
        Serial.print(alturaActual);
        Serial.print("\t");
        Serial.print(referenciaAltura);
        Serial.print("\t");
        Serial.print(uP);
        Serial.print("\t");
        Serial.println(lastTime);
        
        // ultimoError = errorActual;
        lastTime = now;
    }// grabar el ultimo tiempo medido en la variable lastime
}

