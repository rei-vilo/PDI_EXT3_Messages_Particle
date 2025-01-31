///
/// @mainpage   PDI_EXT3_Messages_Particle
///
/// @details    Description of the project
///
/// @author     Rei Vilo
/// @author     https://embeddedcomputing.weebly.com
/// @date       03 Mar 2021
/// @version    201
///
/// @copyright  (c) Rei Vilo, 2021
/// @copyright  CC = BY SA NC
///

#ifndef ARDUINO
#define ARDUINO
#endif // ARDUINO

// Core library for code-sense - IDE-based
#include "Arduino.h"
#include "Particle.h"
#include "ArduinoJson.h"

// Ensure CONFIGURATION_OPTION is set to CONFIGURATION_EXT3_REDBEAR_DUO in hV_Configuration
#include "hV_Configuration.h"

/// Reference
/// @see https://docs.particle.io/reference/device-os/firmware/photon/#ssid-

// Set parameters
#define SMART_CONFIG 1
#define STANDARD_MODE 2
#define PROVISIONING_MODE STANDARD_MODE

///
/// @brief    Send debug as message
///
#define REPORT_MESSAGE 1

// RedBear Duo
// Serial for native USB port
// Serial1 for RBLink USB port
#define mySerial Serial

// Select mode
// SEMI_AUTOMATIC: run
/// @see https://docs.particle.io/reference/device-os/firmware/photon/#semi-automatic-mode
//SYSTEM_MODE(SEMI_AUTOMATIC);
// AUTOMATIC: first connect, then run
/// @see https://docs.particle.io/reference/device-os/firmware/photon/#automatic-mode
SYSTEM_MODE(AUTOMATIC);

// Define structures and classes
///
/// @brief Structure for stack
///
struct stack_s
{
    bool flag; ///< false=empty
    String event; ///< text
};

///
/// @brief Number of lines stringTrace
/// * Screen 4.2 400 x 300 = 10 lines
/// * Screen 3.71 416 x 240 = 8 lines
///
#define LAST_LINE 8

/// Screen 4.2 400 x 300
/// * Top: title, Terminal16x24
/// * Lines 1..11: events, Terminal16x24
/// * Bottom: last update, Terminal8x12
/// Screen 3.71 416 x 240
/// * Top: title, Terminal16x24
/// * Lines 0..9: events, Terminal16x24
/// * Bottom: last update, Terminal8x12
stack_s stackTop;
stack_s stackBottom;
stack_s stackScreen[LAST_LINE];
uint8_t stackIndex = 0;
stack_s stackWork;

// Include application, user and local libraries
#include "SPI.h"
#include "SparkJson.h"
#include "EnglishCalendar.h"

// Particle blink nightmare explained at https://docs.particle.io/tutorials/device-os/led/photon/

// Prototypes
void updateTrace();

// Variables
// Only ~53480 bytes available according to System.freeMemory();
// Enough SRAM for screens up to 4.20"

//Screen_EastRising myScreen(eScreen_EastRising_420_BW);
// === Pervasive Displays iTC
#include "ePaper_EXT3_Basic_Library.h"
// Screen_EPD_EXT3 myScreen(eScreen_EPD_EXT3_154_BWR);
// Screen_EPD_EXT3 myScreen(eScreen_EPD_EXT3_213_BWR);
// Screen_EPD_EXT3 myScreen(eScreen_EPD_EXT3_271_BWR);
// Screen_EPD_EXT3 myScreen(eScreen_EPD_EXT3_287_BWR);
// Screen_EPD_EXT3 myScreen(eScreen_EPD_EXT3_266_BWR);
Screen_EPD_EXT3 myScreen(eScreen_EPD_EXT3_370_BWR);
// Screen_EPD_EXT3 myScreen(eScreen_EPD_EXT3_420_BWR);

// Get from https://www.uuidgenerator.net/
const String eventName = "<first UUID>";
const String readyName = "<second UUID>";

