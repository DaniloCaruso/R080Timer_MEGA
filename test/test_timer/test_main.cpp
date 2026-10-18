// Seam 1: dominio intero con il TimingCore vero e driver finti. I test agiscono
// solo con input dell'utente e tempo che passa.
#include <unity.h>
#include <Controller.h>
#include <Counters.h>
#include <Screen.h>
#include <Settings.h>
#include <SoundScheduler.h>
#include "../fakes/FakeBuzzer.h"
#include "../fakes/FakeLcd.h"
#include "../fakes/FakeLog.h"
#include "../fakes/FakeSevenSeg.h"
#include "../fakes/FakeStorage.h"
#include "../fakes/SimClock.h"

// EEPROM della scheda: sopravvive al Timer, così un riavvio simulato è un Timer nuovo.
static FakeStorage eeprom;

struct Timer {
  SimClock clock;
  FakeSevenSeg seg;
  FakeLcd lcd;
  FakeLog log;
  FakeBuzzer buzzer{clock.nowMs};
  SoundScheduler sound{buzzer};
  Settings settings{eeprom};
  Controller ctl{clock, seg, sound, settings};
  Screen screen{lcd, log, ctl, "2.0.0"};
  Counters counters{eeprom};

  // Avvio come setup(): la causa del reset arriva dal marcatore del watchdog (SystemGuard).
  explicit Timer(bool watchdogReset = false) {
    settings.load();
    counters.boot(watchdogReset);
    if (watchdogReset) screen.showWatchdogReset();
    loop();
  }
  void loop() {  // un giro di loop()
    ctl.update();
    sound.tick(clock.nowMs);
    screen.render(clock.nowMs);
    settings.service(clock.nowMs);
  }
  void type(const char* digits) {
    for (; *digits; digits++) { ctl.onInput(Input::Digit, *digits - '0'); loop(); }
  }
  void press() { ctl.onInput(Input::MainPress, 0); loop(); }
  void hold() { ctl.onInput(Input::StarHold, 0); loop(); }  // `*` tenuto 1 s, poi rilasciato
  void input(Input in, int8_t arg = 0) { ctl.onInput(in, arg); loop(); }
  void star() { input(Input::Star); }
  void hash() { input(Input::Hash); }
  void turnPrecision(int8_t dir) { input(Input::PrecTurn, dir); }
  void clickPrecision() { input(Input::PrecClick); }
  void channel(char c) { input(Input::Channel, c - 'A'); }
  void keyD() { input(Input::KeyD); }
  void holdHash() { input(Input::HashHold); }  // `#` tenuto 1 s, poi rilasciato
  void turnMode(int8_t dir) { input(Input::ModeTurn, dir); }
  void clickMode() { input(Input::ModeClick); }
  void settle() { advance(1600); }  // oltre la schermata di avvio, LCD aggiornato
  // Le due righe dell'LCD dopo i giri di loop necessari a mandarle.
  const char* row(uint8_t r) { advance(2); return lcd.cells[r]; }
  void advance(uint32_t ms) {
    while (ms--) { clock.tick(); loop(); }
  }
};

void setUp() { eeprom.erase(); }
void tearDown() {}

// Controlla la nota n del buzzer: istante, frequenza e durata.
static void assertNote(const Timer& t, uint8_t n, uint32_t atMs, uint16_t hz, uint16_t ms) {
  TEST_ASSERT_TRUE_MESSAGE(n < t.buzzer.count, "nota mancante");
  TEST_ASSERT_EQUAL_UINT32(atMs, t.buzzer.notes[n].atMs);
  TEST_ASSERT_EQUAL_UINT16(hz, t.buzzer.notes[n].hz);
  TEST_ASSERT_EQUAL_UINT16(ms, t.buzzer.notes[n].ms);
}

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

