#include <Arduino.h>
// M5Unified
#include <M5Unified.h>
// WiFi
#include <WiFiClientSecure.h>
// XML parser
#include <TinyXML.h>
// Common header file.
#include "ATOMS3RSS.h"
// Connection information
#include "ATOMS3Internet.h"

// WiFi
WiFiClientSecure client;
// XML
TinyXML xml;
uint8_t buffer[150]; // For XML decoding
// news title
NewsTitle newsTitle;
// variable to store ATOMS3 startup time
unsigned long startTime;
// Init flg
boolean initFlg = false;
// RSS received Flg
boolean rssRecvFlg = false;

// initWiFi
boolean initWiFi()
{
  // Clear Display
  clearDisplay();
  uint8_t retryCount = 0;
  ESP_LOGI(TAG, "Attempting to connect to SSID: %s", SSID);
  M5.Display.println("WiFi接続");
  WiFi.begin(SSID, PASSWORD);
  // 10 times WiFi connected check
  while (WiFi.status() != WL_CONNECTED && retryCount < WIFI_RETRY_COUNT)
  {
    retryCount++;
    // Wait 1 second
    delay(1000);
  }
  // Connected
  if (WiFi.status() == WL_CONNECTED)
  {
    ESP_LOGI(TAG, "Connected to %s", SSID);
    ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
    M5.Display.println("成功");
    return true;
  }
  // Not connected
  else
  {
    ESP_LOGI(TAG, "Fail to Connect to %s", SSID);
    M5.Display.println("失敗");
    return false;
  }
}

// getRSS
boolean getRSS()
{
  // Clear Display
  clearDisplay();
  // clear titles of news
  memset(&newsTitle, 0, sizeof(newsTitle));
  M5.Display.println("RSS取得");
  // WiFi connect check
  if (!(WiFi.status() == WL_CONNECTED))
  {
    ESP_LOGE(TAG, "WiFi not Connected!");
    M5.Display.println("WiFi未接続");
    return false;
  }
  // Start HTTP request
  // Set timeout
  client.setTimeout(HTTP_TIME_OUT);
  // Set yahoo rss root ca
  client.setCACert(YAHOO_RSS_ROOT_CA);
  // client.setInsecure();  //skip verification
  if (!client.connect("news.yahoo.co.jp", 443))
  {
    ESP_LOGE(TAG, "Connection failed!");
    client.stop();
    M5.Display.println("接続失敗");
    return false;
  }
  else
  {
    ESP_LOGI(TAG, "Connected to server!");
    // Make a HTTP request
    client.println("GET https://news.yahoo.co.jp/rss/topics/top-picks.xml HTTP/1.0");
    client.println("Host: news.yahoo.co.jp");
    client.println("Connection: close");
    client.println();
    // Reset XML
    xml.reset();
    ESP_LOGI(TAG, "Start get response header!");
    // header recieved Flg
    boolean headRecvFlg = false;
    // content length
    int conLen = 0;
    // Get response header
    while (client.connected())
    {
      char headerLine[512] = {0};
      size_t hrbSize = client.readBytesUntil('\n', headerLine, sizeof(headerLine));
      ESP_LOGV(TAG, "header: %s", headerLine);
      // timeout
      if (hrbSize == 0)
      {
        ESP_LOGE(TAG, "Connection timed out");
        client.stop();
        M5.Display.println("受信失敗(1)");
        return false;
      }
      // Get Content length
      char *startPos = strstr(headerLine, "Content-Length: ");
      // if 'Content-Length: ' is included in headerLine
      if (startPos != NULL)
      {
        // move pointer Next to 'Content-Length: '
        startPos += sizeof("Content-Length: ") - 1;
        // conten length
        char conLenStr[512] = {0};
        strcpy(conLenStr, startPos);
        // conLenStr's end '\r' to '\0'
        char *crStr = strstr(conLenStr, "\r");
        if (crStr != NULL)
        {
          *crStr = '\0';
        }
        // if conLenStr is enable to cast Number
        if (validateNumber(conLenStr))
        {
          conLen = atoi(conLenStr);
        }
        ESP_LOGV(TAG, "content-Length: %d", conLen);
      }
      // If get header End
      if (strcmp(headerLine, "\r") == 0)
      {
        ESP_LOGI(TAG, "headers received");
        headRecvFlg = true;
        break;
      }
    }
    // Header recieved check
    if (headRecvFlg == false || conLen == 0)
    {
      ESP_LOGE(TAG, "Failed to get header");
      client.stop();
      M5.Display.println("受信失敗(2)");
      return false;
    }
    ESP_LOGI(TAG, "Start get response body!");
    // Received body size
    int recvBodySize = 0;
    // Get response body
    while (client.connected())
    {
      if (recvBodySize >= conLen)
      {
        ESP_LOGV(TAG, "body has already recevied");
        break;
      }
      char bodyBuf[1] = {0};
      size_t hrbSize = client.readBytes(bodyBuf, sizeof(bodyBuf));
      if (hrbSize == 1)
      {
        // Plus readBytes to Received body size
        recvBodySize += hrbSize;
        xml.processChar(bodyBuf[0]);
      }
      else
      {
        ESP_LOGE(TAG, "Failed to get body");
        ESP_LOGI(TAG, "received body size: %d", recvBodySize);
        client.stop();
        M5.Display.println("受信失敗(3)");
        return false;
      }
    }
    ESP_LOGI(TAG, "received body size: %d", recvBodySize);
  }
  client.stop();
  return true;
}

