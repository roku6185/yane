#ifndef _OPCODES_H_
#define _OPCODES_H_

// LDA/LDX/LDY
#define LDA_IMM    0xA9  // 2
#define LDA_ZERO  0xA5  // 3
#define LDA_ZERO_X  0xB5  // 4
#define LDA_ABS    0xAD  // 4
#define LDA_ABS_X  0xBD  // 4*
#define LDA_ABS_Y  0xB9  // 4*
#define LDA_IND_X  0xA1  // 6
#define LDA_IND_Y  0xB1  // 5*

#define LDX_IMM    0xA2  // 2
#define LDX_ZERO  0xA6  // 3
#define LDX_ZERO_Y  0xB6  // 4
#define LDX_ABS    0xAE  // 4
#define LDX_ABS_Y  0xBE  // 4*

#define LDY_IMM    0xA0  // 2
#define LDY_ZERO  0xA4  // 3
#define LDY_ZERO_X  0xB4  // 4
#define LDY_ABS    0xAC  // 4
#define LDY_ABS_X  0xBC  // 4*

// STA/STX/STY
#define STA_ZERO  0x85  // 3
#define STA_ZERO_X  0x95  // 4
#define STA_ABS    0x8D  // 4
#define STA_ABS_X  0x9D  // 5
#define STA_ABS_Y  0x99  // 5
#define STA_IND_X  0x81  // 6
#define STA_IND_Y  0x91  // 6

#define STX_ZERO  0x86  // 3
#define STX_ZERO_Y  0x96  // 4
#define STX_ABS    0x8E  // 4

#define STY_ZERO  0x84  // 3
#define STY_ZERO_X  0x94  // 4
#define STY_ABS    0x8C  // 4

// Logical operations
#define AND_IMM    0x29  // 2
#define AND_ZERO  0x25  // 3
#define AND_ZERO_X  0x35  // 4
#define AND_ABS    0x2D  // 4
#define AND_ABS_X  0x3D  // 4*
#define AND_ABS_Y  0x39  // 4*
#define AND_IND_X  0x21  // 6
#define AND_IND_Y  0x31  // 5*

#define OR_IMM    0x09  // 2
#define OR_ZERO    0x05  // 3
#define OR_ZERO_X  0x15  // 4
#define OR_ABS    0x0D  // 4
#define OR_ABS_X  0x1D  // 4*
#define OR_ABS_Y  0x19  // 4*
#define OR_IND_X  0x01  // 6
#define OR_IND_Y  0x11  // 5*

#define XOR_IMM    0x49  // 2
#define XOR_ZERO  0x45  // 3
#define XOR_ZERO_X  0x55  // 4
#define XOR_ABS    0x4D  // 4
#define XOR_ABS_X  0x5D  // 4*
#define XOR_ABS_Y  0x59  // 4*
#define XOR_IND_X  0x41  // 6
#define XOR_IND_Y  0x51  // 5*

// Branching
#define BPL    0x10  // 2*
#define BNE    0xD0  // 2*
#define BEQ    0xF0  // 2*
#define BCC    0x90  // 2*
#define BCS    0xB0  // 2*
#define BMI    0x30  // 2*
#define BVC    0x50  // 2*
#define BVS    0x70  // 2*

// Clear flags
#define CLC    0x18  // 2
#define CLD    0xD8  // 2
#define CLV    0xB8  // 2
#define CLI    0x58  // 2

// Set flags
#define SEI    0x78  // 2
#define SED    0xF8  // 2
#define SEC    0x38  // 2

// Decrementing
#define DEC_ZERO  0xC6  // 5
#define DEC_ZERO_X  0xD6  // 6
#define DEC_ABS    0xCE  // 6
#define DEC_ABS_X  0xDE  // 7
#define DEX    0xCA  // 2
#define DEY    0x88  // 2

