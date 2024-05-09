/*
 * ocr0_pwm_test.c
 *
 * Created: 2024-05-09 오후 2:12:47
 * Author : HARMAN-27
 */ 
#include "button.h"
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

//button.c에 있는 함수를 가져다 쓰는거므로
//main함수 입장 : 이 함수는 내가 정의한게 아니라 다른 곳에 있습니다.
extern void init_button(void);
extern int get_button(int button_pin, int button_number);

// TIMER interrupt 서비스 루틴
// 16.4ms마다 이곳으로 진입한다.
volatile int time_16ms = 0;

ISR(TIMER0_OVF_vect){
	TCNT0 = 0; // 주의 사항 : CTC가 아닌 경우는 반드시 초기화 해줘야한다.
	time_16ms++;
}


// OC0(PB4) : PWM 파형 출력 신호가 나오도록 초기화 한다.
void init_timer0_pwm(){
	// PB4(OC0)를 출력 모드로 설정한다.
	DDRB |= 1 << 4;
	// timer/count0번을 고속 PWM모드로 설정한다.(p.344 표15-3참고)
	TCCR0 |= 1 << WGM01 | 1 << WGM00;
	
	//비반전 모드로 설정하기 : OCR값을 만나면, PULSE가 HIGH -> LOW로 떨어짐
	TCCR0 |= 1 << COM01; // p.344 표 15-4
	//분주비를 1024로 설정하자
	// 16M/1024 = 15625Hz
	// T = 1/f = 1/15625 = 64us(0.0000064sec)
	// 0.000064sec * 255개 pulse 카운트 : 16.4ms마다 timer0 overflow INT가 발생된다.
	TCCR0 |= 1 << CS02 | 1 << CS01 | 1 << CS00; // 1024로 분주하게됨
	TIMSK |= 1 << TOIE0; // timer0 overflow INT설정
	TCNT0 = 0;
}

/*
	16bit 3번 timer/count를 사용한다.
	PWM출력신호
	======
	PE3 : OC3A
	PE4 : OC3B
	PE5 : OC3C <- 모터 연결
	버튼1은 START/STOP
	버튼2는 spped up(OCR3C : 20증가함, MAX : 250)
	버튼 3번 : spped down(OCR3C : 20씩 감소, min = 60)
*/
void init_timer3_pwm(){
	DDRE |= 1 << 3 | 1 << 4 | 1 << 5;                                                                      
	// 모드 5(p.348,표15-6) : 8비트 고속 PWM설정
	TCCR3A |= 1 << WGM30;
	TCCR3B |= 1 << WGM32; // p.348,표15-6
	// 비반전모드선택 : top값이 0x00ff
	// 비교일치 값 지정 : OCR3C에 지정한다
	TCCR3A |= 1 << COM3C1;
	
	// 분주비를 64로 설정, 16000000 / 64 = 250KhZ
	// 256개의 PULSE로한다.
	// 250kHz에서, 256개의 pulse를 count하면 about 1.02ms가 된다.
	// 절반 pulse : 127 -> about 0.5ms
	TCCR3B |= 1 << CS31 |  1 << CS30; //분주비가 64가 된다.
	OCR3C = 0; // Output Compare Register의 약자(p.321)
}

#if 1
int main(void){
	int start = 0;
	
	init_timer3_pwm();
	init_button();
	DDRE |= 0b00000101; //PE0,PE2 확실한 1
	PORTE  &= 0b11111010; // pe2 시작
	
	
	while(1){
		if(get_button(BUTTON1_PIN,BUTTON1)){
			// start & stop
			start = !start; // 반전시켜준다
			if(start){
				OCR3C = 127; // 모터 기본 회전값 중간 speed로 셋팅
				//255에서 절반이 127이고, 127이 normal speed임
				// 127이면 duty가 1ms 절반?
				PORTE  &= 0b11111011; // pe2 시작
			}else{
				OCR3C = 0; //모터 정지
				PORTE &= 0b1111010l;
				PORTE |= 1 << 2 | 1 << 0; // // 1이면 정지
			}
			
		}else if(get_button(BUTTON2_PIN,BUTTON2)){
			// speed-up
			if(OCR3C >= 250)
				OCR3C = 250; //threshold 값으로
			else
				OCR3C += 20;
		}else if(get_button(BUTTON3_PIN,BUTTON3)){
			//spped-down
			if(OCR3C <= 70)
				OCR3C = 50;
			else
				OCR3C -= 20;
		}
	}
	return 0;
}

#endif

#if 0 //LED 밝기 PWM test
int main(void)
{
	int dim = 0;
	int direction= 1;
	init_timer0_pwm();
	sei(); // global 전역 인터럽트 활성화
	
    while (1) 
    {
		// duty cycle을 제어한다 : 1clock의 소요 시간 : 64us
		//R0 : 1(64us)~255(16.4ms)까지 제어
		// 16.4ms단위로 서서히 밝아 지도록 설정한다. OCR0 : 0~255
		if(time_16ms >= 1){
			time_16ms = 0;	
			OCR0 = dim;
			dim += direction; // 밝기 변화
		}
    }
}
#endif