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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU  General Public License for more details.
 *
 * You should have received a copy of the GNU  General Public License
 * along with rasdaman community.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2003 - 2018 Peter Baumann / rasdaman GmbH.
 *
 * For more information please see <http://www.rasdaman.org>
 * or contact Peter Baumann via <baumann@rasdaman.com>.
 */
package petascope.wcps.handler;

import org.springframework.stereotype.Service;
import petascope.util.CrsUtil;
import petascope.wcps.result.WcpsMetadataResult;
import petascope.wcps.result.WcpsResult;

/**
 * Translator class for imageCrs (CRS:1).  <code>
 * for c in (mr), d in (rgb) return imageCrs(c)
 * </code> translates to  <code>
 * Grid CRS (CRS:1)
 * </code>
 *
 * @author <a href="mailto:bphamhuu@jacobs-university.de">Bang Pham Huu</a>
 */
@Service
public class ImageCrsExpressionHandler extends AbstractOperatorHandler {

    /**
     * Return the Rasql grid CRS of the coverage (CRS:1)
     *
     * @param coverageExpression
     * @return
     */
    public WcpsMetadataResult handle(WcpsResult coverageExpression) {
        String imageCrsUri = CrsUtil.GRID_CRS;
        WcpsMetadataResult wcpsResult = new WcpsMetadataResult(coverageExpression.getMetadata(), imageCrsUri);
        return wcpsResult;
    }
}
