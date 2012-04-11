#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/* A and B equate to how far to shift
 * to get that section of the current word.*/
#define MAX_CYCLES 1000
#define A 4
#define B 10

#define SET 0x1
#define ADD 0x2
#define SUB 0x3
#define MUL 0x4
#define DIV 0x5
#define MOD 0x6
#define SHL 0x7
#define SHR 0x8
#define AND 0x9
#define BOR 0xa
#define XOR 0xb
#define IFE 0xc
#define IFN 0xd
#define IFG 0xe
#define IFB 0xf

typedef unsigned short u16;

u16 registers[8];
u16 PC = 0, SP = 0, O = 0;
u16 ram[0x10000];
int iCycles;

u16 get_op(u16 word) {
  return (word & 0xf);
}

u16 *get_dest(u16 word, u16 shift) {
  u16 a = (word >> shift) & 0x3f;
  if (a <= 0x07)
    return &registers[a];
  else if (a >= 0x08 && a <= 0x0f) {
    iCycles++;
    return &ram[ registers[ a - 0x08 ] ];
  } else if (a >= 0x10 && a <= 0x17) {
    iCycles++;
    return &ram[ ram[PC++] + registers[ a - 0x10 ] ];
  }
  switch(a) {
    case 0x18:
      return &ram[ SP++ ];
    case 0x19:
      return &ram[ SP ];
    case 0x1a:
      return &ram[ --SP ];
    case 0x1b:
      return &SP;
    case 0x1c:
      return &PC;
    case 0x1d:
      return &O;
    case 0x1e:
      iCycles++;
      return &ram[ ram[PC++] ];
    default:
      return NULL;
  }
}

u16 get_val(u16 word, u16 shift) {
  u16 b = (word >> shift) & 0x3f;
  if (b <= 0x07)
    return registers[b];
  else if (b >= 0x08 && b <= 0x0f) {
    iCycles++;
    return ram[ registers[b - 0x08] ];
  } else if (b >= 0x10 && b <= 0x17) {
    iCycles++;
    return ram[ ram[PC++] + registers[b - 0x10] ];
  } else if (b >= 0x20 && b <= 0x3f)
    return b - 0x20;

  switch(b) {
    case 0x18:
      return ram[ SP++ ];
    case 0x19:
      return ram[ SP ];
    case 0x1a:
      return ram[ --SP ];
    case 0x1b:
      return SP;
    case 0x1c:
      return PC;
    case 0x1d:
      return O;
    case 0x1e:
      iCycles++;
      return ram[ ram[PC++] ];
    case 0x1f:
      iCycles++;
      return ram[PC++];
    default:
      break;
  }
}

u16 steps_forward() {
  u16 steps = 1;
  u16 a = (ram[PC] >> A) & 0x3f;
  u16 b = (ram[PC] >> B) & 0x3f;

  if ((a >= 0x08 && a <= 0x17) ||
      (a >= 0x1e && a <= 0x1f))
    steps++;
  if ((b >= 0x08 && b <= 0x17) ||
      (b >= 0x1e && b <= 0x1f))
    steps++;
  return steps;
}

int sigint_received = 0;
void stop_cpu(int signum) {
  sigint_received = 1;
}

void printUsage(char *fileName) {
  printf("Usage: %s <filename>\n", fileName);
  printf("Where <filename> is compiled DCPU-16 instructions.\n");
}

