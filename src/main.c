
#include <Wire.h>
#include <axp20x.h>
#include <FT6236.h>
#include <SPI.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789

#include "ble.h"
#include <WiFi.h>
#include "keyval.h"
#include <queue>
#include "FS.h"
#include "SPIFFS.h"

#include <bigDig.h>
#include <font_8x8_rus.h>
#include <splash.h>

#define FS SPIFFS
#define FORMAT_SPIFFS_IF_FAILED true

#define TFT_CS 5  // define chip select pin
#define TFT_DC 27 // define data/command pin
#define TFT_RST -1
#define TFT_MOSI 19 // Data out
#define TFT_SCLK 18 // Clock out

const int VIBRO_PIN = 4;

#define TFT_MISO 0 //
#define TFT_CLK 18

#define WIDTH 240
#define HEIGHT 240

/*Color depth: 1 (I1), 8 (L8), 16 (RGB565), 24 (RGB888), 32 (XRGB8888)*/
#define LV_COLOR_DEPTH 16

#define ICON_HEIGHT 62
#define ICON_WIDTH 64
#define ICON_BITMAP_BUFFER_SIZE (ICON_HEIGHT * ICON_WIDTH / 8)
#define ICON_RENDER_BUFFER_SIZE (ICON_BITMAP_BUFFER_SIZE * LV_COLOR_DEPTH)
uint8_t receivedIconBitmapBuffer[ICON_BITMAP_BUFFER_SIZE];
uint8_t iconBitmapBuffer[ICON_BITMAP_BUFFER_SIZE];
uint8_t iconRenderBuffer[ICON_RENDER_BUFFER_SIZE];

const uint8_t i2c_sda = 21;
const uint8_t i2c_scl = 22;

const char *ssid = "RestfrontMI";
const char *password = "Rf7531711";

#define TWATCH_DAC_IIS_BCK 26
#define TWATCH_DAC_IIS_WS 25
#define TWATCH_DAC_IIS_DOUT 33
#define I2S_MCLKPIN 0

uint8_t data[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t datar[1] = {0x04};
uint8_t dataw[1] = {0x17};
uint8_t datam[1] = {0x03};
byte trol = 0;
byte touch = 0;
int val = 0;
String streamsong = "                                                                                 ";
#define MAX_AANTAL_KANALEN 75
int array_index = MAX_AANTAL_KANALEN - 1;
char zenderarray[MAX_AANTAL_KANALEN][60];
char urlarray[MAX_AANTAL_KANALEN][120];
int station = 1;
char url[120];
char zendernaam[60];
bool webradio = false;
int volume_keuze = 5;
float offset = 0;
bool info = true;

typedef struct
{
  uint8_t blue;
  uint8_t green;
  uint8_t red;
} lv_color_t;

bool connectionChanged = true;

// Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

SPIClass tftVSPI = SPIClass(SPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&tftVSPI, TFT_CS, TFT_DC, TFT_RST);

AXP20X_Class axp;

FT6236 ts = FT6236();

String Speed;
String nextRd;
String nextRdDesc;
String distToNext;
String totalDist;
String eta;
String ete;
String OldIconHash;
String iconHash;
String speed;

String receivedIconHash = String();
String displayIconHash = String();
bool iconDirty = false;

namespace Pref
{
  bool lightTheme = false;
  int brightness = 40;
  int speedLimit = 60;
} // namespace Pref

std::queue<String> navigationQueue{};
std::vector<String> availableIcons{};

uint16_t writeRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len)
{
  Wire1.beginTransmission(address);
  Wire1.write(reg);
  Wire1.write(data, len);
  return (0 != Wire1.endTransmission());
}

uint16_t readRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len)
{
  Wire1.beginTransmission(address);
  Wire1.write(reg);
  Wire1.endTransmission();
  Wire1.requestFrom((uint8_t)address, (uint8_t)len);
  uint8_t i = 0;
  while (Wire1.available())
  {
    data[i++] = (byte)Wire1.read();
  }
  return 0; // Pass
}

void enableLDO3(bool en = true)
{
  axp.setLDO3Mode(AXP202_LDO3_MODE_DCIN);
  axp.setPowerOutPut(AXP202_LDO3, en);
}

void drawButtons()
{
  tft.fillScreen(ST77XX_BLUE); // синий фон

  tft.drawRect(10, 10, 60, 40, ST77XX_WHITE); // кнопка "prev"
  tft.setCursor(25, 25);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.print("next");

  // По аналогии нарисуем остальные кнопки
}

