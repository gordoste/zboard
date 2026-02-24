#include "led_map.h"

#ifdef WIRING_LEDS_SKIP_FIRST_COLUMN
uint8_t ledSkipList[] = {WIRING_LEDS_SKIP_FIRST_COLUMN};
#else
uint8_t ledSkipList[] = {};
#endif

uint8_t ledSkipNum = sizeof(ledSkipList) / sizeof(ledSkipList[0]);

#ifdef WIRING_FIRST_LED_RIGHT
bool bWiringFirstLEDRight = true;
#else
bool bWiringFirstLEDRight = false;
#endif

#ifdef WIRING_FIRST_LED_BOTTOM
bool bWiringFirstLEDBottom = true;
#else
bool bWiringFirstLEDBottom = false;
#endif

bool ledSkip(uint16_t ledNum)
{
	if (sizeof(ledSkipList) == 0)
	{
		return false;
	}
	for (size_t i = 0; i < ledSkipNum; i++)
	{
		if (ledNum == ledSkipList[i])
		{
			return true;
		}
	}
	return false;
}

uint16_t moonNumToMapNum(uint16_t moonNum)
{
	int iFullCols = moonNum / NUM_ROWS;
	int iLEDsRemaining = moonNum % NUM_ROWS;
	// We get passed the number of the LED in the standard wiring, which has LED #0 at the bottom-left
	// Our LED map has the columns in sequence, with every column numbered from bottom to top
	// If the LED is in an even column (0-indexed), the numbers will be the same.
	// If the LED is in an odd column (0-indexed), the numbers will be reversed since the wiring goes down the next column
	if (iFullCols % 2 == 0)
	{
		return moonNum;
	}
	else
	{
		return (iFullCols * NUM_ROWS) + (NUM_ROWS - 1 - iLEDsRemaining);
	}
}

#ifndef WIRING_CUSTOM
void populate_led_map(uint16_t led_map[])
{
	bool bWiringGoesDownThisCol = !bWiringFirstLEDBottom;
	uint8_t ledsInCol = NUM_ROWS + ledSkipNum; // number of LEDs in each column including skipped LEDs

	uint16_t ledNum = 0;
	int col = bWiringFirstLEDRight ? (NUM_COLS - 1) : 0;   // current column we are filling in the LED map
	int row = bWiringGoesDownThisCol ? (NUM_ROWS - 1) : 0; // current row we are filling in the LED map
	while (col < NUM_COLS && col >= 0)
	{
		while (row < NUM_ROWS && row >= 0)
		{
			// Figure out where to save this LED number in the map
			led_map[col * NUM_ROWS + row] = ledNum;
			row += bWiringGoesDownThisCol ? -1 : 1;
			// Move to the next LED in the chain, skipping any LEDs that need to be skipped
			ledNum++;
			uint8_t ledNumInChunk = ledNum % (2 * ledsInCol);
			// The first clause is for columns 1,3,5, etc. , second clause is for columns 2,4,6, etc. since the wiring goes down one column and up the next
			while ((ledNumInChunk < ledsInCol && ledSkip(ledNumInChunk)) || (ledNumInChunk > ledsInCol && ledSkip(2 * ledsInCol - 1 - ledNumInChunk)))
			{
				ledNum++;
				ledNumInChunk = ledNum % (2 * ledsInCol);
			}
		}
		col += bWiringFirstLEDRight ? -1 : 1;
		if (row == NUM_ROWS)
		{
			row = NUM_ROWS - 1;
		}
		else if (row == -1)
		{
			row = 0;
		}
		bWiringGoesDownThisCol = !bWiringGoesDownThisCol;
	}
}
#else
void populate_led_map_custom(uint16_t led_map[], uint8_t numRows, uint8_t numCols)
{
	// If you want to define your own wiring configuration, you can do that here
	// The led_map[x*numRows + y] array needs to contain the LED number that should be used for the
	// hold at row x, column y. Rows are number L-to-R and columns are numbered from the bottom up.
}
#endif