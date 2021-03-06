"""
 *
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
 * Copyright 2003 - 2015 Peter Baumann / rasdaman GmbH.
 *
 * For more information please see <http://www.rasdaman.org>
 * or contact Peter Baumann via <baumann@rasdaman.com>.
 *
"""

from master.helper.gdal_axis_filler import GdalAxisFiller
from master.helper.gdal_range_fields_generator import GdalRangeFieldsGenerator
from master.importer.coverage import Coverage
from master.importer.importer import Importer
from master.importer.slice import Slice
from master.provider.data.file_data_provider import FileDataProvider
from master.recipe.base_recipe import BaseRecipe
from session import Session
from util.crs_util import CRSUtil
from util.gdal_util import GDALGmlUtil
from util.log import log
from util.gdal_validator import GDALValidator
from config_manager import ConfigManager
from util.file_util import FileUtil
from master.importer.resumer import Resumer

from util.timer_util import Timer


class Recipe(BaseRecipe):
    def __init__(self, session):
        """
        The recipe class for map_mosaic. To get an overview of the ingredients needed for this
        recipe check ingredients/map_mosaic
        :param Session session: the session for this import
        """
        super(Recipe, self).__init__(session)
        self.options = session.get_recipe()['options'] if "options" in session.get_recipe() else {}
        self.importer = None
        self.resumer = Resumer(self.session.get_coverage_id())

        validator = GDALValidator(self.session.files)
        if  ConfigManager.skip == True:
            self.session.files = validator.get_valid_files()

    def validate(self):
        """
        Implementation of the base recipe validate method
        """
        super(Recipe, self).validate()

        if 'band_names' not in self.options:
            self.options['band_names'] = None

    def describe(self):
        """
        Implementation of the base recipe describe method
        """
        importer = self._get_importer()

        slices = importer.get_slices_for_description()
        number_of_files = len(slices)
        log.info("All files have been analyzed. Please verify that the axis subsets of the first {} files above are correct.".format(number_of_files))
        index = 1
        for slice in slices:
            log.info("Slice " + str(index) + ": " + str(slice))
            index += 1

    def ingest(self):
        """
        Starts the ingesting process
        """
        importer = self._get_importer()
        importer.ingest()

    def status(self):
        """
        Implementation of the status method
        :rtype (int, int)
        """
        return self._get_importer().get_progress()

    def _get_slices(self, crs):
        """
        Returns the slices for the collection of files given
        """
        files = self.session.get_files()
        crs_axes = CRSUtil(crs).get_axes(self.session.coverage_id)

        slices = []
        count = 1
        for file in files:
            # NOTE: don't process any imported file from *.resume.json as it is just waisted time
            if not self.resumer.is_file_imported(file.filepath):
                timer = Timer()

                # print which file is analyzing
                FileUtil.print_feedback(count, len(files), file.filepath)
                if not FileUtil.validate_file_path(file.filepath):
                    continue

                valid_coverage_slice = True
                try:
                    subsets = GdalAxisFiller(crs_axes, GDALGmlUtil(file.get_filepath())).fill()
                except Exception as ex:
                    # If skip: true then just ignore this file from importing, else raise exception
                    FileUtil.ignore_coverage_slice_from_file_if_possible(file.get_filepath(), ex)
                    valid_coverage_slice = False

                if valid_coverage_slice:
                    slices.append(Slice(subsets, FileDataProvider(file)))

                timer.print_elapsed_time()
                count += 1

        return slices

    def _get_coverage(self):
        """
        Returns the coverage to be used for the importer
        """
        gdal_dataset = GDALGmlUtil.open_gdal_dataset_from_any_file(self.session.get_files())
        crs = gdal_dataset.get_crs()
        slices = self._get_slices(crs)
        fields = GdalRangeFieldsGenerator(gdal_dataset, self.options['band_names']).get_range_fields()
        coverage = Coverage(self.session.get_coverage_id(), slices, fields, gdal_dataset.get_crs(),
            gdal_dataset.get_band_gdal_type(), self.options['tiling'])
        return coverage

    def _get_importer(self):
        if self.importer is None:
            self.importer = Importer(self.resumer, self._get_coverage(), self.options['wms_import'], self.options['scale_levels'], False)
        return self.importer


    @staticmethod
    def get_name():
        return "map_mosaic"
