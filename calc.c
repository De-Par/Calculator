#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include "header.h"

static const char *p_word_;
static const char *p_word_start_;
static enum TokenType type_; 
static jmp_buf err_jmp_buf;
static char word_[MAX_EXPR_SIZE];
static char err_buf[MAX_BUF_SIZE];
static char *vars_lhs[MAX_PARSE_SYM];
static double vars_rhs[MAX_PARSE_SYM];
static int num_vars = 0;
static int some_num = 0;
static double val;

static void runtime_error(const char *str, ...)
{
    va_list arg_ptr;
    strcpy(err_buf, "Error!");
    va_start(arg_ptr, str);
    vsnprintf(err_buf + strlen(err_buf), sizeof(err_buf) - strlen(err_buf) - 1, str, arg_ptr);
    va_end(arg_ptr);
    err_buf[sizeof(err_buf) - 1] = '\0';  
    longjmp(err_jmp_buf, 1);
}

static int getRandom(int x)
{
    if (x <= 0)
        return 0;
    double r = ((double)(rand() % RAND_MAX)) / ((double)RAND_MAX + 1.0);
    return floor(r * x);
}

static bool percent(int x)
{
    if (x <= 0)
        return false;
    if (x >= 100)
        return true;
    return getRandom(100) > (100 - x);
}

static double doINT(double x)
{
    if (x >= 0)
        return floor(x);
    return -floor(-x);    
}

static double doRANDOM(double x)
{
    return getRandom(x);    
}

static double doPERCENT(double x)
{
    if (percent(x))   
        return 1.0;
    else
        return 0.0;
}

static double doMIN(const double arg1, const double arg2)
{
    return (arg1 < arg2 ? arg1 : arg2);
}

static double doMAX(double arg1, double arg2)
{
    return (arg1 > arg2 ? arg1 : arg2);
}

static double doFMOD(double arg1, double arg2)
{
    if (arg2 == 0.0)
        runtime_error("Divide by zero in mod!");
    return fmod(arg1, arg2);
}

static double doPOW(double arg1, double arg2)
{
    double result;
    int n = (int)arg2;
    if (n > 0 && n <= 64 && (double)n == arg2) 
    {
        result = arg1;
        while (--n) 
            result *= arg1;
        return result;
    } else {
        return pow(arg1, arg2);
    }
}

static FUN1_ENTRY* lookupFun1(char *name)
{
    for (int i = 0; fun1_table[i].fun; ++i) 
    {
        if (!strcmp(name, fun1_table[i].name)) 
            return &fun1_table[i];
    }
    return NULL;
}

static FUN2_ENTRY* lookupFun2(char *name)
{
    for (int i = 0; fun2_table[i].fun; ++i) 
    {
        if (!strcmp(name, fun2_table[i].name)) 
            return &fun2_table[i];
    }
    return NULL;
}

static int saveSymbol(char *lhs, double rhs) 
{
    for (int i = 0; i < num_vars; ++i) 
    {  
        if (!strcmp(vars_lhs[i], lhs)) 
        {
            vars_rhs[i] = rhs;
            return 1;  
        }
    }
    vars_lhs[num_vars] = malloc(strlen(lhs) + 1);
    if (!vars_lhs[num_vars]) 
        return 0;  
    strcpy(vars_lhs[num_vars], lhs);
    vars_rhs[num_vars] = rhs;
    ++num_vars;
    return 0;  
}

static double lookupSymbol(char *lhs)
{
    double rhs;
    if (!strcmp(lhs, "time")) 
        return (double)time(NULL);
    
    for (int i = 0; i < num_vars; ++i) 
    {
        if (!strcmp(vars_lhs[i], lhs))
        {
            rhs = vars_rhs[i];  
            return rhs;  
        }
    }
    rhs = PARSE_ERROR;
    return rhs; 
}

static enum TokenType getToken(bool ignoreSign)
{
    unsigned char cFirstCharacter;
    unsigned char cNextCharacter;
    char *p;
    *word_ = '\0';
    
    while (*p_word_ && isspace(*p_word_)) 
        ++p_word_;
    p_word_start_ = p_word_;       
   
    if (*p_word_ == 0 && type_ == END)          
        runtime_error("Unexpected end of expression!");
    cFirstCharacter = *p_word_;  

    if (cFirstCharacter == 0)  
    {
        strcpy(word_, "<end of expression>");
        return type_ = END;
    }
    cNextCharacter = *(p_word_ + 1);    

