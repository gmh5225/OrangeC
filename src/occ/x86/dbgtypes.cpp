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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "be.h"

#include "dbgtypes.h"
#include "ObjFactory.h"
#include "ObjType.h"
#include "ObjFile.h"

extern "C" TYPE stdvoid;

#define DEBUG_VERSION 4.0

bool dbgtypes::typecompare::operator () (const TYPE *left, const TYPE *right) const
{
    left = basetype(left);
    right = basetype(right);
    if (left->type < right->type)
        return true;
    else if (left->type == right->type)
    {
        if (left->type == bt_func || left->type == bt_ifunc)
        {
            if (operator()(left->btp, right->btp))
                return true;
            if (left->syms->table && right->syms->table)
            {
                const HASHREC *hr1 = left->syms->table[0];
                const HASHREC *hr2 = right->syms->table[0];
                while (hr1 && hr2)
                {
                    SYMBOL *sym1 = (SYMBOL *)hr1->p;
                    SYMBOL *sym2 = (SYMBOL *)hr2->p;
                    if (operator()(sym1->tp, sym2->tp))
                        return true;
                    else if (operator()(sym2->tp, (sym1->tp)))
                            return false;
                    hr1 = hr1->next;
                    hr2 = hr2->next;
                }
                if (hr2)
                    return true;
            }
        }
        else if (left->type == bt_class || left->type == bt_struct || left->type == bt_union || left->type == bt_enum)
        {
            if (strcmp(left->sp->name, right->sp->name) < 0)
                return true;
        }
        else if (left->type == bt_pointer)
        {
            if (left->array && !right->array)
                return true;
            if (operator()(left->btp, right->btp))
                return true;
        }
    }
    return false;
}

