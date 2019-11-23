#include <Arduino.h>
#include <IR_Transmitter.h>
#include <unity.h>

IR_Transmitter transmitter;

void test_function_formatTempVal(void)
{
    TEST_ASSERT_EQUAL(0x02, transmitter.formatTempVal(16));
    TEST_ASSERT_EQUAL(0x0A, transmitter.formatTempVal(20));
    TEST_ASSERT_EQUAL(0x14, transmitter.formatTempVal(25));
}

void setup() {

    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_function_formatTempVal);
    UNITY_END();
}

void loop() {
    digitalWrite(5, HIGH);
    delay(100);
    digitalWrite(5, LOW);
    delay(500);
}
