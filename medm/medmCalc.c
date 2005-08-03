/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* Calc routines for MEDM taken and modified from those in EPICS base
 *
 * Allows building MEDM with other control systems, such as CDEV.  The
 * primary modifications have been to put everything in one file and
 * reformat according to MEDM conventions.  Credit for the routine
 * belongs with the original authors:
 *
 *   Julie Sander and Bob Dalesio
 *   Janet Anderson and Marty Kraimer
 *
 * The external functions are:
 *   calcPerform
 *   postfix
 *
 * The calling program should provide char arrays (as for the EPICS calcRecord):
 *   calc[40];    (for calc expression)
 *   post[200];   (for calculated postfix expression)
 *
 * The input values are passed as a double array with length up to 12
 *   or as a pointer to a double.  */

#define NOT_SET         0
#define TRUE_COND       1
#define FALSE_COND      2

#define         BAD_EXPRESSION  0
#define         FETCH_A         1
#define         FETCH_B         2
#define         FETCH_C         3
#define         FETCH_D         4
#define         FETCH_E         5
#define         FETCH_F         6
#define         FETCH_G         7
#define         FETCH_H         8
#define         FETCH_I         9
#define         FETCH_J         10
#define         FETCH_K         11
#define         FETCH_L         12
#define         ACOS            13
#define         ASIN            14
#define         ATAN            15
#define         COS             16
#define         COSH            17
#define         SIN             18
#define         STORE_A         19
#define         STORE_B         20
#define         STORE_C         21
#define         STORE_D         22
#define         STORE_E         23
#define         STORE_F         24
#define         STORE_G         25
#define         STORE_H         26
#define         STORE_I         27
#define         STORE_J         28
#define         STORE_K         29
#define         STORE_L         30
#define         RIGHT_SHIFT     31
#define         LEFT_SHIFT      32
#define         SINH            33
#define         TAN             34
#define         TANH            35
#define         LOG_2           36
#define         COND_ELSE       37
#define         ABS_VAL         38
#define         UNARY_NEG       39
#define         SQU_RT          40
#define         EXP             41
#define         CEIL            42
#define         FLOOR           43
#define         LOG_10          44
#define         LOG_E           45
#define         RANDOM          46
#define         ADD             47
#define         SUB             48
#define         MULT            49
#define         DIV             50
#define         EXPON           51
#define         MODULO          52
#define         BIT_OR          53
#define         BIT_AND         54
#define         BIT_EXCL_OR     55
#define         GR_OR_EQ        56
#define         GR_THAN         57
#define         LESS_OR_EQ      58
#define         LESS_THAN       59
#define         NOT_EQ          60
#define         EQUAL           61
#define         REL_OR          62
#define         REL_AND         63
#define         REL_NOT         64
#define         BIT_NOT         65
#define         PAREN           66
#define         MAX             67
#define         MIN             68
#define         COMMA           69
#define         COND_IF         70
#define         COND_END        71
#define         CONSTANT        72
#define         CONST_PI        73
#define         CONST_D2R       74
#define         CONST_R2D       75
#define         NINT            76
#define         ATAN2           77
#define         END_STACK       127

#include        <stdlib.h>
#include        <stdio.h>
#include        <string.h>
#include        <math.h>
#include        <ctype.h>

#ifndef PI
#define PI 3.141592654
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Element types for postfix */
#define OPERAND         0
#define UNARY_OPERATOR  1
#define BINARY_OPERATOR 2
#define EXPR_TERM       3
#define COND            4
#define CLOSE_PAREN     5
#define CONDITIONAL     6
#define ELSE            7
#define SEPERATOR       8
#define TRASH           9
#define FLOAT_PT_CONST  10
#define MINUS_OPERATOR  11

#define UNARY_MINUS_I_S_P  7
#define UNARY_MINUS_I_C_P  8
#define UNARY_MINUS_CODE   UNARY_NEG
#define BINARY_MINUS_I_S_P 4
#define BINARY_MINUS_I_C_P 4
#define BINARY_MINUS_CODE  SUB

/* Parsing return values for postfix */
#define FINE             0
#define UNKNOWN_ELEMENT -1
#define END             -2

/*
 * element table
 *
 * structure of an element
 */
struct  expression_element{
    char        element[10];    /* character representation of an element */
    char        in_stack_pri;   /* priority in translation stack */
    char        in_coming_pri;  /* priority when first checking */
    char        type;   /* element type */
    char        code;                   /* postfix representation */
};

