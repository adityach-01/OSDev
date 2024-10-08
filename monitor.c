#include "common.h"
#include "io.h"
#include "monitor.h"

int cursor_x = 0;
int cursor_y = 0;

u16int *video_memory = (u16int *) 0xB8000;

static void move_cursor()
{
   // The screen is 80 characters wide...
   u16int cursorLocation = cursor_y * 80 + cursor_x;
   outb(0x3D4, 14);                  // Tell the VGA board we are setting the high cursor byte.
   outb(0x3D5, cursorLocation >> 8); // Send the high cursor byte.
   outb(0x3D4, 15);                  // Tell the VGA board we are setting the low cursor byte.
   outb(0x3D5, cursorLocation);      // Send the low cursor byte.
}

static void scroll()
{

   // Get a space character with the default colour attributes.
   u8int attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
   u16int blank = 0x20 /* space */ | (attributeByte << 8);

   // Row 25 is the end, this means we need to scroll up
   if(cursor_y >= 25)
   {
       // Move the current text chunk that makes up the screen
       // back in the buffer by a line
       int i;
       for (i = 0*80; i < 24*80; i++)
       {
           video_memory[i] = video_memory[i+80];
       }

       // The last line should now be blank. Do this by writing
       // 80 spaces to it.
       for (i = 24*80; i < 25*80; i++)
       {
           video_memory[i] = blank;
       }
       // The cursor should now be on the last line.
       cursor_y = 24;
   }
}

void monitor_put(char c)
{
   // The background colour is black (0), the foreground is white (15).
   u8int backColour = 0;
   u8int foreColour = 15;

   // The attribute byte is made up of two nibbles - the lower being the
   // foreground colour, and the upper the background colour.
   u8int  attributeByte = (backColour << 4) | (foreColour & 0x0F);
   // The attribute byte is the top 8 bits of the word we have to send to the
   // VGA board.
   u16int attribute = attributeByte << 8;
   u16int *location;

   // Handle a backspace, by moving the cursor back one space
   if (c == 0x08 && cursor_x)
   {
       cursor_x--;
   }

   // Handle a tab by increasing the cursor's X, but only to a point
   // where it is divisible by 8.
   else if (c == 0x09)
   {
       cursor_x = (cursor_x+8) & ~(8-1);
   }

   // Handle carriage return
   else if (c == '\r')
   {
       cursor_x = 0;
   }

   // Handle newline by moving cursor back to left and increasing the row
   else if (c == '\n')
   {
       cursor_x = 0;
       cursor_y++;
   }
   // Handle any other printable character.
   else if(c >= ' ')
   {
       location = video_memory + (cursor_y*80 + cursor_x);
       *location = c | attribute;
       cursor_x++;
   }

   // Check if we need to insert a new line because we have reached the end
   // of the screen.
   if (cursor_x >= 80)
   {
       cursor_x = 0;
       cursor_y ++;
   }

   // Scroll the screen if needed.
   scroll();
   // Move the hardware cursor.
   move_cursor();
}

void monitor_clear()
{
   // Make an attribute byte for the default colours
   u8int attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
   u16int blank = 0x20 /* space */ | (attributeByte << 8);

   int i;
   for (i = 0; i < 80*25; i++)
   {
       video_memory[i] = blank;
   }

   // Move the hardware cursor back to the start.
   cursor_x = 0;
   cursor_y = 0;
   move_cursor();
}

void monitor_write(char *c)
{
   int i = 0;
   while (c[i])
   {
       monitor_put(c[i++]);
   }
}

void monitor_write_dec(u32int n){
    // write the integer n on to the monitor
    char temp[30];
    int index = 0;

    if(n < 0){
        monitor_write("Negative number hai madarchod!!");
        return;
    }
    do{
        u8int dig = n % 10;
        n = n / 10;
        temp[index++] = dig + '0';
    }while(n);

    temp[index] = '\0';
    // reverse temp, length of temp is index
    for(int i = 0; i < index/2; i++){
        char cnt = temp[i];
        temp[i] = temp[index - i - 1];
        temp[index - i - 1] = cnt;
    }

    monitor_write(temp);
}

void monitor_write_hex(u32int n){
    // write the integer n on to the monitor
    char temp[30];
    int index = 0;

    char arr[] = {'A', 'B', 'C', 'D', 'E', 'F'};

    if(n < 0){
        monitor_write("Negative number hai madarchod!!");
        return;
    }
    do{
        u8int dig = n%16;
        n = n / 16;

        if(dig <= 9) temp[index++] = dig + '0';
        else{
            temp[index++] = arr[dig-10];
        }

    }while(n);

    temp[index++] = 'x';
    temp[index++] = '0';
    temp[index] = '\0';

    // reverse temp, length of temp is index
    for(int i = 0; i < index/2; i++){
        char cnt = temp[i];
        temp[i] = temp[index - i - 1];
        temp[index - i - 1] = cnt;
    }

    monitor_write(temp);
}