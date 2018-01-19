#ifndef __STRUCTS_H
#define __STRUCTS_H

// Data
typedef enum VoltageRange {
    ONE_TO_ONE,
    THREE_TO_THREE,
    FIVE_TO_FIVE,
    TWELVE_TO_TWELVE,
    OVERLIMIT_VOLTAGE
} VoltageRange;
typedef enum CurrentRange {
    TEN_TO_TEN,
    TWO_HUNNIT_TO_TWO_HUNNIT,
    OVERLIMIT_CURRENT
} CurrentRange;
typedef enum ResistanceRange {
    ZERO_TO_ONE_K,
    ZERO_TO_ONE_MEG,
    OVERLIMIT_RESISTANCE
} ResistanceRange;
typedef struct {
    int32_t adcValue;
    bool newData;
    bool newSample;
} ADCReading;
typedef struct {
    int32_t adcValue;
    bool isSample;
} ADCRawReading;

// Screen
typedef enum ScreenUpdateType {
    BRIGHTNESS,
    ENTIRE_SCREEN,
    INITIAL_SCREEN,
    READING,
    CLEAR_SCREEN,
    TOP,
    CONTEXT
} ScreenUpdateType;
typedef struct {
    char message[32];
    ScreenUpdateType type;
    char from;
} ScreenUpdate;

// System
typedef enum SystemState {
    STATE_ON,
    STATE_ON_LOGGING,
    STATE_STANDBY
} SystemState;
typedef enum SystemLocationType {
    LOC_NULL,
    LOC_SPLASH,
    LOC_MENU,
        LOC_MEASURE,
            LOC_VOLTAGE_DC,
            LOC_VOLTAGE_AC,
            LOC_CURRENT,
            LOC_RESISTANCE,
            LOC_CONTINUITY,
            LOC_LOGIC,
        LOC_SETTINGS,
            LOC_SAMPLE,
            LOC_BRIGHTNESS,
        LOC_LOGGING,
            LOC_MEASUREMENT,
            LOC_LOG_SAMPLE,
            LOC_NUM_SAMPLES,
            LOC_LOG_PERIOD,
        LOC_POWER,
            LOC_STANDBY,
        LOC_RELAYS,
            LOC_RELAYA,
            LOC_RELAYB,
            LOC_RELAYC,
            LOC_RELAYD,
            LOC_RELAYE
} SystemLocationType;
typedef enum Units {
    UNITS_AC = 1, UNITS_DC,
    UNITS_CURRENT_AC, UNITS_CURRENT,
    UNITS_RES,
    UNITS_CONT,
    UNITS_LOG
} Units;
typedef enum ButtonType {
    B1 , B2 , B3 , B4,
    B5 , B6 , B7 , B8,
    B9 , B10, B11, B12,
    B13, B14, B15, B16,
    NO_BUTTON, NO_INPUT
} ButtonType;
typedef enum Command {
    CMD_NULL,
    CMD_B1 , CMD_B2 , CMD_B3 , CMD_B4,
    CMD_B5 , CMD_B6 , CMD_B7 , CMD_B8,
    CMD_B9 , CMD_B10, CMD_B11, CMD_B12,
    CMD_B13, CMD_B14, CMD_B15, CMD_B16
} Command;
typedef struct {
    SystemLocationType location;
    void (*onLoad)();
    void (*onUnload)();
} SystemLocation;
typedef enum Relay {
    RELAY_A,
    RELAY_B,
    RELAY_C,
    RELAY_D,
    RELAY_E
} Relay;

#endif
