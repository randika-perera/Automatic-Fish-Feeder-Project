/*                                                      Automatic fish food dispenser                                                       */
/*                                        Copyright (C) 2023 Randika Perera.  All rights reserved.                                          */




/*-------------------------------------------  Importing Libraries and Declaration of I/O Pins  --------------------------------------------*/

#include <Servo.h>
Servo myservo;

const int RGB_green_pin=2;
const int RGB_red_pin=3;
const int buzzer_pin=4;
const int uss_trig_pin=5;
const int uss_echo_pin=6;
const int touch_sensor1_pin=7;
const int touch_sensor2_pin=8; 
const int servo_pin=9;
const int tmr_switch_INA_pin=10;
const int tmr_switch_INC_pin=11;
const int qty_switch_INA_pin=12;
const int qty_switch_INC_pin=13;


/*-----------------------------------------------  Declaration of variables and constants  -------------------------------------------------*/

const int fish_food_threshold_level=7;  //If distance between the sensor and surface of fish food is more than 7cm, fish food level is LOW. Else, HIGH.
int current_fish_food_level;

int distance;
int roundeddistance;
long t;

int tmr_switch_INA=1; //  1 or 0
int tmr_switch_INC=1; //  1 or 0
int qty_switch_INA=1; //  1 or 0
int qty_switch_INC=1; //  1 or 0

int tmr_state=1;  //  1:24hour, 2:12hour,   3:8hour
int qty_state=1;  //  1:Low,    2:Medium,   3:High

bool FF_level_state=HIGH; // LOW:Low fish food,  HIGH: Sufficient fish food
bool FF_alarm_state=LOW;  // Alarm can only ring in HIGH state.

const int servo_slot_closed_pos=87;   //Servo position for closed slot (default position).
const int servo_slot_open_pos=66;     //Servo position for opened slot.
int slot_open_delay;                  //Time for which slot remains opened. Depends on qty_state.

//CHANGE THESE DURATIONS AS REQUIRED FOR FINAL PRODUCT
const int low_duration=150;      //Duration to keep slot open for low quantity     : 0.15s  
const int medium_duration=350;   //Duration to keep slot open for medium quantity  : 0.35s  
const int high_duration=500;     //Duration to keep slot open for high quantity    : 0.50s  

unsigned long current_time=0;
unsigned long previous_fishfooddispense_time=0;
unsigned long previous_levelmeasurement_time=0;
unsigned long previous_fflalarm_time=0;

//CHANGE THESE INTERVALS AS REQUIRED FOR FINAL PRODUCT
unsigned long fishfooddispense_interval;                //Depends on tmr_state. If tmrstate=1:24hours, tmrstate=2:12hours, tmrstate=3:8hours.
const unsigned long hour_24_interval = 20000;           //In final product:24 hours. Here it is 20s.
const unsigned long hour_12_interval = 10000;           //In final product:12 hours. Here it is 10s.
const unsigned long hour_8_interval  =  5000;           //In final product: 8 hours. Here it is 5s.
const unsigned long levelmeasurement_interval=10000;    //In final product: 6 hours. Here it is 10s.
const unsigned long fflalarm_interval=10000;            //In final product: 6 hours. Here it is 10s.


/*------------------------------------------------------------  Setup & Loop  --------------------------------------------------------------*/

void setup() 
{

    /*
    Serial.begin(9600);
    */

    pinMode(RGB_green_pin,OUTPUT);
    pinMode(RGB_red_pin,OUTPUT);
    pinMode(buzzer_pin,OUTPUT);
    pinMode(uss_trig_pin,OUTPUT);
    pinMode(uss_echo_pin,INPUT);
    pinMode(touch_sensor1_pin,INPUT);
    pinMode(touch_sensor2_pin,INPUT);
    pinMode(tmr_switch_INA_pin,INPUT);
    pinMode(tmr_switch_INC_pin,INPUT);
    pinMode(qty_switch_INA_pin,INPUT);
    pinMode(qty_switch_INC_pin,INPUT);
    myservo.attach(servo_pin);
    myservo.write(servo_slot_closed_pos);
    digitalWrite(RGB_green_pin,HIGH);
    digitalWrite(RGB_red_pin,HIGH);
    
}


void loop() 
{

    /*
    Serial.print("Timer State =");
    Serial.println(tmr_state);
    Serial.print("Quantity State = ");
    Serial.println(qty_state);
    Serial.println("");
    */
    
    current_time=millis();

    read_switches_and_set_states();
    manual_key();
    release_fish_food_when_required();
    level_measurement();
    low_fish_food_alarm();

}


/*-------------------------------------------------------  User Defined Functions  ---------------------------------------------------------*/

