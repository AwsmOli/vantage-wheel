
#include <Arduino.h>
#include <BleGamepad.h>

// Configure gamepad with 32 buttons, no axes, no hats, no simulation controls
BleGamepadConfiguration bleGamepadConfig;
BleGamepad bleGamepad("VantageWheel", "VantageWheel", 100);

// This version implements a USB HID gamepad device with rotary encoders
// Focus: 5 rotary encoders creating gamepad button inputs

// Rotary Encoder Configuration
// Each encoder needs 2 pins (CLK and DT) + 1 button pin (SW)
// Encoder rotation will trigger button presses (CW = one button, CCW = another)
// Encoder button press will trigger a third button
#define NUM_ENCODERS 5

// Encoder pin definitions (avoid GPIO 15, 16, 19, 20)
// Strapping Pins: 0, 3, 46, 45
const int encoder_gnd_pins[5] = {4,  17,  4, 38, 4};      // gnd pins
const int encoder_vcc_pins[5] = {18, 18, 18, 37, 14};      // vcc pins
const int encoder_btn_pins[5] = {5,   8, 44, 36, 13}; // SW (button) pins
const int encoder_dt_pins[5]  = {6,   9,  1, 35, 12};      // DT pins
const int encoder_clk_pins[5] = {7,  10,  2, 48, 11};      // CLK pins

// Encoder state tracking
struct EncoderState {
  int last_clk;
  int last_dt;
  int last_btn;
  bool rotation_detected;              // True when rotation is in progress
  int pending_direction;               // Direction waiting to be reported: 1=CW, -1=CCW, 0=none
  unsigned long last_change_time;
  unsigned long last_btn_time;
  unsigned long rotation_clear_time;  // Time to clear rotation button
  int active_rotation_button;         // Which rotation button is active (-1 = none)
  const unsigned long debounce_ms = 5;       // Debounce time for state changes
  const unsigned long btn_debounce_ms = 50; // Debounce time for button
  const unsigned long rotation_pulse_ms = 50; // How long rotation buttons stay active
};

EncoderState encoder_states[NUM_ENCODERS];

// Gamepad button state: 32-bit for buttons
uint32_t gamepad_buttons = 0;

// Flag to track if Bluetooth is ready
bool ble_connected = false;

// Note: USB Host functionality removed - Arduino framework doesn't support TinyUSB host APIs
// To add USB host support, this project needs to be converted to ESP-IDF framework

