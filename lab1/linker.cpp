#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

using namespace std;

class SymbolTableEntry
{
public:
    char *symbol; // The symbol name
    int address;  // The absolute address of the symbol
};

class ModuleBaseTableEntry
{
public:
    int moduleNumber; // The module number
    int address;      // The base address of the module
};

class Token
{
public:
    char *token;    // The token
    int lineOffset; // The offset of the token in the line
    int lineNumber;
};

Token getToken(FILE *fp)
{
    static char line[1024];
    static char *token = NULL;
    static int lineNumber = 0;
    static int lineOffset = 0;
    Token result;

    // If there's no current token, read the next line
    if (token == NULL || *token == '\0')
    {
        if (fgets(line, sizeof(line), fp) != NULL)
        {
            lineNumber++;
            token = strtok(line, " \t\n");
            lineOffset = 0;
        }
        else
        {
            result.token = NULL; // No more lines to read
            return result;
        }
    }
    // If there's still a token to return, return it
    if (token != NULL)
    {
        result.token = strdup(token);
        result.lineOffset = token - line + lineOffset;
        result.lineNumber = lineNumber;

        // Move to the next token
        token = strtok(NULL, " \t\n");

        return result;
    }
    // If no token, return NULL to indicate end of tokens
    result.token = NULL;
    return result;
}

int readInteger(FILE *fp)
{
    Token token = getToken(fp);
    if (token.token == NULL)
        return -2;

    for (int i = 0; i < strlen(token.token); i++)
    {
        if (!isdigit(token.token[i]))
            return -1;
    }

    return atoi(token.token);
}

char *readSymbol(FILE *fp)
{
    Token token = getToken(fp);
    if (token.token == NULL)
        return NULL;

    if (!isalpha(token.token[0]))
        return NULL;

    for (int i = 1; i < strlen(token.token); i++)
    {
        if (!isalnum(token.token[i]))
            return NULL;
    }

    return token.token;
}

char *readMARIE(FILE *fp)
{
    Token token = getToken(fp);
    if (token.token == NULL)
        return NULL;

    if (strlen(token.token) != 1 || (token.token[0] != 'A' && token.token[0] != 'I' && token.token[0] != 'R' && token.token[0] != 'E'))
        return NULL;

    return token.token;
}

int pass1(FILE *fp)
{
    // Once you have the getToken() function written above and you verified that your token locations are properly reported,
    // extract it out of the program above and start writing the linker program. Layer the readInt(), readSymbol(),
    // readMARIE() functions on top of it by checking that the token has the correct sequence of characters and length (e.g.
    // integers are all numbers) and test via a simple program. Macros like isdigit(), isalpha(), isalnum()can proof
    // useful.

    while (true)
    {
        int defCount = readInteger(fp);
        printf("defCount: %d\n", defCount);
        if (defCount == -2)
            exit(2);

        for (int i = 0; i < defCount; i++)
        {
            // Read a symbol and a relative address
            Token token = getToken(fp);
            SymbolTableEntry entry;
            entry.symbol = strdup(token.token);
            entry.address = readInteger(fp);
            printf("Symbol: %s, Address: %d\n", entry.symbol, entry.address);
            free(token.token);
        }
        printf("--------------------\n");

        int useCount = readInteger(fp);
        if (useCount == -2)
            exit(2);
        printf("useCount: %d\n", useCount);

        for (int i = 0; i < useCount; i++)
        {
            // Read a symbol
            char *symbol = readSymbol(fp);
            SymbolTableEntry entry;
            entry.symbol = strdup(symbol);
            printf("Symbol: %s\n", entry.symbol);
            // free(token.token);
        }
        printf("--------------------\n");

        int instCount = readInteger(fp);
        if (instCount == -2)
            exit(2);
        printf("instCount: %d\n", instCount);

        for (int i = 0; i < instCount; i++)
        {
            // Read a MARIE instruction
            char *addressMode = readMARIE(fp);
            int instruction = readInteger(fp);
            printf("MARIE: %s, Instruction: %d\n", addressMode, instruction);
        }
        printf("--------------------------------------------------\n");
    }
}

int main(int argc, char *argv[])
{
    FILE *fp = fopen(argv[1], "r");

    if (fp == NULL)
    {
        cout << "Error opening file" << endl;
        return 1;
    }

    pass1(fp);
    fclose(fp);
    return 0;
}