void checkTouch()
{
  TS_Point p = ts.getPoint();

  if (ts.touched())
  {
    Serial.print("x ");
    Serial.print(p.x);
    Serial.print(" y ");
    Serial.println(p.y);
    if (p.x > 10 && p.x < 70 && p.y > 10 && p.y < 50)
    { // если нажата кнопка "prev"

      // выполнить действие для кнопки "prev"
      // webradio = true;
      // station++;
      // if (station>21) {station=21;}
    }
    // аналогично для остальных кнопок
  }
}

/* Recode russian fonts from UTF-8 to Windows-1251 */
String utf8rus(String source)
{
  int i, k;
  String target;
  unsigned char n;
  char m[2] = {'0', '\0'};

  k = source.length();
  i = 0;

  while (i < k)
  {
    n = source[i];
    i++;

    if (n >= 0xC0)
    {
      switch (n)
      {
      case 0xD0:
      {
        n = source[i];
        i++;
        if (n == 0x81)
        {
          n = 0xA8;
          break;
        }
        if (n >= 0x90 && n <= 0xBF)
          n = n + 0x30;
        break;
      }
      case 0xD1:
      {
        n = source[i];
        i++;
        if (n == 0x91)
        {
          n = 0xB8;
          break;
        }
        if (n >= 0x80 && n <= 0x8F)
          n = n + 0x70;
        break;
      }
      }
    }
    m[0] = n;
    target = target + String(m);
  }
  return target;
}

void displayIcon(int16_t x, int16_t y)
{
  // Проверяем, есть ли данные для отображения
  if (!iconRenderBuffer)
  {
    Serial.println("No icon data to display");
    return;
  }

  // Буфер iconRenderBuffer содержит данные в формате RGB565 (16 бит на пиксель)
  // Размер буфера: ICON_WIDTH * ICON_HEIGHT * 2 байта (так как 16 бит = 2 байта)

  uint16_t *iconData = (uint16_t *)iconRenderBuffer;

  // Рисуем иконку попиксельно
  for (uint16_t j = 0; j < ICON_HEIGHT; j++)
  {
    for (uint16_t i = 0; i < ICON_WIDTH; i++)
    {
      // Получаем цвет пикселя из буфера
      uint16_t color = iconData[j * ICON_WIDTH + i];

      // Рисуем пиксель на экране
      tft.drawPixel(x + i, y + j, color);
    }
  }

  // Serial.println("Icon displayed at position: " + String(x) + ", " + String(y));
}

void showmsgXY(int x, int y, int sz, const GFXfont *f, const char *msg)
{
  int16_t x1, y1;
  uint16_t wid, ht;
  tft.setFont(f);
  tft.setCursor(x, y);
  tft.setTextSize(sz);
  tft.print(msg);
}

void draw_screen()
{

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(10);

  // tft.setCursor(90, 20);
  // tft.println(Speed); //скорость

  showmsgXY(100, 100, 2, &FreeSevenSegNumFont, Speed.c_str());
  

  // tft.setCursor(90, 95);
  // tft.println(distToNext);

  tft.setFont(NULL);
  tft.setTextSize(2);
  tft.setCursor(5, 120);
  nextRd.replace("\xC2\xAD", "");
  tft.println(utf8rus(nextRd)); // название дороги маршрута

  // showmsgXY(5, 120, 2, NULL, utf8rus(nextRd).c_str());

  tft.setTextSize(4);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(15, 80);
  String cleandistToNext = distToNext.substring(0, distToNext.indexOf(" "));
  tft.println(utf8rus(cleandistToNext)); // расстояние до поворота

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE); 
  tft.setCursor(5, 180); // Сколько ехать растояние
  tft.println(utf8rus(totalDist));

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(5, 210);
  tft.println(utf8rus(ete)); // Сколько ехать время

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(165, 210);
  String etaWithoutPrefix = eta.substring(eta.indexOf(":") + 2);
  tft.println(utf8rus(etaWithoutPrefix)); // Время прибытия
  

  displayIcon(10, 10);
}
size_t readFile(const String &filename, uint8_t *buffer, const size_t bufferSize)
{
  Serial.println("Reading file: " + filename);
  File file = FS.open(filename, FILE_READ);

  if (!file && !file.isDirectory())
  {
    Serial.println("Failed to open file for reading");
    return 0;
  }

  if (file.size() > bufferSize)
  {
    Serial.println("Error: Buffer overflow");
    return 0;
  }

  size_t length = file.read(buffer, bufferSize);
  file.close();

  return length;
}

