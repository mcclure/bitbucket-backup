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

#define SEGMENTS 2
bool isOn = false;
int lastIn = 0;
int lastOut = 0;

void loop() {
  bool current = digitalRead(INSIG) == HIGH;
  int t = millis();
  
#if DEBUG
  Serial.print("Loop "); Serial.print(digitalRead(INSIG)); Serial.print(current ? "T" : "F"); Serial.print("\n");
#endif

  if (!isOn && current) {
    lastIn = t;
    
  }
  
  isOn = current;
}
