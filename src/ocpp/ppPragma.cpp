/* Software License Agreement
 *
 *     Copyright(C) 1994-2018 David Lindauer, (LADSoft)
 *
 *     This file is part of the Orange C Compiler package.
 *
 *     The Orange C Compiler package is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version, with the addition of the
 *     Orange C "Target Code" exception.
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

#define _CRT_SECURE_NO_WARNINGS

#include "ppPragma.h"
#include "PreProcessor.h"
#include "Errors.h"

Packing* Packing::instance;
FenvAccess* FenvAccess::instance;
CXLimitedRange* CXLimitedRange::instance;
FPContract* FPContract::instance;
Libraries* Libraries::instance;
Aliases* Aliases::instance;
Startups* Startups::instance;

void ppPragma::InitHash()
{
    hash["("] = openpa;
    hash[")"] = closepa;
    hash[","] = comma;
    hash["="] = eq;
}
bool ppPragma::Check(int token, const std::string& args)
{
    if (token == PRAGMA)
    {
        ParsePragma(args);
        return true;
    }
    return false;
}
void ppPragma::ParsePragma(const std::string& args)
{
    Tokenizer tk(args, &hash);
    const Token* id = tk.Next();
    if (id->IsIdentifier())
    {
        if (*id == "STDC")
            HandleSTDC(tk);
        else if (*id == "AUX")
            HandleAlias(tk);
        else if (*id == "PACK")
            HandlePack(tk);
        else if (*id == "LIBRARY")
            HandleLibrary(tk);
        else if (*id == "STARTUP")
            HandleStartup(tk);
        else if (*id == "RUNDOWN")
            HandleRundown(tk);
        else if (*id == "WARNING")
            HandleWarning(tk);
        else if (*id == "ERROR")
            HandleError(tk);
        else if (*id == "FARKEYWORD")
            HandleFar(tk);
        // unmatched is not an error
    }
}
void ppPragma::HandleSTDC(Tokenizer& tk)
{
    const Token* token = tk.Next();
    if (token && token->IsIdentifier())
    {
        std::string name = token->GetId();
        const Token* tokenCmd = tk.Next();
        if (tokenCmd && tokenCmd->IsIdentifier())
        {
            const char* val = tokenCmd->GetId().c_str();
            bool on;
            bool valid = false;
            if (!strcmp(val, "ON"))
            {
                valid = true;
                on = true;
            }
            else if (!strcmp(val, "OFF"))
            {
                valid = true;
                on = false;
            }
            if (valid)
            {
                if (name == "FENV_ACCESS")
                    FenvAccess::Instance()->Add(on);
                else if (name == "CX_LIMITED_RANGE")
                    CXLimitedRange::Instance()->Add(on);
                else if (name == "FP_CONTRACT")
                    FPContract::Instance()->Add(on);
            }
        }
    }
}
void ppPragma::HandlePack(Tokenizer& tk)
{
    const Token* tok = tk.Next();
    if (tok && tok->GetKeyword() == openpa)
    {
        tok = tk.Next();
        int val = -1;
        if (tok)
            if (tok->IsNumeric())
            {
                val = tok->GetInteger();
            }
        tok = tk.Next();
        if (tok)
            if (tok->GetKeyword() == closepa)
            {
                if (val <= 0)
                {
                    Packing::Instance()->Remove();
                }
                else
                {
                    Packing::Instance()->Add(val);
                }
            }
    }
}
void ppPragma::HandleError(Tokenizer& tk) { Errors::Error(tk.GetString()); }
void ppPragma::HandleWarning(Tokenizer& tk)
{
    // check for microsoft warning pragma
    const char* p = tk.GetString().c_str();
    while (isspace(*p))
        p++;
    if (*p != '(')
        Errors::Warning(p);
}
void ppPragma::HandleSR(Tokenizer& tk, bool startup)
{
    const Token* name = tk.Next();
    if (name && name->IsIdentifier())
    {
        std::string id = name->GetId();
        const Token* prio = tk.Next();
        if (prio && prio->IsNumeric() && !prio->IsFloating())
        {
            Startups::Instance()->Add(id, prio->GetInteger(), startup);
        }
    }
}
Startups::~Startups()
{
    for (StartupIterator it = begin(); it != end(); ++it)
    {
        Properties* x = it->second;
        delete x;
    }
    list.clear();
}
void ppPragma::HandleLibrary(Tokenizer& tk)
{
    char buf[260 + 10];
    char* p = buf;
    char* q = p;
    strcpy(buf, tk.GetString().c_str());
    while (isspace(*p))
        p++;
    if (*p == '(')
    {
        do
        {
            p++;
        } while (isspace(*p));
        q = strchr(p, ')');
        if (q)
        {
            while (q != p && isspace(q[-1]))
                q--;
            *q = 0;
            if (*p)
                Libraries::Instance()->Add(p);
        }
    }
}
void ppPragma::HandleAlias(Tokenizer& tk)
{
    const Token* name = tk.Next();
    if (name && name->IsIdentifier())
    {
        std::string id = name->GetId();
        const Token* alias = tk.Next();
        if (alias && alias->GetKeyword() == eq)
        {
            alias = tk.Next();
            if (alias && alias->IsIdentifier())
            {
                Aliases::Instance()->Add(id, alias->GetId());
            }
        }
    }
}
void ppPragma::HandleFar(Tokenizer& tk)
{
    // fixme
}
