#include <stdlib.h>

#include "zboard.h"

#include "led_map.h"
#include "led_patterns.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/drivers/uart.h>

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(main);

#define UART_NODE DT_ALIAS(zboard_input)
#define STRIP_NODE DT_ALIAS(led_strip)

// Problem string structure:
// V1:
//	<holdtype>=[SPE]
//  <holdnum>=[0-9]{1,3}
//  <holdspec>=<holdtype><holdnum>
//  <problem>=~l#<holdspec>(,<holdspec>)*#
// V2:
//	<holdtype>=[SPELRMF]
//  <holdnum>=[0-9]{1,3}
//  <holdspec>=<holdtype><holdnum>
//  <problem>=~[Dl]#<holdspec>(,<holdspec>)*#
// Testing modes:
// t<holdnum>,<holdnum>,<holdnum>#  (applies LED mapping)
// a<holdnum>,<holdnum>,<holdnum>#  (applies LED mapping and lights additional LEDs)
// x<holdnum>,<holdnum>,<holdnum>#  (doesn't apply LED mapping)
// t# or x# 	- clear board

struct led_rgb pixels[STRIP_LENGTH];

const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
static const struct device *const uart_in = DEVICE_DT_GET(UART_NODE);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_SRV_VAL),
};

parse_state_t parse_state = PARSE_START;		  // Current state of the problem string parser
char parseBuffer[PROBLEM_STRING_MAX_LENGTH] = ""; // Problem currently being received
int parseBufferPos = 0;
char strProblem[PROBLEM_STRING_MAX_LENGTH] = "";	   // Complete problem string
char strProblemBackup[PROBLEM_STRING_MAX_LENGTH] = ""; // Copy of complete problem string
bool bProbPending = false;							   // Do we have a problem ready to display?
bool bAdditionalLEDs = false;						   // Additional LED setting
bool bTestMode = false;
bool bApplyLEDMapping = true;

void handleChar(char);

void clearStrip()
{
	for (int i = 0; i < STRIP_LENGTH; i++)
	{
		pixels[i] = COLOR_BLACK.rgb;
	}
	led_strip_update_rgb(strip, pixels, STRIP_LENGTH);
}

void handleChar(char c)
{
	LOG_DBG("%s(%c) - state %d", __func__, c, parse_state);
	switch (parse_state)
	{
	case PARSE_START:
		bAdditionalLEDs = false;
		parseBuffer[0] = '\0';
		parseBufferPos = 0;
		switch (c)
		{
		case '~':
			bApplyLEDMapping = true;
			bTestMode = false;
			parse_state = PARSE_CONFIG;
			return;
		case 'L':
		case 'l':
			bApplyLEDMapping = true;
			bTestMode = false;
			parse_state = PARSE_PROB_START;
			return;
		case 't':
		case 'T':
			bApplyLEDMapping = true;
			bTestMode = true;
			parse_state = PARSE_HOLDS;
			return;
		case 'x':
		case 'X':
			bTestMode = true;
			bApplyLEDMapping = false;
			parse_state = PARSE_HOLDS;
			return;
		case 'a':
		case 'A':
			bTestMode = true;
			bApplyLEDMapping = true;
			bAdditionalLEDs = true;
			parse_state = PARSE_HOLDS;
			return;
		}
		break;
	case PARSE_CONFIG:
		switch (c)
		{
		case 'D':
			bAdditionalLEDs = true;
			parse_state = PARSE_PROB_START;
			return;

		case 'l':
			parse_state = PARSE_PROB_START;
			return;
		}
		break;

	case PARSE_PROB_START:
		if (c == '#')
		{
			parse_state = PARSE_HOLDS;
			return;
		}
		break;

	case PARSE_HOLDS:
		if (c == '#')
		{
			LOG_DBG("Received complete problem");
			parseBuffer[parseBufferPos] = '\0';
			strncpy(strProblem, parseBuffer, sizeof(strProblem));
			bProbPending = true;
			parse_state = PARSE_START;
			return;
		}
		parseBuffer[parseBufferPos++] = c;
		if (parseBufferPos >= sizeof(parseBuffer) - 1)
		{
			LOG_ERR("Problem string overflow");
			parse_state = PARSE_START;
		}
		return;
		break;
	}
}

static void bt_handle_connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		LOG_ERR("Conn failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
	}
	// Restart advertising so that multiple clients can connect
	int rc = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (!err)
	{
		if (rc == -ECONNREFUSED)
		{
			LOG_INF("Conn received, max conns established. Advertising paused.");
		}
		else if (rc)
		{
			LOG_ERR("Conn received. Failed to restart advertising: %d", rc);
		}
		else
		{
			LOG_INF("Connection received. Restarted advertising");
		}
	}
}

static void bt_handle_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
}

// A client has dropped and we have a free connection object again, so we can start advertising if we weren't already
static void bt_handle_recycled(void)
{
	int rc = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (rc)
	{
		LOG_ERR("Conn recycled. Failed to restart advertising: %d", rc);
	}
	else
	{
		LOG_INF("Conn recycled. Restarted advertising");
	}
}

