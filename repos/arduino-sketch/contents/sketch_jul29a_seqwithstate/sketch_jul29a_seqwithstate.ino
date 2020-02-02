#include <ctype.h>
#include <math.h>

#define PLAY 2
#define PITCH 3
#define FUNCS 26
#define STACKMAX 4
#define DEBUG 0
#define RATE 300
#define HIGHTIME 80
static int OCTAVE = 255/5;
static int ROOT = OCTAVE*3;

// Number: Play this note
// Parenthesis: Store a pattern, first letter is the label (must be uppercase letter)
// Uppercase letter: Play stored pattern
// +Number, -Number: Shift base note by semitones
// ++Number, --Number: Shift base note by octaves (or multiply/divide for non-notes)
// x: rest
// r: reset

// Notice cool "drop"
const char song[] = "\
0 +1 0 +1 0 -2 +12 0 r x \
--1 0 1 2 r \
";

typedef enum {
  S_SCAN = 0,
  S_WANTTAG,
  S_WANTRPAREN  
} ParenState;

typedef enum {
  SM_SCAN = 0,
  SM_PLUS,
  SM_PLUS2,
  SM_MINUS,
  SM_MINUS2,
  SM_COUNT,
} ModState;

struct Frame {
  int at;
  int root;
  int pitch;
  int rate;
  int high;
  bool on;
  Frame() : at(0), root(0), pitch(0), rate(RATE), high(HIGHTIME), on(false) {}
};

struct Interp {
  // PARSER
  int sp;
  ParenState state; ModState modState;
  Frame stack[STACKMAX];
  bool looped;
  int func[FUNCS];

  // HACK: Stack pointer starts at 1 so r can work at the lowest level
  Interp() : sp(1), state(S_SCAN), modState(SM_SCAN), looped(false) {
    memset(func, 0, sizeof(func));
  }
  void reset();
  void resetState();
  void next();
  Frame &frame() { return stack[sp]; }
  Frame &lastFrame() { return stack[sp-1]; }
  int &at() { return frame().at; }
  bool takeNumber(int n);
};

void Interp::resetState() {
  state = S_SCAN;
  modState = SM_SCAN;
}
void Interp::reset() {
  sp = 1; // In case of syntax error
  resetState(); // Ibid
  at() = 0;
  looped = true;
}

static int modScale[SM_COUNT] = {0, 1, 12, -1, -12};

bool Interp::takeNumber(int n) { // Return true if "done" with current note
  if (modState == SM_SCAN) {
      frame().on = true;
      frame().pitch = n;
      return true;
  } else {
    frame().root += modScale[modState] * n;
    resetState();
    return false;    
  }
}

void Interp::next() {
  bool isbuilding = false;
  int building = 0;
  bool done = false;
  while (!done) {
    int i = at();
    char ch = song[i];
#if DEBUG
    Serial.print("\nLoop");
    switch(state) { default:break; case S_WANTTAG: Serial.print(" TAG?"); break; case S_WANTRPAREN: Serial.print(" PAREN?"); }
    Serial.print("\nmod ");
    Serial.print(modState);
    Serial.print("  root ");
    Serial.print(frame().root);
    Serial.print("\nstack ");
    Serial.print(sp);
    Serial.print("  at ");
    Serial.print(i);
    Serial.print("  ch ");
    Serial.print(ch);
    Serial.print("\n");
#endif
    if (ch == '\0') {
      bool done = false;
      if (isbuilding)
        done = takeNumber(building);
      reset();
      
      building = 0;
      isbuilding = false;
      
      if (done)
        return;
      else
        continue;
    } else switch (state) {
      case S_WANTTAG:
        if (isupper(ch))
          func[ch-'A'] = i+1;
        state = S_WANTRPAREN;
        break;

      case S_WANTRPAREN:
        if (ch == ')')
          state = S_SCAN;
        break;
      
      case S_SCAN:
        if (isdigit(ch)) {
          building *= 10;
          isbuilding = true;
          building += (ch - '0');
        } else {
          if (isbuilding) {
            if (takeNumber(building))
              return;
            building = 0;
            isbuilding = false;
          }

          if (isupper(ch)) {
            sp++;
            frame() = lastFrame(); // Copy up
            at() = func[ch-'A'];
            continue;
          }

          switch (ch) {
            case 'x':
              frame().on = false;
              done = true;
              break;
            case 'r':
              frame().root = lastFrame().root;
            case '+':
              modState = modState == SM_PLUS ? SM_PLUS2 : SM_PLUS;
              break;
            case '-':
              modState = modState == SM_MINUS ? SM_MINUS2 : SM_MINUS;
              break;
            case '(':
              state = looped ? S_WANTRPAREN : S_WANTTAG;
              break;
            case ')':
              sp--;
              if (sp < 0)
                sp = 0;
              break;
            default: break; // Assume whitespace
          }
        }
        break;
    }
    at()++;
  }
}

static Interp interp;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PLAY, OUTPUT);
  pinMode(PITCH, OUTPUT);
#if DEBUG
  Serial.begin(9600);
#endif
}

void high() {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(PLAY, HIGH);
}

void low() {
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(PLAY, LOW);
}

void loop() {
#if DEBUG
  Serial.print("Booted\n");
#endif
  interp.next();
  Frame note = interp.frame();

  int p = (ROOT*12 + OCTAVE*(note.root + note.pitch))/12;    
  analogWrite(PITCH, p);

  if (note.on) {
    high();
    if (note.high <= note.rate) {
      delay(note.high);
      low();
      delay(note.rate-note.high);
    } else {
      delay(note.rate);
    }
  } else {
    delay(note.rate);
  }
}
