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
package petascope.wcps.metadata.service;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.rasdaman.domain.cis.*;
import org.rasdaman.domain.cis.Coverage;
import org.rasdaman.repository.service.CoverageRepositoryService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import petascope.core.CrsDefinition;
import petascope.exceptions.PetascopeException;
import petascope.exceptions.SecoreException;
import petascope.core.Pair;
import petascope.service.PyramidService;
import petascope.util.BigDecimalUtil;
import petascope.util.CrsUtil;
import petascope.util.ListUtil;
import static petascope.wcps.handler.GeneralCondenserHandler.USING;
import petascope.wcps.metadata.model.RangeField;
import petascope.wcps.metadata.model.WcpsCoverageMetadata;
import petascope.wcps.metadata.model.Axis;
import petascope.wcps.metadata.model.IrregularAxis;
import petascope.wcps.metadata.model.NumericSubset;
import petascope.wcps.metadata.model.NumericTrimming;
import petascope.wcps.metadata.model.ParsedSubset;
import petascope.wcps.metadata.model.RegularAxis;
import petascope.wcps.metadata.model.Subset;
import petascope.wcps.result.WcpsResult;

/**
 * This class translates different types of metadata into WcpsCoverageMetadata.
 *
 * @author <a href="mailto:b.phamhuu@jacobs-university.de">Bang Pham Huu</a>
 * @author <a href="merticariu@rasdaman.com">Vlad Merticariu</a>
 */
@Service
public class WcpsCoverageMetadataTranslator {

    @Autowired
    private CoverageRepositoryService persistedCoverageService;
    @Autowired
    private PyramidService pyramidService;
    @Autowired
    private CoordinateTranslationService coordinateTranslationService;
    @Autowired
    private CoverageAliasRegistry coverageAliasRegistry;


    public WcpsCoverageMetadataTranslator() {

    }
    
    /**
     * Create a WCPS coverage metadata object from a Coverage CIS 1.1 model in
     * database.
     *
     * @param coverageId
     * @return
     * @throws PetascopeException
     * @throws SecoreException
     */
    public WcpsCoverageMetadata create(String coverageId) throws PetascopeException, SecoreException {
        // Only supports GeneralGridCoverage now
        Coverage coverage = this.persistedCoverageService.readCoverageFullMetadataByIdFromCache(coverageId);
        List<GeoAxis> geoAxes = ((GeneralGridCoverage) coverage).getGeoAxes();
        List<IndexAxis> indexAxes = ((GeneralGridCoverage) coverage).getIndexAxes();

        // wcpsCoverageMetadata axis
        String coverageCRS = coverage.getEnvelope().getEnvelopeByAxis().getSrsName();
        List<Axis> axes = buildAxes(coverageCRS, geoAxes, indexAxes);
        List<Axis> originalAxes = buildAxes(coverageCRS, geoAxes, indexAxes);
        List<RangeField> rangeFields = buildRangeFields(coverage.getRangeType().getDataRecord().getFields());
        // parse extra metadata of coverage to map
        String extraMetadata = coverage.getMetadata();
        List<NilValue> nilValues = coverage.getAllUniqueNullValues();
        
        String rasdamanCollectionName = coverage.getRasdamanRangeSet().getCollectionName();
        
        List<String> axisCrsUris = new ArrayList<>();
        for (Axis axis : axes) {
            axisCrsUris.add(axis.getNativeCrsUri());
        }
        
        String crsUri = CrsUtil.CrsUri.createCompound(axisCrsUris);

        WcpsCoverageMetadata wcpsCoverageMetadata = new WcpsCoverageMetadata(coverageId, rasdamanCollectionName,
                                                        coverage.getCoverageType(), axes, crsUri, 
                                                        rangeFields, nilValues, extraMetadata, originalAxes);

        return wcpsCoverageMetadata;
    }

    /**
     * Translate a persisted coverage in database by coverageId to a
     * WcpsCoverageMetadata. NOTE: if a WcpsCoverageMetadata is already
     * translated, it will get from cache.
     *
     * @param coverageId
     * @return
     * @throws petascope.exceptions.PetascopeException
     * @throws petascope.exceptions.SecoreException
     */
    public WcpsCoverageMetadata translate(String coverageId) throws PetascopeException, SecoreException {
        // NOTE: cannot cache a translated WCPS coverage metadata as a WCPS request can slice, trim to the coverage metadata which is stored in cache
        // and the next request will not have the full metadata as the previous one which is big error.
        WcpsCoverageMetadata wcpsCoverageMetadata = this.create(coverageId);

        return wcpsCoverageMetadata;
    }
    
