#include <stdint.h>
#define VIDEO_MEMORY ((uint16_t*)0xb8000)
#define KEYBOARD_PORT 0x60

static uint8_t cursor_x = 0, cursor_y = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void update_cursor() {
    uint16_t position = cursor_y * 80 + cursor_x;
    outb(0x3D4, 14); // Cursor high byte
    outb(0x3D5, position >> 8);
    outb(0x3D4, 15); // Cursor low byte
    outb(0x3D5, position);
}

void set_cursor_position(int x, int y) {
    cursor_x = x;
    cursor_y = y;
    update_cursor();
}

void clear_screen() {
    for (cursor_y = 0; cursor_y < 25; ++cursor_y)
        for (cursor_x = 0; cursor_x < 80; ++cursor_x)
            VIDEO_MEMORY[80 * cursor_y + cursor_x] = (VIDEO_MEMORY[80 * cursor_y + cursor_x] & 0xFF00) | ' ';
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void printf(const char* str) {
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else {
            VIDEO_MEMORY[80 * cursor_y + cursor_x] = (VIDEO_MEMORY[80 * cursor_y + cursor_x] & 0xFF00) | str[i];
            cursor_x++;
        }

        if (cursor_x >= 80) {
            cursor_x = 0;
            cursor_y++;
        }

        if (cursor_y >= 25) {
            clear_screen();
        }

        update_cursor();
    }
}

void putchar(char c) {
    char str[2] = {c, '\0'};
    printf(str);
}

void print_number(int64_t num) {      //In ket qua so nguyen
	char buffer[21];
	int i = 0, isNegative = 0;

	if (num == 0) {
    	putchar('0');
    	return;
	}

	if (num < 0) {
    	isNegative = 1;
    	num = -num;
	}

	while (num > 0) {
    	buffer[i++] = (num % 10) + '0';
    	num /= 10;
	}

	if (isNegative) buffer[i++] = '-';

	while (i--) putchar(buffer[i]);
}

void print_float(double num) {              //In ket qua so thuc
    int64_t integer_part = (int64_t)num;
    int64_t decimal_part = (int64_t)((num - integer_part) * 100); // Lấy hai chữ số thập phân

    print_number(integer_part); // In phần nguyên
    putchar('.'); // In dấu '.'
    if (decimal_part < 10) {
        putchar('0'); // In số 0 nếu phần thập phân chỉ có 1 chữ số
    }
    print_number(decimal_part); // In phần thập phân
}


static char keymap[128][2] = {
    {0, 0}, {27, 27}, {'1', '!'}, {'2', '@'}, {'3', '#'}, {'4', '$'}, {'5', '%'}, {'6', '^'},
    {'7', '&'}, {'8', '*'}, {'9', '('}, {'0', ')'}, {'-', '_'}, {'=', '+'}, {'\b', '\b'},
    {'\t', '\t'}, {'q', 'Q'}, {'w', 'W'}, {'e', 'E'}, {'r', 'R'}, {'t', 'T'}, {'y', 'Y'}, {'u', 'U'},
    {'i', 'I'}, {'o', 'O'}, {'p', 'P'}, {'[', '{'}, {']', '}'}, {'\n', '\n'}, {0, 0}, {'a', 'A'},
    {'s', 'S'}, {'d', 'D'}, {'f', 'F'}, {'g', 'G'}, {'h', 'H'}, {'j', 'J'}, {'k', 'K'}, {'l', 'L'},
    {';', ':'}, {'\'', '"'}, {'`', '~'}, {0, 0}, {'\\', '|'}, {'z', 'Z'}, {'x', 'X'}, {'c', 'C'},
    {'v', 'V'}, {'b', 'B'}, {'n', 'N'}, {'m', 'M'}, {',', '<'}, {'.', '>'}, {'/', '?'}, {0, 0},
};

uint8_t shift_pressed = 0;
uint8_t  caps_lock_active = 0;

void handle_special_keys(uint8_t scan_code) {
    if (scan_code == 0x2A || scan_code == 0x36) shift_pressed = 1;
    else if (scan_code == 0xAA || scan_code == 0xB6) shift_pressed = 0;
    else if (scan_code == 0x3A) caps_lock_active = !caps_lock_active;
}

void handle_backspace(int* index) {
    if (*index > 0) {
        (*index)--;
        cursor_x--;
        VIDEO_MEMORY[80 * cursor_y + cursor_x] = (VIDEO_MEMORY[80 * cursor_y + cursor_x] & 0xFF00) | ' ';
        update_cursor();
    }
}

char keyboard_getchar() {
    char key = 0;
    while (key == 0) {
        uint8_t scancode = inb(KEYBOARD_PORT);
        handle_special_keys(scancode);
        if (scancode < 128) {
            key = shift_pressed ? keymap[scancode][1] : keymap[scancode][0];
            if (caps_lock_active && key >= 'a' && key <= 'z') {
                key -= 32;
            }
            while (inb(KEYBOARD_PORT) == scancode);
        }
    }
    return key;
}

