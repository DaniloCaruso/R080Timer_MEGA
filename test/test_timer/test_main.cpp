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
  Screen screen{lcd, log, ctl, "2.0.0"};

  Timer() { loop(); }
  void loop() { ctl.update(); screen.render(clock.nowMs); }  // un giro di loop()
  void type(const char* digits) {
    for (; *digits; digits++) { ctl.onInput(Input::Digit, *digits - '0'); loop(); }
  }
  void press() { ctl.onInput(Input::MainPress, 0); loop(); }
  void input(Input in, int8_t arg = 0) { ctl.onInput(in, arg); loop(); }
  void star() { input(Input::Star); }
  void hash() { input(Input::Hash); }
  void turnPrecision(int8_t dir) { input(Input::PrecTurn, dir); }
  void clickPrecision() { input(Input::PrecClick); }
  void settle() { advance(1600); }  // oltre la schermata di avvio, LCD aggiornato
  // Le due righe dell'LCD dopo i giri di loop necessari a mandarle.
  const char* row(uint8_t r) { advance(2); return lcd.cells[r]; }
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

// REQ-4.2: in Secondi */# spostano il tempo del valore di precisione (1 s di fabbrica).
static void test_star_hash_step_by_seconds_precision() {
  Timer t;
  t.type("125");
  t.hash();
  TEST_ASSERT_EQUAL_UINT16(135, t.seg.shown);
  t.star();
  t.star();
  TEST_ASSERT_EQUAL_UINT16(115, t.seg.shown);
}

// REQ-4.2: limiti 0,1-999,9 s.
static void test_seconds_step_stays_within_limits() {
  Timer t;
  t.type("5");
  t.star();
  TEST_ASSERT_EQUAL_UINT16(1, t.seg.shown);
  t.type("9995");
  t.hash();
  TEST_ASSERT_EQUAL_UINT16(9999, t.seg.shown);
}

// REQ-5.2: dopo */# la prima cifra inizia un tempo nuovo.
static void test_first_digit_after_star_starts_a_new_time() {
  Timer t;
  t.type("12");
  t.hash();
  t.type("3");
  TEST_ASSERT_EQUAL_UINT16(3, t.seg.shown);
}

// REQ-4.1: lista in Secondi come nel firmware precedente, l'encoder gira in tondo.
static void test_precision_encoder_walks_the_seconds_list() {
  static const char* const rows[] = {"Pr: 1s  ", "Pr: 2s  ", "Pr: 3s  ", "Pr: 4s  ", "Pr: 5s  ", "Pr: 6s  ",
                                     "Pr: 7s  ", "Pr: 8s  ", "Pr: 10s ", "Pr: 15s ", "Pr: 20s "};
  Timer t;
  t.settle();
  for (uint8_t i = 0; i < 11; i++) {
    TEST_ASSERT_EQUAL_STRING_LEN(rows[i], t.row(1), 8);
    t.turnPrecision(1);
  }
  TEST_ASSERT_EQUAL_STRING_LEN("Pr: 1s  ", t.row(1), 8);
  t.turnPrecision(-1);
  TEST_ASSERT_EQUAL_STRING_LEN("Pr: 20s ", t.row(1), 8);
}

// REQ-4.2: il passo di */# segue la precisione scelta.
static void test_star_hash_use_selected_seconds_precision() {
  Timer t;
  t.turnPrecision(1);
  t.turnPrecision(1);  // 3 s
  t.type("100");
  t.hash();
  TEST_ASSERT_EQUAL_UINT16(130, t.seg.shown);
}