    if ((!ignoreSign && (cFirstCharacter == '+' || cFirstCharacter == '-') && (isdigit(cNextCharacter) || cNextCharacter == '.'))
        || isdigit(cFirstCharacter) || (cFirstCharacter == '.' && isdigit(cNextCharacter))) 
    {
        if ((cFirstCharacter == '+' || cFirstCharacter == '-'))
            p_word_++;
        while (isdigit(*p_word_) || *p_word_ == '.')
            p_word_++;

        if (*p_word_ == 'e' || *p_word_ == 'E') {
            p_word_++;           
            if ((*p_word_ == '+' || *p_word_ == '-'))
                p_word_++;      
            while (isdigit(*p_word_))    
                p_word_++;
        }
        STRNCPY(word_, p_word_start_, p_word_ - p_word_start_);
        p = NULL;
        val = strtod(word_, &p);
       
        if (p && *p != '\0')
            runtime_error("Bad numeric literal: %s!", word_);
        return type_ = NUMBER;
    }
    if (cNextCharacter == '=') 
    {
        switch (cFirstCharacter) 
        {
            case '=':
                type_ = EQ;
                break;
            case '<':
                type_ = LE;
                break;
            case '>':
                type_ = GE;
                break;
            case '!':
                type_ = NE;
                break;
            case '+':
                type_ = ASSIGN_ADD;
                break;
            case '-':
                type_ = ASSIGN_SUB;
                break;
            case '*':
                type_ = ASSIGN_MUL;
                break;
            case '/':
                type_ = ASSIGN_DIV;
                break;
            default:
                type_ = NONE;
                break;
        }
        if (type_ != NONE) 
        {
            STRNCPY(word_, p_word_start_, 2);
            p_word_ += 2;       
            return type_;
        }
    }
    switch (cFirstCharacter) 
    {
        case '&':
            if (cNextCharacter == '&')     
            {
                STRNCPY(word_, p_word_start_, 2);
                p_word_ += 2;        
                return type_ = AND;
            }
            break;
        case '|':
            if (cNextCharacter == '|')     
            {
                STRNCPY(word_, p_word_start_, 2);
                p_word_ += 2;        
                return type_ = OR;
            }
            break;
        case '=':
        case '<':
        case '>':
        case '+':
        case '-':
        case '/':
        case '*':
        case '^':
        case '(':
        case ')':
        case ',':
        case '!':
            STRNCPY(word_, p_word_start_, 1);
            ++p_word_;            
            type_ = cFirstCharacter;
            return type_;
    }
    if (!isalpha(cFirstCharacter)) 
    {
        if (cFirstCharacter < ' ') 
            runtime_error("Unexpected character 0x%02x!", cFirstCharacter);
        else
            runtime_error("Unexpected character '%c'!", cFirstCharacter);
    }
    while (isalnum(*p_word_) || *p_word_ == '_')
        ++p_word_;

    STRNCPY(word_, p_word_start_, p_word_ - p_word_start_);
    return type_ = NAME;
}

static double primary(bool get)  
{
    if (get)
        getToken(false); 

    switch (type_) 
    {
        case NUMBER:
        {
            double v = val;
            getToken(true);     
            return v;
        }

        case NAME:
        {
            char word[MAX_EXPR_SIZE];
            double v;
            FUN1_ENTRY *si;
            FUN2_ENTRY *di;
            STRNCPY(word, word_, sizeof(word) - 2);
            getToken(true);

            if (type_ == LHPAREN) 
            {
                if ((si = lookupFun1(word)) != NULL) 
                {
                    double v = expression(true);        
                    CheckToken(RHPAREN);
                    getToken(true);    
                    return si->fun(v);  
                }
                if ((di = lookupFun2(word)) != NULL) 
                {
                    double v1 = expression(true);
                    CheckToken(COMMA);
                    double v2 = expression(true);
                    CheckToken(RHPAREN);
                    getToken(true);     
                    return di->fun(v1, v2);     
                }
                runtime_error("Function '%s' not implemented!", word);
            }
            if ((v = lookupSymbol(word)) == PARSE_ERROR) 
                saveSymbol(word, v);  
            
            switch (type_) 
            { 
                case ASSIGN:
                    v = expression(true);
                    saveSymbol(word, v);
                    break;

                case ASSIGN_ADD:
                    v += expression(true);
                    saveSymbol(word, v);
                    break;

                case ASSIGN_SUB:
                    v -= expression(true);
                    saveSymbol(word, v);
                    break;

                case ASSIGN_MUL:
                    v *= expression(true);
                    saveSymbol(word, v);
                    break;

                case ASSIGN_DIV:
                    double d = expression(true);
                    if (d == 0.0)
                        runtime_error("Divide by zero!");
                    v /= d;
                    saveSymbol(word, v);
                    break;      
                
                default:
                    break;          
            }
            return v;          
        }
        case MINUS:       
            return -primary(true);
        case NOT:                   
            return (primary(true) == 0.0) ? 1.0 : 0.0;
        case LHPAREN:
        {
            double v = commaList(true); 
            CheckToken(RHPAREN);
            getToken(true);     
            return v;
        }
        default:
            if (type_ == END) 
                runtime_error("Unexpected end of expression!");
            else
                runtime_error("Unexpected token: '%s'", word_);
    }
    return 0;
}

