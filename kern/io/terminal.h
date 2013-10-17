void terminal_initialize(bool clearMem);

void terminal_setColour(uint8_t colour);
void terminal_write_string(char *string);
void terminal_putchar(char c);
void terminal_setPos(uint8_t col, uint8_t row);
void terminal_clear();