bool flagTimeOnce = false; // flag for time get from Particle
bool flagFlush = false; // flag for refreshing the screen
bool symbolLED = false; // flag for D7
uint8_t day;
uint8_t oldDay = 0;
uint16_t localSizeX, localSizeY;
uint8_t fontSmall, fontMedium, fontLarge;
String stringTrace = "";
String workString;
bool flagStart = true;
uint32_t memory = 0;
bool isReady = false;

// Lines for display


// Prototypes
void myHandler(const char * event, const char * data);
void printCentered(uint16_t dY, String text, uint16_t colour = myColours.black);
void printStack();
void getParticleTime();
void updateTrace();


// Utilities
//
/// @brief    Wait with countdown
/// @param    second duration, s
///
void wait(String text, uint8_t second)
{
    for (uint8_t i = second; i > 0; i--)
    {
        Serial.print(formatString(" > %s %i  \r", text.c_str(), i));
        delay(1000);
    }
    Serial.print("         \r");
}

void printCentered(uint16_t dY, String text, uint16_t colour)
{
    // Check and resize text to fit in screen
    Serial.println(formatString("  1: .%s.", text.c_str()));
    uint16_t k = myScreen.stringLengthToFitX(text, localSizeX);
    text = text.substring(0, k);
    Serial.println(formatString("  2: .%s.", text.c_str()));

    // Center text
    uint16_t dX = (localSizeX - myScreen.stringSizeX(text)) / 2;
    myScreen.gText(dX, dY, text, colour);
}

void checkConnected()
{
    if (Particle.connected() == false)
    {
        mySerial.print("--- Particle.connect...");
        RGB.control(false);
        Particle.connect();
        while (!Particle.connected())
        {
            mySerial.print(".");
            delay(100);
        }
        mySerial.println(" done");
    }
    digitalWrite(D7, symbolLED);
}

void stackClear()
{
    for (uint8_t i = 0; i < LAST_LINE; i++)
    {
        stackScreen[i].flag = false;
        stackScreen[i].event = "";
    }
    stackIndex = 0;
}

void stackAdd(String text)
{
    if (stackIndex == LAST_LINE - 1)
    {
        for (uint8_t i = 0; i < LAST_LINE - 2; i++)
        {
            stackScreen[i] = stackScreen[i + 1];
        }
        stackScreen[LAST_LINE - 1] = { false, "" };
        stackIndex = LAST_LINE - 2;
    }

    stackScreen[stackIndex].flag = true;
    stackScreen[stackIndex].event = text;
    stackIndex++;
}

// Functions
void printStack()
{
    mySerial.println(" -- printStack");

    uint16_t x = 0;
    uint16_t y = 0;

    myScreen.clear();
    // Top
    Serial.println(formatString("  . Line %s: %s", "top", stackTop.event.c_str()));
    myScreen.selectFont(fontLarge);
    myScreen.setPenSolid(true);
    myScreen.dRectangle(x, y, localSizeX, myScreen.characterSizeY(), myColours.red);
    myScreen.setPenSolid(false);
    myScreen.gText(x, y, stackTop.event, myColours.white, myColours.red);
    y += myScreen.characterSizeY();

    // Lines
    uint8_t number = 0;
    for (uint8_t i = 0; i < LAST_LINE; i++)
    {
        number += (stackScreen[i].flag ? 1 : 0);
    }
    y += myScreen.characterSizeY() * (LAST_LINE - number) / 2;

    for (uint8_t i = 0; i < LAST_LINE; i++)
    {
        if (stackScreen[i].flag)
        {
            Serial.println(formatString("  . Line %i: %s", i, stackScreen[i].event.c_str()));
            // myScreen.gText(x, y, stackScreen[i].event, myColours.black);
            printCentered(y, stackScreen[i].event, myColours.black);
            y += myScreen.characterSizeY();
        }
    }

    // Bottom
    myScreen.selectFont(fontMedium);
    Serial.println(formatString("  . Line %s: %s", "bottom", stackBottom.event.c_str()));
    y = myScreen.screenSizeY() - myScreen.characterSizeY();
    myScreen.gText(x, y, stackBottom.event, myColours.gray);
}

