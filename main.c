#include <stdint.h>
#include <stdio.h>
#include <signal.h>
//Windows Includes
#ifdef _WIN32
#include <Windows.h>
//#include <conio.h>
#endif

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

//Memory Mapp Keyboard Registers
enum{
    MR_KBSR = 0xFE00, //Keyboard Status, key press detected
    MR_KBDR = 0xFE02  //Keyboard Data, which key is pressed
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
//Swap endianess
uint16_t swap_endian(uint16_t x)
{
    //bit shift each byte and join together
    return (x << 8) | (x >> 8);
}

//Read program in from file
void read_program(FILE* file){
    //origin is the memory address that points to the main program location
    uint16_t origin;
    //Read the file to memoery
    fread(&origin, sizeof(origin), 1, file);
    //LC3 programs are big endian flip order to little endian
    origin = swap_endian(origin);

    //load program into memory pointed to at origin
    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p, sizeof(uint16_t), max_read, file);

    //Swap the endianess of the loaded program values
    while(--read > 0){
        *p = swap_endian(*p);
        //move the pointer forward
        ++p;
    }

}

//Read program from location string, check to ensure file exists before opening
int read_image(const char* image_path){
    FILE* file = fopen(image_path, "rb");
    if(!file){
        return 0;
    }
    read_program(file);
    fclose(file);
    return 1;
}

//Keyboard input functions for character WINDOWS
#ifdef _WIN32 
//Input buffering functions for windows
HANDLE hStdin = INVALID_HANDLE_VALUE;
DWORD fdwMode, fdwOldMode;

void disable_input_buffering(){
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &fdwOldMode);
    fdwMode = fdwOldMode ^ ENABLE_ECHO_INPUT ^ ENABLE_LINE_INPUT;
    SetConsoleMode(hStdin, fdwMode);
    FlushConsoleInputBuffer(hStdin);
}

void restore_input_buffering(){
    SetConsoleMode(hStdin, fdwOldMode);
}

uint16_t check_key(){
    return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0;
}
#endif

//Functions to interact with keyboard memory areas
void mem_write(uint16_t address, uint16_t val){
    memory[address] = val;
}

