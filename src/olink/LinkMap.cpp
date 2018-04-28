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

#include "LinkManager.h"
#include "LinkMap.h"
#include "LinkAttribs.h"
#include "LinkPartition.h"
#include "LinkOverlay.h"
#include "LinkRegion.h"
#include "ObjExpression.h"
#include "ObjFile.h"
#include "ObjSection.h"
#include "Utils.h"
#include <iomanip>
#include <set>
#include <iostream>
#include <fstream>

std::fstream &LinkMap::Address(std::fstream &stream, ObjInt base, ObjInt offset, int group)
{
    switch (mode)
    {
        default:
        case eLinear:
            stream << std::setw(6) << std::setfill('0') << std::hex << base + offset;
            break;
        case eSeg32:
            stream << std::setw(2) << std::setfill('0') << std::hex << group << ":" << std::setw(6) << std::setfill('0') << std::hex << offset ;
            break;
        case eSeg16:
            stream << std::setw(4) << std::setfill('0') << std::hex << (base >> 4) << ":" << std::setw(4) << std::setfill('0') << std::hex << (offset + (base & 15));
            break;
    }
    return stream;
}
ObjInt LinkMap::PublicBase(ObjExpression *exp, int &group)
{	
    ObjExpression *find = exp;
    while (find->GetOperator() == ObjExpression::eAdd || find->GetOperator() == ObjExpression:: eSub || find->GetOperator() == ObjExpression::eDiv)
    {
        find = find->GetLeft();
    }
    if (find->GetOperator() != ObjExpression::eSection)
        Utils::fatal("Invalid fixup");
    group = find->GetSection()->GetIndex() + 1;
    find->GetSection()->SetOffset(new ObjExpression(0)); // this wrecks the link but is done last so it is ok
    return overlays[group-1]->GetAttribs().GetAddress();
}
void LinkMap::ShowAttribs(std::fstream &stream, LinkAttribs &attribs, ObjInt offs, int group)
{
    if (group > 0)
    {
        stream << " addr=" ;
        Address(stream, offs, attribs.GetAddress(), group);
    }
    else
        stream << " addr=" << std::setw(6) << std::setfill('0') << std::hex << attribs.GetAddress();
    if (attribs.GetVirtualOffsetSpecified())
        stream << "(" << std::setw(6) << std::setfill('0') << std::hex << attribs.GetVirtualOffset() << ") ";
    stream << " align=" << std::setw(3) << std::setfill('0') << std::hex << attribs.GetAlign();
    stream << " size=" << std::setw(4) << std::setfill('0') << std::hex << attribs.GetSize();
    stream << " max=" << std::setw(4) << std::setfill('0') << std::hex << attribs.GetMaxSize();
    stream << " round=" << std::setw(3) << std::setfill('0') << std::hex << attribs.GetRoundSize();
    stream << " fill=" << std::setw(2) << std::setfill('0') << std::hex << attribs.GetFill();
    stream << std::endl;
}
void LinkMap::ShowPartitionLine(std::fstream &stream, LinkPartition *partition)
{
    if (partition)
    {
        LinkAttribs &attribs = partition->GetAttribs();
        stream << "Partition: " << partition->GetName().c_str();
        ShowAttribs(stream, attribs, 0, -1);
    }
}
void LinkMap::ShowOverlayLine(std::fstream &stream, LinkOverlay *overlay)
{
    if (overlay)
    {
        LinkAttribs &attribs = overlay->GetAttribs();
        stream << "  Overlay: " << overlay->GetName().c_str();
        ShowAttribs(stream, attribs, 0, -1);
    }
}
void LinkMap::ShowRegionLine(std::fstream &stream, LinkRegion *region, ObjInt offs, int group)
{
    if (region)
    {
        LinkAttribs &attribs = region->GetAttribs();
        stream << "    Region: " << region->GetName().c_str();
        ShowAttribs(stream, attribs, offs, group );
    }
}
void LinkMap::ShowFileLine(std::fstream &stream, LinkRegion::OneSection *data, ObjInt n)
{
    stream << "      File: " << data->file->GetName().c_str() << "(" << data->section->GetName().c_str() << ") " ;
    stream << "addr=" << std::setw(6) << std::setfill('0') << std::hex << data->section->GetOffset()->Eval(0) /*+ n*/ << " " ;
    stream << "size=" << std::setw(4) << std::setfill('0') << std::hex << data->section->GetSize()->Eval(0) << " " ;
    stream << std::endl;
}
void LinkMap::ShowSymbol(std::fstream &stream, const MapSymbolData &symbol)
{
    Address(stream, symbol.base, symbol.abs - symbol.base, symbol.group);
    if (symbol.sym->GetUsed())
        stream << "   ";
    else
        stream << " X ";
    stream << symbol.sym->GetSymbol()->GetDisplayName().c_str() << std::setw(1) << std::endl;
}
void LinkMap::NormalSections(std::fstream &stream)
{
    stream << "Partition Map" << std::endl << std::endl;
    for (LinkManager::PartitionIterator it = manager->PartitionBegin(); 
             it != manager->PartitionEnd(); ++it)
    {
        ShowPartitionLine(stream, (*it)->GetPartition());
        if ((*it)->GetPartition())
        {
            for (LinkPartition::OverlayIterator itc = (*it)->GetPartition()->OverlayBegin();
                 itc != (*it)->GetPartition()->OverlayEnd(); ++itc)
            {
                if ((*itc)->GetOverlay())
                {
                    overlays.push_back((*itc)->GetOverlay());
                }
            }
        }
    }
}
void LinkMap::DetailedSections(std::fstream &stream)
{
    stream << "Detailed Section Map" << std::endl << std::endl;
    int group = 1;
    for (LinkManager::PartitionIterator it = manager->PartitionBegin(); 
             it != manager->PartitionEnd(); ++it)
    {
        if ((*it)->GetPartition())
        {
            ShowPartitionLine(stream, (*it)->GetPartition());
            for (LinkPartition::OverlayIterator itc = (*it)->GetPartition()->OverlayBegin();
                 itc != (*it)->GetPartition()->OverlayEnd(); ++itc)
            {
                if ((*itc)->GetOverlay())
                {
                    overlays.push_back((*itc)->GetOverlay());
                    ShowOverlayLine(stream, (*itc)->GetOverlay());
                    for (LinkOverlay::RegionIterator itr = (*itc)->GetOverlay()->RegionBegin();
                             itr != (*itc)->GetOverlay()->RegionEnd(); ++itr)
                    {
                        if ((*itr)->GetRegion())
                        {
                            int n = (*itc)->GetOverlay()->GetAttribs().GetAddress();
                            ShowRegionLine(stream, (*itr)->GetRegion(), n, group);
                            for (LinkRegion::SectionDataIterator it = (*itr)->GetRegion()->NowDataBegin();
                                 it != (*itr)->GetRegion()->NowDataEnd(); ++ it)
                            {
                                for (auto sect : (*it)->sections)
                                {
                                    ShowFileLine (stream, &sect, n);
                                }
                            }
                            for (LinkRegion::SectionDataIterator it = (*itr)->GetRegion()->NormalDataBegin();
                                 it != (*itr)->GetRegion()->NormalDataEnd(); ++ it)
                            {
                                for (auto sect : (*it)->sections)
                                {
                                    ShowFileLine(stream, &sect, n);
                                }
                            }
                            for (LinkRegion::SectionDataIterator it = (*itr)->GetRegion()->PostponeDataBegin();
                                 it != (*itr)->GetRegion()->PostponeDataEnd(); ++ it)
                            {
                                for (auto sect : (*it)->sections)
                                {
                                    ShowFileLine(stream, &sect, n);
                                }
                            }
                        }
                    }
                }
            }
            group++;
        }
    }
}
void LinkMap::Publics(std::fstream &stream)
{
    std::set<MapSymbolData, linkltcomparebyname> byName;
    std::set<MapSymbolData, linkltcomparebyvalue> byValue;
    for (LinkManager::SymbolIterator it = manager->PublicBegin(); it != manager->PublicEnd(); it++)
    {
        int group;
        ObjInt base = PublicBase((*it)->GetSymbol()->GetOffset(), group);
        int ofs = (*it)->GetSymbol()->GetOffset()->Eval(0);
        byName.insert(MapSymbolData((*it), ofs+base, base, group));
        byValue.insert(MapSymbolData((*it), ofs+base, base, group));
    }
    stream << std::endl << "Publics By Name" << std::endl << std::endl;
    for (auto sym : byName)
        ShowSymbol(stream, sym);
    stream << std::endl << "Publics By Value" << std::endl << std::endl;
    for (auto sym : byValue)
        ShowSymbol(stream, sym);
}
void LinkMap::WriteMap()
{
    std::fstream stream(name.c_str(), std::ios::out | std::ios::trunc);
    if (!stream.fail())
    {
        if (type == eDetailed)
            DetailedSections(stream);
        else
            NormalSections(stream);
        if (type != eNormal)
        {
            Publics(stream);
        }
        stream.close();
    }
}
