#include "stdlib.h"
#include "stdint.h"
#include "configuracion.h"
#include "led.h"
#include <string.h>

#define USB_UART LPC_USART2
#define OVERRUN "ERROR OVERRUN\r\n"
#define PARITY "ERROR PARITY\r\n"
#define FRAMING "ERROR FRAMING\r\n"
#define BREAK "ERROR BREAK\r\n"
#define UNKNOWN "ERROR UNKNOWN\r\n"

static uint8_t actualizar = 0;

uint8_t translateError(uint8_t status);

void handleError(uint8_t error);

void handleData(uint8_t data, int8_t *contador);

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
	pUART->LCR = pUART->LCR | UART_LCR_DLAB_EN;

	//El Para obtener 9600bps uso el valor 1328 = 0x530
	//Configuro DLM y DLL
	pUART->DLM = 0x05;
	pUART->DLL = 0x30;

	//Borro todos los valores a configurar en bits 7 al 0
	pUART->LCR = pUART->LCR & 0xFFFFFF00;

	//Configuro los valores
	pUART->LCR = pUART->LCR
		| UART_LCR_PARITY_EN	//habilito paridad
		| UART_LCR_PARITY_EVEN	//paridad par
		| UART_LCR_WLEN8		//8 bits de palabra
		| UART_LCR_SBS_1BIT;	//1 bit de stop

	pUART->TER1 = pUART->TER1 | 0b1;

}

uint8_t UARTDisponible(LPC_USART_T *pUART)
{
	/*  COMPLETAR LA IMPLEMENTACION DE ESTA FUNCION  */
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
		//Veo a que error corresponde y lo traduzco
		*error = translateError(status);

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
	int8_t contador_display = 0; //inicializo el contador del display

	ConfigurarPuertosLaboratorio();
	ConfigurarInterrupcion();
	ConfigurarUART(USB_UART);

	while (1)
	{
		if (actualizar)
		{
			actualizar = 0;
			Led_On(GREEN_LED);

			/*  ESCRIBIR IMPLEMENTACION UARTDISPONIBLE Y ENVIAR LA CUENTA ACTUAL  */

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
 * @param error - Registro LSR con estado de error.
 * @return uint8_t
 **/
uint8_t translateError(uint8_t error)
{
	return 0;
}


/**
 * Manejo de error
 * @param error - El codigo de error segun translateError
 **/
void handleError(uint8_t error)
{
	//no hacemos nada con el error de momento
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