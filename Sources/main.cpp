#include <cstdio>
#include <cstring>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h"
#include "micro/tflite_bridge/micro_error_reporter.h"
#include "micro/micro_interpreter.h"
#include "schema/schema_generated.h"

#include "timer.h"
#include "dht.h"
#include "gpio.h"
#include "clock.hh"
#include "uart.h"
#include "model.hh"

// FreeRTOS Semaphores
SemaphoreHandle_t xSamplingDone;
SemaphoreHandle_t xProcessingDone;

// TensorFlow Lite Micro setup
tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter *error_reporter = &micro_error_reporter;
const tflite::Model *model = ::tflite::GetModel(temperature_quantized_tflite);
using TempOpResolver = tflite::MicroMutableOpResolver<2>;
TempOpResolver resolver;

TfLiteStatus RegisterOps(TempOpResolver &op_resolver)
{
    TF_LITE_ENSURE_STATUS(op_resolver.AddFullyConnected());
    TF_LITE_ENSURE_STATUS(op_resolver.AddSoftmax());
    return kTfLiteOk;
}

// Tensor Arena
const int tensor_arena_size = 4096U;
uint8_t tensor_arena[tensor_arena_size];
tflite::MicroInterpreter interpreter(model, resolver, tensor_arena, tensor_arena_size);

void USART2_Config();
void DebugCallback(const char *s)
{
    UART_SendBuffer(USART2, (uint8_t *)s, strlen(s));
}

// Sensor Setup
DHT dht{GPIOA, 1};
float temp;

// Tasks
void pvDataReadingTask(void *params);
void pvDataProcessingTask(void *params);
void pvDataLoggingTask(void *params);

int main(void)
{
    SetSystemClock180MHz();
    UART2_GPIO_Init();
    USART2_Config();
    TimerInit();
    DHT_begin(dht);
    RegisterDebugLogCallback(DebugCallback);

    xSamplingDone = xSemaphoreCreateBinary();
    xProcessingDone = xSemaphoreCreateBinary();

    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        TF_LITE_REPORT_ERROR(error_reporter, "Model schema version mismatch!\n\r");
        return -1;
    }

    RegisterOps(resolver);
    TF_LITE_ENSURE_STATUS(interpreter.AllocateTensors());

    xTaskCreate(pvDataReadingTask, "Sampling Task", 1000, NULL, 2, NULL);
    xTaskCreate(pvDataProcessingTask, "Processing Task", 1000, NULL, 1, NULL);
    xTaskCreate(pvDataLoggingTask, "Logging Task", 1000, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1)
        ;
    return 0;
}

void USART2_Config()
{
    UARTConfig_t uart2;
    uart2.pUARTx = USART2;
    uart2.Init.BaudRate = 115200U;
    uart2.Init.Mode = UART_MODE_TX_ONLY;
    uart2.Init.Parity = UART_PARITY_NONE;
    uart2.Init.WordLen = UART_WORD_LEN_8BITS;
    UART_Init(&uart2);
}

extern "C" int __io_putchar(int ch)
{
    UART_SendChar(USART2, ch);
    return ch;
}

void pvDataReadingTask(void *params)
{
    while (true)
    {
        temp = DHT_ReadTemperature(dht, false);
        printf("Temperature: %.2f\n\r", temp);
        xSemaphoreGive(xSamplingDone);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void pvDataProcessingTask(void *params)
{
    while (true)
    {
        xSemaphoreTake(xSamplingDone, portMAX_DELAY);
        TfLiteTensor *input = interpreter.input(0);
        if (!input)
        {
            printf("[ERROR] Input tensor is NULL!\n\r");
            continue;
        }

        float xscaled = (temp - 21.7625) / 6.1161;
        input->data.int8[0] = static_cast<int8_t>((xscaled / 0.01795339770615101) - 12);

        if (interpreter.Invoke() != kTfLiteOk)
        {
            printf("[ERROR] Inference Failed!\n\r");
            continue;
        }

        xSemaphoreGive(xProcessingDone);
    }
}

void pvDataLoggingTask(void *params)
{
    while (true)
    {
        xSemaphoreTake(xProcessingDone, portMAX_DELAY);
        TfLiteTensor *output = interpreter.output(0);
        if (!output)
        {
            printf("[ERROR] Output tensor is NULL!\n\r");
            continue;
        }

        float scale = 0.00390625;
        int zero_point = -128;
        float predictions[3];

        for (int i = 0; i < 3; i++)
        {
            predictions[i] = scale * (output->data.int8[i] - zero_point);
        }

        const char *labels[] = {"Cold", "Warm", "Hot"};
        int class_index = 0;
        float max_prob = predictions[0];

        for (int i = 1; i < 3; i++)
        {
            if (predictions[i] > max_prob)
            {
                max_prob = predictions[i];
                class_index = i;
            }
        }

        printf("Predicted: %s (Confidence: %.2f)\n\r", labels[class_index], max_prob);
        printf("--------------------------------------------------\n\r");

        vTaskDelay(pdMS_TO_TICKS(1000)); // Added delay between logs
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("[ERROR] Stack overflow detected in task: %s\n\r", pcTaskName);
    for (;;)
        ; // Halt system for debugging
}
