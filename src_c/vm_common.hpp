#ifndef MUALI_VM_COMMON
#define MUALI_VM_COMMON

typedef uint32_t TypeId;

constexpr uint32_t TYPEID_NULL    = 0;
constexpr uint32_t TYPEID_INT     = 1;
constexpr uint32_t TYPEID_BOOL    = 2;
constexpr uint32_t TYPEID_FLOAT   = 3;
constexpr uint32_t TYPEID_STRING  = 4;
constexpr uint32_t TYPEID_ARRAY   = 5;
constexpr uint32_t TYPEID_DICT    = 6;
constexpr uint32_t TYPEID_FUNC    = 7;
constexpr uint32_t TYPEID_VARIANT = 255;
constexpr uint32_t TYPEID_CUSTOM  = 4096;

struct Function {
    Vec<uint8_t> code;
    size_t num_args = 0;
    size_t num_vars = 0;
    size_t num_regs = 0;
};

struct Global {
    Vec<Shared<Function>> funcs;
    ListMap<String, size_t> func_names;
    ListMap<String, size_t> var_names;
};

constexpr uint8_t OPINFO_CMPE         = 0x00;
constexpr uint8_t OPINFO_CMPNE        = 0x01;
constexpr uint8_t OPINFO_CMPGT        = 0x02;
constexpr uint8_t OPINFO_CMPLT        = 0x03;
constexpr uint8_t OPINFO_CMPGTE       = 0x04;
constexpr uint8_t OPINFO_CMPLTE       = 0x05;


constexpr uint8_t OP_FAULT            = 0x00;

constexpr uint8_t OP_SET              = 0x02;
constexpr uint8_t OP_SETIMM           = 0x03;

constexpr uint8_t OP_ADD              = 0x04;
constexpr uint8_t OP_ADDIMM           = 0x05;

constexpr uint8_t OP_SUB              = 0x06;
constexpr uint8_t OP_SUBIMM           = 0x07;

constexpr uint8_t OP_MUL              = 0x08;
constexpr uint8_t OP_MULIMM           = 0x09;

constexpr uint8_t OP_DIV              = 0x0A;
constexpr uint8_t OP_DIVIMM           = 0x0B;

constexpr uint8_t OP_MOD              = 0x0C;
constexpr uint8_t OP_MODIMM           = 0x0D;

constexpr uint8_t OP_BITAND           = 0x10;
constexpr uint8_t OP_BITANDIMM        = 0x11;
constexpr uint8_t OP_BITOR            = 0x12;
constexpr uint8_t OP_BITORIMM         = 0x13;
constexpr uint8_t OP_BITXOR           = 0x14;
constexpr uint8_t OP_BITXORIMM        = 0x15;
constexpr uint8_t OP_SHL              = 0x16;
constexpr uint8_t OP_SHLIMM           = 0x17;
constexpr uint8_t OP_SHR              = 0x18;
constexpr uint8_t OP_SHRIMM           = 0x19;

constexpr uint8_t OP_NEGATE           = 0x20;
constexpr uint8_t OP_NOT              = 0x21;
constexpr uint8_t OP_BITNOT           = 0x22;

constexpr uint8_t OP_INCI             = 0x24;
constexpr uint8_t OP_DECI             = 0x25;

constexpr uint8_t OP_INCF             = 0x26;
constexpr uint8_t OP_DECF             = 0x27;

constexpr uint8_t OP_INC              = 0x28;
constexpr uint8_t OP_DEC              = 0x29;

constexpr uint8_t OP_DOUBLEF          = 0x2A;
constexpr uint8_t OP_HALVEF           = 0x2B;
constexpr uint8_t OP_RECIPF           = 0x2C;

constexpr uint8_t OP_JINCILT          = 0x2E;
constexpr uint8_t OP_JINCILTIMM       = 0x2F;

constexpr uint8_t OP_CMPI0            = 0x30;
constexpr uint8_t OP_CMPI1            = 0x31;
constexpr uint8_t OP_CMPF0            = 0x32;
constexpr uint8_t OP_CMPF1            = 0x33;

constexpr uint8_t OP_CMPE             = 0x34;
constexpr uint8_t OP_CMPNE            = 0x35;
constexpr uint8_t OP_CMPGT            = 0x36;
constexpr uint8_t OP_CMPLT            = 0x37;
constexpr uint8_t OP_CMPGTE           = 0x38;
constexpr uint8_t OP_CMPLTE           = 0x39;

constexpr uint8_t OP_AND              = 0x3A;
constexpr uint8_t OP_OR               = 0x3B;

