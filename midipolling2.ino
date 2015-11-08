static int notes[256];

void setup() {
  unsigned long freq = 440;
  for( int i=0; i<256; i++ ) {
    notes[i] = freq / 80;
    freq = freq * 16 / 15;
  }
  
  Serial.begin(115200);
  Serial.println( "starting..." );

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TIFR1 = 0;
  OCR1A = 170;
  OCR1B = 31;
  TCCR1B |= ( 1 << WGM12 );
  TCCR1B |= ( 1 << CS10 );
  TIMSK1 |= ( 1 << OCIE1A );
  pinMode( 7, INPUT );  
}

byte fastReadPin7() {
  return PIND >> 7;
}

// must be a power of 2
// buffers are interleaved into 256 bytes
// so buffer size will be 256/BUFFER_COUNT
const byte BUFFER_COUNT = 8;

// state ids
const byte STATE_Idle = 0;
const byte STATE_Start = 4;
const byte STATE_Tick0A = 8;
const byte STATE_Tick0B = 12;
const byte STATE_Bit0 = 16;
const byte STATE_Tick1A = 20;
const byte STATE_Tick1B = 24;
const byte STATE_Bit1 = 28;
const byte STATE_Tick2A = 32;
const byte STATE_Tick2B = 36;
const byte STATE_Bit2 = 40;
const byte STATE_Tick3A = 44;
const byte STATE_Tick3B = 48;
const byte STATE_Bit3 = 52;
const byte STATE_Tick4A = 56;
const byte STATE_Tick4B = 60;
const byte STATE_Bit4 = 64;
const byte STATE_Tick5A = 68;
const byte STATE_Tick5B = 72;
const byte STATE_Bit5 = 76;
const byte STATE_Tick6A = 80;
const byte STATE_Tick6B = 84;
const byte STATE_Bit6 = 88;
const byte STATE_Tick7A = 92;
const byte STATE_Tick7B = 96;
const byte STATE_Bit7 = 100;
const byte STATE_WaitForStop = 104;
const byte STATE_Stop = 108;

// state data
const byte STATES[] = {
  STATE_Start,  STATE_Idle,   0b00000000, 0, // STATE_Idle
  STATE_Tick0A, STATE_Idle,   0b00000000, 0, // STATE_Start
  STATE_Tick0B, STATE_Tick0B, 0b00000000, 0, // STATE_Tick0A
  STATE_Bit0,   STATE_Bit0,   0b00000000, 0, // STATE_Tick0B
  STATE_Tick1A, STATE_Tick1A, 0b00000001, 0, // STATE_Bit0
  STATE_Tick1B, STATE_Tick1B, 0b00000000, 0, // STATE_Tick1A
  STATE_Bit1,   STATE_Bit1,   0b00000000, 0, // STATE_Tick1B
  STATE_Tick2A, STATE_Tick2A, 0b00000010, 0, // STATE_Bit1
  STATE_Tick2B, STATE_Tick2B, 0b00000000, 0, // STATE_Tick2A
  STATE_Bit2,   STATE_Bit2,   0b00000000, 0, // STATE_Tick2B
  STATE_Tick3A, STATE_Tick3A, 0b00000100, 0, // STATE_Bit2
  STATE_Tick3B, STATE_Tick3B, 0b00000000, 0, // STATE_Tick3A
  STATE_Bit3,   STATE_Bit3,   0b00000000, 0, // STATE_Tick3B
  STATE_Tick4A, STATE_Tick4A, 0b00001000, 0, // STATE_Bit3
  STATE_Tick4B, STATE_Tick4B, 0b00000000, 0, // STATE_Tick4A
  STATE_Bit4,   STATE_Bit4,   0b00000000, 0, // STATE_Tick4B
  STATE_Tick5A, STATE_Tick5A, 0b00010000, 0, // STATE_Bit4
  STATE_Tick5B, STATE_Tick5B, 0b00000000, 0, // STATE_Tick5A
  STATE_Bit5,   STATE_Bit5,   0b00000000, 0, // STATE_Tick5B
  STATE_Tick6A, STATE_Tick6A, 0b00100000, 0, // STATE_Bit5
  STATE_Tick6B, STATE_Tick6B, 0b00000000, 0, // STATE_Tick6A
  STATE_Bit6,   STATE_Bit6,   0b00000000, 0, // STATE_Tick6B
  STATE_Tick7A, STATE_Tick7A, 0b01000000, 0, // STATE_Bit6
  STATE_Tick7B, STATE_Tick7B, 0b00000000, 0, // STATE_Tick7A
  STATE_Bit7,   STATE_Bit7,   0b00000000, 0, // STATE_Tick7B
  STATE_WaitForStop, STATE_WaitForStop, 0b10000000, 0, // STATE_Bit7
  STATE_WaitForStop, STATE_Stop, 0b00000000, 0, // STATE_WaitForStop
  STATE_Start,  STATE_Idle,   0b00000000, BUFFER_COUNT, // STATE_Stop
};

static volatile byte buffer[256];
static volatile byte bufferIndex = 0;
static volatile byte readIndex = 0;

ISR(TIMER1_COMPA_vect) {
  static byte next = STATE_Idle;
  static byte currentByte = 0;
  byte val = fastReadPin7();
  byte state = next;
  byte bufferShift = STATES[state+3];
  byte captureMask = STATES[state+2];
  next = STATES[state+val];
  currentByte |= captureMask & (0-val);
  if( bufferShift ) {
    buffer[bufferIndex] = currentByte;
    bufferIndex += bufferShift;
    currentByte = 0;
  }
}

static char hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

void showHex( byte val ) {
  Serial.print( hex[val>>4]  );
  Serial.print( hex[val&0xF] );
}

byte readByte() {
  for( ;; ) {
    byte readTo = bufferIndex;
    if( readTo != readIndex ) {
      byte result = buffer[readIndex];
      readIndex += BUFFER_COUNT;
      return result;
    }
  }
}

void loop() {
  for(;;) {
    byte b = readByte();
    if( b == 0x90 ) {
      int note = notes[readByte()];
      tone(8, note);
      showHex(note);
      Serial.println();
    } else if( b == 0x80 ) {
      noTone(8);
    }
  }
}
