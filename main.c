#include <stdint.h>
#define MEMORY_MAX (1 << 16)



uint16_t memory[MEMORY_MAX];

//Define the Register map
enum{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT //Gives the size of registers
};

uint16_t reg[R_COUNT];

//Createa OpCodes
enum{
    OP_BR = 0, //BRANCH
    OP_ADD, //ADD
    OP_LD, //LOAD
    OP_ST, //STORE
    OP_JSR, //JUMP REGISTER
    OP_AND, //BITWISE AND
    OP_LDR, //LOAD REGISTER
    OP_STR, //STORE REGISTER
    OP_RTI, //
    OP_NOT, //BITWISE NOT
    OP_LDI, //LOAD INDIRECT
    OP_STI, //STORE INDIRECT
    OP_JMP, //JUMP
    OP_RES, //RESERVED
    OP_LEA, //LOAD EFFECTIVE ADDRESS
    OP_TRAP //EXEC TRAP
};

//Condition Flags

enum{
    FL_POS = 1 <<0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2,
};

//Helper Functions
//Extend a number and preserve sign
uint16_t sign_extend(uint16_t number, int bit_count){
    if((number >> (bit_count-1)) & 1){
        number |= (0xFFFF << bit_count);
    }
    return number;
}

//Update the Condition register with OP Flag
void update_flags(uint16_t r){
    if(reg[r] == 0){
        reg[R_COND] = FL_ZRO;
    }else if(reg[r] >> 15){
        reg[R_COND] = FL_NEG;
    }else{
        reg[R_COND] = FL_POS;
    }
}

int main(int argc, const char* argv[]){
    reg[R_COND] = FL_ZRO;

    enum{PC_START = 0x3000};
    reg[R_PC] = PC_START;
    int running = 1;

    //Program load
    if(argc < 2){
        printf("[image-file1]\n");
        exit(2);
    }
    for(int j = 1; j < argc; ++j){
        if(!read_image(argv[j])){
            printf("Failed to load image: %s\n",argv[j]);
            exit(1);
        }
    }

    while(running){
        uint16_t instr = mem_read(reg[R_PC]++);
        uint16_t op = instr >> 12;

        switch(op){
            case OP_ADD:
                break;
            case OP_AND:
                break;
            case OP_NOT:
                break;
            case OP_BR:
                break;
            case OP_JMP:
                break;
            case OP_JSR:
                break;
            case OP_LD:
                break;
            case OP_LDI:
                break;
            case OP_LDR:
                break;
            case OP_LEA:
                break;
            case OP_ST:
                break;
            case OP_STI:
                break;
            case OP_STR:
                break;
            case OP_TRAP:
                break;
            case OP_RES:
            case OP_RTI:
            default:
                break;

        }
    }
}