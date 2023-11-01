/*
  Project Name: OpenFIRE Lightgun (V_1.0)
  Authors: Sam Ballantyne & Mike Lynch
  Date: Oct 2023
  
  This work is licensed under the Creative Commons Attribution-NonCommercial 4.0 International License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc/4.0/ or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

  You are free to:
    - Share: Copy and redistribute the material in any medium or format.
    - Adapt: Remix, transform, and build upon the material.
  
  Under the following terms:
    - Attribution: You must give appropriate credit, provide a link to the license, and indicate if changes were made.
    - NonCommercial: You may not use the material for commercial purposes.
  
  For more details, refer to the full license text available at the aforementioned URL.
*/
 
 /*
    HOW TO USE:
    Press Cali button to start calibration and shoot the curser as it moves around the screen. 
    i.e. center, top, bottom, left, right, center (again)
    Done!
*/

// Forward declaration of classes to allow for conditional compilation


#include <OpenFIRE_Square.h>
OpenFIRE_Square OpenFIREpos;

#include <OpenFIREBoard.h>
#include <OpenFIREConst.h>
#include <Wire.h>
#include <DFRobotIRPositionEx.h>
#include <OpenFIRE_Perspective.h>
#include <MouseAbsolute.h>

#include <hardware/pwm.h>
#include <hardware/irq.h>
// declare PWM ISR
void rp2040pwmIrq(void);

DFRobotIRPositionEx dfrIRPos(Wire);
DFRobotIRPositionEx::Sensitivity_e irSensitivity = DFRobotIRPositionEx::Sensitivity_Default;
OpenFIRE_Perspective OpenFIREper;

constexpr unsigned int IRCamUpdateRate = 209;
unsigned int irPosUpdateTick = 0;

int cali = 0;
int caseNumber = -1;

float res_x = 1920 << 2;         // Resoltuion of postion library 
float res_y = 1080 << 2;              

float _topOffset = 0;            // Led offset in pixels x4 (hold value before updating)
float _bottomOffset = 0;         
float _leftOffset = 0;          
float _rightOffset = 0; 

float topOffset = 0;             // Top Led offset in pixels x4
float bottomOffset = 0;          // Bottom Led offset in pixels x4
float leftOffset = 0;     
float rightOffset = 0;      

float TLled = 500 << 2;               // Top Left Led location
float TRled = 1420 << 2;              // Top Right Led Location
float Bled = 1080 << 2;

float adjX = 512 << 2;                // Cam center x offset
float adjY = 384 << 2;                // Cam caenter y offset

int caliPin = 14;               // Set Calibration Pin
int leftPin = 1;                // Set Left Mouse Pin
int rightPin = 3;               // Set Right Mouse Pin
int middlePin = 4;              // Set Middle Mouse Pin

int buttonState1 = 0;             // Set Button states
int lastButtonState1 = 0;
int buttonState2 = 0;
int lastButtonState2 = 0;
int buttonState3 = 0;
int lastButtonState3 = 0;
int buttonState4 = 0;         
int lastButtonState4 = 0;

int mouseX;                        // Intial Mouse postion buffer
int mouseY;
int mouseXX;                       // Mapped to Absolute Mouse position(0 - 32768) 
int mouseYY;

//////DEBUGGING//////
int rawX[4];                    // RAW Camera Output mapped to screen res (1920x1080)
int rawY[4];
int centerX;                    // Cam Median mapped to screen res (1920x1080)
int centerY;

///mccutheon serial additions

String readString;
String playerName = "Player 1";

int value1;
int value2;
int value3;
char category;

const unsigned long eventInterval = 5000;
unsigned long previousTime = 0;

int rumblePin = 8;
int rumbleDelay = 500;

int solPin = 21;
int solHighDelay = 70;
int solLowDelay = 30;

///



void setup() {

  OpenFIREper.source(adjX, adjY);                                                          
  OpenFIREper.deinit(0);

  pinMode(caliPin, INPUT_PULLUP);         // Set pin modes
  pinMode(leftPin, INPUT_PULLUP);
  pinMode(rightPin, INPUT_PULLUP);
  pinMode(middlePin, INPUT_PULLUP);
   
  MouseAbsolute.begin();
  // Start IR Camera with basic data format
  dfrIRPos.begin(DFROBOT_IR_IIC_CLOCK, DFRobotIRPositionEx::DataFormat_Basic, irSensitivity);
  // IR camera maxes out motion detection at ~300Hz, and millis() isn't good enough
  startIrCamTimer(IRCamUpdateRate);

  Serial.begin(115200); 
    
}

