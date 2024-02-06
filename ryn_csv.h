#include <stdint.h>

/* TODO: #undef all #define's */

#define b32 uint32_t
#define u32 uint32_t
#define u64 uint64_t

#define f32 float

#define RYN_CSV_MAX_VALUE_COUNT 8192

#define IS_FIRST_CHAR(I, Quoted) ((!(Quoted) && (I) == 0) || ((Quoted) && (I) == 1))
#define IS_DIGIT(Char) ((Char) == '0' || (Char) == '1' || (Char) == '2' || (Char) == '3' || (Char) == '4' || \
                        (Char) == '5' || (Char) == '6' || (Char) == '7' || (Char) == '8' || (Char) == '9')
#define IS_START_OF_VALUE(I, Quoted) (((Quoted) && I == 1) || (!(Quoted) && I == 0))
#define IS_START_OF_NUMBER(Char) (IS_DIGIT(Char) || (Char) == '-' || (Char) == '+' || (Char) == '.')



typedef enum
{
    LF = 0x0a,
    CR = 0x0d,
} char_code;

typedef enum
{
    ryn_csv_value_Unknown,
    ryn_csv_value_Empty,
    ryn_csv_value_NewRow,
    ryn_csv_value_Integer,
    ryn_csv_value_Float,
    ryn_csv_value_String,
} ryn_csv_value_type;

typedef struct
{
    ryn_csv_value_type Type;
    u32 Size;
    b32 Quoted;
    union
    {
        struct
        {
            char *Data;
            u64 Size;
        } String;
        u32 Integer;
        f32 Float;
    };
} ryn_csv_value;

typedef struct
{
    b32 Done;
    ryn_csv_value Values[RYN_CSV_MAX_VALUE_COUNT];
} ryn_csv_result;



ryn_csv_value ryn_csv_ParseCsvValue(char *Data, u64 Size);



ryn_csv_value ryn_csv_ParseCsvValue(char *Data, u64 Size)
{
    ryn_csv_value Value = {0};
    
    b32 InQuote = 0;
    b32 IsPotentialNumber = 0;
    b32 NumberIsNegative = 0;
    u32 I;

    Value.Type = ryn_csv_value_String;
    Value.String.Data = Data;

    for (I = 0; I < Size; ++I)
    {
        char Char = Data[I];

        if (I == 0 && Char == CR)
        {
            if (I + 1 < Size && Data[I + 1] == LF)
            {
                ++I;
                Value.Type = ryn_csv_value_NewRow;
                break;
            }
        }
        else if (I == 0 && Char == '"')
        {
            Value.Quoted = 1;
            InQuote = 1;
        }
        else if (!InQuote && Char == ',')
        {
            ++I;
            break;
        }
        else if (I == 0 && Char == ',')
        {
            ++I;
            Value.Type = ryn_csv_value_Empty;
            break;
        }
        else
        {
            if (Value.Quoted && Char == '"')
            {
                if (I + 1 < Size && Data[I + 1] == '"')
                {
                    /* NOTE: Skip past the escaped double-quote. */
                    ++I;
                }
                else
                {
                    InQuote = 0;
                }
            }
        }
    }

    Value.String.Size = I - 1;

    Value.Size = I;
    /* TODO: Check if value is quoted and if the size is that of the empty string.
       If so, set the type to Empty.
    */

    return Value;
}



#undef b32
#undef u32
#undef u64

#undef f32

#undef RYN_CSV_MAX_VALUE_COUNT

#undef IS_FIRST_CHAR
#undef IS_DIGIT
#undef IS_START_OF_VALUE
#undef IS_START_OF_NUMBER