    /**
     * Update geo-referenced axis's geo bounds and grid bounds by downscaled level.
     */
    private void updateAxisBounds(Axis axis, Pair<BigDecimal, BigDecimal> geoSubsetPair, BigDecimal downscaledLevel) {
        BigDecimal scaleRatio = BigDecimalUtil.divide(BigDecimal.ONE, downscaledLevel);
        BigDecimal axisResolutionTmp = axis.getResolution().multiply(downscaledLevel);

        BigDecimal newGridLowerBound = new BigDecimal(axis.getGridBounds().getLowerLimit().multiply(scaleRatio).longValue());
        BigDecimal newGridUpperBound = new BigDecimal(axis.getGridBounds().getUpperLimit().multiply(scaleRatio).longValue());
        BigDecimal newAxisResolution = axisResolutionTmp;
        axis.setResolution(newAxisResolution);

        NumericSubset newOriginalGridBounds = new NumericTrimming(newGridLowerBound, newGridUpperBound);
        axis.setOriginalGridBounds(newOriginalGridBounds);
        
        ParsedSubset<BigDecimal> parsedSubset = new ParsedSubset<>(geoSubsetPair.fst, geoSubsetPair.snd);
        ParsedSubset<Long> gridSubset = coordinateTranslationService.geoToGridForRegularAxis(parsedSubset, 
                                                                        axis.getGeoBounds().getLowerLimit(), axis.getGeoBounds().getUpperLimit(), 
                                                                        newAxisResolution, newGridLowerBound);
        NumericSubset geoSubset = new NumericTrimming(geoSubsetPair.fst, geoSubsetPair.snd);
        axis.setGeoBounds(geoSubset);
        NumericSubset gridBound = new NumericTrimming(new BigDecimal(gridSubset.getLowerLimit().toString()), new BigDecimal(gridSubset.getUpperLimit().toString()));
        axis.setGridBounds(gridBound);
    }
    
    /**
     * If a 2D coverage imported with downscaled collections, then it can scale the grid domains on the first parameter of scale()
     * by the value of selected downscale collection and update coverage name in FROM clause to downscaled collection name.
     * Based on width and height, calculate the downscaledLevel should be applied on grid XY bounds of original WCPS coverage object
     * e.g: Lat axis with grid bound (0:10) will change to (0/2:10/2) = (0:5)
     */
    public void applyDownscaledLevelOnXYGridAxesForScale(WcpsResult coverageExpression, 
                                                         WcpsCoverageMetadata metadata, List<Subset> numericSubsets) throws PetascopeException, SecoreException {
        
        if (metadata.containsOnlyXYAxes()) {
            int width = numericSubsets.get(0).getNumericSubset().getUpperLimit().toBigInteger().intValue() - numericSubsets.get(0).getNumericSubset().getLowerLimit().toBigInteger().intValue();
            int height = numericSubsets.get(1).getNumericSubset().getUpperLimit().toBigInteger().intValue() - numericSubsets.get(1).getNumericSubset().getLowerLimit().toBigInteger().intValue();
        
            List<Axis> xyAxes = metadata.getXYAxes();

            Pair<BigDecimal, BigDecimal> geoSubsetX = new Pair<>(xyAxes.get(0).getGeoBounds().getLowerLimit(), xyAxes.get(0).getGeoBounds().getUpperLimit());
            Pair<BigDecimal, BigDecimal> geoSubsetY = new Pair<>(xyAxes.get(1).getGeoBounds().getLowerLimit(), xyAxes.get(1).getGeoBounds().getUpperLimit());
            Coverage coverage = this.persistedCoverageService.readCoverageFullMetadataByIdFromCache(metadata.getCoverageName());
            BigDecimal downscaledLevel = this.pyramidService.getDownscaledLevel(coverage, geoSubsetX, geoSubsetY, width, height);
            
            // If no imported downscaled collection, then no need to adjust anything
            if (!downscaledLevel.equals(BigDecimal.ONE)) {

                String coverageAlias = this.coverageAliasRegistry.getAliasByCoverageName(metadata.getCoverageName());
                String downscaledCollectionName = this.pyramidService.createDownscaledCollectionName(metadata.getRasdamanCollectionName(), downscaledLevel);
                this.coverageAliasRegistry.updateCoverageMapping(coverageAlias, metadata.getCoverageName(), downscaledCollectionName);

                String translatedRasqlScaleFirstParameter = coverageExpression.getRasql();
                // In case of using condenser as first paramter of scale(), it should not downscale domain of iterators
                int indexOfUsing = translatedRasqlScaleFirstParameter.indexOf(USING);
                String result = "";
                for (int i = 0; i < translatedRasqlScaleFirstParameter.length(); i++){
                    char c = translatedRasqlScaleFirstParameter.charAt(i);
                    String value = String.valueOf(c);
                    int numberOfOpenBrackets = 0;
                    if (c == '[') {
                        value = "";
                        numberOfOpenBrackets++;
                        for (int j = i + 1; j < translatedRasqlScaleFirstParameter.length(); j++) {
                            char d = translatedRasqlScaleFirstParameter.charAt(j);
                            if (d == '[') {
                                numberOfOpenBrackets++;
                            }
                            if (d == ']') {
                                numberOfOpenBrackets--;
                                i = j;
                                if (numberOfOpenBrackets == 0) {
                                    break;
                                }
                            }
                            value += d;
                        }

                        // e.g: CONDENSE min OVER ts in [0:5]  USING c[ts[0],0:99,0:99], only update grid domains after USING (not [0:5])
                        if (indexOfUsing < i) {
                            List<String> boundsList = new ArrayList<>();
                            String[] parts = value.split(",");
                            for (String part : parts) {
                                String updatedPart = part;
                                if (part.contains(":")) {
                                    String[] bounds = part.split(":");
                                    BigDecimal lowerBound = new BigDecimal(bounds[0].trim());
                                    BigDecimal upperBound = new BigDecimal(bounds[1].trim());
                                    String appliedLowerBound = BigDecimalUtil.divide(lowerBound, downscaledLevel).toBigInteger().toString();
                                    String appliedUpperBound = BigDecimalUtil.divide(upperBound, downscaledLevel).toBigInteger().toString();
                                    updatedPart = appliedLowerBound + ":" + appliedUpperBound;
                                }
                                boundsList.add(updatedPart);
                            }
                            value = ListUtil.join(boundsList, ",");
                        }
                        value = "[" + value + "]";
                    }
                    result += value;
                }
                 coverageExpression.setRasql(result);
            }
        }
    }
    
