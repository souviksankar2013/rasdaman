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
 * Copyright 2003 - 2017 Peter Baumann / rasdaman GmbH.
 *
 * For more information please see <http://www.rasdaman.org>
 * or contact Peter Baumann via <baumann@rasdaman.com>.
 */
package petascope.controller;

import java.io.IOException;
import java.util.Map;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import static org.rasdaman.config.ConfigManager.RASQL;
import org.rasdaman.config.VersionManager;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.multipart.MultipartFile;
import petascope.controller.handler.service.KVPRasqlServiceHandler;
import petascope.core.KVPSymbols;
import static petascope.core.KVPSymbols.KEY_UPLOADED_FILE_VALUE;
import petascope.core.response.Response;
import petascope.exceptions.PetascopeException;
import petascope.exceptions.SecoreException;
import petascope.exceptions.WCSException;
import petascope.exceptions.WMSException;
import petascope.util.ExceptionUtil;

/**
 * Controller for Rasql query as RasqlServlet before
 *
 * <a href="mailto:b.phamhuu@jacobs-university.de">Bang Pham Huu</a>
 */
@RestController
public class RasqlController extends AbstractController {

    private static org.slf4j.Logger log = LoggerFactory.getLogger(RasqlController.class);

    @Autowired
    KVPRasqlServiceHandler kvpRasqlServiceHandler;

    @RequestMapping(value = RASQL, method = RequestMethod.POST)
    protected void handlePost(HttpServletRequest httpServletRequest, 
                              HttpServletResponse httpServletResponse,
            @RequestParam(value = KVPSymbols.KEY_UPLOADED_FILE_VALUE, required = false) MultipartFile uploadedMultipartFile) throws Exception {
        Map<String, String[]> kvpParameters = this.buildGetRequestKvpParametersMap(httpServletRequest.getQueryString());

        // A file is uploaded e.g: with WCS clipping extension and WKT text is big string in a text file
        String uploadedFilePath = null;
        if (uploadedMultipartFile != null) {
            // It is a upload file request
            byte[] bytes = this.getUploadedMultipartFileContent(uploadedMultipartFile);
            uploadedFilePath = this.storeUploadFileOnServer(uploadedMultipartFile.getOriginalFilename(), bytes);
            kvpParameters.put(KEY_UPLOADED_FILE_VALUE, new String[] {uploadedFilePath});
        }
        this.requestDispatcher(httpServletRequest, kvpParameters);
    }

    @RequestMapping(value = RASQL, method = RequestMethod.GET)
    @Override
    protected void handleGet(HttpServletRequest httpServletRequest) throws WCSException, IOException, PetascopeException, SecoreException, Exception {
        Map<String, String[]> kvpParameters = this.buildGetRequestKvpParametersMap(httpServletRequest.getQueryString());
        this.requestDispatcher(httpServletRequest, kvpParameters);
    }

    @Override
    protected void requestDispatcher(HttpServletRequest httpServletRequest, Map<String, String[]> kvpParameters) throws IOException, PetascopeException, WCSException, SecoreException, WMSException, Exception {
  
        if (startException != null) {
            throwStartException();
        }
                
        log.info("Received request: " + this.getRequestRepresentation(kvpParameters));
        long start = System.currentTimeMillis();

        Response response = null;
        boolean requestSuccess = true;
        
        try {
            response = kvpRasqlServiceHandler.handle(kvpParameters);
            this.writeResponseResult(response);
        } catch(Exception ex) {
            requestSuccess = false;
            ExceptionUtil.handle(VersionManager.getLatestVersion(KVPSymbols.RASQL_SERVICE), ex, injectedHttpServletResponse);
        } finally {
            long end = System.currentTimeMillis();
            long totalTime = end - start;
            log.info("Request processed in " + String.valueOf(totalTime) + " ms.");

        }
    }
}
