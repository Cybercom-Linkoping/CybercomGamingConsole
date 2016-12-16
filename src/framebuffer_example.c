#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <math.h>

static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static char *fbp = 0;

static long int get_location(int x, int y)
{
  return (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
         (y+vinfo.yoffset) * finfo.line_length;
}

static void put_pixel(long int location, int r, int g, int b)
{
  unsigned short int t = (r << 11) | (g << 6) | b;
  *((unsigned short int*)(fbp + location)) = t;
}

static void draw_line(int x0, int y0, int x1, int y1, int r, int g, int b)
{
  long int loc;
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
  int err = (dx>dy ? dx : -dy)/2, e2;
 
  for(;;) {
    loc = get_location(x0, y0);
    put_pixel(loc, r, g, b);
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
  }
}


static void make_triangle(int x, int y, int s, int r, int g, int b)
{
  int dy, x1, y1, x2, y2, xp, yp;
  dy = (int)((double)s*sqrt(3.0)/4.0);
  x1 = x - (s/2);
  y1 = y + dy;
  x2 = x + (s/2);
  y2 = y - dy;
  draw_line(x1, y1, x2, y1, r, g, b);
  draw_line(x2, y1, x, y2, r, g, b);
  draw_line(x, y2, x1, y1, r, g, b);
}

static void clear_screen(void)
{
  int x, y;
  long int loc;
  for (x = 0; x < 320; ++x)
    for (y = 0; y < 480; ++y)
    {
      loc = get_location(x, y);
      put_pixel(loc, 0, 0, 0);
    }
}

 int main()
 {
     int fbfd = 0;
     long int screensize = 0;
     int x = 0, y = 0;
     long int location = 0;

     // Open the file for reading and writing
     fbfd = open("/dev/fb1", O_RDWR);
     if (fbfd == -1) {
         perror("Error: cannot open framebuffer device");
         exit(1);
     }
     printf("The framebuffer device was opened successfully.\n");

     // Get fixed screen information
     if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
         perror("Error reading fixed information");
         exit(2);
     }

     // Get variable screen information
     if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
         perror("Error reading variable information");
         exit(3);
     }

     printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

     // Figure out the size of the screen in bytes
     screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

     // Map the device to memory
     fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fbfd, 0);
     if ((int)fbp == -1) {
         perror("Error: failed to map framebuffer device to memory");
         exit(4);
     }
     printf("The framebuffer device was mapped to memory successfully.\n");

     int max_colour = (1 << 5) - 1;
     int half_colour = (1 << 4) - 1;
     make_triangle(160, 240, 200, max_colour, max_colour,  max_colour);
     sleep(3);
     make_triangle(160, 240, 200, 0, 0, 0);
     //clear_screen();
     make_triangle(110, 290, 100, max_colour, 0, 0);
     make_triangle(210, 290, 100, 0, max_colour, 0);
     make_triangle(160, 190, 100, 0, 0, max_colour);
     sleep(3);
     make_triangle(110, 290, 100, 0, 0, 0);
     make_triangle(210, 290, 100, 0, 0, 0);
     make_triangle(160, 190, 100, 0, 0, 0);
     
     make_triangle( 85, 315, 50, max_colour, half_colour, 0);
     make_triangle(135, 315, 50, 0, max_colour, half_colour);
     make_triangle(110, 265, 50, half_colour, 0, max_colour);
     make_triangle(185, 315, 50, max_colour, 0, half_colour);
     make_triangle(235, 315, 50, half_colour, max_colour, 0);
     make_triangle(210, 265, 50, 0, half_colour, max_colour);
     make_triangle(135, 215, 50, max_colour, half_colour, half_colour);
     make_triangle(185, 215, 50, half_colour, max_colour, half_colour);
     make_triangle(160, 165, 50, half_colour, half_colour, max_colour);
     sleep(5);
     clear_screen();
     munmap(fbp, screensize);
     close(fbfd);
     return 0;
 }
