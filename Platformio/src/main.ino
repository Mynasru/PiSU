






// Momentary power swith with LED
#define BUTTON_PIN 2
#define BUTTON_LED_PIN 3

#define pressToShdnTime 1000 //ms to press before a shutdown occurs.
#define pressToHrdShdnTime 8000 //ms to press before a hard power shutdown occurs.
#define pressToRebootTime 4000 //ms to press to reset the pi.
#define debounceTime 50
#define restartHoldTime 250+debounceTime //ms to keep pi pin active to reboot
#define powerdownHoldTime 1000+debounceTime //ms to keep pi pin active to powerdown
#define standbyFadeTime 50
#define startupBlinkTime 100
#define shutdownBlinkTime 250
boolean fadeDirection = HIGH;
int ledBrightness = 0;
volatile boolean buttonPressed = false;
boolean buttonTriggered = false;
unsigned long pressTime = 0;
unsigned long pressTimeOld = 0;
volatile unsigned long pressTimeElapsed = 0;

// Raspberry Pi settings
#define PI_FET_PIN 13
#define PI_BOOT_OK_PIN A3
#define PI_SENSE_PIN A1 // Not used anymore
#define PI_SHUTDOWN_PIN A2

// RAMPS 1.4 settings
#define RAMPS_ATX_PIN 4

// ATX PSU settings
#define ATX_PWR_OK_PIN 7
#define ATX_PWR_ON_PIN 6 // Normally high (3.3v !!)

// General variables
#define BUZZER_PIN 9
#define longBeepTime 2000
#define shortBeepTime 200
int state = 0;
#define offState 0
#define startupState 1
#define onState 2
#define shutdownState 3
#define hardShutdownState 4
#define rebootState 5

// Indicator LEDS
#define INDICATOR_LED_R1 12
#define INDICATOR_LED_R2 A0
#define INDICATOR_LED_G1 11
#define INDICATOR_LED_G2 10

void shutdownFunc(){
  boolean i = LOW;
  boolean shutdownnow = LOW;
  boolean tempbootPinState = LOW;
  do{
    i=!i;
    digitalWrite(BUTTON_LED_PIN,i);
    digitalWrite(INDICATOR_LED_R1,i);
    digitalWrite(INDICATOR_LED_G1,i);
    Serial.print(".");
    tempbootPinState = digitalRead(PI_BOOT_OK_PIN);
    delay(shutdownBlinkTime);
    if(tempbootPinState==HIGH && digitalRead(PI_BOOT_OK_PIN)==HIGH){
      shutdownnow = HIGH;
    }
  } while(shutdownnow==LOW);
}

// Setup
void setup(){
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  pinMode(BUTTON_LED_PIN,OUTPUT);
  pinMode(PI_BOOT_OK_PIN,INPUT_PULLUP);
  pinMode(PI_SENSE_PIN,OUTPUT);
  pinMode(PI_SHUTDOWN_PIN,OUTPUT);
  pinMode(RAMPS_ATX_PIN,INPUT_PULLUP);
  pinMode(ATX_PWR_OK_PIN,INPUT);
  pinMode(ATX_PWR_ON_PIN,OUTPUT);
  pinMode(BUZZER_PIN,OUTPUT);
  pinMode(PI_FET_PIN,OUTPUT);
  pinMode(INDICATOR_LED_R1,OUTPUT);
  pinMode(INDICATOR_LED_R2,OUTPUT);
  pinMode(INDICATOR_LED_G1,OUTPUT);
  pinMode(INDICATOR_LED_G2,OUTPUT);

  digitalWrite(ATX_PWR_ON_PIN,HIGH);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), isr, CHANGE);

  Serial.begin(19200);
}

