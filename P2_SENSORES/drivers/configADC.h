
#ifndef CONFIGADC_H_
#define CONFIGADC_H_

#include<stdint.h>

typedef struct
{
	uint16_t chan1;
	uint16_t chan2;
	uint16_t chan3;
	uint16_t chan4;
} MuestrasADC;

typedef struct
{
	uint32_t chan1;
	uint32_t chan2;
	uint32_t chan3;
	uint32_t chan4;
} MuestrasLeidasADC;


void ISR_SensorML(void);
void configADC_DisparaSensorM(void);
void configADC_LeeSensorM(uint16_t *datos);
void configADC_IniciaSensorML(void);


#endif /* CONFIGADC_H_ */
