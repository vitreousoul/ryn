#include <stdint.h>
/* #include <math.h> */
#include <stdlib.h>

/* TODO: #undef all #define's */

#define b32 uint32_t
#define u32 uint32_t
#define u64 uint64_t
#define s32 int32_t

#define f32 float

#define RYN_CSV_MAX_VALUE_COUNT 8192

#define IS_FIRST_CHAR(I, Quoted) ((!(Quoted) && (I) == 0) || ((Quoted) && (I) == 1))
#define IS_DIGIT(Char) ((Char) == '0' || (Char) == '1' || (Char) == '2' || (Char) == '3' || (Char) == '4' || \
                        (Char) == '5' || (Char) == '6' || (Char) == '7' || (Char) == '8' || (Char) == '9')
#define IS_START_OF_VALUE(I, Quoted) (((Quoted) && I == 1) || (!(Quoted) && I == 0))
#define IS_START_OF_NUMBER(Char) (IS_DIGIT(Char) || (Char) == '-' || (Char) == '+' || (Char) == '.')
#define IS_NUMERIC(Char) (IS_DIGIT(Char) || (Char) == ',' || (Char) == '.')



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
    ryn_csv_value_Numeric,
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



char *ryn_csv_StringOfCsvType(ryn_csv_value_type Type);
ryn_csv_value ryn_csv_ParseCsvValue(char *Data, u64 Size);
ryn_csv_value ryn_csv_ParseNumber(ryn_csv_value Value);



char *ryn_csv_StringOfCsvType(ryn_csv_value_type Type)
{
    switch (Type)
    {
    case ryn_csv_value_Unknown: return "Unknown";
    case ryn_csv_value_Empty: return "Empty";
    case ryn_csv_value_NewRow: return "NewRow";
    case ryn_csv_value_Numeric: return "Numeric";
    case ryn_csv_value_Integer: return "Integer";
    case ryn_csv_value_Float: return "Float";
    case ryn_csv_value_String: return "String";
    default: return "";
    }
}

ryn_csv_value ryn_csv_ParseNumber(ryn_csv_value StringValue)
{
    ryn_csv_value Value = {0};
    char *Data = StringValue.String.Data;
    s32 Size = StringValue.String.Size;
    b32 IsFloat = 0;
    b32 IsNan = 0;

    s32 I = 0;

    if (!Size || !IS_START_OF_NUMBER(Data[0]))
    {
        return Value;
    }

    if (Data[0] == '+')
    {
        ++I;
    }
    else if (Data[0] == '-')
    {
        ++I;
    }
    else if (Data[0] == '.')
    {
        IsFloat = 1;
        ++I;
    }

    for (; I < Size; ++I)
    {
        if (Data[I] == ',')
        {
            /* skip */
        }
        else if (Data[I] == '.')
        {
            if (IsFloat)
            {
                IsNan = 1;
                break;
            }
            else
            {
                IsFloat = 1;
            }
        }
        else if (!IS_DIGIT(Data[I]))
        {
            IsNan = 1;
            break;
        }
    }

    if (!IsNan)
    {
        char ConversionBuffer[1024];
        s32 ConversionIndex = 0;

        for (I = 0; I < Size; ++I)
        {
            if (ConversionIndex == 1023)
            {
               break;
            }
            if (Data[I] != ',')
            {
                ConversionBuffer[ConversionIndex] = StringValue.String.Data[I];
                ConversionIndex += 1;
            }
        }

        ConversionBuffer[ConversionIndex] = 0;

        if (IsFloat)
        {
            Value.Type = ryn_csv_value_Float;
            Value.Float = atof(ConversionBuffer);
        }
        else
        {
            Value.Type = ryn_csv_value_Integer;
            Value.Integer = atoi(ConversionBuffer);;
        }
    }

    return Value;
}

ryn_csv_value ryn_csv_ParseCsvValue(char *Data, u64 Size)
{
    ryn_csv_value Value = {0};

    if (!Size)
    {
        return Value;
    }
    
    b32 InQuote = 0;
    b32 IsPotentialNumber = 0;
    u32 I = 0;
    s32 SizeOffset = -1;

    Value.Type = ryn_csv_value_String;
    Value.String.Data = Data;

    if (Data[0] == CR)
    {
        if (Size >= 1 && Data[1] == LF)
        {
            Value.Type = ryn_csv_value_NewRow;
            Value.Size = 2;
            return Value;
        }
    }
    else if (I == 0 && Data[0] == ',')
    {
        ++I;
        Value.Type = ryn_csv_value_Empty;
        Value.Size = 1;
        return Value;
    }

    if (Data[0] == '"')
    {
        Value.Quoted = 1;
        InQuote = 1;
        Value.String.Data = Data + 1;
        ++I;
    }

    if (IS_START_OF_NUMBER(Data[I]))
    {
        IsPotentialNumber = 1;
        Value.Type = ryn_csv_value_Numeric;
    }

    for (; I < Size; ++I)
    {
        char Char = Data[I];

        if (!InQuote && Char == ',')
        {
            ++I;
            break;
        }
        else if (!InQuote && I + 1 < Size && Char == CR && Data[I + 1] == LF)
        {
            SizeOffset = 0;
            break;
        }
        else if (Value.Quoted && Char == '"')
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
        else if (IsPotentialNumber)
        {
            if (!IS_NUMERIC(Data[I]))
            {
                Value.Type = ryn_csv_value_String;
                IsPotentialNumber = 0;
            }
        }
    }

    if (Value.Quoted && I == 3)
    {
        Value.Type = ryn_csv_value_Empty;
    }

    s32 QuotedStringOffset = Value.Quoted ? -2 : 0;

    Value.String.Size = I + SizeOffset + QuotedStringOffset;
    Value.Size = I;

    return Value;
}




#undef b32
#undef u32
#undef u64
#undef s32

#undef f32

#undef RYN_CSV_MAX_VALUE_COUNT

#undef IS_FIRST_CHAR
#undef IS_DIGIT
#undef IS_START_OF_VALUE
#undef IS_START_OF_NUMBER
#undef IS_NUMERIC