// Loop
void loop(){
  switch(state){
    case offState:
      {
        digitalWrite(INDICATOR_LED_R1,LOW);
        digitalWrite(INDICATOR_LED_R2,LOW);
        digitalWrite(INDICATOR_LED_G1,LOW);
        digitalWrite(INDICATOR_LED_G2,LOW);

        Serial.println("Off state");

        while(buttonPressed==false || pressTimeElapsed<debounceTime){
          if(ledBrightness>=255){
            fadeDirection=LOW;
          }
          else if(ledBrightness<=0){
            fadeDirection=HIGH;
          }
          if(fadeDirection){
            ledBrightness++;
          }
          else{
            ledBrightness--;
          }
          analogWrite(BUTTON_LED_PIN,ledBrightness);
          delay(standbyFadeTime);
        }

        state = startupState;
        buttonPressed=false;
      }
      break;
    case startupState:
      {
        digitalWrite(INDICATOR_LED_R1,LOW);
        digitalWrite(INDICATOR_LED_R2,LOW);
        digitalWrite(INDICATOR_LED_G1,HIGH);
        digitalWrite(INDICATOR_LED_G2,LOW);

        delay(1000);

        Serial.println("Checking PSU status");
        digitalWrite(ATX_PWR_ON_PIN,LOW);
        while(!digitalRead(ATX_PWR_OK_PIN)){
          delay(1);
          digitalWrite(INDICATOR_LED_R1,LOW);
          digitalWrite(INDICATOR_LED_R2,HIGH);
          digitalWrite(INDICATOR_LED_G1,HIGH);
          digitalWrite(INDICATOR_LED_G2,LOW);
        }
        Serial.println("PWR OK!");
        digitalWrite(PI_FET_PIN,HIGH);
        Serial.println("Waiting for Pi to boot");
        boolean k = LOW;
        while(digitalRead(PI_BOOT_OK_PIN)){
          digitalWrite(INDICATOR_LED_R1,LOW);
          digitalWrite(INDICATOR_LED_R2,LOW);
          digitalWrite(INDICATOR_LED_G1,LOW);
          digitalWrite(INDICATOR_LED_G2,HIGH);
          k=!k;
          digitalWrite(BUTTON_LED_PIN,k);
          delay(startupBlinkTime);
          Serial.print(".");
        }
        Serial.println("");
        Serial.println("Pi is ready");
        //digitalWrite(PI_SENSE_PIN, HIGH);
        state = onState;
      }
      break;
    case onState:
      {
        digitalWrite(INDICATOR_LED_R1,LOW);
        digitalWrite(INDICATOR_LED_R2,LOW);
        digitalWrite(INDICATOR_LED_G1,HIGH);
        digitalWrite(INDICATOR_LED_G2,HIGH);

        analogWrite(BUTTON_LED_PIN,255);

        if(buttonPressed==true && pressTimeElapsed>=pressToShdnTime && pressTimeElapsed<pressToRebootTime || digitalRead(RAMPS_ATX_PIN)==LOW){
          buttonPressed=false;
          state=shutdownState;
        }
        else if(buttonPressed==true && pressTimeElapsed>=pressToRebootTime && pressTimeElapsed<pressToHrdShdnTime){
          buttonPressed=false;
          state=rebootState;
        }
        else if(buttonPressed==true && pressTimeElapsed>=pressToHrdShdnTime){
          buttonPressed=false;
          state=hardShutdownState;
        }
        else if(digitalRead(PI_BOOT_OK_PIN)==HIGH){
          delay(1000);
          if(digitalRead(PI_BOOT_OK_PIN)==HIGH){
            Serial.println("Pi shutdown itself, so it does not need power anymore :)");
            delay(4000);
            state=hardShutdownState;
          }
        }
      }
      break;
    case shutdownState:
      {
        digitalWrite(PI_SHUTDOWN_PIN,HIGH);
        delay(powerdownHoldTime);
        digitalWrite(PI_SHUTDOWN_PIN, LOW);

        digitalWrite(INDICATOR_LED_R1,HIGH);
        digitalWrite(INDICATOR_LED_R2,LOW);
        digitalWrite(INDICATOR_LED_G1,HIGH);
        digitalWrite(INDICATOR_LED_G2,LOW);
        Serial.println("Power down");
        Serial.println(pressTimeElapsed);
        Serial.println("");

        Serial.println("Pi is shutting down");
        shutdownFunc();
        Serial.println("");

        analogWrite(BUTTON_LED_PIN,0);
        digitalWrite(INDICATOR_LED_R1,HIGH);
        digitalWrite(INDICATOR_LED_R2,LOW);
        digitalWrite(INDICATOR_LED_G1,LOW);
        digitalWrite(INDICATOR_LED_G2,LOW);
        delay(5000);

        digitalWrite(PI_FET_PIN,LOW);
        digitalWrite(ATX_PWR_ON_PIN,HIGH);
        digitalWrite(INDICATOR_LED_R1,LOW);
        digitalWrite(INDICATOR_LED_R2,HIGH);
        digitalWrite(INDICATOR_LED_G1,LOW);
        digitalWrite(INDICATOR_LED_G2,LOW);
        delay(2000);

        state=offState;
      }
      break;
    case hardShutdownState:
      {
        digitalWrite(PI_FET_PIN, LOW);
        digitalWrite(ATX_PWR_ON_PIN, HIGH);

        analogWrite(BUTTON_LED_PIN,52);
        digitalWrite(INDICATOR_LED_R1,HIGH);
        digitalWrite(INDICATOR_LED_R2,HIGH);
        digitalWrite(INDICATOR_LED_G1,LOW);
        digitalWrite(INDICATOR_LED_G2,LOW);
        Serial.println("Hard Shutdown");
        Serial.println(pressTimeElapsed);
        Serial.println("");
        delay(2000);
        state=offState;
      }
      break;
    case rebootState:
      {
        digitalWrite(PI_SHUTDOWN_PIN,HIGH);
        delay(restartHoldTime);
        digitalWrite(PI_SHUTDOWN_PIN, LOW);

        digitalWrite(INDICATOR_LED_R1,LOW);
        digitalWrite(INDICATOR_LED_R2,HIGH);
        digitalWrite(INDICATOR_LED_G1,HIGH);
        digitalWrite(INDICATOR_LED_G2,LOW);
        Serial.println("Reboot");
        Serial.println(pressTimeElapsed);
        Serial.println("");

        Serial.println("Waiting for PI to shutdown");
        shutdownFunc();
        Serial.println("");

        delay(5000);
        //digitalWrite(PI_FET_PIN,LOW);

        analogWrite(BUTTON_LED_PIN,0);
        state=startupState;
        delay(2000);
      }
      break;
  }
}

void isr(){
  if(!buttonTriggered){
    pressTime = millis();
  }
  else{
    pressTimeOld = millis();
    buttonPressed = true;
    pressTimeElapsed = pressTimeOld - pressTime;
  }
  buttonTriggered = !buttonTriggered;
}
