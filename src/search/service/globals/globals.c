/**
 * @file search/service/globals/globals.c
 * @author Julian Kranz
 * @date 7.8.2012
 *
 * @brief This file contains all functions pertaining to the GNUnet Search service's globals component.
 *
 * \latexonly \\ \\ \endlatexonly
 * \em Detailed \em description \n
 * This file contains all functions pertaining to the GNUnet Search service's globals component. This component
 * stores all globally used data.
 */
/*
 *  This file is part of GNUnet Search.
 *
 *  GNUnet Search is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  GNUnet Search is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNUnet Search.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "globals.h"

/**
 * @brief This variable stores a reference to the GNUnet configuration.
 */
struct GNUNET_CONFIGURATION_Handle const *gnunet_search_globals_cfg;
