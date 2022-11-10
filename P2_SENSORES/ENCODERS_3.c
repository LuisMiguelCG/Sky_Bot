
//Librerias
#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"       // TIVA: Definiciones del mapa de memoria
#include "inc/hw_types.h"        // TIVA: Definiciones API
#include "inc/hw_ints.h"         // TIVA: Definiciones para configuracion de interrupciones
#include "driverlib/gpio.h"      // TIVA: Funciones API de GPIO
#include "driverlib/pin_map.h"   // TIVA: Mapa de pines del chip
#include "driverlib/rom.h"       // TIVA: Funciones API incluidas en ROM de micro (MAP_)
#include "driverlib/rom_map.h"   // TIVA: Mapeo automatico de funciones API incluidas en ROM de micro (MAP_)
#include "driverlib/sysctl.h"    // TIVA: Funciones API control del sistema
#include "driverlib/uart.h"      // TIVA: Funciones API manejo UART
#include "driverlib/interrupt.h" // TIVA: Funciones API manejo de interrupciones
#include "utils/uartstdioMod.h"     // TIVA: Funciones API UARTSTDIO (printf)
#include "drivers/buttons.h"     // TIVA: Funciones API manejo de botones
#include "drivers/rgb.h"         // TIVA: Funciones API manejo de RGB
#include "driverlib/pwm.h"       // TIVA: Funciones API manejo PWM
#include "driverlib/adc.h"       // TIVA: Funciones API manejo del ADC
#include "driverlib/timer.h"     // TIVA: Funciones API manejo de Timers
#include "FreeRTOS.h"            // FreeRTOS: definiciones generales
#include "task.h"                // FreeRTOS: definiciones relacionadas con tareas
#include "semphr.h"              // FreeRTOS: definiciones para uso de semaforos
#include "queue.h"               // FreeRTOS: definiciones para uso de ola de mensajes
#include "Configuracion.h"       // Configuracion de perifericos
#include "drivers/configADC.h"   // Configuracion del adc
#include "commands.h"
#include "utils/cpu_usage.h"


//Defines



// Definiciones de tareas


// Variables globales "main"

uint32_t g_ui32CPUUsage;
uint32_t g_ui32SystemClock;
uint32_t estado = PARADA;


//Importante: Los arrays con los parametros tengo que poner como globales !!!
// por que? se pasan por referencia a la funcion TaskCreate,
// como son variables locales automaticas, cuando se llega a ajecutar la tarea
// la tarea la memoria reservada para estas variables ya no existe (el compilador interpreta que ya no se usan tras llamar
// a task create  (falla incluso sin optimizacion y aunque se copien al comenzar la tarea).


//Variables globales tareas e interrupciones

xSemaphoreHandle semaforo_motor;
xSemaphoreHandle semaforo_adc;
uint8_t c_vueltas_drc = 0;
uint8_t c_vueltas_izq = 0;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, unsigned long ulLine)
{
}

#endif

//*****************************************************************************
//
// This hook is called by FreeRTOS when an stack overflow error is detected.
//
//*****************************************************************************
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    //
    // This function can not return, so loop forever.  Interrupts are disabled
    // on entry to this function, so no processor interrupts will interrupt
    // this loop.
    //
    while(1)
    {
    }
}

//Esto se ejecuta cada Tick del sistema. LLeva la estadistica de uso de la CPU (tiempo que la CPU ha estado funcionando)
void vApplicationTickHook( void )
{
    static uint8_t count = 0;

    if (++count == 10)
    {
        g_ui32CPUUsage = CPUUsageTick();
        count = 0;
    }
    //return;
}

//Esto se ejecuta cada vez que entra a funcionar la tarea Idle
void vApplicationIdleHook (void)
{
    SysCtlSleep();
}


//Esto se ejecuta cada vez que entra a funcionar la tarea Idle
void vApplicationMallocFailedHook (void)
{
    while(1);
}

//*****************************************************************************
//
// This task toggles the user selected LED at a user selected frequency.
//
//*****************************************************************************

static portTASK_FUNCTION(ChoqueTask, pvParameters ){

    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_PWM1);

        //
        // Loop forever.
        //
        while(1)
        {
            xSemaphoreTake(semaforo_motor,portMAX_DELAY);


        }
}