void writeFile(const String &filename, const uint8_t *buffer, const size_t &length)
{
  Serial.println("Writing file: " + filename + " size: " + length);
  File file = FS.open(filename, FILE_WRITE);

  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.write(buffer, length);
  file.close();
}

bool isIconExisted(const String &iconHash)
{
  return std::find(availableIcons.begin(), availableIcons.end(), iconHash) !=
         availableIcons.end();
}

void convert1BitBitmapToRgb565(void *dst, const void *src, uint16_t width, uint16_t height, uint16_t color, uint16_t bgColor, bool invert = false)
{
  uint16_t *d = (uint16_t *)dst;
  const uint8_t *s = (const uint8_t *)src;

  auto activeColor = invert ? bgColor : color;
  auto inactiveColor = invert ? color : bgColor;

  for (uint16_t y = 0; y < height; y++)
  {
    for (uint16_t x = 0; x < width; x++)
    {
      if (s[(y * width + x) / 8] & (1 << (7 - x % 8)))
      {
        d[y * width + x] = activeColor;
      }
      else
      {
        d[y * width + x] = inactiveColor;
      }
    }
  }
}

uint16_t lv_color_to_u16(lv_color_t color)
{
  return ((color.red & 0xF8) << 8) + ((color.green & 0xFC) << 3) + ((color.blue & 0xF8) >> 3);
}

lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b)
{
  lv_color_t ret;
  ret.red = r;
  ret.green = g;
  ret.blue = b;
  return ret;
}

void setIconBuffer(const uint8_t *value, const size_t &length)
{
  // Blank icon
  if (!value || length == 0)
  {
    memset(iconRenderBuffer, 0xFF, sizeof(iconRenderBuffer));
    iconDirty = true;
    return;
  }

  // Render icon
  if (length > sizeof(iconRenderBuffer) / LV_COLOR_DEPTH)
  {
    Serial.println("Error: Icon buffer overflow");
  }
  else
  {
    Serial.println("Drawing icon");
    convert1BitBitmapToRgb565(iconRenderBuffer, value, 64, 64, lv_color_to_u16(lv_color_make(0x00, 0x00, 0xFF)),
                              lv_color_to_u16(lv_color_make(0xFF, 0xFF, 0xFF)));
    iconDirty = true;
  }
}

void loadIcon(const String &iconHash)
{
  if (!isIconExisted(iconHash))
  {
    Serial.println("Icon not found");
    return;
  }

  readFile(String("/") + iconHash + ".bin", iconBitmapBuffer, ICON_BITMAP_BUFFER_SIZE);
  setIconBuffer(iconBitmapBuffer, ICON_BITMAP_BUFFER_SIZE);
}

void setIconHash(const String &value)
{
  if (value == displayIconHash)
    return;

  displayIconHash = value;

  Serial.println("Icon hash changed: " + value);

  if (value.isEmpty())
  {
    setIconBuffer(nullptr, 0);
    return;
  }

  if (isIconExisted(value))
  {
    Serial.println("Icon already existed, now display");
    loadIcon(value);
    return;
  }

  // Serial.println("Requesting icon");
  // Request icon
  // notifyCharacteristic(CHA_NAV_TBT_ICON, (uint8_t*)value.c_str(), value.length());
}

void saveIcon(const String &iconHash, const uint8_t *buffer)
{
  if (isIconExisted(iconHash))
  {
    Serial.println("Icon existed");
    return;
  }

  writeFile(String("/") + iconHash + ".bin", buffer, ICON_BITMAP_BUFFER_SIZE);
  availableIcons.push_back(iconHash);

  Serial.println(String("Icon saved: ") + iconHash + ", total: " + availableIcons.size());
}

void removeAllFiles()
{
  File root = FS.open("/");
  File file = root.openNextFile();

  while (file)
  {
    Serial.print("Removing file: ");
    Serial.println(file.path());
    FS.remove(file.path());
    file = root.openNextFile();
  }
}

