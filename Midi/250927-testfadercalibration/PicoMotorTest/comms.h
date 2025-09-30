#pragma once
#include <Arduino.h>
void comms_begin();
bool comms_available_line();
String comms_take_line();
