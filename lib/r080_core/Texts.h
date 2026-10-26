// Testi dell'LCD nelle lingue offerte (ui-languages, TECHNICAL C-1): una tabella in flash con il
// testo di ogni lingua e le colonne disponibili. I testi uguali in ogni lingua (nomi delle Modalità,
// schermata di avvio, unità, formati dei numeri) restano dove servono.
#pragma once

#include <stdint.h>

enum Language : uint8_t { LANG_IT, LANG_EN, LANG_COUNT };

enum TextId : uint8_t { T_WDT_RESET, T_ADV_CONT, T_ADV_STEP, T_CHANNEL, T_STEP, T_MENU_TITLE, T_INFO,
                        T_COUNTERS, T_HEALTH, T_TEST_RELAY, T_TEST_BUZZER, T_TEST_SEVEN_SEG, T_TEST_LCD,
                        T_TEST_KEYPAD, T_TEST_ENCODERS, T_TEST_BUTTON, T_CALIBRATION, T_LCD_EXPOSURE,
                        T_FOCUS_LIMIT, T_LANGUAGE, T_FACTORY, T_EXIT, T_POWER_ONS, T_WDT_RESETS, T_EXPOSURES,
                        T_LAMP_HOURS, T_LOOP_MAX, T_ISR_MAX, T_LOST_TICKS, T_CAL_LOG, T_CAL_PPM,
                        T_CAL_MEASURE, T_CLICK_START, T_LOG_ACTIVE, T_DURATION_FMT, T_LEVEL_FMT,
                        T_MINUTES_FMT, T_RELAY_ON, T_RELAY_OFF, T_SOUND_FMT, T_SEG_ALL, T_KEY_FMT, T_PRESS,
                        T_PRESSED, T_RELEASED, T_CONFIRM_Q, T_YES_NO, T_CLICK_RESET, T_CLICK_CONFIRM,
                        TEXT_COUNT };

const char* text(TextId id, uint8_t lang);  // in flash (Flash.h); lingua sconosciuta: italiano
const char* languageName(uint8_t lang);     // in flash, scritto nella sua lingua
uint8_t textWidth(TextId id);               // colonne disponibili sull'LCD (per i formati: prima dell'espansione)
