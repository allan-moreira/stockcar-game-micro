#include <reg51.h>
#include <intrins.h>
#include <stdlib.h>
#include "Font_Header.h"

#define Data_Port P1

// Pinos de Controle do GLCD
sbit RS  = P3^2;
sbit E   = P3^0;
sbit CS1 = P3^4;
sbit CS2 = P3^3;

// Pinos de Controle do Jogador
sbit Dir = P3^1;  // Move para direita
sbit Esq = P3^5;  // Move para esquerda

#define WIDTH 9
#define HEIGHT 7
#define PLAYER_Y (HEIGHT - 1)
#define MAX_ENEMIES 3

typedef struct { unsigned char x, y, active; } Car;

unsigned char track[HEIGHT][WIDTH];
Car player, enemies[MAX_ENEMIES];
unsigned int score = 0;
unsigned char lives = 3, speed = 150;
signed char track_offset = 0;

void delay(unsigned int k) {
    unsigned int i,j;
    for (i=0; i<k; i++) for (j=0; j<112; j++);
}

void GLCD_Command(unsigned char c) {
    Data_Port = c; RS=0; E=1; nop(); E=0; nop();
}

void GLCD_Data(unsigned char d) {
    Data_Port = d; RS=1; E=1; nop(); E=0; nop();
}

void GLCD_Init() {
    CS1=CS2=1; delay(20);
    GLCD_Command(0x3E); GLCD_Command(0x40);
    GLCD_Command(0xB8); GLCD_Command(0xC0);
    GLCD_Command(0x3F);
}

void GLCD_ClearAll() {
    unsigned char p, c;
    for (p=0; p<8; p++) {
        CS1=1; CS2=0; GLCD_Command(0xB8|p); GLCD_Command(0x40);
        for (c=0; c<64; c++) GLCD_Data(0);
        CS1=0; CS2=1; GLCD_Command(0xB8|p); GLCD_Command(0x40);
        for (c=0; c<64; c++) GLCD_Data(0);
    }
}


void GLCD_Char(char page_no, char col, char c) {
    unsigned char column;
    char Page = ((0xB8)+page_no);
    
    // Verifica se o caractere cruza o limite das p ginas (64 colunas)
    if ((col % 64) > 59) {  // Se faltam menos de 5 colunas para o fim da p gina
        // Primeira parte do caractere (na p gina atual)
        CS1 = (col < 64) ? 1 : 0;
        CS2 = (col < 64) ? 0 : 1;
        GLCD_Command(Page);
        GLCD_Command(0x40 | (col % 64));
        
        // Desenha a parte que cabe na p gina atual
        for (column = 0; column < (64 - (col % 64)); column++) {
            GLCD_Data(font[c][column]);
        }
        
        // Segunda parte do caractere (na pr xima p gina)
        CS1 = (col < 64) ? 0 : 1;
        CS2 = (col < 64) ? 1 : 0;
        GLCD_Command(Page);
        GLCD_Command(0x40);  // Come a no in cio da pr xima p gina
        
        // Desenha o restante do caractere
        for (; column < 5; column++) {
            GLCD_Data(font[c][column]);
        }
        
        // Espa o entre caracteres (na pr xima p gina)
        GLCD_Data(0);
    } else {                                                                                                            
        // Caso normal - caractere cabe inteiro em uma p gina
        if (col < 64) {
            CS1 = 1; CS2 = 0;
        } else {
            CS1 = 0; CS2 = 1;
            col -= 64;
        }
        
        GLCD_Command(Page);
        GLCD_Command(0x40 | col);
        
        for (column = 0; column < 5; column++) {
            GLCD_Data(font[c][column]);
        }
        GLCD_Data(0); // Espa o entre caracteres
    }
}


void GLCD_String(unsigned char page, char *str) {
    unsigned char col=0;
    while (*str) { GLCD_Char(page, col, *str++); col+=6; }
}