// Call when TinyXML find TAG
void XML_callback(uint8_t statusflags, char *tagName, uint16_t tagNameLen, char *data, uint16_t dataLen)
{
  // When Find title tag.
  if ((statusflags & STATUS_TAG_TEXT) && !strcasecmp(tagName, "/rss/channel/item/title"))
  {
    ESP_LOGI(TAG, "title: %s", data);
    if (MAX_TITLE_COUNT > newsTitle.titleCount)
    {
      if (MAX_TITLE_LENGTH > dataLen)
      {
        strcpy(newsTitle.title[newsTitle.titleCount], data);
        newsTitle.titleCount++;
      }
    }
  }
}

// clearDisplay
void clearDisplay()
{
  // fill back ground color to BLACK
  M5.Display.fillScreen(BLACK);
  // reset cursor position
  M5.Display.setCursor(0, 0);
}

// validateNumber
bool validateNumber(const char *input)
{
  int length = strlen(input);
  for (int i = 0; i < length; i++)
  {
    if (!isdigit(input[i]))
    { // if include not Number
      return false;
    }
  }
  return true;
}

void setup()
{
  Serial.begin(115200);
  // Wait one second, For output log in setup functon
  delay(1000);
  // init news title
  memset(&newsTitle, 0, sizeof(newsTitle));
  // Get the startup time of the ATOMS3
  startTime = millis();
  // Config M5
  auto cfg = M5.config();
  // Init M5
  M5.begin(cfg);
  // Set font
  M5.Display.setFont(&fonts::lgfxJapanGothicP_28);
  // Connect to WiFi
  initFlg = initWiFi();
  // Failed to connect WiFi
  if (initFlg == false)
  {
    return;
  }
  // Show result of WiFi Connect for 2 seconds
  delay(2000);
  // XML parser init
  xml.init((uint8_t *)buffer, sizeof(buffer), &XML_callback);
  // getRSS
  rssRecvFlg = getRSS();
}

void loop()
{
  // if init failed
  if (initFlg == false)
  {
    return;
  }
  // calculate elapsed time
  unsigned long elapsedTime = millis() - startTime;
  // 30 miniutes * 60 seconds * 1000 millis
  if (elapsedTime >= 30 * 60 * 1000)
  { // check if 0.5 hour has elapsed
    // getRSS
    rssRecvFlg = getRSS();
    startTime = millis(); // update the start time for the next hour
  }
  // If news title exists
  if (rssRecvFlg == true && newsTitle.titleCount > 0)
  {
    // Reset titleIndex
    if (newsTitle.titleIndex > newsTitle.titleCount - 1)
    {
      newsTitle.titleIndex = 0;
    }
    // Clear Display
    clearDisplay();
    // Show news
    M5.Display.println(newsTitle.title[newsTitle.titleIndex]);
    ESP_LOGV(TAG, "Index:%d Title:%s", newsTitle.titleIndex, newsTitle.title[newsTitle.titleIndex]);
    // Count up titleIndex
    newsTitle.titleIndex++;
  }
  // 10 seconds wait
  delay(10 * 1000);
}