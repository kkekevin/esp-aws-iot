#include <pgmspace.h>

#define THINGNAME "esp32-ctemp-hy" // replace with thing name
#define PHONE_NUMBER "" // replace with phone number

const char WIFI_SSID[] = ""; // replace with wifi ssid              
const char WIFI_PASSWORD[] = ""; // replace with wifi password

const char AWS_IOT_ENDPOINT[] = ""; // replace with iot endpoint

// Amazon Root CA 1 : 
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

// Device Certificate
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)KEY";

// Device Private key
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
)KEY";