void receiveNewIcon(const String &iconHash, const uint8_t *buffer)
{
  if (iconHash == receivedIconHash)
  {
    Serial.println("Icon already received");
    return;
  }

  receivedIconHash = iconHash;
  memcpy(receivedIconBitmapBuffer, buffer, ICON_BITMAP_BUFFER_SIZE);
}

void onCharacteristicWrite(const String &uuid, uint8_t *data, size_t length)
{
  String value = (uuid != CHA_NAV_TBT_ICON) ? String((char *)(data)) : String();

  if (uuid == CHA_SETTINGS)
  {

    Serial.print("Settings: ");
    Serial.println(value);
    const auto kv = kvParseMultiline(value);

    Pref::lightTheme = kv.getOrDefault("lightTheme", "false") == "true";
    Pref::brightness = kv.getOrDefault("brightness", "100").toInt();
    Pref::speedLimit = kv.getOrDefault("speedLimit", "60").toInt();

    //	tft.setBrightness(Pref::brightness);
    //!	Pref::lightTheme ? ThemeControl::light() : ThemeControl::dark();

    if (kv.contains("removeAllFiles"))
    {
      removeAllFiles();
    }
  }

  if (uuid == CHA_NAV)
  {
    Serial.println("Navigation CHA_NAV: " + value);
    navigationQueue.push(value);
    // pongNavigation();
  }

  if (uuid == CHA_NAV_TBT_ICON)
  {

    Serial.println("!!!!!   Navigation CHA_NAV_TBT_ICON: " + value);

    int semicolonIndex = -1;
    for (uint8_t i = 0; i < 16; i++)
    {
      if (data[i] == ';')
      {
        semicolonIndex = i;
        break;
      }
    }

    if (semicolonIndex <= 0)
    {
      Serial.println("No hash found");
      return;
    }

    String iconHash = String((char *)data, semicolonIndex);
    auto iconSize = length - semicolonIndex - 1;
    Serial.println(String("Received icon w/ hash: ") + iconHash + " size: " + iconSize);

    if (iconSize != ICON_BITMAP_BUFFER_SIZE)
    {
      Serial.println("Invalid icon size");
      return;
    }

    receiveNewIcon(iconHash, data + semicolonIndex + 1);

    //!	pongNavigation();
  }

  if (uuid == CHA_GPS_SPEED)
  {
    Serial.println("Navigation GPS_SPEED: " + value);
    navigationQueue.push(String("speed=") + value);

    Speed = value;
    draw_screen();

    //!	pongSpeed();
  }
}

void onConnectionChange(bool connected)
{
  connectionChanged = true;
}

void listFiles()
{
  Serial.println("Listing files");
  File root = FS.open("/");
  File file = root.openNextFile();

  availableIcons.clear();

  while (file)
  {
    String name = file.name();
    String hash = name.substring(0, name.length() - 4);
    Serial.print("File: ");
    Serial.print(name);
    Serial.print(" Hash: ");
    Serial.print(hash);
    Serial.print(" Size: ");
    Serial.println(file.size());
    // Remove extension
    availableIcons.push_back(hash);
    file = root.openNextFile();
  }
}

void createTestIcon()
{
  // Создаем тестовую иконку (градиент или узор)
  uint16_t *iconData = (uint16_t *)iconRenderBuffer;

  for (uint16_t y = 0; y < ICON_HEIGHT; y++)
  {
    for (uint16_t x = 0; x < ICON_WIDTH; x++)
    {
      if ((x + y) % 2 == 0)
      {
        // Красный цвет в формате RGB565
        iconData[y * ICON_WIDTH + x] = 0xF800; // Красный
      }
      else
      {
        // Синий цвет
        iconData[y * ICON_WIDTH + x] = 0x001F; // Синий
      }
    }
  }

  iconDirty = true;
  Serial.println("Test icon created");
}

