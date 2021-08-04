#pragma once
/* Software License Agreement
 * 
 *     Copyright(C) 1994-2021 David Lindauer, (LADSoft)
 * 
 *     This file is part of the Orange C Compiler package.
 * 
 *     The Orange C Compiler package is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     The Orange C Compiler package is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with Orange C.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *     contact information:
 *         email: TouchStone222@runbox.com <David Lindauer>
 * 
 */

namespace Parser
{
extern int inConstantExpression;

long long MaxOut(enum e_bt size, long long value);
void dooper(EXPRESSION** node, int mode);
void addaside(EXPRESSION* node);
EXPRESSION* relptr(EXPRESSION* node, int& offset, bool add = true);
int opt0(EXPRESSION** node);
void enswap(EXPRESSION** one, EXPRESSION** two);
int fold_const(EXPRESSION* node);
int typedconsts(EXPRESSION* node1);
bool msilConstant(EXPRESSION *exp);
void RemoveSizeofOperators(EXPRESSION *constant);
void optimize_for_constants(EXPRESSION** expr);
LEXLIST* optimized_expression(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** expr, bool commaallowed);
}  // namespace Parser