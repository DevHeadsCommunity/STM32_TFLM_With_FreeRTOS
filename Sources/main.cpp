#include <cstdio>
/*-------------FreeRTOS Includes--------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/********Tensorflow lite micro include *************/
#include "micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h"
#include "micro/tflite_bridge/micro_error_reporter.h"
#include "micro/micro_interpreter.h"
#include "schema/schema_generated.h"

/*-------------Custom Driver Includes--------------*/
#include "timer.h"
#include "dht.h"
#include "gpio.h"
#include "clock.hh"
#include "uart.h"
#include "model.hh"

tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter *error_reporter = &micro_error_reporter;

const tflite::Model *model = ::tflite::GetModel(temp_tflite);

using GestureOpResolver = tflite::MicroMutableOpResolver<2>;
GestureOpResolver resolver;

TfLiteStatus RegisterOps(GestureOpResolver &op_resolver)
{
    // Adding Operations to the Operation Resolver
    TF_LITE_ENSURE_STATUS(op_resolver.AddFullyConnected());
    TF_LITE_ENSURE_STATUS(op_resolver.AddSoftmax());
    return kTfLiteOk;
}

const int tensor_arena_size = 4096U;
uint8_t tensor_arena[tensor_arena_size];

tflite::MicroInterpreter interpreter(model, resolver, tensor_arena, tensor_arena_size);

void USART2_Config();

void DebugCallback(const char *s)
{
    UART_SendBuffer(USART2, (uint8_t *)s, strlen(s));
}

DHT dht{GPIOA, 1};
float temp;
int main(void)
{
    SetSystemClock180MHz();
    UART2_GPIO_Init();
    USART2_Config();
    TimerInit();
    DHT_begin(dht);

    RegisterDebugLogCallback(DebugCallback);

    TfLiteTensor *input;
    TfLiteTensor *output;

    // BuiltInBtnExtiInit();

    // Check TensorFlow Lite schema version
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        TF_LITE_REPORT_ERROR(error_reporter,
                             "Model provided is schema version %d not equal "
                             "to supported version %d.\n",
                             model->version(), TFLITE_SCHEMA_VERSION);
    }
    else
    {
        TF_LITE_REPORT_ERROR(error_reporter,
                             "Model Version %d and "
                             "Schema Version %d. Matched!\n",
                             model->version(), TFLITE_SCHEMA_VERSION);
    }

    // Register operations
    RegisterOps(resolver);

    // Allocate tensors
    TF_LITE_ENSURE_STATUS(interpreter.AllocateTensors());

    input = interpreter.input(0);

    // vTaskStartScheduler();
    float scale = 0.00390625;
    int zero_point = 128;

    while (1)
    {
        temp = DHT_ReadTemperature(dht, false);
        printf("Temperature: %.2f. \r\n", temp);

        if (input == NULL)
        {
            printf("[ERROR] Input tensor is NULL!\n");
        }

        input->data.int8[0] = static_cast<int8_t>(temp);

        printf("Input tensor dims: %d\n\r", input->dims->size);
        printf("Input tensor type: %d\n\r", input->type);
        printf("Input tensor data value: %d\n\r", input->data.int8[0]);

        if (interpreter.Invoke() != kTfLiteOk)
        {
            printf("[ERROR] Invocation Failed.\r\n");
        }

        output = interpreter.output(0);

        if (output == NULL)
        {
            printf("[ERROR] Output tensor is NULL!\n");
        }

        // Dequantize and print results
        for (int i = 0; i < 3; i++)
        {
            float real_value = scale * (output->data.int8[i] + zero_point);
            printf("Output[%d]: %.6f\n\r", i, real_value);
        }
        DelayMS(2000);
    }

    return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    for (;;)
        ; // Halt system to debug
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
