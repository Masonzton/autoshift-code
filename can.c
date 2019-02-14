//somethings wrong with this
//probably with the can stuff
//or the wiring
#include "system.h"
#include "usart.h"
#include "can.h"

void sendData_com(char* data, int len);
#define print(s) sendData_com(s,sizeof(s))

static struct usart_module usart_instance;
static struct can_module can_instance;
static struct usart_module com_instance;

#define CAN_RX_STANDARD_FILTER_INDEX_0    0
#define CAN_RX_STANDARD_FILTER_INDEX_1    1
#define CAN_RX_STANDARD_FILTER_ID_0     0x45A
#define CAN_RX_STANDARD_FILTER_ID_0_BUFFER_INDEX     2
#define CAN_RX_STANDARD_FILTER_ID_1     0x469
#define CAN_RX_EXTENDED_FILTER_INDEX_0    0
#define CAN_RX_EXTENDED_FILTER_INDEX_1    1
#define CAN_RX_EXTENDED_FILTER_ID_0     0x100000A5
#define CAN_RX_EXTENDED_FILTER_ID_0_BUFFER_INDEX     1
#define CAN_RX_EXTENDED_FILTER_ID_1     0x10000096
#define CAN_TX_BUFFER_INDEX    0
static uint8_t tx_message_0[CONF_CAN_ELEMENT_DATA_SIZE];
static uint8_t tx_message_1[CONF_CAN_ELEMENT_DATA_SIZE];
static volatile uint32_t standard_receive_index = 0;
static volatile uint32_t extended_receive_index = 0;
static struct can_rx_element_fifo_0 rx_element_fifo_0;
static struct can_rx_element_fifo_1 rx_element_fifo_1;
static struct can_rx_element_buffer rx_element_buffer;

#define CAN_MODULE              CAN0
#define CAN_TX_PIN              PIN_PA24G_CAN0_TX
#define CAN_TX_MUX_SETTING      MUX_PA24G_CAN0_TX
#define CAN_RX_PIN              PIN_PA25G_CAN0_RX
#define CAN_RX_MUX_SETTING      MUX_PA25G_CAN0_RX

#define CDC_MODULE              SERCOM1
#define CDC_SERCOM_MUX_SETTING  USART_RX_1_TX_0_XCK_1
#define CDC_SERCOM_PINMUX_PAD0  PINMUX_PA00D_SERCOM1_PAD0
#define CDC_SERCOM_PINMUX_PAD1  PINMUX_PA01D_SERCOM1_PAD1

/*void configure_usart_cdc(void)
{
	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);
	config_usart.baudrate    = 9600;
	config_usart.mux_setting = CDC_SERCOM_MUX_SETTING;
	config_usart.pinmux_pad0 = CDC_SERCOM_PINMUX_PAD0;
	config_usart.pinmux_pad1 = CDC_SERCOM_PINMUX_PAD1;
	//config_usart.pinmux_pad2 = CDC_SERCOM_PINMUX_PAD2;
	//config_usart.pinmux_pad3 = CDC_SERCOM_PINMUX_PAD3;
	while (usart_init(&usart_instance,
	CDC_MODULE, &config_usart) != STATUS_OK) {
	}
	usart_enable(&usart_instance);
}*/

void configure_usart_com(void)
{
	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);
	config_usart.baudrate    = 9600;
	config_usart.mux_setting = CDC_SERCOM_MUX_SETTING;
	config_usart.pinmux_pad0 = PINMUX_PA00D_SERCOM1_PAD0;
	config_usart.pinmux_pad1 = PINMUX_PA01D_SERCOM1_PAD1;
	//config_usart.pinmux_pad2 = CDC_SERCOM_PINMUX_PAD2;
	//config_usart.pinmux_pad3 = CDC_SERCOM_PINMUX_PAD3;
	while (usart_init(&com_instance,
	SERCOM1, &config_usart) != STATUS_OK) {
	}
	usart_enable(&com_instance);
}

void sendData_com(char* data, int len){
	usart_write_buffer_wait(&com_instance, data, len);
}