// Function to read encoders and update button states
// Each encoder gets 3 button numbers: CW rotation, CCW rotation, and button press
// Encoder 0: buttons 11-13 (CW/CCW/BTN)
// Encoder 1: buttons 14-16 (CW/CCW/BTN)
// Encoder 2: buttons 17-19 (CW/CCW/BTN)
// Encoder 3: buttons 20-22 (CW/CCW/BTN)
// Encoder 4: buttons 23-25 (CW/CCW/BTN)
void readEncoders() {
  for (int i = 0; i < NUM_ENCODERS; i++) {
    int clk_state = digitalRead(encoder_clk_pins[i]);
    int dt_state = digitalRead(encoder_dt_pins[i]);
    int btn_state = digitalRead(encoder_btn_pins[i]);
    unsigned long current_time = millis();
    
    // Check if we need to clear a rotation button (non-blocking)
    if (encoder_states[i].active_rotation_button >= 0 && 
        current_time >= encoder_states[i].rotation_clear_time) {
      gamepad_buttons &= ~(1 << encoder_states[i].active_rotation_button);
      encoder_states[i].active_rotation_button = -1;
    }
    
    // Encoder state machine: only trigger on complete detent (full click)
    // Detent complete when both CLK and DT return to HIGH after a rotation
    if ((current_time - encoder_states[i].last_change_time) > encoder_states[i].debounce_ms) {
      
      // Detect start of rotation: CLK goes LOW
      if (clk_state == LOW && encoder_states[i].last_clk == HIGH && !encoder_states[i].rotation_detected) {
        // Rotation started - determine direction from DT state
        encoder_states[i].pending_direction = (dt_state == HIGH) ? 1 : -1;
        encoder_states[i].rotation_detected = true;
        encoder_states[i].last_change_time = current_time;
      }
      // Detect end of rotation: both CLK and DT back to HIGH (detent position)
      else if (clk_state == HIGH && dt_state == HIGH && 
               encoder_states[i].rotation_detected && 
               (encoder_states[i].last_clk == LOW || encoder_states[i].last_dt == LOW)) {
        
        // Complete detent reached - trigger button
        int button_cw = 10 + (i * 3);   // Clockwise button (starting from button 11)
        int button_ccw = 11 + (i * 3);  // Counter-clockwise button
        
        // Clear any previous rotation button before setting new one
        if (encoder_states[i].active_rotation_button >= 0) {
          gamepad_buttons &= ~(1 << encoder_states[i].active_rotation_button);
        }
        
        if (encoder_states[i].pending_direction == 1) {
          // Clockwise rotation complete
          gamepad_buttons |= (1 << button_cw);
          encoder_states[i].active_rotation_button = button_cw;
          encoder_states[i].rotation_clear_time = current_time + encoder_states[i].rotation_pulse_ms;
          printf("Encoder %d: CW (BTN%d) DETENT\n", i, button_cw + 1);
        } else {
          // Counter-clockwise rotation complete
          gamepad_buttons |= (1 << button_ccw);
          encoder_states[i].active_rotation_button = button_ccw;
          encoder_states[i].rotation_clear_time = current_time + encoder_states[i].rotation_pulse_ms;
          printf("Encoder %d: CCW (BTN%d) DETENT\n", i, button_ccw + 1);
        }
        
        encoder_states[i].rotation_detected = false;
        encoder_states[i].pending_direction = 0;
        encoder_states[i].last_change_time = current_time;
      }
      
      encoder_states[i].last_clk = clk_state;
      encoder_states[i].last_dt = dt_state;
    }
    
    
    if (btn_state != encoder_states[i].last_btn &&
        (current_time - encoder_states[i].last_btn_time) > encoder_states[i].btn_debounce_ms) {
      
      encoder_states[i].last_btn_time = current_time;
      encoder_states[i].last_btn = btn_state;
      
      int button_press = 12 + (i * 3);  // Button press (starting from button 13)
      
      if (btn_state == LOW) {
        // Button pressed
        gamepad_buttons |= (1 << button_press);
        printf("Encoder %d: PRESSED (BTN%d) | buttons=0x%08X\n", i, button_press + 1, gamepad_buttons);
      } else {
        // Button released
        gamepad_buttons &= ~(1 << button_press);
        printf("Encoder %d: RELEASED (BTN%d) | buttons=0x%08X\n", i, button_press + 1, gamepad_buttons);
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  printf("\n\n=== ESP32-S3 Gamepad with Rotary Encoders ===\n");
  
  // Initialize all unique power pins used by encoders
  // Collect unique GND and VCC pins
  bool gnd_configured[49] = {false};  // Track which GPIO pins are configured
  bool vcc_configured[49] = {false};
  
  for (int i = 0; i < NUM_ENCODERS; i++) {
    int gnd_pin = encoder_gnd_pins[i];
    int vcc_pin = encoder_vcc_pins[i];
    
    if (!gnd_configured[gnd_pin]) {
      pinMode(gnd_pin, OUTPUT);
      digitalWrite(gnd_pin, LOW);
      gnd_configured[gnd_pin] = true;
      printf("Configured GND pin: GPIO%d\n", gnd_pin);
    }
    
    if (!vcc_configured[vcc_pin]) {
      pinMode(vcc_pin, OUTPUT);
      digitalWrite(vcc_pin, HIGH);
      vcc_configured[vcc_pin] = true;
      printf("Configured VCC pin: GPIO%d\n", vcc_pin);
    }
  }
  delay(50); // Let power stabilize
  
  // Initialize encoder pins
  for (int i = 0; i < NUM_ENCODERS; i++) {
    // Configure input pins
    // CLK/DT use no pull resistors (encoder has pull-ups), but SW needs pull-up
    pinMode(encoder_clk_pins[i], INPUT);
    pinMode(encoder_dt_pins[i], INPUT);
    pinMode(encoder_btn_pins[i], INPUT_PULLUP);  // SW pin needs pull-up
    
    encoder_states[i].last_clk = digitalRead(encoder_clk_pins[i]);
    encoder_states[i].last_dt = digitalRead(encoder_dt_pins[i]);
    encoder_states[i].last_btn = digitalRead(encoder_btn_pins[i]);
    encoder_states[i].rotation_detected = false;
    encoder_states[i].pending_direction = 0;
    encoder_states[i].last_change_time = 0;
    encoder_states[i].last_btn_time = 0;
    encoder_states[i].rotation_clear_time = 0;
    encoder_states[i].active_rotation_button = -1;
    
    printf("Encoder %d: VCC=%d GND=%d CLK=%d DT=%d BTN=%d | Initial: CLK=%d BTN=%d\n", 
           i, encoder_vcc_pins[i], encoder_gnd_pins[i], encoder_clk_pins[i], 
           encoder_dt_pins[i], encoder_btn_pins[i],
           encoder_states[i].last_clk, encoder_states[i].last_btn);
  }
  printf("Initialized %d rotary encoders with buttons\n", NUM_ENCODERS);
  
  // Initialize BLE Gamepad with custom configuration
  printf("Initializing Bluetooth HID Gamepad...\n");
  
  // Configure gamepad: 32 buttons, no axes, no hats
  bleGamepadConfig.setAutoReport(false);
  bleGamepadConfig.setButtonCount(32);
  bleGamepadConfig.setHatSwitchCount(0);
  
  // Disable all axes
  bleGamepadConfig.setAxesMin(0x0000);
  bleGamepadConfig.setAxesMax(0x0000);
  bleGamepadConfig.setWhichAxes(false, false, false, false, false, false, false, false);  // Disable X, Y, Z, Rx, Ry, Rz, Slider1, Slider2
  bleGamepadConfig.setWhichSimulationControls(false, false, false, false, false); // Disable rudder, throttle, accelerator, brake, steering
  
  bleGamepad.begin(&bleGamepadConfig);
  printf("Gamepad configured: 32 buttons only, no axes\n");
  
  printf("Waiting for Bluetooth connection...\n");
  printf("Look for 'VantageWheel' in your Bluetooth devices\n");
  
  ble_connected = false;
  printf("ESP32-S3 Gamepad Ready\n");
  printf("Device emulates a gamepad with 32 buttons\n");
  printf("Buttons 1-10: Reserved for USB host gamepad (not implemented)\n");
  printf("Buttons 11-25: Rotary encoders (CW/CCW/Press × 5)\n");
  printf("Buttons 26-32: Reserved\n");
}

void loop()
{
  // Check BLE connection status
  if (bleGamepad.isConnected() && !ble_connected) {
    ble_connected = true;
    printf("✓ Bluetooth connected!\n");
  } else if (!bleGamepad.isConnected() && ble_connected) {
    ble_connected = false;
    printf("✗ Bluetooth disconnected\n");
  }
  
  // Read encoders continuously
  readEncoders();
  
  // Only send when buttons change
  static uint32_t last_buttons = 0;
  if (gamepad_buttons != last_buttons) {
    
    // Send to BLE gamepad if connected
    if (ble_connected) {
      // BleGamepad uses individual button press/release, not a bit mask
      // Compare current vs previous state to send changes
      for (int i = 0; i < 32; i++) {
        bool current_state = (gamepad_buttons >> i) & 1;
        bool previous_state = (last_buttons >> i) & 1;
        
        if (current_state != previous_state) {
          if (current_state) {
            bleGamepad.press(i + 1);  // BLE buttons are 1-indexed
          } else {
            bleGamepad.release(i + 1);
          }
        }
      }
      bleGamepad.sendReport();
    }
    
    last_buttons = gamepad_buttons;
  }
  
  delay(1);
}