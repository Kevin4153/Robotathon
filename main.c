/**
 * @file ServoExample.c
 * @author Matthew Yu (matthewjkyu@gmail.com)
 * @brief Making a servo move, change direction, and adjust its speed.
 * @version 0.1
 * @date 2021-09-29
 * @copyright Copyright (c) 2021
 */

/** General imports. */
#include <stdlib.h>

/** Device specific imports. */
#include <lib/PLL/PLL.h>
#include <lib/Timer/Timer.h>
#include <raslib/Servo/Servo.h>
#include <raslib/LineSensor/LineSensor.h>
#include <lib/GPIO/GPIO.h>

void EnableInterrupts(void);    // Defined in startup.s
void DisableInterrupts(void);   // Defined in startup.s
void WaitForInterrupt(void);    // Defined in startup.s

#define led_red PIN_F1
#define led_blue PIN_F2
#define led_green PIN_F3



void moveStop(PWM_t servo, PWM_t servo2) {
    ServoSetSpeed(servo, 0);
    ServoSetSpeed(servo2, 0);
    DelayMillisec(5000);
}
//one of the motors is weaker than the other, servo2 speed is less to compensate
void moveForward(PWM_t servo, PWM_t servo2) {
    ServoSetSpeed(servo, -100);     //left motor turns CCW
    ServoSetSpeed(servo2, 90);      //right motor turns CW
    DelayMillisec(1000);
}
void moveBackward(PWM_t servo, PWM_t servo2) {
    ServoSetSpeed(servo, 100);      //left motor turns CW
    ServoSetSpeed(servo2, -100);    //right motor turns CCW
}

//does a veering motion to the left
void turnLeft(PWM_t servo, PWM_t servo2) {
    ServoSetSpeed(servo, 100);      //left motor turns CW
    ServoSetSpeed(servo2, 80);     //right motor turns CW
    DelayMillisec(1000);
}
//does a veering motion to the right
void turnRight(PWM_t servo, PWM_t servo2) {
    ServoSetSpeed(servo, -100);     //left motor turns CCW
    ServoSetSpeed(servo2, -80);    //right motor turns CCW
    DelayMillisec(1000);
}
void turnLeft90(PWM_t servo, PWM_t servo2) {

    ServoSetSpeed(servo, 100);      //left motor turns CW
    ServoSetSpeed(servo2, 100);     //right motor turns CW
    DelayMillisec(5000);


}

void turnRight90(PWM_t servo, PWM_t servo2) {

    ServoSetSpeed(servo, -100);      //left motor turns CCW
    ServoSetSpeed(servo2, -100);     //right motor turns CCW
    DelayMillisec(5000);
}

int main(void) {
    PLLInit(BUS_80_MHZ);
    DisableInterrupts();

    /*
     * Initialize line sensor with 8 pins:
     * PE3, PE2, PE1, PE0, PD3, PD2, PD1, and PD0
     * PD0 corresponds with pin 1 on line sensor module. PD1 is pin 1, so on so forth
     *
     */
    LineSensorConfig_t lineSensConfig = {
        .pins={AIN1, AIN2, AIN3, AIN4, AIN5, AIN6, AIN7, AIN8},
        .numPins=8,
    };
    /* Initialization of ADC. */
        LineSensor_t sensor = LineSensorInit(lineSensConfig);

    /* Red onboard LED. */
    GPIOConfig_t PF1Config = {
        PIN_F1,
        GPIO_PULL_DOWN,
        true
    };
    /* Initialize PF2 as a GPIO output. This is associated with the BLUE led on
       the TM4C. */
    GPIOConfig_t PF2Config = {
        PIN_F2,
        GPIO_PULL_DOWN,
        true
    };

    /* Initialize PF3 as a GPIO output. This is associated with the GREEN led on
       the TM4C. */
    GPIOConfig_t PF3Config = {
        PIN_F3,
        GPIO_PULL_DOWN,
        true
    };
    GPIOInit(PF1Config);
    GPIOInit(PF2Config);
    GPIOInit(PF3Config);

    //left motor
    ServoConfig_t servo1Config = {
        .pin=PIN_B6,
        .timerID=TIMER_0A
    };
    PWM_t servo = ServoInit(servo1Config);

    //right motor
    ServoConfig_t servo2Config = {
            .pin=PIN_B1,
            .timerID=TIMER_1A
        };
    PWM_t servo2 = ServoInit(servo2Config);

    DelayInit();
    EnableInterrupts();
    while(1) {
        /* Read from the line sensor. */

        /* Read from the line sensor again, but this time using a threshold.
           This threshold corresponds to 2048 / 4095 * 3.3 V. */
        LineSensorGetBoolArray(&sensor, 2048);

        uint8_t avgSide = 0;
        uint8_t i;
        for (i = 0; i < 8; ++i) {
            avgSide += sensor.values[i] << i;
        }

        /* UNCOMMENT THIS CODE LATER *********** */
        //if all sensors on the left are true, turn left 90 degrees
//        if (sensor.values[7] && sensor.values[6] && sensor.values[5] && sensor.values[4]) {
//            GPIOSetBit(led_red, 0);
//            GPIOSetBit(led_blue, 0);
//            GPIOSetBit(led_green, 1);
//            turnLeft90(servo, servo2);
//        }
//
//        //if all sensors on the right are true, turn right 90 degrees
//        if (sensor.values[3] && sensor.values[2] && sensor.values[1] && sensor.values[0]) {
//            GPIOSetBit(led_red, 0);
//            GPIOSetBit(led_blue, 1);
//            GPIOSetBit(led_green, 0);
//            turnRight90(servo, servo2);
//        }

        /* Turn on RED LED if sensor data is none across the board. */
        /* Move forward if there is no sensor data */
        if (avgSide == 0) {
            GPIOSetBit(led_red, 1);
            GPIOSetBit(led_blue, 0);
            GPIOSetBit(led_green, 0);
            moveForward(servo, servo2);
//            moveStop(servo, servo2);
        }
        /* Turn on GREEN LED if sensor data is tending towards the left side. */
        /* Turn left if sensor data is towards left side */
        else if (avgSide >= 0x10) {
            GPIOSetBit(led_red, 0);
            GPIOSetBit(led_blue, 0);
            GPIOSetBit(led_green, 1);
            turnLeft(servo, servo2);
        }
        /* Turn on BLUE LED if sensor data is tending towards the right side. */
        /* Turn right if sensor data is towards right side */
        else {
            GPIOSetBit(led_red, 0);
            GPIOSetBit(led_blue, 1);
            GPIOSetBit(led_green, 0);
            turnRight(servo, servo2);
        }

          /* trying to figure out how long to delayto do 90 degree turn */

//            //turn left 90
//            ServoSetSpeed(servo, 100);      //left motor turns CW
//            ServoSetSpeed(servo2, 100);     //right motor turns CW
//            DelayMillisec(2500);
//
//            ServoSetSpeed(servo, 0);
//            ServoSetSpeed(servo2, 0);
//            DelayMillisec(10000);
//
//            //turn right 90
//            ServoSetSpeed(servo, -100);      //left motor turns CCW
//            ServoSetSpeed(servo2, -100);     //right motor turns CCW
//            DelayMillisec(2500);
//            ServoSetSpeed(servo, 0);
//            ServoSetSpeed(servo2, 0);
//            DelayMillisec(10000);

    }
}