static portTASK_FUNCTION(ADCTask,pvParameters)
{
    uint16_t Valor_SenM;
    unsigned short ind;

    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_PWM1);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    //
    // Bucle infinito, las tareas en FreeRTOS no pueden "acabar", deben "matarse" con la funcion xTaskDelete().
    //
    while(1)
    {
        xSemaphoreTake(semaforo_adc, portMAX_DELAY);

    }
}




//*****************************************************************************
//
// Initialize FreeRTOS and start the initial set of tasks.p
//
//*****************************************************************************
int main(void)
{

    //
    // Set the clocking to run at 40 MHz from the PLL.
    //
    MAP_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
            SYSCTL_OSC_MAIN);


    Configuracion();
    configADC_IniciaSensorML();   //Configura el adc
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);



    // Get the system clock speed.
    g_ui32SystemClock = SysCtlClockGet();

    //Habilita el clock gating de los perifericos durante el bajo consumo
    ROM_SysCtlPeripheralClockGating(false);

    // Inicializa el subsistema de medida del uso de CPU (mide el tiempo que la CPU no este dormida)
    // Para eso utiliza un timer, que aqui hemos puesto que sea el TIMER3 (ultimo parametro que se pasa a la funcion)
    // (y por tanto este no se deberia utilizar para otra cosa).
    // El timer 0 y 1 se utilizan para los LEDS
    CPUUsageInit(g_ui32SystemClock, configTICK_RATE_HZ/10, 3);

    //Crea la tarea que maneja el choque
    if(xTaskCreate(ChoqueTask, "Choque", 512, NULL, tskIDLE_PRIORITY + 2, NULL) != pdTRUE){

        while(1);
    }

    //Crea la tarea del ADC
    if((xTaskCreate(ADCTask, (portCHAR *)"ADC", 256, NULL, tskIDLE_PRIORITY + 1, NULL) != pdTRUE)){

        while(1);

    }

    //Crea los semaforos compartidos por tareas e ISR
     semaforo_motor=xSemaphoreCreateBinary();
     if ((semaforo_motor==NULL))
     {
         while (1);  //No hay memoria para los semaforo
     }

     semaforo_adc=xSemaphoreCreateBinary();
      if ((semaforo_adc==NULL))
      {
          while (1);  //No hay memoria para los semaforo
      }

    //
    // Start the scheduler.  This should not return.
    //
    vTaskStartScheduler();  //el RTOS habilita las interrupciones al entrar aqui, asi que no hace falta habilitarlas

    //
    // In case the scheduler returns for some reason, print an error and loop
    // forever.
    //

    while(1)
    {
    }
}


// Rutinas de interrupcion
void INT_SENSOR(void){

    int32_t i32Status = GPIOIntStatus(GPIO_PORTB_BASE, SENSOR_CONTAC|ENCODER_IZQ|ENCODER_DRC|SENSOR_BAJO);

    //Int del enconder izquierdo
    if(i32Status & ENCODER_DRC){

        c_vueltas_drc = (c_vueltas_drc +1) %36;

    }

    if(i32Status & ENCODER_IZQ){

        c_vueltas_izq = (c_vueltas_izq +1) %36;

    }

    GPIOIntClear(GPIO_PORTB_BASE, SENSOR_CONTAC|ENCODER_IZQ|ENCODER_DRC|SENSOR_BAJO);  //limpiamos flags

}

void STOP_MOVE(void){

    int32_t i32Status = GPIOIntStatus(GPIO_PORTF_BASE,ALL_BUTTONS);

}


void PERIOD_TIMER_ADC(void){

    BaseType_t higherPriorityTaskWoken=pdFALSE;

    xSemaphoreGiveFromISR(semaforo_adc, &higherPriorityTaskWoken);

    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    portEND_SWITCHING_ISR(higherPriorityTaskWoken);

}

void PERIOD_TIMER2_A(void){

    BaseType_t higherPriorityTaskWoken=pdFALSE;

    TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);

    portEND_SWITCHING_ISR(higherPriorityTaskWoken);

}



