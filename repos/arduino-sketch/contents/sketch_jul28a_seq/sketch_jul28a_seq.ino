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
// ++Number, --Number: Shift base note by octaves

const char song[] = "\
(M 0 4 7) \
(N 0 3 7) \
(R M M M M) \
(S N N N N) \
(T M M M ++1 M) \
R S R S T S R S \
";

typedef enum {
  S_SCAN = 0,
  S_WANTTAG,
  S_WANTRPAREN  
} ParenState;

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
  ParenState state;
  Frame stack[STACKMAX];
  bool looped;
  int func[FUNCS];
  
  Interp() : sp(0), state(S_SCAN), looped(false) {
    memset(func, 0, sizeof(func));
  }
  void reset();
  void next();
  Frame &frame() { return stack[sp]; }
  int &at() { return frame().at; }
  bool takeNumber(int n);
};

void Interp::reset() {
  sp = 0; // In case of syntax error
  state = S_SCAN; // Ibid
  at() = 0;
  looped = true;
}

bool Interp::takeNumber(int n) { // Return true if "done" with current note
  frame().on = true;
  frame().pitch = n;
  return true;
}

void Interp::next() {
  bool isbuilding = false;
  int building = 0;
  while (1) {
    int i = at();
    char ch = song[i];
#if DEBUG
    Serial.print("Loop ");
    Serial.print(state);
    Serial.print("\n");
    Serial.print(sp);
    Serial.print(", ");
    Serial.print(i);
    Serial.print(", ");
    Serial.print(ch);
    Serial.print("\n");
#endif
    if (ch == '\0') {
      bool done = false;
      if (isbuilding)
        done = takeNumber(building);
      reset();
      building = 0;
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
          }

          if (isupper(ch)) {
            sp++;
            frame() = stack[sp-1]; // Copy up
            at() = func[ch-'A'];
            continue;
          }

          switch (ch) {
            case '(':
              state = looped ? S_WANTRPAREN : S_WANTTAG;
              break;
            case ')':
              sp--;
              if (sp < 0)
                sp = 0;
              break;
            default:break;
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
