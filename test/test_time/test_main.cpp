// Test di avvio della libreria di dominio: conversioni tra decimi di secondo e millisecondi.
#include <unity.h>
#include <Time.h>

void setUp() {}
void tearDown() {}

static void test_ds_to_ms_converts_tenths_to_milliseconds() {
  TEST_ASSERT_EQUAL_UINT32(100, dsToMs(1));
  TEST_ASSERT_EQUAL_UINT32(999900, dsToMs(DS_MAX));
}

static void test_ms_to_ds_truncates_to_whole_tenths() {
  TEST_ASSERT_EQUAL_UINT16(125, msToDs(12500));
  TEST_ASSERT_EQUAL_UINT16(124, msToDs(12499));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ds_to_ms_converts_tenths_to_milliseconds);
  RUN_TEST(test_ms_to_ds_truncates_to_whole_tenths);
  return UNITY_END();
}
