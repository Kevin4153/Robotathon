/** General imports. */
#include <stdlib.h>
#include <stdio.h>
/** Device specific imports. */
#include <lib/PLL/PLL.h>
#include <lib/Timer/Timer.h>
#include <lib/GPIO/GPIO.h>
#include <raslib/Servo/Servo.h>
#include <raslib/LineSensor/LineSensor.h>
#include <raslib/DistanceSensor/DistanceSensor.h>
#include <raslib/ColorSensor/ColorSensor.h>
#include <lib/I2C/I2C.h>



void EnableInterrupts(void);
void DisableInterrupts(void);
void WaitForInterrupt(void);

#define led_red     PIN_F1
#define led_blue    PIN_F2
#define led_green   PIN_F3
#define button_Y    PIN_C5
#define button_G    PIN_C6
#define button_B    PIN_C7


/*
 * flags for switching game modes:
 * mode_wall = Pacman Maze
 * mode_line = Dance Dance Revolution
 * mode_shoot = Duck Shooting Game
 */
#define mode_wall   0
#define mode_line   1
#define mode_shoot  2
/*
 * red: forward, white: shooting motor
 * green: sharp left, blue: sharp right
 * yellow: veer left, purple: veer right
 */
#define forward     0
#define left90      1
#define veerLeft    2
#define veerRight   3
#define right90     4
#define stop        5
#define cross       6

#define distSens_thresh_front   3900
#define distSens_thresh_left    3900

#define forwardAdjustTime   200
#define defaultAdjustTime   200

/* Global Variables */
int global_delay = 0; //delay used for motor movement times
int game_mode = mode_line; //flag indicating which game mode we are in

void moveStop(PWM_t servo, PWM_t servo2) {
    GPIOSetBit(led_red, 0); // no led for stop
    GPIOSetBit(led_blue, 0);
    GPIOSetBit(led_green, 0);
    ServoSetSpeed(servo, 35);
    ServoSetSpeed(servo2, 30);
    DelayMillisec(1000);
//    DelayMillisec(1000);
}
//one of the motors is weaker than the other, servo2 speed is less to compensate
void moveForward(PWM_t servo, PWM_t servo2) {
    GPIOSetBit(led_red, 1); // red led for moving forward
    GPIOSetBit(led_blue, 0);
    GPIOSetBit(led_green, 0);
    ServoSetSpeed(servo, -80);     //left motor turns CCW -80
    ServoSetSpeed(servo2, 84);      //right motor turns CW 100
    DelayMillisec(global_delay);
}

void moveForward_t(PWM_t servo, PWM_t servo2, int delayTime) {
    GPIOSetBit(led_red, 1); // red led for moving forward
    GPIOSetBit(led_blue, 0);
    GPIOSetBit(led_green, 0);
    ServoSetSpeed(servo, -100);     //left motor turns CCW
    ServoSetSpeed(servo2, 100);      //right motor turns CW
    DelayMillisec(delayTime);
}

void moveBackward(PWM_t servo, PWM_t servo2) {
    GPIOSetBit(led_red, 1); // white led for moving backward
    GPIOSetBit(led_blue, 1);
    GPIOSetBit(led_green, 1);
    ServoSetSpeed(servo, 100);      //left motor turns CW
    ServoSetSpeed(servo2, -100);    //right motor turns CCW
    DelayMillisec(global_delay);
}

//does a veering motion to the left
void turnLeft(PWM_t servo, PWM_t servo2) {
    GPIOSetBit(led_red, 1); // yellow led on for turning left
    GPIOSetBit(led_blue, 0);
    GPIOSetBit(led_green, 1);
    ServoSetSpeed(servo, 12);      //left motor turns CW 12
    ServoSetSpeed(servo2, 77);     //right motor turns CW 77
//    DelayMillisec(2000); // this delay is for testing veering motion
    DelayMillisec(global_delay); //use this delay for testing straight line tracking
}
//does a veering motion to the right
void turnRight(PWM_t servo, PWM_t servo2) {
    GPIOSetBit(led_red, 1); // led purple for veering right
    GPIOSetBit(led_blue, 1);
    GPIOSetBit(led_green, 0);
    ServoSetSpeed(servo, -80);     //left motor turns CCW 80
    ServoSetSpeed(servo2, 70);    //right motor turns CCW 70
//    DelayMillisec(2000); //this delay is for testing veering motion
    DelayMillisec(global_delay); //use this delay for testing straight line tracking
}
void turnLeft90(PWM_t servo, PWM_t servo2) {
    GPIOSetBit(led_red, 0); // led green for sharp left
    GPIOSetBit(led_blue, 0);
    GPIOSetBit(led_green, 1);
    ServoSetSpeed(servo, 100);      //left motor turns CW
    ServoSetSpeed(servo2, 100);     //right motor turns CW
    DelayMillisec(550);
}