void loop() {

  processSerial();
  getPosition();
  startCali();

  if (cali == 1) {
    getPosition();
    calibration();
  } else {
    MouseAbsolute.move(mouseXX, mouseYY, 0);
    mouseButtons();
  }

   //PrintResults(); //commenting out so serial is readable
    
}


void getPosition() {

    int error = dfrIRPos.basicAtomic(DFRobotIRPositionEx::Retry_2);
    if(error == DFRobotIRPositionEx::Error_Success) {
      OpenFIREpos.begin(dfrIRPos.xPositions(), dfrIRPos.yPositions(), dfrIRPos.seen());
      OpenFIREper.warp(OpenFIREpos.X(0), OpenFIREpos.Y(0), OpenFIREpos.X(1), OpenFIREpos.Y(1), OpenFIREpos.X(2), OpenFIREpos.Y(2), OpenFIREpos.X(3), OpenFIREpos.Y(3), TLled, TRled, res_y);
      // Output mapped to screen resolution because offsets are measured in pixels
      mouseX = map(OpenFIREper.getX(), 0, res_x, (0 - leftOffset), (res_x + rightOffset));                 
      mouseY = map(OpenFIREper.getY(), 0, res_y, (0 - topOffset), (res_y + bottomOffset));
      // Output mapped to Mouse resolution
      mouseXX = map(mouseX, 0, res_x, 0, 32678);                 
      mouseYY = map(mouseY, 0, res_y, 0, 32678);

      //////// DEBUGGING /////////
      for (int i = 0; i < 4; i++) {
        rawX[i] = map(OpenFIREpos.X(i), 0, 1023 << 2, 0, 1920);              // RAW Output for viewing in processing sketch mapped to 1920x1080 screen resolution
        rawY[i] = map(OpenFIREpos.Y(i), 0, 768 << 2, 0, 1080);
      }
      centerX = map(OpenFIREpos.testMedianX(), 0, 1023 << 2, 1920, 0);       // Median for viewing in processing
      centerY = map(OpenFIREpos.testMedianY(), 0, 768 << 2, 1080, 0);
      //////// DEBUGGING /////////

    } else if(error == DFRobotIRPositionEx::Error_IICerror) {
      //Serial.println("Device not available!"); //commenting out so serial is readable
      
    }      
    
}

