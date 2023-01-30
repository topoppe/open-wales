#pragma once
#ifndef _LORAWAN_H
#define _LORAWAN_H


#include <Arduino.h>
#include <lmic.h>
#include "credentials.h"


static osjob_t sendjob;
void lorawan_init(unsigned send_interval);
void send_lorawan_data(uint8_t input[], int input_size);
void GoDeepSleep(void);
void LoadLMICFromRTC(void);
void SaveLMICToRTC(int deepsleep_sec);
void shutdown_lorawan(void);
void set_nss_pin(bool state);

#endif