ObjType *dbgtypes::Put(TYPE *tp)
{
    auto val = Lookup(tp);
    if (val)
        return val;
    if (tp->type == bt_any)
    {
        val = factory.MakeType((ObjType::eType)42);
    }
    else if (tp->type == bt_typedef)
    {
        if (!tp->sp->templateLevel)
        {
            val = Put(tp->btp);

            val = factory.MakeType(ObjType::eTypeDef, val);
        }
    }
    else
    {
        if (!tp->bits)
            val = BasicType(tp);
        if (val == nullptr)
        {
            val = ExtendedType(tp);
        }
    }
    fi->Add(val);

    hash[tp] = val;
    return val;

}
void dbgtypes::OutputTypedef(SYMBOL *sp)
{
    Put(sp->tp);
}
ObjType * dbgtypes::Lookup(TYPE *tp)
{

    auto it = hash.find(tp);
    if (it != hash.end())
        return it->second;
    return nullptr;
}
ObjType *dbgtypes::BasicType(TYPE* tp)
{
    static int basicTypes[] = { 35, 34, 40, 40, 48, 41, 46, 49, 45, 0,  42, 42, 47, 50, 50,
        43, 51, 44, 52, 72, 73, 74, 80, 81, 82, 88, 89, 90, 32 };
    static int pointedTypes[] = { 0,       0,       40 + 16, 40 + 16, 48 + 16, 41 + 16, 46 + 16, 49 + 16, 45 + 16, 0,
        42 + 16, 42 + 16, 47 + 16, 50 + 16, 50 + 16, 43 + 16, 51 + 16, 44 + 16, 52 + 16, 72 + 16,
        73 + 16, 74 + 16, 80 + 16, 81 + 16, 82 + 16, 88 + 16, 89 + 16, 90 + 16, 33 };
    int n = 0;
    TYPE* tp1 = basetype(tp);
    if (tp1->type <= bt_void)
    {
        n = basicTypes[tp1->type];
    }
    else if (tp1->type == bt_pointer && !tp1->array && !tp1->vla && !tp1->bits)
    {
        TYPE* tp2 = basetype(tp1->btp);
        if (tp2->type <= bt_void)
        {
            n = pointedTypes[tp2->type];
        }
    }
    if (n)
        return factory.MakeType((ObjType::eType)n);
    return nullptr;

}
ObjType * dbgtypes::TypeName(ObjType *val, char* nm)
{
    if (nm[0] == '_')
        nm++;
    return factory.MakeType(nm, ObjType::eNone, val);
}
void dbgtypes::StructFields(ObjType::eType sel, ObjType *val, int sz, SYMBOL* parent, HASHREC* hr)
{
    if (parent->baseClasses)
    {
        BASECLASS *bc = parent->baseClasses;
        while (bc)
        {
            SYMBOL* sp = (SYMBOL*)bc->cls;
            // we are setting sp->offset here for use later in this function
            if (bc->isvirtual)
            {
                VBASEENTRY* vbase = parent->vbaseEntries;
                while (vbase)
                {
                    if (vbase->cls == bc->cls || sameTemplate(vbase->cls->tp, bc->cls->tp))
                    {
                        sp->offset = vbase->pointerOffset;
                        break;
                    }
                    vbase = vbase->next;
                }
                TYPE *tpl = (TYPE *)Alloc(sizeof(TYPE));
                tpl->type = bt_pointer;
                tpl->size = getSize(bt_pointer);
                tpl->btp = sp->tp;
                ObjType *base = Put(tpl);
                ObjField *field = factory.MakeField(sp->name, base, -1, 0);
                val->Add(field);
            }
            else
            {
                ObjType *base = Put(sp->tp);
                ObjField *field = factory.MakeField(sp->name, base, bc->offset, 0);
                val->Add(field);
            }
            bc = bc->next;
        }
    }
    while (hr)
    {
        SYMBOL* sp = (SYMBOL*)hr->p;
        if (!istype(sp) && sp->tp->type != bt_aggregate)
        {
            ObjType *base = Put(sp->tp);
            ObjField *field = factory.MakeField(sp->name, base, sp->offset, 0);
            val->Add(field);
        }
        hr = hr->next;
    }

}
void dbgtypes::EnumFields(ObjType *val, ObjType* base, int sz, HASHREC* hr)
{
    while (hr)
    {
        SYMBOL *sym = (SYMBOL *)hr;
        ObjField *field = factory.MakeField(sym->name, base, sym->value.i,0);
        val->SetConstVal(sym->value.i);
        val->Add(field);
        hr = hr->next;
    }
}
ObjType *dbgtypes::Function(TYPE* tp)
{
    ObjFunction *val = nullptr;
    ObjType *rv = Put(basetype(tp)->btp);
    int v = 0;
    switch (basetype(tp)->sp->storage_class)
    {
    case sc_virtual:
    case sc_member:
    case sc_mutable:
        v = 4;  // has a this pointer
        break;
    default:
        switch (basetype(tp)->sp->linkage)
        {
        default:
        case lk_cdecl:
            v = 1;
            break;
        case lk_stdcall:
            v = 2;
            break;
        case lk_pascal:
            v = 3;
            break;
        case lk_fastcall:
            v = 4;
            break;
        }
        break;
    }
    if (isstructured(basetype(tp)->btp))
        v |= 32;  // structured return value
    val = factory.MakeFunction("", rv);
    val->SetLinkage((ObjFunction::eLinkage)v);
    HASHREC *hr = basetype(tp)->syms->table[0];
    while (hr)
    {
        SYMBOL* s = (SYMBOL*)hr->p;
        val->Add(Put(s->tp));
        hr = hr->next;
    }
    return val;

}
ObjType * dbgtypes::ExtendedType(TYPE *tp)
{
    tp = basetype(tp);
    ObjType *val = nullptr;
    if (tp->type == bt_pointer)
    {
        val = Put(tp->btp);

        if (tp->vla)
        {
            val = factory.MakeType(ObjType::eVla, val);
        }
        else if (tp->array)
        {
            val = factory.MakeType(ObjType::eArray, val);
            val->SetSize(tp->size);
            val->SetTop(tp->size / basetype(tp)->btp->size);
        }
        else
        {
            val = factory.MakeType(ObjType::ePointer, val);
            val->SetSize(tp->size);
        }
    }
    else if (tp->type == bt_lref)
    {
        val = Put(tp->btp);
        val = factory.MakeType(ObjType::eLRef, val);
    }
    else if (tp->type == bt_rref)
    {
        val = Put(tp->btp);
        val = factory.MakeType(ObjType::eRRef, val);
    }
    else if (isfunction(tp))
    {
        val = Function(tp);
    }
    else
    {
        if (tp->hasbits)
        {
            val = Lookup(tp);
            if (!val)
                val = BasicType(tp);
            val = factory.MakeType(ObjType::eBitField, val);
            val->SetSize(tp->size);
            val->SetStartBit(tp->startbit);
            val->SetBitCount(tp->bits);
        }
        else if (isstructured(tp))
        {
            ObjType::eType sel;
            tp = basetype(tp)->sp->tp;  // find instantiated version in case of C++ struct
            if (tp->type == bt_union)
            {
                sel = ObjType::eUnion;
            }
            else
            {
                sel = ObjType::eStruct;
            }
            val = factory.MakeType(sel);
            if (tp->syms)
                StructFields(sel, val, tp->size, tp->sp, tp->syms->table[0]);
            else
                StructFields(sel, val, tp->size, tp->sp, NULL);
            val = TypeName(val, tp->sp->decoratedName);
        }
        else if (tp->type == bt_ellipse)
        {
            // ellipse results in no debug info
        }
        else if (tp->type == bt_templateselector)
        {
            val = factory.MakeType((ObjType::eType)42);
        }
        else  // enum
        {
            ObjType *base;
            if (tp->type == bt_enum)
                if (tp->btp)
                    base = Put(tp->btp);
                else
                    base = Put(&stdvoid);
            else
                base = Put(tp);

            val = factory.MakeType(ObjType::eEnum);
            if (tp->syms)
                EnumFields(val, base, tp->size, tp->syms->table[0]);
            else
                EnumFields(val, base, tp->size, NULL);
            val = TypeName(val, tp->sp->decoratedName);
       }
    }
    return val;
}

