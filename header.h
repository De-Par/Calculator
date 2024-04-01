#ifndef CALC_H
#define CALC_H

#define PI 3.1415926535897932385
#define E 2.7182818284590452354
#define PARSE_ERROR sqrt(-1)
#define MAX_EXPR_SIZE 1024 
#define MAX_BUF_SIZE 256 
#define MAX_PARSE_SYM 100
#define true 1
#define false 0 
#define bool int

#define STRNCPY(dst, src, len) { strncpy(dst, src, len); if (len >= 0) dst[len] = '\0'; }
#define CheckToken(wanted) { if (type_ != wanted) runtime_error("expected '%c'", (int)wanted); }

static int saveSymbol(char *lhs, double rhs);
static int getRandom(int x);
static double lookupSymbol(char *lhs);  
static double evaluate(char *string); 
static double commaList(bool get);
static double expression(bool get);
static double comparison(bool get);
static double addSubtract(bool get);
static double term(bool get);      
static double primary(bool get);
static bool percent(int x);  
static enum TokenType getToken(bool ignoreSign);  

static double doINT(double x);
static double doRANDOM(double x);
static double doPERCENT(double x);
static double doMIN(double arg1, double arg2);
static double doMAX(double arg1, double arg2);
static double doFMOD(double arg1, double arg2);
static double doPOW(double arg1, double arg2);

typedef struct fun1_entry 
{
    char *name;  
    double (*fun)(double p1);
} const FUN1_ENTRY;

typedef struct fun2_entry 
{
    char *name;  
    double (*fun)(double p1, double p2);
} const FUN2_ENTRY;

enum TokenType 
{
    NONE,
    NAME,
    NUMBER,
    END,
    PLUS='+',
    MINUS='-',
    MULTIPLY='*',
    POWER='^',
    DIVIDE='/',
    ASSIGN='=',
    LHPAREN='(',
    RHPAREN=')',
    COMMA=',',
    NOT='!',
    LT='<',
    GT='>',
    LE,          // <=
    GE,          // >=
    EQ,          // ==
    NE,          // !=
    AND,         // &&
    OR,          // ||
    ASSIGN_ADD,  //  +=
    ASSIGN_SUB,  //  +-
    ASSIGN_MUL,  //  +*
    ASSIGN_DIV   //  +/
};

FUN1_ENTRY fun1_table[] = 
{
    { "sin", sin },
    { "cos", cos },
    { "tan", tan },
    { "asin", asin },
    { "acos", acos },
    { "atan", atan },
    { "sinh", sinh },
    { "cosh", cosh },
    { "tanh", tanh },
    { "exp", exp },
    { "ceil", ceil },
    { "floor", floor },
    { "sqrt", sqrt },
    { "lg", log10 },
    { "ln", log},
    { "log", log},
    { "abs", fabs },
    { "int", doINT },
    { "round", round },
    { "rand", doRANDOM },
    { "percent", doPERCENT },
    { "", NULL }
};

FUN2_ENTRY fun2_table[] = 
{
    { "min", doMIN },
    { "max", doMAX },
    { "mod", doFMOD },
    { "pow", doPOW },
    { "", NULL }
};

#endif 