void turnRight90(PWM_t servo, PWM_t servo2) {
    GPIOSetBit(led_red, 0); // led blue for sharp right
    GPIOSetBit(led_blue, 1);
    GPIOSetBit(led_green, 0);
    ServoSetSpeed(servo, -100);      //left motor turns CCW
    ServoSetSpeed(servo2, -100);     //right motor turns CCW
    DelayMillisec(505);
}

// Releases rubber band that will propel the ping pong ball
//380 ms is 90 degrees
//740 ms is 180 degrees
void shootMotorRelease(PWM_t servo3) {
    GPIOSetBit(led_red, 1); // led white for shooting motor
    GPIOSetBit(led_blue, 1);
    GPIOSetBit(led_green, 1);
    /*turn motor 180~ degrees */
    ServoSetSpeed(servo3, 15);
    DelayMillisec(1480);
    ServoSetSpeed(servo3, 100);
}

// Retracts servo wing so we can prepare next shot
void shootMotorRetract(PWM_t servo3) {
    /*turn motor 135~ degrees */
    ServoSetSpeed(servo3, 15);
    DelayMillisec(570);
    ServoSetSpeed(servo3, 100);
}

// Stops shooting motor from turning
void shootMotorStop(PWM_t servo3) {
    GPIOSetBit(led_red, 1); //led red for stopping
    GPIOSetBit(led_blue, 0);
    GPIOSetBit(led_green, 0);
    ServoSetSpeed(servo3, 100);
    DelayMillisec(14000);
    ServoSetSpeed(servo3, 15);

}
void set_LED_color(int red, int green, int blue, int delayMS){
    GPIOSetBit(led_red, red>=0x10? 1:0);
    GPIOSetBit(led_green, green>=0x10? 1:0);
    GPIOSetBit(led_blue, blue>=0x10? 1:0);

/*
    uint8_t R = (red+1), G = (green+1), B = (blue+1);
    uint8_t cR=0, cG=0, cB=0;
    uint8_t limit = 0x50; // decimal 64
    int i;
    for(i=0; i<delayMS/limit; i++){
        // every milisecond see if certain led needs to be on and off. very ridiculous PWM implementation
        int j;
        for(j=0; j<limit; j++){
            GPIOSetBit(led_red, cR<R? 1:0);
            GPIOSetBit(led_green, cG<G? 1:0);
            GPIOSetBit(led_blue, cB<B? 1:0);
            cR++; cG++; cB++;
            cR = cR>=limit? cR:1;
            cG = cG>=limit? cG:1;
            cB = cB>=limit? cB:1;
            DelayMillisec(1);
        }
    }
*/

    /*
    if(red>=0x10) GPIOSetBit(led_red, 1);
    else GPIOSetBit(led_red, 0);
    if(red>=0x10) GPIOSetBit(led_green, 1);
    else GPIOSetBit(led_green, 0);
    if(red>=0x10) GPIOSetBit(led_blue, 1);
    else GPIOSetBit(led_blue, 0);
     */
}

void respondTileColor(ColorSensor_t sensor, PWM_t servo, PWM_t servo2){
    ColorSensorSample(&sensor);
    int R = sensor.RedValue, G = sensor.GreenValue, B = sensor.BlueValue;
    set_LED_color(R, G, B, 1000);

    /* turns on red LED if highest RGB value is red (i.e. a red object is placed in front of sensor) */
    if(R>(G*3/2) && R>(B*3/2)){
        // turn right
        turnRight90(servo, servo2);
    }

    /* turns on blue LED if highest RGB value is blue (i.e. a blue object is placed in front of sensor) */
    else if(B>(G*5/4) && B>(R*3/2)){
        // turn left
        turnLeft90(servo, servo2);
    }

    /* turns on green LED if highest RGB value is green (i.e. a green object is placed in front of sensor) */
    else if(G>(R*3/2) && G>(B*3/2)){ /* do nothing lmao */}

    /* if green is very close to red and both red and green are higher than blue, color is yellow*/
    else /* if((G<(R+0x20) || R<(G+0x20)) && R>(B+0x10) && G>(B+0x10)) */{
        // 360 degrees turn
        int i;
        for(i=0; i<4; i++) turnRight90(servo, servo2);
    }

//    else set_LED_color(255,255,255, 1000);

    int temp = global_delay;
    global_delay = 1500;
    moveForward(servo, servo2); // move forward a bit at the end to get out of cross zone
    global_delay = temp;
}