/*
 * NOTE: DO NOT CHANGE WITHOUT READING THIS NOTICE !!!!!!!!!!!!!!!!!!!!
 * Because the routine that looks for a match in this table takes the first
 * match it finds, elements whose designations are contained in other elements
 * MUST come first in this list. (e.g. ABS will match A if A preceeds ABS and
 * then try to find BS therefore ABS must be first in this list
 */
static struct expression_element        elements[] = {
  /* element    i_s_p   i_c_p   type_element    internal_rep */
    {"ABS",             7,      8,      UNARY_OPERATOR, ABS_VAL}, /* absolute value */
    {"NOT",             7,      8,      UNARY_OPERATOR, UNARY_NEG}, /* unary negate */
    {"-",               7,      8,      MINUS_OPERATOR, UNARY_NEG}, /* unary negate (or binary op) */
    {"SQRT",            7,      8,      UNARY_OPERATOR, SQU_RT}, /* square root */
    {"SQR",             7,      8,      UNARY_OPERATOR, SQU_RT}, /* square root */
    {"EXP",             7,      8,      UNARY_OPERATOR, EXP}, /* exponential function */
    {"LOGE",            7,      8,      UNARY_OPERATOR, LOG_E}, /* log E */
    {"LN",              7,      8,      UNARY_OPERATOR, LOG_E}, /* log E */
    {"LOG",             7,      8,      UNARY_OPERATOR, LOG_10}, /* log 10 */
    {"ACOS",            7,      8,      UNARY_OPERATOR, ACOS}, /* arc cosine */
    {"ASIN",            7,      8,      UNARY_OPERATOR, ASIN}, /* arc sine */
    {"ATAN2",           7,      8,      UNARY_OPERATOR, ATAN2}, /* arc tangent */
    {"ATAN",            7,      8,      UNARY_OPERATOR, ATAN}, /* arc tangent */
    {"MAX",             7,      8,      UNARY_OPERATOR, MAX}, /* maximum of 2 args */
    {"MIN",             7,      8,      UNARY_OPERATOR, MIN}, /* minimum of 2 args */
    {"CEIL",            7,      8,      UNARY_OPERATOR, CEIL}, /* smallest integer >= */
    {"FLOOR",           7,      8,      UNARY_OPERATOR, FLOOR}, /* largest integer <=  */
    {"NINT",            7,      8,      UNARY_OPERATOR, NINT}, /* nearest integer */
    {"COSH",            7,      8,      UNARY_OPERATOR, COSH}, /* hyperbolic cosine */
    {"COS",             7,      8,      UNARY_OPERATOR, COS}, /* cosine */
    {"SINH",            7,      8,      UNARY_OPERATOR, SINH}, /* hyperbolic sine */
    {"SIN",             7,      8,      UNARY_OPERATOR, SIN}, /* sine */
    {"TANH",            7,      8,      UNARY_OPERATOR, TANH}, /* hyperbolic tangent*/
    {"TAN",             7,      8,      UNARY_OPERATOR, TAN}, /* tangent */
    {"!",               7,      8,      UNARY_OPERATOR, REL_NOT}, /* not */
    {"~",               7,      8,      UNARY_OPERATOR, BIT_NOT}, /* and */
    {"RNDM",            0,      0,      OPERAND,        RANDOM}, /* Random Number */
    {"OR",              1,      1,      BINARY_OPERATOR,BIT_OR}, /* or */
    {"AND",             2,      2,      BINARY_OPERATOR,BIT_AND}, /* and */
    {"XOR",             1,      1,      BINARY_OPERATOR,BIT_EXCL_OR}, /* exclusive or */
    {"PI",              0,      0,      OPERAND,        CONST_PI}, /* pi */
    {"D2R",             0,      0,      OPERAND,        CONST_D2R}, /* pi/180 */
    {"R2D",             0,      0,      OPERAND,        CONST_R2D}, /* 180/pi */
    {"A",               0,      0,      OPERAND,        FETCH_A}, /* fetch var A */
    {"B",               0,      0,      OPERAND,        FETCH_B}, /* fetch var B */
    {"C",               0,      0,      OPERAND,        FETCH_C}, /* fetch var C */
    {"D",               0,      0,      OPERAND,        FETCH_D}, /* fetch var D */
    {"E",               0,      0,      OPERAND,        FETCH_E}, /* fetch var E */
    {"F",               0,      0,      OPERAND,        FETCH_F}, /* fetch var F */
    {"G",               0,      0,      OPERAND,        FETCH_G}, /* fetch var G */
    {"H",               0,      0,      OPERAND,        FETCH_H}, /* fetch var H */
    {"I",               0,      0,      OPERAND,        FETCH_I}, /* fetch var I */
    {"J",               0,      0,      OPERAND,        FETCH_J}, /* fetch var J */
    {"K",               0,      0,      OPERAND,        FETCH_K}, /* fetch var K */
    {"L",               0,      0,      OPERAND,        FETCH_L}, /* fetch var L */
    {"a",               0,      0,      OPERAND,        FETCH_A}, /* fetch var A */
    {"b",               0,      0,      OPERAND,        FETCH_B}, /* fetch var B */
    {"c",               0,      0,      OPERAND,        FETCH_C}, /* fetch var C */
    {"d",               0,      0,      OPERAND,        FETCH_D}, /* fetch var D */
    {"e",               0,      0,      OPERAND,        FETCH_E}, /* fetch var E */
    {"f",               0,      0,      OPERAND,        FETCH_F}, /* fetch var F */
    {"g",               0,      0,      OPERAND,        FETCH_G}, /* fetch var G */
    {"h",               0,      0,      OPERAND,        FETCH_H}, /* fetch var H */
    {"i",               0,      0,      OPERAND,        FETCH_I}, /* fetch var I */
    {"j",               0,      0,      OPERAND,        FETCH_J}, /* fetch var J */
    {"k",               0,      0,      OPERAND,        FETCH_K}, /* fetch var K */
    {"l",               0,      0,      OPERAND,        FETCH_L}, /* fetch var L */
    {"0",               0,      0,      FLOAT_PT_CONST, CONSTANT}, /* flt pt constant */
    {"1",               0,      0,      FLOAT_PT_CONST, CONSTANT}, /* flt pt constant */
    {"2",               0,      0,      FLOAT_PT_CONST, CONSTANT}, /* flt pt constant */
    {"3",               0,      0,      FLOAT_PT_CONST, CONSTANT}, /* flt pt constant */
    {"4",               0,      0,      FLOAT_PT_CONST, CONSTANT}, /* flt pt constant */
    {"5",               0,      0,      FLOAT_PT_CONST, CONSTANT}, /* flt pt constant */
    {"6",               0,      0,      FLOAT_PT_CONST, CONSTANT}, /* flt pt constant */
    {"7",               0,      0,      FLOAT_PT_CONST, CONSTANT}, /* flt pt constant */
    {"8",               0,      0,      FLOAT_PT_CONST, CONSTANT}, /* flt pt constant */
    {"9",               0,      0,      FLOAT_PT_CONST, CONSTANT}, /* flt pt constant */
    {".",               0,      0,      FLOAT_PT_CONST, CONSTANT}, /* flt pt constant */
    {"?",               0,      0,      CONDITIONAL,    COND_IF}, /* conditional */
    {":",               0,      0,      CONDITIONAL,    COND_ELSE}, /* else */
    {"(",               0,      8,      UNARY_OPERATOR, PAREN}, /* open paren */
    {"^",               6,      6,      BINARY_OPERATOR,EXPON}, /* exponentiation */
    {"**",              6,      6,      BINARY_OPERATOR,EXPON}, /* exponentiation */
    {"+",               4,      4,      BINARY_OPERATOR,ADD}, /* addition */
#if 0
    {"-",               4,      4,      BINARY_OPERATOR,SUB}, /* subtraction */
#endif
    {"*",               5,      5,      BINARY_OPERATOR,MULT}, /* multiplication */
    {"/",               5,      5,      BINARY_OPERATOR,DIV}, /* division */
    {"%",               5,      5,      BINARY_OPERATOR,MODULO}, /* modulo */
    {",",               0,      0,      SEPERATOR,      COMMA}, /* comma */
    {")",               0,      0,      CLOSE_PAREN,    PAREN}, /* close paren */
    {"||",              1,      1,      BINARY_OPERATOR,REL_OR}, /* or */
    {"|",               1,      1,      BINARY_OPERATOR,BIT_OR}, /* or */
    {"&&",              2,      2,      BINARY_OPERATOR,REL_AND}, /* and */
    {"&",               2,      2,      BINARY_OPERATOR,BIT_AND}, /* and */
    {">>",              2,      2,      BINARY_OPERATOR,RIGHT_SHIFT}, /* right shift */
    {">=",              3,      3,      BINARY_OPERATOR,GR_OR_EQ}, /* greater or equal*/
    {">",               3,      3,      BINARY_OPERATOR,GR_THAN}, /* greater than */
    {"<<",              2,      2,      BINARY_OPERATOR,LEFT_SHIFT}, /* left shift */
    {"<=",              3,      3,      BINARY_OPERATOR,LESS_OR_EQ},/* less or equal to*/
    {"<",               3,      3,      BINARY_OPERATOR,LESS_THAN}, /* less than */
    {"#",               3,      3,      BINARY_OPERATOR,NOT_EQ}, /* not equal */
    {"=",               3,      3,      BINARY_OPERATOR,EQUAL}, /* equal */
    {""}
};

