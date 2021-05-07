#include <string.h>

class AsmExprNode;

class BitStream
{
  public:
    BitStream(bool BigEndian = false) : bigEndian(BigEndian) { Reset(); }


void SetBigEndian(bool BigEndian) { bigEndian = BigEndian; }
    bool GetBigEndian() { return bigEndian; }

    void Reset()
    {
        bits = 0;
        memset(bytes, 0, sizeof(bytes));
    }
    void Add(int val, int bits);
    int GetBits() { return bits; }
    void GetBytes(unsigned char* dest, int size) { memcpy(dest, bytes, size < (bits + 7) / 8 ? size : (bits + 7) / 8); }

  private:
    int bits;
    unsigned char bytes[64];
    bool bigEndian;
};
enum asmError
{
    AERR_NONE,
    AERR_SYNTAX,
    AERR_OPERAND,
    AERR_BADCOMBINATIONOFOPERANDS,
    AERR_UNKNOWNOPCODE,
    AERR_INVALIDINSTRUCTIONUSE
};


class Coding
{
  public:
#ifdef DEBUG
    std::string name;
#endif
    enum Type
    {
        eot,
        bitSpecified = 1,
        valSpecified = 2,
        fieldSpecified = 4,
        indirect = 8,
        reg = 16,
        stateFunc = 32,
        stateVar = 64,
        number = 128,
        optional = 256,
        native = 512,
        illegal = 1024
    } type;
    int val;
    unsigned char bits;
    unsigned char field;
    char unary;
    char binary;
};
#ifdef DEBUG
#    define CODING_NAME(x) x,
#else
#    define CODING_NAME(x)
#endif

class Numeric
{
  public:
    Numeric(AsmExprNode* n) : node(n) {}
    Numeric() : node(nullptr) {}
    AsmExprNode* node;
    int pos = 0;
    int relOfs = 0;
    int size = 0;
    int used = 0;
};

class InputToken
{
  public:
    enum
    {
        TOKEN,
        REGISTER,
        NUMBER,
        LABEL
    } type;
    AsmExprNode* val;
};
