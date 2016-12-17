/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     COMMENT = 258,
     NAME = 259,
     COLUMN_NAME = 260,
     CREATE = 261,
     TABLE = 262,
     INT = 263,
     STR20 = 264,
     INSERT = 265,
     INTO = 266,
     VALUES = 267,
     LITERAL = 268,
     INTEGER = 269,
     NULL_VALUE = 270,
     SELECT = 271,
     DISTINCT = 272,
     FROM = 273,
     WHERE = 274,
     ORDER = 275,
     BY = 276,
     DELETE = 277,
     DROP = 278,
     OR = 279,
     AND = 280,
     NOT = 281,
     COMP_OP = 282,
     STATEMENT = 283,
     CREATE_TABLE_STATEMENT = 284,
     DROP_TABLE_STATEMENT = 285,
     SELECT_STATEMENT = 286,
     DELETE_STATEMENT = 287,
     INSERT_STATEMENT = 288,
     ATTRIBUTE_TYPE_LIST = 289,
     INSERT_TUPLES = 290,
     NAME_TYPE = 291,
     NAME_LIST = 292,
     VALUE_LIST = 293,
     SELECT_LIST = 294,
     ORDER_OPTION = 295,
     WHERE_OPTION = 296,
     COMPARISON_PREDICATE = 297,
     SEARCH_CONDITION = 298
   };
#endif
/* Tokens.  */
#define COMMENT 258
#define NAME 259
#define COLUMN_NAME 260
#define CREATE 261
#define TABLE 262
#define INT 263
#define STR20 264
#define INSERT 265
#define INTO 266
#define VALUES 267
#define LITERAL 268
#define INTEGER 269
#define NULL_VALUE 270
#define SELECT 271
#define DISTINCT 272
#define FROM 273
#define WHERE 274
#define ORDER 275
#define BY 276
#define DELETE 277
#define DROP 278
#define OR 279
#define AND 280
#define NOT 281
#define COMP_OP 282
#define STATEMENT 283
#define CREATE_TABLE_STATEMENT 284
#define DROP_TABLE_STATEMENT 285
#define SELECT_STATEMENT 286
#define DELETE_STATEMENT 287
#define INSERT_STATEMENT 288
#define ATTRIBUTE_TYPE_LIST 289
#define INSERT_TUPLES 290
#define NAME_TYPE 291
#define NAME_LIST 292
#define VALUE_LIST 293
#define SELECT_LIST 294
#define ORDER_OPTION 295
#define WHERE_OPTION 296
#define COMPARISON_PREDICATE 297
#define SEARCH_CONDITION 298




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

