/*
   RFID reader:
   ============
   Pin layout:
    SDA: PIN_PB2 (JP3_5)
    SCK: PIN_PB5 (JP3_8)
    MOSI: PIN_PB3 (JP3_6)
    MISO: PIN_PB4 (JP3_7)
    IRQ: -
    GND: Ground (JP3_9/10)
    RST: PIN_PB1 (JP3_4)
    VCC: 3.3V

   For Prototype board to Practicum:
    Red line wire at VCC

    Lock servo:
    ===========
    Pin layout:
      Brown: Ground (JP4_9/10)
      RED: VCC(4.8-6.0) (JP4_1/2)
      ORANGE: PIN_PD5(PWM) (JP4_5)

    Lock button:
    ============
    Pin layout:
      Outside button: PIN_PD0 (JP4_3)
      Inside button: PIN_PD1 (JP4_4)
*/
//////////////////////////////////////////////////////////////////////
/* LOCK SECTION */
#include <Servo.h>

#define LOCK_SERVO_PIN PIN_PD6
#define LOCK_BUTTON_PIN PIN_PC3

#define SERVO_LOCK_POS 90
#define SERVO_UNLOCK_POS 170
#define LOCK_BUTTON_DATA_POS 5

Servo lock_servo;
//////////////////////////////////////////////////////////////////////
/* RFID SECTION */

#include <SPI.h>
#include <RFID.h>

#define SS_PIN PIN_PB2
#define RST_PIN PIN_PB1

#define RFID_NUM_LENGTH 5
RFID rfid(SS_PIN, RST_PIN);

// Store rfid number
static uint8_t rfidNum[5] = {0, 0, 0, 0, 0};

void clearRfidNum() {
  for (int i = 0; i < RFID_NUM_LENGTH; i++) {
    rfidNum[i] = 0;
  }
}

//////////////////////////////////////////////////////////////////////
/* USB SECTION */

#include <usbdrv.h>

#define RQ_SET_LED 0
#define RQ_GET_RFID_NUM 1
#define RQ_SET_LOCK_SERVO 2
#define RQ_GET_BUFFER_DATA 9
#define BUFFER_DATA_LENGTH 8

static uint8_t buffer_data[BUFFER_DATA_LENGTH];

void clearRfidNumBuffer() {
  for (int i = 0; i < RFID_NUM_LENGTH; i++) {
    buffer_data[i] = 0;
  }
}

extern "C" usbMsgLen_t usbFunctionSetup(uint8_t data[8])
{
  usbRequest_t *rq = (usbRequest_t*)data;

  if (rq->bRequest == RQ_SET_LED) {
    uint8_t led_val = rq->wValue.bytes[0];
    uint8_t led_no  = rq->wIndex.bytes[0];

    if (led_no == 0)
      digitalWrite(PIN_PC0, led_val);
    else if (led_no == 1)
      digitalWrite(PIN_PC1, led_val);
    else if (led_no == 2)
      digitalWrite(PIN_PC2, led_val);

    return 0;  // return no data back to host
  }

  if (rq->bRequest == RQ_GET_RFID_NUM) {
    usbMsgPtr = (uint8_t *) rfidNum;
    return RFID_NUM_LENGTH;
  }

  if (rq->bRequest == RQ_SET_LOCK_SERVO) {
    uint8_t lock_val = rq->wValue.bytes[0];
    uint8_t lock_check = rq->wIndex.bytes[0];
    if (lock_check != 7) {
      return 0;
    }

    if (lock_val == 0) {
      lock_servo.write(SERVO_UNLOCK_POS);
    } else if (lock_val == 1) {
      lock_servo.write(SERVO_LOCK_POS);
    }

    return 0;
  }

   if (rq->bRequest == RQ_GET_BUFFER_DATA) {
      usbMsgPtr = (uint8_t *) buffer_data;
      return BUFFER_DATA_LENGTH;
    }

  return 0;   /* nothing to do; return no data back to host */
}

//////////////////////////////////////////////////////////////////////
/* COUNTING SECTION */
#define OUTSIDE_BUTTON_PIN PIN_PD0
#define INSIDE_BUTTON_PIN PIN_PD1

#define COUNTING_BUTTON_DATA_POS 6
//////////////////////////////////////////////////////////////////////
/* MAIN CODE */

void setup() {
  pinMode(PIN_PC0, OUTPUT);
  pinMode(PIN_PC1, OUTPUT);
  pinMode(PIN_PC2, OUTPUT);
  pinMode(PIN_PC3, INPUT_PULLUP);
  pinMode(PIN_PC4, INPUT);
  pinMode(PIN_PD3, OUTPUT);
  /* SETUP FOR USB */
  usbInit();
  // enforce re-enumeration of USB devices
  usbDeviceDisconnect();
  delay(300);
  usbDeviceConnect();

  /* SETUP FOR RFID */
  SPI.begin();
  rfid.init();
  digitalWrite(PIN_PC2, HIGH);
  delay(500);
  digitalWrite(PIN_PC2, LOW);

  /* SETUP FOR LOCK SERVO */
  lock_servo.attach(LOCK_SERVO_PIN);
  lock_servo.write(SERVO_UNLOCK_POS);
  pinMode(LOCK_BUTTON_PIN, INPUT_PULLUP);

  /* SETUP FOR COUNTING BUTTON */
  pinMode(OUTSIDE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(INSIDE_BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  usbPoll();
  if (rfid.isCard()) {
    if (rfid.readCardSerial()) {
      digitalWrite(PIN_PC2, HIGH);
      for (int i = 0; i < RFID_NUM_LENGTH; i++) {
        rfidNum[i] = rfid.serNum[i];
        buffer_data[i] = rfid.serNum[i];
      }
    }
  } else {
    digitalWrite(PIN_PC2, LOW);
    clearRfidNum();
    clearRfidNumBuffer();
  }
  buffer_data[LOCK_BUTTON_DATA_POS] = (uint8_t)digitalRead(LOCK_BUTTON_PIN);
  buffer_data[COUNTING_BUTTON_DATA_POS] = (uint8_t)digitalRead(OUTSIDE_BUTTON_PIN);
  buffer_data[COUNTING_BUTTON_DATA_POS + 1] = (uint8_t)digitalRead(INSIDE_BUTTON_PIN);
  rfid.halt();
}
