#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "screen_tools.h"
#include "letter_maps.h"

// frame 포인터 전역 변수 (초기화)
int letter_rows = 0;
int letter_cols = 0;
int total_lines = 0;

char** frame = NULL;

int make_screen(screen_setup_t* setup)
{
    if (setup->data == NULL) return -1;

    // 1. 전체 줄 수(total_lines) 계산 로직
    // 매번 새로 계산하여 데이터 변경에 대응합니다.
    int cnt = 1;
    char* p = setup->data;
    while (*p != '\0') {
        if (*p == '\n') cnt++;
        p++;
    }
    total_lines = cnt; // 전역 변수 업데이트

    // 2. 출력 가능 글자 수 및 프레임 사이즈 계산
    letter_rows = setup->max_r / MAX_MAP_ROWS;
    letter_cols = setup->max_c / MAX_MAP_COLS;

    const int rows = letter_rows + 2;
    const int cols = letter_cols + 2;

    // 3. frame 메모리 관리 (최초 할당 및 재사용)
    if (frame == NULL) {
        frame = (char**)malloc(rows * sizeof(char*));
        for (int i = 0; i < rows; i++) {
            frame[i] = (char*)malloc(cols * sizeof(char));
        }
    }

    // 프레임 초기화 (마진 포함 전체 공백)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            frame[i][j] = ' ';
        }
    }

    // 4. 데이터 배치 로직 (Pointer Offset 활용)
    char* line_start = setup->data;
    int current_line_idx = 0;

    while (line_start != NULL && *line_start != '\0') {
        char* line_end = strchr(line_start, '\n');
        int current_line_len = (line_end) ? (int)(line_end - line_start) : (int)strlen(line_start);

        // 현재 줄이 렌더링 범위(setup->act.y 기준)에 있는지 검사
        if (current_line_idx >= setup->act.y && current_line_idx < setup->act.y + letter_rows) {
            int r_idx = current_line_idx - setup->act.y;

            for (int c_idx = 0; c_idx < letter_cols; c_idx++) {
                int data_x = setup->act.x + c_idx;

                // 데이터 범위 내에 있을 때만 frame의 마진 안쪽(1, 1부터)에 복사
                if (data_x >= 0 && data_x < current_line_len) {
                    frame[r_idx + 1][c_idx + 1] = line_start[data_x];
                }
            }
        }

        if (line_end) {
            line_start = line_end + 1;
            current_line_idx++;
        }
        else {
            break;
        }
    }

    return 0;
}

void moving_screen(screen_setup_t* setup)
{
    int step = setup->act.step;

    for (int r = 0; r < setup->max_r; r++) {
        for (int c = 0; c < setup->max_c; c++) {

            // 왼쪽, 위쪽 고스트 마진을 더해주어 음수(-) 인덱스 접근 원천 차단
            int global_p_x = c + MAX_MAP_COLS;
            int global_p_y = r + MAX_MAP_ROWS;

            // 방향 비트마스킹 처리
            if (setup->dir & MOVING_SCREEN_LEFT) global_p_x += step;
            else if (setup->dir & MOVING_SCREEN_RIGHT) global_p_x -= step;
            else if (setup->dir & MOVING_SCREEN_UP) global_p_y += step;
            else if (setup->dir & MOVING_SCREEN_DOWN) global_p_y -= step;

            // frame에서 몇 번째 글자인지 몫으로 계산
            int char_r = global_p_y / MAX_MAP_ROWS;
            int char_c = global_p_x / MAX_MAP_COLS;

            // 해당 글자에서 몇 번째 픽셀인지 나머지로 계산
            int pixel_r = global_p_y % MAX_MAP_ROWS;
            int pixel_c = global_p_x % MAX_MAP_COLS;

            // 우회 캐스팅 적용 (메모리 뻗음 방지)
            const char** map = getLetterMap(frame[char_r][char_c]);
            const char* real_map = (const char*)map;

            // 1차원 배열로 인식된 데이터에서, 곱하기와 더하기 수식으로 2차원 좌표를 찾아냄
            setup->screen[r][c] = real_map[pixel_r * MAX_MAP_COLS + pixel_c];
        }
    }

    // [수정된 스크롤 좌표 업데이트 로직]
    setup->act.step++;
    if (setup->act.step >= 8) {
        setup->act.step = 0;   // 8픽셀 밀었으면 다시 0으로 리셋

        int data_len = strlen(setup->data);
        if (data_len > 0) {
            if (setup->act.x == data_len - 1)   INCREASE(setup->act.y, total_lines);
			if ((setup->dir & MOVING_SCREEN_LEFT) != 0) INCREASE(setup->act.x, data_len);
			if ((setup->dir & MOVING_SCREEN_RIGHT) != 0)    DECREASE(setup->act.x, data_len);
        }

        // 2. Y축: 전역 변수 total_lines 기준 안전 순환
        if (total_lines > 0) {
			if ((setup->dir & MOVING_SCREEN_UP) != 0)   INCREASE(setup->act.y, total_lines);
			if ((setup->dir & MOVING_SCREEN_DOWN) != 0) DECREASE(setup->act.y, total_lines);
        }

        make_screen(setup);
    }
}

void free_screen()
{
    if (frame != NULL) {
        const int rows = letter_rows + 2;
        for (int i = 0; i < rows; i++) {
            free(frame[i]);
        }
        free(frame);
        frame = NULL; // 댕글링 포인터 방지
    }
}

void print_screen(screen_setup_t* setup)
{
    // ANSI 코드를 사용하여 커서를 무조건 맨 앞으로 이동 후 덮어쓰기
    printf("\033[H");

    for (int i = 0; i < setup->max_r; i++) {
        for (int j = 0; j < setup->max_c; j++) {
            putchar(setup->screen[i][j]);
        }
        putchar('\n');
    }
}

void print_screen_int(screen_setup_t* setup)
{
    unsigned int line = 0;
    // ANSI 코드를 사용하여 커서를 무조건 맨 앞으로 이동 후 덮어쓰기
    printf("\033[H");

    for (int i = 0; i < setup->max_r; i++) {
        for (int j = 0; j < setup->max_c; j++) {
            putchar((setup->screen[i][j] != ' ')?'1':'0');
        }
        putchar('\n');
    }
}