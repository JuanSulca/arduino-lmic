#include "lmic.h"
#include "hal/hal.h"
#include <SPI.h>
#include <Arduino.h>
#include "lmic/oslmic_types.h"
#include "lmic/oslmic.h"
#include <unity.h>
#include "lmic/lmic.c"
#include "aes/ideetron/AES-128_v10.cpp"
#include "aes/other.c"

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8]={ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = { 0x99, 0xD5, 0x84, 0x93, 0xD1, 0x20, 0x5B, 0x43, 0xEF, 0xF9, 0x38, 0xF0, 0xF6, 0x6C, 0x33, 0x9E };
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }

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

//u4_t AESAUX[16/sizeof(u4_t)];
//u4_t AESKEY[11*16/sizeof(u4_t)];

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

void test_micB0_1(void) {
  u4_t devaddr = 0xAB1232F3;
  u4_t seq = 3;
  micB0(devaddr, seq, 1, 11);
  u1_t actual[] = { 0x49, 0x0, 0x0, 0x0, 0x0, 0x1, 0xf3, 0x32, 0x12, 0xab, 0x3, 0x0, 0x0, 0x0, 0x0, 0xb };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(actual, AESaux, 16);
}

void test_micB0_0(void) {
  u4_t devaddr = 0xAB1232F3;
  u4_t seq = 3;
  micB0(devaddr, seq, 0, 6);
  u1_t actual[] = { 0x49, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf3, 0x32, 0x12, 0xab, 0x3, 0x0, 0x0, 0x0, 0x0, 0x6 };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(actual, AESaux, 16);
}

void test_aes_appendMic(void) {
  u4_t devaddr = 0xAB1232F3;
  u4_t seq = 3;
  const u1_t key[] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x0 };
  //composed of /MHDR/DevAddr/FCrtl/FCnt/FOps/Fpor/FramePayload/
  u1_t sampleFrame[255] = { /*MHDR*/0x40, /*DevAddr*/0xab, 0x12, 0x32, 0xf3, /*FCrtl*/0x12, /*FCnt*/0x1b, 0x2a,/*frame*/0x68, 0x65, 0x6c, 0x6c, 0x6f };
  aes_appendMic(key, devaddr, seq-1, /*up*/0, sampleFrame, 13);
  u1_t actual[] = { 0x40, 0xab, 0x12, 0x32, 0xf3, 0x12, 0x1b, 0x2a, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x2B, 0xc7, 0x1b, 0xef };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(actual, sampleFrame, 17);
}

void test_aes_appendMic0(void) {
  //composed of /MHDR/DevAddr/FCrtl/FCnt/FOps/Fpor/FramePayload/
  u1_t sampleFrame[255] = { /*MHDR*/0x40, /*DevAddr*/0xab, 0x12, 0x32, 0xf3, /*FCrtl*/0x12, /*FCnt*/0x1b, 0x2a,/*frame*/0x68, 0x65, 0x6c, 0x6c, 0x6f };
  aes_appendMic0(sampleFrame, 13);
  u1_t actual[] = { 0x40, 0xab, 0x12, 0x32, 0xf3, 0x12, 0x1b, 0x2a, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x28, 0x83, 0xe3, 0xec };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(actual, sampleFrame, 17);
}

void test_aes_verifyMic(void) {
  /*
  payload: 40AE130426800000016F895D98810714E3268295
  NwkSKey: 99D58493D1205B43EFF938F0F66C339E
  AppKey: 0A501524F8EA5FCBF9BDB5AD7D126F75
  */
  u4_t devaddr = 0x260413AE;
  u4_t seq = 0;
  const u1_t key[] = { 0x99, 0xD5, 0x84, 0x93, 0xD1, 0x20, 0x5B, 0x43, 0xEF, 0xF9, 0x38, 0xF0, 0xF6, 0x6C, 0x33, 0x9E };
  u1_t pdu[] = { 0x40, 0xAE, 0x13, 0x04, 0x26, 0x80, 0x00, 0x00, 0x01, 0x6F, 0x89, 0x5D, 0x98, 0x81, 0x07, 0x14, 0xE3, 0x26, 0x82, 0x95 };
  int result = aes_verifyMic(key, devaddr, seq, 0, pdu, 16);
  TEST_ASSERT_EQUAL_INT(1, result);
}

void test_aes_verifyMic0(void) {
  /*
  payload: 20c134abbe356aae1304260000
  NwkSKey: 99D58493D1205B43EFF938F0F66C339E
  AppKey: 0A501524F8EA5FCBF9BDB5AD7D126F75
  */
  u1_t pdu[] = { 0x20, 0xaa, 0x4c, 0x41, 0xde, 0xc0, 0xfb, 0xa2, 0x5f, 0x81, 0x36, 0x1e, 0x5a, 0x6a, 0x19, 0xca, 0xb7 };
  aes_encrypt(pdu+1, 16); // do not forget to use this fucking function before calling aes_verifyMic0 :D
  int result = aes_verifyMic0(pdu, 13);
  TEST_ASSERT_EQUAL_INT(1, result);
}

void test_aes_encrypt(void) {
  u1_t sample[] = { 0x20, 0xaa, 0x4c, 0x41, 0xde, 0xc0, 0xfb, 0xa2, 0x5f, 0x81, 0x36, 0x1e, 0x5a, 0x6a, 0x19, 0xca };
  aes_encrypt(sample, 16);
  u1_t actual[] = { 0x79, 0x4f, 0xa8, 0x6e, 0xc2, 0xe4, 0xaa, 0x64, 0xec, 0x8, 0xd6, 0x3a, 0x32, 0x31, 0x21, 0x6a };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(actual, sample, 16);
}

void test_aes_cipher(void) {
  u4_t devaddr = 0x260413AE;
  u4_t seq = 0;
  const u1_t key[] = { 0x99, 0xD5, 0x84, 0x93, 0xD1, 0x20, 0x5B, 0x43, 0xEF, 0xF9, 0x38, 0xF0, 0xF6, 0x6C, 0x33, 0x9E };
  u1_t pdu[] = { 0x40, 0xAE, 0x13, 0x04, 0x26, 0x80, 0x00, 0x00, 0x01, 0x6F, 0x89, 0x5D, 0x98, 0x81, 0x07, 0x14 };
  aes_cipher(key, devaddr, seq, 0, pdu, 16);
  u1_t actual[] = { 0xb3, 0xfb, 0x12, 0xbd, 0xe8, 0xf1, 0x54, 0x21, 0x51, 0x7e, 0x16, 0x3f, 0x3c, 0x58, 0x1b, 0x8a };
  TEST_ASSERT_EQUAL_UINT8_ARRAY(actual, pdu, 16);
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
  RUN_TEST(test_micB0_1);
  RUN_TEST(test_micB0_0);
  RUN_TEST(test_aes_appendMic);
  RUN_TEST(test_aes_appendMic0);
  RUN_TEST(test_aes_verifyMic);
  RUN_TEST(test_aes_verifyMic0);
  RUN_TEST(test_aes_encrypt);
  RUN_TEST(test_aes_cipher);

  UNITY_END(); // stop unit testing
}

void loop() {
}