/*void sendData_usart(char* data, int len){
	usart_write_buffer_wait(&usart_instance, data, len);
}*/

static void configure_can(void)
{
	uint32_t i;
	/* Initialize the memory. */
	for (i = 0; i < CONF_CAN_ELEMENT_DATA_SIZE; i++) {
		tx_message_0[i] = i;
		tx_message_1[i] = i + 0x80;
	}
	/* Set up the CAN TX/RX pins */
	struct system_pinmux_config pin_config;
	system_pinmux_get_config_defaults(&pin_config);
	pin_config.mux_position = CAN_TX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN_TX_PIN, &pin_config);
	pin_config.mux_position = CAN_RX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN_RX_PIN, &pin_config);
	/* Initialize the module. */
	struct can_config config_can;
	can_get_config_defaults(&config_can);
	can_init(&can_instance, CAN_MODULE, &config_can);
	can_enable_fd_mode(&can_instance);
	can_start(&can_instance);
	/* Enable interrupts for this CAN module */
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN0);
	can_enable_interrupt(&can_instance, CAN_PROTOCOL_ERROR_ARBITRATION
	| CAN_PROTOCOL_ERROR_DATA);
}
static void can_set_standard_filter_0(void)
{
	struct can_standard_message_filter_element sd_filter;
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID2 = CAN_RX_STANDARD_FILTER_ID_0_BUFFER_INDEX;
	sd_filter.S0.bit.SFID1 = CAN_RX_STANDARD_FILTER_ID_0;
	sd_filter.S0.bit.SFEC =
	CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(&can_instance, &sd_filter,
	CAN_RX_STANDARD_FILTER_INDEX_0);
	can_enable_interrupt(&can_instance, CAN_RX_BUFFER_NEW_MESSAGE);
}
static void can_set_standard_filter_1(void)
{
	struct can_standard_message_filter_element sd_filter;
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID1 = CAN_RX_STANDARD_FILTER_ID_1;
	can_set_rx_standard_filter(&can_instance, &sd_filter,
	CAN_RX_STANDARD_FILTER_INDEX_1);
	can_enable_interrupt(&can_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
}
static void can_set_extended_filter_0(void)
{
	struct can_extended_message_filter_element et_filter;
	can_get_extended_message_filter_element_default(&et_filter);
	et_filter.F0.bit.EFID1 = CAN_RX_EXTENDED_FILTER_ID_0;
	et_filter.F0.bit.EFEC =
	CAN_EXTENDED_MESSAGE_FILTER_ELEMENT_F0_EFEC_STRXBUF_Val;
	et_filter.F1.bit.EFID2 = CAN_RX_EXTENDED_FILTER_ID_0_BUFFER_INDEX;
	can_set_rx_extended_filter(&can_instance, &et_filter,
	CAN_RX_EXTENDED_FILTER_INDEX_0);
	can_enable_interrupt(&can_instance, CAN_RX_BUFFER_NEW_MESSAGE);
}
static void can_set_extended_filter_1(void)
{
	struct can_extended_message_filter_element et_filter;
	can_get_extended_message_filter_element_default(&et_filter);
	et_filter.F0.bit.EFID1 = CAN_RX_EXTENDED_FILTER_ID_1;
	can_set_rx_extended_filter(&can_instance, &et_filter,
	CAN_RX_EXTENDED_FILTER_INDEX_1);
	can_enable_interrupt(&can_instance, CAN_RX_FIFO_1_NEW_MESSAGE);
}
static void can_send_standard_message(uint32_t id_value, uint8_t *data)
{
	uint32_t i;
	struct can_tx_element tx_element;
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(id_value);
	tx_element.T1.bit.DLC = 8;
	for (i = 0; i < 8; i++) {
		tx_element.data[i] = *data;
		data++;
	}
	can_set_tx_buffer_element(&can_instance, &tx_element,
	CAN_TX_BUFFER_INDEX);
	can_tx_transfer_request(&can_instance, 1 << CAN_TX_BUFFER_INDEX);
}
static void can_fd_send_standard_message(uint32_t id_value, uint8_t *data)
{
	uint32_t i;
	struct can_tx_element tx_element;
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(id_value);
	tx_element.T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS |
	CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA64_Val);
	for (i = 0; i < CONF_CAN_ELEMENT_DATA_SIZE; i++) {
		tx_element.data[i] = *data;
		data++;
	}
	can_set_tx_buffer_element(&can_instance, &tx_element,
	CAN_TX_BUFFER_INDEX);
	can_tx_transfer_request(&can_instance, 1 << CAN_TX_BUFFER_INDEX);
}
static void can_fd_send_extended_message(uint32_t id_value, uint8_t *data)
{
	uint32_t i;
	struct can_tx_element tx_element;
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_EXTENDED_ID(id_value) |
	CAN_TX_ELEMENT_T0_XTD;
	tx_element.T1.reg = CAN_TX_ELEMENT_T1_EFC | CAN_TX_ELEMENT_T1_FDF |
	CAN_TX_ELEMENT_T1_BRS |
	CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA64_Val);
	for (i = 0; i < CONF_CAN_ELEMENT_DATA_SIZE; i++) {
		tx_element.data[i] = *data;
		data++;
	}
	can_set_tx_buffer_element(&can_instance, &tx_element,
	CAN_TX_BUFFER_INDEX);
	can_tx_transfer_request(&can_instance, 1 << CAN_TX_BUFFER_INDEX);
}
void CAN0_Handler(void)
{
	volatile uint32_t status, i, rx_buffer_index;
	status = can_read_interrupt_status(&can_instance);
	if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
		can_clear_interrupt_status(&can_instance, CAN_RX_BUFFER_NEW_MESSAGE);
		for (i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
			if (can_rx_get_buffer_status(&can_instance, i)) {
				rx_buffer_index = i;
				can_rx_clear_buffer_status(&can_instance, i);
				can_get_rx_buffer_element(&can_instance, &rx_element_buffer,
				rx_buffer_index);
				if (rx_element_buffer.R0.bit.XTD) {
					char* string = "\n\r Extended FD message received in Rx buffer. The received data is: \r\n";
					sendData_com(string, sizeof(string));
					} else {
					char* string = "\n\r Standard FD message received in Rx buffer. The received data is: \r\n";
					sendData_com(string, sizeof(string));
				}
				for (i = 0; i < CONF_CAN_ELEMENT_DATA_SIZE; i++) {
					char* string = rx_element_buffer.data[i];
					sendData_com(string,sizeof(string));
				}
				char* string = "\r\n\r\n";
				sendData_com(string, sizeof(string));
			}
		}
	}
	if (status & CAN_RX_FIFO_0_NEW_MESSAGE) {
		can_clear_interrupt_status(&can_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
		can_get_rx_fifo_0_element(&can_instance, &rx_element_fifo_0,
		standard_receive_index);
		can_rx_fifo_acknowledge(&can_instance, 0,
		standard_receive_index);
		standard_receive_index++;
		if (standard_receive_index == CONF_CAN0_RX_FIFO_0_NUM) {
			standard_receive_index = 0;
		}
		if (rx_element_fifo_0.R1.bit.FDF) {
			
			print("\n\r Standard FD message received in FIFO 0. The received data is: \r\n");
			for (i = 0; i < CONF_CAN_ELEMENT_DATA_SIZE; i++) {
				print(rx_element_fifo_0.data[i]);
			}
			} else {
			print("\n\r Standard normal message received in FIFO 0. The received data is: \r\n");
			for (i = 0; i < rx_element_fifo_0.R1.bit.DLC; i++) {
				print(rx_element_fifo_0.data[i]);
			}
		}
		print("\r\n\r\n");
	}
	if (status & CAN_RX_FIFO_1_NEW_MESSAGE) {
		can_clear_interrupt_status(&can_instance, CAN_RX_FIFO_1_NEW_MESSAGE);
		can_get_rx_fifo_1_element(&can_instance, &rx_element_fifo_1,
		extended_receive_index);
		can_rx_fifo_acknowledge(&can_instance, 0,
		extended_receive_index);
		extended_receive_index++;
		if (extended_receive_index == CONF_CAN0_RX_FIFO_1_NUM) {
			extended_receive_index = 0;
		}
		print("\n\r Extended FD message received in FIFO 1. The received data is: \r\n");
		for (i = 0; i < CONF_CAN_ELEMENT_DATA_SIZE; i++) {
			print(rx_element_fifo_1.data[i]);
		}
		//print("\r\n\r\n");
	}
	if ((status & CAN_PROTOCOL_ERROR_ARBITRATION)
	|| (status & CAN_PROTOCOL_ERROR_DATA)) {
		can_clear_interrupt_status(&can_instance, CAN_PROTOCOL_ERROR_ARBITRATION
		| CAN_PROTOCOL_ERROR_DATA);
		print("Protocol error, please double check the clock in two boards. \r\n\r\n");
	}
}
static void display_menu(void)
{
	print("Menu :\r\n"
	"  -- Select the action:\r\n"
	"  0: Set standard filter ID 0: 0x45A, store into Rx buffer. \r\n"
	"  1: Set standard filter ID 1: 0x469, store into Rx FIFO 0. \r\n"
	"  2: Send FD standard message with ID: 0x45A and 64 byte data 0 to 63. \r\n"
	"  3: Send FD standard message with ID: 0x469 and 64 byte data 128 to 191. \r\n"
	"  4: Set extended filter ID 0: 0x100000A5, store into Rx buffer. \r\n"
	"  5: Set extended filter ID 1: 0x10000096, store into Rx FIFO 1. \r\n"
	"  6: Send FD extended message with ID: 0x100000A5 and 64 byte data 0 to 63. \r\n"
	"  7: Send FD extended message with ID: 0x10000096 and 64 byte data 128 to 191. \r\n"
	"  a: Send normal standard message with ID: 0x469 and 8 byte data 0 to 7. \r\n"
	"  h: Display menu \r\n\r\n");
}