uint16_t mem_read(u_int16_t address){
    if(address = MR_KBSR){
        //Key press detected check which key was pressed
        if(check_key()){
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }else{
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}

void handle_interupt(int signal){
    restore_input_buffering();
    printf("\n");
    exit(-2);
}





int main(int argc, const char* argv[]){
    reg[R_COND] = FL_ZRO;

    enum{PC_START = 0x3000};
    reg[R_PC] = PC_START;
    int running = 1;

    //Terminal Configuration
    signal(SIGINT, handle_interupt);
    disable_input_buffering();


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
            case OP_ADD:{
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
                break;
            }

            case OP_AND:{
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
                break;
            }

            case OP_NOT:{
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t r1 = (instr >> 9) & 0x7;
                reg[r0] = ~reg[r1];
                update_flags(r0);
                break;
            }

            case OP_BR:{
                //Check if Negative,Zero,Positive Flags set
                uint16_t cond = (instr >> 9) & 0x7;
                if(cond & reg[R_COND]){
                    //Flag has been tested and matches register branch condition valid
                    //Move Program Counter to the branch offset
                    reg[R_PC] = reg[R_PC] + sign_extend(instr &0x1FF, 9);
                }
                break;
            }

            case OP_JMP:{
                //Move to location 3 bits 8:6
                uint16_t r1 = (instr >> 6) & 0x7;
                reg[R_PC] = reg[r1];
                break;
            }

            case OP_JSR:{
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
                break;
            }

            case OP_LD:{
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t offset = sign_extend((instr & 0x1FF), 9);
                //Get the memory contents at PC + offset and load into DR
                reg[r0] = mem_read(reg[R_PC] + offset);
                //Update Register flags
                update_flags(r0);
                break;
            }

            case OP_LDI:{
                //Load the address pointed to in memory to DR
                uint16_t r0 = (instr >> 9) &0x7;
                uint16_t offset = sign_extend((instr & 0x1FF), 9);
                //r0 is the memory address pointed to at the memory address specified by the PC + offset
                reg[r0] = mem_read(mem_read(reg[R_PC] + offset));
                update_flags(r0);
                break;
            }

            case OP_LDR:{
                uint16_t r0 = (instr >> 9) &0x7;
                uint16_t baseR = (instr >> 6) & 0x07;
                uint16_t offset = sign_extend(instr & 0x3f, 6);
                //Return Memory at base register  + offset
                reg[r0] = mem_read(reg[baseR] + offset);
                update_flags(r0);
                break;
            }

            case OP_LEA:{
                uint16_t r0 = (instr >> 9) &0x7;
                uint16_t offset = sign_extend((instr & 0x1FF), 9);
                reg[r0] = reg[R_PC] + offset;
                update_flags(r0); 
                break;
            }

            case OP_ST:{
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t offset = sign_extend((instr & 0x1FF), 9);
                mem_write(reg[R_PC] + offset, r0);
                break;
            }

            case OP_STI:{
                uint16_t r0 = (instr >> 9) & 0x7;
                uint16_t offset = sign_extend((instr & 0x1FF), 9);
                mem_write(mem_read(reg[R_PC]+ offset), r0);
                break;
            }

            case OP_STR:{
                uint16_t r0 = (instr >> 9) &0x7;
                uint16_t baseR = (instr >> 6) & 0x07;
                uint16_t offset = sign_extend(instr & 0x3f, 6);
                mem_write(reg[baseR] + offset, r0);
                break;
            }

            case OP_TRAP:{
                //Store current PC
                reg[R_R7] = reg[R_PC];
                //Based Run Trap Code
                switch(instr & 0xFF){
                    case T_GETC:{
                        //Get the next character from input and place in R0
                        reg[R_R0] = (uint16_t) getc(stdin);
                        //Set R_COND for new value
                        update_flags(R_R0);
                        break;
                    }

                    case T_OUT:{
                        //Output a single character to console @ R07:0
                        putc((char) reg[R_R0], stdout);
                        fflush(stdout);
                        break;
                    }

                    case T_PUTS:{
                        //Output NULL terminated string to console
                        //get character in R0
                        uint16_t* character = memory + reg[R_R0];
                        //While the address value is not 0x0000 output the character
                        while(*character){
                            putc((char)*character, stdout);
                            //Increment character to the next memory address
                            ++character;
                        }
                        //finished printing clear the buffer
                        fflush(stdout);
                        break;
                    }

                    case T_IN:{
                        //Prompt for a character input
                        printf("Please enter a character: ");
                        //Retrieve the character into R0
                        reg[R_R0] = (uint16_t) getc(stdin);
                        //Echo the character to output
                        putc((char) reg[R_R0], stdout);
                        //Flush buffer
                        fflush(stdout);
                        //Set the Condition flags
                        update_flags(R_R0);
                        break;
                    }

                    case T_PUTSP:{
                        //print out characters from memory each memory address has two characters print lower character first
                        uint16_t* character = memory + reg[R_R0];
                        while(*character){
                            putc(*character & 0xFF, stdout);
                            char c2 = *character >> 8;
                            //Check if the second character exists if not 0x000 print
                            if(c2){
                                putc(c2, stdout);
                            }
                            //flush buffer
                            fflush(stdout);
                        } 
                        break;
                    }

                    case T_HALT:{
                        printf("Halting!!");
                        fflush(stdout);
                        running = 0;
                        break;
                    }
                }
            }
            case OP_RES:{
                break;
            }

            case OP_RTI:{
                break;
            }

            default:{
                break;
            }
        

        }
    }
    //Restore terminal config during exit
    restore_input_buffering();

}