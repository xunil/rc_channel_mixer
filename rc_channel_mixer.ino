#define DEBUG 1

const int LED_PIN = 13;
int led_state = 1;
// RC channel mixer for the ROBOT.
const int THROTTLE_PIN = 12;
const int STEERING_PIN = 11;
const float MAX_STEER = 0.75;

const int THROTTLE_MIN_LIMIT = 1000;
const int THROTTLE_MAX_LIMIT = 1500;

unsigned long throttle_min = 0xffffffff;
unsigned long throttle_max = 0;
unsigned long throttle_duration;
unsigned long steering_min = 0xffffffff;
unsigned long steering_max = 0;
unsigned long steering_duration;
float throttle_percent;
float steering_percent;

unsigned int left_wheel_speed;
unsigned int right_wheel_speed;

// Slowing factor as you turn the ROBOT
const float A = 1.05;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, led_state);
  pinMode(THROTTLE_PIN, INPUT);
  pinMode(STEERING_PIN, INPUT);

  pinMode(A8, OUTPUT);
  pinMode(A9, OUTPUT);
#if defined(__MK66FX1M0__) // Teensy 3.6
  pinMode(A21, OUTPUT);
  pinMode(A22, OUTPUT);
#endif

  analogWriteFrequency(22, 11718.75); // pin 23 (A9) also changes
  analogWriteFrequency(A21, 11718.75);
  analogWriteResolution(12);
}

void loop() {
  // Throttle is 1000-1500 uS pulses; 1000 is full throttle, 1500 is idle
  // (2000 uS is full brake, but we won't use it)
  // Steering is 1000 to 2000 uS pulses, full left to full right.
  // Center stick is 1500 uS pulses.
  throttle_duration = pulseIn(THROTTLE_PIN, HIGH, 18000);
  steering_duration = pulseIn(STEERING_PIN, HIGH, 18000);

  // Track min/max; only used for debugging
  if (throttle_duration < throttle_min) throttle_min = throttle_duration;
  if (throttle_duration > throttle_max) throttle_max = throttle_duration;
  if (steering_duration < steering_min) steering_min = steering_duration;
  if (steering_duration > steering_max) steering_max = steering_duration;

  // Constrain throttle to (1000,1500)
  if (throttle_duration < THROTTLE_MIN_LIMIT) throttle_duration = THROTTLE_MIN_LIMIT;
  if (throttle_duration > THROTTLE_MAX_LIMIT) throttle_duration = THROTTLE_MAX_LIMIT;

  // Calculate throttle percentage (0.0,1.0)
  throttle_percent = max(0.0, 1.0 - ((throttle_duration - 1000) / 500.0));

  // Calculate steering (-1.0,1.0)
  steering_percent = ((int)steering_duration - 1500) / 500.0;  // -1.0 to 1.0

  // Constrain steering to (-MAX_STEER,MAX_STEER)
  if (abs(steering_percent) >= MAX_STEER) {
    if (steering_percent < 0.0) {
      steering_percent = -MAX_STEER;
    } else {
      steering_percent = MAX_STEER;
    }
  }

  // Roger's steering formula:
  // s = steering (values from -1.0 to 1.0)
  // S = ABS[s]
  // A = adjustable parameter a bit greater than 1
  // R = signal to right motor
  // L = signal to left motor (both signals need to be multiplied by 4096 in practical units)
  // The factor (1-A*S) is an overall slowing down de-rate that applies to all turns.
  // Then R = (1-A*S)(1+s)  and L = (1-A*S)(1-s).

  // Calculate DAC/PWM values.
  left_wheel_speed  = (int)(4096 * throttle_percent * (1.0 - A * abs(steering_percent)) * (1 + steering_percent));
  right_wheel_speed = (int)(4096 * throttle_percent * (1.0 - A * abs(steering_percent)) * (1 - steering_percent));

#ifdef DEBUG
  // Debugging
  Serial.print("Throttle: ");
  Serial.print(throttle_duration);
  Serial.print(" ");
  Serial.print(throttle_percent * 100.0);
  Serial.print("%");
  Serial.print(" min: ");
  Serial.print(throttle_min);
  Serial.print(" max: ");
  Serial.println(throttle_max);
  Serial.print("  Steering: ");
  Serial.print(steering_duration);
  Serial.print(" ");
  Serial.print(steering_percent * 100.0);
  Serial.print("%");
  Serial.print(" left wheel: ");
  Serial.print(left_wheel_speed);
  Serial.print(" right wheel: ");
  Serial.print(right_wheel_speed);
  Serial.print(" min: ");
  Serial.print(steering_min);
  Serial.print(" max: ");
  Serial.println(steering_max);
#endif

#if defined(__MK66FX1M0__) // Teensy 3.6
  analogWrite(A21, left_wheel_speed); // output to DACs
  analogWrite(A22, right_wheel_speed);
#endif
  analogWrite(A8, left_wheel_speed); // output to PWM pins just for giggles
  analogWrite(A9, right_wheel_speed);
  long now = millis();
  if ((now % 100) == 0) {
    led_state = !led_state;
    digitalWrite(LED_PIN, led_state);
  }
}
