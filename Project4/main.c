#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "bits_operations.h"

uint16_t mem_read(uint16_t address);
void PUTS();

uint16_t memory[UINT16_MAX];





enum MEMORY {
	MR_KBSR = 0xFE00, /* keyboard status */
	MR_KBDR = 0xFE02,  /* keyboard data */
	R_ZERO = 0xFFFF /* special zero register */
};

enum TRAPS {
	TRAP_GETC = 0x20,  /* get character from keyboard */
	TRAP_OUT = 0x21,   /* output a character */
	TRAP_PUTS = 0x22,  /* output a word string */
	TRAP_IN = 0x23,    /* input a string */
	TRAP_PUTSP = 0x24, /* output a byte string */
	TRAP_HALT = 0x25   /* halt the program */
};

enum OPPCODES {
	OP_BR = 0, /* branch */ //+
	OP_ADD,    /* add  */ //+
	OP_LD,     /* load */ //+
	OP_ST,     /* store */ //+
	OP_JSR,    /* jump register */ //+
	OP_AND,    /* bitwise and */ //+
	OP_LDR,    /* load register */ //+
	OP_STR,    /* store register */ //+
	OP_RTI,    /* unused */ //+
	OP_NOT,    /* bitwise not */ //+
	OP_LDI,    /* load indirect */ //+
	OP_STI,    /* store indirect */ //+
	OP_JMP,    /* jump */ //+
	OP_RES,    /* reserved (unused) */ //+
	OP_LEA,    /* load effective address */ //+
	OP_TRAP    /* execute trap */
};

enum REGISTERS {
	R_R0 = 0,
	R_R1,
	R_R2,
	R_R3,
	R_R4,
	R_R5,
	R_R6,
	R_R7,
	R_PC, //Counter comands (Счетчик команд)
	R_COND, //Condition flags (Флаги условий)
	R_COUNT //Numbers of registers (Количесво регистров)
};

enum FLAGS {
	FL_POS = 1 << 0, /* P */
	FL_ZRO = 1 << 1, /* Z */
	FL_NEG = 1 << 2, /* N */
};

uint16_t reg[R_COUNT];

//typedef struct VM {
//
//	
//
//	
//}VM;


void mem_write(uint16_t addres, uint16_t value) {
	if (memory[addres] == R_ZERO) memory[addres] = 0;
	else memory[addres] = value;
}


void update_flags(uint16_t r) //Каждый раз когда заносится значение в регистр R0 - R7 мы вызываем эту функцию
{
	if (reg[r] == 0)
	{
		reg[R_COND] = FL_ZRO;
	}
	else if (reg[r] >> 15) /* a 1 in the left-most bit indicates negative */
	{
		reg[R_COND] = FL_NEG;
	}
	else
	{
		reg[R_COND] = FL_POS;
	}
}


//СЛОЖЕНИЕ 2х РЕГИСТРОВ И ЗАПИСЬ В 3й

void ADD(uint16_t instr) {
	uint16_t r0 = (instr >> 9) & 0x7; //СМЕЩАЕМ ВПРАВО до начала зоны регистра и после умножаем на число которое в двоичном виде будет заполнять все биты этого регистра, в данном случае это 111 в 16 ричной системе это 0х7 дабы обрезать число и понять который из 7 регистров нам доступен
	uint16_t r1 = (instr >> 6) & 0x7;
	uint16_t flag = (instr >> 5) & 0x1;
	if (flag) {
		uint16_t imm5 = sign_extend(instr & 0x1F, 5);
		reg[r0] = reg[r1] + imm5;
	}
	else {
		uint16_t r2 = instr & 0x7;
		reg[r0] = reg[r1] + reg[r2];
	}
	update_flags(r0);
}

void LDI(uint16_t instr) {
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset)); //reg[R_PC] содержит саму инструкцию и мы прибавляем к ней значение адреса которое содержит адрес где и хранится сама переменная и извлекаем значения из этого адреса внешним mem_read (offset принимает значения от -256 до 255)
	update_flags(r0);
}

void LD(uint16_t instr) {
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	reg[r0] = mem_read(reg[R_PC] + pc_offset);
	update_flags(r0);
}


void ST(uint16_t instr) {
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	mem_write(reg[R_PC] + pc_offset, reg[r0]);
}


void STI(uint16_t instr) {
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);
}


void STR(uint16_t instr) {
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t base_r = (instr >> 6) & 0x7;
	uint16_t pc_offset = sign_extend(instr & 0x3F, 6);
	mem_write(reg[base_r] + pc_offset, reg[r0]);
}

void RTI(uint16_t instr) {
	//Unused
}

void BR(uint16_t instr) {
	uint16_t cond_flag = (instr >> 9) & 0x7;
	uint16_t pc_offset = sign_extend((instr) & 0x1ff, 9);
	if (cond_flag & reg[R_COND]) {
		reg[R_PC] += pc_offset;
	}
}

void JSR(uint16_t instr) {
	if ((instr >> 11) & 0x1) {
		uint16_t pc_offset = sign_extend(instr & 0x7FF, 11);
		reg[R_PC] += pc_offset;
	}
	else {
		uint16_t r1 = (instr >> 6) & 0x7;
		reg[R_PC] = reg[r1];
	}
}


void JMP(uint16_t instr) {
	uint16_t base_r = (instr >> 6) & 0x7;
	reg[R_PC] = reg[base_r];
}

