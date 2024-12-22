//FORMAT
//Humidity: 0 % | Distance: 0 in | 0 cm | MODE: Default | AVG LUX: 680 | North: 630.86 | East: 631.84 | South: 650.39 | West: 743.16 | LUX REF: 310
//0 | 0 | 0 | 0 | 680 | 630.86 | 631.84 | 650.39 | 743.16 | 310

// LDR and LED setup
const int North_ldr = A1;
const int East_ldr = A2;
const int South_ldr = A3;
const int West_ldr = A4;

const int CenterLED = 5;
const int NorthWhite = 6;
const int EastBlue = 7;
const int EastWhite = 8;
const int SouthWhite = 9;
const int WestBlue = 10;
const int WestWhite = 11;

const int buttonPin = 13;
const int ledMode = 12;
const int potPin = A5;

int THRESHOLD = 50;

// Ultrasonic and Humidity setup
const int trigPin = 3;
const int echoPin = 4;
const int LED = 2; // LED for humidity
const int hSensor = A0;

// Timing variables
unsigned long lastButtonTime = 0;
unsigned long lastLogTime = 0;
unsigned long lastUltrasonicTime = 0;
unsigned long lastBlinkTime = 0;

const unsigned long buttonDebounceDelay = 200; // Debounce delay
const unsigned long logInterval = 500; // Data logging interval
const unsigned long ultrasonicInterval = 500; // Ultrasonic interval
long blinkInterval = 1000; // Blink interval for LEDs
unsigned long lastDataSendTime = 0;

// Variables
int mode = 0;
int luxReference = 600;
int lastButtonState = HIGH;
int avgLux = 0;
//float averageVolts=0;
bool blinking = false;
int blinkCount = 0;

bool sensorActive = false; 
// Additional variables for new features
int dataFrequency = 1000;        // Default frequency of data transmission in milliseconds
String measurementUnit = "Lux";  // Default measurement unit

const int sensorStatusLED = LED_BUILTIN;  // Additional LED pin for sensor status

bool voltageU=false;
//bool sensorsNotEnable=true;

int freq=500;

float voltage[4]; 

void logData(int ldrValues[]);
void HumidityData();
int calculateAverage(int ldrValues[]);
void logLuxData(float luxValues[]);
void defaultMode(int ldrValues[]);
void intensityIndicator(int ldrValues[]);
void Alarm(int ldrValues[]);
void controlDirectionalLEDs(int ldrValues[]);
void turnOffAllLEDs();
void turnOnAllLEDs();

long microsecondsToInches(long microseconds);
long microsecondsToCentimeters(long microseconds);
void averagevoltsLDR();
void turnOffDirection();

// LDR readings
int ldrValues[4] = {
  analogRead(North_ldr),
  analogRead(East_ldr),
  analogRead(South_ldr),
  analogRead(West_ldr)
};
float analogToLux(int analogValue);

