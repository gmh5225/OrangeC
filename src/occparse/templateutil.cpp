/* Software License Agreement
 * 
 *     Copyright(C) 1994-2023 David Lindauer, (LADSoft)
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

#include "compiler.h"
#include "assert.h"
#include <stack>
#include "PreProcessor.h"
#include <malloc.h>
#include "ccerr.h"
#include "initbackend.h"
#include "declare.h"
#include "declcpp.h"
#include "stmt.h"
#include "expr.h"
#include "lambda.h"
#include "occparse.h"
#include "help.h"
#include "cpplookup.h"
#include "mangle.h"
#include "lex.h"
#include "constopt.h"
#include "memory.h"
#include "init.h"
#include "rtti.h"
#include "declcons.h"
#include "exprcpp.h"
#include "inline.h"
#include "beinterf.h"
#include "types.h"
#include "templatedecl.h"
#include "templateutil.h"
#include "templateinst.h"
#include "templatededuce.h"
#include "libcxx.h"
#include "constexpr.h"
#include "symtab.h"
#include "ListFactory.h"
namespace Parser
{
EXPRESSION* GetSymRef(EXPRESSION* n)
{
    EXPRESSION* rv = nullptr;
    std::stack<EXPRESSION*> st;
    st.push(n);
    while (!st.empty())
    {
        EXPRESSION* exp = st.top();
        st.pop();
        switch (exp->type)
        {
            case ExpressionNode::labcon_:
            case ExpressionNode::global_:
            case ExpressionNode::auto_:
            case ExpressionNode::absolute_:
            case ExpressionNode::pc_:
            case ExpressionNode::threadlocal_:
                return exp;
            default:
                if (!isintconst(exp) && !isfloatconst(exp))
                {
                    if (exp->right)
                    {
                        st.push(exp->right);
                    }
                    if (exp->left)
                    {
                        st.push(exp->left);
                    }
                }
                break;
        }
    }
    return rv;
}
bool equalTemplateIntNode(EXPRESSION* exp1, EXPRESSION* exp2)
{
    if (exp1->type == ExpressionNode::templateparam_)
        exp1 = exp1->v.sp->tp->templateParam->second->byNonType.val;
    if (exp2->type == ExpressionNode::templateparam_)
        exp2 = exp2->v.sp->tp->templateParam->second->byNonType.val;
    if (exp1 && exp2)
    {
        if (equalnode(exp1, exp2))
            return true;
        if (isintconst(exp1) && isintconst(exp2) && exp1->v.i == exp2->v.i)
            return true;
    }
    if (!exp1 && !exp2)
        return true;
    return false;
}
bool templatecompareexpressions(EXPRESSION* exp1, EXPRESSION* exp2)
{
    if (isintconst(exp1) && isintconst(exp2))
        return exp1->v.i == exp2->v.i;
    if (exp1->type != exp2->type)
        return false;
    switch (exp1->type)
    {
        case ExpressionNode::global_:
        case ExpressionNode::auto_:
        case ExpressionNode::labcon_:
        case ExpressionNode::absolute_:
        case ExpressionNode::pc_:
        case ExpressionNode::const_:
        case ExpressionNode::threadlocal_:
            return comparetypes(exp1->v.sp->tp, exp2->v.sp->tp, true) || sameTemplate(exp1->v.sp->tp, exp1->v.sp->tp);
        case ExpressionNode::func_: {
            TYPE* tp1 = basetype(exp1->v.sp->tp);
            TYPE* tp2 = basetype(exp2->v.sp->tp);
            if (isfunction(tp1) || isfunction(tp2))
            {
                tp1 = tp1->btp;
                tp2 = tp2->btp;
            }
            else if (tp1->type == BasicType::aggregate_ || tp2->type == BasicType::aggregate_)
            {
                return true;
            }
            else if (tp1->type != tp2->type)
            {
                return false;
            }
            if ((basetype(tp1)->type == BasicType::templateparam_ && tp2->type == BasicType::int_) ||
                (basetype(tp2)->type == BasicType::templateparam_ && tp1->type == BasicType::int_))  // undefined
                return true;
            return comparetypes(tp1, tp2, false) || sameTemplate(tp1, tp2);
        }
        case ExpressionNode::templateselector_:
            return templateselectorcompare(exp1->v.templateSelector, exp2->v.templateSelector);
        default:
            break;
    }
    if (exp1->left && exp2->left)
        if (!templatecompareexpressions(exp1->left, exp2->left))
            return false;
    if (exp1->right && exp2->right)
        if (!templatecompareexpressions(exp1->right, exp2->right))
            return false;
    return true;
}
bool templateselectorcompare(std::vector<TEMPLATESELECTOR>* tsin1, std::vector<TEMPLATESELECTOR>* tsin2)
{
    auto& ts1 = (*tsin1)[1];
    auto& ts2 = (*tsin2)[1];
    if (ts1.isTemplate != ts2.isTemplate || ts1.sp != ts2.sp)
        return false;
    auto tss1 = (*tsin1).begin();
    auto tss2 = (*tsin2).begin();
    ++tss1;
    ++tss1;
    ++tss2;
    ++tss2;
    for (; tss1 != (*tsin1).end() && tss2 != (*tsin2).end(); ++tss1, ++tss2)
        if (strcmp(tss1->name, tss2->name))
            return false;
    if (tss1 != (*tsin1).end() || tss2 != (*tsin2).end())
        return false;
    if (ts1.isTemplate)
    {
        if (!exactMatchOnTemplateParams(ts1.templateParams, ts2.templateParams))
            return false;
    }
    return true;
}
bool templatecomparetypes(TYPE* tp1, TYPE* tp2, bool exact, bool sameType)
{
    if (!tp1 || !tp2)
        return false;
    if (basetype(tp1)->type == BasicType::templateselector_ && basetype(tp2)->type == BasicType::templateselector_)
    {
        auto left = basetype(tp1)->sp->sb->templateSelector;
        auto right = basetype(tp2)->sp->sb->templateSelector;
        if ((*left)[1].isDeclType ^ (*right)[1].isDeclType)
            return false;
        auto tss1 = (*left).begin();
        auto tss2 = (*right).begin();
        ++tss1;
        ++tss1;
        ++tss2;
        ++tss2;
        for (; tss1 != (*left).end() && tss2 != (*right).end(); ++tss1, ++tss2)
            if (strcmp(tss1->name, tss2->name))
                return false;
        return tss1 == (*left).end() && tss2 == (*right).end();
    }
    else
    {
        if (basetype(tp1)->type == BasicType::templateselector_ || basetype(tp2)->type == BasicType::templateselector_)
            return true;
    }
    if (sameType && (isref(tp1) != isref(tp2) || (isref(tp1) && basetype(tp1)->type != basetype(tp2)->type)))
        return false;
    if (!comparetypes(tp1, tp2, exact) && (!sameType || !sameTemplate(tp1, tp2)))
        return false;
    if (isint(tp1) && basetype(tp1)->btp && basetype(tp1)->btp->type == BasicType::enum_)
        tp1 = basetype(tp1)->btp;
    if (isint(tp2) && basetype(tp2)->btp && basetype(tp2)->btp->type == BasicType::enum_)
        tp2 = basetype(tp2)->btp;
    if (basetype(tp1)->type != basetype(tp2)->type)
        if (isref(tp1) || !isref(tp2))
            return false;
    if (basetype(tp1)->type == BasicType::enum_)
    {
        if (basetype(tp1)->sp != basetype(tp2)->sp)
            return false;
    }
    return true;
}
bool exactMatchOnTemplateParams(std::list<TEMPLATEPARAMPAIR>* old, std::list<TEMPLATEPARAMPAIR>* sym)
{
    if (old && sym)
    {
        auto ito = old->begin();
        auto itoe = old->end();
        auto its = sym->begin();
        auto itse = sym->end();
        if (ito->second->type == TplType::new_)
            ++ito;
        if (its->second->type == TplType::new_)
            ++its;
        for (; ito != itoe && its != itse; ++ito, ++its)
        {
            if (ito->second->type != its->second->type)
                break;
            if (its->second->packed)
            {
                if (ito->second->packed)
                {
                    if (ito->second->byPack.pack)
                        if (!exactMatchOnTemplateParams(ito->second->byPack.pack, its->second->byPack.pack))
                            return false;
                }
                else if (its->second->byPack.pack)
                {
                    for (; ito != itoe; ++ito)
                        if (ito->second->type != its->second->type)
                            return false;
                    return ++its == itse;
                }
                else
                {
                    return ++its == itse;
                }
            }
            else if (ito->second->type == TplType::template_)
            {
                if (!exactMatchOnTemplateParams(ito->second->byTemplate.args, its->second->byTemplate.args))
                    break;
            }
            else if (ito->second->type == TplType::int_)
            {
                if (!templatecomparetypes(ito->second->byNonType.tp, its->second->byNonType.tp, true))
                    if (ito->second->byNonType.tp->type != BasicType::templateparam_ &&
                        its->second->byNonType.tp->type != BasicType::templateparam_)
                        break;
                if (ito->second->byNonType.dflt && its->second->byNonType.dflt &&
                    !templatecompareexpressions(ito->second->byNonType.dflt, its->second->byNonType.dflt))
                    break;
            }
        }
        if (ito != itoe && ito->second->packed)
            ito = itoe;
        return ito == itoe && its == itse;
    }
    return !old && !sym;
}
bool exactMatchOnTemplateArgs(std::list<TEMPLATEPARAMPAIR>* old, std::list<TEMPLATEPARAMPAIR>* sym)
{
    if (old && sym)
    {
        auto ito = old->begin();
        auto itoe = old->end();
        auto its = sym->begin();
        auto itse = sym->end();
        if (ito != itoe && ito->second->type == TplType::new_)
            ++ito;
        if (its != itse && its->second->type == TplType::new_)
            ++its;
        for (; ito != itoe && its != itse; ++ito, ++its)
        {
            if (ito->second->type != its->second->type)
                return false;
            if (ito->second->packed)
            {
                return false;
            }
            switch (ito->second->type)
            {
                case TplType::typename_:
                    if (sameTemplate(ito->second->byClass.dflt, its->second->byClass.dflt))
                    {
                        auto to = ito->second->byClass.dflt;
                        auto ts = its->second->byClass.dflt;
                        if (isref(to))
                            to = basetype(to)->btp;
                        if (isref(ts))
                            ts = basetype(ts)->btp;
                        if (!exactMatchOnTemplateArgs(basetype(to)->sp->templateParams, basetype(ts)->sp->templateParams))
                            return false;
                    }
                    else
                    {
                        if (!templatecomparetypes(ito->second->byClass.dflt, its->second->byClass.dflt, true))
                            return false;
                        if (!templatecomparetypes(its->second->byClass.dflt, ito->second->byClass.dflt, true))
                            return false;
                        if (isarray(ito->second->byClass.dflt) != isarray(its->second->byClass.dflt))
                            return false;
                        if (isarray(ito->second->byClass.dflt))
                            if (!!basetype(ito->second->byClass.dflt)->esize != !!basetype(its->second->byClass.dflt)->esize)
                                return false;
                    }
                    {
                        TYPE* ts = its->second->byClass.dflt;
                        TYPE* to = ito->second->byClass.dflt;
                        if (isref(ts))
                            ts = basetype(ts)->btp;
                        if (isref(to))
                            to = basetype(to)->btp;
                        if (isconst(ts) != isconst(to))
                            return false;
                        if (isvolatile(ts) != isvolatile(to))
                            return false;
                    }
                    break;
                case TplType::template_:
                    if (ito->second->byTemplate.dflt != its->second->byTemplate.dflt)
                        return false;
                    break;
                case TplType::int_:
                    if (!templatecomparetypes(ito->second->byNonType.tp, its->second->byNonType.tp, true))
                        return false;
                    if (!!ito->second->byNonType.dflt != !!its->second->byNonType.dflt)
                        return false;
                    if (ito->second->byNonType.dflt && its->second->byNonType.dflt &&
                        !templatecompareexpressions(ito->second->byNonType.dflt, its->second->byNonType.dflt))
                        return false;
                    break;
                default:
                    break;
            }
        }
        return ito == itoe && its == itse;
    }
    return !old && !sym;
}
static std::list<TEMPLATEPARAMPAIR>* mergeTemplateDefaults(std::list<TEMPLATEPARAMPAIR>* old, std::list<TEMPLATEPARAMPAIR>* sym,
                                                           bool definition)
{
    std::list<TEMPLATEPARAMPAIR>* rv = sym;
    if (old && sym)
    {
        auto ito = old->begin();
        auto itoe = old->end();
        auto its = sym->begin();
        auto itse = sym->end();
        for (; ito != itoe && its != itse; ++ito, ++its)
        {
            if (!definition && ito->first)
            {
                its->first = ito->first;
                its->first->tp->templateParam = &*its;
            }
            switch (its->second->type)
            {
                case TplType::template_:
                    its->second->byTemplate.args =
                        mergeTemplateDefaults(ito->second->byTemplate.args, its->second->byTemplate.args, definition);
                    if (ito->second->byTemplate.txtdflt && its->second->byTemplate.txtdflt)
                    {
                        if (!CompareLex(ito->second->byNonType.txtdflt, its->second->byNonType.txtdflt))
                            errorsym(ERR_MULTIPLE_DEFAULT_VALUES_IN_TEMPLATE_DECLARATION, its->first);
                    }
                    else if (!its->second->byTemplate.txtdflt)
                    {
                        its->second->byTemplate.txtdflt = ito->second->byTemplate.txtdflt;
                        its->second->byTemplate.txtargs = ito->second->byTemplate.txtargs;
                    }
                    break;
                case TplType::typename_:
                    if (ito->second->byClass.txtdflt && its->second->byClass.txtdflt)
                    {
                        if (!CompareLex(ito->second->byNonType.txtdflt, its->second->byNonType.txtdflt))
                            errorsym(ERR_MULTIPLE_DEFAULT_VALUES_IN_TEMPLATE_DECLARATION, its->first);
                    }
                    else if (!its->second->byClass.txtdflt)
                    {
                        its->second->byClass.txtdflt = ito->second->byClass.txtdflt;
                        its->second->byClass.txtargs = ito->second->byClass.txtargs;
                    }
                    break;
                case TplType::int_:
                    if (ito->second->byNonType.txtdflt && its->second->byNonType.txtdflt)
                    {
                        if (!CompareLex(ito->second->byNonType.txtdflt, its->second->byNonType.txtdflt))
                            errorsym(ERR_MULTIPLE_DEFAULT_VALUES_IN_TEMPLATE_DECLARATION, its->first);
                    }
                    else if (!its->second->byNonType.txtdflt)
                    {
                        its->second->byNonType.txtdflt = ito->second->byNonType.txtdflt;
                        its->second->byNonType.txttype = ito->second->byNonType.txttype;
                        its->second->byNonType.txtargs = ito->second->byNonType.txtargs;
                    }
                    break;
                case TplType::new_:  // specialization
                    break;
                default:
                    break;
            }
        }
    }
    return rv;
}
static void checkTemplateDefaults(std::list<TEMPLATEPARAMPAIR>* args)
{
    SYMBOL* last = nullptr;
    if (args)
    {
        for (auto&& arg : *args)
        {
            void* txtdflt = nullptr;
            switch (arg.second->type)
            {
                case TplType::template_:
                    checkTemplateDefaults(arg.second->byTemplate.args);
                    txtdflt = arg.second->byTemplate.txtdflt;
                    break;
                case TplType::typename_:
                    txtdflt = arg.second->byClass.txtdflt;
                    break;
                case TplType::int_:
                    txtdflt = arg.second->byNonType.txtdflt;
                    break;
                default:
                    break;
            }
            if (last && !txtdflt)
            {
                errorsym(ERR_MISSING_DEFAULT_VALUES_IN_TEMPLATE_DECLARATION, last);
                break;
            }
            if (txtdflt)
                last = arg.first;
        }
    }
}
bool matchTemplateSpecializationToParams(std::list<TEMPLATEPARAMPAIR>* param, std::list<TEMPLATEPARAMPAIR>* special, SYMBOL* sp)
{
    if (param && special)
    {
        auto itp = param->begin();
        auto itpe = param->end();
        auto its = special->begin();
        auto itse = special->end();
        if (itp->second->type == TplType::new_)
            ++itp;
        for (; itp != itpe && !itp->second->packed && its != itse; ++itp, ++its)
        {
            if (itp->second->type != its->second->type)
            {
                if (itp->second->type != TplType::typename_ || its->second->type != TplType::template_)
                    errorsym(ERR_INCORRECT_ARGS_PASSED_TO_TEMPLATE, sp);
            }
            else if (itp->second->type == TplType::template_)
            {
                if (!exactMatchOnTemplateParams(itp->second->byTemplate.args, its->second->byTemplate.dflt->templateParams))
                    errorsym(ERR_INCORRECT_ARGS_PASSED_TO_TEMPLATE, sp);
            }
            else if (itp->second->type == TplType::int_)
            {
                if (itp->second->byNonType.tp->type != BasicType::templateparam_ &&
                    !comparetypes(itp->second->byNonType.tp, its->second->byNonType.tp, false) &&
                    (!ispointer(itp->second->byNonType.tp) || !isconstzero(itp->second->byNonType.tp, its->second->byNonType.dflt)))
                    errorsym(ERR_INCORRECT_ARGS_PASSED_TO_TEMPLATE, sp);
            }
        }
        if (itp != itpe)
        {
            if (!itp->second->packed)
            {
                errorsym(ERR_TOO_FEW_ARGS_PASSED_TO_TEMPLATE, sp);
            }
            else
            {
                param = nullptr;
                special = nullptr;
            }
        }
        else if (its != itse)
        {
            if (its->second->packed)
                special = nullptr;
            else
                errorsym(ERR_TOO_MANY_ARGS_PASSED_TO_TEMPLATE, sp);
        }
        return itp == itpe && its == itse;
    }
    return !param && !special;
}
static void checkMultipleArgs(std::list<TEMPLATEPARAMPAIR>* sym)
{
    if (sym)
    {
        for (auto it = sym->begin(); it != sym->end(); ++it)
        {
            if (it->first)
            {
                auto it1 = it;
                for (++it1; it1 != sym->end(); ++it1)
                {
                    if (it1->first && !strcmp(it->first->name, it1->first->name))
                    {
                        currentErrorLine = 0;
                        errorsym(ERR_DUPLICATE_IDENTIFIER, it->first);
                    }
                }
            }
            if (it->second->type == TplType::template_)
            {
                checkMultipleArgs(it->second->byTemplate.args);
            }
        }
    }
}
std::list<TEMPLATEPARAMPAIR>* TemplateMatching(LEXLIST* lex, std::list<TEMPLATEPARAMPAIR>* old, std::list<TEMPLATEPARAMPAIR>* sym,
                                               SYMBOL* sp, bool definition)
{
    (void)lex;
    std::list<TEMPLATEPARAMPAIR>* rv = nullptr;
    currents->sp = sp;
    if (old)
    {
        if (sym->front().second->bySpecialization.types)
        {
            auto ito = old->begin();
            auto itoe = old->end();
            std::list<TEMPLATEPARAMPAIR>* transfer;
            matchTemplateSpecializationToParams(old, sym->front().second->bySpecialization.types, sp);
            rv = sym;
            transfer = sym->front().second->bySpecialization.types;
            auto ittransfer = sym->front().second->bySpecialization.types->begin();
            auto itetransfer = sym->front().second->bySpecialization.types->end();
            for (++ito; ito != itoe && ittransfer != itetransfer && !ito->second->packed; ++ito, ++ittransfer)
            {
                if (ittransfer->second->type != TplType::typename_ ||
                    basetype(ittransfer->second->byClass.dflt)->type != BasicType::templateselector_)
                {
                    ittransfer->second->byClass.txtdflt = ito->second->byClass.txtdflt;
                    ittransfer->second->byClass.txtargs = ito->second->byClass.txtargs;
                    if (ittransfer->second->type == TplType::int_)
                        ittransfer->second->byNonType.txttype = ito->second->byNonType.txttype;
                }
            }
        }
        else if (sym->size() > 1)
        {
            if (!exactMatchOnTemplateParams(old, sym))
            {
                error(ERR_TEMPLATE_DEFINITION_MISMATCH);
            }
            else
            {
                rv = mergeTemplateDefaults(old, sym, definition);
                checkTemplateDefaults(rv);
            }
        }
        else
        {
            rv = sym;
        }
    }
    else
    {
        rv = sym;
        checkTemplateDefaults(sym);
    }
    checkMultipleArgs(sym);
    return rv;
}
bool typeHasTemplateArg(TYPE* t);
static bool structHasTemplateArg(std::list<TEMPLATEPARAMPAIR>* tplx)
{
    if (tplx)
    {
        std::stack<std::list<TEMPLATEPARAMPAIR>::iterator> tps;
        auto itl = tplx->begin();
        auto itle = tplx->end();
        for (; itl != itle;)
        {
            if (itl->second->type == TplType::typename_)
            {
                if (itl->second->packed)
                {
                    if (itl->second->byPack.pack)
                    {
                        tps.push(itl);
                        tps.push(itle);
                        itl = itl->second->byPack.pack->begin();
                        itle = itl->second->byPack.pack->end();
                        continue;
                    }
                }
                else
                {
                    if (typeHasTemplateArg(itl->second->byClass.dflt))
                        return true;
                }
            }
            else if (itl->second->type == TplType::template_)
            {
                if (structHasTemplateArg(itl->second->byTemplate.args))
                    return true;
            }
            ++itl;
            if (itl == itle && tps.size())
            {
                itle = tps.top();
                tps.pop();
                itl = tps.top();
                tps.pop();
                ++itl;
            }
        }
    }
    return false;
}
bool typeHasTemplateArg(TYPE* t)
{
    if (t)
    {
        while (ispointer(t) || isref(t))
            t = t->btp;
        if (isfunction(t))
        {
            t = basetype(t);
            if (typeHasTemplateArg(t->btp))
                return true;
            for (auto sym : *t->syms)
            {
                if (typeHasTemplateArg(sym->tp))
                    return true;
            }
        }
        else if (basetype(t)->type == BasicType::templateparam_)
            return true;
        else if (isstructured(t))
        {
            std::list<TEMPLATEPARAMPAIR>* tpx = basetype(t)->sp->templateParams;
            if (structHasTemplateArg(tpx))
                return true;
        }
    }
    return false;
}
void TemplateValidateSpecialization(std::list<TEMPLATEPARAMPAIR>* arg)
{
    if (arg && arg->front().second->bySpecialization.types)
    {
        bool found = false;
        for (auto&& t : *arg->front().second->bySpecialization.types)
        {
            if (t.second->type == TplType::typename_ && typeHasTemplateArg((TYPE*)t.second->byClass.dflt))
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            error(ERR_PARTIAL_SPECIALIZATION_MISSING_TEMPLATE_PARAMETERS);
        }
    }
}
void GetPackedTypes(TEMPLATEPARAMPAIR** packs, int* count, std::list<TEMPLATEPARAMPAIR>* args)
{
    if (args)
    {
        for (auto&& arg : *args)
        {
            if (arg.second->type == TplType::typename_)
            {
                if (arg.second->packed)
                {
                    packs[(*count)++] = &arg;
                }
            }
            else if (arg.second->type == TplType::delete_)
            {
                GetPackedTypes(packs, count, arg.second->byDeferred.args);
            }
        }
    }
}
void saveParams(SYMBOL** table, int count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        if (table[i] && table[i]->templateParams)
        {
            for (auto&& param : *table[i]->templateParams)
            {
                if (param.second->type != TplType::new_)
                    param.second->hold = param.second->byClass.val;
            }
        }
    }
}
void restoreParams(SYMBOL** table, int count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        if (table[i] && table[i]->templateParams)
        {
            for (auto&& param : *table[i]->templateParams)
            {
                if (param.second->type != TplType::new_)
                    param.second->byClass.val = (TYPE*)param.second->hold;
            }
        }
    }
}
static std::list<INITLIST*>* ExpandArguments(EXPRESSION* exp)
{
    std::list<INITLIST*>* rv = nullptr;
    bool dofunc = false;
    bool doparam = false;
    if (exp->v.func->arguments)
    {
        for (auto arg : *exp->v.func->arguments)
        {
            if (arg->exp && (arg->exp->type == ExpressionNode::func_ || arg->exp->type == ExpressionNode::funcret_))
            {
                dofunc = true;
            }
            if (arg->tp && basetype(arg->tp)->type == BasicType::templateparam_)
            {
                doparam |= !templateNestingCount || instantiatingTemplate;
            }
        }
        if (doparam)
        {
            TYPE* tp = nullptr;
            for (auto arg : *exp->v.func->arguments)
            {
                TYPE* tp1 = basetype(arg->tp);
                if (tp1 && tp1->type == BasicType::templateparam_)
                {
                    if (tp1->templateParam->second->packed)
                    {
                        if (tp1->templateParam->second->byPack.pack)
                        {
                            for (auto&& tpx : *tp1->templateParam->second->byPack.pack)
                            {
                                auto dflt = tpx.second->byClass.val;
                                if (!dflt)
                                    dflt = tpx.second->byClass.dflt;
                                if (dflt)
                                {
                                    tp = tpx.second->byClass.val;
                                    if (isconst(arg->tp))
                                        tp = MakeType(BasicType::const_, tp);
                                    if (isvolatile(arg->tp))
                                        tp = MakeType(BasicType::volatile_, tp);
                                    if (!rv)
                                        rv = initListListFactory.CreateList();
                                    auto arg1 = Allocate<INITLIST>();
                                    arg1->tp = tp;
                                    arg1->exp = intNode(ExpressionNode::c_i_, 0);
                                    rv->push_back(arg1);
                                }
                            }
                        }
                    }
                    else
                    {
                        auto arg1 = Allocate<INITLIST>();
                        *arg1 = *arg;
                        tp = tp1->templateParam->second->byClass.val;
                        if (tp)
                        {
                            if (isconst(arg->tp))
                                tp = MakeType(BasicType::const_, tp);
                            if (isvolatile(arg->tp))
                                tp = MakeType(BasicType::volatile_, tp);
                            arg1->tp = tp;
                        }
                        if (!rv)
                            rv = initListListFactory.CreateList();
                        rv->push_back(arg1);
                    }
                }
                else
                {
                    auto arg1 = Allocate<INITLIST>();
                    *arg1 = *arg;
                    if (!rv)
                        rv = initListListFactory.CreateList();
                    rv->push_back(arg1);
                }
            }
        }
        if (dofunc)
        {
            for (auto arg : *exp->v.func->arguments)
            {
                if (arg->exp)
                {
                    SYMBOL* syms[200];
                    int count = 0, n = 0;
                    GatherPackedVars(&count, syms, arg->exp);
                    if (count)
                    {
                        for (int i = 0; i < count; i++)
                        {
                            int n1 = CountPacks(syms[i]->tp->templateParam->second->byPack.pack);
                            if (n1 > n)
                                n = n1;
                        }
                        int oldIndex = packIndex;
                        for (int i = 0; i < n; i++)
                        {
                            std::deque<TEMPLATEPARAM*> defaults;
                            std::deque<std::pair<TYPE**, TYPE*>> types;
                            packIndex = i;
                            auto arg1 = Allocate<INITLIST>();
                            *arg1 = *arg;
                            if (!rv)
                                rv = initListListFactory.CreateList();
                            rv->push_back(arg1);
                            if (arg1->exp->type == ExpressionNode::func_)
                            {
                                if (arg1->exp->v.func->templateParams)
                                {
                                    for (auto&& tpx : *arg1->exp->v.func->templateParams)
                                    {
                                        if (tpx.second->type != TplType::new_)
                                        {
                                            defaults.push_back(tpx.second);
                                            if (tpx.second->packed && tpx.second->byPack.pack)
                                            {
                                                auto it = tpx.second->byPack.pack->begin();
                                                auto ite = tpx.second->byPack.pack->end();
                                                for (int j = 0; j < packIndex && it != ite; ++j, ++it)
                                                    ;
                                                if (it != ite)
                                                    tpx.second = it->second;
                                            }
                                        }
                                    }
                                }
                                if (arg1->exp->v.func->arguments)
                                {
                                    for (auto il : *arg1->exp->v.func->arguments)
                                    {
                                        TYPE** tp = &il->tp;
                                        while ((*tp)->btp)
                                            tp = &(*tp)->btp;
                                        if ((*tp)->type == BasicType::templateparam_)
                                        {
                                            auto tpx = (*tp)->templateParam;
                                            if (tpx->second->packed && tpx->second->byPack.pack)
                                            {
                                                auto it = tpx->second->byPack.pack->begin();
                                                auto ite = tpx->second->byPack.pack->end();
                                                for (int j = 0; j < packIndex && it != ite; ++j, ++it)
                                                    ;
                                                if (it != ite && it->second->type == TplType::typename_ && it->second->byClass.val)
                                                {
                                                    types.push_back(std::pair<TYPE**, TYPE*>(tp, *tp));
                                                    (*tp) = it->second->byClass.val;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            arg1->tp = LookupTypeFromExpression(arg1->exp, nullptr, false);
                            if (arg1->tp && isref(arg1->tp))
                            {
                                bool rref = basetype(arg1->tp)->type == BasicType::rref_;
                                arg1->tp = basetype(arg1->tp)->btp;
                                if (rref)
                                    (arg1->tp)->rref = true;
                                else
                                    (arg1->tp)->lref = true;
                            }
                            if (arg1->tp == nullptr)
                                arg1->tp = arg->tp;
                            if (arg1->exp->type == ExpressionNode::func_)
                            {
                                if (arg1->exp->v.func->templateParams)
                                {
                                    for (auto&& tpx : *arg1->exp->v.func->templateParams)
                                    {
                                        if (tpx.second->type != TplType::new_)
                                        {
                                            tpx.second = defaults.front();
                                            defaults.pop_front();
                                        }
                                    }
                                }
                                for (auto&& t : types)
                                {
                                    *(t.first) = t.second;
                                }
                                types.clear();
                            }
                        }
                        packIndex = oldIndex;
                    }
                    else
                    {
                        auto arg1 = Allocate<INITLIST>();
                        *arg1 = *arg;
                        arg1->tp = LookupTypeFromExpression(arg1->exp, nullptr, false);
                        if (arg1->tp == nullptr)
                            arg1->tp = &stdany;
                        if (!rv)
                            rv = initListListFactory.CreateList();
                        rv->push_back(arg1);
                    }
                }
                else
                {
                    auto arg1 = Allocate<INITLIST>();
                    *arg1 = *arg;
                    if (!rv)
                        rv = initListListFactory.CreateList();
                    rv->push_back(arg1);
                }
            }
        }
        else if (!rv)
        {
            rv = exp->v.func->arguments;
        }
    }
    return rv;
}
void PushPopDefaults(std::deque<TYPE*>& defaults, std::list<TEMPLATEPARAMPAIR>* tpx, bool dflt, bool push);
static void PushPopDefaults(std::deque<TYPE*>& defaults, EXPRESSION* exp, bool dflt, bool push)
{
    std::stack<EXPRESSION*> stk;
    stk.push(exp);
    while (!stk.empty())
    {
        auto top = stk.top();
        stk.pop();
        if (top->type == ExpressionNode::templateselector_)
        {
            auto ts = top->v.templateSelector->begin();
            ++ts;
            if (ts->isTemplate && ts->templateParams)
                PushPopDefaults(defaults, ts->templateParams, dflt, push);
        }
        else
        {
            if (top->left)
                stk.push(top->left);
            if (top->right)
                stk.push(top->right);
        }
    }
}
void PushPopDefaults(std::deque<TYPE*>& defaults, std::list<TEMPLATEPARAMPAIR>* tpx, bool dflt, bool push)
{
    std::stack<std::list<TEMPLATEPARAMPAIR>::iterator> stk;
    if (tpx)
    {
        for (auto&& item : *tpx)
        {
            if (item.second->type != TplType::new_)
            {
                if (push)
                {
                    defaults.push_back(item.second->packed ? (TYPE*)1 : (TYPE*)0);
                    defaults.push_back(dflt ? item.second->byClass.dflt : item.second->byClass.val);
                    if (item.second->packed)
                    {
                        PushPopDefaults(defaults, item.second->byPack.pack, dflt, push);
                    }
                }
                else if (defaults.size())
                {
                    if (dflt)
                    {
                        item.second->packed = !!defaults.front();
                        defaults.pop_front();
                        if (dflt)
                            item.second->byClass.dflt = defaults.front();
                        else
                            item.second->byClass.val = defaults.front();
                    }
                    else
                    {
                        item.second->byClass.val = defaults.front();
                    }
                    defaults.pop_front();
                    if (item.second->packed)
                    {
                        PushPopDefaults(defaults, item.second->byPack.pack, dflt, push);
                    }
                }
                else
                {
                    if (dflt)
                    {
                        item.second->byClass.dflt = nullptr;
                    }
                    else
                    {
                        item.second->byClass.val = nullptr;
                    }
                }
                if (!item.second->packed &&
                    ((dflt && item.second->type == TplType::typename_ && item.second->byClass.dflt &&
                      isstructured(item.second->byClass.dflt) && basetype(item.second->byClass.dflt)->sp->templateParams) ||
                     (!dflt && item.second->type == TplType::typename_ && item.second->byClass.val &&
                      isstructured(item.second->byClass.val) && basetype(item.second->byClass.val)->sp->templateParams)))
                {
                    PushPopDefaults(defaults,
                                    basetype(dflt ? item.second->byClass.dflt : item.second->byClass.val)->sp->templateParams, dflt,
                                    push);
                }
                if (!item.second->packed && ((dflt && item.second->type == TplType::int_ && item.second->byClass.dflt) ||
                                             (!dflt && item.second->type == TplType::typename_ && item.second->byClass.val)))
                {
                    PushPopDefaults(defaults, dflt ? item.second->byNonType.dflt : item.second->byNonType.val, dflt, push);
                }
            }
        }
    }
}
std::list<TEMPLATEPARAMPAIR>* ExpandParams(EXPRESSION* exp)
{
    if (templateNestingCount && !instantiatingTemplate)
        return exp->v.func->templateParams;
    if (!exp->v.func->templateParams)
        return nullptr;
    bool found = false;
    for (auto&& tpx : *exp->v.func->templateParams)
    {
        if (tpx.second->packed)
        {
            found = true;
            break;
        }
    }
    if (!found)
        return exp->v.func->templateParams;
    std::list<TEMPLATEPARAMPAIR>* rv = nullptr;
    for (auto&& tpx : *exp->v.func->templateParams)
    {
        if (tpx.second->packed)
        {
            if (tpx.second->byPack.pack)
            {
                if (!rv)
                    rv = templateParamPairListFactory.CreateList();
                for (auto tpx1 : *tpx.second->byPack.pack)
                {
                    rv->push_back(tpx1);
                    rv->back().second = tpx1.second;
                    if (tpx1.second->byClass.val)
                        rv->back().second->byClass.dflt = tpx1.second->byClass.val;
                }
            }
        }
        else
        {
            if (!rv)
                rv = templateParamPairListFactory.CreateList();
            rv->push_back(tpx);
            if (tpx.second->byClass.val)
                rv->back().second->byClass.dflt = tpx.second->byClass.val;
        }
    }
    return rv;
}
static TYPE* LookupUnaryMathFromExpression(EXPRESSION* exp, Keyword kw, std::list<TEMPLATEPARAMPAIR>* enclosing, bool alt)
{
    TYPE* tp1 = LookupTypeFromExpression(exp->left, enclosing, alt);
    if (!tp1)
        return nullptr;
    if (isref(tp1))
        tp1 = basetype(tp1)->btp;
    auto exp1 = exp->left;
    ResolveTemplateVariable(&tp1, &exp1, nullptr, nullptr);
    if (!insertOperatorFunc(ovcl_binary_numericptr, kw, nullptr, &tp1, &exp1, nullptr, nullptr, nullptr, _F_SIZEOF))
    {
        castToArithmetic(false, &tp1, &exp1, kw, nullptr, true);
        if (isstructured(tp1))
            return nullptr;
        if (ispointer(tp1))
            return nullptr;
    }
    return tp1;
}
static TYPE* LookupBinaryMathFromExpression(EXPRESSION* exp, Keyword kw, std::list<TEMPLATEPARAMPAIR>* enclosing, bool alt)
{
    TYPE* tp1 = LookupTypeFromExpression(exp->left, enclosing, alt);
    if (!tp1)
        return nullptr;
    TYPE* tp2 = LookupTypeFromExpression(exp->right, enclosing, alt);
    if (!tp2)
        return nullptr;
    if (isref(tp1))
        tp1 = basetype(tp1)->btp;
    if (isref(tp2))
        tp2 = basetype(tp2)->btp;
    auto exp1 = exp->left;
    auto exp2 = exp->right;
    ResolveTemplateVariable(&tp1, &exp1, tp2, nullptr);
    ResolveTemplateVariable(&tp2, &exp2, tp1, nullptr);
    if (!insertOperatorFunc(ovcl_binary_numericptr, kw, nullptr, &tp1, &exp1, tp2, exp2, nullptr, _F_SIZEOF))
    {
        if (kw == Keyword::leftshift_ || kw == Keyword::rightshift_)
        {
            castToArithmetic(false, &tp1, &exp1, kw, tp2, true);
            if (isstructured(tp1) || isstructured(tp2))
                return nullptr;
            if (ispointer(tp1) || ispointer(tp2))
                return nullptr;
        }
        else if (kw == Keyword::lt_ || kw == Keyword::gt_ || kw == Keyword::leq_ || kw == Keyword::geq_ || kw == Keyword::eq_ ||
                 kw == Keyword::neq_ || kw == Keyword::land_ || kw == Keyword::lor_)
        {
            if (isstructured(tp1) || isstructured(tp2))
                return nullptr;
            return &stdbool;
        }
        else if ((kw != Keyword::plus_ && kw != Keyword::minus_) || (!ispointer(tp1) && !ispointer(tp2)))
        {
            castToArithmetic(false, &tp1, &exp1, kw, tp2, true);
            castToArithmetic(false, &tp2, &exp2, (Keyword)-1, tp1, true);
            if (isstructured(tp1) || isstructured(tp2))
                return nullptr;
            if (ispointer(tp1) || ispointer(tp2))
                return nullptr;
            tp1 = destSize(tp1, tp2, nullptr, nullptr, false, nullptr);
        }
        else
        {
            if (isstructured(tp1) || isstructured(tp2))
                return nullptr;
            if (ispointer(tp1) && ispointer(tp2))
            {
                tp1 = &stdint;  // ptrdiff_t
            }
            else
            {
                if (ispointer(tp2))
                    tp1 = tp2;
            }
        }
    }
    return tp1;
}
TYPE* LookupTypeFromExpression(EXPRESSION* exp, std::list<TEMPLATEPARAMPAIR>* enclosing, bool alt)
{
    EXPRESSION* funcList[100];
    int count = 0;
    switch (exp->type)
    {
        case ExpressionNode::dot_:
        case ExpressionNode::pointsto_: {
            TYPE* tp = LookupTypeFromExpression(exp->left, nullptr, false);
            if (!tp)
                return tp;
            while (exp->type == ExpressionNode::dot_ || exp->type == ExpressionNode::pointsto_)
            {
                if (exp->type == ExpressionNode::pointsto_)
                {
                    if (!ispointer(tp))
                        return nullptr;
                    tp = basetype(tp)->btp;
                }
                EXPRESSION* next = exp->right;
                if (next->type == ExpressionNode::dot_ || next->type == ExpressionNode::pointsto_)
                {
                    next = exp->left;
                }
                STRUCTSYM s;
                while (isref(tp))
                    tp = basetype(tp)->btp;
                //                tp = PerformDeferredInitialization(tp, nullptr);
                s.str = basetype(tp)->sp;
                addStructureDeclaration(&s);
                while (next->type == ExpressionNode::funcret_)
                    next = next->left;
                if (next->type == ExpressionNode::thisref_)
                    next = next->left;
                if (next->type == ExpressionNode::func_)
                {
                    TYPE* ctype = tp;
                    SYMBOL* sym = classsearch(next->v.func->sp->name, false, false, false);
                    if (!sym)
                    {
                        dropStructureDeclaration();
                        break;
                    }
                    FUNCTIONCALL* func = Allocate<FUNCTIONCALL>();
                    *func = *next->v.func;
                    func->sp = sym;
                    func->thistp = MakeType(BasicType::pointer_, tp);
                    func->thisptr = intNode(ExpressionNode::c_i_, 0);
                    func->arguments = ExpandArguments(next);
                    auto oldnoExcept = noExcept;
                    sym = GetOverloadedFunction(&ctype, &func->fcall, sym, func, nullptr, true, false, 0);
                    noExcept = oldnoExcept;
                    if (!sym)
                    {
                        dropStructureDeclaration();
                        break;
                    }
                    EXPRESSION* temp = varNode(ExpressionNode::func_, sym);
                    temp->v.func = func;
                    temp = exprNode(ExpressionNode::thisref_, temp, nullptr);
                    temp->v.t.thisptr = intNode(ExpressionNode::c_i_, 0);
                    temp->v.t.tp = tp;
                    tp = LookupTypeFromExpression(temp, nullptr, false);
                }
                else
                {
                    SYMBOL* sym = classsearch(GetSymRef(next)->v.sp->name, false, false, false);
                    if (!sym)
                    {
                        dropStructureDeclaration();
                        break;
                    }
                    tp = sym->tp;
                }
                dropStructureDeclaration();
                exp = exp->right;
            }
            if (exp->type != ExpressionNode::dot_ && exp->type != ExpressionNode::pointsto_)
                return tp;
            return nullptr;
        }
        case ExpressionNode::void_:
            while (exp->type == ExpressionNode::void_ && exp->right)
            {
                if (!LookupTypeFromExpression(exp->left, enclosing, alt))
                    return nullptr;
                exp = exp->right;
            }
            if (exp)
            {
                return LookupTypeFromExpression(exp, enclosing, alt);
            }
            return nullptr;
        case ExpressionNode::not__lvalue_:
        case ExpressionNode::lvalue_:
        case ExpressionNode::argnopush_:
        case ExpressionNode::void_nz_:
        case ExpressionNode::shiftby_:
            return LookupTypeFromExpression(exp->left, enclosing, alt);
        case ExpressionNode::global_:
        case ExpressionNode::auto_:
        case ExpressionNode::labcon_:
        case ExpressionNode::absolute_:
        case ExpressionNode::pc_:
        case ExpressionNode::const_:
        case ExpressionNode::threadlocal_: {
            TYPE* rv = exp->v.sp->tp;
            if (rv->type == BasicType::templateparam_ || (isref(rv) && basetype(rv->btp)->type == BasicType::templateparam_))
                rv = SynthesizeType(rv, nullptr, false);
            return rv;
        }
        case ExpressionNode::x_label_:
            return &stdpointer;
        case ExpressionNode::c_bit_:
        case ExpressionNode::c_bool_:
        case ExpressionNode::x_bool_:
        case ExpressionNode::x_bit_:
        case ExpressionNode::l_bool_:
        case ExpressionNode::l_bit_:
            return &stdbool;
        case ExpressionNode::c_c_:
        case ExpressionNode::x_c_:
        case ExpressionNode::l_c_:
            return &stdchar;
        case ExpressionNode::c_uc_:
        case ExpressionNode::x_uc_:
        case ExpressionNode::l_uc_:
            return &stdunsignedchar;
        case ExpressionNode::c_wc_:
        case ExpressionNode::x_wc_:
        case ExpressionNode::l_wc_:
            return &stdwidechar;
        case ExpressionNode::c_s_:
        case ExpressionNode::x_s_:
        case ExpressionNode::l_s_:
            return &stdshort;
        case ExpressionNode::c_u16_:
        case ExpressionNode::x_u16_:
        case ExpressionNode::l_u16_:
            return &stdchar16t;
        case ExpressionNode::c_us_:
        case ExpressionNode::x_us_:
        case ExpressionNode::l_us_:
            return &stdunsignedshort;
        case ExpressionNode::c_i_:
        case ExpressionNode::x_i_:
        case ExpressionNode::l_i_:
        case ExpressionNode::structelem_:
            return &stdint;
        case ExpressionNode::c_ui_:
        case ExpressionNode::x_ui_:
        case ExpressionNode::l_ui_:
        case ExpressionNode::sizeofellipse_:
            return &stdunsigned;
        case ExpressionNode::x_bitint_: {
            auto rv = MakeType(BasicType::bitint_);
            rv->bitintbits = exp->v.b.bits;
            return rv;
        }
        case ExpressionNode::x_ubitint_: {
            auto rv = MakeType(BasicType::unsigned_bitint_);
            rv->bitintbits = exp->v.b.bits;
            return rv;
        }
        case ExpressionNode::x_inative_:
        case ExpressionNode::l_inative_:
            return &stdinative;
        case ExpressionNode::x_unative_:
        case ExpressionNode::l_unative_:
            return &stdunative;
        case ExpressionNode::c_u32_:
        case ExpressionNode::x_u32_:
        case ExpressionNode::l_u32_:
            return &stdchar32t;
        case ExpressionNode::c_l_:
        case ExpressionNode::x_l_:
        case ExpressionNode::l_l_:
            return &stdlong;
        case ExpressionNode::c_ul_:
        case ExpressionNode::x_ul_:
        case ExpressionNode::l_ul_:
            return &stdunsignedlong;
        case ExpressionNode::c_ll_:
        case ExpressionNode::x_ll_:
        case ExpressionNode::l_ll_:
            return &stdlonglong;
        case ExpressionNode::c_ull_:
        case ExpressionNode::x_ull_:
        case ExpressionNode::l_ull_:
            return &stdunsignedlonglong;
        case ExpressionNode::c_f_:
        case ExpressionNode::x_f_:
        case ExpressionNode::l_f_:
            return &stdfloat;
        case ExpressionNode::c_d_:
        case ExpressionNode::x_d_:
        case ExpressionNode::l_d_:
            return &stddouble;
        case ExpressionNode::c_ld_:
        case ExpressionNode::x_ld_:
        case ExpressionNode::l_ld_:
            return &stdlongdouble;
        case ExpressionNode::c_p_:
        case ExpressionNode::x_p_:
            return LookupTypeFromExpression(exp->left, enclosing, alt);
        case ExpressionNode::l_ref_: {
            TYPE* tp = LookupTypeFromExpression(exp->left, enclosing, alt);
            if (tp && isref(tp))
                tp = basetype(tp)->btp;
            return tp;
        }
        case ExpressionNode::c_string_:
        case ExpressionNode::l_string_:
        case ExpressionNode::x_string_:
            return &std__string;
        case ExpressionNode::x_object_:
        case ExpressionNode::l_object_:
            return &std__object;
        case ExpressionNode::l_p_: {
            TYPE* tp = LookupTypeFromExpression(exp->left, enclosing, alt);
            if (tp && ispointer(tp))
                tp = basetype(tp)->btp;
            return tp;
        }

        case ExpressionNode::c_sp_:
        case ExpressionNode::x_sp_:
        case ExpressionNode::l_sp_:
            return &stdchar16t;
        case ExpressionNode::c_fp_:
        case ExpressionNode::x_fp_:
        case ExpressionNode::l_fp_:
            return &stdpointer;  // fixme
        case ExpressionNode::c_fc_:
        case ExpressionNode::x_fc_:
        case ExpressionNode::l_fc_:
            return &stdfloatcomplex;
        case ExpressionNode::c_dc_:
        case ExpressionNode::x_dc_:
        case ExpressionNode::l_dc_:
            return &stddoublecomplex;
        case ExpressionNode::c_ldc_:
        case ExpressionNode::x_ldc_:
        case ExpressionNode::l_ldc_:
            return &stdlongdoublecomplex;
        case ExpressionNode::c_fi_:
        case ExpressionNode::x_fi_:
        case ExpressionNode::l_fi_:
            return &stdfloatimaginary;
        case ExpressionNode::c_di_:
        case ExpressionNode::x_di_:
        case ExpressionNode::l_di_:
            return &stddoubleimaginary;
        case ExpressionNode::c_ldi_:
        case ExpressionNode::x_ldi_:
        case ExpressionNode::l_ldi_:
            return &stdlongdoubleimaginary;
        case ExpressionNode::nullptr_:
            return &stdnullpointer;
        case ExpressionNode::memberptr_:
            return &stdpointer;
        case ExpressionNode::mp_as_bool_:
            return &stdbool;
        case ExpressionNode::mp_compare_:
            return &stdbool;
        case ExpressionNode::trapcall_:
        case ExpressionNode::intcall_:
            return &stdvoid;
        case ExpressionNode::construct_:
            return exp->v.construct.tp;
        case ExpressionNode::funcret_:
            while (exp->type == ExpressionNode::funcret_)
            {
                funcList[count++] = exp;
                exp = exp->left;
            }
            /* fall through */
        case ExpressionNode::func_: {
            TYPE* rv;
            EXPRESSION* exp1 = nullptr;
            if (basetype(exp->v.func->functp)->type != BasicType::aggregate_ && !isstructured(exp->v.func->functp) &&
                !basetype(exp->v.func->functp)->sp->sb->externShim)
            {
                if (exp->v.func->asaddress)
                {
                    rv = MakeType(BasicType::pointer_, exp->v.func->functp);
                }
                else if (exp->v.func->sp->name == overloadNameTab[CI_CONSTRUCTOR])
                {
                    return basetype(exp->v.func->thistp)->btp;
                }
                else
                {
                    rv = basetype(exp->v.func->functp)->btp;
                }
            }
            else
            {
                TYPE* tp1 = nullptr;
                SYMBOL* sp;
                std::deque<TYPE*> defaults;
                if (exp->v.func->templateParams)
                {
                    for (auto&& tpx : *exp->v.func->templateParams)
                    {
                        if (tpx.second->type != TplType::new_)
                        {
                            defaults.push_back(tpx.second->byClass.dflt);
                            defaults.push_back(tpx.second->byClass.val);
                            if (tpx.second->byClass.val)
                                tpx.second->byClass.dflt = tpx.second->byClass.val;
                        }
                    }
                }
                std::list<INITLIST*>* old = exp->v.func->arguments;
                std::list<TEMPLATEPARAMPAIR>* oldp = exp->v.func->templateParams;
                exp->v.func->arguments = ExpandArguments(exp);
                exp->v.func->templateParams = ExpandParams(exp);
                auto oldnoExcept = noExcept;
                sp = GetOverloadedFunction(&tp1, &exp1, exp->v.func->sp, exp->v.func, nullptr, false, false, 0);
                noExcept = oldnoExcept;
                exp->v.func->arguments = old;
                exp->v.func->templateParams = oldp;
                if (exp->v.func->templateParams)
                {
                    for (auto&& tpx : *exp->v.func->templateParams)
                    {
                        if (tpx.second->type != TplType::new_)
                        {
                            tpx.second->byClass.dflt = defaults.front();
                            defaults.pop_front();
                            tpx.second->byClass.val = defaults.front();
                            defaults.pop_front();
                        }
                    }
                }
                if (sp)
                {
                    rv = basetype(sp->tp)->btp;
                }
                else
                {
                    rv = nullptr;
                }
            }
            while (count > 1 && rv)
            {
                TYPE* rve = rv;
                exp = funcList[--count];
                while (isref(rve))
                    rve = basetype(rve)->btp;
                if (isfuncptr(rve) || isfunction(rve) || isstructured(rve))
                {
                    std::list<INITLIST*>* old = nullptr;
                    if (exp->v.func)
                    {
                        old = exp->v.func->arguments;
                        exp->v.func->arguments = ExpandArguments(exp);
                    }
                    if (isstructured(rve))
                    {
                        rv = rve;
                        if (!exp->v.func || !insertOperatorParams(nullptr, &rv, &exp1, exp->v.func, 0))
                            rv = &stdany;
                        if (isfunction(rv))
                            rv = basetype(rv)->btp;
                    }
                    else if (isfunction(rve))
                    {
                        bool ascall = exp->v.func->ascall;
                        exp->v.func->ascall = true;
                        TYPE* tp1 = nullptr;
                        SYMBOL* sym = rve->sp;
                        if (sym->tp->type != BasicType::aggregate_)
                            sym = sym->sb->overloadName;
                        rv = basetype(rve)->btp;
                        auto oldnoExcept = noExcept;
                        sym = GetOverloadedFunction(&tp1, &exp1, sym, exp->v.func, nullptr, false, false, 0);
                        noExcept = oldnoExcept;
                        if (!sym)
                            rv = &stdany;
                        else
                            rv = basetype(sym->tp)->btp;
                        exp->v.func->ascall = ascall;
                    }
                    else
                    {
                        rv = basetype(basetype(rve)->btp)->btp;
                    }
                    if (exp->v.func)
                    {
                        exp->v.func->arguments = old;
                    }
                    if (isconst(rve))
                    {
                        // to make LIBCXX happy
                        rv = MakeType(BasicType::const_, rv);
                    }
                }
                else
                    break;
            }
            return rv;
        }
        case ExpressionNode::lt_:
        case ExpressionNode::ult_: {
            return LookupBinaryMathFromExpression(exp, Keyword::lt_, enclosing, alt);
        }
        case ExpressionNode::le_:
        case ExpressionNode::ule_: {
            return LookupBinaryMathFromExpression(exp, Keyword::leq_, enclosing, alt);
        }
        case ExpressionNode::gt_:
        case ExpressionNode::ugt_: {
            return LookupBinaryMathFromExpression(exp, Keyword::gt_, enclosing, alt);
        }
        case ExpressionNode::ge_:
        case ExpressionNode::uge_: {
            return LookupBinaryMathFromExpression(exp, Keyword::geq_, enclosing, alt);
        }
        case ExpressionNode::eq_: {
            return LookupBinaryMathFromExpression(exp, Keyword::eq_, enclosing, alt);
        }
        case ExpressionNode::ne_: {
            return LookupBinaryMathFromExpression(exp, Keyword::neq_, enclosing, alt);
        }
        case ExpressionNode::land_: {
            return LookupBinaryMathFromExpression(exp, Keyword::land_, enclosing, alt);
        }
        case ExpressionNode::lor_: {
            return LookupBinaryMathFromExpression(exp, Keyword::lor_, enclosing, alt);
        }
        case ExpressionNode::uminus_:
            return LookupUnaryMathFromExpression(exp, Keyword::minus_, enclosing, alt);
        case ExpressionNode::not_:
            return LookupUnaryMathFromExpression(exp, Keyword::not_, enclosing, alt);
        case ExpressionNode::compl_:
            return LookupUnaryMathFromExpression(exp, Keyword::complx_, enclosing, alt);
        case ExpressionNode::auto_inc_:
            return LookupUnaryMathFromExpression(exp, Keyword::autoinc_, enclosing, alt);
        case ExpressionNode::auto_dec_:
            return LookupUnaryMathFromExpression(exp, Keyword::autodec_, enclosing, alt);
        case ExpressionNode::bits_:
            return LookupTypeFromExpression(exp->left, enclosing, alt);
        case ExpressionNode::assign_: {
            TYPE* tp1 = LookupTypeFromExpression(exp->left, enclosing, alt);
            if (tp1)
            {
                while (isref(tp1))
                    tp1 = basetype(tp1)->btp;
                if (isconst(tp1))
                    return nullptr;
                if (isstructured(tp1))
                {
                    SYMBOL* cons = search(basetype(tp1)->syms, overloadNameTab[CI_ASSIGN]);
                    if (!cons)
                        return nullptr;
                    TYPE* tp2 = LookupTypeFromExpression(exp->left, enclosing, alt);
                    TYPE* ctype = cons->tp;
                    TYPE thistp = {};
                    FUNCTIONCALL funcparams = {};
                    INITLIST a = {};
                    std::list<INITLIST*> args = {&a};
                    EXPRESSION x = {}, *xx = &x;
                    x.type = ExpressionNode::auto_;
                    x.v.sp = cons;
                    a.tp = tp2;
                    a.exp = &x;
                    funcparams.arguments = &args;
                    MakeType(thistp, BasicType::pointer_, basetype(tp1));
                    funcparams.thistp = &thistp;
                    funcparams.thisptr = &x;
                    funcparams.ascall = true;
                    auto oldnoExcept = noExcept;
                    cons = GetOverloadedFunction(&ctype, &xx, cons, &funcparams, nullptr, false, true, _F_SIZEOF);
                    noExcept = oldnoExcept;
                    if (!cons || cons->sb->deleted)
                    {
                        return nullptr;
                    }
                    tp1 = basetype(cons->tp)->btp;
                    while (isref(tp1))
                        tp1 = basetype(tp1)->btp;
                }
            }
            return tp1;
        }
        case ExpressionNode::templateparam_:
            if (exp->v.sp->tp->templateParam->second->type == TplType::typename_)
            {
                if (exp->v.sp->tp->templateParam->second->packed)
                {
                    TYPE* rv = &stdany;
                    if (packIndex < 0)
                    {
                        if (exp->v.sp->tp->templateParam->second->byPack.pack)
                            rv = exp->v.sp->tp->templateParam->second->byPack.pack->front().second->byClass.val;
                    }
                    else if (exp->v.sp->tp->templateParam->second->byPack.pack)
                    {
                        auto itl = exp->v.sp->tp->templateParam->second->byPack.pack->begin();
                        auto itel = exp->v.sp->tp->templateParam->second->byPack.pack->end();
                        for (int i = 0; i < packIndex && itl != itel; i++, ++itl)
                            ;
                        if (itl != itel)
                            rv = itl->second->byClass.val;
                    }
                    return rv;
                }
                return exp->v.sp->tp->templateParam->second->byClass.val;
            }
            return nullptr;
        case ExpressionNode::templateselector_: {
            EXPRESSION* exp1 = copy_expression(exp);
            optimize_for_constants(&exp1);
            if (exp1->type != ExpressionNode::templateselector_)
                return LookupTypeFromExpression(exp1, enclosing, alt);
            return nullptr;
        }
        // the following several work because the frontend should have cast both expressions already
        case ExpressionNode::cond_: {
            TYPE* tl = LookupTypeFromExpression(exp->right->left, enclosing, alt);
            TYPE* tr = LookupTypeFromExpression(exp->right->right, enclosing, alt);
            if (isarithmetic(tl))
            {
                return destSize(tl, tr, &exp->right->left, &exp->right->right, false, nullptr);
            }
            else
            {
                return tl;
            }
        }
        case ExpressionNode::lsh_:
            return LookupBinaryMathFromExpression(exp, Keyword::leftshift_, enclosing, alt);
        case ExpressionNode::rsh_:
        case ExpressionNode::ursh_:
            return LookupBinaryMathFromExpression(exp, Keyword::rightshift_, enclosing, alt);
        case ExpressionNode::arraymul_:
        case ExpressionNode::arraylsh_:
        case ExpressionNode::arraydiv_:
        case ExpressionNode::arrayadd_:
        case ExpressionNode::structadd_: {
            TYPE* tp1 = LookupTypeFromExpression(exp->left, enclosing, alt);
            if (!tp1)
                return nullptr;
            TYPE* tp1a = tp1;
            if (isref(tp1a))
                tp1a = basetype(tp1a)->btp;
            TYPE* tp2 = LookupTypeFromExpression(exp->right, enclosing, alt);
            if (!tp2)
                return nullptr;
            TYPE* tp2a = tp2;
            if (isref(tp2a))
                tp2a = basetype(tp2a)->btp;
            return destSize(tp1a, tp2a, nullptr, nullptr, false, nullptr);
            break;
        }
        case ExpressionNode::mul_:
        case ExpressionNode::umul_:
            return LookupBinaryMathFromExpression(exp, Keyword::star_, enclosing, alt);
        case ExpressionNode::mod_:
        case ExpressionNode::umod_:
            return LookupBinaryMathFromExpression(exp, Keyword::mod_, enclosing, alt);
        case ExpressionNode::div_:
        case ExpressionNode::udiv_:
            return LookupBinaryMathFromExpression(exp, Keyword::divide_, enclosing, alt);
        case ExpressionNode::and_:
            return LookupBinaryMathFromExpression(exp, Keyword::and_, enclosing, alt);
        case ExpressionNode::or_:
            return LookupBinaryMathFromExpression(exp, Keyword::or_, enclosing, alt);
        case ExpressionNode::xor_:
            return LookupBinaryMathFromExpression(exp, Keyword::uparrow_, enclosing, alt);
        case ExpressionNode::add_:
            return LookupBinaryMathFromExpression(exp, Keyword::plus_, enclosing, alt);
        case ExpressionNode::sub_:
            return LookupBinaryMathFromExpression(exp, Keyword::minus_, enclosing, alt);
        case ExpressionNode::blockclear_:
        case ExpressionNode::stackblock_:
        case ExpressionNode::blockassign_:
            switch (exp->left->type)
            {
                case ExpressionNode::global_:
                case ExpressionNode::auto_:
                case ExpressionNode::labcon_:
                case ExpressionNode::absolute_:
                case ExpressionNode::pc_:
                case ExpressionNode::const_:
                case ExpressionNode::threadlocal_:
                    return exp->left->v.sp->tp;
                default:
                    break;
            }
            if (exp->right)
                switch (exp->right->type)
                {
                    case ExpressionNode::global_:
                    case ExpressionNode::auto_:
                    case ExpressionNode::labcon_:
                    case ExpressionNode::absolute_:
                    case ExpressionNode::pc_:
                    case ExpressionNode::const_:
                    case ExpressionNode::threadlocal_:
                        return exp->right->v.sp->tp;
                    default:
                        break;
                }
            return nullptr;
        case ExpressionNode::thisref_:
        case ExpressionNode::select_:
            return LookupTypeFromExpression(exp->left, enclosing, alt);
        default:
            diag("LookupTypeFromExpression: unknown expression type");
            return nullptr;
    }
}
bool HasUnevaluatedTemplateSelectors(EXPRESSION* exp)
{
    if (exp)
    {
        if (exp->left && HasUnevaluatedTemplateSelectors(exp->left))
            return true;
        if (exp->left && HasUnevaluatedTemplateSelectors(exp->right))
            return true;
        if (exp->type == ExpressionNode::templateselector_)
        {
            optimize_for_constants(&exp);
            return exp->type == ExpressionNode::templateselector_;
        }
    }
    return false;
}
TYPE* TemplateLookupTypeFromDeclType(TYPE* tp)
{
    static int nested;
    if (nested >= 10)
    {
        return nullptr;
    }
    nested++;
    EXPRESSION* exp = tp->templateDeclType;
    auto rv = LookupTypeFromExpression(exp, nullptr, false);
    nested--;
    return rv;
}