// Incrementing
#define INC_ZERO  0xE6  // 5
#define INC_ZERO_X  0xF6  // 6
#define INC_ABS    0xEE  // 6
#define INC_ABS_X  0xFE  // 7
#define INX    0xE8  // 2
#define INY    0xC8  // 2

// Transfer
#define TAX    0xAA  // 2
#define TAY    0xA8  // 2
#define TSX    0xBA  // 2
#define TXA    0x8A  // 2
#define TXS    0x9A  // 2
#define TYA    0x98  // 2

// Jump
#define JSR    0x20  // 6
#define JMP_ABS    0x4C  // 3
#define JMP_IND    0x6C  // 5

// Stack
#define PHP    0x08  // 3
#define PLP    0x28  // 4
#define PHA    0x48  // 3
#define PLA    0x68  // 4

// Compare
#define CMP_IMM    0xC9  // 2
#define CMP_ZERO  0xC5  // 3
#define CMP_ZERO_X  0xD5  // 4
#define CMP_ABS    0xCD  // 4
#define CMP_ABS_X  0xDD  // 4*
#define CMP_ABS_Y  0xD9  // 4*
#define CMP_IND_X  0xC1  // 6
#define CMP_IND_Y  0xD1  // 5*

#define ADC_IMM    0x69  // 2
#define ADC_ZERO  0x65  // 3
#define ADC_ZERO_X  0x75  // 4
#define ADC_ABS    0x6D  // 4
#define ADC_ABS_X  0x7D  // 4*
#define ADC_ABS_Y  0x79  // 4*
#define ADC_IND_X  0x61  // 6
#define ADC_IND_Y  0x71  // 5*

#define SBC_IMM    0xE9  // 2
#define SBC_IMM2  0xEB  // 2
#define SBC_ZERO  0xE5  // 3
#define SBC_ZERO_X  0xF5  // 4
#define SBC_ABS    0xED  // 4
#define SBC_ABS_X  0xFD  // 4*
#define SBC_ABS_Y  0xF9  // 4*
#define SBC_IND_X  0xE1  // 6
#define SBC_IND_Y  0xF1  // 5*


#define CPX_IMM    0xE0  // 2
#define CPX_ZERO  0xE4  // 3
#define CPX_ABS    0xEC  // 4

#define CPY_IMM    0xC0  // 2
#define CPY_ZERO  0xC4  // 3
#define CPY_ABS    0xCC  // 4

#define ASL_ACC    0x0A  // 2
#define ASL_ZERO  0x06  // 5
#define ASL_ZERO_X  0x16  // 6
#define ASL_ABS    0x0E  // 6
#define ASL_ABS_X  0x1E  // 7

#define LSR_ACC    0x4A  // 2
#define LSR_ZERO  0x46  // 5
#define LSR_ZERO_X  0x56  // 6
#define LSR_ABS    0x4E  // 6
#define LSR_ABS_X  0x5E  // 7

#define ROL_ACC    0x2A  // 2
#define ROL_ZERO  0x26  // 5
#define ROL_ZERO_X  0x36  // 6
#define ROL_ABS    0x2E  // 6
#define ROL_ABS_X  0x3E  // 7

#define ROR_ACC    0x6A  // 2
#define ROR_ZERO  0x66  // 5
#define ROR_ZERO_X  0x76  // 6
#define ROR_ABS    0x6E  // 6
#define ROR_ABS_X  0x7E  // 7

// Double commands
#define LAX_IND_X  0xA3  // 6
#define LAX_ZERO  0xA7  // 3
#define LAX_IMM    0xAB  // 2
#define LAX_ABS    0xAF  // 4
#define LAX_IND_Y  0xB3  // 5*
#define LAX_ZERO_Y  0xB7  // 4
#define LAX_ABS_Y  0xBF  // 4*

#define SAX_IND_X  0x83  // 6
#define SAX_ZERO  0x87  // 3
#define SAX_ABS    0x8F  // 4
#define SAX_ZERO_Y  0x97  // 4

