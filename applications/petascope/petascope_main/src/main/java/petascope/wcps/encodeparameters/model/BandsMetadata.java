/*
  *  This file is part of rasdaman community.
  * 
  *  Rasdaman community is free software: you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation, either version 3 of the License, or
  *  (at your option) any later version.
  * 
  *  Rasdaman community is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  *  See the GNU  General Public License for more details.
  * 
  *  You should have received a copy of the GNU  General Public License
  *  along with rasdaman community.  If not, see <http://www.gnu.org/licenses/>.
  * 
  *  Copyright 2003 - 2018 Peter Baumann / rasdaman GmbH.
  * 
  *  For more information please see <http://www.rasdaman.org>
  *  or contact Peter Baumann via <baumann@rasdaman.com>.
 */
package petascope.wcps.encodeparameters.model;

import com.fasterxml.jackson.annotation.JsonAnyGetter;
import com.fasterxml.jackson.annotation.JsonAnySetter;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * A class to represent the bands's metadata (bands element) inside gmlcov:metadata element
 * @author <a href="mailto:b.phamhuu@jacobs-university.de">Bang Pham Huu</a>
 */
public class BandsMetadata {
    
    public static final String BANDS_METADATA_ROOT_TAG = "bands";
    
    private Map<String, Map<String, String>> bandsAttributesMap;
    
    public BandsMetadata() {
        this.bandsAttributesMap = new LinkedHashMap<>();
    }

    @JsonAnyGetter
    // Unwrap this map
    public Map<String, Map<String, String>> getBandsAttributesMap() {
        return bandsAttributesMap;
    }

    public void setBandsAttributesMap(Map<String, Map<String, String>> bandsAttributesMap) {
        this.bandsAttributesMap = bandsAttributesMap;
    }
    
    @JsonAnySetter
    public void addKeyValue(String key, Map<String, String> values) {
        this.bandsAttributesMap.put(key, values);
    }
}
