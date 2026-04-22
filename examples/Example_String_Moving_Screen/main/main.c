#include <stdio.h>
#include <string.h>
#include "rom/ets_sys.h"
#include "screen_tools.h"

// 1. 외부에서 고정 배열(정적 배열) 선언
// (1x4 글자 모듈이라면 픽셀 해상도는 8x32가 됩니다)
#define PIXEL_ROWS 8
#define PIXEL_COLS 32

char my_screen[PIXEL_ROWS][PIXEL_COLS];
char* screen_ptrs[PIXEL_ROWS]; // char**를 속이기 위한 징검다리 배열

void app_main(void)
{
    char my_text[] = "HELLO WORLD NICE TO MEET YOU";

    // 2. 징검다리 포인터 배열 세팅 (malloc 없이 구조체 완벽 호환!)
    for (int i = 0; i < PIXEL_ROWS; i++) {
        screen_ptrs[i] = my_screen[i];

        // 배열 초기화 (초기 더미값 지우기)
        for (int j = 0; j < PIXEL_COLS; j++) {
            my_screen[i][j] = ' ';
        }
    }

    // 3. 스크린 설정 구조체 초기화
    screen_setup_t setup;
    setup.max_r = PIXEL_ROWS;
    setup.max_c = PIXEL_COLS;
    setup.dir = MOVING_SCREEN_LEFT;

    // 구조체에 포인터 배열 꽂기! (이제 setup.screen[r][c]로 완벽 작동)
    setup.screen = screen_ptrs;

    // 출력할 데이터 
    setup.data = my_text;

    // 타일 스크롤링 위치 초기화
    setup.act.x = 0;
    setup.act.y = 0;
    setup.act.step = 0;

    // 최초 1회 렌더링 프레임 생성
    make_screen(&setup);

    // 터미널 화면 찌꺼기 완전 삭제
    printf("\033[2J");

    // 4. 메인 애니메이션 루프
    int total_length = strlen(my_text);
    int total_pixels = total_length * 8 + PIXEL_COLS; // 글자가 화면을 완전히 빠져나갈 때까지

    for (int i = 0; i < total_pixels; i++)
    {
        // ★ 콘솔 깜빡임 제거: 커서를 (0,0)으로 이동 후 덮어쓰기
        printf("\033[H");

        // 미세 스크롤 데이터 적용
        moving_screen(&setup);

        // 화면 출력
        //print_screen(&setup);
        print_screen_int(&setup);

        // 스크롤 속도 조절 (50ms)
        ets_delay_us(50);
    }

    // 5. 메모리 해제 (make_screen 내부의 frame만 해제)
    free_screen();
    // my_screen은 정적 배열이므로 free 할 필요 없음! (메모리 관리 완벽)
}