/* Function prototypes */

long calcPerform(double *parg, double *presult, char  *post);
long postfix(char *pinfix, char *ppostfix, short *perror);
static int find_element(char *pbuffer, struct expression_element **pelement,
  short *pno_bytes);
static int get_element(char *pinfix, struct expression_element  **pelement,
  short *pno_bytes);
static double local_random();

/* Global variables */

static unsigned short seed = 0xa3bf;
static unsigned short multy = 191 * 8 + 5;  /* 191 % 8 == 5 */
static unsigned short addy = 0x3141;

/* This module contains the code for processing the arithmetic
 * expressions defined in calculation records. postfix must be called
 * to convert a valid infix expression to postfix. calcperform
 * calculates the postfix expression.
 *
 * Subroutines
 *
 *      Public
 *
 * calcPerform          perform the calculation
 *          args
 *              double *pargs   address of arguments (12)
 *              double *presult address of result
 *              char   *rpcl    address of reverse polish buffer
 *          returns
 *              0               fetched successfully
 *              -1              fetch failed
 *
 * Private routine for calcPerform
 *      local_random            random number generator
 *          returns
 *              double value between 0.00 and 1.00
 */
long calcPerform(double *parg, double *presult, char *post)
{
    double *pstacktop;  /* stack of values      */
    double              stack[80];
    short               temp1;
    short       i;
    double              *top;
    int                 itop;           /* integer top value    */
    int                 inexttop;       /* ineteger next to top value   */
    short               cond_flag;      /* conditional else flag        */
    short               got_if;

  /* initialize flag  */
    cond_flag = NOT_SET;
    pstacktop = &stack[0];

#if 0
    for (i=0;i<184;i++){
        printf ("%d_",post[i]);
        if ( post[i] == END_STACK ) break;
        if ( post[i] == 71 ) i=i+8;
    }
    printf ("*FINISHED*\n");
#endif

    if(*post == BAD_EXPRESSION) return(-1);

  /* set post to postfix expression in calc structure */
    top = pstacktop;

  /* polish calculator loop */
    while (*post != END_STACK){

        switch (*post){
        case FETCH_A:
            ++pstacktop;
            *pstacktop = parg[0];
            break;

        case FETCH_B:
            ++pstacktop;
            *pstacktop = parg[1];
            break;

        case FETCH_C:
            ++pstacktop;
            *pstacktop = parg[2];
            break;

        case FETCH_D:
            ++pstacktop;
            *pstacktop = parg[3];
            break;

        case FETCH_E:
            ++pstacktop;
            *pstacktop = parg[4];
            break;

        case FETCH_F:
            ++pstacktop;
            *pstacktop = parg[5];
            break;

        case FETCH_G:
            ++pstacktop;
            *pstacktop = parg[6];
            break;

        case FETCH_H:
            ++pstacktop;
            *pstacktop = parg[7];
            break;

        case FETCH_I:
            ++pstacktop;
            *pstacktop = parg[8];
            break;

        case FETCH_J:
            ++pstacktop;
            *pstacktop = parg[9];
            break;

        case FETCH_K:
            ++pstacktop;
            *pstacktop = parg[10];
            break;

        case FETCH_L:
            ++pstacktop;
            *pstacktop = parg[11];
            break;

        case CONST_PI:
            ++pstacktop;
            *pstacktop = PI;
            break;

        case CONST_D2R:
            ++pstacktop;
            *pstacktop = PI/180.;
            break;

        case CONST_R2D:
            ++pstacktop;
            *pstacktop = 180./PI;
            break;

        case ADD:
            --pstacktop;
            *pstacktop = *pstacktop + *(pstacktop+1);
            break;

        case SUB:
            --pstacktop;
            *pstacktop = *pstacktop - *(pstacktop+1);
            break;

        case MULT:
            --pstacktop;
            *pstacktop = *pstacktop * *(pstacktop+1);
            break;

        case DIV:
            --pstacktop;
            if (*(pstacktop+1) == 0) /* can't divide by zero */
              return(-1);
            *pstacktop = *pstacktop / *(pstacktop+1);
            break;

        case COND_IF:
          /* if false condition then skip true expression */
            if (*pstacktop == 0.0) {
                                /* skip to matching COND_ELSE */
                for (got_if=1; got_if>0 && *(post+1) != END_STACK; ++post) {
                    if (*(post+1) == CONSTANT  ) post+=8;
                    else if (*(post+1) == COND_IF  ) got_if++;
                    else if (*(post+1) == COND_ELSE) got_if--;
                }
            }
          /* remove condition from stack top */
            --pstacktop;
            break;

        case COND_ELSE:
          /* result, true condition is on stack so skip false condition  */
          /* skip to matching COND_END */
            for (got_if=1; got_if>0 && *(post+1) != END_STACK; ++post) {
                if (*(post+1) == CONSTANT  ) post+=8;
                else if (*(post+1) == COND_IF ) got_if++;
                else if (*(post+1) == COND_END) got_if--;
            }
            break;

        case COND_END:
            break;

        case ABS_VAL:
            if (*pstacktop < 0) *pstacktop = -*pstacktop;
            break;

        case UNARY_NEG:
            *pstacktop = -1* (*pstacktop);
            break;

        case SQU_RT:
            if (*pstacktop < 0) return(-1);     /* undefined */
            *pstacktop = sqrt(*pstacktop);
            break;

        case EXP:
            *pstacktop = exp(*pstacktop);
            break;

        case LOG_10:
            *pstacktop = log10(*pstacktop);
            break;

        case LOG_E:
            *pstacktop = log(*pstacktop);
            break;

        case RANDOM:
            ++pstacktop;
            *pstacktop = local_random();
            break;

        case EXPON:
            --pstacktop;
            if (*pstacktop == 0) break;
            if (*pstacktop < 0){
                temp1 = (int) *(pstacktop+1);
                                /* is exponent an integer */
                if ((*(pstacktop+1) - (double)temp1) != 0) return (-1);
                *pstacktop = exp(*(pstacktop+1) * log(-*pstacktop));
                                /* is value negative */
                if ((temp1 % 2) > 0) *pstacktop = -*pstacktop;
            }else{
                *pstacktop = exp(*(pstacktop+1) * log(*pstacktop));
            }
            break;

        case MODULO:
            --pstacktop;
            itop = (int)*pstacktop;
            inexttop = (int)*(pstacktop+1);
            if (inexttop == 0)
              return(-1);
            i =  itop % inexttop;
            *pstacktop = i;
            break;

        case REL_OR:
            --pstacktop;
            *pstacktop = (*pstacktop || *(pstacktop+1));
            break;

        case REL_AND:
            --pstacktop;
            *pstacktop = (*pstacktop && *(pstacktop+1));
            break;

        case BIT_OR:
          /* force double values into integers and or them */
            itop = (int)*pstacktop;
            inexttop = (int)*(pstacktop-1);
            --pstacktop;
            *pstacktop = (inexttop | itop);
            break;

        case BIT_AND:
          /* force double values into integers and and them */
            itop = (int)*pstacktop;
            inexttop = (int)*(pstacktop-1);
            --pstacktop;
            *pstacktop = (inexttop & itop);
            break;

        case BIT_EXCL_OR:
          /*force double values to integers to exclusive or them*/
            itop = (int)*pstacktop;
            inexttop = (int)*(pstacktop-1);
            --pstacktop;
            *pstacktop = (inexttop ^ itop);
            break;

        case GR_OR_EQ:
            --pstacktop;
            *pstacktop = *pstacktop >= *(pstacktop+1);
            break;

        case GR_THAN:
            --pstacktop;
            *pstacktop = *pstacktop > *(pstacktop+1);
            break;

        case LESS_OR_EQ:
            --pstacktop;
            *pstacktop = *pstacktop <= *(pstacktop+1);
            break;

        case LESS_THAN:
            --pstacktop;
            *pstacktop = *pstacktop < *(pstacktop+1);
            break;

        case NOT_EQ:
            --pstacktop;
            *pstacktop = *pstacktop != *(pstacktop+1);
            break;

        case EQUAL:
            --pstacktop;
            *pstacktop = (*pstacktop == *(pstacktop+1));
            break;

        case RIGHT_SHIFT:
            itop = (int)*pstacktop;
            inexttop = (int)*(pstacktop-1);
            --pstacktop;
            *pstacktop = (inexttop >> itop);
            break;

        case LEFT_SHIFT:
            itop = (int)*pstacktop;
            inexttop = (int)*(pstacktop-1);
            --pstacktop;
            *pstacktop = (inexttop << itop);
            break;

        case MAX:
            --pstacktop;
            if (*pstacktop < *(pstacktop+1))
              *pstacktop = *(pstacktop+1);
            break;

        case MIN:
            --pstacktop;
            if (*pstacktop > *(pstacktop+1))
              *pstacktop = *(pstacktop+1);
            break;


        case ACOS:
            *pstacktop = acos(*pstacktop);
            break;

        case ASIN:
            *pstacktop = asin(*pstacktop);
            break;

        case ATAN:
            *pstacktop = atan(*pstacktop);
            break;

        case ATAN2:
            --pstacktop;
            *pstacktop = atan2(*(pstacktop+1), *pstacktop);
            break;

        case COS:
            *pstacktop = cos(*pstacktop);
            break;

        case SIN:
            *pstacktop = sin(*pstacktop);
            break;

        case TAN:
            *pstacktop = tan(*pstacktop);
            break;

        case COSH:
            *pstacktop = cosh(*pstacktop);
            break;

        case SINH:
            *pstacktop = sinh(*pstacktop);
            break;

        case TANH:
            *pstacktop = tanh(*pstacktop);
            break;

        case CEIL:
            *pstacktop = ceil(*pstacktop);
            break;

        case FLOOR:
            *pstacktop = floor(*pstacktop);
            break;

        case NINT:
            *pstacktop = (double)(long)((*pstacktop) >= 0 ? (*pstacktop)+0.5 : (*pstacktop)-0.5);
            break;

        case REL_NOT:
            *pstacktop = ((*pstacktop)?0:1);
            break;

        case BIT_NOT:
            itop = (int)*pstacktop;
            *pstacktop = ~itop;
            break;

        case CONSTANT:
            ++pstacktop;
            ++post;
            if ( post == NULL ) {
                ++post;
                printf("%.7s bad constant in expression\n",post);
                break;
            }
            memcpy((void *)pstacktop,post,8);
            post+=7;
            break;
        default:
            printf("%d bad expression element\n",*post);
            break;
        }

      /* move ahead in postfix expression */
        ++post;
    }

  /* if everything is peachy,the stack should end at its first position */
    if (++top == pstacktop)
      *presult = *pstacktop;
    else
      return(-1);
    return(0);
}

