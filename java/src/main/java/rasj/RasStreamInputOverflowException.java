package rasj;

import rasj.global.*;

/*
* This file is part of rasdaman community.
*
* Rasdaman community is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Rasdaman community is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with rasdaman community.  If not, see <http://www.gnu.org/licenses/>.
*
* Copyright 2003, 2004, 2005, 2006, 2007, 2008, 2009 Peter Baumann /
rasdaman GmbH.
*
* For more information please see <http://www.rasdaman.org>
* or contact Peter Baumann via <baumann@rasdaman.com>.
*/
/** ***********************************************************
 * <pre>
 *
 * PURPOSE:
 * This exception is thrown if an initialization overflow has occured.
 * It typically arises when a stream input operator is invoked more
 * often than the object has dimensions.
 * @version $Revision: 1.5 $
 *
 *
 *
 * COMMENTS:
 *
 * </pre>
 *********************************************************** */


public class RasStreamInputOverflowException extends RasException {
    
    /**
     * Standard constructor.
     */
    protected RasStreamInputOverflowException() {
        super(RasGlobalDefs.STREAM_INPUT_OVERFLOW);
    }

    /**
     * Returns the error message.
     * @return the error message.
     **/
    public String getMessage() {
        if (super.getMessage() == null) {
            return RasErrorTexts.getErrorMessage(errNo);
        } else {
            return super.getMessage();
        }
    }

}