void getParticleTime()
{
    // Synchronise time from Particle cloud
    // https://docs.particle.io/reference/device-os/firmware/photon/#particle-synctime-
    Particle.syncTime();

    // Current time
    mySerial.print(" -- getParticleTime...");
    while (!Particle.syncTimeDone())
    {
        mySerial.print(".");
        delay(300);
    }
    mySerial.println(Time.timeStr());
    flagTimeOnce = true;
}

// Utilities
// Particle received message
void myHandler(const char * event, const char * data)
{
    isReady = false;

    // https://docs.particle.io/reference/device-os/firmware/photon/#cloud-functions
    String topic = event; // 64 characters maximum
    String value = data; // 255 characters maximum

    mySerial.println("--- Particle handler");
    mySerial.print("  ! topic: ");
    mySerial.println(topic);
    mySerial.print("  ! value: ");
    mySerial.println(value);

    DynamicJsonBuffer jsonBuffer;
    JsonObject & root = jsonBuffer.parseObject((char *)value.c_str());
    topic = (String)(const char *)root["action"];

    if (topic == "LED")
    {
        value = (String)(const char *)root["data"];
        mySerial.print(">>> symbolLED: .");
        mySerial.print(value);
        mySerial.println(".");
        symbolLED = (value == "true");
        digitalWrite(D7, symbolLED);
    }
    else if (topic == "CLEAR")
    {
        value = "0";
        stackClear();
        myScreen.clear();
        symbolLED = false;
        digitalWrite(D7, symbolLED);
        flagFlush = true;
    }
    else if (topic == "UP")
    {
        value = "0";
        for (uint8_t i = 0; i < stackIndex; i++)
        {
            stackScreen[i] = stackScreen[i + 1];
        }
        stackScreen[stackIndex] = { false, "" };
        stackIndex = stackIndex - 1;
        printStack();
        flagFlush = true;
    }
    else if (topic == "UPDATE")
    {
        // First event
        String value1 = (String)(const char *)root["data"]["1"];
        String value2 = (String)(const char *)root["data"]["2"];

#if (LOCAL_SERIAL > 0)
        Serial.println(formatString("%2i: .%s.", 1, value1.c_str()));
        Serial.println(formatString("%2i: .%s.", 2, value2.c_str()));
#endif // LOCAL_SERIAL

        if (value1 != "")
        {
            stackAdd(value1);
        }
        if (value2 != "")
        {
            stackAdd(value2);
        }

        // Bottom
        stackBottom.flag = true;
        stackBottom.event = formatString("Last update: %02i:%02i:%02i", Time.hour(), Time.minute(), Time.second());

        Serial.println(formatString("%2i: .%s.", 2, value.c_str()));

        printStack();
        flagFlush = true;
    }
    updateTrace();

    delay(100);
}

void updateTrace()
{
#if (REPORT_MESSAGE > 0)
    mySerial.println("--- updateTrace()");

    stringTrace = "";

    // Open
    stringTrace += "{\"action\":\"REPORT\""; // start of message
    stringTrace += ",\"data\":{"; // start of data

    // Top
    stringTrace += "\"top\":\"" + stackTop.event + "\"";

    // Lines
    bool flag = false;
    stringTrace += ",\"lines\":\"";
    for (uint8_t index = 0; index <= LAST_LINE - 1; index++)
    {
        if (stackScreen[index].flag)
        {
            if (flag)
            {
                stringTrace += "<br>";
            }
            flag = true;
            stringTrace += stackScreen[index].event;
        }
    }
    stringTrace += "\"";

    // Bottom
    stringTrace += ",\"bottom\":\"" + stackBottom.event + "\"";

    // Miscellaneous
    stringTrace += ",\"LED\":\"";
    stringTrace += (symbolLED ? "true" : "false");

    stringTrace += "\"}"; // end of data

    // Close
    stringTrace += "}"; // end of message

    mySerial.print("  . ");
    mySerial.println(stringTrace);
#endif // REPORT_MESSAGE
}


