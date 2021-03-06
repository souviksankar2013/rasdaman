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
 * Copyright 2003 - 2017 Peter Baumann /
 rasdaman GmbH.
 *
 * For more information please see <http://www.rasdaman.org>
 * or contact Peter Baumann via <baumann@rasdaman.com>.
 */

///<reference path="../../../common/_common.ts"/>

module gml {

    export class ReferenceableGridCoverage extends AbstractGridCoverage {       

        private generalGridAxisSource:rasdaman.common.ISerializedObject;

        public constructor(source:rasdaman.common.ISerializedObject) {
            super(source)

            this.currentSource = source.getChildAsSerializedObject("gmlrgrid:ReferenceableGridByVectors");
        }

        protected parseAxisTypesAndOffsetVectors(): void {

            // Iterate all offsetVectors element of coverage
            this.currentSource.getChildrenAsSerializedObjects("gmlrgrid:generalGridAxis").forEach((element) => {

                let coefficientsElement = element.getChildAsSerializedObject("gmlrgrid:GeneralGridAxis").getChildAsSerializedObject("gmlrgrid:coefficients")
                if (coefficientsElement.getValueAsString() === "") {
                    this.axisTypes.push(this.REGULAR_AXIS);
                } else {
                    this.axisTypes.push(this.IRREGULAR_AXIS);
                }
                              
                let offsetVectorElement = element.getChildAsSerializedObject("gmlrgrid:GeneralGridAxis").getChildAsSerializedObject("gmlrgrid:offsetVector");                
                let tmpArray:string[] = offsetVectorElement.getValueAsString().split(" ");

                for (let i = 0; i < tmpArray.length; i++) {
                    // e.g: <gml:offsetVector>-0.5 0</gml:offsetVector>
                    if (tmpArray[i] != "0") {
                        if (this.axisTypes[this.axisTypes.length - 1] !== this.IRREGULAR_AXIS) {
                            // Regular axis
                            this.offsetVectors.push(tmpArray[i]);
                        } else {
                            // Irregular axis
                            this.offsetVectors.push(this.IRREGULAR_AXIS_RESOLUTION);
                        }
                        break;
                    }
                }

            });
        }
    }

}