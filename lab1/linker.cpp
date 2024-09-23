#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>

using namespace std;

class SymbolTableEntry
{
public:
    string symbol;       // The symbol name
    int Absoluteaddress; // The absolute address of the symbol
    int relativeAddress; // The relative address of the symbol
};

class ModuleBaseTableEntry
{
public:
    int moduleNumber; // The module number
    int baseAddress;  // The base address of the module
};

class Token
{
public:
    string *token;  // The token
    int lineOffset; // The offset of the token in the line
    int lineNumber; // The line number of the token
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
        result.token = new string(token);
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

    for (int i = 0; i < token.token->length(); i++)
    {
        if (!isdigit(token.token->at(i)))
            return -1;
    }

    return std::stoi(*token.token);
}

string *readSymbol(FILE *fp)
{
    Token token = getToken(fp);
    if (token.token == NULL)
        return NULL;

    if (!isalpha(token.token->at(0)))
        return NULL;

    for (int i = 1; i < token.token->length(); i++)
    {
        if (!isalnum(token.token->at(i)))
            return NULL;
    }
    return token.token;
}

char *readMARIE(FILE *fp)
{
    Token token = getToken(fp);
    if (token.token == NULL)
        return NULL;

    if (token.token->length() != 1 ||
        (token.token->at(0) != 'M' &&
         token.token->at(0) != 'A' &&
         token.token->at(0) != 'R' &&
         token.token->at(0) != 'I' &&
         token.token->at(0) != 'E'))
        return NULL;

    return strdup(token.token->c_str());
}

int pass1(FILE *fp)
{
    while (true)
    {
        int defCount = readInteger(fp);
        cout << "defCount: " << defCount << endl;
        if (defCount == -2)
            exit(2);

        for (int i = 0; i < defCount; i++)
        {
            // Read a symbol and a relative address
            Token token = getToken(fp);
            SymbolTableEntry entry;
            entry.symbol = strdup(token.token->c_str());
            entry.relativeAddress = readInteger(fp);
            cout << "Symbol: " << entry.symbol << ", Relative Address: " << entry.relativeAddress << endl;
            free(token.token);
        }
        cout << "-------------------" << endl;

        int useCount = readInteger(fp);
        if (useCount == -2)
            exit(2);
        cout << "useCount: " << useCount << endl;

        for (int i = 0; i < useCount; i++)
        {
            // Read a symbol
            string *symbol = readSymbol(fp);
            SymbolTableEntry entry;
            entry.symbol = strdup(symbol->c_str());
            cout << "Symbol: " << entry.symbol << endl;
        }
        cout << "-------------------" << endl;

        int instCount = readInteger(fp);
        if (instCount == -2)
            exit(2);
        cout << "instCount: " << instCount << endl;

        for (int i = 0; i < instCount; i++)
        {
            // Read a MARIE instruction
            char *addressMode = readMARIE(fp);
            int instruction = readInteger(fp);
            cout << "MARIE: " << addressMode << ", Instruction: " << instruction << endl;
        }
        cout << "--------------------------------------------------" << endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "Usage: " << argv[0] << " <filename>" << endl;
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        cout << "Error opening file: " << argv[1] << endl;
        return 1;
    }

    pass1(fp);
    fclose(fp);
    return 0;
}