void update_hud() {
    char buf[21];
    buf[0]= 23; buf[1]=14; buf[2]= 21; buf[3]= 22; buf[4]= 16; buf[5]=12;
    buf[6]= 1 +(score/10000)%10; buf[7]= 1 +(score/1000)%10;
    buf[8]= 1 +(score/100)%10; buf[9]= 1 +(score/10)%10;
    buf[10]= 1 +score%10; buf[11]= 0; buf[12]= 0; buf[13]= 25;
    buf[14]= 18; buf[15]= 15; buf[16]= 13; buf[17]= 23; buf[18]= 12;
    buf[19]= 1 +lives; buf[20]=0;
    GLCD_String(0, buf);
}

void init_game() {
    unsigned char i,j;
    track_offset=0; score=0; lives=3;
    for (i=0; i<HEIGHT; i++) for (j=0; j<WIDTH; j++)
        track[i][j]=(j==0||j==WIDTH-1)? 11: 0;
    player.x=WIDTH/2; player.y=PLAYER_Y;
    track[player.y][player.x]= 26;
    for (i=0; i<MAX_ENEMIES; i++) enemies[i].active=0;
}

void spawn_enemy() {
    unsigned char i, x, left=1+track_offset, right=WIDTH-2+track_offset;
    if (left<1) left=1; if (right>WIDTH-2) right=WIDTH-2;
    for (i=0; i<MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            do { x=left+(rand()%(right-left+1)); } while (track[0][x]!=0);
            enemies[i].x=x; enemies[i].y=0; enemies[i].active=1; track[0][x]=26;
            break;
        }
    }
}

void game_over() {
	char a[] = {0,0,17,13,20,16,0,21,25,16,22};
    GLCD_ClearAll(); GLCD_String(3, a); while(1);
}

void check_collisions() {
    unsigned char i;
    for (i=0; i<MAX_ENEMIES; i++) {
        if (enemies[i].active && enemies[i].y==player.y && enemies[i].x==player.x) {
            lives--; enemies[i].active=0; track[player.y][player.x]=0;
            if (!lives) game_over();
        }
    }
}

void update_game() {
    unsigned char i,j, left, right;
    if (Dir==0 && player.x<WIDTH-2) { track[player.y][player.x]=0; player.x++; delay(50); }
    else if (Esq==0 && player.x>1) { track[player.y][player.x]=0; player.x--; delay(50); }
    if ((rand()%4)==0) {
        char s=(rand()%3)-1;
        if (track_offset+s>=-2 && track_offset+s<=2) track_offset+=s;
    }
    for (i=HEIGHT-1; i>0; i--)
        for (j=0; j<WIDTH; j++) track[i][j]=track[i-1][j];
    left=1+track_offset; right=WIDTH-2+track_offset;
    if (left<0) left=0; if (right>=WIDTH) right=WIDTH-1;
    for (i=0; i<WIDTH; i++) track[0][i]=(i==left||i==right)? 11:((i>left&&i<right)? 0: 0);
    for (i=0; i<MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            track[enemies[i].y][enemies[i].x]=0;
            enemies[i].y++;
            if (enemies[i].y>=HEIGHT) enemies[i].active=0;
            else track[enemies[i].y][enemies[i].x]=26;
        }
    }
    if ((rand()%10)==0) spawn_enemy();
    if (track[player.y][player.x]==11) {
        lives--; track[player.y][player.x]=0;
        if (!lives) game_over();
        player.x=WIDTH/2;
    }
    track[player.y][player.x]=26;
    check_collisions(); update_hud();
}

void draw_game() {
    unsigned char i,j,x;
    for (i=0; i<HEIGHT; i++)
        for (j=0; j<WIDTH; j++) {
            x=20+j*6;
            if (x<128) GLCD_Char(i+1, x, track[i][j]);
        }
}

void tela_inicio() {
	char a[] = {0,0,23,24,21,14,19,0,14,13,22};
    GLCD_ClearAll(); GLCD_String(3, a); delay(2000);
}

void main() {
    tela_inicio(); GLCD_Init(); GLCD_ClearAll(); init_game();
    while (1) {
        update_game(); draw_game(); delay(speed); score++;
        if ((score%100)==0 && speed>50) speed-=5;
		}
}