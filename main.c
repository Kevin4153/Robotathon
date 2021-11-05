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


void EnableInterrupts(void);    // Defined in startup.s
void DisableInterrupts(void);   // Defined in startup.s
void WaitForInterrupt(void);    // Defined in startup.s


int main(void) {
    PLLInit(BUS_80_MHZ);
    DisableInterrupts();

    /*
     * Initialize line sensor with 8 pins:
     * PE3, PE2, PE1, PE0, PD3, PD2, PD1, and PD0
     * PD0 corresponds with pin 1 on line sensor module. PD1 is pin 1, so on so forth
     * Using TIMER0A at 20 HZ
     */
    LineSensorConfig_t config = {
            .pins={AIN1, AIN2, AIN3, AIN4, AIN5, AIN6, AIN7, AIN8},
            .numPins=8,
            .repeatFrequency=20,
            .isThresholded=true,
            .threshold=2048 // This threshold corresponds to 2048 / 4095 * 3.3 V.
            // Uses ADC Module 0, Sequencer 0, Timer 0A by default.
        };

    /* Initialization of ADC. */
        LineSensor_t sensor = LineSensorInit(config);

    DelayInit();

    /* Red onboard LED. */
    GPIOConfig_t PF1 = {
        .pin=PIN_F1,
        .isOutput=true
    };
    GPIOInit(PF1);
    /* Blue onboard LED. */
    GPIOConfig_t PF2 = {
        .pin=PIN_F2,
        .isOutput=true
    };
    GPIOInit(PF2);

    /* Warning to users who have epilepsy - bright flashing colors. */
    //left motor
    ServoConfig_t config = {
        .pin=PIN_B6,
        .timerID=TIMER_0A
    };

    /* Initialize PF3 as a GPIO output. This is associated with the GREEN led on
       the TM4C. */
    GPIOConfig_t PF3Config = {
        PIN_F3,
        GPIO_PULL_DOWN,
        true
    };

    PWM_t servo = ServoInit(config);

    //right motor
    ServoConfig_t config2 = {
            .pin=PIN_B1,
            .timerID=TIMER_1A
        };
    PWM_t servo2 = ServoInit(config2);

    /* Put all of these functions in a separate file for cleanliness or smtn*/

    void moveStop() {
        /* old stop moving code */
        /* Make the servo stall for 5 seconds. This should be not moving.
                           To tune your servo, you want to turn the front screw until the servo
                           is not moving when the light is off, and to move when the light is
                           on. */

                       /* GPIOSetBit(PIN_F1, 0);
                        GPIOSetBit(PIN_F2, 0);
                        ServoSetSpeed(servo, 0);
                        ServoSetSpeed(servo2, 0);
                        DelayMillisec(5000); */
        ServoSetSpeed(servo, 0);
        ServoSetSpeed(servo2, 0);
    }
    void moveForward() {
        /* old forward code
         * // Make the servo go forward for 5 seconds.
                    GPIOSetBit(PIN_F1, 0);
                    GPIOSetBit(PIN_F2, 1);
                    ServoSetSpeed(servo, -20);
                    ServoSetSpeed(servo2, 20);
                    DelayMillisec(2500);
                    ServoSetSpeed(servo, -100);
                    ServoSetSpeed(servo2, 100);
                    DelayMillisec(2500);*/
        ServoSetSpeed(servo, -100); //left motor turns CCW
        ServoSetSpeed(servo2, 100); //right motor turns CW
        return;
    }
    void moveBackward() {
        /* old backward code */
        /* Make the servo go backward for 5 seconds. */
    /*    GPIOSetBit(PIN_F1, 1);
        GPIOSetBit(PIN_F2, 0);
        ServoSetSpeed(servo, 100);      //servo turns CW
        ServoSetSpeed(servo2, -100);    //servo2 turns CCW
        DelayMillisec(2500);
        ServoSetSpeed(servo, 20);
        ServoSetSpeed(servo2, -20);
        DelayMillisec(2500); */

        ServoSetSpeed(servo, 100);  //left motor turns CW
        ServoSetSpeed(servo2, -100);//right motor turns CCW
        return;
    }

    void turnLeft() {
        /* old turn left code */
        /* Make the servo turn left for 5 seconds. */
    /*    GPIOSetBit(PIN_F1, 1);
        GPIOSetBit(PIN_F2, 0);
        ServoSetSpeed(servo, 100);      //servo turns CW
        ServoSetSpeed(servo2, 100);     //servo turns CW
        DelayMillisec(2500);
        ServoSetSpeed(servo, 20);
        ServoSetSpeed(servo2, 20);
        DelayMillisec(2500); */

        ServoSetSpeed(servo, 100);      //left motor turns CW
        ServoSetSpeed(servo2, 100);     //right motor turns CW
    }

    void turnRight(){
        /* old turn right code */
    }

    void turnRight() {
        /* old turn right code */
        /* Make the servo turn right for 5 seconds. */
    /*    GPIOSetBit(PIN_F1, 1);
        GPIOSetBit(PIN_F2, 0);
        ServoSetSpeed(servo, -100);      //servo turns CCW
        ServoSetSpeed(servo2, -100);     //servo turns CCW
        DelayMillisec(2500);
        ServoSetSpeed(servo, -20);
        ServoSetSpeed(servo2, -20);
        DelayMillisec(2500); */

        ServoSetSpeed(servo, -100);      //left motor turns CCW
        ServoSetSpeed(servo2, -100);     //right motor turns CCW
    }
    EnableInterrupts();
    while(1) {
        uint8_t avgSide = 0;
        uint8_t i;
        for (i = 0; i < 8; ++i) {
            avgSide += sensor.values[i] << i;
        }

        /* Turn on RED LED if sensor data is none across the board. */
        /* Move forward if there is no sensor data */
        if (avgSide == 0) {
            GPIOSetBit(PIN_F1, 1);
            GPIOSetBit(PIN_F2, 0);
            GPIOSetBit(PIN_F3, 0);
            moveForward();
        }
        /* Turn on GREEN LED if sensor data is tending towards the left side. */
        /* Turn left if sensor data is towards left side */
        else if (avgSide >= 0x10) {
            GPIOSetBit(PIN_F1, 0);
            GPIOSetBit(PIN_F2, 0);
            GPIOSetBit(PIN_F3, 1);
            turnLeft();
        }
        /* Turn on BLUE LED if sensor data is tending towards the right side. */
        /* Turn right if sensor data is towards right side */
        else {
            GPIOSetBit(PIN_F1, 0);
            GPIOSetBit(PIN_F2, 1);
            GPIOSetBit(PIN_F3, 0);
            turnRight();
        }

    }
}
