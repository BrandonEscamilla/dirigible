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
BlynkTimer timerReadAlt;

// Variables Globales
unsigned char mts;
unsigned char x;
double Po, T, P; //presion del punto inicial para h=0;
unsigned char status;
unsigned short Altitud;
unsigned short AltitudPromedio;

// Configuración de wifi
char auth[] = "5c2ebcc6a3ab4221983424df769cc3be";
#define ESP_SSID "ARRIS-BD92" // Your network name here
#define ESP_PASS "5793D2ECE1F316DC" // Your network password here

void setup(){

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
    timerReadAlt.setInterval(500, readAlt);

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
            Altitud=0;
            Blynk.virtualWrite(V0,T);
            for(x=0;x<5;x++){
                status = bmp180.startPressure(3);//Inicio lectura de presión
                if (status != 0)
                {        
                    delay(status);//Pausa para que finalice la lectura        
                    status = bmp180.getPressure(P,T);//Obtenemos la presión
                    if (status != 0)
                    {                    
                        //-------Calculamos la altura con respecto al punto de referencia--------
                        Serial.print("+");
                        Altitud += bmp180.altitude(P,Po);            
                    }      
                }
            }

            AltitudPromedio = (Altitud*0.20);
            Serial.print("AltitudPromedio: ");
            Serial.println(AltitudPromedio);
            Blynk.virtualWrite(V6,P);
            Blynk.virtualWrite(V5,(AltitudPromedio));      
        }   
    } 
}

BLYNK_WRITE(V2){
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
}

