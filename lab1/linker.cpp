#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <vector>

using namespace std;

class SymbolTableEntry
{
public:
    string symbol;       // The symbol name
    int absoluteAddress; // The absolute address of the symbol
    int relativeAddress; // The relative address of the symbol
    string errorMessage; // The error message for the symbol
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

vector<int> instructions;                     // The instructions
vector<SymbolTableEntry> symbolTable;         // The symbol table
vector<ModuleBaseTableEntry> moduleBaseTable; // The module base table

Token getToken(FILE *fp)
{
    static char line[1024];
    static char *token = NULL;
    static int lineNumber = 0;
    static int lineOffset = 0;
    Token result;

    while (true)
    {
        // If there's no current token, read the next line
        if (token == NULL || *token == '\0')
        {
            if (fgets(line, sizeof(line), fp) != NULL)
            {
                lineNumber++;
                // Handle the case where the line is empty
                if (line[0] == '\n' || line[0] == '\0')
                {
                    continue; // Skip the empty line and go to the next one
                }
                token = strtok(line, " \t\n");
                lineOffset = 0;
            }
            else
            {
                result.token = NULL; // No more lines to read
                return result;
            }
        }

        // If there's a token to return, return it
        if (token != NULL)
        {
            result.token = new string(token);
            result.lineOffset = token - line + lineOffset;
            result.lineNumber = lineNumber;

            // Move to the next token
            token = strtok(NULL, " \t\n");
            return result;
        }
    }
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

int addSymbolToSymbolTable(SymbolTableEntry symbolEntry, ModuleBaseTableEntry moduleEntry)
{
    for (int i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].symbol == symbolEntry.symbol)
        {
            cout << "Warning: Module " << moduleEntry.moduleNumber << ": " << symbolEntry.symbol << " redefinition ignored" << endl;
            symbolTable[i].errorMessage = "Error: This variable is multiple times defined; first value used";
            return 1;
        }
    }
    symbolTable.push_back(symbolEntry);
    return 0;
}

SymbolTableEntry getSymbolFromSymbolTable(string symbol)
{
    for (int i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].symbol == symbol)
            return symbolTable[i];
    }
    SymbolTableEntry entry;
    entry.symbol = "";
    return entry;
}

void printDefList(vector<SymbolTableEntry> defList)
{
    cout << "Definition List" << endl;
    for (int i = 0; i < defList.size(); i++)
        cout << defList[i].symbol << "=" << defList[i].relativeAddress << endl;
    cout << endl;
}

void printUseList(vector<SymbolTableEntry> useList)
{
    cout << "Use List" << endl;
    for (int i = 0; i < useList.size(); i++)
        cout << useList[i].symbol << endl;
    cout << endl;
}

void printSymbolTable()
{
    cout << "Symbol Table" << endl;
    for (int i = 0; i < symbolTable.size(); i++)
        cout << symbolTable[i].symbol << "=" << symbolTable[i].absoluteAddress << " " << symbolTable[i].errorMessage << endl;
    cout << endl;
}

void printMemoryMap()
{
    cout << "Memory Map" << endl;
    for (int i = 0; i < instructions.size(); i++)
        // TODO: print index padded to 3 digits (for index 0, print 000)
        cout << setw(3) << setfill('0') << i << ": " << instructions[i] << endl;
}

void pass1(FILE *fp)
{
    int moduleNumber = 0;
    int instCount = 0;
    while (true)
    {
        ModuleBaseTableEntry moduleEntry;
        moduleEntry.moduleNumber = moduleNumber++;
        moduleEntry.baseAddress = moduleEntry.moduleNumber == 0
                                      ? 0
                                      : moduleBaseTable[moduleEntry.moduleNumber - 1].baseAddress + instCount;

        int defCount = readInteger(fp);
        if (defCount == -2)
            return;

        for (int i = 0; i < defCount; i++)
        {
            Token token = getToken(fp);
            SymbolTableEntry symbolEntry;
            symbolEntry.symbol = strdup(token.token->c_str());
            symbolEntry.relativeAddress = readInteger(fp);
            symbolEntry.absoluteAddress = moduleEntry.baseAddress + symbolEntry.relativeAddress;
            addSymbolToSymbolTable(symbolEntry, moduleEntry);
            free(token.token);
        }

        int useCount = readInteger(fp);
        if (useCount == -2)
            exit(2);

        for (int i = 0; i < useCount; i++)
        {
            string *symbol = readSymbol(fp);
            SymbolTableEntry entry;
            entry.symbol = strdup(symbol->c_str());
        }

        instCount = readInteger(fp);
        if (instCount == -2)
            exit(2);

        for (int i = 0; i < instCount; i++)
        {
            char *addressMode = readMARIE(fp);
            int instruction = readInteger(fp);
        }
        moduleBaseTable.push_back(moduleEntry);
    }
}

void pass2(FILE *fp)
{
    int moduleNumber = 0;
    int instCount = 0;
    while (true)
    {
        ModuleBaseTableEntry moduleEntry = moduleBaseTable[moduleNumber++];
        vector<SymbolTableEntry> currentDefList;
        vector<SymbolTableEntry> currentUseList;

        int defCount = readInteger(fp);
        if (defCount == -2)
            return;

        for (int i = 0; i < defCount; i++)
        {
            Token token = getToken(fp);
            SymbolTableEntry symbolEntry;
            symbolEntry.symbol = strdup(token.token->c_str());
            symbolEntry.relativeAddress = readInteger(fp);
            symbolEntry.absoluteAddress = moduleEntry.baseAddress + symbolEntry.relativeAddress;
            currentDefList.push_back(symbolEntry);
            free(token.token);
        }

        int useCount = readInteger(fp);
        if (useCount == -2)
            exit(2);

        for (int i = 0; i < useCount; i++)
        {
            string *symbol = readSymbol(fp);
            SymbolTableEntry entry;
            entry.symbol = strdup(symbol->c_str());
            currentUseList.push_back(entry);
        }

        instCount = readInteger(fp);
        if (instCount == -2)
            exit(2);

        for (int i = 0; i < instCount; i++)
        {
            char *addressMode = readMARIE(fp);
            int instruction = readInteger(fp);

            int opcode = instruction / 1000;
            int operand = instruction % 1000;
            int updatedInstruction = instruction;

            switch (addressMode[0])
            {
            case 'M':
                updatedInstruction = moduleBaseTable[operand].baseAddress;
                break;
            case 'A':
                if (operand >= 512)
                {
                    // TODO: Do something here
                    ;
                }
                break;
            case 'R':
                updatedInstruction = instruction + moduleEntry.baseAddress;
                break;
            case 'I':
                if (operand >= 900)
                {
                    // TODO: Do something here
                    ;
                }
                break;
            case 'E':
                SymbolTableEntry symbolEntry = currentUseList[operand % symbolTable.size()];
                SymbolTableEntry matchedSymbol = getSymbolFromSymbolTable(symbolEntry.symbol);
                updatedInstruction = opcode * 1000 + matchedSymbol.absoluteAddress;
                break;
            }
            instructions.push_back(updatedInstruction);
        }
        // clear the current def and use lists
        currentDefList.clear();
        currentUseList.clear();
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "Error: No input file specified. Usage: " << argv[0] << " <input file>" << endl;
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        cout << "Error opening file: " << argv[1] << endl;
        return 1;
    }

    pass1(fp);
    printSymbolTable();

    // reset the file pointer to the beginning of the file
    fseek(fp, 0, SEEK_SET);

    pass2(fp);
    printMemoryMap();

    fclose(fp);
    return 0;
}