static double term(bool get)      
{
    double left = primary(get);
    while (true) 
    {
        switch (type_) 
        {
            case POWER:
                left = pow(left, primary(true));
                break;
            case MULTIPLY:
                left *= primary(true);
                break;
            case DIVIDE:
                double d = primary(true);
                if (d == 0.0)
                    runtime_error("Divide by zero!");
                left /= d;
                break;
            default:
                return left;
        }
    }
}

static double addSubtract(bool get)        
{
    double left = term(get);
    while (true) 
    {
        switch (type_) 
        {
            case PLUS:
                left += term(true);
                break;
            case MINUS:
                left -= term(true);
                break;
            default:
                return left;
        }
    }
}

static double comparison(bool get) 
{
    double left = addSubtract(get);
    while (true) 
    {
        switch (type_) 
        {
            case LT:
                left = left < addSubtract(true) ? 1.0 : 0.0;
                break;
            case GT:
                left = left > addSubtract(true) ? 1.0 : 0.0;
                break;
            case LE:
                left = left <= addSubtract(true) ? 1.0 : 0.0;
                break;
            case GE:
                left = left >= addSubtract(true) ? 1.0 : 0.0;
                break;
            case EQ:
                left = left == addSubtract(true) ? 1.0 : 0.0;
                break;
            case NE:
                left = left != addSubtract(true) ? 1.0 : 0.0;
                break;
            default:
                return left;
        }
    }
}

static double expression(bool get) 
{
    double left = comparison(get);
    while (true) 
    {
        switch (type_) 
        {
            case AND:
            case OR:
                double d = comparison(true);
                if (type_ == AND) 
                    left = (left != 0.0) && (d != 0.0);
                else
                    left = (left != 0.0) || (d != 0.0);
                break;

            default:
                return left;
        }
    }
}

static double commaList(bool get)  
{
    if (some_num == 0)   
        srand(time(NULL));
    double left = expression(get);

    while (true) 
    {
        switch (type_) 
        {
            case COMMA:
                left = expression(true);
                break;             
            default:
                return left;
        }
    }
}

static double evaluate(char *expr) 
{
    err_buf[0] = '\0';  
    if(!setjmp(err_jmp_buf)) 
    {
        saveSymbol("pi", PI); 
        saveSymbol("PI", PI);
        saveSymbol("e",  E); 
        p_word_ = expr;
        type_ = NONE;
        double v = commaList(true);

        if (type_ != END)
            runtime_error("Unexpected text at end of expression: '%s'", p_word_start_);
        return v;
    } else {
        return PARSE_ERROR; 
    }
}

int main(void)
{
    char *p, expr[MAX_EXPR_SIZE];
    double result = 0;

    while (true) 
    {
        printf("Input expression:\n---> "); 
        fflush(stdout);  
        if (fgets(expr, sizeof(expr), stdin)) 
        {
            p = expr + strlen(expr) - 1;  
            while (p >= expr && *p < ' ') 
                *(p--) = '\0';

            if (*expr == '\0') 
            {
                printf("\nBye!\n\n");
                break; 
            }
            result = evaluate(expr);  
            printf("Result is %.16g\n\n", fabs(result) < 1e-12 ? 0 : result);  
            if (err_buf[0]) 
                printf("%s\n", &err_buf[0]); 
        }
    }
    return 0;
}