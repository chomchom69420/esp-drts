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
TickType_t xTaskP1_duration = 80;
TickType_t xTaskS1_duration = 2;
TickType_t xTaskA1_duration = 1;
TickType_t xTaskP2_duration = 82;
TickType_t xTaskS2_duration = 3;
TickType_t xTaskA2_duration = 2;
TickType_t sample_period = 100;

//Task handles
TaskHandle_t xTaskP1_handle;
TaskHandle_t xTaskS2_handle;
TaskHandle_t xTaskA2_handle;
TaskHandle_t xTaskControl_handle;

//Queue handles
QueueHandle_t xQueue_P1_mx1;
QueueHandle_t xQueue_P1_my1;
QueueHandle_t xQueue_S2_mx2;
QueueHandle_t xQueue_A2_my2;

//Message struct 
typedef enum {
    emx1,
    emy1,
    emx2,
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
        xSendData.eDataType = emx2;
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

void vTaskControl (void *parameters) {
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = sample_period/10;
    twai_status_info_t xTWAIStatus;
    twai_message_t xRecvMessage;
    twai_message_t xTransmitMessage;
    for(;;) {
        xLastWakeTime = xTaskGetTickCount();

        //Check if the CAN RX queue is empty or not. If not, store the RX messages in the corresponding task queues
        if(twai_get_status_info(&xTWAIStatus)!=ESP_OK) {
            printf("Issue with getting CAN info...");
        }
        else {
            while(xTWAIStatus.msgs_to_rx > 0) {
                if(twai_receive(&xRecvMessage, 0)==ESP_OK) {
                    //Check the message data type 
                    Data_t xMessage;
                    xMessage.ucValue = xRecvMessage.data[0];
                    xMessage.eDataType = (int)(xRecvMessage.data[1] << (8*3) |  xRecvMessage.data[2] << (8*2) | xRecvMessage.data[3] << 8 | xRecvMessage.data[4]);

                    //Processor 2 is only receiving two types of messages: mx1, my2
                    if (xMessage.eDataType == emx1) {
                        xQueueSendToBack(xQueue_P1_mx1, &xMessage.ucValue, 0);
                    }
                    else if (xMessage.eDataType == emy2) {
                        xQueueSendToBack(xQueue_A2_my2, &xMessage.ucValue, 0);
                    }
                }
            }
        }

        //If task output queues have stuff, send them over to CAN

        //For Queue connected to output of S2 
        while (uxQueueMessagesWaiting(xQueue_S2_mx2) > 0) {
            //Get the message from the queue
            Data_t xMessage;
            xQueueReceive(xQueue_S2_mx2, &xMessage, 0);
            
            //send over CAN
            xTransmitMessage.identifier = 0xAAAA;
            xTransmitMessage.flags = TWAI_MSG_FLAG_NONE;
            xTransmitMessage.data_length_code = 5;
            xTransmitMessage.data[0] = xMessage.ucValue;
            xTransmitMessage.data[1] = xMessage.eDataType & 0xFF000000;
            xTransmitMessage.data[2] = xMessage.eDataType & 0x00FF0000;
            xTransmitMessage.data[3] = xMessage.eDataType & 0x0000FF00;
            xTransmitMessage.data[4] = xMessage.eDataType & 0x000000FF;

            //Queue message for transmission
            twai_transmit(&xTransmitMessage, 0);
        }

        //For Queue connected to output of P1
        while (uxQueueMessagesWaiting(xQueue_P1_my1) > 0) {
            //Get the message from the queue
            Data_t xMessage;
            xQueueReceive(xQueue_P1_my1, &xMessage, 0);
            
            //send over CAN
            xTransmitMessage.identifier = 0xAAAA;
            xTransmitMessage.flags = TWAI_MSG_FLAG_NONE;
            xTransmitMessage.data_length_code = 5;
            xTransmitMessage.data[0] = xMessage.ucValue;
            xTransmitMessage.data[1] = xMessage.eDataType & 0xFF000000;
            xTransmitMessage.data[2] = xMessage.eDataType & 0x00FF0000;
            xTransmitMessage.data[3] = xMessage.eDataType & 0x0000FF00;
            xTransmitMessage.data[4] = xMessage.eDataType & 0x000000FF;

            //Queue message for transmission
            twai_transmit(&xTransmitMessage, 0);
        }

        //Suspend until one of the tasks wakes it up
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
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

    xQueue_P1_mx1 = xQueueCreate(5, sizeof(Data_t));
    xQueue_P1_my1 = xQueueCreate(5, sizeof(Data_t));
    xQueue_S2_mx2 = xQueueCreate(5, sizeof(Data_t));
    xQueue_A2_my2 = xQueueCreate(5, sizeof(Data_t));

    if(xQueue_P1_mx1 == NULL || xQueue_P1_my1 == NULL || xQueue_S2_mx2 == NULL || xQueue_A2_my2 == NULL) {
        printf("Failed to initialize queues");
        for(;;);
    }

    xTaskCreatePinnedToCore(vTask_A2, "Actuation Task 2", 2048, (void *) NULL, 2, &xTaskA2_handle, 1);
    xTaskCreatePinnedToCore(vTask_S2, "Sensing Task 2", 2048, (void *) NULL, 2, &xTaskS2_handle, 1);
    xTaskCreatePinnedToCore(vTask_P1, "Processing Task 1", 2048, (void *) NULL, 2, &xTaskP1_handle, 1);
    xTaskCreatePinnedToCore(vTaskControl, "Control Task", 4096, (void *) NULL, 2, &xTaskControl_handle, 1);
}