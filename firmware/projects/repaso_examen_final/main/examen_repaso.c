/*! @mainpage Alimentador automático de mascotas
 *
 * @section genDesc General Description
 * Sistema de alimentación automática de mascotas basado en ESP-EDU.
 * Controla el nivel de agua mediante HC-SR04 y el peso de alimento
 * mediante una balanza analógica.
 *
 * @section hardConn Hardware Connection
 *
 * |    Periférico         |   ESP32   |
 * |:---------------------:|:---------:|
 * | HC-SR04 ECHO          | GPIO_3    |
 * | HC-SR04 TRIGGER       | GPIO_2    |
 * | HC-SR04 VCC           | 5V        |
 * | HC-SR04 GND           | GND       |
 * | Electroválvula (señal)| GPIO_4    |
 * | Balanza (señal anal.) | CH0       |
 * | Dispensador alimento  | GPIO_6    |
 *
 * @section changelog Changelog
 * |   Date     | Description        |
 * |:----------:|:-------------------|
 * | 26/03/2026 | Document creation  |
 *
 * @author @author Santiago Salinas (santiago.salinas@ingeneieria.uner.edu)
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
#include "hc_sr04.h"
#include "uart_mcu.h"
#include "gpio_mcu.h"
#include "analog_io_mcu.h"

/*==================[macros and definitions]=================================*/
#define GPIO_ECHO_PIN              GPIO_3
#define GPIO_TRIGGER_PIN           GPIO_2
#define GPIO_ELECTROVALVULA_PIN    GPIO_4
#define GPIO_ALIMENTO_PIN          GPIO_6
#define ADC_BALANZA_CH             CH0

#define AGUA_VOLUMEN_MIN_CM3       500.0f
#define AGUA_VOLUMEN_MAX_CM3       2500.0f
#define ALIMENTO_PESO_MIN_G        50.0f
#define ALIMENTO_PESO_MAX_G        500.0f
#define BALANZA_CAPACIDAD_MAX_G    1000.0f
#define SENSOR_ALTURA_CM           30.0f
#define RECIPIENTE_AREA_CM2        314.16f   // pi * 10^2
#define TAREA_PERIODO_MS           5000

/*==================[internal data definition]===============================*/
/** @brief Indica si el sistema está activo */
bool sistema_activo = false;

/** @brief Volumen de agua en cm3 (compartida entre tareas) */
float volumen_agua_cm3 = 0.0f;

/** @brief Peso del alimento en gramos (compartida entre tareas) */
float peso_alimento_g = 0.0f;

/*==================[internal functions declaration]=========================*/

/**
 * @brief Callback de interrupción de SWITCH_1
 * @details Alterna el estado del sistema y actualiza el LED_1
 */
void Tecla1Handler(void *param) {
    sistema_activo = !sistema_activo;
    if (sistema_activo) LedOn(LED_1);
    else                LedOff(LED_1);
}

/**
 * @brief Tarea de control del nivel de agua
 * @details Mide distancia con HC-SR04, calcula volumen y controla
 *          la electroválvula. Se ejecuta cada 5 segundos.
 */
static void ControlAguaTask(void *pvParameter) {
    float distancia_cm, nivel_cm;

    while (true) {
        if (sistema_activo) {
            distancia_cm = HcSr04ReadDistanceInCentimeters();
            nivel_cm     = SENSOR_ALTURA_CM - distancia_cm;
            volumen_agua_cm3 = RECIPIENTE_AREA_CM2 * nivel_cm;

            if (volumen_agua_cm3 < AGUA_VOLUMEN_MIN_CM3) {
                GPIOOn(GPIO_ELECTROVALVULA_PIN);
            } else if (volumen_agua_cm3 > AGUA_VOLUMEN_MAX_CM3) {
                GPIOOff(GPIO_ELECTROVALVULA_PIN);
            }
        }
        vTaskDelay(TAREA_PERIODO_MS / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Tarea de control del alimento
 * @details Lee la balanza analógica, convierte a gramos y controla
 *          el mecanismo dispensador. Se ejecuta cada 5 segundos.
 */
static void ControlAlimentoTask(void *pvParameter) {
    uint16_t valor_mv;

    while (true) {
        if (sistema_activo) {
            AnalogInputReadSingle(ADC_BALANZA_CH, &valor_mv);
            // 0 mV = 0g, 3300 mV = 1000g
            peso_alimento_g = (valor_mv * BALANZA_CAPACIDAD_MAX_G) / 3300.0f;

            if (peso_alimento_g < ALIMENTO_PESO_MIN_G) {
                GPIOOn(GPIO_ALIMENTO_PIN);
            } else if (peso_alimento_g > ALIMENTO_PESO_MAX_G) {
                GPIOOff(GPIO_ALIMENTO_PIN);
            }
        }
        vTaskDelay(TAREA_PERIODO_MS / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Tarea de reporte por UART
 * @details Envía el estado de agua y alimento por puerto serie cada 5s.
 */
static void ReporteUartTask(void *pvParameter) {
    char mensaje[64];

    while (true) {
        if (sistema_activo) {
            sprintf(mensaje, "Agua: %.0f cm3, Alimento: %.0f gr\r\n",
                    volumen_agua_cm3, peso_alimento_g);
            UartSendString(UART_PC, mensaje);
        }
        vTaskDelay(TAREA_PERIODO_MS / portTICK_PERIOD_MS);
    }
}

/*==================[external functions definition]==========================*/
/** @brief Función principal
 * @details Inicializa los periféricos y crea las tareas.
 */
void app_main(void) {
    LedsInit();
    SwitchesInit();

    // GPIOs de salida
    GPIOInit(GPIO_ELECTROVALVULA_PIN, GPIO_OUTPUT);
    GPIOInit(GPIO_ALIMENTO_PIN, GPIO_OUTPUT);

    // Sensor ultrasonido
    HcSr04Init(GPIO_ECHO_PIN, GPIO_TRIGGER_PIN);

    // Entrada analógica (balanza)
    analog_input_config_t adc_config = {
        .input    = ADC_BALANZA_CH,
        .mode     = ADC_SINGLE,
        .func_p   = NULL,
        .param_p  = NULL,
        .sample_frec = 0
    };
    AnalogInputInit(&adc_config);

    // UART
    serial_config_t uart_config = {
        .port      = UART_PC,
        .baud_rate = 115200,
        .func_p    = NULL,
        .param_p   = NULL
    };
    UartInit(&uart_config);

    // Interrupción de tecla 1 (con LED incluido en el handler)
    SwitchActivInt(SWITCH_1, Tecla1Handler, NULL);

    // Crear tareas (una por función)
    xTaskCreate(ControlAguaTask,     "Agua",     2048, NULL, 5, NULL);
    xTaskCreate(ControlAlimentoTask, "Alimento", 2048, NULL, 5, NULL);
    xTaskCreate(ReporteUartTask,     "UART",     2048, NULL, 5, NULL);
}
/*==================[end of file]============================================*/