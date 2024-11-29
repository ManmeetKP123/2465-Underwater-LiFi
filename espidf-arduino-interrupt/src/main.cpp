#include <Arduino.h>
#include "soc/rtc.h"
#include "freertos/xtensa_timer.h"
#include "esp_intr_alloc.h"

hw_timer_t *My_timer = NULL;
TaskHandle_t Task1;

void IRAM_ATTR onTimer(){
  digitalWrite(32, HIGH);
  digitalWrite(32, LOW);
}

void Task1code( void * parameter) {
  for(;;) {
    digitalWrite(32, HIGH);
    digitalWrite(32, LOW);
    vTaskDelay(1);
  }
}

void setup(){
  pinMode(8,OUTPUT);
  pinMode(32,OUTPUT);
  
  xTaskCreatePinnedToCore( // create a task on core 0 to make sure that still works with core 1 interrupts disabled later
      Task1code, /* Function to implement the task */
      "Task1", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &Task1,  /* Task handle. */
      0); /* Core where the task should run */

  // My_timer = timerBegin(0, 80, true);
  // timerAttachInterrupt(My_timer, &onTimer, true);
  // timerAlarmWrite(My_timer, 1000, true);
  // timerAlarmEnable(My_timer);
  ESP_INTR_DISABLE(XT_TIMER_INTNUM); // disables the tick interrupt
}

void loop(){
  digitalWrite(8,HIGH); // toggle a pin quickly - if everything is working there will be no gaps in the toggling signal
  digitalWrite(8,LOW);
}