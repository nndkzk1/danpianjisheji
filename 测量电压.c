#include <mega16.h>
#include <delay.h>

flash char led_7[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
flash char position[4] = {0b00000000, 0b01000000, 0b00100000, 0b00010000};

unsigned char dis_buff[4] = {0, 0, 0, 0};
bit time_2ms_ok;

// ADC电压值（mV）转显示缓冲区
void adc_to_disbuffer(unsigned int adc_mv)
{
    dis_buff[0] = adc_mv % 10;          // 个位（mV）→ 数码管最右
    dis_buff[1] = (adc_mv / 10) % 10;   // 十位（mV）→ 数码管右2
    dis_buff[2] = (adc_mv / 100) % 10;  // 百位（mV）→ 数码管右3
    dis_buff[3] = (adc_mv / 1000) % 10; // 千位（mV）→ 数码管最左（电压整数位）
}

// 数码管动态扫描显示
void display(void)
{
    char i;
    for (i = 0; i < 4; i++)
    {
        // 1. 消影：先关闭所有数码管和段码
        PORTC = 0xFF;
        PORTD = 0;
        PORTA.6 = 1;

        // 2. 输出段码：最左位（i=3，电压整数位）后点亮小数点
        if (i == 3)
        {
            PORTC = led_7[dis_buff[i]] | 0x80;
        }
        else
        {
            PORTC = led_7[dis_buff[i]];
        }

        // 3. 选通当前数码管
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

        // 4. 保持显示，保证人眼视觉暂留
        delay_ms(2);
    }
}

// Timer 0 比较匹配中断服务（2ms触发一次）
interrupt[TIM0_COMP] void timer0_comp_isr(void)
{
    time_2ms_ok = 1;
}

// ADC 转换完成中断服务（自动触发，完成后读取并换算电压）
interrupt[ADC_INT] void adc_isr(void)
{
    unsigned int adc_data, adc_mv;
    adc_data = ADCW;
    adc_mv = (unsigned long)adc_data * 5000 / 1024;
    adc_to_disbuffer(adc_mv);
}

void main(void)
{
    // 端口初始化
    PORTC = 0xFF; // 段码初始关闭
    DDRC = 0xFF;

    PORTD = 0; // 位选初始关闭
    PORTA.6 = 0;
    DDRD = 0b01110000;
    DDRA.6 = 1;

    // T/C0 初始化
    TCCR0 = 0x0B;
    TCNT0 = 0x00;
    OCR0 = 0x7C;
    TIMSK = 0x02;

    // ADC 初始化
    ADMUX = 0x40;
    SFIOR &= 0x1F;
    SFIOR |= 0x60;
    ADCSRA = 0xAD;

#asm("sei")

    while (1)
    {
        if (time_2ms_ok)
        {
            display();
            time_2ms_ok = 0;
        }
    }
}