void get_expression(char* expr) {
    int index = 0;
    char c;
    
    while (1) {
        c = keyboard_getchar();
        if (c == '\n') {
            expr[index] = '\0';
            putchar(c); // Print newline
            break;
        } else if (c == '\b') {
            handle_backspace(&index);
        } else {
            putchar(c);
            expr[index++] = c;
        }
    }
}

void print_menu() {
    clear_screen();
    
    // Tính toán vị trí giữa cho nội dung menu
    int start_x = (80 - 36) / 2;  // Giả sử menu có độ rộng 35 ký tự (chieu rong)
    int start_y = 2 ; //Chieu cao
    set_cursor_position(start_x, start_y);  // (22 ; 2)
    printf("+---------------------------------------+");
    set_cursor_position(start_x, start_y + 1);
    printf("|     Calculator by The Chill Team      |");
    set_cursor_position(start_x, start_y + 2);
    printf("+---------------------------------------+");
    set_cursor_position(start_x, start_y + 3);
    printf("| Enter an expression (e.g., 2+3):      |");
    set_cursor_position(start_x, start_y + 4);
    printf("+---------------------------------------+");
    set_cursor_position(start_x, start_y + 5);
    printf("|                                       |");
    set_cursor_position(start_x, start_y + 6);
    printf("|                                       |");
    set_cursor_position(start_x, start_y + 7);
    printf("|                                       |");
    set_cursor_position(start_x, start_y + 8);
    printf("+---------------------------------------+");
     set_cursor_position(start_x, start_y + 9);
    printf("|    S    |    /    |    *    |    -    |");      //S la phim dac biet
    set_cursor_position(start_x, start_y + 10);
    printf("|---------+---------+---------+---------|");
    set_cursor_position(start_x, start_y + 11);
    printf("|    7    |    8    |    9    |         |");
    set_cursor_position(start_x, start_y + 12);
    printf("|---------+---------+---------|    +    |");
    set_cursor_position(start_x, start_y + 13);
    printf("|    4    |    5    |    6    |         |");
    set_cursor_position(start_x, start_y + 14);
    printf("|---------+---------+---------+---------|");
    set_cursor_position(start_x, start_y + 15);
    printf("|    1    |    2    |    3    |         |");
    set_cursor_position(start_x, start_y + 16);
    printf("|---------+---------+---------|    =    |");
    set_cursor_position(start_x, start_y + 17);
    printf("|         0         |    .    |         |");
    set_cursor_position(start_x, start_y + 18);
    printf("+---------------------------------------+");
    
    // Di chuyển con trỏ tới vị trí nhập
    set_cursor_position(start_x + 2, start_y + 5);  //Vi tri nhap la (24 ; 7)
    update_cursor();
}

void print_result(int64_t result) {
    // In kết quả ra giữa màn hình sau khi tính toán dua vao vi tri nhap            
    set_cursor_position(24, 8);
    printf("Result: ");
    print_number(result);
    printf("\n");
}

void pause() {
    // Thông báo ở vị trí giữa dưới cùng của màn hình dua vao vi tri nhap           
    set_cursor_position(24, 9);
    printf("Press Any Key To Continue...");
    keyboard_getchar();
    clear_screen();
}

int64_t parse_number(char** str) {
    int64_t num = 0;
    int isNegative = 0;

    if (**str == '-') {
        isNegative = 1;
        (*str)++;
    }

    while (**str >= '0' && **str <= '9') {
        num = num * 10 + (**str - '0');
        (*str)++;
    }

    return isNegative ? -num : num;
}

int64_t perform_operation(int64_t num1, char op, int64_t num2, int* error) {
    switch (op) {
        case '+': return num1 + num2;
        case '-': return num1 - num2;
        case '*': return num1 * num2;
        case '/':
            if (num2 == 0) {
                *error = 1;
                set_cursor_position(24, 8);
                printf("Error: Division by zero!\n");
                return 0;
            }
            else {
                double result = (double)num1 / (double)num2;
                set_cursor_position(24, 8);
                printf("Result: "); 
                print_float(result);   // In kết quả dang thập phân
                printf("\n");
                *error = 1;
                return 0; // Trả về 0 vì kết quả phép chia đã được in dưới dạng số thực
            }
        default:
            *error = 1;
            set_cursor_position(24, 8);
            printf("Invalid operator!\n");
            return 0;
    }
}

void kernel_main(void) {
    while (1) {
        char expr[20];
        int error = 0;
        print_menu();
        get_expression(expr);

        char* p = expr;
        int64_t num1 = parse_number(&p);
        char op = *p++;
        int64_t num2 = parse_number(&p);

        int64_t result = perform_operation(num1, op, num2, &error);

        if (error == 0) {
            print_result(result);
            printf("\n");
        }
        pause();
    }
}

