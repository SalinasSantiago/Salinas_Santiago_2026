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
#include "led.h"
#include "switch.h"	
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

/** @brief Handler para activar la tarea de medir distancia */
TaskHandle_t medir_distancia_task_handle = NULL;
/** @brief Handle para activar la tarea de controlar el alimento */
TaskHandle_t controlar_alimento_task_handle = NULL; 
/** @brief Handler para activar la tarea de nviar mensajes por uart */
TaskHandle_t uart_task_handle = NULL;	


///La balanza analógica nos devolverá una señal de 0,0V cuando no tenga carga y 3,3V
///cuando alcance su máximo de capacidad (1.000g). Las mediciones de peso se deben
///realizar cada 5 segundos. 0.0v 0g entonces 50g son 0.165v, 500g son 1.65v
uint16_t valor_analogico = 0.0;
#define DEFADC_BALANZA_CH CH0

//** Variables para almacenar los valores a enviar por UART*/
float volumen_agua_uart = 0.0;
float peso_alimento_uart = 0.0;

//** Variable para controlar el sistema */
bool tecla1 = false; 

/*==================[internal functions declaration]=========================*/
/** @brief Handler para el temporizador general 
 * Este temporizador se activa cada 5 se00000	gundos y notifica a las tareas de medir distancia y controlar el alimento para que realicen sus respectivas acciones.
 */
*/


	

void TimerGeneralHandler(void *param){
	vTaskNotifyGiveFromISR(medir_distancia_task_handle, NULL);
	vTaskNotifyGiveFromISR(controlar_alimento_task_handle, NULL);
	vTaskNotifyGiveFromISR(uart_task_handle, NULL);
}


/**
 * @brief Handler de interrupción de la tecla 1 (TEC1)
 */
void Tecla1Handler(){
    tecla1 = !tecla1;   // Activa o detiene la medición
}



void ControlAlimentoTask(void* pvParameters){
	float peso;
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		 AnalogInputReadSingle(ADC_BALANZA_CH, &valor_analogico)
		if(tecla1){  
		peso = (valor_analogico * 1000.0) / 3300.0; // Convertir el valor analógico a peso en gramos
		peso_alimento_uart = peso;
		if (peso < 50.0){
			GPIOOn(GPIO_ALIMENTO_PIN);
		} else if (peso > 500.0){
			GPIOOff(GPIO_ALIMENTO_PIN);
		}
	}}
}

/** @brief Handler para la tarea de controlar el agua 		
 * 
 */
void ControlAguaTask(void* pvParameters ){

	float distancia;
	float nivel_agua;
	float volumen_agua;
	const float area = 314.16; 

	while(1){
		
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if(tecla1){
			 
		
		distancia = HcSr04ReadDistanceInCentimeters();
		nivel_agua = 30.0 - distancia;
		volumen_agua = area * nivel_agua;
		volumen_agua_uart = volumen_agua;

		if (volumen_agua < 500.0){
			GPIOOn(GPIO_ELECTRO_VALVULA_PIN);
		} else if (volumen_agua > 2500.0){
			GPIOOff(GPIO_ELECTRO_VALVULA_PIN);
		}
	}
	}


}



void ReporteUartTask(void* pvParameters){
    char mensaje[100];
    
    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        if (sistema_activo) {
            LedOn(LED_1); // Indica que está encendido
            sprintf(mensaje, "Agua: %.0f cm3, Alimento: %.0f gr\r\n", volumen_agua_uart, peso_alimento_uart);
            UartSendString(UART_PC, mensaje);
        } else {
            LedOff(LED_1);
        }
    }
}

/*==================[external functions definition]==========================*/
void app_main(void){
	 
	 GPIOInit(GPIO_ELECTRO_VALVULA_PIN, GPIO_OUTPUT);
	 HcSr04Init(GPIO_ECHO_PIN, GPIO_TRIGGER_PIN);  
	 GPIOInit(GPIO_ALIMENTO_PIN, GPIO_OUTPUT);
	 GPIOInit(GPIO_BALANZA_PIN, GPIO_INPUT);  
	LedsInit();	
	SwitchsInit();
	 

	 analog_input_config_t adc_config = {
        .input = ADC_BALANZA_CH, 
        .mode = ADC_SINGLE, 
        .func_p = NULL,         
        .param_p = NULL,
        .sample_fnc = 0
    };  
    AnalogInputInit(&adc_config); 


	// Para la uart
    serial_config_t my_uart = {
        .port = UART_PC,
        .baud_rate = 115200, /*!< baudrate (bits per second) */
        .func_p = NULL,      /*!< Pointer to callback function to call when receiving data (= UART_NO_INT if not requiered)*/
        .param_p = NULL      /*!< Pointer to callback function parameters */
    };
    UartInit(&my_uart);

	 /// timer
	timer_config_t timer_config = {
		.timer = TIMER_A,
		.period = TIMER_PERIOD_US,
		.func_p = TimerAguaHandler,
		.param_p = NULL
	};
	TimerInit(&timer_config);
	TimerStart(timer_config.timer);
	/// tareas
	xTaskCreate(ControlAguaTask, "Control Agua", 2048, NULL, 5, &medir_distancia_task_handle);
	xTaskCreate(ControlAlimentoTask, "Control Alimento", 2048, NULL, 5, &controlar_alimento_task_handle);
}
/*==================[end of file]============================================*/