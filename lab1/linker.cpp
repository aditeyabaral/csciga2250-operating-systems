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
    int moduleNumber;    // The module number it is defined in
    string symbol;       // The symbol name
    int absoluteAddress; // The absolute address of the symbol
    int relativeAddress; // The relative address of the symbol
    string errorMessage; // The error message for the symbol
    bool used;           // Whether the symbol is used
};

class ModuleBaseTableEntry
{
public:
    int moduleNumber; // The module number
    int baseAddress;  // The base address of the module
    int length;       // The length of the module (number of instructions)
};

class Token
{
public:
    string *token;  // The token
    int lineOffset; // The offset of the token in the line
    int lineNumber; // The line number of the token
};

class Instruction
{
public:
    int counter;         // the instruction counter
    int instruction;     // The instruction
    string errorMessage; // The error message for the instruction
};

vector<Instruction> instructions;             // The instructions in the memory map
vector<SymbolTableEntry> symbolTable;         // The symbol table
vector<ModuleBaseTableEntry> moduleBaseTable; // The module base table

void __parseerror(int errcode, int linenum, int lineoffset)
{
    static char *errstr[] = {
        "TOO_MANY_DEF_IN_MODULE", // > 16
        "TOO_MANY_USE_IN_MODULE", // > 16
        "TOO_MANY_INSTR",         // total num_instr exceeds memory size (512)
        "NUM_EXPECTED",           // Number expect, anything >= 2^30 is not a number either
        "SYM_EXPECTED",           // Symbol Expected
        "MARIE_EXPECTED",         // Addressing Expected which is M/A/R/I/E
        "SYM_TOO_LONG",           // Symbol Name is too long
    };
    cout << "Parse Error line " << linenum << " offset " << lineoffset << ": " << errstr[errcode] << endl;
    exit(1);
}

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
    if (token.token->length() > 16)
        return NULL;

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

SymbolTableEntry *getSymbolFromSymbolTable(string symbol)
{
    for (int i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].symbol == symbol)
            return &symbolTable[i];
    }
    return NULL;
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
        cout << setw(3) << setfill('0') << i << ": " << instructions[i].instruction << " " << instructions[i].errorMessage << endl;
    cout << endl;
}

void printWarningMessages()
{
    for (int i = 0; i < symbolTable.size(); i++)
    {
        if (!symbolTable[i].used)
            cout << "Warning: Module " << symbolTable[i].moduleNumber << ": " << symbolTable[i].symbol << " was defined but never used" << endl;
    }
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

        vector<SymbolTableEntry> defList;
        vector<SymbolTableEntry> useList;

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
            symbolEntry.moduleNumber = moduleEntry.moduleNumber;
            symbolEntry.used = false;
            addSymbolToSymbolTable(symbolEntry, moduleEntry);
            defList.push_back(symbolEntry);
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
        moduleEntry.length = instCount;

        for (int i = 0; i < instCount; i++)
        {
            char *addressMode = readMARIE(fp);
            int instruction = readInteger(fp);
        }
        moduleBaseTable.push_back(moduleEntry);

        for (int i = 0; i < defList.size(); i++)
        {
            SymbolTableEntry *symbolEntry = getSymbolFromSymbolTable(defList[i].symbol);
            if (symbolEntry->relativeAddress > moduleEntry.length)
            {
                cout << "Warning: Module " << moduleEntry.moduleNumber << ": " << symbolEntry->symbol << "=" << symbolEntry->relativeAddress << " valid=[0.." << moduleEntry.length - 1 << "] assume zero relative" << endl;
                symbolEntry->relativeAddress = 0;
                symbolEntry->absoluteAddress = moduleEntry.baseAddress + symbolEntry->relativeAddress;
            }
        }

        // clear the current def and use list
        defList.clear();
        useList.clear();
    }
}

void pass2(FILE *fp)
{
    int moduleNumber = 0;
    int instCount = 0;
    int currentInstCount = 0;
    cout << "Memory Map" << endl;
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
            string intructionErrorMessage = "";
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
                    intructionErrorMessage = "Error: Absolute address exceeds machine size; zero used";
                    updatedInstruction = opcode * 1000;
                }
                break;
            case 'R':
                if (operand > moduleEntry.length)
                {
                    intructionErrorMessage = "Error: Relative address exceeds module size; relative zero used";
                    instruction = opcode * 1000;
                }
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
                int absoluteAddress = 0;
                if (operand >= currentUseList.size())
                {
                    intructionErrorMessage = "Error: External operand exceeds length of uselist; treated as relative=0";
                }
                else
                {
                    SymbolTableEntry symbolEntry = currentUseList[operand];
                    SymbolTableEntry *matchedSymbol = getSymbolFromSymbolTable(symbolEntry.symbol);
                    if (matchedSymbol == NULL)
                    {
                        intructionErrorMessage = "Error: " + symbolEntry.symbol + " is not defined; zero used";
                        for (int i = 0; i < currentUseList.size(); i++)
                        {
                            if (currentUseList[i].symbol == symbolEntry.symbol)
                            {
                                currentUseList[i].used = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        matchedSymbol->used = true;
                        absoluteAddress = matchedSymbol->absoluteAddress;
                        for (int i = 0; i < currentUseList.size(); i++)
                        {
                            if (currentUseList[i].symbol == symbolEntry.symbol)
                            {
                                currentUseList[i].used = true;
                                break;
                            }
                        }
                    }
                }
                updatedInstruction = opcode * 1000 + absoluteAddress;
                break;
            }
            instructions.push_back({currentInstCount++, updatedInstruction, intructionErrorMessage});
        }

        // print the memory map
        for (int i = 0; i < instructions.size(); i++)
            cout << setw(3) << setfill('0') << instructions[i].counter << ": " << instructions[i].instruction << " " << instructions[i].errorMessage << endl;

        // if any symbols in the use list are not used, print a warning message
        for (int i = 0; i < currentUseList.size(); i++)
        {
            if (!currentUseList[i].used)
                cout << "Warning: Module " << moduleEntry.moduleNumber << ": uselist[" << i << "]=" << currentUseList[i].symbol << " was not used" << endl;
        }

        // clear the current def, use, and instructions list
        currentDefList.clear();
        currentUseList.clear();
        instructions.clear();
    }
    cout << endl
         << endl;
}

int main(int argc, char *argv[])
{
    // Check if the input file is specified
    if (argc < 2)
    {
        cout << "Error: No input file specified. Usage: " << argv[0] << " <input file>" << endl;
        return 1;
    }

    // Open the input file and check if it exists
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        cout << "Error opening file: " << argv[1] << endl;
        return 1;
    }

    // Perform the first pass and print the symbol table
    pass1(fp);          // Pass 1
    printSymbolTable(); // Print the symbol table

    // reset the file pointer to the beginning of the file
    fseek(fp, 0, SEEK_SET);

    // Perform the second pass and print the memory map
    pass2(fp); // Pass 2
    // printMemoryMap(); // Print the memory map

    printWarningMessages();

    fclose(fp);
    return 0;
}