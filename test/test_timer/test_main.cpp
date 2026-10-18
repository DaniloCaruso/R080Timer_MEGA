// Seam 1: dominio intero con il TimingCore vero e driver finti. I test agiscono
// solo con input dell'utente e tempo che passa.
#include <unity.h>
#include <Controller.h>
#include <Screen.h>
#include "../fakes/FakeLcd.h"
#include "../fakes/FakeLog.h"
#include "../fakes/FakeSevenSeg.h"
#include "../fakes/SimClock.h"

struct Timer {
  SimClock clock;
  FakeSevenSeg seg;
  FakeLcd lcd;
  FakeLog log;
  Controller ctl{clock, seg};
  Screen screen{lcd, log, "2.0.0"};

  Timer() { loop(); }
  void loop() { ctl.update(); screen.render(clock.nowMs); }  // un giro di loop()
  void type(const char* digits) {
    for (; *digits; digits++) { ctl.onInput(Input::Digit, *digits - '0'); loop(); }
  }
  void press() { ctl.onInput(Input::MainPress, 0); loop(); }
  void advance(uint32_t ms) {
    while (ms--) { clock.tick(); loop(); }
  }
};

void setUp() {}
void tearDown() {}

// REQ-5.1
static void test_digits_enter_from_the_right() {
  Timer t;
  t.type("1");
  TEST_ASSERT_EQUAL_UINT16(1, t.seg.shown);
  t.type("2");
  TEST_ASSERT_EQUAL_UINT16(12, t.seg.shown);
  t.type("5");
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
}

// REQ-5.1
static void test_fifth_digit_pushes_out_the_oldest() {
  Timer t;
  t.type("12345");
  TEST_ASSERT_EQUAL_UINT16(2345, t.seg.shown);
}

// REQ-3.5, REQ-1.2
static void test_start_runs_countdown_for_the_set_time() {
  Timer t;
  t.type("125");
  t.press();
  TEST_ASSERT_TRUE(t.clock.relay);
  t.advance(12499);
  TEST_ASSERT_TRUE(t.clock.relay);
  t.advance(1);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(12500, t.clock.relayOffAtMs - t.clock.relayOnAtMs);
}

// REQ-3.6
static void test_second_press_cancels_the_countdown() {
  Timer t;
  t.press();
  t.advance(3000);
  t.press();
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(3000, t.clock.relayOffAtMs);
  t.advance(10000);
  TEST_ASSERT_FALSE(t.clock.relay);
}

// Il 7 segmenti mostra il tempo residuo arrotondato al decimo superiore.
static void test_seven_seg_counts_down_during_countdown() {
  Timer t;
  t.type("125");
  t.press();
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
  t.advance(100);
  TEST_ASSERT_EQUAL_UINT16(124, t.seg.shown);
  t.advance(1);
  TEST_ASSERT_EQUAL_UINT16(124, t.seg.shown);
  t.advance(12300);  // residuo 99 ms
  TEST_ASSERT_EQUAL_UINT16(1, t.seg.shown);
  uint32_t writes = t.seg.writes;
  t.advance(98);
  TEST_ASSERT_EQUAL_UINT32(writes, t.seg.writes);  // un invio per decimo, non per ms
}

// REQ-3.7
static void test_seven_seg_returns_to_set_time_after_end() {
  Timer t;
  t.type("125");
  t.press();
  t.advance(12500);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
}

// REQ-3.7
static void test_seven_seg_returns_to_set_time_after_cancel() {
  Timer t;
  t.type("125");
  t.press();
  t.advance(5000);
  t.press();
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
}

// REQ-5.2
static void test_first_digit_after_exposure_starts_a_new_time() {
  Timer t;
  t.type("12");
  t.press();
  t.advance(1200);
  t.type("3");
  TEST_ASSERT_EQUAL_UINT16(3, t.seg.shown);
}

// REQ-5.2: anche dopo un Annullamento.
static void test_first_digit_after_cancel_starts_a_new_time() {
  Timer t;
  t.type("12");
  t.press();
  t.press();
  t.type("3");
  TEST_ASSERT_EQUAL_UINT16(3, t.seg.shown);
}

// REQ-5.9: durante l'Esposizione le cifre non cambiano il tempo impostato.
static void test_digits_are_ignored_during_countdown() {
  Timer t;
  t.type("12");
  t.press();
  t.type("9");
  t.advance(1200);
  TEST_ASSERT_EQUAL_UINT16(12, t.seg.shown);
}