    /**
     * Create a WcpsCoverageMetadata metadata object depending on the input GeoXY axes domains. 
     * This method will check which Rasdaman downscaled collection the metadata object should contain and also the XY grid domains accordingly.
     * 
     * NOTE: It needs to select a suitable downscaled collection based on geo XY subsets to help reduce the time to process Rasql on lower resolution collection.
     */
    public WcpsCoverageMetadata createForDownscaledLevelByGeoXYSubsets(WcpsCoverageMetadata metadata, 
            Pair<BigDecimal, BigDecimal> geoSubsetX, Pair<BigDecimal, BigDecimal> geoSubsetY, Integer width, Integer height) throws PetascopeException, SecoreException {
        
        Coverage coverage = this.persistedCoverageService.readCoverageFullMetadataByIdFromCache(metadata.getCoverageName());
        
        WcpsCoverageMetadata newMetadata = metadata;
        String collectionName = coverage.getRasdamanRangeSet().getCollectionName();
        // Depend on the geo XY axes subsets, select a suitable downscaled level (it must be the lowest level which is valid for both X and Y axes).
        BigDecimal downscaledLevel = this.pyramidService.getDownscaledLevel(coverage, geoSubsetX, geoSubsetY, width, height);
        if (downscaledLevel.compareTo(BigDecimal.ONE) > 0) {
            collectionName = this.pyramidService.createDownscaledCollectionName(collectionName, downscaledLevel);
        }
        
        for (int i = 0; i < newMetadata.getAxes().size(); i++) {
            Axis axis = newMetadata.getAxes().get(i);
            
            if (axis.isXYGeoreferencedAxis()) {
                if (axis.isXAxis()) {
                    this.updateAxisBounds(axis, geoSubsetX, downscaledLevel);
                } else {
                    this.updateAxisBounds(axis, geoSubsetY, downscaledLevel);
                }
                    
                axis = new RegularAxis(axis.getLabel(), axis.getGeoBounds(), axis.getOriginalGridBounds(), axis.getGridBounds(),
                                       axis.getNativeCrsUri(), axis.getCrsDefinition(), axis.getAxisType(), axis.getAxisUoM(), 
                                       axis.getRasdamanOrder(), axis.getOrigin(), axis.getResolution(), axis.getOriginalGeoBounds());
                
                // Replace the old geo axis with new geo axis.
                newMetadata.updateAxisByIndex(i, axis);
            }
        }
        // If a downscaled collection is selected, metadata object should use this one for other processes.
        newMetadata.setRasdamanCollectionName(collectionName);
        
        return newMetadata;
    }
    

    /**
     * Build list of RangeField for WcpsCoverageMetadata object
     *
     * @param fields
     * @return
     */
    private List<RangeField> buildRangeFields(List<Field> fields) {
        List<RangeField> rangeFields = new ArrayList<>();
        // each field contains one quantity
        for (Field field : fields) {
            Quantity quantity = field.getQuantity();
            RangeField rangeField = new RangeField();
            // Data type for each band of coverage
            rangeField.setDataType(quantity.getDataType());
            rangeField.setName(field.getName());
            rangeField.setDescription(quantity.getDescription());
            rangeField.setDefinition(quantity.getDefinition());
            rangeField.setNodata(quantity.getNilValuesList());
            rangeField.setUomCode(quantity.getUom().getCode());
            rangeField.setAllowedValues(quantity.getAllowedValues());

            rangeFields.add(rangeField);
        }

        return rangeFields;
    }

