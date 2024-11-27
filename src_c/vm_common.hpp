#ifndef MUALI_VM_COMMON
#define MUALI_VM_COMMON

//#define OPCODES_ALWAYS_8BIT
//#define OPCODES_ALWAYS_16BIT
// var length if neither defined
// fastest is 8-bit; second-fastest is var length (for some reason...)

#ifdef OPCODES_ALWAYS_8BIT
#define INTERPRETER_OPCODE_TABLE_BITS 8
#else
#define INTERPRETER_OPCODE_TABLE_BITS 16 // tune: can be from 12 to 16 (inclusive).
#endif

//#define VARLEN_VARREG_8BIT // fastest, but dangerous
#define VARLEN_VARREG_16BIT // second-fastest
// The following defines only affect var-reg operand encoding and not opcode encoding.
//#define VARLEN_VARREG_SB15
// Sign-bit format that only supports up to 32k vars/registers in a single function. You don't need more than that, right?
//#define VARLEN_VARREG_SB15LE
// Little endian variant.
//#define VARLEN_VARREG_LEB128
// LEB128 is little-endian var length int. otherwise VLQ, which is big endian. VLQ decodes faster, so this should be left disabled.
//#define VARLEN_VARREG_LZ4LIKE
// The LZ4 compression algorithm uses a very silly unary-like variable length integer scheme for match lengths.
// This format can be turned on here. It's slow and big, don't use it.

// If none of the above VARLEN_VARREG... defines are defined, VLQ encoding will be used, which is the third fastest

typedef uint32_t TypeId;

constexpr uint32_t TYPEID_NULL    = 0;
constexpr uint32_t TYPEID_INT     = 1;
constexpr uint32_t TYPEID_BOOL    = 2;
constexpr uint32_t TYPEID_FLOAT   = 3;
constexpr uint32_t TYPEID_STRING  = 4;
constexpr uint32_t TYPEID_ARRAY   = 5;
constexpr uint32_t TYPEID_DICT    = 6;
constexpr uint32_t TYPEID_FUNC    = 7;
constexpr uint32_t TYPEID_VARIANT = 31;
constexpr uint32_t TYPEID_CUSTOM  = 32;
constexpr uint32_t TYPEID_INVALID = -1;

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


// giving these high indexes for testing
constexpr uint16_t OP_SET              = 0x802;
constexpr uint16_t OP_SETIMM           = 0x803;

constexpr uint16_t OP_SUBIMM_I         = 0x807;
constexpr uint16_t OP_ADD_F            = 0x814;

constexpr uint16_t OP_SHLIMM_I         = 0x906;

constexpr uint16_t OP_INCI_INT         = 0xA04;
constexpr uint16_t OP_DECI_INT         = 0xA0F;
constexpr uint16_t OP_NEGATE_F         = 0xA10;

constexpr uint16_t OP_JINCILT          = 0xD0E;
constexpr uint16_t OP_JINCILTIMM       = 0xD0F;
constexpr uint16_t OP_JINCILTIMM_INT   = 0xD10;

constexpr uint16_t OP_SQRT             = 0xF10;
constexpr uint16_t OP_EXIT             = 0xF12;

constexpr uint16_t OP_TOSTRING         = 0xF00;
constexpr uint16_t OP_TOINT            = 0xF01;
constexpr uint16_t OP_TOFLOAT          = 0xF02;
constexpr uint16_t OP_FTOIBITS         = 0xF03;
constexpr uint16_t OP_ITOFBITS         = 0xF04;

constexpr uint16_t OP_FAULT            = 0xFF;

constexpr uint16_t OP_ADD              = 0x84;
constexpr uint16_t OP_ADDIMM           = 0x85;

constexpr uint16_t OP_SUB              = 0x86;
constexpr uint16_t OP_SUBIMM           = 0x87;

constexpr uint16_t OP_MUL              = 0x88;
constexpr uint16_t OP_MULIMM           = 0x89;

constexpr uint16_t OP_DIV              = 0x8A;
constexpr uint16_t OP_DIVIMM           = 0x8B;

constexpr uint16_t OP_MOD              = 0x8C;
constexpr uint16_t OP_MODIMM           = 0x8D;

constexpr uint16_t OP_BITAND           = 0x90;
constexpr uint16_t OP_BITANDIMM        = 0x91;
constexpr uint16_t OP_BITOR            = 0x92;
constexpr uint16_t OP_BITORIMM         = 0x93;
constexpr uint16_t OP_BITXOR           = 0x94;
constexpr uint16_t OP_BITXORIMM        = 0x95;
constexpr uint16_t OP_SHL              = 0x96;
constexpr uint16_t OP_SHLIMM           = 0x97;
constexpr uint16_t OP_SHR              = 0x98;
constexpr uint16_t OP_SHRIMM           = 0x99;

