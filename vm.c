#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

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

u16 get_op(u16 word) {
  return (word & 0xf);
}

u16 get_a(u16 word) {
  return ((word >> 4) & 0x3f);
}

u16 get_b(u16 word) {
  return ((word >> 10) & 0x3f);
}

int sigint_received = 0;
void stop_cpu(int signum) {
  sigint_received = 1;
}

void dump(u16* PC, u16* current, u16* SP, u16* registers, u16* ram) {
  int i;
  u16 stack = *SP;
  char reg_list[] = "ABCXYZIJ";

  printf("Current word: %#.4x\n", *current);

  printf("Registers:\n");
  for (i = 0; i < 8; i++) {
    printf("\t%c: %#.4x\n", reg_list[i], registers[i]);
  }
  printf("\nStack:\n");
  while (stack != 0) {
    printf("\t%#.4x: %#.4x\n", stack, ram[stack++]);
  }
}

void printUsage(char *fileName) {
  printf("Usage: %s <filename>\n", fileName);
  printf("Where <filename> is compiled DCPU-16 instructions.\n");
}

int main(int argc, char *argv[]) {
  u16 command;
  u16 cur;
  u16 ram[0x10000];
  char reg_names[] = "ABCXYZIJ";
  char *isDebug = getenv("DEBUG");
  int maxCycles = 0;

  if (argc < 2) {
    printUsage(argv[0]);
    return 0;
  }

  if (strcmp(argv[1], "-h") == 0 ||
      strcmp(argv[1], "--help") == 0) {
    printUsage(argv[0]);
    return 0;
  }
  
  char *fileName = argv[argc-1];
  if (strcmp(argv[1], "-n") == 0) {
    if (argc < 4) {
      printUsage(argv[0]);
      return 0;
    } else {
      maxCycles = atoi(argv[2]);
    }
  }


  /* Registers:
   *  0x00 - 0x07 -> A,B.C,X,Y,Z,I,J
   *  Not named, but easier to address!
   */
  u16 registers[8];
  u16 PC = 0, SP = 0, O = 0;
  u16 *target;
  u16 value;
  unsigned int a, b, i;
  unsigned int cycles = 0;
  FILE *fp;
  fp = fopen(fileName, "rb");
  if (fp == NULL) {
    printf("Error opening file: %s\n", fileName);
    return 1;
  }
  else {
    while (!feof(fp)) {
      fread(&ram[PC++], 2, 1, fp);
    }
  }
  fclose(fp);
  a = PC - 1;
  PC = 0;
  
  signal(SIGINT, stop_cpu);
  if (!maxCycles)
    printf("Beginning emulation.  Press Ctrl+C to exit\n");
  while (1) {
    cycles++;
    cur = ram[PC++];
    command = get_op(cur);
    a = get_a(cur);
    b = get_b(cur);
    if (isDebug != NULL) {
      printf("Cycle: %d ", cycles);
      printf("Current: %#.4x ", cur);
      printf("Command: %#.2x\n", command);
    }

    if (command == 0) {
      if (a == 0x01) {
        cycles++;
        ram[ --SP ] = PC;
        if (b < 0x07) {
          value = registers[b];
        } else if (b > 0x07 && b < 0x10) {
          value = ram[ registers[ b - 0x08 ] ];
        } else if (b > 0x0f && b < 0x18) {
          value = ram[ ram[ PC++ ] + registers[ b - 0x10 ] ];
        } else if (b == 0x18) {
          value = ram[ SP++ ];
        } else if (b == 0x19) {
          value = ram[ SP ];
        } else if (b == 0x1a) {
          value = ram[ --SP ];
        } else if (b == 0x1b) {
          value = SP;
        } else if (b == 0x1c) {
          value = PC;
        } else if (b == 0x1d) {
          value = O;
        } else if (b == 0x1e) {
          value = ram[ ram[ PC++ ] ];
          cycles++;
        } else if (b == 0x1f) {
          value = ram[ PC++ ];
          cycles++;
        } else if (b > 0x1f && b < 0x40) {
          value = (b - 0x20);
        }
        PC = value;
      }
      continue;
    }

    if (a < 0x08) {
      target = &registers[a];
    } else if (a > 0x07 && a < 0x10) {
      target = &ram[ registers[ a - 0x08 ] ];
    } else if (a > 0x0f && a < 0x18) {
      target = &ram[ ram[PC++] + registers[a - 0x10] ];
      cycles++;
    } else if (a == 0x18) {
      target = &ram[ SP++ ];
    } else if (a == 0x19) {
      target = &ram[SP];
    } else if (a == 0x1a) {
      target = &ram[--SP];
    } else if (a == 0x1b) {
      target = &SP;
    } else if (a == 0x1c) {
      target = &PC;
    } else if (a == 0x1d) {
      target = &O;
    } else if (a == 0x1e) {
      target = &ram[ram[PC++]];
      cycles++;
    } else {
      continue;
    }

    if (b < 0x07) {
      value = registers[b];
    } else if (b > 0x07 && b < 0x10) {
      value = ram[ registers[ b - 0x08 ] ];
    } else if (b > 0x0f && b < 0x18) {
      value = ram[ ram[ PC++ ] + registers[ b - 0x10 ] ];
    } else if (b == 0x18) {
      value = ram[ SP++ ];
    } else if (b == 0x19) {
      value = ram[ SP ];
    } else if (b == 0x1a) {
      value = ram[ --SP ];
    } else if (b == 0x1b) {
      value = SP;
    } else if (b == 0x1c) {
      value = PC;
    } else if (b == 0x1d) {
      value = O;
    } else if (b == 0x1e) {
      value = ram[ ram[ PC++ ] ];
      cycles++;
    } else if (b == 0x1f) {
      value = ram[ PC++ ];
      cycles++;
    } else if (b > 0x1f && b < 0x40) {
      value = (b - 0x20);
    }

    switch(get_op(cur)) {
      case SET:
        *target = value;
        break;
      case ADD:
        cycles++;
        a = *target + value;
        if (a > 0xffff) {
          *target = (a & 0xffff);
          O = 0x0001;
        } else {
          *target = (u16) a;
        }
        break;
      case SUB:
        cycles++;
        a = *target - value;
        if (a > 0xffff) {
          *target = (a & 0xffff);
          O = 0xffff;
        } else {
          *target = (u16) a;
        }
        break;
      case MUL:
        cycles++;
        O = ((*target * value) >> 0x10) & 0xffff;
        *target *= value;
        break;
      case DIV:
        cycles += 2; // Div takes 3 cycles.
        if (value == 0) {
          *target = 0;
        } else {
          O = ((*target << 16)/value) & 0xffff;
          *target /= value;
        }
        break;
      case MOD:
        cycles += 2;
        if (value == 0) {
          *target = 0;
        } else {
          *target %= value;
        }
        break;
      case SHL:
        cycles++;
        O = ((*target << value) >> 0x10) & 0xffff;
        *target = *target << value;
        break;
      case SHR:
        cycles++;
        O = ((*target >> 16) >> value) & 0xffff;
        *target = *target >> value;
        break;
      case AND:
        *target = *target & value;
        break;
      case BOR:
        *target = *target | value;
        break;
      case XOR:
        *target = *target ^ value;
        break;
      case IFE:
        cycles++;
        // If a value == b value, proceed.  Otherwise skip next instruction.
        if (*target == value) {
          break;
        } else {
          if ((get_a(ram[PC]) > 0x07) && (get_a(ram[PC]) < 0x18) ||
              (get_a(ram[PC]) > 0x1d) && (get_a(ram[PC]) < 0x20)) 
            PC++;
          if ((get_b(ram[PC]) > 0x07) && (get_b(ram[PC]) < 0x18) ||
              (get_b(ram[PC]) > 0x1d) && (get_b(ram[PC]) < 0x20)) 
            PC++;
          PC++;
        }
        break;
      case IFN:
        cycles++;
        // Inverse of IFE
        if (*target != value) {
          break;
        } else {
          if ((get_a(ram[PC]) > 0x07) && (get_a(ram[PC]) < 0x18) ||
              (get_a(ram[PC]) > 0x1d) && (get_a(ram[PC]) < 0x20)) 
            PC++;
          if ((get_b(ram[PC]) > 0x07) && (get_b(ram[PC]) < 0x18) ||
              (get_b(ram[PC]) > 0x1d) && (get_b(ram[PC]) < 0x20)) 
            PC++;
          PC++;
        }
        break;
      case IFG:
        cycles++;
        if (*target > value) {
          break;
        } else {
          if ((get_a(ram[PC]) > 0x07) && (get_a(ram[PC]) < 0x18) ||
              (get_a(ram[PC]) > 0x1d) && (get_a(ram[PC]) < 0x20)) 
            PC++;
          if ((get_b(ram[PC]) > 0x07) && (get_b(ram[PC]) < 0x18) ||
              (get_b(ram[PC]) > 0x1d) && (get_b(ram[PC]) < 0x20)) 
            PC++;
          PC++;
        }
        break;
      case IFB:
        cycles++;
        if ((*target & value) != 0) {
          break;
        } else {
          if ((get_a(ram[PC]) > 0x07) && (get_a(ram[PC]) < 0x18) ||
              (get_a(ram[PC]) > 0x1d) && (get_a(ram[PC]) < 0x20)) 
            PC++;
          if ((get_b(ram[PC]) > 0x07) && (get_b(ram[PC]) < 0x18) ||
              (get_b(ram[PC]) > 0x1d) && (get_b(ram[PC]) < 0x20)) 
            PC++;
          PC++;
        }
        break;
      default:
        printf("Crapped out\n");
        return 1;
    }
    if (maxCycles && cycles >= maxCycles) {
      break;
    }
    if (sigint_received == 1) {
      break;
    }
  }

  printf("Cycles: %d\n", cycles);
  printf("Registers:\n");
  for (i = 0; i < 8; i++)
    printf("\t%c:\t%#.4x\n", reg_names[i], registers[i]);
  printf("Good-bye!\n");

  return 0;
}