// REQ-4.1, REQ-4.4: lista F-Stop come nel firmware precedente; riga 2 precisione alle
// colonne 0-5 e Offset alle colonne 6-8 (TECHNICAL C-13), riga 1 tipo [F].
static void test_precision_encoder_walks_the_fstop_list() {
  static const char* const rows[] = {"Pr1/1  00", "Pr1/2  00", "Pr1/3  00", "Pr1/4  00",
                                     "Pr1/6  00", "Pr1/8  00", "Pr1/12 00", "Pr1/16 00",
                                     "Pr1/24 00", "Pr1/32 00", "Pr1/64 00"};
  Timer t;
  t.settle();
  t.clickPrecision();
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [F]  ", t.row(0));
  for (uint8_t i = 0; i < 11; i++) {
    TEST_ASSERT_EQUAL_STRING_LEN(rows[i], t.row(1), 9);
    TEST_ASSERT_EQUAL_STRING("Ch:A \x01\x02", t.row(1) + 9);
    t.turnPrecision(1);
  }
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1  00", t.row(1), 9);
}

// REQ-4.3, REQ-4.4: base 10,0 s e 1/3.
static void test_hash_moves_by_a_third_of_stop_from_base() {
  Timer t;
  t.settle();
  t.type("100");
  t.turnPrecision(1);
  t.turnPrecision(1);  // 1/3
  t.clickPrecision();
  t.hash();
  TEST_ASSERT_EQUAL_UINT16(126, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/3 +01", t.row(1), 9);
  t.hash();
  TEST_ASSERT_EQUAL_UINT16(159, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/3 +02", t.row(1), 9);
  t.star();
  t.star();
  t.star();
  TEST_ASSERT_EQUAL_UINT16(79, t.seg.shown);  // 10 * 2^(-1/3) = 7,94
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/3 -01", t.row(1), 9);
}

// REQ-4.4: l'Offset resta visibile anche da fermo, senza riscritture.
static void test_offset_stays_on_screen() {
  Timer t;
  t.settle();
  t.clickPrecision();
  t.hash();
  t.hash();
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1 +02", t.row(1), 9);
  uint32_t writes = t.lcd.writes;
  t.advance(5000);
  TEST_ASSERT_EQUAL_UINT32(writes, t.lcd.writes);
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1 +02", t.lcd.cells[1], 9);
}

// REQ-4.3: l'Offset non va oltre il punto in cui il tempo uscirebbe da 0,1-999,9 s.
static void test_offset_stops_where_time_would_leave_limits() {
  Timer t;
  t.settle();
  t.type("5000");
  t.clickPrecision();  // 1/1
  t.hash();            // 1000,0 s: fuori limite
  TEST_ASSERT_EQUAL_UINT16(5000, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1  00", t.row(1), 9);
  t.star();
  TEST_ASSERT_EQUAL_UINT16(2500, t.seg.shown);
  t.type("5");
  for (int i = 0; i < 5; i++) t.star();  // 0,25 -> 0,3 / 0,125 -> 0,1 / 0,0625 -> 0,1 / 0,03: fuori limite
  TEST_ASSERT_EQUAL_UINT16(1, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1 -03", t.row(1), 9);
}

// REQ-4.3, REQ-4.4: l'Offset ha due cifre, si ferma a +99.
static void test_offset_stops_at_two_digits() {
  Timer t;
  t.settle();
  t.type("100");
  t.turnPrecision(-1);  // 1/64
  t.clickPrecision();
  for (int i = 0; i < 120; i++) t.hash();
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/64+99", t.row(1), 9);
  TEST_ASSERT_EQUAL_UINT16(292, t.seg.shown);  // 10 * 2^(99/64) = 29,22
}

// REQ-4.5: cambiare precisione in F-Stop ricalcola il tempo da Tempo base e Offset.
static void test_turning_precision_in_fstop_recomputes_time() {
  Timer t;
  t.type("100");
  t.turnPrecision(1);
  t.turnPrecision(1);  // 1/3
  t.clickPrecision();
  t.hash();
  t.hash();
  t.turnPrecision(1);  // 1/4: 10 * 2^(2/4)
  TEST_ASSERT_EQUAL_UINT16(141, t.seg.shown);
  t.turnPrecision(-1);
  t.turnPrecision(-1);  // 1/2: 10 * 2^(2/2)
  TEST_ASSERT_EQUAL_UINT16(200, t.seg.shown);
}

// REQ-4.5, REQ-4.3: se il cambio di precisione porta il tempo oltre 999,9 s il tempo si
// limita, `#` non va oltre e `*` torna indietro.
static void test_offset_beyond_limit_after_precision_change_can_come_back() {
  Timer t;
  t.settle();
  t.type("3000");
  for (int i = 0; i < 3; i++) t.turnPrecision(1);  // 1/4
  t.clickPrecision();
  for (int i = 0; i < 3; i++) t.hash();  // 300 * 2^(3/4) = 504,5 s
  TEST_ASSERT_EQUAL_UINT16(5045, t.seg.shown);
  for (int i = 0; i < 3; i++) t.turnPrecision(-1);  // 1/1: 300 * 2^3 = 2400 s
  TEST_ASSERT_EQUAL_UINT16(9999, t.seg.shown);
  t.hash();
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1 +03", t.row(1), 9);
  t.star();  // +02: 1200 s, ancora oltre
  TEST_ASSERT_EQUAL_UINT16(9999, t.seg.shown);
  t.star();  // +01: 600 s
  TEST_ASSERT_EQUAL_UINT16(6000, t.seg.shown);
}

// REQ-4.6: il pulsante alterna Secondi e F-Stop; passando a F-Stop il Tempo base è il
// tempo corrente con Offset zero.
static void test_precision_click_toggles_type_and_rebases() {
  Timer t;
  t.settle();
  t.type("100");
  t.hash();  // Secondi, 1 s: 11,0 s
  t.clickPrecision();
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1  00", t.row(1), 9);
  t.hash();  // 1/1 da 11,0 s
  TEST_ASSERT_EQUAL_UINT16(220, t.seg.shown);
  t.clickPrecision();
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [S]  ", t.row(0));
  TEST_ASSERT_EQUAL_STRING_LEN("Pr: 1s   Ch:A", t.row(1), 13);
  TEST_ASSERT_EQUAL_UINT16(220, t.seg.shown);
  t.clickPrecision();
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1  00", t.row(1), 9);
  t.hash();
  TEST_ASSERT_EQUAL_UINT16(440, t.seg.shown);
}

// REQ-5.3: un tempo digitato diventa il Tempo base con Offset zero.
static void test_typed_time_becomes_base_with_zero_offset() {
  Timer t;
  t.settle();
  t.clickPrecision();
  t.hash();
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1 +01", t.row(1), 9);
  t.type("50");
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1  00", t.row(1), 9);
  t.hash();
  TEST_ASSERT_EQUAL_UINT16(100, t.seg.shown);
}

// REQ-5.9: durante l'Esposizione l'encoder della precisione è ignorato.
static void test_precision_encoder_is_ignored_during_countdown() {
  Timer t;
  t.settle();
  t.press();
  t.turnPrecision(1);
  t.clickPrecision();
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [S]  ", t.row(0));
  TEST_ASSERT_EQUAL_STRING_LEN("Pr: 1s  ", t.row(1), 8);
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
  RUN_TEST(test_star_hash_step_by_seconds_precision);
  RUN_TEST(test_seconds_step_stays_within_limits);
  RUN_TEST(test_first_digit_after_star_starts_a_new_time);
  RUN_TEST(test_precision_encoder_walks_the_seconds_list);
  RUN_TEST(test_star_hash_use_selected_seconds_precision);
  RUN_TEST(test_precision_encoder_walks_the_fstop_list);
  RUN_TEST(test_hash_moves_by_a_third_of_stop_from_base);
  RUN_TEST(test_offset_stays_on_screen);
  RUN_TEST(test_offset_stops_where_time_would_leave_limits);
  RUN_TEST(test_offset_stops_at_two_digits);
  RUN_TEST(test_turning_precision_in_fstop_recomputes_time);
  RUN_TEST(test_offset_beyond_limit_after_precision_change_can_come_back);
  RUN_TEST(test_precision_click_toggles_type_and_rebases);
  RUN_TEST(test_typed_time_becomes_base_with_zero_offset);
  RUN_TEST(test_precision_encoder_is_ignored_during_countdown);
  return UNITY_END();
}
