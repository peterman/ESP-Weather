#define ALTITUDE 220
#define STASSID "ASM-modul"
#define STAPSK  "dreebit001"
#define HOSTNAME "ESP8266-"

FS* filesystem = &LittleFS;



const char* ap_default_ssid = STASSID; ///< Default SSID.
const char* ap_default_psk = STAPSK; ///< Default PSK.
const float cToKOffset = 273.15;
float temperature, dewpoint, humidity, humidity_r, pressure, pressure_r;
unsigned long startTime;
int delayTime;


ESP8266WebServer server(80);
//holds the current upload
File fsUploadFile;
Adafruit_BME280 bme;
