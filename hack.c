#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define RAM_SIZE 24577
#define ROM_SIZE 32768 // 2**15(32K)

typedef int16_t s16;
typedef uint16_t u16;
typedef uint8_t u8;

struct cpu_input
{
    u16 inM;
    u16 instruction;
    bool reset;
};

struct cpu_output
{
    u16 outM;
    bool writeM; 
    u16 addressM;
    u16 pc;
};

void cpu(struct cpu_input *input, struct cpu_output *output);

/* Takes a string containing a binary number in ascii and big endian notation and converts it into a u16 */
u16 convert_binary_string_to_u16(const char *string);

/* Reads a hack assembly program and writes it to the buffer */
void load_program_from_file(const char* filename, u16 *buffer, int buffersize);

/* Prints a buffer to the screen where each pixel is represented by one bit */
void display_buffer(u16 *buffer, int height, int width);


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: ./hack <hack-file>\n");
        return -1;
    }
    u16 data_memory[RAM_SIZE];
    u16 instruction_memory[ROM_SIZE];
    struct cpu_input cpu_in;
    struct cpu_output cpu_out;
    // Initialize everything to zero
    memset(data_memory, 0, RAM_SIZE * 2);
    memset(instruction_memory, 0, ROM_SIZE * 2);
    memset(&cpu_in, 0, sizeof(struct cpu_input));    
    memset(&cpu_out, 0, sizeof(struct cpu_output));
    load_program_from_file(argv[1], instruction_memory, ROM_SIZE);

#if DEBUG 
    // TODO: Test the CPU with assert statements
    assert(convert_binary_string_to_u16("0000000000000001") == 1);
    assert(convert_binary_string_to_u16("1000000000000000") == 32768);
    assert(convert_binary_string_to_u16("1000000000000001") == 32769);
#endif

    for(;;)
    {
        if (cpu_out.addressM >= RAM_SIZE || cpu_out.pc >= ROM_SIZE ||
            cpu_out.addressM < 0 || cpu_out.pc < 0)
        {
            fprintf(stderr, "RAM size: %d | Tried to access location %d\nROM size: %d | Tried to access location %d\n", RAM_SIZE, cpu_out.addressM, ROM_SIZE, cpu_out.pc);
            break;
        }
        // If the write bit is set write to the memory 
        if (cpu_out.writeM)
            data_memory[cpu_out.addressM] = cpu_out.outM;
        // Fetch the data and the instruction requested by the cpu
        cpu_in.instruction = instruction_memory[cpu_out.pc];
        cpu_in.inM = data_memory[cpu_out.addressM];

        // Execute
        cpu(&cpu_in, &cpu_out);

        display_buffer(&data_memory[24575], 256, 512);
        printf("\n\n\n");
        
    }
    return EXIT_SUCCESS;
}


