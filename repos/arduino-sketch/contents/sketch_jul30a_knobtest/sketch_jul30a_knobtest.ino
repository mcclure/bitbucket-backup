#include <ctype.h>
#include <math.h>

#define DEBUG 0
#define KNOB 0
#define SPEAKER 3

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SPEAKER, OUTPUT);
  //pinMode(KNOB, INPUT)
#if DEBUG
  Serial.begin(9600);
#endif
}

void loop() {
  int value = analogRead(KNOB);
  analogWrite(SPEAKER, value);
#if DEBUG
  Serial.print("Read ");
  Serial.print(value);
  Serial.print("\n");
#endif
  digitalWrite(LED_BUILTIN, value > 512 ? HIGH : LOW);
}
