#include "lmic.h"
#include "hal/hal.h"
#include <SPI.h>
#include <Arduino.h>
#include "lmic/oslmic_types.h"
#include "lmic/oslmic.h"
#include <unity.h>
#include "lmic/lmic.c"

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8]={ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 6,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 5,
  .dio = {2, 3, 4},
};


void test_os_rlsbf2(void) {
  u1_t data[6] = { 0x1, 0xa, 0x2, 0x3, 0x4, 0x5 };
  xref2cu1_t dataPointer = data;
  u2_t result = os_rlsbf2(dataPointer);
  TEST_ASSERT_EQUAL(0xA01, result);
}

void test_os_rlsbf4(void) {
  u1_t data[6] = { 0x1, 0xa, 0x2, 0x3, 0x4, 0x5 };
  xref2cu1_t dataPointer = data;
  u4_t result = os_rlsbf4(dataPointer);
  TEST_ASSERT_EQUAL(0x3020A01, result);
}

void test_os_rmsbf4(void) {
  u1_t data[6] = { 0x1, 0xa, 0x2, 0x3, 0x4, 0x5 };
  xref2cu1_t dataPointer = data;
  u4_t result = os_rmsbf4(dataPointer);
  TEST_ASSERT_EQUAL(0x10A0203, result);
}

void test_os_wlsbf2(void) {
  u2_t data = 0xA1;
  u1_t zero[] = { 0x0 , 0x0, 0x0 };
  xref2u1_t buff = zero;
  os_wlsbf2(buff, data);
  u1_t actual[] = { 0xA1, 0x0, 0x0};
  TEST_ASSERT_EQUAL_UINT8_ARRAY(buff, actual, 3);
}

void test_os_wlsbf4(void) {
  u4_t data = 0xa12c;
  u1_t zero[] = { 0x0 , 0x0, 0x0, 0x0 };
  xref2u1_t buff = zero;
  os_wlsbf4(buff, data);
  u1_t actual[] = { 0x2c, 0xa1, 0x0, 0x0 };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(buff, actual, 4);
}

void test_os_wmsbf4(void) {
  u4_t data = 0xa12c;
  u1_t zero[] = { 0x0 , 0x0, 0x0, 0x0 };
  xref2u1_t buff = zero;
  os_wmsbf4(buff, data);
  u1_t actual[] = { 0x0, 0x0, 0xa1, 0x2c };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(buff, actual, 4);
}

void test_os_crc16(void) {
  u1_t data[] = { 0x0 , 0x0, 0x0, 0x0 };
  xref2u1_t buff = data;
  u2_t result = os_crc16(buff, 4);
  TEST_ASSERT_EQUAL(0x0, result);
}

void test_os_crc16_nonZero(void) {
  u1_t data[] = { 0x1 , 0x2, 0x1, 0x3 };
  xref2u1_t buff = data;
  u2_t result = os_crc16(buff, 4);
  TEST_ASSERT_EQUAL(0x1B86, result);
}

void setup() {
  delay(2000);

  UNITY_BEGIN();    // IMPORTANT LINE!

  RUN_TEST(test_os_rlsbf2);
  RUN_TEST(test_os_rlsbf4);
  RUN_TEST(test_os_rmsbf4);
  RUN_TEST(test_os_wlsbf2);
  RUN_TEST(test_os_wlsbf4);
  RUN_TEST(test_os_wmsbf4);
  RUN_TEST(test_os_crc16);
  RUN_TEST(test_os_crc16_nonZero);

  UNITY_END(); // stop unit testing
}

void loop() {
}
