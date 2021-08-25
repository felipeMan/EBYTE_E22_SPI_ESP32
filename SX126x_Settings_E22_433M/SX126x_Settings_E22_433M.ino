/*
   RadioLib SX126x Settings Example

   This example shows how to change all the properties of LoRa transmission.
   RadioLib currently supports the following settings:
    - pins (SPI slave select, DIO1, DIO2, BUSY pin)
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word
    - output power during transmission
    - CRC
    - preamble length
    - TCXO voltage
    - DIO2 RF switch control

   Other modules from SX126x family can also be used.

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

#define ESP32
//#define TX_EN
#define RX_EN

#define OLED

#ifdef TX_EN
#undef OLED
#endif

#ifdef OLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
//
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

void init_oled(void);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
unsigned long ping_counter = 0;
#endif

#ifdef ESP32
#pragma config board="ESP32 Wrover Module"
#pragma config board="ESP32 Wrover Module"
//#pragma config port="4"
#endif

#ifdef TX_EN
void tx_routine (void);
void init_tx    (void);
#endif

#ifdef RX_EN
void rx_routine (void);
void init_rx    (void);
// flag to indicate that a packet was received
volatile bool receivedFlag = false;
// disable interrupt when it's not needed
volatile bool enableInterrupt = true;
void setFlag(void);
#endif

#ifdef ESP32
#pragma config board="ESP32 Wrover Module"
#define NSS_PIN  5
#define DIO1_PIN 34
#define DIO2_PIN 39 // not connected
#define BUSY_PIN 32
#define SCLK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23
// #define NSS_PIN   5
#define RST_PIN  12
// GPIOs to control E22 RX_EN and TX_EN
#define RADIO_RXEN 27
#define RADIO_TXEN 26
#define FREQ   433.5
#endif

// SX1262 has the following connections:
SX1262 radio = new Module(NSS_PIN, DIO1_PIN, DIO2_PIN, BUSY_PIN);


#ifdef OLED
void init_oled(void) {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  }
  //
  display.clearDisplay();
  //
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.printf("LORAv02\n");
  display.display();
}
#endif

//#ifdef TX_EN
void init_tx(void) {
  if (radio.setFrequency(433.5) == ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true);
  }
  // set bandwidth to 250 kHz
  if (radio.setBandwidth(250.0) == ERR_INVALID_BANDWIDTH) {
    Serial.println(F("Selected bandwidth is invalid for this module!"));
    while (true);
  }
  // set spreading factor to 10
  if (radio.setSpreadingFactor(10) == ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(F("Selected spreading factor is invalid for this module!"));
    while (true);
  }
  // set coding rate to 6
  if (radio.setCodingRate(6) == ERR_INVALID_CODING_RATE) {
    Serial.println(F("Selected coding rate is invalid for this module!"));
    while (true);
  }
  // set LoRa sync word to 0xAB
  if (radio.setSyncWord(0xAB) != ERR_NONE) {
    Serial.println(F("Unable to set sync word!"));
    while (true);
  }
  // set output power to 10 dBm (accepted range is -17 - 22 dBm)
  if (radio.setOutputPower(22) == ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("Selected output power is invalid for this module!"));
    while (true);
  }
  // set over current protection limit to 80 mA (accepted range is 45 - 240 mA)
  // NOTE: set value to 0 to disable overcurrent protection
  if (radio.setCurrentLimit(80) == ERR_INVALID_CURRENT_LIMIT) {
    Serial.println(F("Selected current limit is invalid for this module!"));
    while (true);
  }
  // set LoRa preamble length to 15 symbols (accepted range is 0 - 65535)
  if (radio.setPreambleLength(15) == ERR_INVALID_PREAMBLE_LENGTH) {
    Serial.println(F("Selected preamble length is invalid for this module!"));
    while (true);
  }
  // disable CRC
  if (radio.setCRC(false) == ERR_INVALID_CRC_CONFIGURATION) {
    Serial.println(F("Selected CRC is invalid for this module!"));
    while (true);
  }
  // Some SX126x modules have TCXO (temperature compensated crystal
  // oscillator). To configure TCXO reference voltage,
  // the following method can be used.
  if (radio.setTCXO(2.4) == ERR_INVALID_TCXO_VOLTAGE) {
    Serial.println(F("Selected TCXO voltage is invalid for this module!"));
    while (true);
  }
  // Some SX126x modules use DIO2 as RF switch. To enable
  // this feature, the following method can be used.
  // NOTE: As long as DIO2 is configured to control RF switch,
  //       it can't be used as interrupt pin!
  if (radio.setDio2AsRfSwitch() != ERR_NONE) {
    Serial.println(F("Failed to set DIO2 as RF switch!"));
    while (true);
  }
  Serial.println(F("All settings succesfully changed!"));
}
//#endif

#ifdef RX_EN
void init_rx(void) {
  // set the function that will be called
  // when new packet is received
  radio.setDio1Action(setFlag);

  init_tx();

  // start listening for LoRa packets
  Serial.print(F("[SX1262] Starting to listen ... "));
  int state = radio.startReceive();
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // check if the interrupt is enabled
  if (!enableInterrupt) {
    return;
  }

  // we got a packet, set the flag
  receivedFlag = true;
}
#endif

void setup() {
  Serial.begin(115200);

#ifdef ESP32
  SPI.begin(SCLK_PIN, MISO_PIN, MOSI_PIN, NSS_PIN);
#endif
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);
  delay(100);
  digitalWrite(RST_PIN, HIGH);
  // initialize SX1268 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin();
  //
  if (state == ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
  Serial.print(F("[SX1268] Initializing ... "));
  // carrier frequency:           915.0 MHz
  // bandwidth:                   500.0 kHz
  // spreading factor:            6
  // coding rate:                 5
  // sync word:                   0x34 (public network/LoRaWAN)
  // output power:                2 dBm
  // preamble length:             20 symbols
  // you can also change the settings at runtime
  // and check if the configuration was changed successfully
  // set carrier frequency to 433.5 MHz
  init_oled();

#ifdef TX_EN
  init_tx();
#endif

#ifdef RX_EN
  init_rx();
#endif
}

#ifdef RX_EN
void rx_routine (void) {
  if (receivedFlag) {
    // disable the interrupt service routine while
    // processing the data
    enableInterrupt = false;

    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    String str;
    int state = radio.readData(str);

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int state = radio.readData(byteArr, 8);
    */

    if (state == ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[SX1262] Received packet!"));

      // print data of the packet
      Serial.print(F("[SX1262] Data:\t\t"));
      Serial.println(str);

      // print RSSI (Received Signal Strength Indicator)
      Serial.print(F("[SX1262] RSSI:\t\t"));
      String RSSI_str = String(radio.getRSSI());
      Serial.print(RSSI_str);
      Serial.println(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      Serial.print(F("[SX1262] SNR:\t\t"));
      String SNR_str = String(radio.getSNR());
      Serial.print(SNR_str);
      Serial.println(F(" dB"));

#ifdef OLED
      display.clearDisplay();
      display.setCursor(0, 10);
      // Display static text
      display.printf("Rssi=%s dBm\n", RSSI_str);
      display.setCursor(0, 20);
      display.printf("SnrValue=%s\n", SNR_str);
      display.setCursor(0, 30);
      display.printf("Cnt=%s\n", str);
      display.display();
#endif


    } else if (state == ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);

    }

    // put module back to listen mode
    radio.startReceive();

    // we're ready to receive more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }
}
#endif

#ifdef TX_EN
int num = 0;
char cstr[16];

void tx_routine (void) {
  Serial.print(F("[SX1262] Transmitting packet ... "));
  // you can transmit C-string or Arduino string up to
  // 256 characters long
  // NOTE: transmit() is a blocking method!
  //       See example SX126x_Transmit_Interrupt for details
  //       on non-blocking transmission method.
  num = num + 1;
  itoa(num, cstr, 10);
  int state = radio.transmit(cstr);
  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD, 0xEF};
    int state = radio.transmit(byteArr, 8);
  */
  if (state == ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(F("success!"));
    // print measured data rate
    Serial.print(F("[SX1262] Datarate:\t"));
    Serial.print(radio.getDataRate());
    Serial.println(F(" bps"));
  } else if (state == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));
  } else if (state == ERR_TX_TIMEOUT) {
    // timeout occured while transmitting packet
    Serial.println(F("timeout!"));
  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
  // wait for a second before transmitting again
  delay(1000);
  if (num == 999) {
    num = 0;
  }

}
#endif
//
void loop() {
#ifdef TX_EN
  tx_routine();
#endif
#ifdef RX_EN
  rx_routine();
#endif
}
