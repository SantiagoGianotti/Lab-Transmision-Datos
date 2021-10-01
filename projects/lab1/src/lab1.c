#include "stdlib.h"
#include "stdint.h"
#include "configuracion.h"
#include "led.h"
#include <string.h>
#include <stdio.h>

#define USB_UART LPC_USART2
#define OVERRUN "ERROR OVERRUN\r\n"
#define PARITY "ERROR PARITY\r\n"
#define FRAMING "ERROR FRAMING\r\n"
#define BREAK "ERROR BREAK\r\n"
#define UNKNOWN "ERROR UNKNOWN\r\n"

static uint8_t actualizar = 0;

const char* translateError(uint8_t status);

void handleError(uint8_t error);

void handleData(uint8_t data, int8_t *contador);

void sendByte(LPC_USART_T *pUART, int8_t info);

void SysTick_Handler(void)
{
	static int contador = 0;

	contador++;
	if (contador % 500 == 0)
	{
		Led_Toggle(RED_LED);
	}
	if (contador % 1000 == 0)
	{
		contador = 0;
		actualizar = 1;
	}
}

void ConfigurarUART(LPC_USART_T *pUART)
{
	Chip_UART_Init(pUART);

	//Habilito la escritura del registro DLM y DLL
	//Utilizo una mascara para escribir en el bit 7
	pUART->LCR |= UART_LCR_DLAB_EN;

	//El Para obtener 9600bps uso el valor 1328 = 0x530
	//Configuro DLM y DLL
	pUART->DLM = 0x05;
	pUART->DLL = 0x30;

	//Borro todos los valores a configurar en bits 7 al 0
	pUART->LCR &= 0xFFFFFF00;

	//Configuro los valores
	pUART->LCR |= 
		UART_LCR_PARITY_EN	//habilito paridad
		| UART_LCR_PARITY_EVEN	//paridad par
		| UART_LCR_WLEN8		//8 bits de palabra
		| UART_LCR_SBS_1BIT;	//1 bit de stop

	pUART->TER2 = UART_TER2_TXEN;

}

uint8_t UARTDisponible(LPC_USART_T *pUART)
{
	//Reviso que el THRE este vacio y listo para enviar.
	return pUART->LSR & UART_LSR_THRE;
}

/**
 * La función devuelve:
 *    0 - Si no hay datos a ser leidos
 *    1 - Si un dato fue leido
 *    2 - Si hay error en la lectura
 * @param pUART Aca se introduce el USB_UART 
 * @param data Puntero a donde guardamos el dato
 * @param error Puntero donde guardamos el error
 * @return codigo
 **/
uint8_t UARTLeerByte(LPC_USART_T *pUART, uint8_t *data, uint8_t *error)
{
	uint8_t status = pUART->LSR;

	//Primero verifico si existe algun error
	if( status & UART_LSR_RXFE)
	{
		//Devuelvo el estado del lsr asi lo analizo en otro lugar
		//con una mascara me encargo de solo pasar la información de los errores.
		*error = status & (
			UART_LSR_OE
			| UART_LSR_PE
			| UART_LSR_FE
			| UART_LSR_BI
		);

		//Devuelvo 2 indicando que hay error.
		return 2;
	}

	if(status & UART_LSR_RDR)
	{
		//saco la info del buffer de recepcion
		*data = pUART->RBR;

		//Devuelvo 1 indicando que hay datos
		return 1;
	}

	//Devuelvo 0 indicando nulo
	return 0;
}

int main(void)
{
	uint8_t data;
	uint8_t error;

	//inicializo el contador del display como int con signo asi
	//programar el contador es mas sencillo.
	int8_t contador_display = 0;

	ConfigurarPuertosLaboratorio();
	ConfigurarInterrupcion();
	ConfigurarUART(USB_UART);

	while (1)
	{
		if (actualizar)
		{
			actualizar = 0;

			Led_On(GREEN_LED);

			if(UARTDisponible(USB_UART))
			{
				sendByte(USB_UART,contador_display);
			}

			Led_Off(GREEN_LED);
		}

		switch (UARTLeerByte(USB_UART, &data, &error))
		{
			case 0:
				//Si no hay caracteres no tengo nada para hacer.
				break;

			case 1:
				//Si hay caracteres manejo la información
				handleData(data, &contador_display);
				break;
			
			case 2:
				//Si hay error manejo el error
				handleError(error);
				break;
		}
	}
}

/**
 * Este metodo se encarga de enumerar el tipo de error encontrado.
 * de momento no hace nada ya que no hacemos nada con los errores en el lab.
 * NOTA: solo muestra un tipo de error a la vez, ya que es improbable que se de mas de uno.
 * @param error - Registro LSR con estado de error.
 * @return uint8_t
 **/
const char* translateError(uint8_t error)
{
	//Si es error de overrun
	if (error & UART_LSR_OE)
	{
		return OVERRUN;
	}

	//Si es error de paridad
	if (error & UART_LSR_PE)
	{
		return PARITY;
	}

	//Si es error de framing
	if (error & UART_LSR_FE)
	{
		return FRAMING;
	}

	//Si es error de breack
	if (error & UART_LSR_BI)
	{
		return BREAK;
	}

	return UNKNOWN;
}

/**
 * Manejo de error
 * @param error - El codigo de error segun translateError
 **/
void handleError(uint8_t error)
{
	//Printeo el error ya que no se definio ninguna conducta.
	printf("%s", translateError(error));
}

/**
 * Manejo de datos
 * @param data - El dato a ser procesado.
 **/
void handleData(uint8_t data, int8_t *contador)
{

	switch (data)
	{
		//aumento la cuenta
		case 'q':
			*contador++;
			break;
		//disminuyo la cuenta
		case 'w':
			*contador--;
			break;

		//reinicio la cuenta
		case 'e':
			*contador = 0;
			break;
	}

	//Verifico que el contador no se pase de 100 o 0.
	if( *contador > 99 )
	{
		*contador = 0;
	}
	else if( *contador < 0 )
	{
		*contador = 99;
	}
}

void sendByte(LPC_USART_T *pUART, int8_t info)
{
	//hago un cast al byte para que se envie sin signo.
	pUART->THR = (uint32_t) info;
}