/*
 * FOOTSWITCHER for Teensy 3.2
 * 
 * This sketch is designed to be used with a programmable looper pedal & the
 * Bugera 6260 amplifier.
 * 
 * It maps 3 separate amp settings (clean/lead, reverb on/off, fx loop on/off)
 * to the respective settings on the amp.
 * 
 * The Bugera 6260 comes with a footswitch that works by translating 4 momentary
 * switches to pulsed voltages that the amp can sense. In addition, the reverb &
 * fx loop settings are saved separately by the amp for each of the channels.
 * 
 * Meanwhile, the looper provides continuously open/shorted signals for these
 * settings.
 * 
 * So, this sketch reacts when the input switches change from open<->closed, generating
 * pulses with the correct voltage.
 */
// Teensy 3.x / Teensy LC have the LED on pin 13
const byte ledPin = 13;
// Input pins from looper
const byte looperChannelPin = 2;
const byte looperReverbPin = 3;
const byte looperFxPin = 4;
// Input pins from amp
const byte ampReverbLED = 20;
const byte ampChannelLED = 19;
const byte ampFxLED = 18;
// Output pins to amp
const byte ampCleanPin = 14;
const byte ampLeadPin = 15;
const byte ampReverbPin = 16;
const byte ampFxPin = 17;

// Constants

const byte DebugSerial = LOW;

// These output switching events are queued up
enum ampSwitch: byte {
  blank     = 0,
  cleanOn   = 1,
  leadOn    = 2,
  reverbOn  = 3,
  fxOn      = 4,
  cleanOff  = 5,
  leadOff   = 6,
  reverbOff = 7,
  fxOff     = 8,
  checkLED  = 9
};
// How long to debounce the input switches
const unsigned long debounceDelay = 200; // milliseconds
// How long to keep the momentary switches on and wait in between events.
const unsigned long refresh = 15; // milliseconds
// Length of the switch queue.
const unsigned int qLength = 100; // 10ms * 100 = 1s worth of events

// State variables

// The state of the input switches
byte stateChannel, stateReverb, stateFx;
// The actual desired value (should match looper state)
byte channel, reverb, fx;
// debounce timers
unsigned long debounceChannel = millis();
unsigned long debounceReverb = millis();
unsigned long debounceFx = millis();
// event timer
unsigned long prevTime = millis();
// event queue state
ampSwitch qSwitches[qLength];
ampSwitch* qEnd = qSwitches + qLength;
ampSwitch* qHead = qSwitches;
ampSwitch* qTail = qSwitches;

// the setup() method runs once, when the sketch starts
void setup() {  
  // Teensy LED
  pinMode(ledPin, OUTPUT);
  // Inputs from Looper
  pinMode(looperChannelPin, INPUT_PULLUP);
  pinMode(looperReverbPin, INPUT_PULLUP);
  pinMode(looperFxPin, INPUT_PULLUP);
  // Inputs from Bugera 6260
  pinMode(ampChannelLED, INPUT);
  pinMode(ampFxLED, INPUT);
  pinMode(ampReverbLED, INPUT);
  // Outputs to Bugera 6260
  pinMode(ampCleanPin, OUTPUT);
  pinMode(ampLeadPin, OUTPUT);
  pinMode(ampFxPin, OUTPUT);
  pinMode(ampReverbPin, OUTPUT);
  // Other
  memset(qSwitches, 0, sizeof(qSwitches));
  // Reset everything
  digitalWrite(ampLeadPin, LOW);
  digitalWrite(ampCleanPin, HIGH);
  delay(10);
  digitalWrite(ampCleanPin, LOW);
  delay(10);
  digitalWrite(ampFxPin, LOW);
  delay(10);
  digitalWrite(ampReverbPin, LOW);
  delay(10);  
}

void queue(byte s) {
  *qTail = (ampSwitch)s;
  qTail++;
  if(qTail >= qEnd) {
    qTail = qSwitches;
  }
  *qTail = ampSwitch::blank;
}

