/* See LICENSE file for license details */

/* Standard library includes */
#include <cstdio>
#include <cstring>

/* Pico libs*/
#include "pico/time.h"
#include "pico/sem.h"
#include "pico/multicore.h"
#include "hardware/vreg.h"

/* Murmulator Specific Libs*/
#include "vga.h"
//#include "ps2kbd_mrmltr.h"
//#include "ff.h"

#include "agnes.h"

#define LED_PIN 25

#include "rom2.h"

static agnes_t *agnes;
#define X2(a) (a | (a << 8))
#define X4(a) (a | (a << 8) | (a << 16) | (a << 24))
#define VGA_RGB_222(r, g, b) ((r << 4) | (g << 2) | b)
uint8_t *screen;

/*static FATFS fs;
extern "C" void __not_in_flash_func(process_kbd_report)(hid_keyboard_report_t const *report, hid_keyboard_report_t const *prev_report)
{
    printf("HID key report modifiers %2.2X report ", report->modifier);
    for (int i = 0; i < 6; ++i)
        printf("%2.2X", report->keycode[i]);
    printf("\n");
}
static Ps2Kbd_Mrmltr ps2kbd(pio1, PS2KBD_GPIO_FIRST, process_kbd_report);*/

static const sVmode *vmode = NULL;
struct semaphore vga_start_semaphore;

/* Renderer loop on Pico's second core */
void __time_critical_func(render_loop)()
{
    // Allow core to be locked from other core
    multicore_lockout_victim_init();
    VgaLineBuf *linebuf;
    printf("Render on Core#%i running...\n", get_core_num());

    sem_acquire_blocking(&vga_start_semaphore);
    VgaInit(vmode, 640, 480);

    while (linebuf = get_vga_line())
    {
		memcpy(&linebuf->line, &screen[linebuf->row*AGNES_SCREEN_WIDTH], AGNES_SCREEN_WIDTH);
    }

    __builtin_unreachable();
}

void get_input(const uint8_t *state, agnes_input_t *out_input) {
    memset(out_input, 0, sizeof(agnes_input_t));
/*
    if (state[SDL_SCANCODE_Z])      out_input->a = true;
    if (state[SDL_SCANCODE_X])      out_input->b = true;
    if (state[SDL_SCANCODE_LEFT])   out_input->left = true;
    if (state[SDL_SCANCODE_RIGHT])  out_input->right = true;
    if (state[SDL_SCANCODE_UP])     out_input->up = true;
    if (state[SDL_SCANCODE_DOWN])   out_input->down = true;
    if (state[SDL_SCANCODE_RSHIFT]) out_input->select = true;
    if (state[SDL_SCANCODE_RETURN]) out_input->start = true;
*/
}


/* Main program loop on Pico's first core */
void __time_critical_func(main_loop)()
{
    printf("Main loop on Core#%i running...\n", get_core_num());
    agnes = agnes_make();

    bool ok = agnes_load_ines_data(agnes, (void *)&rom, sizeof(rom));
    if (!ok) {
        printf("Loading %s failed.\n", "ROM");
        return;
    }
    uint_fast32_t frames = 0;
    uint64_t start_time = time_us_64();
    screen = agnes_get_screen_buffer(agnes);


    while (ok = agnes_next_frame(agnes))
    {
        frames++;
        gpio_put(LED_PIN, 1);
//        ps2kbd.tick();
        if (!ok) {
            printf("Getting next frame failed.\n");
            return;
        }

        uint64_t end_time;
        uint32_t diff;
        uint32_t fps;

        end_time = time_us_64();
        diff = end_time - start_time;
        fps = ((uint64_t) frames * 1000 * 1000) / diff;
        printf("Frames: %u\n"
               "Time: %lu us\n"
               "FPS: %lu\n",
               frames, diff, fps);
        stdio_flush();
        frames = 0;
        start_time = time_us_64();

        gpio_put(LED_PIN, 0);
    }
    __builtin_unreachable();
}

/******************************************************************************
 * Main code entry point
 *****************************************************************************/
/*

bool initSDCard()
{
    FRESULT fr;
    DIR dir;
    FILINFO file;

    sleep_ms(1000);

    printf("Mounting SDcard");
    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK)
    {
        printf("SD card mount error: %d", fr);

        return false;
    }
    printf("\n");

    fr = f_chdir("/");
    if (fr != FR_OK)
    {
        printf("Cannot change dir to / : %d\r\n", fr);
        return false;
    }

    printf("Listing %s\n", ".");

    f_opendir(&dir, ".");
    while (f_readdir(&dir, &file) == FR_OK && file.fname[0])
    {
        printf("%s", file.fname);
    }

    f_closedir(&dir);

    return true;
}
*/

int main()
{
    /* Overclock. */
    {
        const unsigned vco = 1596 * 1000 * 1000; /* 266MHz */
        const unsigned div1 = 6, div2 = 1;

        vreg_set_voltage(VREG_VOLTAGE_1_10);
        sleep_ms(2);
 //       set_sys_clock_pll(vco, div1, div2);
        set_sys_clock_khz(288 * 1000, true);
        sleep_ms(2);
    }
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    vmode = Video(DEV_VGA, RES_HVGA);

    sleep_ms(50);
    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_loop);
    sem_release(&vga_start_semaphore);
    //ps2kbd.init_gpio();

    main_loop();
}