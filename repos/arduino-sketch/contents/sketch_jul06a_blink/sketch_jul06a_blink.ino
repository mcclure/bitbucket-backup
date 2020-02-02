#define PLAY 2
#define PITCH 3

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PLAY, OUTPUT);
  pinMode(PITCH, OUTPUT);
}

static int i = 0, p = 0;
const int rate = 300;
const int highTime = 80;

void high() {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(PLAY, HIGH);
}

void low() {
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(PLAY, LOW);
}

const char *pattern =
 "XXXX X XX X XX X";

void loop() {
  char v = pattern[i];
  if (!v) {
    i = 0;
    v = pattern[i];
  }

  if (v == 'X') {
    analogWrite(PITCH, p % 256);
    high();
    delay(highTime);
    low();
    delay(rate-highTime);
  } else {
    delay(rate);
  }
  i++;
  p++;
}
