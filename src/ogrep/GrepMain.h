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

#ifndef GREPMAIN_H
#define GREPMAIN_H
#include "RegExp.h"
#include "CmdSwitch.h"

class GrepMain
{
public:

    int Run(int argc, char **argv);
    void SetModes();
    void DisplayMatch(const std::string &fileName, int &matchCount, int lineno, const char *text);
    void FindLine(const std::string fileName, int &matchCount, int &matchLine, char **matchPos, char *curpos, bool matched);
    void OneFile(RegExpContext &regexp, const std::string &fileName, int &openCount);
private:
    
    static CmdSwitchParser SwitchParser;

    static CmdSwitchBool recurseDirs;
    static CmdSwitchBool caseInSensitive;
    static CmdSwitchBool completeWords;
    static CmdSwitchBool regularExpressions;	
    static CmdSwitchBool displayMatchCount;
    static CmdSwitchBool displayFileNamesOnly;
    static CmdSwitchBool displayLineNumbers;
    static CmdSwitchBool unixMode;
    static CmdSwitchBool displayNonMatching;
    static CmdSwitchBool verboseMode;
    static CmdSwitchBool showHelp;
    // not actual parameters
    static CmdSwitchBool displayFileNames;
    static CmdSwitchBool displayHeaderFileName;
    
    static char *usageText;
    static char *helpText;
} ;
#endif