int main(int argc, char *argv[]) {
  char reg_names[] = "ABCXYZIJ";
  char *fileName = argv[1];

  u16 *target;
  u16 value;
  u16 current;

  unsigned int a, b, i;
  unsigned int cycles = 0;

  if (argc < 2) {
    printUsage(argv[0]);
    return 0;
  }

  if (strcmp(argv[1], "-h") == 0 ||
      strcmp(argv[1], "--help") == 0) {
    printUsage(argv[0]);
    return 0;
  }
  
  FILE *fp;
  fp = fopen(fileName, "rb");
  if (fp == NULL) {
    printf("Error opening file: %s\n", fileName);
    return 1;
  }
  else {
    /* Program loader. */
    while (!feof(fp)) {
      fread(&ram[PC++], 2, 1, fp);
    }
  }
  fclose(fp);
  PC = 0;
  
  signal(SIGINT, stop_cpu);
  
  while(1) {
    cycles++;
    if (--iCycles > 0)
      continue;

    current = ram[PC++];

    // Simulate the consumption of cycles.
    switch(get_op(current)) {

      case 0x0:
        // TODO: Don't depend on get_val here.
        // If Notch moves above 0xf non-basic opcodes
        // this will add an extra cycle
        switch(get_val(current, A)) {
          case 0x01:
            iCycles = 2;
            ram[--SP] = PC;
            PC = *get_dest(current, B);
            break;
          default:
            break;
        }
        break;
        // End special opcodes

      case SET:
        iCycles = 1;
        *get_dest(current, A) = get_val(current, B);
        break;

      case ADD:
        iCycles = 2;
        target = get_dest(current, A);
        value = *target + get_val(current, B);
        if (value > 0xffff)
          O = 1;
        else
          O = 0;
        *target = value & 0xffff;
        break;

      case SUB:
        iCycles = 2;
        target = get_dest(current, A);
        value = *target - get_val(current, B);
        if (value > 0xffff)
          O = 0xffff;
        else
          O = 0;
        *target = value & 0xffff;
        break;

      case MUL:
        iCycles = 2;
        target = get_dest(current, A);
        value = get_val(current, B);
        *target *= value;
        O = ((*target * value)>>16) & 0xffff;
        break;

      case DIV:
        iCycles = 3;
        target  = get_dest(current, A);
        value   = get_val(current, B);

        if (value == 0) {
          *target = 0;
          O = 0;
          break;
        }
        *target /= value;
        O = ((*target<<16)/value) & 0xffff;
        break;

      case MOD:
        iCycles = 3;
        target  = get_dest(current, A);
        value   = get_val (current, B);

        if (value == 0) {
          *target = 0;
          O = 0;
          break;
        }
        
        *target %= value;
        break;

      case SHL:
        iCycles = 2;
        target  = get_dest(current, A);
        value   = get_val (current, B);

        *target = *target << value;

        O = ((*target << value) >> 16) & 0xffff;
        break;

      case SHR:
        iCycles = 2;
        target  = get_dest(current, A);
        value   = get_val (current, B);

        *target = *target >> value;

        O = ((*target << 16) >> value) & 0xffff;
        break;

      case AND:
        iCycles = 1;
        *get_dest(current, A) &= get_val(current, B);
        break;

      case BOR:
        iCycles = 1;
        *get_dest(current, A) |= get_val(current, B);
        break;

      case XOR:
        iCycles = 1;
        *get_dest(current, A) ^= get_val(current, B);
        break;

      case IFE:
        iCycles = 2;

        if (get_val(current, A) != get_val(current, B))
          PC += steps_forward();
        else
          iCycles++;
        break;

      case IFN:
        iCycles = 2;

        if (get_val(current, A) == get_val(current, B))
          PC += steps_forward();
        else
          iCycles++;
        break;

      case IFG:
        iCycles = 2;

        if (get_val(current, A) < get_val(current, B))
          PC += steps_forward();
        else
          iCycles++;
        break;

      case IFB:
        iCycles = 2;

        if ((get_val(current, A) & get_val(current, B)) == 0)
          PC += steps_forward();
        else
          iCycles++;
        break;
    } // End of opcodes.
    if (sigint_received)
      break;
    if (cycles > MAX_CYCLES)
      break;
  }

  printf("Cycles: %d\n", cycles);
  printf("Registers:\n");
  for (i = 0; i < 8; i++) {
    printf("\t%c: %#.4x\n", reg_names[i], registers[i]);
  }

  return 0;
}