void setup() {
  pinMode(CenterLED, OUTPUT);
  pinMode(NorthWhite, OUTPUT);
  pinMode(EastBlue, OUTPUT);
  pinMode(EastWhite, OUTPUT);
  pinMode(SouthWhite, OUTPUT);
  pinMode(WestBlue, OUTPUT);
  pinMode(WestWhite, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(ledMode, OUTPUT);
  pinMode(sensorStatusLED, OUTPUT);
  digitalWrite(sensorStatusLED, LOW); 

  Serial.begin(115200);
}

void loop() {
  unsigned long currentTime = millis();
  // Button handling with debounce
  int buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH && (currentTime - lastButtonTime >= buttonDebounceDelay)) {
    mode = (mode + 1) % 3; // Cycle through modes
    lastButtonTime = currentTime;
  }
  lastButtonState = buttonState;

  // LDR readings
  int ldrValues[4] = {
    analogRead(North_ldr),
    analogRead(East_ldr),
    analogRead(South_ldr),
    analogRead(West_ldr)
  };

  luxReference = map(analogRead(potPin), 0, 1023, 300, 800);
  avgLux = calculateAverage(ldrValues);

  // Mode-specific tasks
  switch (mode) {
    case 0:
      defaultMode(ldrValues);
      break;
    case 1:
      intensityIndicator(ldrValues);
      break;
    case 2:
      Alarm(ldrValues);
      break;
  }

  // Ultrasonic readings periodically
  if (currentTime - lastUltrasonicTime >= ultrasonicInterval) {
    long duration, inches, cm;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    inches = microsecondsToInches(duration);
    cm = microsecondsToCentimeters(duration);   

    lastUltrasonicTime = currentTime;
  }

    // Command Handling via Serial
      if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim(); // Correct: modifies `input` directly, does not return a value.
        int separatorIndex = input.indexOf(' ');

        if (separatorIndex != -1) {
            String cmd = input.substring(0, separatorIndex); // Extract command
            String param = input.substring(separatorIndex + 1); // Extract parameter

            cmd.trim();  // Trim the command
            param.trim(); // Trim the parameter

            // Process commands
            if (cmd.equalsIgnoreCase("MODE")) {
                int newMode = param.toInt();
                if (newMode >= 0 && newMode <= 2) { // Valid mode range
                    mode = newMode;
                } else {
                    Serial.println("Invalid MODE parameter. Use 0, 1, or 2.");
                }

                /////////////////////////////////////////////////////
            } else if (cmd.equalsIgnoreCase("UNIT")) {
                if (param.equalsIgnoreCase("Lux") ) {
                    voltageU=false;
                    measurementUnit = param;
 
                } else if(param.equalsIgnoreCase("Voltage")){
                    voltageU=true;
                    measurementUnit = param;
                    
                }else{
                    Serial.println("Invalid UNIT parameter. Use 'Lux' or 'Voltage'.");
                }
            } else if (cmd.equalsIgnoreCase("FREQ")) {
                freq = param.toInt();
                if (freq > 0) {
                    dataFrequency = freq;
                    //delay(freq);   
                } 
            } else if (cmd.equalsIgnoreCase("SENSOR")) {
                if (param.equalsIgnoreCase("ON")) {
                    sensorActive = true;
                    digitalWrite(sensorStatusLED, HIGH);
                } else if (param.equalsIgnoreCase("OFF")) {
                    sensorActive = false;
                    digitalWrite(sensorStatusLED, LOW);
                } 

            } else {
                Serial.println("Unknown command.");
            }
        } else {
            Serial.println("Invalid input. Use format: cmd PARAM.");
        }
    }

    // Regularly send sensor data if enabled
    if (sensorActive && millis() - lastDataSendTime >= dataFrequency) {
        sensorActive=true;
        lastDataSendTime = millis();
    }

  // Check if SENSOR is active
  if (sensorActive) {
    HumidityData(); 

    //Serial.print("Distance: ");
    long duration, inches, cm;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    inches = microsecondsToInches(duration);
    cm = microsecondsToCentimeters(duration);

    Serial.print(inches);
    //" in | "
    Serial.print(" | ");
    Serial.print(cm);
    //" cm | "
    Serial.print(" | ");
    logData(ldrValues);
  }else {
    //"Humidity: 0 % | "
    Serial.print("0 | ");
    //"Distance: 0 in | 0 cm | "
    Serial.print("0 | 0 | ");
    logData(ldrValues);
    
  }
  // Log lux values periodically
  if (currentTime - lastLogTime >= logInterval) {
    float luxValues[4];
    for (int i = 0; i < 4; i++) {
      luxValues[i] = analogToLux(ldrValues[i]);
    }
    //logLuxData(luxValues);
    lastLogTime = currentTime;
    //logData(ldrValues);
  }
  
}