void calibration() {

  buttonState2 = digitalRead(leftPin);

  if (buttonState2 != lastButtonState2) {
      if (buttonState2 == LOW) {
        caseNumber++;
        if (caseNumber > 6) {
          caseNumber = 0;
        }
        switch (caseNumber) {
          
          case 0:
          MouseAbsolute.move(32768/2, 32768/2, 0);
          break;

          case 1:
          // Resest Offsets
          topOffset = 0;             
          bottomOffset = 0;         
          leftOffset = 0; 
          rightOffset = 0;
                                      
          // Set Cam center offsets
          adjX = (OpenFIREpos.testMedianX() - (512 << 2)) * cos(OpenFIREpos.Ang()) - (OpenFIREpos.testMedianY() - (384 << 2)) * sin(OpenFIREpos.Ang()) + (512 << 2);       
          adjY = (OpenFIREpos.testMedianX() - (512 << 2)) * sin(OpenFIREpos.Ang()) + (OpenFIREpos.testMedianY() - (384 << 2)) * cos(OpenFIREpos.Ang()) + (384 << 2);
          // Work out Led locations by assuming height is 100%
          TLled = (res_x / 2) - ( (OpenFIREpos.W() * (res_y  / OpenFIREpos.H()) ) / 2);            
          TRled = (res_x / 2) + ( (OpenFIREpos.W() * (res_y  / OpenFIREpos.H()) ) / 2);
          // Update Cam centre in perspective library
          OpenFIREper.source(adjX, adjY);                                                          
          OpenFIREper.deinit(0);
          // Move move to first calibration point
          MouseAbsolute.move(32768/2, 0, 0);
          break;

        case 2:
          // Set Offset buffer
          _topOffset = mouseY;
          // Move move to next calibration point
          MouseAbsolute.move(32768/2, 32768, 0);
        break;

        case 3:
          // Set Offset buffer
          _bottomOffset = (res_y - mouseY);
          // Move move to next calibration point
          MouseAbsolute.move(0, 32768/2, 0);
        break;

        case 4:
          // Set Offset buffer
          _leftOffset = mouseX;
          // Move move to next calibration point
          MouseAbsolute.move(32768, 32768/2, 0);
        break;

        case 5:
          // Set Offset buffer
          _rightOffset = (res_x - mouseX);
          delay(100);
          // Convert Offset buffer to Offset
          topOffset = _topOffset;
          bottomOffset = _bottomOffset;
          leftOffset = _leftOffset;
          rightOffset = _rightOffset;
          // Move move to next calibration point
          MouseAbsolute.move(32768/2, 32768/2, 0);
        break;

        case 6:
          // Apply new Cam center offsets with Offsets applied
          adjX = (OpenFIREpos.testMedianX() - (512 << 2)) * cos(OpenFIREpos.Ang()) - (OpenFIREpos.testMedianY() - (384 << 2)) * sin(OpenFIREpos.Ang()) + (512 << 2);       
          adjY = (OpenFIREpos.testMedianX() - (512 << 2)) * sin(OpenFIREpos.Ang()) + (OpenFIREpos.testMedianY() - (384 << 2)) * cos(OpenFIREpos.Ang()) + (384 << 2);
          OpenFIREper.source(adjX, adjY);                                     // Update Cam centre in perspective library
          OpenFIREper.deinit(0);
          // Break Cali
          cali = 0;
        break;
      }
      } else {}
  }   

  lastButtonState2 = buttonState2;
      
  }


void startCali () {

  buttonState1 = digitalRead(caliPin);

  if (buttonState1 != lastButtonState1) {
    if (buttonState1 == LOW) {
      cali = 1;
    }
    else { // do nothing
    }
    delay(50);
  }
  lastButtonState1 = buttonState1;

}

void mouseButtons() {    // Setup Left, Right & Middle Mouse buttons

  buttonState2 = digitalRead(leftPin);
  buttonState3 = digitalRead(rightPin);
  buttonState4 = digitalRead(middlePin);     

  if (buttonState2 != lastButtonState2) {
    if (buttonState2 == LOW) {
      MouseAbsolute.press(MOUSE_LEFT);
    }
    else {
      MouseAbsolute.release(MOUSE_LEFT);
    }
    delay(10);
  }

  if (buttonState3 != lastButtonState3) {
    if (buttonState3 == LOW) {
      MouseAbsolute.press(MOUSE_RIGHT);
    }
    else {
      MouseAbsolute.release(MOUSE_RIGHT);
    }
    delay(10);
  }

  if (buttonState4 != lastButtonState4) {     
    if (buttonState4 == LOW) {
      MouseAbsolute.press(MOUSE_MIDDLE);
    }
    else {
      MouseAbsolute.release(MOUSE_MIDDLE);
    }
    delay(10);
  }

  lastButtonState2 = buttonState2;
  lastButtonState3 = buttonState3;
  lastButtonState4 = buttonState4;            
}


void PrintResults() {    // Print results for debugging
    
    Serial.print( rawX[0]);
    Serial.print( "," );
    Serial.print( rawY[0]);
    Serial.print( "," );
    Serial.print( rawX[1]);
    Serial.print( "," );
    Serial.print( rawY[1]);
    Serial.print( "," );
    Serial.print( rawX[2]);
    Serial.print( "," );
    Serial.print( rawY[2]);
    Serial.print( "," );
    Serial.print( rawX[3]);
    Serial.print( "," );
    Serial.print( rawY[3]);
    Serial.print( "," );
    Serial.print( mouseX / 4 );
    Serial.print( "," );
    Serial.print( mouseY / 4 );
    Serial.print( "," );
    Serial.print( centerX);
    Serial.print( "," );
    Serial.println( centerY);

}

