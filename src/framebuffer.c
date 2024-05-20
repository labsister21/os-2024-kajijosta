#include "header/text/framebuffer.h"
#include "header/stdlib/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "header/cpu/portio.h"

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg,
                       uint8_t bg)
{
  FRAMEBUFFER_MEMORY_OFFSET[(row * 80 + col) * 2] = c;
  FRAMEBUFFER_MEMORY_OFFSET[(row * 80 + col) * 2 + 1] = (bg << 4) | (fg & 0x0F);
}

void framebuffer_set_cursor(uint8_t r, uint8_t c)
{
  uint16_t pos = r * 80 + c;
  out(CURSOR_PORT_CMD, 0x0F);
  out(CURSOR_PORT_DATA, (uint8_t)(pos & 0xFF));
  out(CURSOR_PORT_CMD, 0x0E);
  out(CURSOR_PORT_DATA, (uint8_t)((pos >> 8) & 0xFF));
}

void framebuffer_clear(void)
{
  unsigned black;
  int i;

  black = 0x00 | (0x00 << 8);

  for (i = 0; i < 25 * 2; i++)
  {
    memset((uint8_t *)(FRAMEBUFFER_MEMORY_OFFSET + i * 80), black, 80 * 2);
  }
  for (size_t i = 0; i < 25 * 80; i++)
  {
    framebuffer_write(i / 80, i % 80, ' ', 0x07, 0x00);
  }

  framebuffer_set_cursor(0, 0);
}

void framebuffer_set_background_wallpaper(void)
{
  const char *mountain[] = {
      "         /\\                         ",
      "        /  \\                        ",
      "       /    \\                       ",
      "      /      \\                      ",
      "     /        \\     k a j i j OS t a",
      "    /          \\                    ",
      "   /____________\\                   ",
      "____________________________________"};

  uint8_t row, col;
  for (row = 0; row < 8; row++)
  {
    for (col = 0; col < 36; col++)
    {
      framebuffer_write(row, col, mountain[row][col], 0x07, 0x00);
    }
  }
}