constexpr uint16_t OP_NEGATE           = 0xA0;
constexpr uint16_t OP_NOT              = 0xA1;
constexpr uint16_t OP_BITNOT           = 0xA2;

constexpr uint16_t OP_INCI             = 0xA4;
constexpr uint16_t OP_DECI             = 0xA5;

constexpr uint16_t OP_INCF             = 0xA6;
constexpr uint16_t OP_DECF             = 0xA7;

constexpr uint16_t OP_INC              = 0xA8;
constexpr uint16_t OP_DEC              = 0xA9;

constexpr uint16_t OP_DOUBLEF          = 0xAA;
constexpr uint16_t OP_HALVEF           = 0xAB;
constexpr uint16_t OP_RECIPF           = 0xAC;

constexpr uint16_t OP_CMPI0            = 0xB0;
constexpr uint16_t OP_CMPI1            = 0xB1;
constexpr uint16_t OP_CMPF0            = 0xB2;
constexpr uint16_t OP_CMPF1            = 0xB3;

constexpr uint16_t OP_CMPE             = 0xB4;
constexpr uint16_t OP_CMPNE            = 0xB5;
constexpr uint16_t OP_CMPGT            = 0xB6;
constexpr uint16_t OP_CMPLT            = 0xB7;
constexpr uint16_t OP_CMPGTE           = 0xB8;
constexpr uint16_t OP_CMPLTE           = 0xB9;

constexpr uint16_t OP_AND              = 0xBA;
constexpr uint16_t OP_OR               = 0xBB;

constexpr uint16_t OP_SETNULL          = 0xC0;
constexpr uint16_t OP_SETZEROI         = 0xC1;
constexpr uint16_t OP_SETZEROF         = 0xC2;
constexpr uint16_t OP_SETONEI          = 0xC3;
constexpr uint16_t OP_SETONEF          = 0xC4;
constexpr uint16_t OP_SETNEGONEI       = 0xC5;
constexpr uint16_t OP_SETNEGONEF       = 0xC6;
constexpr uint16_t OP_SETEMPTYSTR      = 0xC7;
constexpr uint16_t OP_SETEMPTYARRAY    = 0xC8;
constexpr uint16_t OP_SETEMPTYDICT     = 0xC9;
constexpr uint16_t OP_SETTRUE          = 0xCA;
constexpr uint16_t OP_SETFALSE         = 0xCB;

constexpr uint16_t OP_GET_INDEX        = 0xD0;
constexpr uint16_t OP_SET_INDEX        = 0xD1;
constexpr uint16_t OP_SET_INDEXIMM     = 0xD2;

constexpr uint16_t OP_GET_MEMBER       = 0xD4;
constexpr uint16_t OP_SET_MEMBER       = 0xD5;
constexpr uint16_t OP_SET_MEMBERIMM    = 0xD6;

constexpr uint16_t OP_GET_GLOBAL       = 0xD8;
constexpr uint16_t OP_SET_GLOBAL       = 0xD9;
constexpr uint16_t OP_SET_GLOBALIMM    = 0xDA;

constexpr uint16_t OP_CALL             = 0xE0;
constexpr uint16_t OP_CALL_INDIRECT    = 0xE1;
constexpr uint16_t OP_CALLDISCARD      = 0xE2;
constexpr uint16_t OP_CALLD_INDIRECT   = 0xE3;

constexpr uint16_t OP_J                = 0xE4;
constexpr uint16_t OP_JIF              = 0xE5;
constexpr uint16_t OP_JIFNOT           = 0xE6;
constexpr uint16_t OP_JIFNULL          = 0xE7;
constexpr uint16_t OP_JIFNOTNULL       = 0xE8;
constexpr uint16_t OP_JCMP             = 0xE9;
constexpr uint16_t OP_JCMPIMM          = 0xEA;
constexpr uint16_t OP_JILTIMM          = 0xEB;

constexpr uint16_t OP_RETURNVAL        = 0xEC;
constexpr uint16_t OP_RETURNIMM        = 0xED;

constexpr uint16_t OP_BECOME           = 0xEE;
constexpr uint16_t OP_BECOME_INDIRECT  = 0xEF;

constexpr uint16_t OP_NOOP             = 0xF0;

#endif // MUALI_VM_COMMON
