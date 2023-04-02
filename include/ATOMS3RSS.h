#ifndef ATOMS3RSS_H_
#define ATOMS3RSS_H_

// Tag name of log.
static const char *TAG = "ATOMS3RSS";

// WiFi connect check retry count
static const uint8_t WIFI_RETRY_COUNT = 10;
// HTTP timeout(millisecond)
static const int HTTP_TIME_OUT = 5000;

// News title
static const uint8_t MAX_TITLE_COUNT = 10;
static const uint8_t MAX_TITLE_LENGTH = 128;
typedef struct
{
    char title[MAX_TITLE_COUNT][MAX_TITLE_LENGTH];
    uint8_t titleCount;
    uint8_t titleIndex;
} NewsTitle;

boolean initWiFi();
boolean getRSS();
void clearDisplay();
bool validateNumber(const char *input);
void XML_callback(
    uint8_t statusflags, char *tagName, uint16_t tagNameLen,
    char *data, uint16_t dataLen);

#endif
