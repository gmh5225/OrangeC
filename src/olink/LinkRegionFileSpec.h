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

#ifndef LINKREGIONFILESPEC_H
#define LINKREGIONFILESPEC_H

#include "ObjTypes.h"
#include <vector>

class LinkRegionFileSpec
{
    public:
        enum eType { eStar, eQuestionMark, eSpan };
        LinkRegionFileSpec(eType Type, const ObjString &Span = "") : type(Type), span(Span) {}
        ~LinkRegionFileSpec() {}
        
        eType GetType() { return type; }
        ObjString &GetSpan() { return span; }
        
    private:
        enum eType type;
        ObjString span;
    
};
class LinkRegionFileSpecContainer
{
    public:
        LinkRegionFileSpecContainer(const ObjString &Spec);
        ~LinkRegionFileSpecContainer();
        bool Matches(const ObjString &Spec);
        
    private:
        std::vector<LinkRegionFileSpec *> specs;
};
#endif
