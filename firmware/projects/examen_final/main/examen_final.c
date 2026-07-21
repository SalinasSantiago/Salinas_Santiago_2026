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
 * |   UART_TX	   	| 	UART_RX		|
 * |   UART_RX	   	| 	UART_TX		|
 * |   PIN Electrovalvula	GPIO_19 |
 * |  PIN  Balanza	| 	GPIO_20	|
 * |   Tecla 1	   	| 	GPIO_4		|
 * |   LED 1	   	| 	GPIO_18		|
 * |  PIN Alimento	| 	GPIO_6		|
 * 
 * 
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 21/7/2026 | Document creation		                         |
 *
 * @author Santiago Salinas Sosa santiago.salinas@ingenieria.edu.ar
 *
 */ 
/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"
#include "switch.h"	

#include "hc_sr04.h"
#include "uart_mcu.h"
#include "gpio_mcu.h"

#include "analog_io_mcu.h"


/*==================[macros and definitions]=================================*/
#define DELAY_TASK_MS 5000 //delay de 5 segundos para las tareas de control de agua, alimento y uart
#define GPIO_TRIGGER_PIN GPIO_2
#define GPIO_ECHO_PIN GPIO_3

#define GPIO_ELECTRO_VALVULA_PIN GPIO_19  
#define GPIO_BALANZA_PIN GPIO_20

#define GPIO_ALIMENTO_PIN GPIO_6

#define LED_1 GPIO_11
#define SWITCH_1 GPIO_4
/*==================[internal data definition]===============================*/



///La balanza analógica nos devolverá una señal de 0,0V cuando no tenga carga y 3,3V
///cuando alcance su máximo de capacidad (1.000g). Las mediciones de peso se deben
///realizar cada 5 segundos. 0.0v 0g entonces 50g son 0.165v, 500g son 1.65v
uint16_t valor_analogico = 0.0;


//** Variables para almacenar los valores de peso y volumen  enviar por UART*/
float volumen_agua = 0.0;
float peso = 0.0;

//** Variable para controlar el sistema */
bool tecla1 = false; 

/*==================[internal functions declaration]=========================*/


/**
 * @brief Handler de interrupción de la tecla 1 (TEC1)
 *  se debe utilizar la tecla 1 para iniciar y 
	detener el sistema, y el LED_1 para indicar cuando el mismo está encendido.
 */


void Tecla1Handler(){
	
    tecla1 = !tecla1;   // Activa o detiene la medición
	if(tecla1){
		LedOn(LED_1);
	}
	else{
		LedOff(LED_1);
	}
}


/**
 * @brief 
 * La balanza analógica nos devolverá una señal de 0,0V
	cuando no tenga carga y 3,3V cuando alcance su máximo de capacidad (1.000g).
	Las mediciones de peso se deben realizar cada 5 segundos.
 * @param pvParameters 
 */
void ControlAlimentoTask(void* pvParameters){
	
	while(1){
		AnalogInputReadSingle(CH0, &valor_analogico);
		if(tecla1){  

		peso = (valor_analogico * 1000.0) / 3300.0; 
		if (peso < 50.0){
			GPIOOn(GPIO_ALIMENTO_PIN);
		} else if (peso > 500.0){
			GPIOOff(GPIO_ALIMENTO_PIN);
		}
	}
}

	vTaskDelay(pdMS_TO_TICKS(DELAY_TASK_MS));
}

/** @brief Tarea encargada de controlar el suministro de agua activando o
 *  desactivando la electroválvula según el nivel de agua en el recipiente	
 *  debe controlar el suministro de agua. El recipiente de agua es de aproximadamente 3000 cm3 de capacidad
	 (20cm de diámetro y 9,5cm de altura). Para el llenado del recipiente se debe activar una electroválvula 
	 (poniendo en alto un GPIO a elección). La electroválvula se debe accionar cuando se detecte que el nivel
	  en recipiente cae por debajo del medio litro. La electroválvula se debe cerrar cuando en el recipiente se alcancen los 2500cm3. 
Para medir el nivel de agua en el recipiente se utiliza un HC-SR04 ubicado por encima del recipiente, 
a 30 cm de distancia de la base del mismo. Las mediciones de nivel de agua se deben realizar cada 5 segundos.
 */
void ControlAguaTask(void* pvParameters ){
	
	float distancia;
	float nivel_agua;
	const float radio = 10.0; // r
	const float area = 3.1415 * radio * radio; // 	
	

	while(1){
		

		if(tecla1){
			 
		
		distancia = HcSr04ReadDistanceInCentimeters();
		nivel_agua =  9.5 - (30.0 - distancia); 
		volumen_agua = area * nivel_agua;

		if (volumen_agua < 500.0){
			GPIOOn(GPIO_ELECTRO_VALVULA_PIN);
		} else if (volumen_agua > 2500.0){
			GPIOOff(GPIO_ELECTRO_VALVULA_PIN);
		}
	}
	}
	delayTask(pdMS_TO_TICKS(DELAY_TASK_MS));

}


/** @brief Tarea encargada de los valores de volumen y peso por UART cada 5 segundos
 * 
 */
void ReporteUartTask(void* pvParameters){
    char mensaje[100];
    """ el sistema debe informar, cada 5 segundos, a través de la UART el estado de ambos recipientes.
	 Para esto se deben transmitir por el puerto serie a la PC mensajes con el formato:
	Agua: xxx cm3, Alimento: xxx gr
	"""
    while(1){
        
        
        if (sistema_activo) {
            LedOn(LED_1); // Indica que está encendido
            sprintf(mensaje, "Agua: %.0f cm3, Alimento: %.0f gr\r\n", volumen, peso);
            UartSendString(UART_PC, mensaje);
        } else {
            LedOff(LED_1);
        }
    }
	delayTask(pdMS_TO_TICKS(DELAY_TASK_MS));
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
        .input = CH0, 
        .mode = ADC_SINGLE, 
        .func_p = NULL,         
        .param_p = NULL,
        .sample_frec = 0
    };  
    AnalogInputInit(&adc_config); 


	// Configuracion UART 
    serial_config_t my_uart = {
        .port = UART_PC,
        .baud_rate = 115200, /*!< baudrate (bits per second) */
        .func_p = NULL,      /*!< Pointer to callback function to call when receiving data (= UART_NO_INT if not requiered)*/
        .param_p = NULL      /*!< Pointer to callback function parameters */
    };
    UartInit(&my_uart);

	
	/// tareas
	/** 
	 * @brief Definicion de tarea de control de agua 
	 */
	xTaskCreate(ControlAguaTask, "Control Agua", 2048, NULL, 5, NULL);
	/**
	 * @brief Definicion de tarea de control de aliemnto
	 * 
	 */
	xTaskCreate(ControlAlimentoTask, "Control Alimento", 2048, NULL, 5, NULL);

	/**
	 * @brief Definicion de tarea de reporte por UART
	 * 
	 */
	xTaskCreate(ReporteUartTask, "Reporte UART", 2048, NULL, 5, NULL);

	/** @brief Definicion de ISR para la tecla 1 y activar led
	 * 
	 */
	


}
/*==================[end of file]============================================*/