int getLineResult(LineSensor_t sensor, PWM_t servo, PWM_t servo2){
    LineSensorGetIntArray(&sensor);
    LineSensorGetBoolArray(&sensor, 2048);

    // try detect cross
    int leftHalf = sensor.values[0]+sensor.values[1]+sensor.values[2]+sensor.values[3];
    int rightHalf = sensor.values[4]+sensor.values[5]+sensor.values[6]+sensor.values[7];
    // at a cross
    if(leftHalf>=3 && rightHalf>=3) return cross;

    // detected left 90 degrees turn. move forward a little before returning direction
    // mostly for adjustment
    if(sensor.values[1] + sensor.values[0] == 2) return right90;

    // detected right 90 degrees turn. move forward a little before returning direction
    // mostly for adjustment
    if(sensor.values[6] + sensor.values[7] == 2) return left90;

    if((sensor.values[2] + sensor.values[3] == 2)) return veerRight;
    if(sensor.values[1] + sensor.values[2] == 2) return veerRight;
//    if(sensor.values[2] || sensor.values[3]) return veerRight;
    if(sensor.values[4] + sensor.values[5] == 2) return veerLeft;
    if(sensor.values[6] + sensor.values[5] == 2) return veerLeft;
//    if(sensor.values[4] || sensor.values[5]) return veerLeft;

    return forward;
}
/*
int getLineResult(LineSensor_t sensor, PWM_t servo, PWM_t servo2){
    LineSensorGetIntArray(&sensor);
    LineSensorGetBoolArray(&sensor, 2048);
    //        uint8_t avgSide = 0;
    //        uint8_t i;
    //        for (i = 0; i < 8; ++i) {
    //            avgSide += sensor.values[i] << i;
    //        }


    // if detected line on most of left side and most of right side or a cross, stop motor
    int leftHalf = sensor.values[0]+sensor.values[1]+sensor.values[2]+sensor.values[3];
    int rightHalf = sensor.values[4]+sensor.values[5]+sensor.values[6]+sensor.values[7];
    if(leftHalf>=3 && rightHalf>=3) return stop;
    // detected left 90 degrees turn. move forward a little before returning direction
    // mostly for adjustment
    if(sensor.values[1] + sensor.values[0] == 2) return right90;

    // detected right 90 degrees turn. move forward a little before returning direction
    // mostly for adjustment
    if(sensor.values[6] + sensor.values[7] == 2) return left90;

    if((sensor.values[2] + sensor.values[3] == 2)) return veerRight;
    if(sensor.values[1] + sensor.values[2] == 2) return veerRight;
//    if(sensor.values[2] || sensor.values[3]) return veerRight;
    if(sensor.values[4] + sensor.values[5] == 2) return veerLeft;
    if(sensor.values[6] + sensor.values[5] == 2) return veerLeft;
//    if(sensor.values[4] || sensor.values[5]) return veerLeft;

    return forward;
}
*/

void testMotorBehavior(LineSensor_t sensor, PWM_t servo, PWM_t servo2){
    int stage = 0;
    while(1){

        int direction = getLineResult(sensor, servo, servo2);

    /*
     * the robot should:
     *      stop for 2 seconds,
     *      forward 2 seconds,
     *      backward 2 seconds(back to the same place),
     *      veer left 2 seconds,
     *      veer right 2 seconds,
     *      90 degrees turn left,
     *      90 degrees turn right
     *      repeat
     */
//        moveStop(servo,servo2);
//        turnLeft90(servo, servo2);
//        moveStop(servo, servo2);
//        turnRight90(servo, servo2);
        switch(stage){
//            case 0: moveStop(servo, servo2); break;
//            case 1: moveForward(servo, servo2); break;
//            case 2: moveBackward(servo, servo2); break;
//            case 3: turnLeft(servo, servo2); break;
//            case 4: turnRight(servo, servo2); break;
//            case 5: turnLeft90(servo, servo2); break;
//            case 6: turnRight90(servo, servo2); break;
//            default: moveStop(servo, servo2);

            //for just testing veering motion
              case 0: moveStop(servo, servo2); break;
              case 1: turnLeft90(servo, servo2); break;
              case 2: moveStop(servo, servo2); break;
              case 3: turnRight90(servo, servo2); break;
              default: turnRight90(servo, servo2);
        }
        stage++;
//        (stage) %= 7; // 0 <= stage < 7
//        for just testing veering motion
        stage %= 4;
    }
}

