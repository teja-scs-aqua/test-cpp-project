//==============================
// Function parser v2.8 by Warp
//==============================
// =============================================================================
//   Usage license:
// =============================================================================
// 
// Copyright © 2003-2005 Juha Nieminen, Joel Yliluoma
// 
//   This library is distributed under two distinct usage licenses depending
// on the software ("Software" below) which uses the Function Parser library
// ("Library" below).
//   The reason for having two distinct usage licenses is to make the library
// compatible with the GPL license while still being usable in other non-GPL
// (even commercial) software. See fparser_license.txt for more details.

// Configuration file
// ------------------

// NOTE:
// This file is for the internal use of the function parser only.
// You don't need to include this file in your source files, just
// include "fparser.h".

/*
 Comment out the following line if your compiler supports the (non-standard)
 asinh, acosh and atanh functions and you want them to be supported. If
 you are not sure, just leave it (those function will then not be supported).
*/
#define NO_ASINH


/*
 Uncomment the following line to disable the eval() function if it could
 be too dangerous in the target application.
 Note that even though the maximum recursion level of eval() is limited,
 it is still possible to write functions using it which take enormous
 amounts of time to evaluate even though the maximum recursion is never
 reached. This may be undesirable in some applications.
*/
//#define DISABLE_EVAL


/*
 Maximum recursion level for eval() calls:
*/
#define EVAL_MAX_REC_LEVEL 1000


/*
 Comment out the following lines out if you are not going to use the
 optimizer and want a slightly smaller library. The Optimize() method
 can still be called, but it will not do anything.
 If you are unsure, just leave it. It won't slow down the other parts of
 the library.
*/
#ifndef NO_SUPPORT_OPTIMIZER
#define SUPPORT_OPTIMIZER
#endif


/*
 Epsilon value used with the comparison operators (must be non-negative):
 (Comment it out if you don't want to use epsilon in comparisons. Might
 lead to marginally faster evaluation of the comparison operators, but
 can introduce inaccuracies in comparisons.)
*/
#define FP_EPSILON 1e-14
