#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/logging/log.h>

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(main);

// Only one of these should be uncommented
// #define WIRING_NORMAL	// Wiring starts bottom-left, then goes up column, then right to next column and down
#define WIRING_REVERSE // Wiring starts top-right, then goes down column, then left to next column and up
// #define WIRING_BOTTOM_RIGHT // Wiring starts bottom-right, then up column, then left and down next column, etc.

#define LED_BRIGHTNESS 64
#define PROBLEM_STRING_MAX_LENGTH 256

#define UART_NODE DT_ALIAS(zboard_input)
#define STRIP_NODE DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define RGB(_r, _g, _b) {.r = (_r), .g = (_g), .b = (_b)}
#define COLOR(_r, _g, _b, _name) {.rgb = RGB((_r), (_g), (_b)), .name = (_name)}

typedef struct colorType
{
	struct led_rgb rgb;
	const char *name;
} color_t;

static const color_t color_list[] = {
	COLOR(LED_BRIGHTNESS, 0x00, 0x00, "red"),
	COLOR(0x00, LED_BRIGHTNESS, 0x00, "green"),
	COLOR(0x00, 0x00, LED_BRIGHTNESS, "blue"),
	COLOR(LED_BRIGHTNESS, LED_BRIGHTNESS, 0x00, "yellow"),
	COLOR(0x00, LED_BRIGHTNESS, LED_BRIGHTNESS, "cyan"),
	COLOR(LED_BRIGHTNESS, 0x00, LED_BRIGHTNESS / 2, "pink"),
	COLOR(LED_BRIGHTNESS / 2, 0x00, LED_BRIGHTNESS, "violet"),
	COLOR(LED_BRIGHTNESS, LED_BRIGHTNESS, LED_BRIGHTNESS, "white"),
	COLOR(0x00, 0x00, 0x00, "black"),
};

#define COLOR_RED color_list[0]
#define COLOR_GREEN color_list[1]
#define COLOR_BLUE color_list[2]
#define COLOR_YELLOW color_list[3]
#define COLOR_CYAN color_list[4]
#define COLOR_PINK color_list[5]
#define COLOR_VIOLET color_list[6]
#define COLOR_WHITE color_list[7]
#define COLOR_BLACK color_list[8]

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
// x<holdnum>,<holdnum>,<holdnum>#  (doesn't apply LED mapping)
// t# or x# 	- clear board

typedef enum parseState
{
	PARSE_START,
	PARSE_CONFIG,
	PARSE_PROB_START,
	PARSE_HOLDS
} parse_state_t;

#define NUM_ROWS 18
#define NUM_COLS 11
#define NUM_PIXELS NUM_ROWS *NUM_COLS
static u_int16_t led_map[NUM_PIXELS];

static struct led_rgb pixels[STRIP_NUM_PIXELS];

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);
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
bool bBetaLEDs = false;								   // Beta LED setting
bool bTestMode = false;
bool bApplyLEDMapping = true;

void handleChar(char);

void clearStrip()
{
	for (int i = 0; i < STRIP_NUM_PIXELS; i++)
	{
		pixels[i] = COLOR_BLACK.rgb;
	}
	led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
}

void handleChar(char c)
{
	LOG_DBG("%s(%c) - state %d", __func__, c, parse_state);
	switch (parse_state)
	{
	case PARSE_START:
		bBetaLEDs = false;
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
		}
		break;
	case PARSE_CONFIG:
		switch (c)
		{
		case 'D':
			bBetaLEDs = true;
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

static void bt_connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		LOG_ERR("Conn failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
	}
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

static void bt_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));
}

static void bt_recycled(void)
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
	.connected = bt_connected,
	.disconnected = bt_disconnected,
	.recycled = bt_recycled};

void renderProblem()
{
	clearStrip();
	LOG_INF("Problem string: %s", strProblem);

	strncpy(strProblemBackup, strProblem, sizeof(strProblemBackup) - 1); // store copy of problem string
	strProblemBackup[sizeof(strProblemBackup) - 1] = '\0';

	// Handle beta LEDs here (TODO)

	char *token = strtok(strProblem, ",");
	int ledCount = 0;
	while (token)
	{ // render all normal LEDs (possibly overriding beta LEDs)
		ledCount++;
		char holdType = bTestMode ? 'P' : token[0];			 // Hold descriptions consist of a hold type (S, P, E) (omitted in test mode)...
		int holdNum = atoi(bTestMode ? token : (token + 1)); // ... and a hold number
		int ledNum = bApplyLEDMapping ? led_map[holdNum] : holdNum;
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
		LOG_INF("%c%d --> %d (%s)", holdType, holdNum, ledNum, led_color->name);
		token = strtok(NULL, ",");
	}
	int err = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
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

// Initialize the LED mapping
#ifdef WIRING_NORMAL
	for (uint16_t i = 0; i < NUM_PIXELS; i++)
	{
		led_map[i] = i;
	}
#endif
#ifdef WIRING_REVERSE
	for (uint16_t i = 0; i < NUM_PIXELS; i++)
	{
		led_map[i] = NUM_PIXELS - 1 - i;
	}
#endif
#ifdef WIRING_BOTTOM_RIGHT // Wiring starts bottom-right, then up column, then left and down next column, etc.
	for (int col = 0; col < NUM_COLS; col++)
	{
		for (int row = 0; row < NUM_ROWS; row++)
		{
			uint16_t moonboard_lednum = NUM_ROWS * col + (col % 2 == 0 ? row : (NUM_ROWS - 1 - row));
			uint16_t zboard_lednum = NUM_ROWS * (NUM_COLS - 1 - col) + (col % 2 == 0 ? row : (NUM_ROWS - 1 - row));
			led_map[moonboard_lednum] = zboard_lednum;
		}
	}
#endif

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

	/* configure interrupt and callback to receive data */
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

	char c;
	while (1)
	{
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
