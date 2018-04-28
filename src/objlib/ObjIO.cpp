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

#include "objio.h"

/*
ObjIOBase::ObjIOBase(ObjString  Name) : name(Name),
    translatorName(""), bitsPerMAU(8), MAUS(1), bigEndian(false), debugInfo(false)
     { ObjIOContainer::Instance()->Add(this); }
ObjIOBase::~ObjIOBase() { ObjIOContainer::Instance()->Remove(this);}
*/

void ObjIOContainer::Add(ObjIOBase *ObjIO)
{
    if (ObjIO)
        ioModules.push_back(ObjIO);
}
void ObjIOContainer::Remove(ObjIOBase *ObjIO)
{
    for (IOBaseIterator it = IOBaseBegin(); it != IOBaseEnd(); ++it)
    {
        if ((*it)->GetName() == ObjIO->GetName())
        {
            ioModules.erase(it);
            break;
        }
    }
}
