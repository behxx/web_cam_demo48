#include <Servo.h>

static const int servoPin = 12;
static int degreePos;


int dispenseFn(int turns, int lastTurn)
{
    Servo servo1;
    servo1.attach(servoPin);

    for (int i = 0; i < turns; i ++)
    { 
      if (!(i%2))
      { 
        lastTurn == 60 ? degreePos = 150 : degreePos = 60;
        servo1.write(degreePos);
        Serial.println("servo rotate");
        Serial.println(degreePos);
        Serial.println("delay 1.5s");
        lastTurn = degreePos;
        delay(1500);
      }
      else 
      {
        lastTurn == 60 ? degreePos = 150 : degreePos = 60;
        servo1.write(degreePos);
        Serial.println("servo rotate");
        Serial.println(degreePos);
        Serial.println("delay 1.5s");
        lastTurn = degreePos;
        delay(1500);
      }
    }
    Serial.println("Done dispense");
    return lastTurn;
}