static bool hastemplate(EXPRESSION* exp)
{
    if (!exp)
        return false;
    if (exp->type == ExpressionNode::templateparam_ || exp->type == ExpressionNode::templateselector_)
        return true;
    return hastemplate(exp->left) || hastemplate(exp->right);
}
void clearoutDeduction(TYPE* tp)
{
    while (1)
    {
        switch (tp->type)
        {
            case BasicType::pointer_:
                if (isarray(tp) && tp->etype)
                {
                    clearoutDeduction(tp->etype);
                }
                tp = tp->btp;
                break;
            case BasicType::templateselector_:
                clearoutDeduction((*tp->sp->sb->templateSelector)[1].sp->tp);
                return;
            case BasicType::const_:
            case BasicType::volatile_:
            case BasicType::lref_:
            case BasicType::rref_:
            case BasicType::restrict_:
            case BasicType::far_:
            case BasicType::near_:
            case BasicType::seg_:
            case BasicType::lrqual_:
            case BasicType::rrqual_:
            case BasicType::derivedfromtemplate_:
                tp = tp->btp;
                break;
            case BasicType::memberptr_:
                clearoutDeduction(tp->sp->tp);
                tp = tp->btp;
                break;
            case BasicType::func_:
            case BasicType::ifunc_: {
                for (auto sym : *tp->syms)
                {
                    clearoutDeduction(sym->tp);
                }
                tp = tp->btp;
                break;
            }
            case BasicType::templateparam_:
                tp->templateParam->second->byClass.temp = nullptr;
                return;
            default:
                return;
        }
    }
}
int pushContext(SYMBOL* cls, bool all)
{
    STRUCTSYM* s;
    int rv;
    if (!cls)
        return 0;
    rv = pushContext(cls->sb->parentClass, true);
    if (cls->sb->templateLevel)
    {
        s = Allocate<STRUCTSYM>();
        s->tmpl = copyParams(cls->templateParams, false);
        addTemplateDeclaration(s);
        rv++;
    }
    if (all)
    {
        s = Allocate<STRUCTSYM>();
        s->str = cls;
        addStructureDeclaration(s);
        rv++;
    }
    return rv;
}
void SetTemplateNamespace(SYMBOL* sym)
{
    sym->sb->templateNameSpace = symListFactory.CreateList();
    sym->sb->templateNameSpace->insert(sym->sb->templateNameSpace->begin(), nameSpaceList.begin(), nameSpaceList.end());
    sym->sb->templateNameSpace->reverse();  //   will be reversed back when used
}
int PushTemplateNamespace(SYMBOL* sym)
{
    int rv = 0;
    for (auto sp : nameSpaceList)
        sp->sb->value.i++;
    if (sym && sym->sb->templateNameSpace)
    {
        for (auto it = sym->sb->templateNameSpace->begin(); it != sym->sb->templateNameSpace->end(); ++it)
        {
            auto itnext = it;
            ++itnext;
            auto sp = *it;
            if (!sp->sb->value.i || (itnext == sym->sb->templateNameSpace->end() && nameSpaceList.front() != sp))
            {
                if (sp->sb->nameSpaceValues)
                {
                    sp->sb->value.i++;

                    nameSpaceList.push_front(sp);
                    globalNameSpace->push_front(sp->sb->nameSpaceValues->front());

                    rv++;
                }
            }
        }
    }
    return rv;
}
void PopTemplateNamespace(int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        Optimizer::LIST* nlist;
        SYMBOL* sp;
        globalNameSpace->pop_front();
        nameSpaceList.front()->sb->value.i--;
        nameSpaceList.pop_front();
    }
    for (auto sp : nameSpaceList)
        sp->sb->value.i--;
}
static SYMBOL* FindTemplateSelector(std::vector<TEMPLATESELECTOR>* tso)
{
    if (!templateNestingCount)
    {
        SYMBOL* ts = (*tso)[1].sp;
        SYMBOL* sp = nullptr;
        TYPE* tp;

        if (ts && ts->sb && ts->sb->instantiated)
        {
            sp = ts;
        }
        else
        {
            auto tp = ts->tp;
            if (basetype(ts->tp)->type == BasicType::templateparam_ &&
                basetype(ts->tp)->templateParam->second->type == TplType::typename_)
            {
                tp = basetype(ts->tp)->templateParam->second->byClass.val;
            }
            if (!tp || !isstructured(tp))
            {
                sp = nullptr;
            }
            else
            {
                ts = basetype(tp)->sp;
                if ((*tso)[1].isTemplate)
                {
                    if ((*tso)[1].templateParams)
                    {
                        std::list<TEMPLATEPARAMPAIR>* currentx = (*tso)[1].templateParams;
                        std::deque<TYPE*> types;
                        std::deque<EXPRESSION*> expressions;
                        for (auto&& current : *currentx)
                        {
                            if (current.second->type == TplType::typename_)
                            {
                                types.push_back(current.second->byClass.dflt);
                                if (current.second->byClass.val)
                                    current.second->byClass.dflt = current.second->byClass.val;
                            }
                            else if (current.second->type == TplType::int_)
                            {
                                expressions.push_back(current.second->byNonType.dflt);
                                if (current.second->byNonType.val)
                                    current.second->byNonType.dflt = current.second->byNonType.val;
                            }
                        }
                        sp = GetClassTemplate(ts, (*tso)[1].templateParams, false);
                        tp = nullptr;
                        if (sp)
                            sp = TemplateClassInstantiateInternal(sp, (*tso)[1].templateParams, false);
                        for (auto&& current : *currentx)
                        {
                            if (current.second->type == TplType::typename_)
                            {
                                if (types.size())
                                {
                                    current.second->byClass.dflt = types.front();
                                    types.pop_front();
                                }
                            }
                            else if (current.second->type == TplType::int_)
                            {
                                if (expressions.size())
                                {
                                    current.second->byNonType.dflt = expressions.front();
                                    expressions.pop_front();
                                }
                            }
                        }
                    }
                    else
                    {
                        sp = nullptr;
                    }
                }
                else if (basetype(ts->tp)->type == BasicType::templateselector_)
                {
                    sp = nullptr;
                }
                else if (isstructured(ts->tp))
                {
                    sp = ts;
                }
            }
        }
        if (sp)
        {
            sp = basetype(PerformDeferredInitialization(sp->tp, nullptr))->sp;
            if ((sp->sb->templateLevel == 0 || sp->sb->instantiated) &&
                (!sp->templateParams || allTemplateArgsSpecified(sp, sp->templateParams)))
            {
                auto find = tso->begin();
                ++find;
                ++find;
                for (; find != tso->end() && sp; ++find)
                {
                    SYMBOL* spo = sp;
                    if (!isstructured(spo->tp))
                        break;

                    sp = search(spo->tp->syms, find->name);
                    if (!sp)
                    {
                        sp = classdata(find->name, spo, nullptr, false, false);
                        if (sp == (SYMBOL*)-1)
                            sp = nullptr;
                        if (sp && find->isTemplate)
                        {
                            sp = GetClassTemplate(sp, find->templateParams, theCurrentFunc);
                            if (sp)
                                sp->tp = PerformDeferredInitialization(sp->tp, theCurrentFunc);
                        }
                    }
                    if (sp && sp->sb->access != AccessLevel::public_ && !resolvingStructDeclarations)
                    {
                        sp = nullptr;
                        break;
                    }
                }
                if (sp && find == tso->end())
                    return sp;
            }
        }
    }
    return nullptr;
}
static void FixIntSelectors(EXPRESSION** exp)
{
    if ((*exp)->left)
        FixIntSelectors(&(*exp)->left);
    if ((*exp)->right)
        FixIntSelectors(&(*exp)->right);
    if ((*exp)->type == ExpressionNode::templateselector_ ||
        ((*exp)->type == ExpressionNode::construct_ && (*exp)->v.construct.tp->type == BasicType::templateselector_))
    {
        std::list<TEMPLATEPARAMPAIR>* currentx;
        if ((*exp)->type == ExpressionNode::templateselector_)
            currentx = (*(*exp)->v.templateSelector)[1].templateParams;
        else
            currentx = (*(*exp)->v.construct.tp->sp->sb->templateSelector)[1].templateParams;
        std::list<TEMPLATEPARAMPAIR>* orig = currentx;
        std::deque<TYPE*> types;
        std::deque<EXPRESSION*> expressions;
        if (currentx)
        {
            for (auto current : *currentx)
            {
                if (current.second->type == TplType::typename_)
                {
                    types.push_back(current.second->byClass.dflt);
                    if (current.second->byClass.val)
                        current.second->byClass.dflt = current.second->byClass.val;
                }
                else if (current.second->type == TplType::int_)
                {
                    expressions.push_back(current.second->byNonType.dflt);
                    if (current.second->byNonType.val)
                        current.second->byNonType.dflt = current.second->byNonType.val;
                }
            }
        }
        optimize_for_constants(exp);
        if (orig)
        {
            for (auto current : *orig)
            {
                if (current.second->type == TplType::typename_)
                {
                    if (!types.empty())
                    {
                        current.second->byClass.dflt = types.front();
                        types.pop_front();
                    }
                }
                else if (current.second->type == TplType::int_)
                {
                    if (!expressions.empty())
                    {
                        current.second->byNonType.dflt = expressions.front();
                        expressions.pop_front();
                    }
                }
            }
        }
    }
}
static std::list<TEMPLATEPARAMPAIR>* ResolveTemplateSelector(SYMBOL* sp, TEMPLATEPARAMPAIR* arg, bool byVal)
{
    std::list<TEMPLATEPARAMPAIR>* rv = nullptr;
    if (arg)
    {
        bool toContinue = false;
        TYPE* tp;
        if (byVal)
            tp = arg->second->byClass.val;
        else
            tp = arg->second->byClass.dflt;
        if (arg->second->type == TplType::typename_ && tp)
        {
            while (ispointer(tp) || isref(tp))
                tp = basetype(tp)->btp;
            if (basetype(tp)->type == BasicType::templateselector_)
                toContinue = true;
        }
        if (arg->second->type == TplType::int_ && tp)
        {
            if (byVal)
            {
                if (!isintconst(arg->second->byNonType.val) && !isfloatconst(arg->second->byNonType.val))
                    toContinue = true;
            }
            else
            {
                if (!isintconst(arg->second->byNonType.dflt) && !isfloatconst(arg->second->byNonType.dflt))
                    toContinue = true;
            }
        }
        if (toContinue)
        {
            std::vector<TEMPLATESELECTOR>* tso = nullptr;
            TYPE* tp = arg->second->byClass.dflt;
            rv = templateParamPairListFactory.CreateList();
            rv->push_back(TEMPLATEPARAMPAIR{nullptr, nullptr});
            if (arg->second->type == TplType::typename_ && tp)
            {
                while (ispointer(tp) || isref(tp))
                    tp = basetype(tp)->btp;
                if (basetype(tp)->type == BasicType::templateselector_)
                    tso = basetype(tp)->sp->sb->templateSelector;
                if (tso)
                {
                    SYMBOL* sp = FindTemplateSelector(tso);
                    if (sp)
                    {
                        if (istype(sp))
                        {
                            TYPE** txx;
                            rv->back().second = Allocate<TEMPLATEPARAM>();
                            *rv->back().second = *arg->second;
                            rv->back().first = arg->first;
                            if (byVal)
                            {
                                txx = &rv->back().second->byClass.val;
                            }
                            else
                            {
                                txx = &rv->back().second->byClass.dflt;
                                rv->back().second->byClass.val = nullptr;
                            }
                            *txx = CopyType(arg->second->byClass.dflt, true, [sp, tso](TYPE*& old, TYPE*& newx) {
                                if (newx->type == BasicType::templateselector_)
                                {
                                    newx = sp->tp;
                                    if (isstructured(newx) && !templateNestingCount && basetype(newx)->sp->sb->templateLevel &&
                                        !basetype(newx)->sp->sb->instantiated)
                                    {
                                        SYMBOL* sp1 = basetype(newx)->sp;
                                        sp1 = GetClassTemplate((*tso)[1].sp, sp1->templateParams, false);
                                    }
                                }
                            });

                            UpdateRootTypes(byVal ? rv->back().second->byClass.val : rv->back().second->byClass.dflt);
                        }
                        else
                        {
                            rv->back().second = arg->second;
                            rv->back().first = arg->first;
                        }
                    }
                    else
                    {
                        rv->back().second = arg->second;
                        rv->back().first = arg->first;
                    }
                }
                else
                {
                    rv->back().second = arg->second;
                    rv->back().first = arg->first;
                }
            }
            else if (arg->second->type == TplType::int_ && tp)
            {
                rv->back().second = Allocate<TEMPLATEPARAM>();
                *rv->back().second = *arg->second;
                rv->back().first = arg->first;
                if (arg->second->packed)
                {
                    auto packed = arg->second->byPack.pack;
                    if (packed)
                    {
                        auto newv = arg->second->byPack.pack = templateParamPairListFactory.CreateList();
                        for (auto tpx : *packed)
                        {
                            newv->push_back(TEMPLATEPARAMPAIR(tpx.first, Allocate<TEMPLATEPARAM>()));
                            newv->back().second = Allocate<TEMPLATEPARAM>();
                            *newv->back().second = *tpx.second;
                        }
                    }
                }
                else
                {
                    rv->back().second->byNonType.dflt = copy_expression(arg->second->byNonType.dflt);
                    FixIntSelectors(&rv->back().second->byNonType.dflt);
                    optimize_for_constants(&rv->back().second->byNonType.dflt);
                }
            }
            else
            {
                rv->back().second = arg->second;
                rv->back().first = arg->first;
            }
        }
    }
    return rv;
}
static std::list<TEMPLATEPARAMPAIR>* CopyArgsBack(std::list<TEMPLATEPARAMPAIR>* args, TEMPLATEPARAMPAIR* hold[], int k1)
{
    auto orig = args;
    int k = 0;
    std::list<TEMPLATEPARAMPAIR>* rv = args;
    std::list<TEMPLATEPARAMPAIR>::iterator t;
    std::list<TEMPLATEPARAMPAIR>::iterator te = t;
    std::stack<std::list<TEMPLATEPARAMPAIR>::iterator> tas;
    if (args)
    {
        t = args->begin();
        te = args->end();
        for (; t != te;)
        {
            if (t->second->packed)
            {
                if (t->second->byPack.pack && t->second->byPack.pack->size())
                {
                    tas.push(t);
                    tas.push(te);
                    te = t->second->byPack.pack->end();
                    t = t->second->byPack.pack->begin();
                }
                else
                {
                    ++t;
                    continue;
                }
            }
            if (hold[k++] != &*t)
                break;
            ++t;
            if (t == te && !tas.empty())
            {
                if (hold[k++] != nullptr)
                    break;
                te = tas.top();
                tas.pop();
                t = tas.top();
                tas.pop();
                ++t;
            }
        }
    }
    if (t != te)
    {
        k = 0;
        rv = templateParamPairListFactory.CreateList();
        for (auto&& old : *args)
        {
            if (old.second->packed)
            {
                rv->push_back(TEMPLATEPARAMPAIR{old.first, Allocate<TEMPLATEPARAM>()});
                *rv->back().second = *old.second;
                rv->back().first = old.first;
                rv->back().second->byPack.pack = templateParamPairListFactory.CreateList();
                while (hold[k] != nullptr)
                {
                    rv->back().second->byPack.pack->push_back(*hold[k++]);
                }
                k++;
            }
            else
            {
                rv->push_back(*hold[k++]);
            }
        }
        // this should be looked at.   It is part of the problem with GetClassTemplate modifying the input argument list
        if (rv->begin()->second->type == TplType::new_)
        {
            auto t1 = args->begin();
            ++t1;
            if (&*t1 == hold[1])
            {
                *args = *rv;
                rv = args;
            }
        }
        else
        {
            if (&*args->begin() == hold[0])
            {
                *args = *rv;
                rv = args;
            }
        }
    }
    return rv;
}
std::list<TEMPLATEPARAMPAIR>* ResolveTemplateSelectors(SYMBOL* sp, std::list<TEMPLATEPARAMPAIR>* args, bool byVal)
{
    std::stack<std::list<TEMPLATEPARAMPAIR>::iterator> tas;
    int k = 0;
    TEMPLATEPARAMPAIR* hold[200];
    if (args)
    {
        auto t = args->begin();
        auto te = args->end();
        for (; t != te;)
        {
            if (t->second->packed)
            {
                if (t->second->byPack.pack && t->second->byPack.pack->size())
                {
                    tas.push(t);
                    tas.push(te);
                    te = t->second->byPack.pack->end();
                    t = t->second->byPack.pack->begin();
                }
                else
                {
                    hold[k++] = nullptr;
                    ++t;
                    continue;
                }
            }
            auto a = ResolveTemplateSelector(sp, &*t, byVal);
            hold[k++] = a ? &*a->begin() : &*t;
            ++t;
            if (t == te && !tas.empty())
            {
                hold[k++] = nullptr;
                te = tas.top();
                tas.pop();
                t = tas.top();
                tas.pop();
                ++t;
            }
        }
    }
    return CopyArgsBack(args, hold, k);
}
TYPE* ResolveTemplateSelectors(SYMBOL* sp, TYPE* tp)
{
    TEMPLATEPARAM tpa = {};
    tpa.type = TplType::typename_;
    tpa.byClass.dflt = tp;
    std::list<TEMPLATEPARAMPAIR> tpx = {{nullptr, &tpa}};
    auto tpl2 = ResolveTemplateSelectors(sp, &tpx, false);
    return tpl2->front().second->byClass.dflt;
    ;
}
std::list<TEMPLATEPARAMPAIR>* ResolveDeclType(SYMBOL* sp, TEMPLATEPARAMPAIR* tpx, bool returnNull)
{
    if (tpx->second->type == TplType::typename_ && tpx->second->byClass.dflt &&
        tpx->second->byClass.dflt->type == BasicType::templatedecltype_)
    {
        std::list<TEMPLATEPARAMPAIR>* rv = templateParamPairListFactory.CreateList();
        rv->push_back(TEMPLATEPARAMPAIR{tpx->first, Allocate<TEMPLATEPARAM>()});
        *rv->back().second = *tpx->second;
        rv->back().second->byClass.dflt = TemplateLookupTypeFromDeclType(rv->back().second->byClass.dflt);
        if (!rv->back().second->byClass.dflt)
            rv->back().second->byClass.dflt = &stdany;
        return rv;
    }
    else
    {
        if (returnNull)
            return nullptr;
        std::list<TEMPLATEPARAMPAIR>* rv = templateParamPairListFactory.CreateList();
        rv->push_back(*tpx);
        return rv;
    }
}
std::list<TEMPLATEPARAMPAIR>* ResolveDeclTypes(SYMBOL* sp, std::list<TEMPLATEPARAMPAIR>* args)
{
    if (!templateNestingCount)
    {
        std::stack<std::list<TEMPLATEPARAMPAIR>::iterator> tas;
        STRUCTSYM s;
        s.tmpl = args;
        addTemplateDeclaration(&s);
        int k = 0;
        TEMPLATEPARAMPAIR* hold[200];
        if (args)
        {
            auto t = args->begin();
            auto te = args->end();
            for (; t != te;)
            {
                if (t->second->packed)
                {
                    if (t->second->byPack.pack && t->second->byPack.pack->size())
                    {
                        tas.push(t);
                        tas.push(te);
                        te = t->second->byPack.pack->end();
                        t = t->second->byPack.pack->begin();
                    }
                    else
                    {
                        hold[k++] = nullptr;
                        ++t;
                        continue;
                    }
                }
                hold[k] = &*t;
                auto t1 = ResolveDeclType(sp, &*t, true);
                if (t1)
                    hold[k] = &*t1->begin();
                k++;
                ++t;
                if (t == te && !tas.empty())
                {
                    hold[k++] = nullptr;
                    te = tas.top();
                    tas.pop();
                    t = tas.top();
                    tas.pop();
                    ++t;
                }
            }
        }
        dropStructureDeclaration();
        return CopyArgsBack(args, hold, k);
    }
    return args;
}
static std::list<TEMPLATEPARAMPAIR>* ResolveConstructor(SYMBOL* sym, TEMPLATEPARAMPAIR* tpx)
{
    std::list<TEMPLATEPARAMPAIR>* rv = nullptr;
    if (tpx->second->type == TplType::int_ && tpx->second->byNonType.dflt &&
        tpx->second->byNonType.dflt->type == ExpressionNode::construct_)
    {
        rv = templateParamPairListFactory.CreateList();
        rv->push_back(TEMPLATEPARAMPAIR{tpx->first, Allocate<TEMPLATEPARAM>()});
        *rv->back().second = *tpx->second;
        if (rv->back().second->byNonType.dflt->v.construct.tp->type == BasicType::templateselector_)
        {
            SYMBOL* sp = FindTemplateSelector(rv->back().second->byNonType.dflt->v.construct.tp->sp->sb->templateSelector);
            if (sp)
                rv->back().second->byNonType.dflt->v.construct.tp = sp->tp;
        }
        optimize_for_constants(&rv->back().second->byNonType.dflt);
    }
    return rv;
}