void startIrCamTimer(int frequencyHz)
{

    rp2040EnablePWMTimer(0, frequencyHz);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, rp2040pwmIrq);
    irq_set_enabled(PWM_IRQ_WRAP, true);

}


void rp2040EnablePWMTimer(unsigned int slice_num, unsigned int frequency)
{
    pwm_config pwmcfg = pwm_get_default_config();
    float clkdiv = (float)clock_get_hz(clk_sys) / (float)(65535 * frequency);
    if(clkdiv < 1.0f) {
        clkdiv = 1.0f;
    } else {
        // really just need to round up 1 lsb
        clkdiv += 2.0f / (float)(1u << PWM_CH1_DIV_INT_LSB);
    }
    
    // set the clock divider in the config and fetch the actual value that is used
    pwm_config_set_clkdiv(&pwmcfg, clkdiv);
    clkdiv = (float)pwmcfg.div / (float)(1u << PWM_CH1_DIV_INT_LSB);
    
    // calculate wrap value that will trigger the IRQ for the target frequency
    pwm_config_set_wrap(&pwmcfg, (float)clock_get_hz(clk_sys) / (frequency * clkdiv));
    
    // initialize and start the slice and enable IRQ
    pwm_init(slice_num, &pwmcfg, true);
    pwm_set_irq_enabled(slice_num, true);
}

void rp2040pwmIrq(void)
{
    pwm_hw->intr = 0xff;
    irPosUpdateTick = 1;
}

void processSerial () {



  while (Serial.available()) {
    char c = Serial.read();
    readString += c;
  }

  if (readString.length() >0) {
    
    //Serial.println(readString);
    category = readString.charAt(0);
    //Serial.println(category);
    switch (category) {
      case 'R':
        value1 = (readString.substring(1,2)).toInt();
        value2 = (readString.substring(3,4)).toInt();
        value3 = (readString.substring(5,9)).toInt();
        Serial.println("R");
        Serial.println(value1);
        Serial.println(value2);
        Serial.println(value3);    
        Serial.println("");           
        break;
      case 'O':
        value1 = (readString.substring(1,2)).toInt();
        value2 = (readString.substring(3,6)).toInt();
        Serial.println("O");
        Serial.println(value1);
        Serial.println(value2);
        Serial.println("");
        break;
      case 'M':
        value1 = (readString.substring(1,2)).toInt();
        value2 = (readString.substring(3,4)).toInt();
        Serial.println("M");
        Serial.println(value1);
        Serial.println(value2);
        Serial.println("");
        break;
      case 'F':
        value1 = (readString.substring(1,2)).toInt();
        value2 = (readString.substring(3,4)).toInt();
        value3 = (readString.substring(5,9)).toInt();
        Serial.println("F");
        Serial.println(value1);
        Serial.println(value2);
        Serial.println(value3);
        Serial.println("");
        break;
    }

    if (readString == "sol"){
        Serial.println("solenoid");
        Serial.println("");

        for(int i = 0; i<=4; i++) {
            digitalWrite(solPin,HIGH);
            delay(solHighDelay);
            digitalWrite(solPin,LOW);
            delay(solLowDelay);
        }
        
    }

    if (readString == "cam"){
        //Serial.println("processing valuess");
        //Serial.println("");

        PrintResults();
        
    }
    
    if (readString == "rum"){
        Serial.println("rumble");
        Serial.println("");
        digitalWrite(rumblePin,HIGH);
        delay(rumbleDelay);
        digitalWrite(rumblePin,LOW);
    }
    if (readString == "getPlayerName"){
        Serial.println(playerName);
        Serial.println("");
    }
        if(readString.indexOf("setPlayerName") >= 0){
        Serial.println("old player name is " + playerName);
        //playerName =
        if(readString.indexOf(" ") >= 13){
        //Serial.print("space is at ");
        //Serial.println(readString.indexOf(" "));
        //Serial.println(readString.substring(readString.indexOf(" ") + 1,readString.length()));
        playerName = (readString.substring(readString.indexOf(" ") + 1,readString.length()));
        Serial.println("new player name is " + playerName);
        }
    }
    
    readString="";
  }

}
