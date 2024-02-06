#include <stdio.h>

#if RYN_TEST_csv

#include "ryn_csv.h"

static unsigned long GetStringLength(char *Data)
{
    unsigned long Length = 0;

    while (Data[Length++]);

    return Length;
}

static void TestFunction(void)
{
    char *Data = "2024/01/26,\"0Some Text with a numeric start\",\"0.12\",\"1,234.56\"";
    unsigned long Size = GetStringLength(Data);
    unsigned long I = 0;

    while (I < Size)
    {
        char *OffsetData = Data + I;
        ryn_csv_value Value = ryn_csv_ParseCsvValue(OffsetData, Size - I);

        printf("\nValue:\n  Type: %d\n  Size: %d\n  Quoted: %d\n  String: ", Value.Type, Value.Size, Value.Quoted);

        if (!Value.Quoted) printf("\"");
        for (unsigned long C = 0; C < Value.String.Size; ++C)
        {
            printf("%c", Value.String.Data[C]);
        }
        if (!Value.Quoted) printf("\"");
        printf("\n");

        I += Value.Size;

        if (Value.Size == 0 || I >= Size)
        {
            break;
        }
    }
}

#elif RYN_TEST_prof

#include "ryn_prof.h"

static void TestFunction(void)
{
    printf("Running test for ryn_prof... but not really.\n");
}

#else

static void TestFunction(void)
{
    printf("Error: No test function specified.\n");
}

#endif


int main(void)
{
    TestFunction();
}
