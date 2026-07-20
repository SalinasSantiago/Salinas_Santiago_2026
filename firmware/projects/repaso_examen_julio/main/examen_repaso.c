/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *- Sebe detectar la presencia de vehículos detrás de la bicicleta. Para ello
	se utiliza el sensor de ultrasonido
 Una tarea se encargara de le lectura del sensor de proximidad midiendo distancia

-  Se indicará mediante los leds de la placa la distancia de los vehículos a la bicicleta, encendiéndose de la siguiente manera
	Otra tarea se encargara de enceder los leds 
	● Led verde para distancias mayores a 5 metros
	● Led verde y amarillo para distancias entre 5 y 3 metros
	● Led verde, amariilo y rojo para distancias menores a 3 metros.

Otra tarea se encargare de la lectura del acelerómetro ubicado en el casco tiene la finalidad de detectar golpes o caídas. Se
trata de un acelerómetro analógico triaxial (3 canales), muestreado a 100Hz, con una
salida de 1.65 V para 0 G y una sensibilidad de 0.3V/G:
Si la sumatoria (escalar) de la aceleración en los tres ejes supera los 4G se deberá
enviar el siguiente mensaje a la aplicación:
● “Caída detectada”

- Otra tarea se encargara de enviar y hacer sonar las notificacones por notificación se envía utilizando un módulo bluetooth conectado al
segundo puerto serie de la placa ESP-EDU. Se enviarán los siguientes mensajes:
● “Precaución, vehículo cerca”, para distancias entre 3 a 5 metros
● “Peligro, vehículo cerca”, para distancias menores a 3 metros
Además de los mensajes se deberá activar una alarma sonora mediante un buzzer activo. Este
dispositivo suena cuando uno de los GPIOs de la placa se pone en alto y se apaga
cuando se pone en bajo. La alarma sonará con una frecuencia de 1 segundo en el caso de precaución y cada 0.5 segundos en el caso de peligro.


La solucion consistira en 3 tareas:
 Una para leer el sensor de proximidad usando freertos con una tarea es 2 veces por seg (2Hz) solo usare delaytask
 Para el acelerometro usare un timer para que me interrumpa cada 10ms (100Hz) y lea el acelerometro
 Finalmente una tarea para las notificacion tanto el buzzer, mensajes y leds, esta tarea se ejecutara cada 100ms (10Hz) y se encargara de encender los leds,
 enviar los mensajes y activar el buzzer segun la distancia leida por el sensor de proximidad y la caida detectada por el acelerometro.




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
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hc_sr04.h" 
#include "led.h"
#include "buzzer.h"
#include "uart_mcu.h"

#include "timer_mcu.h"
#include "analog_io_mcu.h"

/*==================[macros and definitions]=================================*/

#define LEJOS 500 
#define CERCANO 300

#define mensaje_precaucion "Precaución, vehículo cerca"
#define mensaje_peligro "Peligro, vehículo cerca"
#define mensaje_caida "Caída detectada"

#define PERIODEO_LECTURA_PROXIMIDAD 500000
#define PERIODO_LECTURA_ACELEROMETRO 10000
#define PERIODO_NOTIFICACIONES 100000

#define LED_ROJO GPIO_8
#define LED_AMARILLO GPIO_9
#define LED_VERDE GPIO_10

#define BUZZER GPIO_6 


/* Parametros de acelerometro */
//Para 0G se tinen 1.65V, y la sensibilidad es de 0.3V/G, por lo que para 4G se tendra un voltaje de 2.85V
#define UMBRAL_CAIDA_mV 2850
#define VOLTAJE_0G 1650
#define SENSIBILIDAD_mV_G 300



/*==================[internal data definition]===============================*/
uint16_t distancia = 0;
TaskHandle_t ControlCaidaTaskHandle = NULL;
uint16_t aceleracion_x = 0;
uint16_t aceleracion_y = 0; 
uint16_t aceleracion_z = 0;

uint16_t aceleracion_total = 0;
bool caida_detectada = false;
/*==================[internal functions declaration]=========================*/

void TimerLecturaAcelerometroCallback(void *arg){
    vTaskNotifyGiveFromISR(ControlCaidaTask, pdFALSE);
}

void ControlCaidaTask(void *pvParameters){
    
         AnalogInputReadSingle(CH0, &aceleracion_x);
         AnalogInputReadSingle(CH1, &aceleracion_y);
         AnalogInputReadSingle(CH2, &aceleracion_z);

        aceleracion_total = aceleracion_x + aceleracion_y + aceleracion_z;

        if (aceleracion_total > UMBRAL_CAIDA_mV){
            caida_detectada = true;
        } else {
            caida_detectada = false;
        }
}

