#include <mega16.h>
#include <delay.h>

// 数码管相关定义
flash char led_7[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
flash char position[4] = {0b00000000, 0b01000000, 0b00100000, 0b00010000};
char time[2] = {0, 0};              // time[0]为秒, time[1]为分
char dis_buff[4];                   // 显示缓冲区
volatile char time_update_flag = 0; // 时间更新标志

// 流水灯相关定义
char led_index = 0;                    // 当前要亮的灯索引（0-7）
volatile unsigned int sec_counter = 0; // 秒计数器（每1秒加1）
volatile char led_update_flag = 0;     // 流水灯更新标志

// Timer1 比较匹配A中断服务程序(1ms触发一次)
interrupt[TIM1_COMPA] void timer1_compa_isr(void)
{
    static unsigned int ms_counter = 0;

    // 1. 时间计数(1秒更新一次)
    ms_counter++;
    if (ms_counter >= 1000)
    {
        ms_counter = 0;
        if (++time[0] >= 60)
        {
            time[0] = 0;
            if (++time[1] >= 60)
                time[1] = 0;
        }
        time_update_flag = 1;
        sec_counter++;
        led_update_flag = 1;
    }
}

// 数码管显示函数
void display(void)
{
    char i;
    for (i = 0; i < 4; i++)
    {
        PORTC = led_7[dis_buff[i]];
        if (i == 0)
        {
            PORTA.6 = 1;
            PORTD = 0;
        }
        else
        {
            PORTD = position[i];
            PORTA.6 = 0;
        }
        delay_ms(2);
        PORTC = 0xFF;
        PORTD = 0;
    }
}

// 时间转显示缓冲区
void time_to_disbuffer(void)
{
    char i, j = 0;
    for (i = 0; i < 2; i++)
    {
        dis_buff[j++] = time[i] % 10; // 个位
        dis_buff[j++] = time[i] / 10; // 十位
    }
}

// 流水灯更新函数
void update_led(void)
{
    // 根据秒计数器判断状态：
    // sec_counter为偶数（0,2,4...）→ 亮当前灯（第1、3、5...秒）
    // sec_counter为奇数（1,3,5...）→ 熄灭所有灯（第2、4、6...秒）
    if (sec_counter % 2 == 0)
    {
        PORTB = ~(1 << led_index); // 点亮当前索引的灯（低电平点亮）
    }
    else
    {
        PORTB = 0xFF; // 熄灭所有灯（高电平）
        led_index++;
        if (led_index > 7)
            led_index = 0; // 循环0-7
    }
}

// 初始化Timer1
void timer1_init(void)
{
    TCCR1B = (1 << WGM12) | (1 << CS11);
    OCR1A = 499;
    TIMSK = (1 << OCIE1A);
}

// 端口初始化
void port_init(void)
{
    // 流水灯端口(PORTB)
    DDRB = 0xFF;
    PORTB = 0xFF;

    // 数码管端口
    DDRC = 0xFF;
    PORTC = 0xFF;
    DDRD = 0b01110000;
    PORTD = 0;
    DDRA.6 = 1;
    PORTA.6 = 0;
}

void main(void)
{
    port_init();
    timer1_init();
    time_to_disbuffer();
#asm("sei");

    while (1)
    {
        display();

        // 检测时间更新标志
        if (time_update_flag)
        {
            time_update_flag = 0;
            time_to_disbuffer();
        }

        // 检测流水灯更新标志（每1秒触发一次）
        if (led_update_flag)
        {
            led_update_flag = 0;
            update_led();
        }
    }
}