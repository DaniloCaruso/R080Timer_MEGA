#include "Texts.h"

#include "Flash.h"

// Una riga per testo, nell'ordine di TextId: colonne disponibili, poi italiano e inglese.
// T_LEVEL_FMT ha 15 colonne perché %d può diventare -99.
struct Entry {
  uint8_t width;
  char t[LANG_COUNT][17];
};
static const Entry kTexts[] PROGMEM = {
    {16, {"RESET: WDT", "RESET: WDT"}},  // T_WDT_RESET
    {2, {"CO", "CO"}},  // T_ADV_CONT
    {2, {"PP", "SS"}},  // T_ADV_STEP
    {3, {"Ch:", "Ch:"}},  // T_CHANNEL
    {3, {"St:", "St:"}},  // T_STEP
    {16, {"MENU SERVIZIO", "SERVICE MENU"}},  // T_MENU_TITLE
    {15, {"Info", "Info"}},  // T_INFO
    {15, {"Contatori", "Counters"}},  // T_COUNTERS
    {15, {"Salute", "Health"}},  // T_HEALTH
    {15, {"Test rele", "Relay test"}},  // T_TEST_RELAY
    {15, {"Test buzzer", "Buzzer test"}},  // T_TEST_BUZZER
    {15, {"Test 7 segmenti", "7-segment test"}},  // T_TEST_SEVEN_SEG
    {15, {"Test LCD", "LCD test"}},  // T_TEST_LCD
    {15, {"Test tastierino", "Keypad test"}},  // T_TEST_KEYPAD
    {15, {"Test encoder", "Encoder test"}},  // T_TEST_ENCODERS
    {15, {"Test pulsante", "Button test"}},  // T_TEST_BUTTON
    {15, {"Calibrazione", "Calibration"}},  // T_CALIBRATION
    {15, {"LCD esposizione", "LCD exposure"}},  // T_LCD_EXPOSURE
    {15, {"Tempo max Focus", "Focus max time"}},  // T_FOCUS_LIMIT
    {15, {"Lingua/Language", "Lingua/Language"}},  // T_LANGUAGE
    {15, {"Ripristino fabb", "Factory reset"}},  // T_FACTORY
    {15, {"Esci", "Exit"}},  // T_EXIT
    {16, {"Accensioni", "Power-ons"}},  // T_POWER_ONS
    {16, {"Reset watchdog", "Watchdog resets"}},  // T_WDT_RESETS
    {16, {"Esposizioni", "Exposures"}},  // T_EXPOSURES
    {16, {"Ore lampada", "Lamp hours"}},  // T_LAMP_HOURS
    {16, {"Loop max", "Loop max"}},  // T_LOOP_MAX
    {16, {"ISR max", "ISR max"}},  // T_ISR_MAX
    {16, {"Tick persi", "Lost ticks"}},  // T_LOST_TICKS
    {16, {"Log calibrazione", "Calibration log"}},  // T_CAL_LOG
    {16, {"Correzione ppm", "PPM correction"}},  // T_CAL_PPM
    {16, {"Durata 600 s", "Measured 600 s"}},  // T_CAL_MEASURE
    {16, {"Clic: avvia", "Click: start"}},  // T_CLICK_START
    {16, {"Attivo, D: esci", "Active, D: exit"}},  // T_LOG_ACTIVE
    {16, {"%ld,%02ld s", "%ld.%02ld s"}},  // T_DURATION_FMT
    {15, {"Livello 1-8: %d", "Level 1-8: %d"}},  // T_LEVEL_FMT
    {16, {"Minuti: %ld", "Minutes: %ld"}},  // T_MINUTES_FMT
    {16, {"Clic: acceso", "Click: on"}},  // T_RELAY_ON
    {16, {"Clic: spento", "Click: off"}},  // T_RELAY_OFF
    {16, {"Suono %u: clic", "Sound %u: click"}},  // T_SOUND_FMT
    {16, {"Accesi: 888.8", "All on: 888.8"}},  // T_SEG_ALL
    {16, {"Tasto: %c", "Key: %c"}},  // T_KEY_FMT
    {16, {"Premi", "Press"}},  // T_PRESS
    {16, {"Premuto", "Pressed"}},  // T_PRESSED
    {16, {"Rilasciato", "Released"}},  // T_RELEASED
    {16, {"Confermi?", "Confirm?"}},  // T_CONFIRM_Q
    {16, {"clic=Si D=No", "click=Yes D=No"}},  // T_YES_NO
    {16, {"Clic: ripristina", "Click: reset"}},  // T_CLICK_RESET
    {16, {"Clic: conferma", "Click: confirm"}},  // T_CLICK_CONFIRM
};
static_assert(sizeof kTexts / sizeof kTexts[0] == TEXT_COUNT, "una riga per TextId");

static const char kLanguageNames[LANG_COUNT][9] PROGMEM = {"Italiano", "English"};

const char* text(TextId id, uint8_t lang) { return kTexts[id].t[lang < LANG_COUNT ? lang : uint8_t(LANG_IT)]; }

const char* languageName(uint8_t lang) { return kLanguageNames[lang < LANG_COUNT ? lang : uint8_t(LANG_IT)]; }

uint8_t textWidth(TextId id) { return fbyte(&kTexts[id].width); }