/*
 * RAND
 *
 * generates a random number between 0 and 1 using the
 * seed = (multy * seed) + addy         Random Number Generator by Knuth
 *                                              SemiNumerical Algorithms
 *                                              Chapter 1
 * randy = seed / 65535.0          To normalize the number between 0 - 1
 */
static double local_random()
{
    double  randy;

  /* random number */
    seed = (seed * multy) + addy;
    randy = (float) seed / 65535.0;

  /* between 0 - 1 */
    return(randy);
}

/*
 * Postfix Subroutines
 *
 *      Public
 *
 * postfix              convert an algebraic expression to symbolic postfix
 *      args
 *              pinfix          the algebraic expression
 *              ppostfix        the symbolic postfix expression
 *      returns
 *              0               successful
 *              -1              not successful
 * Private routines for postfix
 *
 * find_element         finds a symbolic element in the expression element tbl
 *      args
 *              pbuffer         pointer to the infox expression element
 *              pelement        pointer to the expression element table entry
 *              pno_bytes       pointer to the size of this element
 *      returns
 *              TRUE            element found
 *              FALSE           element not found
 * get_element          finds the next expression element in the infix expr
 *      args
 *              pinfix          pointer into the infix expression
 *              pelement        pointer to the expression element table
 *              pno_bytes       size of the element in the infix expression
 *              plink           pointer to a resolved database reference (N/A)
 *      returns
 *              FINE            found an expression element
 *              VARIABLE        found a database reference
 *              UNKNOWN_ELEMENT unknown element found in the infix expression
 * match_element        finds an alpha element in the expression table
 *      args
 *              pbuffer         pointer to an alpha expression element
 *              pelement        pointer to the expression element table
 *      returns
 *              TRUE            found the element in the element table
 *              FLASE           expression element not found
 */

