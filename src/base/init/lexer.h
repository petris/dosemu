/*
 *  (C) Copyright 1992, ..., 2007 the "DOSEMU-Development-Team".
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * We are intercepting the yylex() function calls from the parser
 */
#ifndef LEXER_H
#define LEXER_H
#define OUR_YY_DECL int yylex (YYSTYPE* yylval)
OUR_YY_DECL;

extern void tell_lexer_if(int value);
extern void tell_lexer_loop(int cfile, int value);

#ifndef LEXER
extern void yyrestart(FILE *input_file);
extern FILE* yyin;
#endif

extern void yyerror(char *, ...);
extern void yywarn(char *, ...);
extern char *yy_vbuffer;
extern int include_stack_ptr;
extern char *include_fnames[];
extern int include_lines[];
extern int line_count;
extern int last_include;

#endif
