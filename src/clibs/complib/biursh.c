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
#include "bi.h"

void __stdcall _RTL_FUNC ___biursh(UNDERLYING_TYPE* ans, UNDERLYING_TYPE* left, int bits, int shift)
{
    const int words = WORDS(bits);
    MASK(left, bits);
    if (shift >= bits)
    {
        memset(ans, 0, words * sizeof(UNDERLYING_TYPE));
    }
    else
    {
        const int ofswords = shift/(sizeof(UNDERLYING_TYPE) * CHAR_BIT);
        const int ofsbits = shift %(sizeof(UNDERLYING_TYPE) * CHAR_BIT);
        const int mibits = sizeof(UNDERLYING_TYPE) * CHAR_BIT - ofsbits;
        if (ofsbits == 0)
        {
            for (int i = 0; i < words; i++)
            {
                int n = i + ofswords;
                if (n < words)
                {
                    ans[i] = left[n];       
                }
                else
                {
                    ans[i] = 0;
                }
            }
        }
        else
        {
            for (int i = 0; i < words; i++)
            {
                int n = i + ofswords;
                if (n < words)
                {
                    ans[i] = left[n] >> ofsbits;
                    if (n < words - 1)
                         ans[i] |= left[n+1] << mibits;
                }
                else
                {
                    ans[i] = 0;
                }
            }
        }
    }
}
