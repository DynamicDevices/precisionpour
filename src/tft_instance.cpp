/**
 * TFT_eSPI Instance
 * 
 * This file creates the TFT_eSPI instance after User_Setup.h has been processed
 */

#include "config.h"

// Include User_Setup.h before TFT_eSPI.h
#include "User_Setup.h"
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