int main(void) {
	system_init();
	configure_usart_com();
	display_menu();
	uint16_t key;
	while(true){
		if (usart_read_wait(&com_instance, &key) == STATUS_OK) {
			switch (key) {
				case 'h':
				display_menu();
				break;
				case '0':
				print("  0: Set standard filter ID 0: 0x45A, store into Rx buffer. \r\n");
				can_set_standard_filter_0();
				break;
				case '1':
				print("  1: Set standard filter ID 1: 0x469, store into Rx FIFO 0. \r\n");
				can_set_standard_filter_1();
				break;
				case '2':
				print("  2: Send standard message with ID: 0x45A and 64 byte data 0 to 63. \r\n");
				can_fd_send_standard_message(CAN_RX_STANDARD_FILTER_ID_0, tx_message_0);
				break;
				case '3':
				print("  3: Send standard message with ID: 0x469 and 64 byte data 128 to 191. \r\n");
				can_fd_send_standard_message(CAN_RX_STANDARD_FILTER_ID_1, tx_message_1);
				break;
				case '4':
				print("  4: Set extended filter ID 0: 0x100000A5, store into Rx buffer. \r\n");
				can_set_extended_filter_0();
				break;
				case '5':
				print("  5: Set extended filter ID 1: 0x10000096, store into Rx FIFO 1. \r\n");
				can_set_extended_filter_1();
				break;
				case '6':
				print("  6: Send extended message with ID: 0x100000A5 and 64 byte data 0 to 63. \r\n");
				can_fd_send_extended_message(CAN_RX_EXTENDED_FILTER_ID_0, tx_message_0);
				break;
				case '7':
				print("  7: Send extended message with ID: 0x10000096 and 64 byte data 128 to 191. \r\n");
				can_fd_send_extended_message(CAN_RX_EXTENDED_FILTER_ID_1, tx_message_1);
				break;
				case 'a':
				print("  a: Send normal standard message with ID: 0x469 and 8 byte data 0 to 7. \r\n");
				can_send_standard_message(CAN_RX_STANDARD_FILTER_ID_1, tx_message_0);
				break;
				default:
				break;
			}
		}
	}
}