void loop() {
  unsigned long timeNow = millis();
  byte prevChannel = stateChannel;
  byte prevReverb = stateReverb;
  byte prevFx = stateFx;
  byte reverbLED, fxLED;

  // Handle 64bit wraparound
  if(timeNow < prevTime) {
    prevTime = timeNow;
    debounceChannel = timeNow;
    debounceReverb = timeNow;
    debounceFx = timeNow;
  }
  
  stateChannel = digitalRead(looperChannelPin);
  stateReverb = digitalRead(looperReverbPin);
  stateFx = digitalRead(looperFxPin);
  
  if (stateChannel != prevChannel && timeNow > debounceChannel + debounceDelay) {
    channel = stateChannel;
    if(channel) {
      queue(ampSwitch::leadOn);
      queue(ampSwitch::leadOff);
    }
    else {
      queue(ampSwitch::cleanOn);
      queue(ampSwitch::cleanOff);
    }
    queue(ampSwitch::checkLED);
    debounceChannel = timeNow;
  }
  if (stateFx != prevFx && timeNow > debounceFx + debounceDelay) {
    fx = stateFx;
    queue(ampSwitch::fxOn);
    queue(ampSwitch::fxOff);
    debounceFx = timeNow;
  }
  if (stateReverb != prevReverb && timeNow > debounceReverb + debounceDelay) {
    reverb = stateReverb;
    queue(ampSwitch::reverbOn);
    queue(ampSwitch::reverbOff);
    debounceReverb = timeNow;
  }
  // If it's been a while (1s) since we've switched anything, check the LEDs to verify our state.
  if(timeNow >= prevTime + 1000 && qHead == qTail) {
    queue(ampSwitch::checkLED);    
  }
  // Check if it's been a long enough time to refresh and send signals.
  if (timeNow < prevTime + refresh || qHead == qTail || *qHead == ampSwitch::blank) {
    return;
  }
  fxLED = digitalRead(ampFxLED);
  reverbLED = digitalRead(ampReverbLED);
  if(DebugSerial) {
    Serial.print("Reverb SW: ");
    Serial.print(reverb);
    Serial.print(", FX SW: ");
    Serial.print(fx);
    Serial.print(", Reverb LED: ");
    Serial.print(reverbLED);
    Serial.print(", FX LED: ");
    Serial.println(fxLED);
    //Serial.print("Switch: ");
    //Serial.print(*qHead);      
  }
  switch(*qHead) {
    case ampSwitch::leadOn:
      digitalWrite(ampLeadPin, HIGH);
      digitalWrite(ledPin, HIGH);
      break;
    case ampSwitch::leadOff:
      digitalWrite(ampLeadPin, LOW);  
      digitalWrite(ledPin, LOW);
      break;
    case ampSwitch::cleanOn:
      digitalWrite(ampCleanPin, HIGH);  
      digitalWrite(ledPin, HIGH);
      break;
    case ampSwitch::cleanOff:
      digitalWrite(ampCleanPin, LOW);  
      digitalWrite(ledPin, LOW);
      break;
    case ampSwitch::reverbOn:
      if(reverbLED != reverb) {
        digitalWrite(ampReverbPin, HIGH);  
        digitalWrite(ledPin, HIGH);
      }
      break;
    case ampSwitch::reverbOff:
      digitalWrite(ampReverbPin, LOW);  
      digitalWrite(ledPin, LOW);
      break;
    case ampSwitch::fxOn:
      if(fxLED != fx) {
        digitalWrite(ampFxPin, HIGH);  
        digitalWrite(ledPin, HIGH);
      }
      break;
    case ampSwitch::fxOff:
      digitalWrite(ampFxPin, LOW);  
      digitalWrite(ledPin, LOW);
      break;
    case ampSwitch::checkLED:
      if(fxLED != fx) {
        queue(ampSwitch::fxOn);
        queue(ampSwitch::fxOff);
      }
      if(reverbLED != reverb) {
        queue(ampSwitch::reverbOn);
        queue(ampSwitch::reverbOff);
      }
      break;
    default:
      break;
  }
  *qHead = ampSwitch::blank;
  qHead++;
  if(qHead >= qEnd) {
    qHead = qSwitches;
  }
  prevTime = timeNow;
}