static struct bt_conn_cb bt_conn_cb_zboard = {
	.connected = bt_handle_connected,
	.disconnected = bt_handle_disconnected,
	.recycled = bt_handle_recycled};

void renderProblem()
{
	clearStrip();
	LOG_INF("Problem string: %s", strProblem);

	strncpy(strProblemBackup, strProblem, sizeof(strProblemBackup) - 1); // store copy of problem string
	strProblemBackup[sizeof(strProblemBackup) - 1] = '\0';

	char *token = strtok(strProblem, ",");
	int ledCount = 0;
	uint16_t ledNum;
	while (token)
	{
		ledCount++;
		char holdType = bTestMode ? 'P' : token[0];			 // Hold descriptions consist of a hold type (S, P, E) (omitted in test mode)...
		int moonNum = atoi(bTestMode ? token : (token + 1)); // ... and a hold number
		uint16_t mapNum = moonNumToMapNum(moonNum);
		ledNum = bApplyLEDMapping ? led_map[mapNum] : moonNum;
		const color_t *led_color = &COLOR_BLACK;
		switch (holdType)
		{
		case 'S':
		case 's':
			led_color = &COLOR_GREEN;
			break;
		case 'R':
		case 'r':
		case 'P':
		case 'p':
			led_color = &COLOR_BLUE;
			break;
		case 'E':
		case 'e':
			led_color = &COLOR_RED;
			break;
		case 'L':
		case 'l':
			led_color = &COLOR_VIOLET;
			break;
		case 'M':
		case 'm':
			led_color = &COLOR_PINK;
			break;
		case 'F':
		case 'f':
			led_color = &COLOR_CYAN;
			break;
		}
		pixels[ledNum] = led_color->rgb;
		LOG_INF("%c%d --> %d (%s)", holdType, moonNum, ledNum, led_color->name);

		if (bAdditionalLEDs)
		{
			if (mapNum % NUM_ROWS != NUM_ROWS - 1) // Not in the top row
			{
				// If we're not using the LED mapping, just get the next LED
				uint16_t ledAboveNum = bApplyLEDMapping ? led_map[mapNum + 1] : ledNum + 1;
				pixels[ledAboveNum] = COLOR_YELLOW.rgb;
				LOG_INF("add. %d", ledAboveNum);
			}
			else
			{
				LOG_DBG("LED %d is in the top row, skipping additional LED", ledNum);
			}
		}
		token = strtok(NULL, ",");
	}
	int err = led_strip_update_rgb(strip, pixels, STRIP_LENGTH);
	if (err)
	{
		LOG_ERR("Failed to update LED strip: %d", err);
	}
	else
	{
		LOG_INF("Rendered problem with %d LEDs", ledCount);
	}
}

void input_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_in))
	{
		return;
	}

	if (!uart_irq_rx_ready(uart_in))
	{
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(uart_in, &c, 1) == 1)
	{
		handleChar(c);
	}
}

int main(void)
{
	initialize_led_map();

	if (device_is_ready(strip))
	{
		LOG_INF("Found LED strip device %s", strip->name);
	}
	else
	{
		LOG_ERR("LED strip device %s is not ready", strip->name);
		return 0;
	}

	int err;

	if (!device_is_ready(uart_in))
	{
		LOG_ERR("UART device not found!");
		return 0;
	}

	// configure interrupt and callback to receive data
	err = uart_irq_callback_set(uart_in, input_cb);

	if (err < 0)
	{
		if (err == -ENOTSUP)
		{
			LOG_ERR("Interrupt-driven UART API support not enabled\n");
		}
		else if (err == -ENOSYS)
		{
			LOG_ERR("UART device does not support interrupt-driven API\n");
		}
		else
		{
			LOG_ERR("Error setting UART callback: %d\n", err);
		}
		return 0;
	}
	uart_irq_rx_enable(uart_in);

	// Enable Bluetooth and start advertising
	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Failed to enable bluetooth: %d", err);
		return err;
	}

	err = bt_conn_cb_register(&bt_conn_cb_zboard);
	if (err)
	{
		LOG_ERR("Failed to register BT conn callback: %d", err);
		return err;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err)
	{
		LOG_ERR("Failed to start advertising: %d", err);
		return err;
	}

	LOG_INF("Bluetooth setup complete, advertising as '%s'", DEVICE_NAME);

	led_startup_pattern();
	clearStrip();

	char c;
	while (1)
	{
		// Since receiving characters is handled by interrupts ( see input_cb() ), our main loop
		// just checks if we have a problem ready to display and renders it.
		// We do want to parse all characters in the UART receive queue first though.
		if (bProbPending)
		{
			err = uart_poll_in(uart_in, &c);
			if (err == -1)
			{
				renderProblem();
				bProbPending = false;
				strProblem[0] = '\0';
			}
			else if (err == 0)
			{
				handleChar(c);
			}
			else
			{
				LOG_ERR("Error polling UART: %d", err);
			}
		}

		k_sleep(K_MSEC(100));
	}

	return 0;
}
