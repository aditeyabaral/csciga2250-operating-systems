#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <vector>

using namespace std;

// A Symbol class to store the symbol information
class Symbol
{
public:
    string name;         // The symbol name
    int absoluteAddress; // The absolute address of the symbol
    int relativeAddress; // The relative address of the symbol
    int moduleNumber;    // The module number it is defined in
    bool used;           // Whether the symbol has been used or redefined
    string errorMessage; // The error message for the symbol
};

// A Module class to store the module information
class Module
{
public:
    int number;      // The module number
    int size;        // The length of the module (number of instructions)
    int baseAddress; // The base address of the module
};

// A Token class to store the token information
class Token
{
public:
    string *value;
    int lineOffset; // The offset of the token in the line
    int lineNumber; // The line number of the token
};

// An Instruction class to store the instruction information
class Instruction
{
public:
    int counter;         // the instruction counter
    int instruction;     // The instruction
    string errorMessage; // The error message for the instruction
};

// A vector to store the symbol table and module base table
vector<Symbol> symbolTable;     // The symbol table
vector<Module> moduleBaseTable; // The module base table

// Compute 2^30 for the maximum integer value
const int MAX_INTEGER_VALUE = pow(2, 30);

void __parseerror(int errcode, int linenum, int lineoffset)
{
    // Error messages as defined in the specification
    string errstr[] = {
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
    // Define the static variables for the line, token, line number, line offset, and previous line length
    static char line[1024]; // TODO: Check this limit
    static char *token = NULL;
    static int lineNumber = 0;
    static int lineOffset = 0;
    static int prevLineLength = 0;
    Token result;

    while (true)
    {
        // If there's no current token, read the next line
        if (token == NULL || *token == '\0')
        {
            if (fgets(line, sizeof(line), fp) != NULL)
            {
                lineNumber++;                  // Increment the line number
                prevLineLength = strlen(line); // Store the length of the line before tokenization
                // Handle the case where the line is empty
                if (line[0] == '\n' || line[0] == '\0')
                    continue;                  // Skip the empty line and go to the next one
                lineOffset = 1;                // Reset the line offset
                token = strtok(line, " \t\n"); // Get the first token in the line
            }
            else // No more lines to read
            {
                // the line offset should be the length of the previous line
                lineOffset = prevLineLength;
                result.value = NULL;
                result.lineNumber = lineNumber;
                result.lineOffset = lineOffset;
                lineNumber = 0; // Reset the line number
                return result;
            }
        }

        // If there's a token to return, return it
        if (token != NULL)
        {
            lineOffset = token - line + 1;    // Calculate the line offset
            result.value = new string(token); // Copy the token to the result
            result.lineOffset = lineOffset;   // Calculate the line offset
            result.lineNumber = lineNumber;   // Set the line number
            // Move to the next token
            token = strtok(NULL, " \t\n"); // Get the next token
            return result;
        }
    }
}

int getTotalInstructionsInModuleBaseTable()
{
    // Calculate the total number of instructions
    int totalInstructions = 0;
    for (int i = 0; i < moduleBaseTable.size(); i++)
        totalInstructions += moduleBaseTable[i].size;
    return totalInstructions;
}

int *readInteger(FILE *fp, bool checkDefCount = false, bool checkUseCount = false, bool checkInstCount = false, bool canBeNull = false)
{
    Token token = getToken(fp);
    if (token.value == NULL) // No more tokens to read
    {
        if (!canBeNull)
            // If the token is NULL and it should not be NULL, throw an error
            __parseerror(3, token.lineNumber, token.lineOffset);
        return NULL; // This indicates EOF
    }

    for (int i = 0; i < token.value->length(); i++)
    {
        // Check if the token is a number
        if (!isdigit(token.value->at(i)))
            __parseerror(3, token.lineNumber, token.lineOffset);
    }

    // Obtain the integer value of the token
    int *value = new int(stoi(*token.value));

    // Check if the integer is >= 2^30
    if (*value >= MAX_INTEGER_VALUE)
        __parseerror(3, token.lineNumber, token.lineOffset);

    // Check if the number of definitions or uses is larger than 16
    if (checkDefCount && *value > 16)
        __parseerror(0, token.lineNumber, token.lineOffset);
    if (checkUseCount && *value > 16)
        __parseerror(1, token.lineNumber, token.lineOffset);

    // Check if the total number of instructions is larger than the memory size
    if (checkInstCount && *value + getTotalInstructionsInModuleBaseTable() > 512)
        __parseerror(2, token.lineNumber, token.lineOffset);

    return value;
}

Symbol readSymbol(FILE *fp, Module module, bool isDef = true)
{
    Token token = getToken(fp);
    // Check if the token is NULL or doesn't start with an alphabet
    if (token.value == NULL || !isalpha(token.value->at(0)))
        __parseerror(4, token.lineNumber, token.lineOffset);
    // Check if the token is too long
    if (token.value->length() > 16)
        __parseerror(6, token.lineNumber, token.lineOffset);
    // Check if the token contains only alphanumeric characters
    for (int i = 1; i < token.value->length(); i++)
    {
        if (!isalnum(token.value->at(i)))
            __parseerror(4, token.lineNumber, token.lineOffset);
    }

    // Create a new symbol
    Symbol symbol = Symbol();
    symbol.name = strdup(token.value->c_str());
    symbol.moduleNumber = module.number;
    symbol.used = false;
    symbol.errorMessage = "";

    // Read the relative address if it's a definition
    if (isDef)
    {
        symbol.relativeAddress = *readInteger(fp);
        // Compute the absolute address of the symbol
        symbol.absoluteAddress = module.baseAddress + symbol.relativeAddress;
    }
    return symbol;
}

char readMARIE(FILE *fp)
{
    Token token = getToken(fp);
    // Check if there are no more tokens to read or the token is not a valid MARIE addressing mode
    if (token.value == NULL || token.value->length() != 1 ||
        (token.value->at(0) != 'M' &&
         token.value->at(0) != 'A' &&
         token.value->at(0) != 'R' &&
         token.value->at(0) != 'I' &&
         token.value->at(0) != 'E'))
        __parseerror(5, token.lineNumber, token.lineOffset);

    return token.value->at(0);
}

int checkSymbolInSymbolTable(string symbolName)
{
    // Check if the symbol is already defined
    for (int i = 0; i < symbolTable.size(); i++)
    {
        // If the symbol is found, return its index
        if (symbolTable[i].name == symbolName)
            return i;
    }
    return -1;
}

Symbol *getSymbolFromSymbolTable(string symbolName)
{
    // Get the symbol from the symbol table if it exists
    if (checkSymbolInSymbolTable(symbolName) != -1)
        return &symbolTable[checkSymbolInSymbolTable(symbolName)];
    return NULL;
}

bool addSymbolToSymbolTable(Symbol symbol, Module module)
{
    // Check if the symbol is already defined
    int index = checkSymbolInSymbolTable(symbol.name);
    if (index != -1)
    {
        symbolTable[index].errorMessage = "Error: This variable is multiple times defined; first value used";
        return true;
    }
    // Else, add the symbol to the symbol table
    symbolTable.push_back(symbol);
    return false;
}

void printSymbolTable()
{
    cout << "Symbol Table" << endl;
    for (int i = 0; i < symbolTable.size(); i++)
        // Print the symbol table with the absolute addresses with the error messages
        cout << symbolTable[i].name << "=" << symbolTable[i].absoluteAddress << " " << symbolTable[i].errorMessage << endl;
    cout << endl;
}

void pass1(FILE *fp)
{
    int moduleNumber = 0;   // The current module number
    int totalInstCount = 0; // The total number of instructions
    while (true)
    {
        // Initialize an empty module and its def list
        Module module;
        vector<Symbol> defList;

        // Read the module number
        module.number = moduleNumber++;
        // Compute the base address of the module
        module.baseAddress = module.number == 0
                                 ? 0
                                 : moduleBaseTable[module.number - 1].baseAddress + moduleBaseTable[module.number - 1].size;

        // Read the number of symbol definitions in the module
        int *defCount = readInteger(fp, true, false, false, true);
        if (defCount == NULL)
            break; // No more tokens to read, EOF reached

        // Iterate through the symbol definitions
        for (int i = 0; i < *defCount; i++)
        {
            // Read the symbol and add it to the symbol table
            Symbol symbol = readSymbol(fp, module, true);
            bool existingSymbolCheck = addSymbolToSymbolTable(symbol, module);
            symbol.used = existingSymbolCheck;
            defList.push_back(symbol);
        }

        // Read the number of symbol uses in the module
        int *useCount = readInteger(fp, false, true);

        // Iterate through the symbol uses
        for (int i = 0; i < *useCount; i++)
            Symbol symbol = readSymbol(fp, module, false);

        // Read the number of instructions in the module
        int *instCount = readInteger(fp, false, false, true);

        // Update the module size
        module.size = *instCount;

        // Iterate through the instructions
        for (int i = 0; i < *instCount; i++)
        {
            // Read the addressing mode and instruction
            char addressMode = readMARIE(fp);
            int *instruction = readInteger(fp);
        }

        // Add the module to the module base table
        moduleBaseTable.push_back(module);

        for (int i = 0; i < defList.size(); i++)
        {
            // Check if the total number of instructions exceeds the memory size and print a warning message
            if (defList[i].relativeAddress > module.size && !defList[i].used)
            {
                // Print a warning message
                cout << "Warning: Module " << module.number << ": " << defList[i].name << "=" << defList[i].relativeAddress << " valid=[0.." << module.size - 1 << "] assume zero relative" << endl;
                // Update the symbol's relative address and absolute address in the symbol table
                defList[i].relativeAddress = 0;
                defList[i].absoluteAddress = module.baseAddress + defList[i].relativeAddress;
                Symbol *symbol = getSymbolFromSymbolTable(defList[i].name);
                symbol->relativeAddress = defList[i].relativeAddress;
                symbol->absoluteAddress = defList[i].absoluteAddress;
            }
            // Check if the symbol was defined multiple times
            else if (defList[i].used)
                cout << "Warning: Module " << module.number << ": " << defList[i].name << " redefinition ignored" << endl;
        }

        // clear the def list
        defList.clear();
    }

    // Print the symbol table
    printSymbolTable();
}

void instructionHandler(char addressMode, int operand, int opcode, int instruction, Module module, int *globalInstCount, vector<Symbol> *useList, vector<Instruction> *instructions)
{
    // Initialize the updated instruction and error message
    int newInstruction = instruction;
    string errorMessage = "";

    if (opcode >= 10)
    {
        opcode = 9;
        operand = 999;
        instruction = opcode * 1000 + operand;
        newInstruction = instruction;
        errorMessage = "Error: Illegal opcode; treated as 9999";
    }
    else
    {

        // Handle the instruction based on the addressing mode
        switch (addressMode)
        {
        case 'M': // Replace with the base address of the module
                  // If the operand exceeds the module base table size, print an error message
            if (operand >= moduleBaseTable.size())
            {
                errorMessage = "Error: Illegal module operand ; treated as module=0";
                operand = 0;
            }
            newInstruction = opcode * 1000 + moduleBaseTable[operand].baseAddress;
            break;
        case 'A': // Absolute address
            if (operand >= 512)
            // If the absolute address exceeds the machine size, print an error message
            {
                errorMessage = "Error: Absolute address exceeds machine size; zero used";
                newInstruction = opcode * 1000;
            }
            break;
        case 'R': // Replace relative address with the base address of the module
            if (operand > module.size)
            // If the relative address exceeds the module size, print an error message
            {
                errorMessage = "Error: Relative address exceeds module size; relative zero used";
                instruction = opcode * 1000;
            }
            newInstruction = instruction + module.baseAddress;
            break;
        case 'I': // Operand is unchanged
            // If the immediate operand exceeds 900, print an error message
            if (operand >= 900)
            {
                errorMessage = "Error: Illegal immediate operand; treated as 999";
                operand = 999;
                newInstruction = opcode * 1000 + operand;
            }
            break;
        case 'E':                    // Index into the use list with the operand
            int absoluteAddress = 0; // The default absolute address
            if (operand >= (*useList).size())
                // If the external operand exceeds the length of the use list, print an error message
                errorMessage = "Error: External operand exceeds length of uselist; treated as relative=0";
            else
            {
                Symbol symbol = (*useList)[operand];
                // Check if the symbol is defined
                int symbolTableIndex = checkSymbolInSymbolTable(symbol.name); // Check if the symbol is defined
                if (symbolTableIndex == -1)
                    // If the symbol is not defined, print an error message
                    errorMessage = "Error: " + symbol.name + " is not defined; zero used";
                else
                {
                    // Update the symbol's used flag and absolute address
                    symbolTable[symbolTableIndex].used = true;
                    absoluteAddress = symbolTable[symbolTableIndex].absoluteAddress;
                }
                // Update the use list to reflect the symbol's usage
                for (int i = 0; i < (*useList).size(); i++)
                {
                    if ((*useList)[i].name == symbol.name)
                    {
                        (*useList)[i].used = true;
                        break;
                    }
                }
            }
            // Update the instruction with the absolute address
            newInstruction = opcode * 1000 + absoluteAddress;
            break;
        }
    }

    // TODO: Rewrite without a global instruction counter (try using moduleBaseTable.size() instead)
    (*instructions).push_back({*globalInstCount, newInstruction, errorMessage});
    (*globalInstCount)++;
}

void pass2(FILE *fp)
{
    int moduleNumber = 0;     // The current module number
    int globalInstCount = 0;  // The global instruction counter
    bool syntaxError = false; // Whether a syntax error has occurred
    cout << "Memory Map" << endl;

    // Check if the module base table is empty
    if (moduleBaseTable.size() == 0)
        return;

    // Begin the second pass
    while (true)
    {
        // Fetch the current module and initialize its use and instructions list
        Module module = moduleBaseTable[moduleNumber++];
        vector<Symbol> useList;
        vector<Instruction> instructions;

        // Read the number of symbol definitions in the module
        int *defCount = readInteger(fp, true, false, false, true);
        if (defCount == NULL)
        {
            // EOF reached, no more tokens to read
            // Print all the symbols that are defined but not used
            for (int i = 0; i < symbolTable.size(); i++)
            {
                if (!symbolTable[i].used)
                    cout << "Warning: Module " << symbolTable[i].moduleNumber << ": " << symbolTable[i].name << " was defined but never used" << endl;
            }
            return;
        }

        // Iterate through the symbol definitions
        for (int i = 0; i < *defCount; i++)
            // Read the symbol
            Symbol symbol = readSymbol(fp, module, true);

        // Read the number of symbol uses in the module
        int *useCount = readInteger(fp, false, true);

        // Iterate through the symbol uses and add them to the use list
        for (int i = 0; i < *useCount; i++)
        {
            Symbol symbol = readSymbol(fp, module, false);
            useList.push_back(symbol);
        }

        // Read the number of instructions in the module
        int *instCount = readInteger(fp, false, false, true);

        // Iterate through the instructions
        for (int i = 0; i < *instCount; i++)
        {
            string intructionErrorMessage = ""; // The error message for the instruction
            // Read the addressing mode and instruction
            char addressMode = readMARIE(fp);
            int *instruction = readInteger(fp);
            // Extract the opcode and operand from the instruction
            int opcode = *instruction / 1000;
            int operand = *instruction % 1000;
            // Handle the instruction based on the addressing mode
            instructionHandler(addressMode, operand, opcode, *instruction, module, &globalInstCount, &useList, &instructions);
        }

        // Print the memory map
        for (int i = 0; i < instructions.size(); i++)
            cout << setw(3) << setfill('0') << instructions[i].counter << ": "
                 << setw(4) << setfill('0') << instructions[i].instruction << " "
                 << instructions[i].errorMessage << endl;

        // If any symbols in the use list are not used, print a warning message
        if (!syntaxError)
        {
            for (int i = 0; i < useList.size(); i++)
            {
                if (!useList[i].used)
                    cout << "Warning: Module " << module.number << ": uselist[" << i << "]=" << useList[i].name << " was not used" << endl;
            }
        }

        // clear the current use and instructions list
        useList.clear();
        instructions.clear();
    }
}

int main(int argc, char *argv[])
{
    // Check if the input file has been specified
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
    pass1(fp); // Pass 1

    // reset the file pointer to the beginning of the file
    fseek(fp, 0, SEEK_SET);

    // Perform the second pass and print the memory map
    pass2(fp); // Pass 2

    fclose(fp);
    return 0;
}