void lineSensing (LineSensor_t sensor, ColorSensor_t color_sensor, PWM_t servo, PWM_t servo2) {
    while (1) {
        int direction = getLineResult(sensor, servo, servo2);

        switch(direction){
            case forward: moveForward(servo, servo2); break;
            case veerLeft: turnLeft(servo, servo2); break;
            case veerRight:turnRight(servo, servo2); break;
            case right90:
                moveForward_t(servo, servo2, forwardAdjustTime);
                turnRight90(servo, servo2);
                break;
            case left90:
                moveForward_t(servo, servo2, forwardAdjustTime);
                turnLeft90(servo, servo2);
                break;
            case stop:
                moveStop(servo, servo2); break;
            case cross: respondTileColor(color_sensor, servo, servo2); break;
            default: moveBackward(servo, servo2); break;
        }
    }
}

void distanceSensing(DistanceSensor_t frontSensor, DistanceSensor_t leftSensor, PWM_t servo, PWM_t servo2) {
    int safeZone_inner = 3950, safeZone_outter = 3850;
    int leftDist = 0;
//    while (1) {
        /* Read from front and left distance sensor */
        DistanceSensorGetInt(&frontSensor);
        DistanceSensorGetInt(&leftSensor);

        leftDist = leftSensor.value;

        /* convert int value of sensor into a boolean value */
        /* ********* FIGURE OUT THRESHOLD VALUE FOR WALL DETECTION *********** */
        DistanceSensorGetBool(&frontSensor, distSens_thresh_front);
        DistanceSensorGetBool(&leftSensor, distSens_thresh_left);

        /* if the front and left sensor are detecting a wall, turn right 90 degrees */
        if (frontSensor.value == 1 && leftSensor.value == 1) {
            turnRight90(servo, servo2);
        }
        /* if only the front sensor is detecting a wall, turn left 90 degrees */
        else if (frontSensor.value == 1) {
            turnLeft90(servo, servo2);
        }
//        /* if only the left sensor is detecting a wall, veer right a little bit */
//        if (leftSensor.value == 1) {
//            global_delay = 50;
//            turnRight(servo, servo2);
//        }
        else {
            int time_adjustAmount = 50;             // CHANGE THIS AS WE CALIBRATE IDEAL DISTANCE
            int oldDelay = global_delay;            // save the old global delay
            /*if(leftDist > safeZone_inner){          // if too close for comfort, drift right
                global_delay = time_adjustAmount;   // change the global delay to the needed amount for precise adjustment
                turnRight(servo, servo2);
            }
            else if(leftDist < safeZone_outter){    // when too far from left wall, drift left
                global_delay = time_adjustAmount;
                turnLeft(servo, servo2);
            }
            else*/ moveForward(servo, servo2);
            global_delay = oldDelay;                // restore the global delay
        }
    }

//}

void shooting (PWM_t servo3) {
//    while (1) {

        shootMotorRelease(servo3);
        shootMotorStop(servo3);

//    }
}

int gamemodeSelection(){
    while(1){
        int y = GPIOGetBit(button_Y);
        int g = GPIOGetBit(button_G);
        int b = GPIOGetBit(button_B);
        if(y) return mode_wall;
        if(g) return mode_line;
        if(b) return mode_shoot;
        DelayMillisec(1);
    }
}