// Add setup code
void setup()
{
    mySerial.begin(115200);
    delay(1000);

    wait("Wait", 16);

    mySerial.println("--- setup()");

    // Expose variables for debugging
#if (REPORT_MESSAGE > 0)
    Particle.variable("Trace", stringTrace);
#endif // REPORT_MESSAGE
    Particle.variable("Memory", memory);

    // Blue LED
    pinMode(D7, OUTPUT);
    digitalWrite(D7, LOW);

    memory = System.freeMemory();
    digitalWrite(D7, symbolLED);
    RGB.control(symbolLED);

    stackClear();

    // Default
    stackWork.flag = false;
    stackWork.event = "";

    mySerial.println();
    mySerial.println("=== " __FILE__);
    mySerial.println("=== " __DATE__ " " __TIME__);
    mySerial.println();
    mySerial.println("--- setup()");

    mySerial.print("  . myScreen.begin... ");

    myScreen.begin();
    myScreen.setOrientation(7); // landscape
    myScreen.setOrientation((myScreen.getOrientation() + 10) % 4); // landscape

    mySerial.println(myScreen.WhoAmI());

    // Fonts
#if (FONT_MODE == USE_FONT_TERMINAL)
    fontSmall = Font_Terminal8x12;
    fontMedium = Font_Terminal12x16;
    fontLarge = Font_Terminal16x24;
#else
#error FONT_MODE = USE_FONT_TERMINAL
#endif // FONT_MODE

    localSizeX = myScreen.screenSizeX();
    localSizeY = myScreen.screenSizeY();

    Serial.println(formatString("  . size= %i x %i", localSizeX, localSizeY));

    mySerial.println(" -- Particle");
    mySerial.print("  . System.version: ");
    mySerial.println(System.version());
    mySerial.print("  . Subscribe... ");

    // Connect to Particle cloud
    // https://docs.particle.io/reference/device-os/firmware/photon/#particle-subscribe-
    uint8_t result = Particle.subscribe(eventName, myHandler, MY_DEVICES);

    mySerial.println(result ? "done" : "error");
    mySerial.println();
    mySerial.println("--- loop()");

    // Time
    Time.zone(+1); // GMT +1
    delay(1000); // 1 s
}

void loop()
{
    memory = System.freeMemory();
    checkConnected();

    if (flagStart == true)
    {
        flagStart = false;
        flagFlush = true;
    }

    time_t lastSyncTimestamp;
    uint32_t lastSync = Particle.timeSyncedLast(lastSyncTimestamp);

    day = Time.day();
    if ((day != oldDay) or (lastSync == 0))
    {
        mySerial.println("--- New day");

        oldDay = day;

        // Synchronise time, Format today
        getParticleTime();
        workString = "Today:";
        workString += " " + LocalDay[Time.weekday() - 1];
        workString += " " + String(Time.day());
        workString += " " + LocalMonth[Time.month() - 1];
        workString += " " + String(Time.year());
        stackTop.flag = true;
        stackTop.event = workString;

        Serial.println(formatString("  . %s: %s", "Today", stackTop.event.c_str()));

        printStack();
        updateTrace();
    }

    if (flagFlush and flagTimeOnce)
    {
        Particle.publish(readyName, "false");

        mySerial.print("--- myScreen.flush()... ");

        myScreen.flush();

        mySerial.println("done");

        flagFlush = false;
        isReady = true;
        Particle.publish(readyName, "true");
        mySerial.println("  ! Ready");
    }
}