// REQ-3.8, REQ-8.7: start con tempo zero emette il beep di errore e resta fermo.
static void test_start_with_zero_time_beeps_error_and_stays_idle() {
  Timer t;
  t.type("0");
  t.advance(10);
  t.press();
  TEST_ASSERT_FALSE(t.clock.relay);
  t.advance(1);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(0, t.seg.shown);
  TEST_ASSERT_EQUAL_UINT8(1, t.buzzer.count);
  assertNote(t, 0, 10, 150, 300);
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

// REQ-8.1, REQ-8.7: beep acuto breve all'inizio dell'Esposizione.
static void test_start_beeps() {
  Timer t;
  t.advance(2000);
  t.press();
  TEST_ASSERT_EQUAL_UINT8(1, t.buzzer.count);
  assertNote(t, 0, 2000, 1000, 60);
}

// REQ-1.6, REQ-8.7: tick grave a ogni secondo dall'inizio del Countdown.
static void test_metronome_ticks_every_second_of_countdown() {
  Timer t;
  t.type("35");
  t.press();
  t.advance(3499);
  TEST_ASSERT_EQUAL_UINT8(4, t.buzzer.count);
  assertNote(t, 1, 1000, 300, 80);
  assertNote(t, 2, 2000, 300, 80);
  assertNote(t, 3, 3000, 300, 80);
}

// Suono di fine: 600, 450, 300 Hz da 150 ms con pause di 80 ms, a partire da atMs.
static void assertEndSound(const Timer& t, uint8_t first, uint32_t atMs) {
  TEST_ASSERT_EQUAL_UINT8(first + 3, t.buzzer.count);
  assertNote(t, first, atMs, 600, 150);
  assertNote(t, first + 1, atMs + 230, 450, 150);
  assertNote(t, first + 2, atMs + 460, 300, 150);
}

// REQ-8.4, REQ-8.7: tre toni discendenti alla fine del Countdown.
static void test_end_of_countdown_plays_end_sound() {
  Timer t;
  t.type("35");
  t.press();
  t.advance(5000);
  assertEndSound(t, 4, 3500);
}

// REQ-8.4: lo stesso Suono di fine all'Annullamento.
static void test_cancel_plays_end_sound() {
  Timer t;
  t.type("35");
  t.press();
  t.advance(500);
  t.press();
  t.advance(2000);
  assertEndSound(t, 1, 500);
}

// REQ-5.5, REQ-7.5: `*` tenuto 1 s alterna il mute con il beep di conferma, toglie e
// rimette la nota sull'LCD e non tocca il tempo impostato.
static void test_holding_star_toggles_mute_with_confirm_beep() {
  Timer t;
  t.type("12");
  t.advance(2000);
  t.hold();
  t.advance(1);
  TEST_ASSERT_EQUAL_UINT8(1, t.buzzer.count);
  assertNote(t, 0, 2000, 1200, 40);
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:A  \x02", t.lcd.cells[1]);
  TEST_ASSERT_EQUAL_UINT16(12, t.seg.shown);
  TEST_ASSERT_FALSE(t.clock.relay);

  t.advance(1000);
  t.hold();
  t.advance(1);
  TEST_ASSERT_EQUAL_UINT8(2, t.buzzer.count);
  assertNote(t, 1, 3001, 1200, 40);
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:A \x01\x02", t.lcd.cells[1]);
}

// REQ-8.5: con il mute restano muti avvio, Metronomo ed errore; il Suono di fine suona.
static void test_mute_silences_everything_but_end_sound() {
  Timer t;
  t.hold();
  t.advance(100);
  t.type("0");
  t.press();
  t.type("15");
  t.press();
  t.advance(1400);
  TEST_ASSERT_EQUAL_UINT8(1, t.buzzer.count);  // la sola conferma del mute
  t.advance(1000);
  assertEndSound(t, 1, 1600);
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

// REQ-6.1, REQ-6.2: A, B e C richiamano ciascuno la propria impostazione completa.
static void test_channels_store_and_restore_whole_setting() {
  Timer t;
  t.settle();
  t.type("100");
  t.turnPrecision(1);
  t.turnPrecision(1);  // 1/3
  t.clickPrecision();
  t.hash();
  TEST_ASSERT_EQUAL_UINT16(126, t.seg.shown);
  t.channel('B');
  TEST_ASSERT_EQUAL_UINT16(100, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [S]  ", t.row(0));
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:B \x01\x02", t.row(1));
  t.turnMode(1);
  t.type("50");
  t.holdHash();
  t.channel('A');
  TEST_ASSERT_EQUAL_UINT16(126, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [F]  ", t.row(0));
  TEST_ASSERT_EQUAL_STRING("Pr1/3 +01Ch:A \x01\x02", t.row(1));
  t.hash();  // Tempo base e Offset ripristinati
  TEST_ASSERT_EQUAL_UINT16(159, t.seg.shown);
  t.channel('B');
  TEST_ASSERT_EQUAL_UINT16(50, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING("TEST STRIP [S]CO", t.row(0));
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:B \x01 ", t.row(1));
  t.channel('C');
  TEST_ASSERT_EQUAL_UINT16(100, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [S]  ", t.row(0));
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:C \x01\x02", t.row(1));
}

// REQ-5.4: `D` azzera tempo, Tempo base e Offset del solo Canale attivo.
static void test_key_d_clears_time_base_and_offset_of_active_channel() {
  Timer t;
  t.settle();
  t.type("100");
  t.clickPrecision();
  t.hash();
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1 +01", t.row(1), 9);
  t.keyD();
  TEST_ASSERT_EQUAL_UINT16(0, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING_LEN("Pr1/1  00", t.row(1), 9);
  t.hash();  // Tempo base zero: l'F-Stop non ha da dove partire
  TEST_ASSERT_EQUAL_UINT16(0, t.seg.shown);
  t.channel('B');
  TEST_ASSERT_EQUAL_UINT16(100, t.seg.shown);
}

// REQ-5.6: `#` tenuto 1 s alterna l'Abilitazione relè del Canale attivo con il beep di
// conferma; a relè disabilitato l'Esposizione gira con la lampada spenta.
static void test_holding_hash_toggles_relay_enable() {
  Timer t;
  t.type("15");
  t.advance(2000);
  t.holdHash();
  t.advance(1);
  TEST_ASSERT_EQUAL_UINT8(1, t.buzzer.count);
  assertNote(t, 0, 2000, 1200, 40);
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:A \x01 ", t.lcd.cells[1]);
  TEST_ASSERT_EQUAL_UINT16(15, t.seg.shown);
  t.press();
  TEST_ASSERT_FALSE(t.clock.relay);
  t.advance(1000);
  TEST_ASSERT_EQUAL_UINT16(5, t.seg.shown);  // il tempo gira comunque
  TEST_ASSERT_FALSE(t.clock.relay);
  t.advance(500);
  TEST_ASSERT_EQUAL_UINT16(15, t.seg.shown);
  t.channel('B');  // l'Abilitazione è per Canale
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:B \x01\x02", t.row(1));
  t.channel('A');
  t.holdHash();
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:A \x01\x02", t.row(1));
  t.press();
  TEST_ASSERT_TRUE(t.clock.relay);
}

// REQ-5.10, REQ-6.3, REQ-7.10: una tacca cambia Modalità di uno, in tondo, e il tempo resta.
static void test_mode_encoder_steps_one_mode_and_keeps_time() {
  Timer t;
  t.settle();
  t.type("125");
  t.turnMode(1);
  TEST_ASSERT_EQUAL_STRING("TEST STRIP [S]CO", t.row(0));
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
  t.turnMode(1);
  TEST_ASSERT_EQUAL_STRING("FOCUS           ", t.row(0));
  TEST_ASSERT_EQUAL_STRING("         Ch:A \x01\x02", t.row(1));
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
  t.turnMode(1);
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [S]  ", t.row(0));
  t.turnMode(-1);
  TEST_ASSERT_EQUAL_STRING("FOCUS           ", t.row(0));
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
}

// REQ-5.9: durante l'Esposizione tastierino, encoder e cambio Canale sono ignorati.
static void test_channel_keys_and_mode_encoder_are_ignored_during_exposure() {
  Timer t;
  t.settle();
  t.press();
  t.channel('B');
  t.keyD();
  t.turnMode(1);
  t.holdHash();
  t.press();
  TEST_ASSERT_EQUAL_UINT16(100, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [S]  ", t.row(0));
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:A \x01\x02", t.row(1));
}

// REQ-6.7: il salvataggio parte 2 s dopo l'ultima modifica.
static void test_settings_are_saved_two_seconds_after_last_change() {
  Timer t;
  uint32_t atBoot = eeprom.written;  // i contatori di avvio
  t.type("12");
  t.advance(1000);
  t.type("5");
  t.advance(1999);
  TEST_ASSERT_EQUAL_UINT32(atBoot, eeprom.written);
  t.advance(1);
  TEST_ASSERT_TRUE(eeprom.written > atBoot);
  Timer rebooted;
  TEST_ASSERT_EQUAL_UINT16(125, rebooted.seg.shown);
}

// REQ-6.7: all'avvio di un'Esposizione si salva subito.
static void test_settings_are_saved_when_exposure_starts() {
  Timer t;
  uint32_t atBoot = eeprom.written;
  t.type("125");
  t.press();
  TEST_ASSERT_TRUE(eeprom.written > atBoot);
  Timer rebooted;
  TEST_ASSERT_EQUAL_UINT16(125, rebooted.seg.shown);
}

// REQ-6.8: si scrivono solo i byte cambiati; se nulla cambia, niente scritture.
static void test_save_writes_only_changed_bytes() {
  Timer t;
  t.type("100");
  t.advance(2000);
  uint32_t first = eeprom.written;
  t.type("125");  // cambiano tempo e Tempo base (byte basso) e il CRC
  t.advance(2000);
  TEST_ASSERT_EQUAL_UINT32(first + 3, eeprom.written);
  t.turnPrecision(1);
  t.turnPrecision(-1);
  t.advance(2000);
  TEST_ASSERT_EQUAL_UINT32(first + 3, eeprom.written);
}

// REQ-6.4: dopo un riavvio si ritrovano Canale attivo, impostazioni dei Canali e mute.
static void test_reboot_restores_active_channel_channels_and_mute() {
  {
    Timer t;
    t.type("125");
    t.channel('B');
    t.turnMode(1);
    t.type("50");
    t.holdHash();
    t.channel('C');
    t.clickPrecision();
    t.hold();  // mute
    t.advance(2000);
  }
  Timer t;
  t.settle();
  TEST_ASSERT_EQUAL_UINT16(100, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [F]  ", t.row(0));
  TEST_ASSERT_EQUAL_STRING("Pr1/1  00Ch:C  \x02", t.row(1));
  t.channel('A');
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
  t.channel('B');
  TEST_ASSERT_EQUAL_UINT16(50, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING("TEST STRIP [S]CO", t.row(0));
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:B   ", t.row(1));
}

// Salva un'impostazione lontana da quella di fabbrica: Canale B, 5,0 s, mute.
static void saveNonFactorySettings() {
  Timer t;
  t.channel('B');
  t.type("50");
  t.hold();
  t.advance(2000);
}

// Valori di fabbrica (TECHNICAL C-10): Canale A, Countdown, Secondi 1 s, 10,0 s, relè abilitato, mute spento.
static void assertFactoryDefaults() {
  Timer t;
  t.settle();
  TEST_ASSERT_EQUAL_UINT16(100, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [S]  ", t.row(0));
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:A \x01\x02", t.row(1));
  t.channel('B');
  TEST_ASSERT_EQUAL_UINT16(100, t.seg.shown);
}

// CRC8 polinomio 0x07, scritto qui per ricostruire un blocco con un CRC valido.
static uint8_t crc8(const uint8_t* p, uint8_t n) {
  uint8_t crc = 0;
  while (n--) {
    crc ^= *p++;
    for (int i = 0; i < 8; i++) crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
  }
  return crc;
}

// REQ-2.5: CRC errato porta ai valori di fabbrica.
static void test_bad_crc_loads_factory_defaults() {
  saveNonFactorySettings();
  eeprom.bytes[42] ^= 0x01;
  assertFactoryDefaults();
}

// Un Canale attivo fuori da A-C, anche con CRC giusto, non viene usato.
static void test_out_of_range_channel_loads_factory_defaults() {
  saveNonFactorySettings();
  eeprom.bytes[4] = 7;  // primo byte dopo l'intestazione: Canale attivo
  eeprom.bytes[42] = crc8(eeprom.bytes, 42);
  assertFactoryDefaults();
}

// REQ-2.5: versione sconosciuta, anche con CRC giusto, porta ai valori di fabbrica.
static void test_unknown_version_loads_factory_defaults() {
  saveNonFactorySettings();
  eeprom.bytes[2] = 2;
  eeprom.bytes[42] = crc8(eeprom.bytes, 42);
  assertFactoryDefaults();
}

// REQ-6.6: il vecchio formato (magic 0xA5 all'indirizzo 0, 13 byte) porta ai valori di fabbrica.
static void test_old_format_loads_factory_defaults() {
  saveNonFactorySettings();
  static const uint8_t old[13] = {0xA5, 10, 0, 2, 0, 50, 0, 50, 0, 50, 1, 1, 1};
  memcpy(eeprom.bytes, old, sizeof old);
  assertFactoryDefaults();
}

// REQ-3.2, REQ-3.6, REQ-8.1, REQ-8.2, REQ-8.4: in Focus la lampada resta accesa e il 7 segmenti
// mostra il tempo trascorso, senza Metronomo; la seconda pressione la spegne con il Suono di fine.
static void test_focus_lights_lamp_and_shows_elapsed_time_until_second_press() {
  Timer t;
  t.type("125");
  t.turnMode(-1);
  t.press();
  TEST_ASSERT_TRUE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(0, t.seg.shown);
  t.advance(1500);
  TEST_ASSERT_EQUAL_UINT16(15, t.seg.shown);
  t.advance(60000);
  TEST_ASSERT_TRUE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(615, t.seg.shown);
  TEST_ASSERT_EQUAL_UINT8(1, t.buzzer.count);  // solo l'avvio
  assertNote(t, 0, 0, 1000, 60);
  t.press();
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(61500, t.clock.relayOffAtMs);
  assertNote(t, 1, 61500, 600, 150);
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
}

// REQ-3.2: il Focus parte anche con tempo zero; a relè disabilitato la lampada resta spenta.
static void test_focus_starts_with_zero_time_and_respects_relay_enable() {
  Timer t;
  t.keyD();
  t.turnMode(-1);
  t.holdHash();
  t.press();
  TEST_ASSERT_FALSE(t.clock.relay);
  t.advance(2000);
  TEST_ASSERT_EQUAL_UINT16(20, t.seg.shown);
}

// REQ-3.3, REQ-3.4: Spegnimento automatico dopo 5 minuti (valore di fabbrica), avviso 10 s prima.
static void test_focus_turns_lamp_off_after_five_minutes_with_warning() {
  Timer t;
  t.turnMode(-1);
  t.press();
  t.advance(289999);
  TEST_ASSERT_EQUAL_UINT8(1, t.buzzer.count);
  t.advance(1);
  assertNote(t, 1, 290000, 450, 200);
  t.advance(300);
  assertNote(t, 2, 290300, 450, 200);
  t.advance(9699);
  TEST_ASSERT_TRUE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(2999, t.seg.shown);
  t.advance(1);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(300000, t.clock.relayOffAtMs);
  assertNote(t, 3, 300000, 600, 150);
  TEST_ASSERT_EQUAL_UINT16(100, t.seg.shown);
  t.press();  // si riaccende da capo
  TEST_ASSERT_TRUE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(0, t.seg.shown);
}

// REQ-7.12: valori di fabbrica 7 segmenti livello 3, LCD livello 3, LCD in Esposizione livello 1.
static void test_factory_brightness_levels() {
  Timer t;
  t.settle();
  TEST_ASSERT_EQUAL_UINT8(3, t.seg.level);
  TEST_ASSERT_EQUAL_UINT8(3, t.lcd.level);
  t.press();
  TEST_ASSERT_EQUAL_UINT8(1, t.lcd.level);
}

// REQ-5.7, REQ-7.2, REQ-7.11: in Focus `*`/`#` regolano l'LCD su 8 livelli, salvati; il tempo resta.
static void test_star_hash_in_focus_set_lcd_brightness() {
  {
    Timer t;
    t.turnMode(-1);
    t.hash();
    TEST_ASSERT_EQUAL_UINT8(4, t.lcd.level);
    for (int i = 0; i < 10; i++) t.hash();
    TEST_ASSERT_EQUAL_UINT8(8, t.lcd.level);
    for (int i = 0; i < 10; i++) t.star();
    TEST_ASSERT_EQUAL_UINT8(1, t.lcd.level);
    t.hash();
    TEST_ASSERT_EQUAL_UINT8(2, t.lcd.level);
    TEST_ASSERT_EQUAL_UINT8(3, t.seg.level);
    TEST_ASSERT_EQUAL_UINT16(100, t.seg.shown);
    t.advance(2000);
  }
  Timer rebooted;
  TEST_ASSERT_EQUAL_UINT8(2, rebooted.lcd.level);
}

// REQ-5.8, REQ-7.2, REQ-7.11: in Focus `0`+`*`/`#` regolano il 7 segmenti su 8 livelli, salvati.
static void test_zero_with_star_hash_in_focus_set_seven_seg_brightness() {
  {
    Timer t;
    t.turnMode(-1);
    t.input(Input::ZeroHash);
    TEST_ASSERT_EQUAL_UINT8(4, t.seg.level);
    for (int i = 0; i < 10; i++) t.input(Input::ZeroHash);
    TEST_ASSERT_EQUAL_UINT8(8, t.seg.level);
    for (int i = 0; i < 10; i++) t.input(Input::ZeroStar);
    TEST_ASSERT_EQUAL_UINT8(1, t.seg.level);
    t.input(Input::ZeroHash);
    TEST_ASSERT_EQUAL_UINT8(2, t.seg.level);
    TEST_ASSERT_EQUAL_UINT8(3, t.lcd.level);
    TEST_ASSERT_EQUAL_UINT16(100, t.seg.shown);
    t.advance(2000);
  }
  Timer rebooted;
  TEST_ASSERT_EQUAL_UINT8(2, rebooted.seg.level);
}

// REQ-5.7, REQ-5.8: fuori dal Focus `*`/`#` restano passi di tempo e `0`+`*`/`#` non fa nulla.
static void test_brightness_keys_work_only_in_focus() {
  Timer t;
  t.input(Input::ZeroHash);
  t.hash();
  TEST_ASSERT_EQUAL_UINT8(3, t.seg.level);
  TEST_ASSERT_EQUAL_UINT8(3, t.lcd.level);
  TEST_ASSERT_EQUAL_UINT16(110, t.seg.shown);
}

// REQ-7.3, REQ-7.4: durante ogni Esposizione l'LCD scende al livello di esposizione e poi torna;
// il 7 segmenti resta alla luminosità scelta.
static void test_lcd_dims_during_exposure_and_seven_seg_keeps_its_level() {
  Timer t;
  t.turnMode(-1);
  t.hash();
  t.input(Input::ZeroHash);
  t.press();
  TEST_ASSERT_EQUAL_UINT8(1, t.lcd.level);
  TEST_ASSERT_EQUAL_UINT8(4, t.seg.level);
  t.press();
  TEST_ASSERT_EQUAL_UINT8(4, t.lcd.level);
  t.turnMode(1);
  t.press();
  TEST_ASSERT_EQUAL_UINT8(1, t.lcd.level);
  t.advance(10000);
  TEST_ASSERT_EQUAL_UINT8(4, t.lcd.level);
  TEST_ASSERT_EQUAL_UINT8(4, t.seg.level);
}

// REQ-7.10: in Focus la riga 2 non mostra la precisione, anche con la lampada accesa.
static void test_focus_shows_no_precision_while_lit() {
  Timer t;
  t.settle();
  t.clickPrecision();
  t.turnMode(-1);
  t.press();
  TEST_ASSERT_EQUAL_STRING("         Ch:A \x01\x02", t.row(1));
}

// REQ-9.9: dopo un reset da watchdog l'LCD mostra `RESET: WDT` per 3 s, poi l'uso normale.
static void test_watchdog_reset_shows_message_for_three_seconds() {
  Timer t(true);
  t.advance(1);
  TEST_ASSERT_EQUAL_STRING("RESET: WDT      ", t.lcd.cells[0]);
  TEST_ASSERT_EQUAL_STRING("                ", t.lcd.cells[1]);
  t.advance(2997);  // 2998 ms
  TEST_ASSERT_EQUAL_STRING("RESET: WDT      ", t.lcd.cells[0]);
  t.advance(4);
  TEST_ASSERT_EQUAL_STRING("COUNTDOWN  [S]  ", t.lcd.cells[0]);
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:A \x01\x02", t.lcd.cells[1]);
}

// Ogni accensione incrementa il contatore delle accensioni, che sopravvive al riavvio.
static void test_each_power_on_is_counted() {
  { Timer t; }
  { Timer t; }
  Timer t;
  TEST_ASSERT_EQUAL_UINT16(3, t.counters.d.powerOns);
  TEST_ASSERT_EQUAL_UINT16(0, t.counters.d.watchdogResets);
}

// REQ-9.9: un avvio dopo un reset da watchdog incrementa il suo contatore, non quello delle accensioni.
static void test_watchdog_reset_is_counted() {
  { Timer t; }
  { Timer t(true); }
  Timer t(true);
  TEST_ASSERT_EQUAL_UINT16(1, t.counters.d.powerOns);
  TEST_ASSERT_EQUAL_UINT16(2, t.counters.d.watchdogResets);
}

// Il blocco contatori ha il suo CRC8: se è corrotto i contatori ripartono da zero, le impostazioni restano.
static void test_corrupt_counters_restart_from_zero_and_keep_settings() {
  { Timer t; }
  saveNonFactorySettings();
  eeprom.bytes[0x040 + 16] ^= 0x01;  // CRC del blocco contatori (17 byte)
  Timer t;
  t.settle();
  TEST_ASSERT_EQUAL_UINT16(1, t.counters.d.powerOns);
  TEST_ASSERT_EQUAL_UINT16(50, t.seg.shown);  // Canale B, 5,0 s
}

// REQ-2.8, REQ-2.1: dopo un riavvio a metà Esposizione il timer è fermo con il relè spento.
static void test_reboot_mid_exposure_stays_idle_with_relay_off() {
  {
    Timer t;
    t.type("125");
    t.press();
    t.advance(3000);
    TEST_ASSERT_TRUE(t.clock.relay);
  }
  Timer t;
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_FALSE(t.clock.running());
  t.advance(20000);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);  // il tempo impostato, pronto per ripartire
  TEST_ASSERT_EQUAL_UINT8(0, t.buzzer.count);  // nessun suono di fine di un'Esposizione mai ripresa
}

// Istanti delle note a hz, in ordine.
static uint8_t notesAt(const Timer& t, uint16_t hz, uint32_t* atMs, uint8_t max) {
  uint8_t n = 0;
  for (uint8_t i = 0; i < t.buzzer.count && n < max; i++)
    if (t.buzzer.notes[i].hz == hz) atMs[n++] = t.buzzer.notes[i].atMs;
  return n;
}

// Test strip dalla Modalità di fabbrica (Countdown): una tacca dell'encoder della Modalità.
static void enterTestStrip(Timer& t) { t.turnMode(1); }

// REQ-3.9, REQ-3.11, REQ-8.7: in Secondi gli Step cadono a T, T+P, T+2P; la lampada resta
// accesa e a ogni Step suona il beep di step (600 Hz, 150 ms).
static void test_strip_seconds_steps_keep_lamp_on_and_beep() {
  Timer t;
  enterTestStrip(t);
  t.turnPrecision(1);  // 2 s
  t.type("50");
  t.press();
  t.advance(9500);
  TEST_ASSERT_TRUE(t.clock.relay);
  uint32_t at[8];
  TEST_ASSERT_EQUAL_UINT8(3, notesAt(t, 600, at, 8));
  TEST_ASSERT_EQUAL_UINT32(5000, at[0]);
  TEST_ASSERT_EQUAL_UINT32(7000, at[1]);
  TEST_ASSERT_EQUAL_UINT32(9000, at[2]);
  for (uint8_t i = 0; i < t.buzzer.count; i++)
    if (t.buzzer.notes[i].hz == 600) TEST_ASSERT_EQUAL_UINT16(150, t.buzzer.notes[i].ms);
}

// REQ-3.10: F-Stop 1/2 da 10,0 s, ogni Step calcolato dalla base: 10,0, 14,1, 20,0, 28,3 s.
static void test_strip_fstop_steps_from_base() {
  Timer t;
  enterTestStrip(t);
  t.type("100");
  t.turnPrecision(1);  // 1/2
  t.clickPrecision();
  t.press();
  t.advance(30000);
  uint32_t at[8];
  TEST_ASSERT_EQUAL_UINT8(4, notesAt(t, 600, at, 8));
  TEST_ASSERT_EQUAL_UINT32(10000, at[0]);
  TEST_ASSERT_EQUAL_UINT32(14100, at[1]);
  TEST_ASSERT_EQUAL_UINT32(20000, at[2]);
  TEST_ASSERT_EQUAL_UINT32(28300, at[3]);
  TEST_ASSERT_EQUAL_STRING("TEST STRIP [F]CO", t.row(0));  // il tipo resta visibile durante la prova
}

// REQ-3.10: con 1/64 e base 0,1 s l'arrotondamento darebbe Step uguali: crescono di 0,1 s.
static void test_strip_fstop_steps_grow_at_least_a_tenth() {
  Timer t;
  enterTestStrip(t);
  t.type("1");
  t.turnPrecision(-1);  // 1/64
  t.clickPrecision();
  t.press();
  t.advance(350);
  uint32_t at[8];
  TEST_ASSERT_EQUAL_UINT8(3, notesAt(t, 600, at, 8));
  TEST_ASSERT_EQUAL_UINT32(100, at[0]);
  TEST_ASSERT_EQUAL_UINT32(200, at[1]);
  TEST_ASSERT_EQUAL_UINT32(300, at[2]);
}

// REQ-7.5, REQ-7.6, REQ-7.7: riga 1 come il firmware precedente (Avanzamento `CO`); durante la
// prova `St:n` al posto della precisione e il 7 segmenti col tempo cumulativo.
static void test_strip_shows_step_number_and_cumulative_time() {
  Timer t;
  t.settle();
  enterTestStrip(t);
  TEST_ASSERT_EQUAL_STRING("TEST STRIP [S]CO", t.row(0));
  t.type("50");
  t.press();
  TEST_ASSERT_EQUAL_STRING("St:1     Ch:A \x01\x02", t.row(1));
  TEST_ASSERT_EQUAL_UINT16(0, t.seg.shown);
  t.advance(2498);  // 2,5 s dallo start: row() ne ha fatti passare 2
  TEST_ASSERT_EQUAL_UINT16(25, t.seg.shown);
  t.advance(2500);
  TEST_ASSERT_EQUAL_UINT16(50, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING("St:2     Ch:A \x01\x02", t.row(1));
  t.press();
  TEST_ASSERT_EQUAL_STRING("Pr: 1s   Ch:A \x01\x02", t.row(1));
  TEST_ASSERT_EQUAL_UINT16(50, t.seg.shown);  // REQ-3.7
}

// REQ-4.7: in Test strip il pulsante dell'encoder della Modalità alterna l'Avanzamento;
// in Countdown non fa nulla.
static void test_mode_click_toggles_advance_in_test_strip() {
  Timer t;
  t.settle();
  t.clickMode();
  enterTestStrip(t);
  TEST_ASSERT_EQUAL_STRING("TEST STRIP [S]CO", t.row(0));
  t.clickMode();
  TEST_ASSERT_EQUAL_STRING("TEST STRIP [S]PP", t.row(0));
  t.clickMode();
  TEST_ASSERT_EQUAL_STRING("TEST STRIP [S]CO", t.row(0));
}

// REQ-3.15, REQ-3.13, REQ-8.4: il pulsante principale annulla il test strip con il Suono di
// fine; la prova successiva riparte dal primo Step.
static void test_strip_cancel_and_restart_from_first_step() {
  Timer t;
  enterTestStrip(t);
  t.type("20");
  t.press();
  t.advance(3500);  // Step a 2,0 e 3,0 s
  t.press();
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(3500, t.clock.relayOffAtMs);
  t.advance(1000);
  uint32_t at[8];
  TEST_ASSERT_EQUAL_UINT8(1, notesAt(t, 450, at, 8));  // seconda nota del Suono di fine
  TEST_ASSERT_EQUAL_UINT32(3730, at[0]);
  t.press();
  TEST_ASSERT_TRUE(t.clock.relay);
  t.advance(3000);
  // 600 Hz: Step a 2,0 e 3,0 s, prima nota del Suono di fine, poi i primi Step della nuova prova.
  TEST_ASSERT_EQUAL_UINT8(5, notesAt(t, 600, at, 8));
  TEST_ASSERT_EQUAL_UINT32(3500, at[2]);
  TEST_ASSERT_EQUAL_UINT32(4500 + 2000, at[3]);
  TEST_ASSERT_EQUAL_UINT32(4500 + 3000, at[4]);
}

// REQ-8.2, REQ-8.3, REQ-8.7: nel test strip niente Metronomo; prima di ogni Step il
// Preavviso di tre tick (800 Hz, 30 ms) a -300, -200, -100 ms.
static void test_strip_prewarns_and_has_no_metronome() {
  Timer t;
  enterTestStrip(t);
  t.type("50");
  t.press();
  t.advance(5500);
  uint32_t at[8];
  TEST_ASSERT_EQUAL_UINT8(0, notesAt(t, 300, at, 8));
  TEST_ASSERT_EQUAL_UINT8(3, notesAt(t, 800, at, 8));
  TEST_ASSERT_EQUAL_UINT32(4700, at[0]);
  TEST_ASSERT_EQUAL_UINT32(4800, at[1]);
  TEST_ASSERT_EQUAL_UINT32(4900, at[2]);
  for (uint8_t i = 0; i < t.buzzer.count; i++)
    if (t.buzzer.notes[i].hz == 800) TEST_ASSERT_EQUAL_UINT16(30, t.buzzer.notes[i].ms);
}

// REQ-3.14, REQ-3.13: a 999,9 s il test strip si conclude con il Suono di fine; la prova
// successiva riparte dal primo Step.
static void test_strip_concludes_at_limit_and_restarts() {
  Timer t;
  enterTestStrip(t);
  t.type("9000");
  t.turnPrecision(-1);  // 20 s: Step a 900,0, 920,0, ... 980,0 s
  t.press();
  t.advance(999899);
  TEST_ASSERT_TRUE(t.clock.relay);
  t.advance(1);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(999900, t.clock.relayOffAtMs);
  t.advance(1000);
  uint32_t at[8];
  TEST_ASSERT_EQUAL_UINT8(1, notesAt(t, 450, at, 8));
  TEST_ASSERT_EQUAL_UINT32(999900 + 230, at[0]);
  TEST_ASSERT_EQUAL_UINT16(9000, t.seg.shown);
  t.press();
  t.advance(900000);
  TEST_ASSERT_EQUAL_UINT8(7, notesAt(t, 600, at, 8));  // 5 Step, fine, primo Step della nuova prova
  TEST_ASSERT_EQUAL_UINT32(1000900 + 900000, at[6]);
}

// Test strip passo-passo dalla Modalità di fabbrica.
static void enterSteppedStrip(Timer& t) {
  enterTestStrip(t);
  t.clickMode();
}

// REQ-3.12, REQ-1.3: primo impulso T, poi a ogni pressione solo l'incremento fino allo Step
// successivo; tra gli Step la lampada resta spenta.
static void test_stepped_strip_exposes_base_then_increments() {
  Timer t;
  enterSteppedStrip(t);
  t.turnPrecision(1);  // 2 s: Step a 5,0, 7,0, 9,0 s
  t.type("50");
  t.press();
  TEST_ASSERT_TRUE(t.clock.relay);
  uint32_t on = t.clock.nowMs;
  t.advance(5000);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(5000, t.clock.relayOffAtMs - on);
  t.advance(4000);  // in pausa la lampada resta spenta
  TEST_ASSERT_FALSE(t.clock.relay);
  for (int k = 0; k < 2; k++) {
    t.press();
    TEST_ASSERT_TRUE(t.clock.relay);
    on = t.clock.nowMs;
    t.advance(2000);
    TEST_ASSERT_FALSE(t.clock.relay);
    TEST_ASSERT_EQUAL_UINT32(2000, t.clock.relayOffAtMs - on);
  }
}

// REQ-7.8, REQ-7.6: in pausa il 7 segmenti mostra il tempo cumulativo dello Step successivo
// (F-Stop 1/2 da 10,0 s: 14,1 poi 20,0 s) e l'LCD il numero di quello Step.
static void test_stepped_strip_pause_shows_next_step() {
  Timer t;
  t.settle();
  enterSteppedStrip(t);
  t.type("100");
  t.turnPrecision(1);  // 1/2
  t.clickPrecision();
  t.press();
  t.advance(10000);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(141, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING("St:2     Ch:A \x01\x02", t.row(1));
  t.press();
  t.advance(2000);  // durante l'incremento: tempo cumulativo
  TEST_ASSERT_EQUAL_UINT16(120, t.seg.shown);
  t.advance(2100);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(200, t.seg.shown);
  TEST_ASSERT_EQUAL_STRING("St:3     Ch:A \x01\x02", t.row(1));
}

// REQ-3.12: una pressione letta nello stesso giro di loop dello Step (evento non ancora
// raccolto) esegue l'incremento, non riavvia la prova dal primo Step.
static void test_press_in_same_loop_as_step_resumes() {
  Timer t;
  enterSteppedStrip(t);
  t.type("50");
  t.press();
  t.advance(4999);
  t.clock.tick();  // lo Step scatta nella ISR, il loop legge prima gli input
  t.press();
  TEST_ASSERT_TRUE(t.clock.relay);
  uint32_t on = t.clock.nowMs;
  t.advance(1000);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(1000, t.clock.relayOffAtMs - on);
}

// Porta il passo-passo da 5,0 s in pausa al secondo Step (6,0 s).
static void pauseAtSecondStep(Timer& t) {
  enterSteppedStrip(t);
  t.type("50");
  t.press();
  t.advance(5000);
  t.press();
  t.advance(1000);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(70, t.seg.shown);
}

// Dopo la conclusione: 7 segmenti al Tempo base, nessuna ripresa, la prova riparte dal primo Step.
static void assertStripConcludedAndRestarts(Timer& t) {
  TEST_ASSERT_EQUAL_UINT16(50, t.seg.shown);
  t.advance(1000);
  uint32_t at[8];
  TEST_ASSERT_EQUAL_UINT8(1, notesAt(t, 450, at, 8));  // Suono di fine
  t.press();
  TEST_ASSERT_TRUE(t.clock.relay);
  uint32_t on = t.clock.nowMs;
  t.advance(5000);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(5000, t.clock.relayOffAtMs - on);
}

// REQ-3.16: `D` in pausa conclude il test strip senza azzerare il tempo.
static void test_key_d_in_pause_concludes_stepped_strip() {
  Timer t;
  pauseAtSecondStep(t);
  t.keyD();
  assertStripConcludedAndRestarts(t);
}

// REQ-3.16: anche A/B/C in pausa concludono il test strip.
static void test_channel_in_pause_concludes_stepped_strip() {
  Timer t;
  pauseAtSecondStep(t);
  t.channel('A');
  assertStripConcludedAndRestarts(t);
}

// REQ-3.16: anche il cambio di Modalità in pausa conclude il test strip.
static void test_mode_turn_in_pause_concludes_stepped_strip() {
  Timer t;
  pauseAtSecondStep(t);
  t.turnMode(1);
  assertStripConcludedAndRestarts(t);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_factory_brightness_levels);
  RUN_TEST(test_star_hash_in_focus_set_lcd_brightness);
  RUN_TEST(test_zero_with_star_hash_in_focus_set_seven_seg_brightness);
  RUN_TEST(test_brightness_keys_work_only_in_focus);
  RUN_TEST(test_lcd_dims_during_exposure_and_seven_seg_keeps_its_level);
  RUN_TEST(test_focus_shows_no_precision_while_lit);
  RUN_TEST(test_focus_lights_lamp_and_shows_elapsed_time_until_second_press);
  RUN_TEST(test_focus_starts_with_zero_time_and_respects_relay_enable);
  RUN_TEST(test_focus_turns_lamp_off_after_five_minutes_with_warning);
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
  RUN_TEST(test_start_with_zero_time_beeps_error_and_stays_idle);
  RUN_TEST(test_lcd_shows_name_and_version_at_power_on);
  RUN_TEST(test_lcd_shows_countdown_layout_after_splash);
  RUN_TEST(test_lcd_is_not_written_without_changes);
  RUN_TEST(test_countdown_ends_normally_with_lcd_offline);
  RUN_TEST(test_lcd_is_redrawn_when_it_comes_back_online);
  RUN_TEST(test_start_beeps);
  RUN_TEST(test_metronome_ticks_every_second_of_countdown);
  RUN_TEST(test_end_of_countdown_plays_end_sound);
  RUN_TEST(test_cancel_plays_end_sound);
  RUN_TEST(test_holding_star_toggles_mute_with_confirm_beep);
  RUN_TEST(test_mute_silences_everything_but_end_sound);
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
  RUN_TEST(test_channels_store_and_restore_whole_setting);
  RUN_TEST(test_key_d_clears_time_base_and_offset_of_active_channel);
  RUN_TEST(test_holding_hash_toggles_relay_enable);
  RUN_TEST(test_mode_encoder_steps_one_mode_and_keeps_time);
  RUN_TEST(test_channel_keys_and_mode_encoder_are_ignored_during_exposure);
  RUN_TEST(test_settings_are_saved_two_seconds_after_last_change);
  RUN_TEST(test_settings_are_saved_when_exposure_starts);
  RUN_TEST(test_save_writes_only_changed_bytes);
  RUN_TEST(test_reboot_restores_active_channel_channels_and_mute);
  RUN_TEST(test_bad_crc_loads_factory_defaults);
  RUN_TEST(test_out_of_range_channel_loads_factory_defaults);
  RUN_TEST(test_unknown_version_loads_factory_defaults);
  RUN_TEST(test_old_format_loads_factory_defaults);
  RUN_TEST(test_watchdog_reset_shows_message_for_three_seconds);
  RUN_TEST(test_each_power_on_is_counted);
  RUN_TEST(test_watchdog_reset_is_counted);
  RUN_TEST(test_corrupt_counters_restart_from_zero_and_keep_settings);
  RUN_TEST(test_reboot_mid_exposure_stays_idle_with_relay_off);
  RUN_TEST(test_strip_seconds_steps_keep_lamp_on_and_beep);
  RUN_TEST(test_strip_fstop_steps_from_base);
  RUN_TEST(test_strip_fstop_steps_grow_at_least_a_tenth);
  RUN_TEST(test_strip_shows_step_number_and_cumulative_time);
  RUN_TEST(test_mode_click_toggles_advance_in_test_strip);
  RUN_TEST(test_strip_cancel_and_restart_from_first_step);
  RUN_TEST(test_strip_prewarns_and_has_no_metronome);
  RUN_TEST(test_strip_concludes_at_limit_and_restarts);
  RUN_TEST(test_stepped_strip_exposes_base_then_increments);
  RUN_TEST(test_stepped_strip_pause_shows_next_step);
  RUN_TEST(test_press_in_same_loop_as_step_resumes);
  RUN_TEST(test_key_d_in_pause_concludes_stepped_strip);
  RUN_TEST(test_channel_in_pause_concludes_stepped_strip);
  RUN_TEST(test_mode_turn_in_pause_concludes_stepped_strip);
  return UNITY_END();
}
