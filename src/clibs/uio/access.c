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

#include <io.h>
#include <sys/stat.h>
#include <errno.h>
int   _RTL_FUNC access(const char *__file, int __level)
{
	struct stat stat_st;
	int mode = 0;
	if (stat(__file, &stat_st) == -1)
		return -1;
	if (__level == F_OK || S_ISDIR(stat_st.st_mode))
		return 0;
	if (__level & R_OK)
		mode |= S_IRUSR;
	if (__level & W_OK)
		mode |= S_IWUSR;
	if (__level & X_OK)
		mode |= S_IXUSR;
	if (stat_st.st_mode & mode)
		return 0;
	errno = EACCES;
	return -1;
}
int   _RTL_FUNC _access(const char *__file, int __level)
{
	return access(__file, __level);
}