void defaultMode(int ldrValues[]) {
  // Example of non-blocking LED blinking
  unsigned long currentTime = millis();

  if (currentTime - lastBlinkTime >= blinkInterval) {
    digitalWrite(ledMode, !digitalRead(ledMode)); // Toggle the LED state
    lastBlinkTime = currentTime;
  }

  // Check if all LDR values are similar
  bool allSimilar = true;
  for (int i = 1; i < 4; i++) {
    if (abs(ldrValues[i] - ldrValues[0]) > THRESHOLD) {
      allSimilar = false;
      break;
    }
  }

  if (allSimilar) {
    turnOffAllLEDs();
    static unsigned long lastBlinkCenterTime = millis();
    const unsigned long blinkIntervalCenter = 500; // Blink interval for CenterLED (500 ms)
    unsigned long currentBlinkTime = millis();
    if (currentBlinkTime - lastBlinkCenterTime >= blinkIntervalCenter) {
      digitalWrite(CenterLED, HIGH);
      lastBlinkCenterTime = currentBlinkTime;
    }
  } else {
    controlDirectionalLEDs(ldrValues);
  }
}

void controlDirectionalLEDs(int ldrValues[]) {
  const int ledPins[4][2] = {{NorthWhite, -1}, {EastBlue, EastWhite}, {SouthWhite, -1}, {WestBlue, WestWhite}};
  const int thresholds[4][2] = {{900, -1}, {650, 900}, {900, -1}, {650, 750}};

  int activeDirections = 0;

  for (int i = 0; i < 4; i++) {
    bool isBlueActive = false, isWhiteActive = false;

    // Check blue threshold
    if (ledPins[i][0] != -1 && ldrValues[i] >= thresholds[i][0]) {
      digitalWrite(ledPins[i][0], HIGH); // Turn on blue LED
      isBlueActive = true;
    } else if (ledPins[i][0] != -1) {
      digitalWrite(ledPins[i][0], LOW); // Turn off blue LED
    }

    // Check white threshold
    if (ledPins[i][1] != -1 && ldrValues[i] >= thresholds[i][1]) {
      digitalWrite(ledPins[i][1], HIGH); // Turn on white LED
      isWhiteActive = true;
    } else if (ledPins[i][1] != -1) {
      digitalWrite(ledPins[i][1], LOW); // Turn off white LED
    }

    // Mark active directions
    if (isBlueActive || isWhiteActive) {
      activeDirections |= (1 << i);
    }
  }

  // Prevent opposite directions from shining simultaneously
  if ((activeDirections & 0b0101) == 0b0101) { // North and South both active
    turnOffDirection(0); // Turn off North
    turnOffDirection(2); // Turn off South
  }
  if ((activeDirections & 0b1010) == 0b1010) { // East and West both active
    turnOffDirection(1); // Turn off East
    turnOffDirection(3); // Turn off West
  }
}

// Helper function to turn off LEDs for a specific direction
void turnOffDirection(int directionIndex) {
  const int ledPins[4][2] = {{NorthWhite, -1}, {EastBlue, EastWhite}, {SouthWhite, -1}, {WestBlue, WestWhite}};
  for (int j = 0; j < 2; j++) {
    if (ledPins[directionIndex][j] != -1) {
      digitalWrite(ledPins[directionIndex][j], LOW);
    }
  }
}

void intensityIndicator(int ldrValues[]) {
  const long blinkInterval = 500; 
  unsigned long currentTime = millis();

  if (currentTime - lastBlinkTime >= blinkInterval) {
    digitalWrite(ledMode, !digitalRead(ledMode)); // Toggle the LED state
    lastBlinkTime = currentTime;
  }

  turnOffAllLEDs();

  const int ledPins[5] = {EastWhite, EastBlue, CenterLED, WestBlue, WestWhite};
  int level = (avgLux > 550 && avgLux <= 650) ? 1 :
              (avgLux > 650 && avgLux <= 750) ? 2 :
              (avgLux > 750 && avgLux <= 850) ? 3 :
              (avgLux > 850 && avgLux <= 950) ? 4 :
              (avgLux > 950) ? 5 : 0;

  for (int i = 0; i < level; i++) {
    digitalWrite(ledPins[i], HIGH);
  }
}

void Alarm(int ldrValues[]) {
  //blinkLED(ledMode, 3, 100);
  const long blinkInterval = 250; 
  unsigned long currentTime = millis();

  if (currentTime - lastBlinkTime >= blinkInterval) {
    digitalWrite(ledMode, !digitalRead(ledMode)); // Toggle the LED state
    lastBlinkTime = currentTime;
  }

  if (avgLux > luxReference) {
    //Serial.println("ALARM ACTIVATED");
    for (int i = 0; i < 3; i++) {
      turnOnAllLEDs();
      delay(200);
      turnOffAllLEDs();
      delay(200);
    }
  }
}