void NOT(uint16_t instr) {
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t r1 = (instr >> 6) & 0x7;

	reg[r0] = ~reg[r1];
	update_flags(r0);
}

void AND(uint16_t instr) {
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t r1 = (instr >> 6) & 0x7;
	uint16_t flag = (instr >> 5) & 0x1;
	if (flag) {
		uint16_t imm5 = sign_extend(instr & 0x1F, 5);
		reg[r0] = reg[r1] & imm5;
	}
	else {
		uint16_t r2 = instr & 0x7;
		reg[r0] = reg[r1] & reg[r2];
	}
	update_flags(r0);
}

void LEA(uint16_t instr) {
	uint16_t r0 = (instr >> 9) & 0x7;
	uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
	reg[r0] = reg[R_PC] + pc_offset;
	update_flags(r0);
}

void TRAP(uint16_t instr) {

	switch (instr & 0xFF)
	{
	case TRAP_GETC:
		reg[R_R0] = (uint16_t)getchar();
		break;
	case TRAP_OUT:
		break;
	case TRAP_PUTS:
		PUTS();
		break;
	case TRAP_IN:
		break;
	case TRAP_PUTSP:
		break;
	case TRAP_HALT:
		puts("HALT");
		fflush(stdout);
		system("pause");
		exit(0);
		break;
	}
}


typedef struct s_Node {
	struct s_Node *next;
	void* value;
}Node;

typedef struct s_List {
	int size;
	Node * head;
	Node * tail;
}List;

typedef struct Stack {
	List* list;
}Stack;

List*createList() {
	List*list = malloc(sizeof(List));
	list->size = 0;
	list->head = NULL;
	list->tail = list->head;
	return list;
}

void listPush(List*list, void *value) {
	Node *node = malloc(sizeof(Node));
	if (list->size == NULL) {
		node->value = value;
		node->next = list->head;
		list->tail = node;
		list->head = node;
	}
	else {
		node->value = value;
		node->next = list->head;
		list->head = node;
	}
	list->size++;
}

void * listPop(List*list) {
	if (list->size == NULL) return NULL;
	Node*node = list->head;
	void * ret_val = node->value;
	list->size--;
	list->head = node->next;
	free(node);
	if (list->size == NULL) {
		list->head = NULL;
		list->tail = NULL;
	}
	return ret_val;
}

void listPushBack(List*list, void*value) {
	Node*node = malloc(sizeof(Node));
	node->value = value;
	list->tail->next = node;
	list->tail = node;
	list->size++;
}

void * listPeek(List*list) {
	if (list->size == NULL) return NULL;
	return list->head->value;
}

void listFree(List*list) {
	for (int i = 0; list->size; i++)
		listPop(list);
	free(list);
}

Stack* createStack() {
	Stack*stack = malloc(sizeof(Stack));
	stack->list = createList();
	return stack;
}

void stackPush(Stack*stack, void*value) {
	listPush(stack->list, value);
}

void* stackPeek(Stack*stack) {
	if (stack->list->size == NULL) return NULL;
	return stack->list->head->value;
}

void *stackPop(Stack*stack) {
	return listPop(stack->list);
}

int stackCount(Stack*stack) {
	return stack->list->size;
}




uint16_t mem_read(uint16_t address) {
	if (address == 60000)
	{
		if (0)
		{
			memory[60000] = (1 << 15);
			memory[61000] = getchar();
		}
		else
		{
			memory[60000] = 0;
		}
	}
	if (address == 65000) { memory[65000] = 0; } /* zero register */
	return memory[address];
}

void PUTS() {
	uint16_t * c = memory + reg[R_R0]-1;
	while (*c) {
		putc((char)*c, stdout);
		++c;
	}
	fflush(stdout);
}

int main() {
	enum { PC_START = 0x3000 };
	reg[R_PC] = PC_START;//Счетчик инструкций
	memory[PC_START] = 0xE00A;
	char * string = "Hello world\n";
	static int i;
	while (i < strlen(string)) {
		memory[PC_START + 10 + i]= *(string+i);
		i++;
	}
	memory[PC_START + 1] = 0xF022;
	memory[PC_START + 2] = 0xF025;
		while (1) {
			uint16_t instr = mem_read(reg[R_PC]++); //ПОЛУЧЕНИЕ ИНСТРУКЦИИ
			uint16_t op = instr >> 12;
			switch (op)
			{
			case OP_ADD:
				ADD(instr);
				break;
			case OP_AND:
				AND(instr);
				break;
			case OP_NOT:
				NOT(instr);
				break;
			case OP_BR:
				BR(instr);
				break;
			case OP_JMP:
				JMP(instr);
				break;
			case OP_JSR:
				JSR(instr);
				break;
			case OP_LD:
				LD(instr);
				break;
			case OP_LDI:
				LDI(instr);
				break;
			case OP_LDR:
				//LDR(instr);
				break;
			case OP_LEA:
				LEA(instr);
				break;
			case OP_ST:
				ST(instr);
				break;
			case OP_STI:
				STI(instr);
				break;
			case OP_STR:
				STR(instr);
				break;
			case OP_TRAP:
				TRAP(instr);
				break;
			case OP_RES:
			case OP_RTI:
			default:
				abort();
				break;
			}
		}
	system("pause");
	return 0;
}

