#
# copyright : (c) 2010 Maxime Lenoir, Alain Coulais,
#                      Sylwester Arabas and Orion Poplawski
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#

find_library(UDUNITS_LIBRARIES NAMES udunits2)
find_path(UDUNITS_INCLUDE_DIR NAMES udunits2.h)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDUNITS DEFAULT_MSG UDUNITS_LIBRARIES UDUNITS_INCLUDE_DIR)

mark_as_advanced(
UDUNITS_LIBRARIES
UDUNITS_INCLUDE_DIR
)
