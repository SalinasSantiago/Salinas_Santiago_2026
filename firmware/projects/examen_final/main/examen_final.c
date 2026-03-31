/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * El sistema esta compuerto por 2 sensores y 3 bombas 
 *El sensor de humedad se conecta a un GPIO de 
 * la placa y cambia su estado de “0” a “1” lógico cuando la humedad presente en la tierra es inferior a la necesaria
 * La bomba de agua deberá encenderse en este caso. 

 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Santiago Salinas (santiago.salinas@ingenieria.uner.edu.ar)
 *
 */





/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "analog_io_mcu.h"
#include "gpio_mcu.h"
#include "led.h"
#include "switch.h"
#include "uart_mcu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/*==================[macros and definitions]=================================*/
#define TAREA_PERIODO_UART_MS           5000
#define TAREA_PERIODO_CONTROL_MS         3000


#define GPIO_BOMBA_AGUA_PIN             GPIO_8
#define GPIO_HUMEDAD_PIN                GPIO_9

#define GPIO_SENSOR_PH_PIN              GPIO_0
#define GPIO_BOMBA_ALCALINA_PIN         GPIO_5
#define GPIO_BOMBA_ACIDA_PIN            GPIO_6

#define PH_ALCALINO_MIN                  6.0
#define PH_ACIDO_MAX                     6.4 

#define ADC_PH_SENSOR_CH 			     CH0 
#define PH_MAX 						   14.0	
#define VOLTAGE_REF_MV                  3300.0




/*==================[internal data definition]===============================*/
bool sistema_activo = false;
float ph_actual_uart = 0.0;
char mensaje_uart[100];



/*==================[internal functions declaration]=========================*/
/** @brief Tarea de control de humedad
 * @details Lee el estado del sensor de humedad y controla la bomba de agua en consecuencia.
 */

void ControlHumedadTask(void *pvParameter) {
	bool humedad = false;
	SwitchesRead(); 
	while (true) {

		if( sistema_activo ){
			
			humedad = GPIORead(GPIO_HUMEDAD_PIN); 

			if (humedad) {
				mensaje_uart[0] = "Humedad Correcta\r\n";
				GPIOOn(GPIO_BOMBA_AGUA_PIN);
			} else {
				mensaje_uart[0] = "Humedad Incorrecta\r\n";
				mensaje_uart[1] = 'Bomba de agua encendida\r\n';
				GPIOOff(GPIO_BOMBA_AGUA_PIN); 
			}

			
	}
		vTaskDelay(TAREA_PERIODO_CONTROL_MS / portTICK_PERIOD_MS); 
}
}

/** @brief Tarea de control de pH
 * @details Lee el valor del sensor de pH y controla las bombas en consecuencia luego espera .
 */
static void ControlPH(void *pvParameter) {
	SwitchesRead(); 
    uint16_t valor_mv=0;
    float ph=0.0;
    while (true) {
        if (sistema_activo) {
            AnalogInputReadSingle(ADC_PH_SENSOR_CH, &valor_mv);
            // 0 mV = 0 , 3300 mV = 14 ph 
            ph = (valor_mv * PH_MAX) / VOLTAGE_REF_MV;
			ph_actual_uart = ph;
            if (ph < PH_ALCALINO_MIN) {
				mensaje_uart[0] = "Bomba alcalina encendida \r\n";
                GPIOOn(GPIO_BOMBA_ALCALINA_PIN);
            } else if (ph > PH_ACIDO_MAX) {
				mensaje_uart[0] = "Bomba acida encendida \r\n";
                GPIOOff(GPIO_BOMBA_ACIDA_PIN);
            }
        }
        vTaskDelay(TAREA_PERIODO_CONTROL_MS / portTICK_PERIOD_MS);
    }
}


/**
 * @brief Tarea de reporte por UART
 * @details Envía el estado del ph y el mesanje segun corresponda por puerto serie cada 5s.
 */
static void ReporteUartTask(void *pvParameter) {
    char mensaje[64];
	SwitchesRead(); 
    while (true) {
        if (sistema_activo) {
			
            UartSendString(UART_PC, mensaje);
			sprintf(mensaje, "Valor de pH: \r\n", ph_actual_uart);
        }
        vTaskDelay(TAREA_PERIODO_UART_MS / portTICK_PERIOD_MS);
    }
}

void TeclasEncendidoHandler(void *pvParameter) {
    
	sistema_activo = true;
}

void TeclasApagadoHandler(void *pvParameter) {
    sistema_activo = false;
}

/*==================[external functions definition]==========================*/

/** @brief Función principal
 * @details Inicializa los periféricos y crea las tareas.
 */
void app_main(void){
	GPIOInit(GPIO_HUMEDAD_PIN, GPIO_INPUT);
	GPIOInit(GPIO_BOMBA_AGUA_PIN, GPIO_OUTPUT);
	SwitchesInit();


	analog_input_config_t adc_config = {
        .input    = ADC_PH_SENSOR_CH,
        .mode     = ADC_SINGLE,
        .func_p   = NULL,
        .param_p  = NULL,
        .sample_frec = 0
    };
    AnalogInputInit(&adc_config);



	serial_config_t uart_config = {
        .port      = UART_PC,
        .baud_rate = 115200,
        .func_p    = NULL,
        .param_p   = NULL
    };
    UartInit(&uart_config);

    
    SwitchActivInt(SWITCH_1, TeclasEncendidoHandler, NULL);
    SwitchActivInt(SWITCH_2, TeclasApagadoHandler, NULL);

	xTaskCreate(ControlHumedadTask, "ControlHumedadTask", 2048, NULL, 1, NULL);
	xTaskCreate(ControlPH, "ControlPHTask", 2048, NULL, 1, NULL);
	xTaskCreate(ReporteUartTask, "ReporteUartTask", 2048, NULL, 1, NULL);
}
/*==================[end of file]============================================*/