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

//Trap Codes
enum{
    T_GETC =0,
    T_OUT,
    T_PUTS,
    T_IN,
    T_PUTSP,
    T_HALT
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
                //Destination Register (3 bits 11:9)
                uint16_t r0 = (instr >> 9) & 0x7;
                //SR1 (3 bits 8:6)
                uint16_t r1 = (instr >> 6) & 0x7;
                //Check for imm mode (1 bit 5:5)
                uint16_t imm5_flag = (instr >> 5) & 0x1;
                if (imm5_flag){
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] + imm5;
                }else{
                    uint16_t r2 = (instr & 0x7); //SR2 3 bits 2:0
                    reg[r0] = reg[r1] + reg[r2];
                }
                update_flags(r0);
            case OP_AND:
                //Destination Register (3 bits 11:9)
                uint16_t r0 = (instr >> 9) & 0x7;
                //SR1 (3 bits 8:6)
                uint16_t r1 = (instr >> 6) & 0x7;
                //Check for imm mode (1 bit 5:5)
                uint16_t imm5_flag = (instr >> 5) & 0x1;
                if (imm5_flag){
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] & imm5;
                }else{
                    uint16_t r2 = (instr & 0x7); //SR2 3 bits 2:0
                    reg[r0] = reg[r1] & reg[r2];
                }
                update_flags(r0);
            case OP_NOT:
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 9) & 0x7;
                reg[r0] = ~reg[r1];
                update_flags(r0);
            case OP_BR:
                //Check if Negative,Zero,Positive Flags set
                uint16_t cond = (instr >> 9) & 0x7;
                if(cond & reg[R_COND]){
                    //Flag has been tested and matches register branch condition valid
                    //Move Program Counter to the branch offset
                    reg[R_PC] = reg[R_PC] + sign_extend(instr &0x1FF, 9);
                }
            case OP_JMP:
                //Move to location 3 bits 8:6
                uint16_t r1 = (instr >> 6) & 0x7;
                reg[R_PC] = reg[r1];
            case OP_JSR:
                //Save Program Counter to R7
                reg[R_R7] = reg[R_PC];
                //Check if JSR or JSRR 1 bit 11:11
                if (((instr >> 11) & 0x1) == 0){
                    //JSRR Jump to base register
                    reg[R_PC] = reg[(instr >> 6) & 0x7];
                }else{
                    //JSR Jump to offset
                    reg[R_PC] = reg[R_PC] + sign_extend(instr & 0x7FF, 11);
                }
            case OP_LD:
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t offset = sign_extend((instr & 0x1FF), 9);
                //Get the memory contents at PC + offset and load into DR
                reg[r0] = mem_read(reg[R_PC] + offset);
                //Update Register flags
                update_flags(r0);
            case OP_LDI:
                //Load the address pointed to in memory to DR
                uint16_t r0 = (instr >> 9) &0x7;
                uint16_t offset = sign_extend((instr & 0x1FF), 9);
                //r0 is the memory address pointed to at the memory address specified by the PC + offset
                reg[r0] = mem_read(mem_read(reg[R_PC] + offset));
                update_flags(r0);
            case OP_LDR:
                uint16_t r0 = (instr >> 9) &0x7;
                uint16_t baseR = (instr >> 6) & 0x07;
                uint16_t offset = sign_extend(instr & 0x3f, 6);
                //Return Memory at base register  + offset
                reg[r0] = mem_read(reg[baseR] + offset);
                update_flags(r0);
            case OP_LEA:
                uint16_t r0 = (instr >> 9) &0x7;
                uint16_t offset = sign_extend((instr & 0x1FF), 9);
                reg[r0] = reg[R_PC] + offset;
                update_flags(r0); 
            case OP_ST:
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t offset = sign_extend((instr >> 0x1FF), 9);
                mem_write(reg[R_PC] + offset, r0);
            case OP_STI:
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t offset = sign_extend((instr & 0x1FF), 9);
                mem_write(mem_read(reg[R_PC]+ offset), r0);
            case OP_STR:
                uint16_t r0 = (instr >> 9) &0x7;
                uint16_t baseR = (instr >> 6) & 0x07;
                uint16_t offset = sign_extend(instr & 0x3f, 6);
                mem_write(reg[baseR] + offset, r0);
            case OP_TRAP:
                //Store current PC
                reg[R_R7] = reg[R_PC];
                //Based Run Trap Code
                switch(instr & 0xFF){
                
                }
            case OP_RES:
            case OP_RTI:
            default:
                break;

        }
    }
}