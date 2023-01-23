/*********
  Originally based on https://RandomNerdTutorials.com
  Heavily modified by Ted Dunning
*********/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>

// Display bottom right
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// 4 = GPIO2, 9 = GPIO 3, 2 = GPIO4
const int oneWireBus = 2;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// Current temperatures
float* temps = new float[4];
// history of temperatures
float** history = new float*[4];
#define HIST_SIZE 56
#define LINE_WIDTH 4

void setup() {
  for (int sensor = 0; sensor < 4; sensor++) {
    history[sensor] = new float[HIST_SIZE];
    for (int t = 0; t < HIST_SIZE; t++) {
      history[sensor][t] = -1000.0;
    }
  }
  u8g2.begin();
  // Start the Serial Monitor
  Serial.begin(115200);
  // Start the DS18B20 sensor
  sensors.begin();
}

void loop() {
  // read from the temp sensors
  sensors.requestTemperatures();
  for (int sensor = 0; sensor < 4; sensor++) {
    temps[sensor] = sensors.getTempCByIndex(sensor);
    Serial.printf("%d: t = %.1f\n", sensor, temps[sensor]);
  }

  // display current data in text form or a sparkline of recent history
  Serial.println("display");
  updateOLED(temps);
  delay(2000);
  Serial.println("graph");
  updateSpark(temps);
  delay(1500);
}

/**
 * Writes an array of temperatures to the display
 */
void updateOLED(float* tx) {
  char buf[10];
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_fub20_tr);
    for (int row = 0; row < 4; row++) {
      snprintf(buf, 10, "%.1f", tx[row]);
      int x = (row % 2) * 64;
      int y = ((row / 2) + 1) * 27;
      u8g2.drawStr(x, y, buf);
    }
  } while ( u8g2.nextPage() );
}

// this is the array of pixels we use to stage the spark lines
// good for a HIST_SIZE x 32 bitmap
static uint8_t* bits = new uint8_t[HIST_SIZE / 8 * 32];

/**
 * Draw recent temperatures as four sparklines
 */
void updateSpark(float* tx) {
  u8g2.firstPage();

  // shift in new data while computing summaries of the
  // historical data
  int n = 1;
  float minT = tx[0];
  float maxT = tx[0];
  float mean = tx[0];
  for (int sensor = 0; sensor < 4; sensor++) {
    for (int t = 0; t < HIST_SIZE - 1; t++) {
      // shift old data
      float z = history[sensor][t + 1];
      history[sensor][t] = z;
      // -1000 is the marker for missing data
      if (z != -1000.0) {
        // accumulate summaries
        n++;
        if (z < minT) {
          minT = z;
        }
        if (z > maxT) {
          maxT = z;
        }
        mean += z;
      }
    }
    // add in latest measurement
    history[sensor][HIST_SIZE - 1] = tx[sensor];
  }

  // adjust for the fact that we looked at four sensors
  mean = mean / n;
  n = n / 4;
  // make sure that there is at least some range in the plot
  if (minT > mean - 1.0) {
    minT = mean - 1.0;
  }
  if (maxT < mean + 1.0) {
    maxT = mean + 1.0;
  }
  Serial.printf("%d %.1f %.1f %.1f\n", n, minT, maxT, mean);

  // for each sensor, stage the plot and then display it
  for (int sensor = 0; sensor < 4; sensor++) {
    // first clear old plot
    for (int i = 0; i < HIST_SIZE / 8 * 32; i++) {
      bits[i] = 0;
    }
    // then draw history for current sensor
    for (int t = 0; t < HIST_SIZE; t++) {
      // draw limit marks every 8 samples
      if (t % 8 == 0) {
        draw(t, minT, maxT, mean, minT, 1);
        draw(t, minT, maxT, mean, maxT, 1);
      }
      if (history[sensor][t] == -1000.0) {
        continue;
      }
      // draw the actual data with a wide line
      draw(t, minT, maxT, mean, history[sensor][t], 4);
    }
    // finally, copy the current plot into the right place on the display
    int x = (sensor % 2) * 64;
    int y = (sensor / 2) * 32 + 2;
    u8g2.drawXBM(x, y, HIST_SIZE, 32, bits);
  }
  u8g2.nextPage();
}

/**
 * Converts a single value into marks in the bitmap
 * after scaling for the data range and the display
 * details
 */
void draw(int t, float minT, float maxT, float mean, float val, int dy) {
  int ix = t / 8;
  int mask = 1 << (t % 8);
  int y = floor(25.0 * (1.0 - (val - minT) / (maxT - minT)));
  int down = dy/2;
  int up = (dy + 1)/2;
  for (int w = -down; w < up; w++) {
    int iy = max(0, y + w) * HIST_SIZE / 8;
    bits[ix + iy] |= mask;
  }
}