// REQ-3.8 (il beep di errore arriva con il ticket dei suoni)
static void test_start_with_zero_time_does_nothing() {
  Timer t;
  t.type("0");
  t.press();
  TEST_ASSERT_FALSE(t.clock.relay);
  t.advance(1);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(0, t.seg.shown);
}

// REQ-7.9: nome e versione per 1,5 s dall'accensione.
static void test_lcd_shows_name_and_version_at_power_on() {
  Timer t;
  t.advance(1);
  TEST_ASSERT_EQUAL_STRING(" - R080 Timer - ", t.lcd.cells[0]);
  TEST_ASSERT_EQUAL_STRING("     v2.0.0     ", t.lcd.cells[1]);
  t.advance(1497);  // 1498 ms
  TEST_ASSERT_EQUAL_STRING(" - R080 Timer - ", t.lcd.cells[0]);
  TEST_ASSERT_EQUAL_STRING("     v2.0.0     ", t.lcd.cells[1]);
}

// REQ-7.5: stessa disposizione del firmware precedente (pre-rewrite, presentation.h):
// riga 1 modalità e tipo di precisione, riga 2 precisione, Canale, nota e spina.
// Valori di fabbrica: Countdown, Secondi 1 s, Canale A, mute spento, relè abilitato.
static void test_lcd_shows_countdown_layout_after_splash() {
  Timer t;
  t.advance(1500);
  t.advance(1);
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [S]  ", t.lcd.cells[0]);
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:A \x01\x02", t.lcd.cells[1]);
}

// Da fermo il bus I²C resta inattivo se nulla cambia.
static void test_lcd_is_not_written_without_changes() {
  Timer t;
  t.advance(1600);
  uint32_t writes = t.lcd.writes;
  t.advance(10000);
  TEST_ASSERT_EQUAL_UINT32(writes, t.lcd.writes);
}

// REQ-2.6: con l'LCD guasto l'Esposizione prosegue sul solo 7 segmenti e il guasto va nel log.
static void test_countdown_ends_normally_with_lcd_offline() {
  Timer t;
  t.advance(1600);
  t.lcd.isOnline = false;
  t.type("125");
  t.press();
  t.advance(12499);
  TEST_ASSERT_TRUE(t.clock.relay);
  t.advance(1);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(12500, t.clock.relayOffAtMs - t.clock.relayOnAtMs);
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
  TEST_ASSERT_EQUAL_UINT32(1, t.log.lines);  // una sola riga, non una per giro
  TEST_ASSERT_EQUAL_STRING("LCD offline", t.log.last);
}

// Dopo un guasto il driver reinizializza l'LCD: il contenuto va ridisegnato tutto.
static void test_lcd_is_redrawn_when_it_comes_back_online() {
  Timer t;
  t.advance(1600);
  t.lcd.isOnline = false;
  t.advance(10);
  t.lcd.fill('?');  // contenuto perso
  t.lcd.isOnline = true;
  t.advance(2);
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [S]  ", t.lcd.cells[0]);
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:A \x01\x02", t.lcd.cells[1]);
  TEST_ASSERT_EQUAL_STRING("LCD online", t.log.last);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_digits_enter_from_the_right);
  RUN_TEST(test_fifth_digit_pushes_out_the_oldest);
  RUN_TEST(test_start_runs_countdown_for_the_set_time);
  RUN_TEST(test_second_press_cancels_the_countdown);
  RUN_TEST(test_seven_seg_counts_down_during_countdown);
  RUN_TEST(test_seven_seg_returns_to_set_time_after_end);
  RUN_TEST(test_seven_seg_returns_to_set_time_after_cancel);
  RUN_TEST(test_first_digit_after_exposure_starts_a_new_time);
  RUN_TEST(test_first_digit_after_cancel_starts_a_new_time);
  RUN_TEST(test_digits_are_ignored_during_countdown);
  RUN_TEST(test_start_with_zero_time_does_nothing);
  RUN_TEST(test_lcd_shows_name_and_version_at_power_on);
  RUN_TEST(test_lcd_shows_countdown_layout_after_splash);
  RUN_TEST(test_lcd_is_not_written_without_changes);
  RUN_TEST(test_countdown_ends_normally_with_lcd_offline);
  RUN_TEST(test_lcd_is_redrawn_when_it_comes_back_online);
  return UNITY_END();
}