    /**
     * Build list of axes for WcpsCoverageMetadata from the coverage's axes
     *
     * @param geoDomains
     * @param gridDomains
     * @return
     */
    private List<Axis> buildAxes(String coverageCRS, List<GeoAxis> geoAxes, List<IndexAxis> indexAxes) throws PetascopeException, SecoreException {
        List<Axis> result = new ArrayList();
        
        for (int i = 0; i < geoAxes.size(); i++) {
            GeoAxis geoAxis = geoAxes.get(i);
            String axisLabel = geoAxis.getAxisLabel();

            // NOTE: the order of geo CRS axes could be different from the index axes
            // e.g: CRS is EPSG:4326&Ansidate so the geo order is: Lat, Long, t, but in rasdaman, grid stored as: t, Lat, Long
            IndexAxis indexAxis = null;
            for (int j = 0; j < indexAxes.size(); j++) {
                indexAxis = indexAxes.get(j);
                String indexAxisLabelTmp = indexAxis.getAxisLabel();
                if (axisLabel.equals(indexAxisLabelTmp)) {
                    break;
                }
            }

            // geoBounds is the geo bounds of axis in the coverage (but can be modified later by subsets)
            NumericSubset originalGeoBounds = new NumericTrimming(geoAxis.getLowerBoundNumber(), geoAxis.getUpperBoundNumber());
            NumericSubset geoBounds = new NumericTrimming(geoAxis.getLowerBoundNumber(), geoAxis.getUpperBoundNumber());            
            NumericSubset originalGridBounds = new NumericTrimming(new BigDecimal(indexAxis.getLowerBound()), new BigDecimal(indexAxis.getUpperBound()));
            NumericSubset gridBounds = new NumericTrimming(new BigDecimal(indexAxis.getLowerBound()), new BigDecimal(indexAxis.getUpperBound()));
            
            String crsUri = geoAxis.getSrsName();

            CrsDefinition crsDefinition = CrsUtil.getCrsDefinition(crsUri);
            // x, y, t,...
            String axisType = CrsUtil.getAxisTypeByIndex(coverageCRS, i);

            // Get the metadata of CRS (needed when using TimeCrs)
            String axisUoM = geoAxis.getUomLabel();
            // the order of geo axis stored in rasdaman as grid axis (e.g: CRS order EPSG:4326 Lat, Long, but in grid order is: Long, Lat)
            int gridAxisOrder = indexAxis.getAxisOrder();

            // NOTE: this needs the "sign" of offset vector as well
            BigDecimal scalarResolution = geoAxis.getResolution();
            BigDecimal originNumber = this.getOriginNumber(geoAxis);

            // Check domainElement's type
            if (geoAxis.isIrregular()) {
                // All stored coefficients for irregular axis in coverage
                List<BigDecimal> directPositions = ((org.rasdaman.domain.cis.IrregularAxis) geoAxis).getDirectPositionsAsNumbers();
                result.add(new IrregularAxis(axisLabel, geoBounds, originalGridBounds, gridBounds,
                        crsUri, crsDefinition, axisType, axisUoM, gridAxisOrder,
                        originNumber, scalarResolution, directPositions, originalGeoBounds));
            } else {

                result.add(new RegularAxis(axisLabel, geoBounds, originalGridBounds, gridBounds,
                        crsUri, crsDefinition, axisType, axisUoM, gridAxisOrder,
                        originNumber, scalarResolution, originalGeoBounds));
            }            
        }
        return result;
    }

    private BigDecimal getOriginNumber(GeoAxis geoAxis) throws PetascopeException, SecoreException {
        BigDecimal origin;

        if (geoAxis.isIrregular()) {
            // Only supports irregular with positive resolution
            origin = geoAxis.getLowerBoundNumber();
        } else {
            BigDecimal resolution = geoAxis.getResolution();
            BigDecimal lowerBound = geoAxis.getLowerBoundNumber();
            BigDecimal upperBound = geoAxis.getUpperBoundNumber();
            //if axis is regular positive axis we apply formula: origin = (geoMinValue + 0.5) * resolution (> 0)
            if (resolution.compareTo(BigDecimal.ZERO) > 0) {
                origin = lowerBound.add(BigDecimal.valueOf(1.0 / 2)
                        .multiply(resolution)).stripTrailingZeros();
            } else {
                // if axis is regular negative axis, origin = (geoMaxValue + 0.5) * resolution (< 0)
                origin = upperBound.add(BigDecimal.valueOf(1.0 / 2)
                        .multiply(resolution)).stripTrailingZeros();
            }
        }

        return origin;
    }
}
