# This file is part of rasdaman community.
#
# Rasdaman community is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Rasdaman community is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with rasdaman community.  If not, see <http://www.gnu.org/licenses/>.
#
# Copyright 2003-2016 Peter Baumann /
# rasdaman GmbH.
#
# For more information please see <http://www.rasdaman.org>
# or contact Peter Baumann via <baumann@rasdaman.com>.

# Try to find the AStyle executable.
# Once done this will define
# AStyle_FOUND - System has AStyle executable.
# AStyle_EXECUTABLE - The AStyle executable.


find_program(AStyle_EXECUTABLE NAMES astyle)

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set AStyle_FOUND to TRUE if all listed variables are TRUE
find_package_handle_standard_args(AStyle DEFAULT_MSG AStyle_EXECUTABLE)

mark_as_advanced(AStyle_EXECUTABLE)