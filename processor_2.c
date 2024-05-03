#include <stdio.h>
#include "driver/gpio.h"
#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "sdkconfig.h"

//Timing configuration
TickType_t xTaskS2_duration = 3;
TickType_t xTaskA2_duration = 2;
TickType_t xTaskP1_duration = 80;


//Queues for input and output messages
QueueHandle_t xQueue_S2_mx2;
QueueHandle_t xQueue_A2_my2;
QueueHandle_t xQueue_P1_mx1;
QueueHandle_t xQueue_P1_my1;


//Task handles
TaskHandle_t xTaskS2_handle;
TaskHandle_t xTaskA2_handle;
TaskHandle_t xTaskP1_handle;
TaskHandle_t xTaskControl_handle;


//Message struct
typedef enum {
    emx1,
    emx2,
    emy1,
    emy2
} DataType_t;

typedef struct {
    uint8_t ucValue;
    DataType_t eDataType;
} Data_t;

void vTask_S2 (void *parameters) {
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = sample_period;
    Data_t xSendData;
    BaseType_t xStatus;
    for (;;) {
        xLastWakeTime = xTaskGetTickCount();
        volatile long int a=0;
        printf("S2 start: %lu\n", pdTICKS_TO_MS(xTaskGetTickCount()));
        TickType_t timestamp = xTaskGetTickCount();
        while (xTaskGetTickCount() - timestamp < xTaskS2_duration) {
            a++;
        }

        //Store data to be sent
        xSendData.eDataType = emx1;
        xSendData.ucValue  = 200;
        xStatus = xQueueSendToBack(xQueue_S2_mx2, (const void *) &xSendData, 0);
        if(xStatus==errQUEUE_FULL) {
            printf("Queue is full!");
        }
        printf("S2 end: %lu\n", pdTICKS_TO_MS(xTaskGetTickCount()));

        //Resume control task for sending this over to CAN
        vTaskResume(xTaskControl_handle);

        //Delay until next time period
        vTaskDelayUntil(&xLastWakeTime, xPeriod); //reference, period
    }
}

void vTask_A2 (void *parameters) {
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = sample_period;
    Data_t xReceivedData;
    BaseType_t xStatus;
    for (;;) {
        xLastWakeTime = xTaskGetTickCount();

        //Read from the input queue
        xStatus = xQueueReceive(xQueue_A2_my2, &xReceivedData, 0);
        if(xStatus == errQUEUE_EMPTY) {
            printf("Queue is empty!");
        }

        volatile long int a=0;
        printf("A2 start: %lu\n", pdTICKS_TO_MS(xTaskGetTickCount()));
        TickType_t timestamp = xTaskGetTickCount();
        while (xTaskGetTickCount() - timestamp < xTaskA2_duration) {
            a++;
        }
        printf("A2 end: %lu\n", pdTICKS_TO_MS(xTaskGetTickCount()));
        vTaskDelayUntil(&xLastWakeTime, xPeriod); //reference, period
    }
}

void vTask_P1 (void *parameters) {
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = sample_period;
    Data_t xReceivedData;
    Data_t xSendData;
    BaseType_t xStatus;
    for (;;) {
        xLastWakeTime = xTaskGetTickCount();

        //Read from the input queue 
        xStatus = xQueueReceive(xQueue_P1_mx1, &xReceivedData, 0);
        if(xStatus == errQUEUE_EMPTY) {
            printf("Queue is empty!");
        }

        volatile long int a=0;
        printf("P1 start: %lu\n", pdTICKS_TO_MS(xTaskGetTickCount()));
        TickType_t timestamp = xTaskGetTickCount();
        while (xTaskGetTickCount() - timestamp < xTaskP1_duration) {
            a++;
        }
        printf("P1 end: %lu\n", pdTICKS_TO_MS(xTaskGetTickCount()));

        //Before exiting send to the output queue 
        xSendData.eDataType = emy1;
        xSendData.ucValue = 100;
        xStatus = xQueueSendToBack(xQueue_P1_my1, (const void *) &xSendData, 0);

        if(xStatus==errQUEUE_FULL) {
            printf("Queue is full!");
        }

        //Resume the control task
        vTaskResume(xTaskControl_handle);

        vTaskDelayUntil(&xLastWakeTime, xPeriod); //reference, period
    }
}

void app_main(void)
{
    //Setup CAN
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    //Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }

    //Start TWAI driver
    if (twai_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        return;
    }

    xQueue_S2_mx2 = xQueueCreate(5, sizeof(Data_t));
    xQueue_A2_my2 = xQueueCreate(5, sizeof(Data_t));
    xQueue_P1_mx1 = xQueueCreate(5, sizeof(Data_t));
    xQueue_P1_my1 = xQueueCreate(5, sizeof(Data_t));


    xTaskCreatePinnedToCore(vTask_S2, "S2", 2048, (void *) NULL, 2, &xTaskS2_handle, 1);
    xTaskCreatePinnedToCore(vTask_A2, "A2", 2048, (void *) NULL, 2, &xTaskA2_handle, 1);
    xTaskCreatePinnedToCore(vTask_P1, "P1", 2048, (void *) NULL, 2, &xTaskP1_handle, 1);
    xTaskCreatePinnedToCore(vTaskControl, "Control Task", 4096, (void *) NULL, 3, &xTaskControl_handle, 1);

}
