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
	for (size_t i = 0; i < ledSkipNum; i++)
	{
		if (ledNum == ledSkipList[i])
		{
			return true;
		}
	}
	return false;
}

uint16_t moonNumToMapNum(uint16_t moonNum, uint8_t numRows, uint8_t numCols)
{
	// We get passed the number of the LED in the standard wiring, which has LED #0 at the top-left
	// First we calculate how many complete pairs of 2-columns are prior to the given LED...
	int numColPairs = moonNum / (2 * numRows);
	// ... and how many LEDs remain
	int numLEDsRemaining = moonNum % (2 * numRows);
	// Our LED map has the columns in sequence, from bottom to top
	// If we have more than a full column left, then the column that the LED is in will be wired upwards so we can just add that number
	// If we have less than a full column, then we need to calculate how far the LED is from the bottom and add that
	return (numColPairs * 2 * numRows) + (numLEDsRemaining >= numRows ? numLEDsRemaining : numRows - 1 - numLEDsRemaining);
}

#ifndef WIRING_CUSTOM
void populate_led_map(uint16_t led_map[], uint8_t numRows, uint8_t numCols)
{
	bool bWiringGoesDownThisCol = !bWiringFirstLEDBottom;
	uint8_t ledsInCol = numRows + ledSkipNum; // number of LEDs in each column including skipped LEDs

	uint16_t ledNum = 0;
	int col = bWiringFirstLEDRight ? (numCols - 1) : 0;	  // current column we are filling in the LED map
	int row = bWiringGoesDownThisCol ? (numRows - 1) : 0; // current row we are filling in the LED map
	while (col < numCols && col >= 0)
	{
		while (row < numRows && row >= 0)
		{
			// Figure out where to save this LED number in the map
			led_map[col * numRows + row] = ledNum;
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
		if (row == numRows)
		{
			row = numRows - 1;
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