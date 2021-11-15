#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

static jmp_buf position;

enum
{    
    SUCCESS = 1,
    SIZE_NOTATION = 1000
};

////////////////////////////////  ERRORS  ////////////////////////////////

void SyntaxError()
{
    printf("syntax error");
    longjmp(position, SUCCESS);
}

void DivisionByZero()
{
    printf("division by zero");
    longjmp(position, SUCCESS);
}

void OtherError(int line)
{
    printf("other error (line %d)", line);
    longjmp(position, SUCCESS);
}

////////////////////////////////  VALUE TYPE  ////////////////////////////////

typedef struct TValue
{
    char Operator;
    int Number;
} TValue;

TValue CreateValue(char operator, int number)
{
    TValue new;
    new.Number = number;
    new.Operator = operator;
    return new;
}

////////////////////////////////  STACK  ////////////////////////////////

struct Tstack 
{
    TValue Value;
    struct Tstack* Next;
};

typedef struct Tstack* TStack;

TStack CreateStack() 
{
    return NULL;
}

bool IsEmptyStack(TStack stack)
{
    return stack == NULL;
}

TValue GetTopStack(TStack stack)
{
    if (IsEmptyStack(stack))
    {
        OtherError(__LINE__);
    }
    
    return stack->Value;
}

TValue PopStack(TStack* stack)
{
    if (IsEmptyStack(*stack))
    {
        OtherError(__LINE__);
    }

    TValue top = GetTopStack(*stack);
    TStack removeElem = *stack;
    *stack = (*stack)->Next;
    free(removeElem);
    return top;
}

void PushStack(TStack* stack, TValue value)
{
    TStack new = malloc(sizeof(*new));

    if (!new)
    {
        OtherError(__LINE__);
    }

    TStack last = *stack;
    new->Value = value;
    new->Next = last;
    *stack = new;
}

void DestroyStack(TStack* stack) 
{
    while (!IsEmptyStack(*stack)) 
    {
        PopStack(stack);
    }
}

////////////////////////////////  INPUT - OUTPUT  ////////////////////////////////

bool IsDigit(char symbol)
{
    return symbol >= '0' && symbol <= '9';
}

bool IsOperator(char symbol)
{
    return symbol == '+' || symbol == '-' || symbol == '*' || symbol == '/' || symbol == '(' || symbol == ')';
}

void CheckBrackets(const char* line)
{
    TStack stack = CreateStack();
    int index = 0;

    while(line[index] != '\0')
    {
        if(line[index] == ')')
        {
            if(!stack || index - PopStack(&stack).Number == 1)
            {
                SyntaxError();
            }
        }
        else if(line[index] == '(')
        {
            PushStack(&stack, CreateValue(line[index], index));
        }
        
        ++index;
    }

    if(stack)
    {
        DestroyStack(&stack);
        SyntaxError();
    }
}

void CheckSymbols(const char* line)
{
    while(*line)
    {
        if(!IsOperator(*line) && !IsDigit(*line))
        {
            SyntaxError();
        }

        ++line;
    }
}

void InputInfix(char* infix)
{
    if(gets(infix) == NULL)
    {
        OtherError(__LINE__);
    }

    if(*infix == '\0')
    {
        SyntaxError();
    }

    CheckSymbols(infix);
    CheckBrackets(infix);
}

////////////////////////////////  POSTFIX NOTATION AND CALCULATOR  ////////////////////////////////

int OperatorPriority(char value)
{
    switch(value)
    {
        case '(': return 0;
        case '-': return 1;
        case '+': return 1; 
        case '*': return 2;     
        case '/': return 2;
    }

    OtherError(__LINE__);
}

int SymbolToDigit(char symbol)
{
    return symbol - '0';
}

int StringToNumber(char* numberLine)
{
    int number = 0;
    int power = 1;
    int length = strlen(numberLine);

    for(int i = 0; i < length; ++i)
    {
        number += power * SymbolToDigit(numberLine[length - i - 1]);
        power *= 10;
    }

    return number;  
}

void CalcExpression(TStack* operatorStack, TStack* numberStack)
{
    if (IsEmptyStack(*operatorStack) || IsEmptyStack(*numberStack) || IsEmptyStack((*numberStack)->Next))
    {
        SyntaxError();
    }

    char operator = PopStack(operatorStack).Operator;
    int second = PopStack(numberStack).Number;
    int first = PopStack(numberStack).Number;

    switch(operator)
    {
        case '+': 
            PushStack(numberStack, CreateValue('\0', first + second));
            return;
        case '-': 
            PushStack(numberStack, CreateValue('\0', first - second));
            return;
        case '*': 
            PushStack(numberStack, CreateValue('\0', first * second));
            return;
        case '/':
            if(second == 0)
            {
                DestroyStack(operatorStack);
                DestroyStack(numberStack);
                DivisionByZero();
            }
            PushStack(numberStack, CreateValue('\0', first / second));
            return;
    }

    OtherError(__LINE__);
}

int Calc(const char* infix) 
{
    TStack operatorStack = CreateStack();
    TStack numberStack = CreateStack();
    size_t index = 0;

    while(infix[index] != '\0')
    {
        if(IsDigit(infix[index]))
        {
            char number[SIZE_NOTATION] = { 0 };
            for(int i = 0; IsDigit(infix[index]); ++index, ++i)
            {
                number[i] = infix[index];
            }
            
            PushStack(&numberStack, CreateValue('\0', StringToNumber(number)));
            memset(number, 0, SIZE_NOTATION);
        }
        else
        {
            if(infix[index] == '(')
            {
                PushStack(&operatorStack, CreateValue(infix[index], 0));
            }
            else if(infix[index] == ')')
            {
                while(GetTopStack(operatorStack).Operator != '(')
                {
                    CalcExpression(&operatorStack, &numberStack);
                }

                PopStack(&operatorStack);
            }
            else
            {
                while(!IsEmptyStack(operatorStack) && OperatorPriority(GetTopStack(operatorStack).Operator) >= OperatorPriority(infix[index]))
                {
                    CalcExpression(&operatorStack, &numberStack);
                }

                PushStack(&operatorStack, CreateValue(infix[index], 0));
            }

            ++index;
        }
    }

    while(!IsEmptyStack(operatorStack))
    {
        CalcExpression(&operatorStack, &numberStack);
    }

    int result = PopStack(&numberStack).Number;

    DestroyStack(&operatorStack);
    DestroyStack(&numberStack);

    return result;
}

////////////////////////////////  MAIN  ////////////////////////////////

int main()
{
    char infix [SIZE_NOTATION + 1] = { 0 };
    
    if(setjmp(position) == 0)
    {
        InputInfix(infix);
        printf("%d", Calc(infix));
    }

    return EXIT_SUCCESS;
}