void cpu(struct cpu_input *input, struct cpu_output *output)
{
    // Declare the registers
    static u16 A, PC;
    static s16 D;
    // There a two types of instructions which can be identified by the highest bit
    // If the highest bit is set it's a C-instruction else a A-instruction
    // The A-instruction is used to set the A register
    // The C-instruction is used for computation
    bool c_instruction = ((input->instruction >> 15) & 1) == 1;
    if (c_instruction) // Handle C-instruction
    {
        u16 computed_result;
        // Decode the instruction
        bool comp[] = {
	    ((input->instruction >> 12) & 1) == 1,
            ((input->instruction >> 11) & 1) == 1,
            ((input->instruction >> 10) & 1) == 1,
            ((input->instruction >> 9) & 1) == 1,
            ((input->instruction >> 8) & 1) == 1,
            ((input->instruction >> 7) & 1) == 1,
            ((input->instruction >> 6) & 1) == 1
        };
        bool dest[] = {
            ((input->instruction >> 5) & 1) == 1,
            ((input->instruction >> 4) & 1) == 1,
            ((input->instruction >> 3) & 1) == 1
        };
        bool jump[] = {
            ((input->instruction >> 2) & 1) == 1,
            ((input->instruction >> 1) & 1) == 1,
            (input->instruction & 1) == 1
        };
        
        // Execute the computation specified by the comp flags.
        // The first flags specifies which operants to use. The D register is always the first operant.
        // If the first comp flag is set M which stands for the fetched memory is the second operant
        // if it isn't set A is the other operand
        u16 second_operand = comp[0] ? input->inM : A;
        if (comp[0] && !comp[1] && comp[2] && !comp[3] && comp[4] && !comp[5]) 
            computed_result = 0;
        else if (comp[0] && comp[1] && comp[2] && comp[3] && comp[4] && comp[5]) 
            computed_result = 1;
        else if (comp[0] && comp[1] && comp[2] && !comp[3] && comp[4] && !comp[5]) 
            computed_result = -1;
        else if (!comp[0] && !comp[1] && comp[2] && comp[3] && !comp[4] && !comp[5]) 
            computed_result = D;
        else if (comp[0] && comp[1] && !comp[2] && !comp[3] && !comp[4] && !comp[5]) 
            computed_result = second_operand;
        else if (!comp[0] && !comp[1] && comp[2] && comp[3] && !comp[4] && comp[5]) 
            computed_result = !D;
        else if (comp[0] && comp[1] && !comp[2] && !comp[3] && !comp[4] && comp[5]) 
            computed_result = !second_operand;
        else if (!comp[0] && !comp[1] && comp[2] && comp[3] && comp[4] && comp[5]) 
            computed_result = -D;
        else if (comp[0] && comp[1] && !comp[2] && !comp[3] && comp[4] && comp[5]) 
            computed_result = -second_operand;
        else if (!comp[0] && comp[1] && comp[2] && comp[3] && comp[4] && comp[5]) 
            computed_result = D+1;
        else if (comp[0] && comp[1] && !comp[2] && comp[3] && comp[4] && comp[5]) 
            computed_result = second_operand+1;
        else if (!comp[0] && !comp[1] && comp[2] && comp[3] && comp[4] && !comp[5]) 
            computed_result = D-1;
        else if (comp[0] && comp[1] && !comp[2] && !comp[3] && comp[4] && !comp[5]) 
            computed_result = second_operand-1;
        else if (!comp[0] && !comp[1] && !comp[2] && !comp[3] && comp[4] && !comp[5]) 
            computed_result = D+second_operand;
        else if (!comp[0] && comp[1] && !comp[2] && !comp[3] && comp[4] && comp[5]) 
            computed_result = D-second_operand;
        else if (!comp[0] && !comp[1] && !comp[2] && comp[3] && comp[4] && comp[5]) 
            computed_result = second_operand-D;
        else if (!comp[0] && !comp[1] && !comp[2] && !comp[3] && !comp[4] && !comp[5]) 
            computed_result = D&second_operand;
        else if (!comp[0] && comp[1] && !comp[2] && comp[3] && !comp[4] && comp[5]) 
            computed_result = D|second_operand;

        // Store the computed result in different registers and/or the memory
        // based on the destination flags.
        if (dest[0])
            A = computed_result;
        if (dest[1])
            D = computed_result;
        if (dest[2])
            output->writeM = true; 
        output->outM = computed_result;

        // Set the pc to the value of the A-register if the condition specified by
        // the jump flags is satisfied
	if (
            (!jump[0] && !jump[1] && jump[2] && (computed_result > 0)) ||
            (!jump[0] && jump[1] && !jump[2] && (computed_result = 0)) ||
            (!jump[0] && jump[1] && jump[2] && (computed_result >= 0)) ||
            (jump[0] && !jump[1] && !jump[2] && (computed_result < 0)) ||
            (jump[0] && !jump[1] && jump[2] && (computed_result != 0)) ||
            (jump[0] && jump[1] && !jump[2] && (computed_result <= 0)) ||
            (jump[0] && jump[1] && jump[2])
            )
            PC = A - 1; // A - 1 because the pc gets incremented at the end of the program
    } 
    else // Handle A-instruction
    { 
        A = input->instruction;
        output->writeM = false;
    }
    if (input->reset)
        output->pc = PC = 0;
    output->addressM = A;
    output->pc = ++PC;
}


void load_program_from_file(const char* filename, u16 *buffer, int buffersize)
{
    FILE* program = fopen(filename, "r");
    if (!program)
        return;

    char line_buffer[16];
    for (int i = 0; fread(line_buffer, 16, 1, program) && i < buffersize; i++)
    {
        // Skipt the two bytes indicating a new line
        fseek(program, 2, SEEK_CUR);
        buffer[i] = convert_binary_string_to_u16(line_buffer); 
    }
    fclose(program);
}


u16 convert_binary_string_to_u16(const char *string)
{
    u16 result = 0;
    for (int i = 0; i < 16; i++)
    {
        result = string[i] - 48 ? result | 32768 >> i: result; 
    }
    return result;
}


void display_buffer(u16 *buffer, int height, int width)
{
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            bool white = buffer[(width/16)*i + j/8] & (1 >> (j % 8));
            if (white)
                printf(" ");
            else
                printf("#");
        }
        printf("\n");
    }
}