void read_switches_and_set_states()     //Reads the rocker switches, determines the qty and tmr states, sets slot_open_delay and fishfooddispense_interval.
{
    tmr_switch_INA=digitalRead(tmr_switch_INA_pin);
    tmr_switch_INC=digitalRead(tmr_switch_INC_pin);
    qty_switch_INA=digitalRead(qty_switch_INA_pin);
    qty_switch_INC=digitalRead(qty_switch_INC_pin);

    if ((tmr_switch_INA==HIGH)&&(tmr_switch_INC==LOW))
    {
        tmr_state=1;    //Tmr 24hour
    }
    else if ((tmr_switch_INA==HIGH)&&(tmr_switch_INC==HIGH))
    {
        tmr_state=2;    //Tmr 12hour
    }
    else if ((tmr_switch_INA==LOW)&&(tmr_switch_INC==HIGH))
    {
        tmr_state=3;    //Tmr 8hour
    }

    if ((qty_switch_INA==HIGH)&&(qty_switch_INC==LOW))
    {
        qty_state=3;    //Qty Low
    }
    else if ((qty_switch_INA==HIGH)&&(qty_switch_INC==HIGH))
    {
        qty_state=2;    //Qty Medium
    }
    else if ((qty_switch_INA==LOW)&&(qty_switch_INC==HIGH))
    {
        qty_state=1;    //Qty High
    }
    
    if (qty_state==1)
    {
        slot_open_delay=low_duration;
    }
    else if (qty_state==2)
    {
        slot_open_delay=medium_duration;
    }
    else if (qty_state==3)
    {
        slot_open_delay=high_duration;
    }

    if (tmr_state==1)
    {
        fishfooddispense_interval=hour_24_interval;
    }
    else if (tmr_state==2)
    {
        fishfooddispense_interval=hour_12_interval;    
    }
    else if (tmr_state==3)
    {
        fishfooddispense_interval=hour_8_interval;
    }
}


void release_fish_food()        //Instantaneously releases fish food through the slot.    
{
    myservo.write(servo_slot_open_pos);
    delay(slot_open_delay);
    myservo.write(servo_slot_closed_pos);
}


void release_fish_food_when_required()    //Releases fish food according to the fishfooddispense timer.
{
    if ((current_time-previous_fishfooddispense_time)>=fishfooddispense_interval)
    {
        release_fish_food();
        previous_fishfooddispense_time=previous_fishfooddispense_time+fishfooddispense_interval;
    }
}


void manual_key()       //Reads both touch sensors continuously. If either is pressed, instantaneously releases fish food according to the currently selected qty state. Also resets timers.
{

    int tsi_1=digitalRead(touch_sensor1_pin);
    int tsi_2=digitalRead(touch_sensor2_pin);

    if ((tsi_1==HIGH)||(tsi_2==HIGH))
    {

        release_fish_food();

        //All 3 timers will get reset when manual key is pressed.
        previous_fishfooddispense_time=current_time;
        previous_levelmeasurement_time=current_time;
        previous_fflalarm_time=current_time;

    }

}


void level_measurement()        //According to levelmeasurement timer, this measures fish food level, sets FF_level_state and sets colour of the RGB led.
{
    if ((current_time-previous_levelmeasurement_time)>=levelmeasurement_interval)
    {

        current_fish_food_level=measure_distance_using_uss();

        if (current_fish_food_level>=fish_food_threshold_level)
        {
            FF_level_state=LOW;
        }
        else 
        {
            FF_level_state=HIGH;
        }

        //RGB Leds are common anode type. Output HIGH:OFF, Ouput LOW:ON.
        if (FF_level_state==LOW)
        {
            digitalWrite(RGB_green_pin,HIGH);   //Off green led
            digitalWrite(RGB_red_pin,LOW);      //On red led
        }
        else
        {
            digitalWrite(RGB_red_pin,HIGH);     //Off red led
            digitalWrite(RGB_green_pin,LOW);    //On green led
        }

        previous_levelmeasurement_time=previous_levelmeasurement_time+levelmeasurement_interval;

    }
}


int measure_distance_using_uss()        //Measures and returns the distance from ultrasonic sensor to surface of fish food (in cm).
{
    digitalWrite(uss_trig_pin,HIGH);
    delayMicroseconds(10);
    digitalWrite(uss_trig_pin,LOW);
    t=pulseIn(uss_echo_pin,HIGH);
    distance=t*330/2/10000;
    roundeddistance=round(distance);
    return roundeddistance; 
}


void low_fish_food_alarm()      //Rings the alarm according to fflalarm timer.
{

    if (FF_level_state==HIGH)
    {
        FF_alarm_state=LOW;
    }

    if ((FF_level_state==LOW)&&(FF_alarm_state==LOW))
    {
        FF_alarm_state=HIGH;
        previous_fflalarm_time=millis();
    }

    if ((FF_alarm_state==HIGH)&&((current_time-previous_fflalarm_time)>=fflalarm_interval))
    {
        alarm();
        FF_alarm_state=LOW;
        previous_fflalarm_time=previous_fflalarm_time+fflalarm_interval;
    }
}


void alarm()        //Simulates an alarm using an active buzzer.
{
    digitalWrite(buzzer_pin,HIGH);
    delay(200);
    digitalWrite(buzzer_pin,LOW);
}




/*-------------------------------------------------------------------------------------------------------------------------------*/