void setup()
{
  tftVSPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI, TFT_RST);
  Serial.begin(115200);
  ts.begin(2, 23, 32);
  Wire1.begin(i2c_sda, i2c_scl);
  int ret = axp.begin(Wire1);
  ret = axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);
  //----
  axp.enableIRQ(AXP202_ALL_IRQ, AXP202_OFF);
  axp.adc1Enable(0xFF, AXP202_OFF);
  axp.adc2Enable(0xFF, AXP202_OFF);
  axp.adc1Enable(AXP202_BATT_VOL_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1, AXP202_ON);
  axp.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ, AXP202_ON);
  axp.clearIRQ();

  enableLDO3(true);
  //----

  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH); // screen ON

  tft.init(240, 240, SPI_MODE2);

  if (!FS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    Serial.println("Error mounting SPIFFS");
    return;
  }

  listFiles();

  initBle();

  tft.cp437(true);
  tft.fillScreen(ST77XX_BLUE);
  tft.setTextColor(ST77XX_WHITE);
  //tft.setTextSize(2);
  //  tft.setCursor(30, 100);
  //  tft.println("Naffigator v1.0");

  // showmsgXY(5, 180, 1, &FreeSevenSegNumFont, "01234");
  showmsgXY(30, 100, 1, NULL, "Naffigator v1.0");

  createTestIcon();
  pinMode(VIBRO_PIN, OUTPUT);
  // Убеждаемся, что мотор выключен при старте
  digitalWrite(VIBRO_PIN, HIGH);
  delay(100);
  //tft.fillScreen(ST77XX_BLACK);
  digitalWrite(VIBRO_PIN, LOW);
}

// tft.drawPixel(rotxxx, rotyyy, Rcolor);
// tft.drawLine(wireframe[0][0], wireframe[0][1], wireframe[1][0], wireframe[1][1], Rcolor);

uint8_t b = 0;

void processQueue()
{
  if (navigationQueue.empty())
    return;

  const auto &data = navigationQueue.front();
  const auto kv = kvParseMultiline(data);

  if (kv.contains("nextRd"))
  {
    nextRd = kv.getOrDefault("nextRd").c_str();
  }
  if (kv.contains("nextRdDesc"))
  {
    nextRdDesc = kv.getOrDefault("nextRdDesc");
  }
  if (kv.contains("distToNext"))
  {
    distToNext = kv.getOrDefault("distToNext");
  }
  if (kv.contains("totalDist"))
  {
    totalDist = kv.getOrDefault("totalDist");
  }
  if (kv.contains("eta"))
  {
    eta = kv.getOrDefault("eta");
  }
  if (kv.contains("ete"))
  {
    ete = kv.getOrDefault("ete");
  }
  if (kv.contains("iconHash"))
  {
    if (!(OldIconHash == iconHash))
    {
      OldIconHash = iconHash;
      digitalWrite(VIBRO_PIN, HIGH);
      delay(120);
      digitalWrite(VIBRO_PIN, LOW);
    }
    iconHash = kv.getOrDefault("iconHash");
    setIconHash(iconHash);
  }
  if (kv.contains("speed"))
  {
    speed = kv.getOrDefault("speed");
  }

  navigationQueue.pop();
}

void UI_update()
{
  if (iconDirty)
  {
    iconDirty = false;
  }
}

void Data_update()
{
  if (receivedIconHash.isEmpty())
    return;

  // Save icon for later use
  if (!isIconExisted(receivedIconHash))
  {
    saveIcon(receivedIconHash, receivedIconBitmapBuffer);
  }

  // Display icon
  if (receivedIconHash == displayIconHash)
  {
    setIconBuffer(receivedIconBitmapBuffer, ICON_BITMAP_BUFFER_SIZE);
  }

  receivedIconHash = String();
}

void clearNavigationData()
{
  nextRd = String();
  nextRdDesc = String();
  eta = String();
  ete = String();
  distToNext = String();
  totalDist = String();
  iconHash = String();
  receivedIconHash = String();
  speed = String();
}

void displaySplash(int16_t x, int16_t y)
{
  // Рисуем иконку попиксельно
  for (uint16_t j = 0; j < 64; j++)
  {
    for (uint16_t i = 0; i < 64; i++)
    {
      // Получаем цвет пикселя из буфера
      uint16_t color = splash[j * ICON_WIDTH + i];

      // Рисуем пиксель на экране
      tft.drawPixel(x + i, y + j, color);
    }
  }
}

void loop()
{
  UI_update();
  Data_update();
  processQueue();
  delay(20);

  // Connection status
  if (connectionChanged)
  {
    connectionChanged = false;

    if (!deviceConnected)
    {
      tft.fillScreen(ST77XX_BLACK);
      showmsgXY(22, 75, 1, &Dialog_plain_24, "Naffigator v1.0");
      displaySplash(88, 108);

      navigationQueue = std::queue<String>();
      clearNavigationData();
    } else
    {
      tft.fillScreen(ST77XX_BLACK);
    }
  }
}