/*
 * FIND_ELEMENT
 *
 * find the pointer to an entry in the element table
 */
static int find_element(char *pbuffer, struct expression_element **pelement,
  short *pno_bytes)
{

  /* compare the string to each element in the element table */
    *pelement = &elements[0];
    while ((*pelement)->element[0] != '\0') {
        if (strncmp(pbuffer,(*pelement)->element,
          strlen((*pelement)->element)) == 0){
            *pno_bytes += strlen((*pelement)->element);
            return(TRUE);
        }
        *pelement += 1;
    }
    return(FALSE);
}

/*
 * GET_ELEMENT
 *
 * get an expression element
 */
static int get_element(char *pinfix, struct expression_element  **pelement,
  short *pno_bytes)
{

  /* get the next expression element from the infix expression */
    if (*pinfix == '\0') return(END);
    *pno_bytes = 0;
    while (*pinfix == 0x20){
        *pno_bytes += 1;
        pinfix++;
    }
    if (*pinfix == '\0') return(END);
    if (!find_element(pinfix,pelement,pno_bytes))
      return(UNKNOWN_ELEMENT);
    return(FINE);


}

/*
 * POSTFIX
 *
 * convert an infix expression to a postfix expression
 */
long postfix(char *pinfix, char *ppostfix, short *perror)
{
    short               no_bytes;
    register short      operand_needed;
    register short      new_expression;
    struct expression_element   stack[80];
    struct expression_element   *pelement;
    register struct expression_element  *pstacktop;
    double              constant;
    register char   *pposthold, *pc;
    char in_stack_pri, in_coming_pri, code;
    char           *ppostfixStart = ppostfix;

  /* convert infix expression to upper case */
    for (pc=pinfix; *pc; pc++) {
        if (islower(*pc)) *pc = toupper(*pc);
    }

  /* place the expression elements into postfix */
    operand_needed = TRUE;
    new_expression = TRUE;
    *ppostfix = END_STACK;
    *perror = 0;
    if (* pinfix == 0 )
      return(0);
    pstacktop = stack;
    while (get_element(pinfix,&pelement,&no_bytes) != END){
        pinfix += no_bytes;
        switch (pelement->type){

        case OPERAND:
            if (!operand_needed){
                *perror = 5;
                *ppostfixStart = BAD_EXPRESSION; return(-1);
            }

          /* add operand to the expression */
            *ppostfix++ = pelement->code;

            operand_needed = FALSE;
            new_expression = FALSE;
            break;

        case FLOAT_PT_CONST:
            if (!operand_needed){
                *perror = 5;
                *ppostfixStart = BAD_EXPRESSION; return(-1);
            }

          /* add constant to the expression */
            *ppostfix++ = pelement->code;
            pposthold = ppostfix;

            pinfix-=no_bytes;
            while (*pinfix == ' ') *ppostfix++ = *pinfix++;
            while (TRUE) {
                if ( ( *pinfix >= '0' && *pinfix <= '9' ) || *pinfix == '.' ) {
                    *ppostfix++ = *pinfix;
                    pinfix++;
                } else if ( *pinfix == 'E' || *pinfix == 'e' ) {
                    *ppostfix++ = *pinfix;
                    pinfix++;
                    if (*pinfix == '+' || *pinfix == '-' ) {
                        *ppostfix++ = *pinfix;
                        pinfix++;
                    }
                } else break;
            }
            *ppostfix++ = '\0';

            ppostfix = pposthold;
            if ( sscanf(ppostfix,"%lg",&constant) != 1) {
                *ppostfix = '\0';
            } else {
                memcpy(ppostfix,(void *)&constant,8);
            }
            ppostfix+=8;

            operand_needed = FALSE;
            new_expression = FALSE;
            break;

        case BINARY_OPERATOR:
            if (operand_needed){
                *perror = 4;
                *ppostfixStart = BAD_EXPRESSION; return(-1);
            }

          /* add operators of higher or equal priority to       */
          /* postfix notation                           */
            while ((pstacktop >= stack+1) &&
              (pstacktop->in_stack_pri >= pelement->in_coming_pri)) {
                *ppostfix++ = pstacktop->code;
                pstacktop--;
            }

          /* add new operator to stack */
            pstacktop++;
            *pstacktop = *pelement;

            operand_needed = TRUE;
            break;

        case UNARY_OPERATOR:
            if (!operand_needed){
                *perror = 5;
                *ppostfixStart = BAD_EXPRESSION; return(-1);
            }

          /* add operators of higher or equal priority to       */
          /* postfix notation                           */
            while ((pstacktop >= stack+1) &&
              (pstacktop->in_stack_pri >= pelement->in_coming_pri)) {
                *ppostfix++ = pstacktop->code;
                pstacktop--;
            }

          /* add new operator to stack */
            pstacktop++;
            *pstacktop = *pelement;

            new_expression = FALSE;
            break;

        case MINUS_OPERATOR:
            if (operand_needed){
              /* then assume minus was intended as a unary operator */
                in_coming_pri = UNARY_MINUS_I_C_P;
                in_stack_pri = UNARY_MINUS_I_S_P;
                code = UNARY_MINUS_CODE;
                new_expression = FALSE;
            }
            else {
              /* then assume minus was intended as a binary operator */
                in_coming_pri = BINARY_MINUS_I_C_P;
                in_stack_pri = BINARY_MINUS_I_S_P;
                code = BINARY_MINUS_CODE;
                operand_needed = TRUE;
            }

          /* add operators of higher or equal priority to       */
          /* postfix notation                           */
            while ((pstacktop >= stack+1) &&
              (pstacktop->in_stack_pri >= in_coming_pri)) {
                *ppostfix++ = pstacktop->code;
                pstacktop--;
            }

          /* add new operator to stack */
            pstacktop++;
            *pstacktop = *pelement;
            pstacktop->in_stack_pri = in_stack_pri;
            pstacktop->code = code;

            break;

        case SEPERATOR:
            if (operand_needed){
                *perror = 4;
                *ppostfixStart = BAD_EXPRESSION; return(-1);
            }

          /* add operators to postfix until open paren */
            while (pstacktop->element[0] != '('){
                if (pstacktop == stack+1 ||
                  pstacktop == stack){
                    *perror = 6;
                    *ppostfixStart = BAD_EXPRESSION; return(-1);
                }
                *ppostfix++ = pstacktop->code;
                pstacktop--;
            }
            operand_needed = TRUE;
            break;

        case CLOSE_PAREN:
            if (operand_needed){
                *perror = 4;
                *ppostfixStart = BAD_EXPRESSION; return(-1);
            }

          /* add operators to postfix until matching paren */
            while (pstacktop->element[0] != '('){
                if (pstacktop == stack+1 ||
                  pstacktop == stack){
                    *perror = 6;
                    *ppostfixStart = BAD_EXPRESSION; return(-1);
                }
                *ppostfix++ = pstacktop->code;
                pstacktop--;
            }
            pstacktop--;        /* remove ( from stack */
            break;

        case CONDITIONAL:
            if (operand_needed){
                *perror = 4;
                *ppostfixStart = BAD_EXPRESSION; return(-1);
            }

          /* add operators of higher priority to        */
          /* postfix notation                           */
            while ((pstacktop >= stack+1) &&
              (pstacktop->in_stack_pri > pelement->in_coming_pri)) {
                *ppostfix++ = pstacktop->code;
                pstacktop--;
            }

          /* add new element to the postfix expression */
            *ppostfix++ = pelement->code;

          /* add : operator with COND_END code to stack */
            if (pelement->element[0] == ':'){
                pstacktop++;
                *pstacktop = *pelement;
                pstacktop->code = COND_END;
            }

            operand_needed = TRUE;
            break;

        case EXPR_TERM:
            if (operand_needed && !new_expression){
                *perror = 4;
                *ppostfixStart = BAD_EXPRESSION; return(-1);
            }

          /* add all operators on stack to postfix */
            while (pstacktop >= stack+1){
                if (pstacktop->element[0] == '('){
                    *perror = 6;
                    *ppostfixStart = BAD_EXPRESSION; return(-1);
                }
                *ppostfix++ = pstacktop->code;
                pstacktop--;
            }

          /* add new element to the postfix expression */
            *ppostfix++ = pelement->code;

            operand_needed = TRUE;
            new_expression = TRUE;
            break;


        default:
            *perror = 8;
            *ppostfixStart = BAD_EXPRESSION; return(-1);
        }
    }
    if (operand_needed){
        *perror = 4;
        *ppostfixStart = BAD_EXPRESSION; return(-1);
    }

  /* add all operators on stack to postfix */
    while (pstacktop >= stack+1){
        if (pstacktop->element[0] == '('){
            *perror = 6;
            *ppostfixStart = BAD_EXPRESSION; return(-1);
        }
        *ppostfix++ = pstacktop->code;
        pstacktop--;
    }
    *ppostfix = END_STACK;

    return(0);
}
