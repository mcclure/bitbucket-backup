#include <ctype.h>
#include <math.h>

#define INSIG 3
#define OUTSIG 5
#define DEBUG 0

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(INSIG, INPUT);
  pinMode(OUTSIG, OUTPUT);
#if DEBUG
  Serial.begin(9600);
#endif
}

void high() {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(OUTSIG, HIGH);
}

void low() {
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(OUTSIG, LOW);
}

bool isOn = false;
bool silence = true;

void loop() {
  bool current = digitalRead(INSIG) == HIGH;
#if DEBUG
  Serial.print("Loop "); Serial.print(digitalRead(INSIG)); Serial.print(current ? "T" : "F"); Serial.print("\n");
#endif
#if 0
  if (current) high(); else low();
#else
  if (!isOn && current) {
    silence = !silence;
    if (!silence)
      high();
  } else if (isOn && !current) {
    if (!silence)
      low();
  }
  isOn = current;
#endif
}
