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
 *     along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *     contact information:
 *         email: TouchStone222@runbox.com <David Lindauer>
 */

#include "PEObject.h"
#include "Utils.h"
#include "ObjFile.h"
#include "ObjSymbol.h"
#include "ObjExpression.h"

#include <string>
#include <ctype.h>
#include <time.h>
#include <map>
#include <iostream>
#include <string.h>

class namelt
{
    public:
        bool operator () (ObjExportSymbol *left, ObjExportSymbol *right)
        {
            return left->GetName() < right->GetName();
        }
} ;
void PEExportObject::Setup(ObjInt &endVa, ObjInt &endPhys)
{
    if (virtual_addr == 0)
    {
        virtual_addr = endVa;
    }
    else
    {
        if (virtual_addr != endVa)
            Utils::fatal("Internal error");
    }
    raw_addr = endPhys;
    std::map<std::string, ObjInt> publics;
    for (ObjFile::SymbolIterator it = file->PublicBegin(); it != file->PublicEnd(); ++it)
    {
        publics[(*it)->GetName()] = (*it)->GetOffset()->Eval(0);
    }


    int pos = moduleName.find_last_of("\\");
    if (pos == std::string::npos)
    {
        pos = moduleName.find_last_of(":");
        if (pos == std::string::npos)
            pos = 0;
    }
    std::string name = moduleName.substr(pos);

    size = initSize = sizeof(Header) + 1 + name.size();

    unsigned minOrd=0xffffffff; /* max ordinal num */
    unsigned maxOrd=0;
    unsigned count = 0;
    std::set<ObjExportSymbol *, namelt> names;
    for (ObjFile::SymbolIterator it = file->ExportBegin(); it != file->ExportEnd(); ++it)
    {
        ObjExportSymbol *s = (ObjExportSymbol *)(*it);
        names.insert(s);
        if (!s->GetByOrdinal())
        {
            initSize = (size += s->GetName().size() + 1 + 6);
        }
        if (s->GetOrdinal() != 0xffffffff)
        {
            int n = s->GetOrdinal();
            if (n < minOrd)
                minOrd = n;
            if (n > maxOrd)
                maxOrd = n;
        }
        count++;
    }				
    if (maxOrd >= minOrd)
    {
        count = count > (maxOrd-minOrd + 1) ? count : maxOrd-minOrd + 1;
    }
    else
    {
        minOrd = 1;
        maxOrd = count + minOrd - 1;
    }
    initSize = (size +=	4 * count);
    data = new unsigned char[initSize];
    memset(data, 0, initSize);
    Header *header = (Header *)data;

    header->time = time(0);
    header->ord_base = minOrd;
    header->n_eat_entries = count;
    header->n_name_ptrs = names.size();
    unsigned *rvaTable = (unsigned *)(data + sizeof(Header));
    unsigned *nameTable = (unsigned *)(((unsigned char *)rvaTable) + 4 * count);
    unsigned short *ordinalTable = (unsigned short *)(((unsigned char *)nameTable) + 4 * names.size());
    unsigned char *stringTable = (unsigned char *)(((unsigned char *)ordinalTable) + 2 * names.size());

    header->address_rva = virtual_addr + ((unsigned char *)rvaTable) - data;
    header->name_rva = virtual_addr + ((unsigned char *)nameTable) - data;
    header->ordinal_rva = virtual_addr + ((unsigned char *)ordinalTable) - data;

    /* process numbered exports */
    for (ObjFile::SymbolIterator it = file->ExportBegin(); it != file->ExportEnd(); ++it)
    {
        ObjExportSymbol *s = (ObjExportSymbol *)(*it);
        if (s->GetByOrdinal())
        {
            int n = s->GetOrdinal();
            if (rvaTable[n - minOrd] != 0)
                std::cout << "Warning: Export '" << s->GetDisplayName().c_str() << "' duplicates an ordinal" << std::endl;
            int addr = publics[s->GetName()];
            if (addr == 0)
                std::cout << "Warning: Export '" << s->GetDisplayName().c_str() << "' has no related public" << std::endl;
                
            rvaTable[n - minOrd] = addr - imageBase;
        }
    }			
                
    /* process non-numbered exports */
    pos = 0;
    for (ObjFile::SymbolIterator it = file->ExportBegin(); it != file->ExportEnd(); ++it)
    {
        ObjExportSymbol *s = (ObjExportSymbol *)(*it);
        if (!s->GetByOrdinal())
        {
            while(rvaTable[pos])
                pos++;
            s->SetOrdinal(pos + minOrd);
            int addr = publics[s->GetName()];
            if (addr == 0)
                std::cout << "Warning: Export '" << s->GetDisplayName().c_str() << "' has no related public" << std::endl;
            rvaTable[pos] = addr - imageBase;
        }
    }
    // process named exports
    for (auto name : names)
    {
        *nameTable++ = (unsigned)((unsigned char *)stringTable - data + virtual_addr);
        *ordinalTable++ = name->GetOrdinal() - minOrd;
        if (flat && name->GetName()[0] == '_')
        {
            strcpy((char *)stringTable, name->GetName().c_str()+1);
            stringTable += name->GetName().size()-1;
        }
        else
        {
            strcpy((char *)stringTable, name->GetName().c_str());
            stringTable += name->GetName().size();
        }
        *stringTable++ = 0;
    }
    // throw in the DLL name
    if (name.size())
    {
        header->exe_name_rva = (unsigned)((unsigned char *)stringTable - data + virtual_addr);
        for (int i=0; i < name.size(); i++)
            *stringTable++ = toupper(name[i]);
        *stringTable++ = 0;
    }
    endVa = ObjectAlign(objectAlign, endVa + size);
    endPhys = ObjectAlign(fileAlign, endPhys + initSize);
}