void averagevoltsLDR(){
  float sumVolts=0.0;
  for(int i=0;i<4;i++){
    sumVolts+=voltage[i]; 
  }
  float averageVolts= sumVolts/4;
  Serial.print(averageVolts);
}

void logLuxData(float luxValues[]) {
  if(!voltageU){
    /*Serial.print(" | North: "); Serial.print(luxValues[0], 2);
    Serial.print(" | East: "); Serial.print(luxValues[1], 2);
    Serial.print(" | South: "); Serial.print(luxValues[2], 2);
    Serial.print(" | West: "); Serial.print(luxValues[3], 2);
    Serial.print(" | LUX REF: "); Serial.println(luxReference);*/
    Serial.print(avgLux);
    Serial.print(" | "); Serial.print(luxValues[0], 2);
    Serial.print(" | "); Serial.print(luxValues[1], 2);
    Serial.print(" | "); Serial.print(luxValues[2], 2);
    Serial.print(" | "); Serial.print(luxValues[3], 2);
    Serial.print(" | "); Serial.println(luxReference);
    delay(freq);
  }else{
    averagevoltsLDR();
    voltage[4];  // Array to store voltage values
    for (int i = 0; i < 4; i++) {
        int analogValue = analogRead(i);  // Read the analog value for each LDR
        voltage[i] = map(analogValue, 0, 1023, 0, 5000) / 1000.0; // Convert to voltage (0-5V)
        //Serial.print(" | volts "); Serial.print(i); Serial.print(": "); Serial.print(voltage[i], 3); Serial.print(" V");
        Serial.print(" | "); Serial.print(voltage[i], 3); //Serial.print(" V");
    }///////////////////////////////////////////////////////////////////
    Serial.print(" | "); Serial.println(luxReference);
    delay(freq);
  }
}

void turnOffAllLEDs() {
  digitalWrite(CenterLED, LOW);
  digitalWrite(NorthWhite, LOW);
  digitalWrite(EastBlue, LOW);
  digitalWrite(EastWhite, LOW);
  digitalWrite(SouthWhite, LOW);
  digitalWrite(WestBlue, LOW);
  digitalWrite(WestWhite, LOW);
}
void turnOnAllLEDs() {
  digitalWrite(CenterLED, HIGH);
  digitalWrite(NorthWhite, HIGH);
  digitalWrite(EastBlue, HIGH);
  digitalWrite(EastWhite, HIGH);
  digitalWrite(SouthWhite, HIGH);
  digitalWrite(WestBlue, HIGH);
  digitalWrite(WestWhite, HIGH);
}

int calculateAverage(int ldrValues[]) {
  int sum = 0;
  for (int i = 0; i < 4; i++) {
    sum += ldrValues[i];
  }
  return sum / 4;
}

long microsecondsToInches(long microseconds) {
  return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds) {
   return microseconds / 29 / 2;
}
void logData(int ldrValues[]) {
  float luxValues[4] = {
  analogToLux(ldrValues[0]),
  analogToLux(ldrValues[1]),
  analogToLux(ldrValues[2]),
  analogToLux(ldrValues[3])
  };

  //Serial.print("MODE: ");
  //Serial.print(mode == 0 ? "Default | " : mode == 1 ? "Intensity Indicator | " : "Alarm | ");
  Serial.print(mode == 0 ? "0 | " : mode == 1 ? "1 | " : "2 | ");
  //Serial.print("AVG LUX: "); 
  //Serial.print(avgLux);
  logLuxData(luxValues);

  
  // Ultrasonic and Humidity readings  
}
void HumidityData(){
  int humidityValue = map(analogRead(hSensor), 0, 1023, 100, 0);
  //Serial.print("Humidity: ");
  Serial.print(humidityValue);
  //"% | "
  Serial.print(" | ");

  if (humidityValue > 3) {
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
}
float analogToLux(int analogValue) {
  return (analogValue / 1024.0) * 1000; 
}