#define DCP_IND_X  0xC3  // 8
#define DCP_ZERO  0xC7  // 5
#define DCP_ABS    0xCF  // 6
#define DCP_IND_Y  0xD3  // 8
#define DCP_ZERO_X  0xD7  // 6
#define DCP_ABS_Y  0xDB  // 7
#define DCP_ABS_X  0xDF  // 7

#define ISC_IND_X  0xE3  // 8
#define ISC_ZERO  0xE7  // 5
#define ISC_ABS    0xEF  // 6
#define ISC_IND_Y  0xF3  // 8
#define ISC_ZERO_X  0xF7  // 6
#define ISC_ABS_Y  0xFB  // 7
#define ISC_ABS_X  0xFF  // 7

#define SLO_IND_X  0x03  // 8
#define SLO_ZERO  0x07  // 5
#define SLO_ABS    0x0F  // 6
#define SLO_IND_Y  0x13  // 8
#define SLO_ZERO_X  0x17  // 6
#define SLO_ABS_Y  0x1B  // 7
#define SLO_ABS_X  0x1F  // 7

#define RLA_IND_X  0x23  // 8
#define RLA_ZERO  0x27  // 5
#define RLA_ABS    0x2F  // 6
#define RLA_IND_Y  0x33  // 8
#define RLA_ZERO_X  0x37  // 6
#define RLA_ABS_Y  0x3B  // 7
#define RLA_ABS_X  0x3F  // 7

#define SRE_IND_X  0x43  // 8
#define SRE_ZERO  0x47  // 5
#define SRE_ABS    0x4F  // 6
#define SRE_IND_Y  0x53  // 8
#define SRE_ZERO_X  0x57  // 6
#define SRE_ABS_Y  0x5B  // 7
#define SRE_ABS_X  0x5F  // 7

#define RRA_IND_X  0x63  // 8
#define RRA_ZERO  0x67  // 5
#define RRA_ABS    0x6F  // 6
#define RRA_IND_Y  0x73  // 8
#define RRA_ZERO_X  0x77  // 6
#define RRA_ABS_Y  0x7B  // 7
#define RRA_ABS_X  0x7F  // 7

#define ANC_IMM    0x0B  // 2
#define ANC_IMM2  0x2B  // 2

#define ALR_IMM    0x4B  // 2

#define ARR_IMM    0x6B  // 2

#define AXS_IMM    0xCB  // 2

#define SHY_ABS_X  0x9C  // 5
#define SHX_ABS_Y  0x9E  // 5

// Misc.
#define BRK    0x00  // 7

// NOPs
#define NOP    0xEA  // 2
#define NOP2    0x04  // 3
#define NOP3    0x44  // 3
#define NOP4    0x64  // 3
#define NOP5    0x1A  // 2
#define NOP6    0x3A  // 2
#define NOP7    0x5A  // 2
#define NOP8    0x7A  // 2
#define NOP9    0xDA  // 2
#define NOP10    0xFA  // 2
#define NOP11    0x0C  // 4
#define NOP12    0x14  // 4
#define NOP13    0x34  // 4
#define NOP14    0x54  // 4
#define NOP15    0x74  // 4
#define NOP16    0xD4  // 4
#define NOP17    0xF4  // 4
#define NOP18    0x80  // 2
#define NOP19    0x1C  // 4*
#define NOP20    0x3C  // 4*
#define NOP21    0x5C  // 4*
#define NOP22    0x7C  // 4*
#define NOP23    0xDC  // 4*
#define NOP24    0xFC  // 4*
#define NOP25    0x89  // 2
#define NOP26    0x82  // 2
#define NOP27    0xC2  // 2
#define NOP28    0xE2  // 2

#define BIT_ZERO  0x24  // 3
#define BIT_ABS    0x2C  // 4
#define RTS    0x60  // 6
#define RTI    0x40  // 6

#endif
