/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * @section hardConn Hardware Connection 
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	ECHO	 	| 	GPIO_3		|
 * | 	TRIG	 	| 	GPIO_2		|
 * | 	VCC	   	    | 	 5V			|
 * | 	GND	   	    | 	GND			|
 * 
 * 
 * 
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 26/3/2026 | Document creation		                         |
 *
 * @author Santiago Salinas Sosa (salinassantiago9@gmail.com)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// HC SR04 sensor ultrasonico para medir distancia donde va de 0 a 9.5  cada 5s
//v= PI * R * R * h  
// activar cuando v= 500cm3 y desactivar cuando v= 2500 cm3
/// 
#include "hc_sr04.h"
// Uart debemos enviar por el monitor serie Agua: xxx cm3, Alimento: xxx gr cada 5s para eso voy a leer la entrada 
// analogica que va de 0v a 3.3v  donde 3.3v son 1000g es decir 3.3mv son 100g 

#include "uart_mcu.h"
// Debemos enviar una señal en alto por gpio cada vez que el alimento sea menor a 50g y en bajo cuando sea mayor a 500g
#include "gpio_mcu.h"
// Como la medición de distancia es cada 5s, el envío por uart y la señal por gpio también deben ser cada 5s,
// por lo que se puede usar un solo timer para esto
#include "timer_mcu.h"

#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/
#define TIMER_PERIOD_US 5000000
#define GPIO_TRIGGER_PIN GPIO_2
#define GPIO_ECHO_PIN GPIO_3
#define GPIO_ELECTRO_VALVULA_PIN GPIO_4   
#define GPIO_BALANZA_PIN GPIO_5	
#define GPIO_ALIMENTO_PIN GPIO_6
#define UART_TX_PIN GPIO_16
#define UART_RX_PIN GPIO_17	

/*==================[internal data definition]===============================*/
TaskHandle_t medir_distancia_task_handle;

///La balanza analógica nos devolverá una señal de 0,0V cuando no tenga carga y 3,3V
///cuando alcance su máximo de capacidad (1.000g). Las mediciones de peso se deben
///realizar cada 5 segundos. 0.0v 0g entonces 50g son 0.165v, 500g son 1.65v


/*==================[internal functions declaration]=========================*/
void TimerHandler( void* param){
	vTaskNotifyGiveFromISR(medir_distancia_task_handle, pdFALSE);    // 
}


void ControlAlimentoTask(void* pvParameters){
	float peso;
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		peso = AnalogRead(GPIO_BALANZA_PIN) * 1000.0 / 3.3; 
		if (peso < 50.0){
			GPIOOn(GPIO_ALIMENTO_PIN);
		} else if (peso > 500.0){
			GPIOOff(GPIO_ALIMENTO_PIN);
		}
	}
}


void ControlAguaTask(void* pvParameters){

	float distancia;
	float nivel_agua;
	float volumen_agua;
	const float area = 314.16; 

	while(1){
		
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		distancia = HcSr04ReadDistanceInCentimeters();
		nivel_agua = 30.0 - distancia;
		volumen_agua = area * nivel_agua;
		

		if (volumen_agua < 500.0){
			GPIOOn(GPIO_ELECTRO_VALVULA_PIN);
		} else if (volumen_agua > 2500.0){
			GPIOOff(GPIO_ELECTRO_VALVULA_PIN);
		}
		}
	}


















/*==================[external functions definition]==========================*/
void app_main(void){
	 /// gpios 
	 GPIOInit(GPIO_ELECTRO_VALVULA_PIN, GPIO_OUTPUT);
	 HcSr04Init(GPIO_ECHO_PIN, GPIO_TRIGGER_PIN);  
	 GPIOInit(GPIO_ALIMENTO_PIN, GPIO_OUTPUT);
	 GPIOInit(GPIO_BALANZA_PIN, GPIO_INPUT);
	 /// timer
	timer_config_t timer_config = {
		.timer = TIMER_A,
		.period = TIMER_PERIOD_US,
		.func_p = TimeraHandler,
		.param_p = NULL
	};
	TimerInit(&timer_config);
	TimerStart(timer_config.timer);
	/// tareas
	xTaskCreate(ControlAguaTask, "Control Agua", 2048, NULL, 5, &medir_distancia_task_handle);
	xTaskCreate(ControlAlimentoTask, "Control Alimento", 2048, NULL, 5, NULL);
}
/*==================[end of file]============================================*/