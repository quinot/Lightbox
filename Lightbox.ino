#include <Adafruit_SSD1306.h>
#include <Encoder.h>

// OLED screen

#define OLED_I2C 0x3c
#define OLED_W 128
#define OLED_H 64
#define OLED_RST -1
Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, OLED_RST);

// Rotary encoder, with switch

Encoder rotary(2, 3);
int rot_state = -1;

#define SW_PIN 4
byte sw_state = 1; // Normally open contact (pullup)

// LED strip switch

#define LED_MODE_PIN 5
byte uv_state = 0;

// Front panel (AMP CT connector, CT.1 = GND)

#define BUT_1_PIN 6       // CT.6 Scan
#define BUT_2_PIN 7       // CT.5 Print
#define BUT_3_PIN 8       // CT.4 Brush

#define LED_RED_PIN   11  // CT.3
#define LED_GREEN_PIN 12  // CT.2

// PWM output

#define PWM_PIN 10
byte pwm_state = 0;

#define RAMPUP 4000


void oled_setup() {
  oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C);

  oled.setTextColor(WHITE, BLACK);

  oled.clearDisplay();

#if 0
  // Test screen
  for (byte screen=0; screen < 4; screen++) {
    for (byte l=0 ; l < OLED_H ; l++) {
      for (byte c=0 ; c < OLED_W ; c++) {
        byte color;
        switch (screen) {
          case 0: color = WHITE; break;
          case 1: 
          case 2:
            color = (((screen & 1) ? l : c) & 1) ? WHITE : BLACK; break;
          case 3 : color = BLACK; break;
        }
        oled.drawPixel(c, l, color);
      }
    }
    oled.display();
    if (screen < 3)
      delay(300);
  }
#else
  oled.setCursor(0, 40);
  oled.setTextSize(2);
  oled.print("LightBox");
  oled.display();
#endif
}

void oled_show_state(void) {
    oled.clearDisplay();

    oled.setCursor(0, 0);
    oled.setTextSize(2);
    oled.print("PWM ");
    oled.print(pwm_state ? "ON  " : "OFF ");

    if (uv_state) {
      oled.setTextColor(BLACK, WHITE);
      oled.print("UV");
      oled.setTextColor(WHITE, BLACK);
    }

    oled.setCursor(0, 40);
    oled.setTextSize(3);
    oled.print(rot_state);
    oled.print("%");

    oled.display();
}

void setup() {
  // Set up OLED display

  oled_setup();

  // Set up pin modes

  for (byte pin = 2; pin <= 8; pin++)
    pinMode(pin, INPUT_PULLUP);

  for (byte pin = 10; pin <= 13; pin++) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
  }

  // Fade up LED strip for test

  for (int j = 0; j < 256; j++) {
    analogWrite(PWM_PIN, j);
    delay(RAMPUP / 256);
  }
  analogWrite(PWM_PIN, 0);
 
  // Initialize intensity at 50%

  rotary.write(50 * 4);
};

void loop() {
  // Front panel of the scanner (buttons and bicolor LED, just for show)

  byte scan_state  = !digitalRead(BUT_1_PIN);
  byte print_state = !digitalRead(BUT_2_PIN);
  byte brush_state = !digitalRead(BUT_3_PIN);

  // Scan  = Green
  // Brush = Orange
  // Print = Red

  // Red is too intense compared to green, so only turn it on at 50% PWM

  // digitalWrite(LED_RED_PIN, brush_state || print_state);
  analogWrite(LED_RED_PIN, (brush_state || print_state) ? 128 : 0);

  digitalWrite(LED_GREEN_PIN, brush_state || scan_state);

  // LED strip on/off switch (bistable from the rotary encoder switch)

  byte sw = digitalRead(SW_PIN);
  byte pwm_change = (!sw && sw_state);
  pwm_state ^= pwm_change;
  sw_state = sw;
  digitalWrite(LED_BUILTIN, pwm_state);

  // Intensity setting: read rotary encoder and clamp it to 0..100

  int rot = rotary.read() / 4;
  // Encoder library reads 4 edges per click

  int clamped_rot = (rot < 0) ? 0 : ((rot > 100) ? 100 : rot);
  if (clamped_rot != rot)
    rotary.write(clamped_rot * 4);

  // UV mode sense

  byte uv = !digitalRead(LED_MODE_PIN);
  byte uv_change = (uv != uv_state);
  uv_state = uv;

  // Update outputs

  if (pwm_change || uv_change || clamped_rot != rot_state) {
    rot_state = clamped_rot;
    analogWrite(PWM_PIN, (pwm_state ? map(rot_state, 0, 100, 0, 255) : 0));
    oled_show_state();
  }

  delay(100);
}