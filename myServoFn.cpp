#include <Servo.h>

static const int servoPin = 12;
static int degreePos = 0;


void dispenseFn(int turns)
{
    Servo servo1;
    servo1.attach(servoPin);

    for (int i = 0; i < turns; i ++)
    { 
      if (!(i%2))
      { 
        servo1.read() == 0 ? degreePos = 90 : degreePos = 0;
        servo1.write(degreePos);
        Serial.println("servo rotate");
        Serial.println(degreePos);
        Serial.println("delay 1.5s");
        delay(1500);
      }
      else 
      {
        servo1.read() == 0 ? degreePos = 90 : degreePos = 0;
        servo1.write(degreePos);
        Serial.println("servo rotate");
        Serial.println(degreePos);
        Serial.println("delay 1.5s");
        delay(1500);
      }
    }
}
