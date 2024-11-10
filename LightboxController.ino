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
byte uv_sense = 0;
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

  // Enable interrupt for UV sense

  TIMSK2 |= _BV(TOIE2);

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
  // On/off switch

  byte sw = digitalRead(SW_PIN);
  byte pwm_change = (!sw && sw_state);
  pwm_state ^= pwm_change;
  sw_state = sw;
  digitalWrite(LED_BUILTIN, pwm_state);

  // Front panel
  byte scan_state  = !digitalRead(BUT_1_PIN);
  byte print_state = !digitalRead(BUT_2_PIN);
  byte brush_state = !digitalRead(BUT_3_PIN);

  // Scan  = Green
  // Brush = Yellow
  // Print = Red
  // digitalWrite(LED_RED_PIN, brush_state || print_state);
  analogWrite(LED_RED_PIN, (brush_state || print_state) ? 128 : 0);
  digitalWrite(LED_GREEN_PIN, brush_state || scan_state);

  // Intensity setting

  int new_val = rotary.read() / 4;
  int adj_val = (new_val < 0) ? 0 : ((new_val > 100) ? 100 : new_val);
  if (adj_val != new_val)
    rotary.write(adj_val * 4);

  // UV mode sense
  // Pin is sensed during timer interrupt for proper sync with PWM
  // uv_sense is valid only if we're generating an active PWM cycle.

  byte uv_new_state = (pwm_state && rot_state) ? uv_sense : 0;
  byte uv_change = (uv_new_state != uv_state);
  uv_state = uv_new_state;

  // Update outputs

  if (pwm_change || uv_change || adj_val != rot_state) {
    rot_state = adj_val;

    // Output must be stable while we check the UV state
    digitalWrite(PWM_PIN, pwm_state);
    uv_state = !digitalRead(LED_MODE_PIN);

    // Now set PWM
    analogWrite(PWM_PIN, (pwm_state ? map(rot_state, 0, 100, 0, 255) : 0));
    oled_show_state();
  }

  delay(100);
}

// Timer2 overflow is exactly at the middle of the ON phase of PWM, whatever the duty cycle.
// So that's the place where we can sample the LED sense mode pin.
ISR(TIMER2_OVF_vect) {
  uv_sense = !digitalRead(LED_MODE_PIN);
}
