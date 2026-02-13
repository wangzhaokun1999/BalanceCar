
#include <Arduino.h>
#include <String.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;
sensors_event_t a, g, temp; // acceleration, vitesse du gyropode, température

char FlagCalcul = 0; // flag pour indiquer que les grandeurs ont été lues et que le calcul peut être effectué par la fonction contrôle
float Te = 5;        // période d'échantillonage en ms
float Tau = 1000;    // constante de temps du filtre en ms

// mesure du MPU
float tetagr, tetaomg, tetaF; // angle de gravité mesuré par accéléromètre, angle, angle une fois filtré

// coefficient du filtre
float A, B;

void controle(void *parameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        mpu.getEvent(&a, &g, &temp);                        // lecture des grandeurs du MPU6050
        tetagr = atan(a.acceleration.x / a.acceleration.y); // calcul de l'angle de gravité à partir des mesures d'accélération
        tetaF = A * tetaF + B * tetagr;                     // filtrage de l'angle de gravité Passe-Bas
        tetaomg = A * Tau / 1000 * g.gyro.z + B * tetaomg;  // filtrage de la vitesse du gyropode Passe-Bas

        FlagCalcul = 1;
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(Te));
    }
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.printf("Bonjour \n\r");

    // Try to initialize de la librarie MPU6050
    if (!mpu.begin())
    {
        Serial.println("Failed to find MPU6050 chip");
        while (1)
        {
            delay(10);
        }
    }
    Serial.println("MPU6050 Found!");

    xTaskCreate(
        controle,   // nom de la fonction
        "controle", // nom de la tache que nous venons de vréer
        10000,      // taille de la pile en octet
        NULL,       // parametre
        10,         // tres haut niveau de priorite
        NULL        // descripteur
    );

    // calcul coeff filtre
    A = 1 / (1 + Tau / Te);
    B = Tau / Te * A;
}

void reception(char ch)
{
    static int i = 0;
    static String chaine = "";
    String commande;
    String valeur;
    int index, length;

    if ((ch == 13) or (ch == 10))
    {
        index = chaine.indexOf(' ');
        length = chaine.length();
        if (index == -1)
        {
            commande = chaine;
            valeur = "";
        }
        else
        {
            commande = chaine.substring(0, index);
            valeur = chaine.substring(index + 1, length);
        }

        if (commande == "Tau")
        {
            Tau = valeur.toFloat();
            // calcul coeff filtre
            A = 1 / (1 + Tau / Te);
            B = Tau / Te * A;
        }
        if (commande == "Te")
        {
            Te = valeur.toInt();
            A = 1 / (1 + Tau / Te);
            B = Tau / Te * A;
        }

        chaine = "";
    }
    else
    {
        chaine += ch;
    }
}

void loop()
{
    if (FlagCalcul == 1)
        // affichage des angles en radians
        Serial.printf("%lf %lf %lf %lf \n", tetagr, tetaomg, tetaF, tetaomg + tetagr);

    FlagCalcul = 0;
}


void serialEvent()
{

    while (Serial.available() > 0) // tant qu'il y a des caractères à lire
    {
        reception(Serial.read());
    }
}