constexpr uint8_t OP_SETNULL          = 0x60;
constexpr uint8_t OP_SETZEROI         = 0x61;
constexpr uint8_t OP_SETZEROF         = 0x62;
constexpr uint8_t OP_SETONEI          = 0x63;
constexpr uint8_t OP_SETONEF          = 0x64;
constexpr uint8_t OP_SETNEGONEI       = 0x65;
constexpr uint8_t OP_SETNEGONEF       = 0x66;
constexpr uint8_t OP_SETEMPTYSTR      = 0x67;
constexpr uint8_t OP_SETEMPTYARRAY    = 0x68;
constexpr uint8_t OP_SETEMPTYDICT     = 0x69;
constexpr uint8_t OP_SETTRUE          = 0x6A;
constexpr uint8_t OP_SETFALSE         = 0x6B;
/*
constexpr uint8_t OP_ADDI             = 0x70;
constexpr uint8_t OP_ADDANY           = 0x71;
constexpr uint8_t OP_ADDIMMI          = 0x72;
constexpr uint8_t OP_ADDIMMANY        = 0x73;

constexpr uint8_t OP_SUBI             = 0x74;
constexpr uint8_t OP_SUBANY           = 0x75;
constexpr uint8_t OP_SUBIMMI          = 0x76;
constexpr uint8_t OP_SUBIMMANY        = 0x77;

constexpr uint8_t OP_MULI             = 0x78;
constexpr uint8_t OP_MULANY           = 0x79;
constexpr uint8_t OP_MULIMMI          = 0x7A;
constexpr uint8_t OP_MULIMMANY        = 0x7B;

constexpr uint8_t OP_DIVI             = 0x7C;
constexpr uint8_t OP_DIVANY           = 0x7D;
constexpr uint8_t OP_DIVIMMI          = 0x7E;
constexpr uint8_t OP_DIVIMMANY        = 0x7F;
*/

constexpr uint8_t OP_TOSTRING         = 0x80;
constexpr uint8_t OP_TOINT            = 0x81;
constexpr uint8_t OP_TOFLOAT          = 0x82;
constexpr uint8_t OP_FTOIBITS         = 0x83;
constexpr uint8_t OP_ITOFBITS         = 0x84;

constexpr uint8_t OP_GET_INDEX        = 0xB0;
constexpr uint8_t OP_SET_INDEX        = 0xB1;
constexpr uint8_t OP_SET_INDEXIMM     = 0xB2;

constexpr uint8_t OP_GET_MEMBER       = 0xB4;
constexpr uint8_t OP_SET_MEMBER       = 0xB5;
constexpr uint8_t OP_SET_MEMBERIMM    = 0xB6;

constexpr uint8_t OP_GET_GLOBAL       = 0xB8;
constexpr uint8_t OP_SET_GLOBAL       = 0xB9;
constexpr uint8_t OP_SET_GLOBALIMM    = 0xBA;

constexpr uint8_t OP_SQRT             = 0xC0;

constexpr uint8_t OP_CALL             = 0xD0;
constexpr uint8_t OP_CALL_INDIRECT    = 0xD1;
constexpr uint8_t OP_CALLDISCARD      = 0xD2;
constexpr uint8_t OP_CALLD_INDIRECT   = 0xD3;

constexpr uint8_t OP_J                = 0xD4;
constexpr uint8_t OP_JIF              = 0xD5;
constexpr uint8_t OP_JIFNOT           = 0xD6;
constexpr uint8_t OP_JIFNULL          = 0xD7;
constexpr uint8_t OP_JIFNOTNULL       = 0xD8;
constexpr uint8_t OP_JCMP             = 0xD9;
constexpr uint8_t OP_JCMPIMM          = 0xDA;
constexpr uint8_t OP_JILTIMM          = 0xDB;

constexpr uint8_t OP_RETURNVAL        = 0xDC;
constexpr uint8_t OP_RETURNIMM        = 0xDD;

constexpr uint8_t OP_BECOME           = 0xDE;
constexpr uint8_t OP_BECOME_INDIRECT  = 0xDF;

constexpr uint8_t OP_NOOP             = 0xE0;
constexpr uint8_t OP_EXIT             = 0xEE;

// 0xF_ are reserved for extended instructions if i run out of opcode space
//constexpr uint8_t OP_EXT0             = 0xF0;
//constexpr uint8_t OP_EXT1             = 0xF1;
// ....
// this gives me a second 12-bit opcode space if i need it

#endif // MUALI_VM_COMMON
