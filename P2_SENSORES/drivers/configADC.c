#include<stdint.h>
#include<stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_adc.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"
#include "configADC.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"




static QueueHandle_t cola_sensor;



//Provoca el disparo de una conversion (hemos configurado el ADC con "disparo software" (Processor trigger)
void configADC_DisparaSensorM(void)
{
    ADCProcessorTrigger(ADC0_BASE,1);

}

void configADC_IniciaSensorML(void)
{
			    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
			    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_ADC0);

				//HABILITAMOS EL GPIOE
				SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
				SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOE);
				// Enable pin PE3|PE2 for ADC AIN0|AIN1
				GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);


				//CONFIGURAR SECUENCIADOR 1
				ADCSequenceDisable(ADC0_BASE,1);

				//Configuramos la velocidad de conversion al maximo (1MS/s)
				ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_RATE_FULL, 1);

				ADCSequenceConfigure(ADC0_BASE,1,ADC_TRIGGER_PROCESSOR,0);	//Disparo software (processor trigger)
				ADCSequenceStepConfigure(ADC0_BASE,1,0,ADC_CTL_CH0);
				ADCSequenceStepConfigure(ADC0_BASE,1,1,ADC_CTL_CH0);
				ADCSequenceStepConfigure(ADC0_BASE,1,2,ADC_CTL_CH0);
				ADCSequenceStepConfigure(ADC0_BASE,1,3,ADC_CTL_CH0 |ADC_CTL_IE |ADC_CTL_END );	//La ultima muestra provoca la interrupcion
				ADCSequenceEnable(ADC0_BASE,1); //ACTIVO LA SECUENCIA

				//Habilita las interrupciones
				ADCIntClear(ADC0_BASE,1);
				ADCIntEnable(ADC0_BASE,1);
				IntPrioritySet(INT_ADC0SS1,configMAX_SYSCALL_INTERRUPT_PRIORITY);
				IntEnable(INT_ADC0SS1);

				//Creamos una cola de mensajes para la comunicacion entre la ISR y la tara que llame a configADC_LeeADC(...)
                cola_sensor=xQueueCreate(8,sizeof(uint16_t));
				if (cola_sensor==NULL)
				{
					while(1);
				}
}


void configADC_LeeSensorM(uint16_t *datos)
{
	xQueueReceive(cola_sensor,datos,portMAX_DELAY);
}


void ISR_SensorML(void)
{
	portBASE_TYPE higherPriorityTaskWoken=pdFALSE;

	MuestrasLeidasADC Leidas_SenM;
	MuestrasADC Finales_SenM;

	uint16_t Media_SenM;

    ADCIntClear(ADC0_BASE,1);                                       //LIMPIAMOS EL FLAG DE INTERRUPCIONES
    ADCSequenceDataGet(ADC0_BASE,1,(uint32_t *)&Leidas_SenM);       //COGEMOS LOS DATOS GUARDADOS

    //Pasamos de 32 bits a 16 (el conversor es de 12 bits, así que sólo son significativos los bits del 0 al 11)
    Finales_SenM.chan1=Leidas_SenM.chan1;
    Finales_SenM.chan2=Leidas_SenM.chan2;
    Finales_SenM.chan3=Leidas_SenM.chan3;
    Finales_SenM.chan4=Leidas_SenM.chan4;

    Media_SenM = (Finales_SenM.chan1 + Finales_SenM.chan2 + Finales_SenM.chan3 + Finales_SenM.chan4)/4;

    //Guardamos en la cola
    xQueueSendFromISR(cola_sensor,&Media_SenM,&higherPriorityTaskWoken);
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);


}