int main(void) {

    PLLInit(BUS_80_MHZ);
    DisableInterrupts();



//------------On-board LED Init------------
    /* Red onboard LED. */
    GPIOConfig_t PF1Config = {
        PIN_F1,
        GPIO_PULL_DOWN,
        true
    };
    /* Blue onboard LED */
    GPIOConfig_t PF2Config = {
        PIN_F2,
        GPIO_PULL_DOWN,
        true
    };

    /* Green onboard LED */
    GPIOConfig_t PF3Config = {
        PIN_F3,
        GPIO_PULL_DOWN,
        true
    };
    GPIOInit(PF1Config);
    GPIOInit(PF2Config);
    GPIOInit(PF3Config);

//------------Motor Init------------
    /* Left motor initialization */
    ServoConfig_t servo1Config = {
        .pin=M0_PB6,
    };
    PWM_t servo = ServoInit(M0_PB6);

    /* Right motor initialization */
    ServoConfig_t servo2Config = {
        .pin=M0_PB7,
    };
    PWM_t servo2 = ServoInit(M0_PB7);

//------------Color sensor Init------------
    /* I2C configuration for the color sensor*/
        I2CConfig_t i2ccon =  {
            .module=I2C_MODULE_0, // This uses pins PB2 (SCL) and PB3 (SDA).
            .speed=I2C_SPEED_400_KBPS //baud rate of 400 kilobits per second
        };
        /* Initialize color sensor */
        ColorSensorConfig_t color_config = {
            .I2CConfig= i2ccon,
            .isInterrupt=false, //use interrupt
            .samplingFrequency = 100, //200 hz
            .timerID=TIMER_4A //timer 1A
        };
        ColorSensor_t color_sensor = ColorSensorInit(color_config);


    /* Shooting motor initialization */
    ServoConfig_t servo3Config = {
        .pin=PIN_C4,
    }; 
    PWM_t servo3 = ServoInit(M0_PC4);

//------------Button Init------------
    /* yellow button: PC5 */
    GPIOConfig_t PC5Config = {
        button_Y,
        GPIO_PULL_DOWN,
        false
    };
    /* green button: PC6 */
    GPIOConfig_t PC6Config = {
        button_G,
        GPIO_PULL_DOWN,
        false
    };
    /* blue button: PC7 */
    GPIOConfig_t PC7Config = {
        button_B,
        GPIO_PULL_DOWN,
        false
    };
    GPIOInit(PC5Config);
    GPIOInit(PC6Config);
    GPIOInit(PC7Config);

    DelayInit();
    EnableInterrupts();
    global_delay = 50;

    //************* test script for motor **************
//    testMotorBehavior(sensor, servo, servo2);    // comment this out as needed
    //**************************************************

//    while(1){
//        respondTileColor(color_sensor, servo, servo2);
//        DelayMillisec(1000);
//
//    }

    game_mode = gamemodeSelection();

//------------Distance Sensor Init------------
    /* Front sensor initialization */
    /* pin PE4 is associated with frontSensor */
    //DistanceSensor_t leftSensor, frontSensor;

    if(game_mode == mode_wall){

    }

    //------------Line Sensor Init------------
    /*
     * Initialize line sensor with 8 pins:
     * linesensorconfig array ===AN0, AN1, AN2, ....AN7 =
     * sensor.val[0], sensor.val[1], ..... sensor.val[7] =
     * PE3, PE2, PE1, PE0, PD3, PD2, PD1, PE5 =
     * line sensor pin1, pin 2, pin 3, ..... pin 8
     *
     *
     */
    //LineSensor_t sensor;
    if(game_mode == mode_line){

    }

//    if(game_mode == mode_wall){
//        GPIOSetBit(led_red, 1); //led red for stopping
//            GPIOSetBit(led_blue, 0);
//            GPIOSetBit(led_green, 1);
//    }
//    else if(game_mode == mode_line){
//        GPIOSetBit(led_red, 0); //led red for stopping
//            GPIOSetBit(led_blue, 0);
//            GPIOSetBit(led_green, 0);
//    }
//    else if(game_mode == mode_shoot){
//        GPIOSetBit(led_red, 0); //led red for stopping
//            GPIOSetBit(led_blue, 1);
//            GPIOSetBit(led_green, 0);
//    }
//    else{
//        GPIOSetBit(led_red, 0); //led red for stopping
//        GPIOSetBit(led_blue, 0);
//        GPIOSetBit(led_green, 0);
//    }
//    DelayMillisec(1000);




/*------------Line Sensing and Color Tiles------------*/
      if (game_mode == mode_line) {
          LineSensorConfig_t lineSensConfig = {
              .pins={AIN0, AIN1, AIN2, AIN3, AIN4, AIN5, AIN6, AIN7},
              .numPins=8,
              .repeatFrequency=20,
              .isThresholded=true,
              .threshold=2048, // This threshold corresponds to 2048 / 4095 * 3.3 V.
              .module=ADC_MODULE_1
              // Uses ADC Module 1, Sequencer 0, Timer 0A by default.
          };
          /* Initialization of ADC */
          LineSensor_t sensor = LineSensorInit(lineSensConfig);
//          lineSensing(sensor, color_sensor, servo, servo2);
          while (1) {
              int direction = getLineResult(sensor, servo, servo2);

              switch(direction){
                  case forward: moveForward(servo, servo2); break;
                  case veerLeft: turnLeft(servo, servo2); break;
                  case veerRight:turnRight(servo, servo2); break;
                  case right90:
                      moveForward_t(servo, servo2, forwardAdjustTime);
                      turnRight90(servo, servo2);
                      break;
                  case left90:
                      moveForward_t(servo, servo2, forwardAdjustTime);
                      turnLeft90(servo, servo2);
                      break;
                  case stop:
                      moveStop(servo, servo2); break;
                  case cross: respondTileColor(color_sensor, servo, servo2); break;
                  default: moveBackward(servo, servo2); break;
              }
          }
      }

/*------------Wall Detection Maze------------*/
        if (game_mode == mode_wall) {
            DistanceSensorConfig_t frontSensConfig = {
                .pin=AIN9,
                .module=ADC_MODULE_0
            };
            DistanceSensor_t frontSensor = DistanceSensorInit(frontSensConfig);

            /* Left sensor initialization */
            /* pin PB5 is associated with leftSensor */
            DistanceSensorConfig_t leftSensConfig = {
                .pin=AIN11,
                .module=ADC_MODULE_1
            };
            DistanceSensor_t leftSensor = DistanceSensorInit(leftSensConfig);

            while (1) {
//            distanceSensing(frontSensor, leftSensor, servo, servo2);
                int safeZone_inner = 3950, safeZone_outter = 3850;
                int leftDist = 0;
        //    while (1) {
                /* Read from front and left distance sensor */
                DistanceSensorGetInt(&frontSensor);
                DistanceSensorGetInt(&leftSensor);

                leftDist = leftSensor.value;

                /* convert int value of sensor into a boolean value */
                /* ********* FIGURE OUT THRESHOLD VALUE FOR WALL DETECTION *********** */
                DistanceSensorGetBool(&frontSensor, distSens_thresh_front);
                DistanceSensorGetBool(&leftSensor, distSens_thresh_left);

                /* if the front and left sensor are detecting a wall, turn right 90 degrees */
                if (frontSensor.value == 1 && leftSensor.value == 1) {
                    turnRight90(servo, servo2);
                }
                /* if only the front sensor is detecting a wall, turn left 90 degrees */
                else if (frontSensor.value == 1) {
                    turnLeft90(servo, servo2);
                }
        //        /* if only the left sensor is detecting a wall, veer right a little bit */
        //        if (leftSensor.value == 1) {
        //            global_delay = 50;
        //            turnRight(servo, servo2);
        //        }
                else {
                    int time_adjustAmount = 50;             // CHANGE THIS AS WE CALIBRATE IDEAL DISTANCE
                    int oldDelay = global_delay;            // save the old global delay
                    /*if(leftDist > safeZone_inner){          // if too close for comfort, drift right
                        global_delay = time_adjustAmount;   // change the global delay to the needed amount for precise adjustment
                        turnRight(servo, servo2);
                    }
                    else if(leftDist < safeZone_outter){    // when too far from left wall, drift left
                        global_delay = time_adjustAmount;
                        turnLeft(servo, servo2);
                    }
                    else*/ moveForward(servo, servo2);
                    global_delay = oldDelay;                // restore the global delay
                }
            }
        }


///*------------shooting game------------*/
        if (game_mode == mode_shoot) {
            while(1) {
            shootMotorRelease(servo3);
            shootMotorStop(servo3);
            }
        }
        GPIOSetBit(led_red, 1); //led red for stopping
        GPIOSetBit(led_blue, 1);
        GPIOSetBit(led_green, 1);
        DelayMillisec(3000);


}

