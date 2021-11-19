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


void EnableInterrupts(void);
void DisableInterrupts(void);
void WaitForInterrupt(void);

#define led_red     PIN_F1
#define led_blue    PIN_F2
#define led_green   PIN_F3

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
 * red: forward, white: backward
 * green: sharp left, blue: sharp right
 * yellow: veer left, purple: veer right
 */
#define forward     0
#define left90      1
#define veerLeft    2
#define veerRight   3
#define right90     4

#define distSens_thresh_front   3900
#define distSens_thresh_left    3900

#define forwardAdjustTime   200
#define defaultAdjustTime   200

/* Global Variables */
int global_delay = 0; //delay used for motor movement times
int game_mode = 1; //flag indicating which game mode we are in

void moveStop(PWM_t servo, PWM_t servo2) {
    GPIOSetBit(led_red, 0); // no led for stop
    GPIOSetBit(led_blue, 0);
    GPIOSetBit(led_green, 0);
    ServoSetSpeed(servo, 35);
    ServoSetSpeed(servo2, 30);
    DelayMillisec(200);
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

// ***********CHANGE THE SERVO TO THE THIRD SERVO FOR THE SHOOTY PART ******
void shootMotor(PWM_t servo) {
    ServoSetSpeed(servo, 100);
    DelayMillisec(1000);
}
int getLineResult(LineSensor_t sensor, PWM_t servo, PWM_t servo2){
    LineSensorGetIntArray(&sensor);
    LineSensorGetBoolArray(&sensor, 2048);
    //        uint8_t avgSide = 0;
    //        uint8_t i;
    //        for (i = 0; i < 8; ++i) {
    //            avgSide += sensor.values[i] << i;
    //        }


//    // detected
//    int leftHalf = sensor.values[0]+sensor.values[1]+sensor.values[2]+sensor.values[3];
//    int rightHalf = sensor.values[4]+sensor.values[5]+sensor.values[6]+sensor.values[7];
//    if(leftHalf>=3 && rightHalf>=3){
//        // at a cross confirmed
//
//
//    }
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

void lineSensing (LineSensor_t sensor, PWM_t servo, PWM_t servo2) {
    while (1) {
        /* get line sensor value and convert it to boolean value */
        LineSensorGetIntArray(&sensor);
        LineSensorGetBoolArray(&sensor, 2048);

        uint8_t avgSide = 0;
        uint8_t i;
        for (i = 0; i < 8; ++i) {
            avgSide += sensor.values[i] << i;
        }

        /* Turn on RED LED if sensor data is none across the board. */
        /* Move forward if there is no sensor data */
        if (avgSide == 0) {
            moveForward(servo, servo2);
//            moveStop(servo, servo2);
        }

        /* UNCOMMENT THIS CODE LATER *********** */
        //if all sensors on the left are true, turn left 90 degrees
        //avgSide >= 0b01000000 or 0x40 ---> turnLeft90
        //avgSide <= 0b00000010 or 0x02 ---> turnRight90
        else if (sensor.values[6] + sensor.values[7] == 2) {
//            moveStop(servo, servo2);
            turnLeft90(servo, servo2);
//            moveStop(servo, servo2);
            moveForward(servo, servo2);
        }
        //if all sensors on the right are true, turn right 90 degrees
        else if (sensor.values[1] + sensor.values[0] == 2) {
//            moveStop(servo, servo2);
            turnRight90(servo, servo2);
//            moveStop(servo, servo2);
            moveForward(servo, servo2);
        }

        /* Turn on GREEN LED if sensor data is tending towards the left side. */
        /* Turn left if sensor data is towards left side */
        else if (avgSide >= 0x10) {
            turnLeft(servo, servo2);
        }
        /* Turn on BLUE LED if sensor data is tending towards the right side. */
        /* Turn right if sensor data is towards right side */
        else if (avgSide <= 0x10) {
            global_delay = 15;
            turnRight(servo, servo2);
            global_delay = 50;
        }
    }
}

void distanceSensing(DistanceSensor_t frontSensor, DistanceSensor_t leftSensor, PWM_t servo, PWM_t servo2) {
    while (1) {
        /* Read from front and left distance sensor */
        DistanceSensorGetInt(&frontSensor);
        DistanceSensorGetInt(&leftSensor);

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
            moveForward(servo, servo2);
        }
    }

}

void shooting (PWM_t servo, PWM_t servo2) {

}
int main(void) {

    PLLInit(BUS_80_MHZ);
    DisableInterrupts();

//    /* Front sensor initialization */
//    /* pin PE4 is associated with frontSensor */
//    DistanceSensorConfig_t frontSensConfig = {
//            .pin=AIN9,
//            .module=ADC_MODULE_0
//        };
//    DistanceSensor_t frontSensor = DistanceSensorInit(frontSensConfig);
//
//    /* Left sensor initialization */
//    /* pin PB5 is associated with leftSensor */
//    DistanceSensorConfig_t leftSensConfig = {
//            .pin=AIN11,
//            .module=ADC_MODULE_1
//        };
//    DistanceSensor_t leftSensor = DistanceSensorInit(leftSensConfig);

    /*
     * Initialize line sensor with 8 pins:
     * linesensorconfig array ===AN0, AN1, AN2, ....AN7 =
     * sensor.val[0], sensor.val[1], ..... sensor.val[7] =
     * PE3, PE2, PE1, PE0, PD3, PD2, PD1, PE5 =
     * line sensor pin1, pin 2, pin 3, ..... pin 8
     *
     *
     */
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

    /* Left motor initialization */
    ServoConfig_t servo1Config = {
        .pin=PIN_B6,
        .timerID=TIMER_0A
    };
    PWM_t servo = ServoInit(servo1Config);

    /* Right motor initialization */
    ServoConfig_t servo2Config = {
            .pin=PIN_B1,
            .timerID=TIMER_1A
        };
    PWM_t servo2 = ServoInit(servo2Config);

    DelayInit();
    EnableInterrupts();
    global_delay = 50;

    //************* test script for motor **************
//    testMotorBehavior(sensor, servo, servo2);    // comment this out as needed
    //**************************************************

    //wait a total of 5 seconds before starting
//    moveStop(servo, servo2);
//    DelayMillisec(3000);

    while(1) {

/*------------Wall Detection Maze------------
 * Make sure front and left sensor are using different ADC modules
 * and that no other component is using them.
 * Robot constantly moves forward unless it detects wall on left or
 *  in front and left.
 */
//        if (game_mode == mode_wall) {
//            distanceSensing(frontSensor, leftSensor, servo, servo2);
//        }
/*------------Line Sensing and Color Tiles------------*/
        if (game_mode == mode_line) {
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
                default: moveBackward(servo, servo2); break;
            }
        }
///*------------Shooting Game------------*/
//        if (game_mode == mode_shoot) {
//
//        }











    }
}
