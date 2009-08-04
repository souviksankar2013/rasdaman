/*
 * This file is part of PetaScope.
 *
 * PetaScope is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * PetaScope is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with PetaScope. If not, see <http://www.gnu.org/licenses/>.
 *
 * For more information please see <http://www.PetaScope.org>
 * or contact Peter Baumann via <baumann@rasdaman.com>.
 *
 * Copyright 2009 Jacobs University Bremen, Peter Baumann.
 */


package wcst.transaction;

//~--- JDK imports ------------------------------------------------------------

import java.io.FileInputStream;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Savepoint;
import java.sql.Statement;

import java.util.Properties;

/**
 * Class to facilitate access to the PostgreSQL metadata database.
 *
 * @author Andrei Aiordachioaie
 */
public class MetadataDb
{
    Connection con;
    String driver, url, user, pass;
    ResultSet result;
    Statement sta;

    Savepoint savepoint;        // for rolling back changes

    /**
     * Default Constructor. Initializes the database access information from a file on disk
     *
     * @param dbParamsFile path to the properties file
     */
    public MetadataDb(String dbParamsFile) throws WCSTException
    {
        Properties dbParams = new Properties();

        try
        {
            dbParams.load(new FileInputStream(dbParamsFile));

            driver = dbParams.getProperty("metadata_driver");
            url = dbParams.getProperty("metadata_url");
            user = dbParams.getProperty("metadata_user");
            pass = dbParams.getProperty("metadata_pass");

            Class.forName(driver);  // load the needed JDBC library 
            con = DriverManager.getConnection(url, user, pass);
            con.setAutoCommit(false);
            savepoint = con.setSavepoint();
        }
        catch (Exception e)
        {
            log("User = " + user);
//            log("Pass = " + pass);
            log("URL = " + url);
            log("Driver = " + driver);

            e.printStackTrace();
            throw new WCSTException("NoApplicableCode", "Could not connect to the metadata database!", "Could not connect to the metadata database!");
        }
    }

    private void log(String msg)
    {
        System.out.println(msg);
    }

    /**
     * Execute a query on the database that is currently open. Use this function
     * for queries that should return results (SELECT).
     * @param query SQL of the query
     * @return a ResultSet object, with the results of the query
     */
    public ResultSet executeSelectQuery(String query)
    {
        try
        {
            sta = con.createStatement();
            result = sta.executeQuery(query);
        }
        catch (SQLException ex)
        {
            System.err.println("SQLException: " + ex.getMessage());
            result = null;
        }

        return result;
    }

    /**
     * Execute a query on the database that is currently open. Use this function
     * for queries that should NOT return any results (CREATE, UPDATE, DELETE etc)
     * @param query SQL of the query
     * @return The number of affected rows
     */
    public int executeUpdateQuery(String query) throws SQLException
    {
        int rows = -1;

        sta = con.createStatement();
        rows = sta.executeUpdate(query);
        sta.close();

        log(rows + " row(s) updated");
        return rows;
    }

    /**
     * Commits the active transactions and closes the database.
     * @throws SQLException
     */
    public void commitChangesAndClose() throws SQLException
    {
        con.commit();
        con.close();
    }

    /**
     * Aborts active transactions and close the database.
     * @throws SQLException
     */
    public void abortChangesAndClose() throws SQLException
    {
        con.rollback(savepoint);
        con.close();
    }
}
