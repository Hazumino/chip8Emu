#include <chrono>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <random>

const unsigned int START_ADDRESS = 0x200;

const unsigned int FONTSET_START_ADDRESS = 0x50;

const unsigned int FONTSET_SIZE = 80;

class Chip8 {
public:
  uint8_t registers[16]{};
  uint8_t memory[4096]{};
  uint16_t index{};
  uint16_t pc{};
  uint16_t stack[16];
  uint8_t sp{};
  uint8_t delayTimer{};
  uint8_t soundTimer{};
  uint8_t keypad[16]{};
  uint32_t video[64 * 32]{};
  uint16_t opcode;

  uint8_t fontset[FONTSET_SIZE] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };

  Chip8() {
    pc = START_ADDRESS;

    for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
      memory[FONTSET_START_ADDRESS + i] = fontset[i];
    }
  }

  // Clear Screen OPCODE
  void OP_00E0() { wmemset(video, 0, sizeof(video)); }

  // Return OPCODE
  void OP_00EE() {
    --sp;
    pc = stack[sp];
  }

  // Jump Address OPCODE
  void OP_1NNN() {
    // And bitwise operation to mask the lower 12 bits to extract the address
    uint16_t address = opcode & 0x0FFFu;
    pc = address;
  }

  // Call Address OPCODE
  void OP_2NNN() {
    // And bitwise operation to mask the lower 12 bits to extract the address
    stack[sp] = pc;
    sp++;
    uint16_t address = opcode & 0x0FFFu;
    pc = address;
  }

  // If Equal OPCODE
  void OP_3XKK() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] == byte) {
      pc += 2;
    }
  }

  // S Not Equal OPCODE
  void OP_4XKK() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;

    if (registers[Vx] != byte) {
      pc += 2;
    }
  }

  void OP_5XY0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = opcode & 0x000Fu >> 4u;

    if (registers[Vx] == registers[Vy]) {
      pc += 2;
    }
  }

  // Set Vx to value kk
  void OP_6XKK() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = opcode & 0x00FFu;

    registers[Vx] = value;
  }

  // Add values of Vx into the values of KK
  void OP_7XKK() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t value = opcode & 0x00FFu;

    registers[Vx] = registers[Vx] + value;
  }

  // Vx equal to Vy
  void OP_8XY0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = opcode & 0x000Fu >> 4u;
    registers[Vx] = registers[Vy];
  }

  // OR between Vx and Vy
  void OP_8XY1() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = opcode & 0x000Fu >> 4u;
    registers[Vx] |= registers[Vy];
  }

  // AND between Vx and Vy
  void OP_8XY2() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = opcode & 0x000Fu >> 4u;
    registers[Vx] &= registers[Vy];
  }

  // XOR between vx and Vy
  void OP_8XY3() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = opcode & 0x000Fu >> 4u;
    registers[Vx] ^= registers[Vy];
  }

  // ADD with sum
  void OP_8XY4() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = opcode & 0x000Fu >> 4u;
    uint8_t total = registers[Vx] + registers[Vy];
    if (total > 255u) {
      registers[0xF] = 1;

    } else {
      registers[0xF] = 0;
    }

    registers[Vx] = total & 0xFFu;
  }

  // SUB VF
  void OP_8XY5() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = opcode & 0x000Fu >> 4u;
    uint8_t total = registers[Vx] - registers[Vy];
    if (registers[Vx] > registers[Vy]) {
      registers[0xF] = 1;

    } else {
      registers[0xF] = 0;
    }

    registers[Vx] = total;
  }

  // Shift Register Right
  void OP_8XY6() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[0xF] = (registers[Vx] & 0x1u);

    // Right shift by 1
    registers[Vx] >>= 1;
  }

  // SUBN Vx, Vy
  void OP_8XY7() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = opcode & 0x000Fu >> 4u;
    uint8_t total = registers[Vx] - registers[Vy];

    if (registers[Vy] > registers[Vx]) {
      registers[0xF] = 1;

    } else {
      registers[0xF] = 0;
    }

    registers[Vx] = total;
  }

  // Shift Register Left
  void OP_8XY8() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

    registers[0xF] = (registers[Vx] & 0x80u) >> 7u;

    // Left shift by 1
    registers[Vx] <<= 1;
  }

  // Skip next instruction if Vx != Vy
  void OP_9XY0() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = opcode & 0x000Fu >> 4u;

    if (registers[Vx] != registers[Vy]) {
      pc += 2;
    }
  }

  void OP_ANNN() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = opcode & 0x000Fu >> 4u;

    if (registers[Vx] != registers[Vy]) {
      pc += 2;
    }
  }

  // Jump to location nnn + V0
  void OP_BNNN() {
    uint8_t address = opcode & 0x0FFFu;
    pc = registers[0] + address;
  }

  // Generate random byte and add to register Vx a Bitwise AND between kk and
  // random byte (0 to 255)
  void OP_CXKK() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t byte = opcode & 0x00FFu;
    registers[Vx] = randByte(randGen) & byte;
  }

  void OP_DXYN() {
    uint8_t Vx = (opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (opcode & 0x00F0u) >> 4u;
  }

  void LoadRom(char const *filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (file.is_open()) {
      // Get size of file and allocate a buffer to hold the contents
      std::streampos size = file.tellg();
      char *buffer = new char[size];

      // Go back to the beginning of the file and fill the buffer
      file.seekg(0, std::ios::beg);
      file.read(buffer, size);
      file.close();

      // Load the ROM contents into the Chip8's memory starting at 0x200
      for (long i = 0; i < size; i++) {
        memory[START_ADDRESS + i] = buffer[i];
      }
    }
  }
};