/**
 * @brief Tarea para leer el sensor de proximidad y control de proximidad 
 *  Si detecta proximidad de un vehiculo, enciende los leds correspondientes y envia notificaciones
 * @param pvParameters  puntero a los parametros de la tarea
 */
void LecturaProximidadTask(void *pvParameters){
while(1){
   distancia = HcSr04ReadDistanceInCentimeters();
   vTaskDelay( PERIODEO_LECTURA_PROXIMIDAD / portTICK_PERIOD_MS);
}
}



/**
 * @brief Tarea para enviar notificaciones de proximidad y caida
 * 
 * @param pvParameters  puntero a los parametros de la tarea
 */
void ControlNotificacionesTask(void *pvParameters){
    while(1){
//  LED_3 = (1 << 0), /**< Color red. Routed to GPIO_5 */
   // LED_2 = (1 << 1), /**< Color yellow. Routed to GPIO_10 */
   /// LED_1 = (1 << 2), /**< Color green. Routed to GPIO_11 */
   LedsOffAll();
   if (distancia < CERCANO){
    LedOn(LED_3);
    LedOn(LED_2);
    LedOn(LED_1);
      BuzzerOn();
      UartSendString(UART_CONNECTOR, mensaje_peligro);
   } else if ( distancia < LEJOS){
      LedOn(LED_2);
      LedOn(LED_1);
      BuzzerOn();
      UartSendString(UART_CONNECTOR, mensaje_precaucion);
   } else {
      LedOn(LED_1);
      BuzzerOff();
   }

   if(caida_detectada){
      UartSendString(UART_CONNECTOR, mensaje_caida);}

    
    vTaskDelay(PERIODO_NOTIFICACIONES / portTICK_PERIOD_MS);
}
}



/*==================[external functions definition]==========================*/
void app_main(void){
	
    /** 
     * @brief Inicializacion de los perifericos
     */

     HC_SR04_Init(GPIO_3, GPIO_2);
     // Inicializacion de los leds
     LedInit();
    
     // Inicializacion del buzzer
     BuzzerInit(BUZZER);

     //Inicializo UART 
     serial_config_t uart_config = {
        .port = UART_CONNECTOR,
        .baud_rate = 115200,
        .func_p = NULL,
        .param_p = NULL			
    }
     UartInit( &uart_config);

     ///Inicializacion ADS
     /** 
      * typedef struct {			
    *  	adc_ch_t input;			/*!< Inputs: CH0, CH1, CH2, CH3 
	* adc_mode_t mode;		/*!< Mode: single read or continuous read
	* void *func_p;			/*!< Pointer to callback function for convertion end (only for continuous mode) 
	* void *param_p;			/*!< Pointer to callback function parameters (only for continuous mode) 
	uint16_t sample_frec;	/*!< Sample frequency min: 20kHz - max: 2MHz (only for continuous mode)  
    } analog_input_config_t;	
      * 
      */
    //Se configuran los ADC*/

	analog_input_config_t X = {
		.input = CH0,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = NULL,
	};
	AnalogInputInit(&X);

		analog_input_config_t Y = {
		.input = CH1,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = NULL,
	};
	AnalogInputInit(&Y);

		analog_input_config_t Z = {
		.input = CH2,
		.mode = ADC_SINGLE,
		.func_p = NULL,
		.param_p = NULL,
		.sample_frec = NULL,
	};
	AnalogInputInit(&Z);
     /// Inicializacion del timer para la lectura del acelerometro
     

    timer_config_t  timer_detecion_caida= {
        .timer = TIMER_C,
        .period = PERIODO_LECTURA_ACELEROMETRO,
        .callback = TimerLecturaAcelerometroCallback,
        .param_p = NULL};

    TimerInit(&timer_detecion_caida);

    /**
     * @brief cREACION DE LAS TAREAS
     */
     * 
     */

     xTaskCreate(LecturaProximidadTask, "LecturaProximidadTask", 2048, NULL, 1, NULL);
     xTaskCreate(ControlCaidaTask, "ControlCaidaTask", 2048, NULL, 1, &ControlCaidaTaskHandle);
     xTaskCreate(ControlNotificacionesTask, "ControlNotificacionesTask", 2048, NULL, 1, NULL);

     TimerStart(timer_detecion_caida.timer);
}    

/*==================[end of file]============================================*/