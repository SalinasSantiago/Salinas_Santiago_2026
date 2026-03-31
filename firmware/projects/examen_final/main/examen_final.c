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
#define GPIO_BOMBA_AGUA_PIN             GPIO_8
#define GPIO_HUMEDAD_PIN                GPIO_9
#define TAREA_PERIODO_CONTROL_MS         3000
/*==================[internal data definition]===============================*/


/** @brief Tarea de control de humedad
 * @details Lee el estado del sensor de humedad y controla la bomba de agua en consecuencia.
 */

void ControlHumedadTask(void *pvParameter) {
	while (true) {
		bool humedad = GPIORead(GPIO_HUMEDAD_PIN); // Leer humedad del sensor

		if (humedad) {
			GPIOOn(GPIO_BOMBA_AGUA_PIN); // Encender bomba si humedad es baja
		} else {
			GPIOOff(GPIO_BOMBA_AGUA_PIN); // Apagar bomba si humedad es adecuada
		}

		vTaskDelay(TAREA_PERIODO_CONTROL_MS / portTICK_PERIOD_MS); 
	}

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Hello world!\n");
}
/*==================[end of file]============================================*/