 /******************************************************************************
 *
 * Module: UART
 *
 * File Name: uart.c
 *
 * Description: Source file for the UART Tiva driver
 *
 * Author: Ahmed Wael Ibrahim Mohamed - 20P3343
 *
 *******************************************************************************/

#include "uart.h"
#include "tm4c123gh6pm.h"
#include "bitwise_operation.h"

/*******************************************************************************
 *                      Functions Definitions                                  *
 *******************************************************************************/
/*
 * Description :
 * Functional responsible for Initialize the UART device by:
 * 1. Setup the Frame format like number of data bits, parity bit type and number of stop bits.
 * 2. Enable the UART.
 * 3. Setup the UART baud rate.
 */
void UART_init(const UART_ConfigType *config)
{
    /* Providing clock to UART Module 5 */
    SET_BIT(SYSCTL_RCGCUART_R, 5);
    /* Wait till UART Module 5 is ready */
    while(BIT_IS_CLEAR(SYSCTL_PRUSB_R, 5));

    /* Enable PortE in Run Mode -- Port containing UART Module 5 */
    SET_BIT(SYSCTL_RCGCGPIO_R, 4);
    /* Wait till PortA is ready */
    while(BIT_IS_CLEAR(SYSCTL_PRGPIO_R, 4));

    /* Enabling UART
        -- You need to disable UARTEN before any configurations
        UARTCTL[0] is UARTEN 
     */
    CLEAR_BIT(UART5_CTL_R, 0);

    /* Setting PA0(RX) PA1(TX) Pins to be used for alternated actions */
    SET_BIT(GPIO_PORTE_AFSEL_R, 4);
    SET_BIT(GPIO_PORTE_AFSEL_R, 5);
    /* PUR, PDR Registers are not used */

    /* Setting RX pin as input */
    CLEAR_BIT(GPIO_PORTE_DIR_R, 4);
    /* Setting TX pin as output */
    SET_BIT(GPIO_PORTE_DIR_R, 5);

    /* Setting RX pin as input */
    CLEAR_BIT(GPIO_PORTE_DEN_R, 4);
    /* Setting TX pin as output */
    SET_BIT(GPIO_PORTE_DEN_R, 5);

    /* Setting the port multiplexing control of portA to use UART */
    GPIO_PORTA_PCTL_R |= (GPIO_PORTA_PCTL_R & 0xFF00FFFF) | 0x00110000;

    /* Setting Baud Rate = 9600
        -- BRD = Fck / (CLK_DIV * baud_rate);
        -- CLK_DIV is 8 or 16 
        16x10^6 / (16 * 9600) = 104.16667
        Integer Baud Rate Data = 104
    */
    UART5_IBRD_R = (UART5_IBRD_R & 0xFFFF0000) | 0x68;
    
    /* Setting Baud Rate = 9600
        Fraction Baud Rate Data = 0.1667*65 = 10.67 == 11 = 0xB
    */
    UART5_FBRD_R = (UART5_FBRD_R & 0xFFFFFFC0) | 0x0B;

    /* Setting the word length of the data to be transmitted
        -- BITS(5-6): Word Length (0x3 = 8 bits)
        -- BITS(4): FIFO Enable (1 to enable)
     */
    UART5_LCRH_R = (UART5_LCRH_R & 0xFFFFFF00)
                    | ((config->s_mode)<<5)
                    | ((config->s_stop)<<3)
                    | ((config->s_parity)<<2)
                    | UART_LCRH_FEN;

    /* Enabling Parity */
    if ((config->s_parity) == DISABLED)
        CLEAR_BIT(UART5_LCRH_R, 1);
    else
        SET_BIT(UART5_LCRH_R, 1);

    /* Enable UART Receive/Transmit and UART module enable */
    SET_BIT(UART5_CTL_R, UART_TRANSMIT_EN);  /* Enable UART5 Transmit */
    SET_BIT(UART5_CTL_R, UART_RECEIVE_EN);   /* Enable UART5 Receive */
    SET_BIT(UART5_CTL_R, UART_MODULE_EN);    /* Enable UART5 Module */
}

/*
 * Description :
 * Functional responsible for send byte to another UART device.
 */
void UART_sendByte(const uint8 data)
{
	/* UART5 TX FIFO FULL FLAG is set indicating that the FIFO is full
        so wait till the FIFO is empty then fill it with new data
	 */
	while(BIT_IS_SET(UART5_FR_R,FIFO_TRANSMIT_FULL)){}

	/*
	 * Put the required data in the UDR register and it also clear the UDRE flag as
	 * the UDR register is not empty now
	 */
	UART5_DR_R = data;
}

/*
 * Description :
 * Functional responsible for receive byte from another UART device.
 */
uint8 UART_recieveByte(void)
{
	/* UART5 RX FIFO FULL FLAG is set indicating that the FIFO is full
        so wait till the FIFO is empty then read the newly filled data
	 */
    while(BIT_IS_SET(UART5_FR_R,FIFO_RECEIVE_FULL)){}

	/*
	 * Read the received data from the Rx buffer (UDR)
	 * The RXC flag will be cleared after read the data
	 */
    return UART5_DR_R;		
}

/*
 * Description :
 * Send the required string through UART to the other UART device.
 */
void UART_sendString(const uint8 *Str)
{
	uint8 i = 0;

	/* Send the whole string */
	while(Str[i] != '\0')
	{
		UART_sendByte(Str[i]);
		i++;
	}
}

/*
 * Description :
 * Receive the required string until the '#' symbol through UART from the other UART device.
 */
void UART_receiveString(uint8 *Str)
{
	uint8 i = 0;

	/* Receive the first byte */
	Str[i] = UART_recieveByte();

	/* Receive the whole string until the '#' */
	while(Str[i] != '#')
	{
		i++;
		Str[i] = UART_recieveByte();
	}

	/* After receiving the whole string plus the '#', replace the '#' with '\0' */
	Str[i] = '\0';
}