std::list<TEMPLATEPARAMPAIR>* ResolveClassTemplateArgs(SYMBOL* sp, std::list<TEMPLATEPARAMPAIR>* args)
{
    std::list<TEMPLATEPARAMPAIR>* rv = args;
    std::stack<std::list<TEMPLATEPARAMPAIR>::iterator> tas;
    TEMPLATEPARAMPAIR* hold[200];
    int k = 0;
    if (args)
    {
        auto t = args->begin();
        auto te = args->end();
        for (; t != te;)
        {
            bool ellipsis = t->second->ellipsis;
            if (t->second->packed)
            {
                if (t->second->byPack.pack && t->second->byPack.pack->size())
                {
                    tas.push(t);
                    tas.push(te);
                    te = t->second->byPack.pack->end();
                    t = t->second->byPack.pack->begin();
                }
                else
                {
                    hold[k++] = nullptr;
                    ++t;
                    continue;
                }
            }
            int count = 0, n = 0;
            SYMBOL* syms[200];
            if (ellipsis)
            {
                if (t->second->type == TplType::int_)
                {
                    if (t->second->packed)
                    {
                        if (t->second->byPack.pack)
                            for (auto tpx : *t->second->byPack.pack)
                                GatherPackedVars(&count, syms, tpx.second->byNonType.dflt);
                    }
                    else
                    {
                        GatherPackedVars(&count, syms, t->second->byNonType.dflt);
                    }
                }
                else if (t->second->type == TplType::typename_)
                {
                    if (t->second->packed)
                    {
                        if (t->second->byPack.pack)
                            for (auto&& tpx : *t->second->byPack.pack)
                                GatherPackedTypes(&count, syms, tpx.second->byClass.dflt);
                    }
                    else
                    {

                        GatherPackedTypes(&count, syms, t->second->byClass.dflt);
                    }
                }
                for (int i = 0; i < count; i++)
                {
                    auto rv = TypeAliasSearch(syms[i]->name, false);
                    if (rv && rv->second->packed)
                    {
                        int n1 = CountPacks(rv->second->byPack.pack);
                        if (n1 > n)
                            n = n1;
                    }
                }
            }
            n--;
            int oldIndex = packIndex;
            for (int i = n < 0 ? -1 : 0; i <= n; i++)
            {
                if (n >= 0)
                    packIndex = i;
                hold[k] = &*t;
                auto t1 = ResolveDeclType(sp, &*hold[k], true);
                if (t1)
                    hold[k] = &*t1->begin();
                auto t2 = ResolveTemplateSelector(sp, &*hold[k], false);
                if (t2)
                    hold[k] = &*t2->begin();
                auto t3 = ResolveConstructor(sp, &*hold[k]);
                if (t3)
                    hold[k] = &*t3->begin();
                k++;
            }
            packIndex = oldIndex;
            ++t;
            if (t == te && !tas.empty())
            {
                hold[k++] = nullptr;
                te = tas.top();
                tas.pop();
                t = tas.top();
                tas.pop();
                ++t;
            }
        }
    }
    return CopyArgsBack(args, hold, k);
}
void copySyms(SYMBOL* found1, SYMBOL* sym)
{
    auto src = sym->templateParams;
    auto dest = found1->templateParams;
    auto itsrc = src->begin();
    auto itdest = dest->begin();
    for (; itsrc != src->end() && itdest != dest->end(); ++itsrc, ++itdest)
    {
        if (itsrc->second->type != TplType::new_)
        {
            SYMBOL* hold = itdest->first;
            TYPE* tp = CopyType(itsrc->first->tp);
            itdest->first = CopySymbol(itsrc->first);
            itdest->first->tp = tp;
            if (hold)
            {
                itdest->first->name = hold->name;
            }
            UpdateRootTypes(itdest->first->tp);
            itdest->first->tp->templateParam = &*itdest;
        }
    }
}

}  // namespace Parser