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

#ifndef _SERVERCOMM_HH_
#define _SERVERCOMM_HH_

#include "raslib/error.hh"
#include "raslib/oid.hh"
#include "raslib/minterval.hh"

// forward declarations
class AdminIf;
class BaseType;
class ClientTblElt;
class MDDColl;
struct RPCMarray;
struct RPCOIdEntry;
struct ExecuteQueryRes;
struct ExecuteUpdateRes;

//@ManMemo: Module: {\bf servercomm}

/*@Doc:
  The class servercomm describes the one and only server communication object
  that can exist in a RasDaMan RPC server. It manages listening for client and
  maps incoming calls to the respective remote procedures (which reside in the
  file manager.cc). These remote procedures are global functions
  which mainly concern with RPC call processing and finally call the methods
  of this servercomm class to forward client requests.
*/
class ServerComm
{
private:
    /// singleton
    static ServerComm *serverCommInstance;
    /// the client table which holds information about the calling clients
    static ClientTblElt *clientTbl;
    
public:

    /// default constructor
    ServerComm();

    ServerComm(const ServerComm &) = delete;

    /// destructor
    virtual ~ServerComm();

    /// adds an entry to the client table (used in RasServerEntry)
    void addClientTblEntry(ClientTblElt *context);
    /**
      Adds the context entry passed to the client table.
      Throws an exception if context==NULL.
    */

    /// deletes an entry of the client table (must be public because it is used in the global garbage collection function)
    unsigned short deleteClientTblEntry(unsigned long ClientId);
    /**
      Deletes the entry of the client table corresponding to the given client id.
      If no corresponding id is found, false is returned.
    */

    /// returns a pointer to the context of the calling client, 0 it there is no context
    ClientTblElt *getClientContext(unsigned long ClientId);
    /**
      Returns a pointer to the context of the calling client. This is done by
      searching the client table maintained by the server for the given client id.
      If there is no context corresponding to the client id, 0 is returned.

      Attention: After a client context was successfully received it has to be
                 released using its member function release();
    */

    // quick hack function used when stopping server to abort transaction and close db
    static void abortEveryThingNow();

    /// print server status with client table content to {\tt s}
    virtual void printServerStatus();

    // -----------------------------------------------------------------------------------------
    // DB methods: open, close, create, destroy
    // -----------------------------------------------------------------------------------------

    ///
    /// open database
    virtual unsigned short openDB(unsigned long callingClientId, const char *dbName, const char *userName);
    /**
      The method opens the database with {\tt dbName}. The return value means the following:

      \begin{tabular}{lll}
      0 && database successfully opened\\
      1 && client context not found\\
      2 && database does not exist\\
      3 && database is already open\\
      \end{tabular}
    */

    /// close current database
    virtual unsigned short closeDB(unsigned long callingClientId);
    /**
      The return value has the following meaning:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      \end{tabular}
    */

    /// create a database
    virtual unsigned short createDB(char *name);

    /// destroy a database
    virtual unsigned short destroyDB(char *name);

    // -----------------------------------------------------------------------------------------
    // Transaction (TA) methods: begin, commit, abort, isTAOpen
    // -----------------------------------------------------------------------------------------

    ///
    /// open transaction
    virtual unsigned short beginTA(unsigned long callingClientId, unsigned short readOnly = 0);
    /**
      The return value has the following meaning:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && other transaction already active\\
      \end{tabular}
    */

    /// commit current transaction
    virtual unsigned short commitTA(unsigned long callingClientId);
    /**
      The return value has the following meaning:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      \end{tabular}
    */

    /// abort current transaction
    virtual unsigned short abortTA(unsigned long callingClientId);
    /**
      The return value has the following meaning:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      \end{tabular}
    */

    /// is transaction open currently?
    virtual bool isTAOpen(unsigned long callingClientId);
    /**
      The return value has the following meaning:
      \begin{tabular}{lll}
      true && a transaction is open\\
      false && no transaction is open\\
      \end{tabular}
    */

    // -----------------------------------------------------------------------------------------
    // Execute rasql queries (select, update, insert)
    // -----------------------------------------------------------------------------------------

    ///
    /// executes a retrieval query and prepares the result for transfer with \Ref{getNextMDD}.
    virtual unsigned short
    executeQuery(unsigned long callingClientId, const char *query, ExecuteQueryRes &returnStructure,
                 bool insert = false);
    /**
      Executes a query and puts the result in the actual transfer collection.
      The first parameter is the unique client id
      for which the query should be executed. The second parameter is the
      query itself represented as a string. Third parameter indicates if the
      query is an insert query (if true), otherwise a regular select query.

      Return values
      \begin{tabular}{lll}
      0 && operation was successful - result collection holds MDD elements\\
      1 && operation was successful - result collection holds non-MDD elements\\
      2 && operation was successful - result collection has no elements\\
      3 && client context not found\\
      4 && parse errror\\
      5 && execution error\\
      \end{tabular}

      Communication protocol (return value = 0)
      \begin{tabular}{lll}
      \Ref{executeQuery} && \\
      ->                 && \Ref{getNextMDD} \\
                         && ->               && \Ref{getNextTile} \\
                         &&                  && : \\
                         && :\\
      \Ref{endTransfer} \\
      \end{tabular}

      Communication protocol (return value = 1)
      \begin{tabular}{lll}
      \Ref{executeQuery} && \\
      ->                 && \Ref{getNextElement} \\
                         && :\\
      \Ref{endTransfer} \\
      \end{tabular}
    */

    ///
    /// prepares transfer of MDD constants and execution of update query
    virtual unsigned short initExecuteUpdate(unsigned long callingClientId);
    /**
      Return values:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      \end{tabular}

      Communication protocol
      \begin{tabular}{lll}
      \Ref{initExecuteUpdate} && \\
      ->                      && \Ref{startInsertTransMDD} \\
                              && ->                        && \Ref{insertTile} \\
                              &&                           && :\\
                              && \Ref{endInsertMDD}\\
                              && :\\
      \Ref{executeUpdate}     && \\
      \end{tabular}

      Note: Method \Ref{executeUpdate} can be invoked without the \Ref{initExecuteUpdate}
            prolog in case of no constant MDD objects.
    */

    /// executes an update query
    virtual unsigned short
    executeUpdate(unsigned long callingClientId, const char *query, ExecuteUpdateRes &returnStructure);
    /**
      Executes an update query.
      The first parameter is the unique client id
      for which the query should be executed. The second parameter is the
      query itself represented as a string.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && parse errror\\
      3 && execution error\\
      \end{tabular}
    */

    // insert query returning results
    virtual unsigned short
    executeInsert(unsigned long callingClientId, const char *query, ExecuteQueryRes &returnStructure);
    /**
      Executes a query and puts the result in the actual transfer collection.
      The first parameter is the unique client id
      for which the query should be executed. The second parameter is the
      query itself represented as a string.

      Return values
      \begin{tabular}{lll}
      0 && operation was successful - result collection holds MDD elements\\
      1 && operation was successful - result collection holds non-MDD elements\\
      2 && operation was successful - result collection has no elements\\
      3 && client context not found\\
      4 && parse errror\\
      5 && execution error\\
      \end{tabular}

      Communication protocol (return value = 0)
      \begin{tabular}{lll}
      \Ref{executeQuery} && \\
      ->                 && \Ref{getNextMDD} \\
                         && ->               && \Ref{getNextTile} \\
                         &&                  && : \\
                         && :\\
      \Ref{endTransfer} \\
      \end{tabular}

      Communication protocol (return value = 1)
      \begin{tabular}{lll}
      \Ref{executeQuery} && \\
      ->                 && \Ref{getNextElement} \\
                         && :\\
      \Ref{endTransfer} \\
      \end{tabular}
    */

    // -----------------------------------------------------------------------------------------
    // Insert MDD / tile
    // -----------------------------------------------------------------------------------------

    /// create a new persistent MDD object for tile based transfers
    virtual unsigned short startInsertPersMDD(unsigned long callingClientId,
            const char *collName, r_Minterval &domain,
            unsigned long typeLength, const char *typeName, r_OId &oid);
    /**
      Creates an object for tile based transfer with method \Ref{insertTile} to be
      inserted into the specified MDD collection.

      Parameters
      \begin{tabular}{lll}
      {\tt callingClientId}  && unique client id of the calling client\\
      {\tt collName}         && name of the collection to insert the MDD object\\
      {\tt domain}           && spatial domain\\
      {\tt typeLength}       && size of base type in bytes\\
      {\tt typeName}         && type structure as string representation\\
      {\tt oid}              && object identifier\\
      \end{tabular}

      Return values
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && MDD type name not found\\
      3 && types of MDD and collection are incompatible\\
      4 && MDD and its type are incompatible\\
      5 && collection does not exist\\
      6 && creation of persistent object failed\\
      \end{tabular}

      Communication protocol
      \begin{tabular}{lll}
      \Ref{startInsertPersMDD} && \\
      ->                       && \Ref{insertTile} \\
                               && :\\
      \Ref{endInsertMDD} && \\
      \end{tabular}
    */

    ///
    /// prepares an MDD (transient) for transfer of tiles
    virtual unsigned short startInsertTransMDD(unsigned long callingClientId,
            r_Minterval &domain,
            unsigned long typeLength, const char *typeName);
    /**
      Creates an object for tile based transfer with method \Ref{insertTile}.

      The first parameter is the unique client id for which the MDD should be created.
      The second parameter is the
      name of the collection to insert the MDD object. The third parameter holds the
      spatial domain of the following MDD object and {\tt typeLength} specifies the size of
      the base type in bytes. The last one gives the type structure as string representation.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && MDD type name not found\\
      3 && MDD and its type are incompatible\\
      \end{tabular}
    */

    /// finishes the MDD creation and inserts the MDD into the collection
    virtual unsigned short endInsertMDD(unsigned long callingClientId,
                                        int isPersistent);
    /**
      Parameters
      \begin{tabular}{lll}
      {\tt callingClientId}  && unique client id of the calling client\\
      {\tt isPersistent}     && determines wheather it is a persistent or a transient MDD\\
      \end{tabular}
    */

    /// insert a tile into a persistent MDD object
    virtual unsigned short insertTile(unsigned long callingClientId,
                                      bool isPersistent, RPCMarray *rpcMarray, r_Minterval *tileSize = NULL);
    /**
      Splits (if tileSize != NULL) and inserts a tile into the current MDD object.

      Parameters
      \begin{tabular}{lll}
      {\tt callingClientId}  && unique client id of the calling client\\
      {\tt isPersistent}     && determines wheather it is a persistent or a transient tile\\
      {\tt rpcMarray}        && RPC representation of the tile\\
      {\tt tileSize}         && r_Minterval specifying the tile-size\\
      \end{tabular}

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && base type name of inserting tile is not supported\\
      3 && base type does not match MDD type\\
      \end{tabular}
    */

    // -----------------------------------------------------------------------------------------
    // Fetch query results: next MDD, scalar, tile
    // -----------------------------------------------------------------------------------------

    ///
    /// get the domain of the next MDD of the actual transfer collection
    virtual unsigned short getNextMDD(unsigned long callingClientId,
                                      r_Minterval &mddDomain, char *&typeName, char *&typeStructure,
                                      r_OId &oid, unsigned short &currentFormat);
    /**
      The Method gets the domain of the next MDD of the actual transfer collection.
      The first parameter is the unique client id. The second parameter returns the
      domain of the MDD to be transfered. {\tt typeName} returns the name of the
      MDD type and its structure.
      Transfer of MDD data is tile-based using the method \Ref{getNextTile}.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful, at least one MDD is left in the transfer collection\\
      1 && nothing left in the transfer collection\\
      2 && client context not found, no tiles in the MDD object, no actual transfer collection \\
      \end{tabular}
    */

    /**
     * Called by getNextElement to help handling of struct elements. It works
     * for nested structs as well. Only used in case endianess needs changing.
     */
    virtual void swapScalarElement(char *buffer, const BaseType *baseType);

    /// get the next scalar element in the actual transfer collection.
    virtual unsigned short getNextElement(unsigned long callingClientId,
                                          char *&buffer, unsigned int &bufferSize);
    /**
      The Method gets the next non-MDD element in the actual transfer collection.
      The first parameter is the unique client id. The second parameter returns a
      pointer to the memory occupied by the next element and the third one delivers
      the size of the buffer.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful, at least one element is left in the transfer collection\\
      1 && operation succesful, nothing left in the transfer collection\\
      2 && client context not found, no tiles in the MDD object, no actual transfer collection \\
      \end{tabular}
    */

    /// get next tile of the actual MDD of the actual transfer collection
    virtual unsigned short getNextTile(unsigned long callingClientId,
                                       RPCMarray **rpcMarray);
    /**
      The Method gets the next tile of the actual MDD of the actual transfer collection.
      The first parameter is the unique client id. The second parameter is the
      RPC representation of the Marray representing the tile. If a tile is too large to be
      transferred in one piece, the data is split. To get the rest of the data, consecutively
      use this method.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful, no further MDDs are left\\
      1 && operation was successful, at least one MDD is left in the transfer collection\\
      2 && operation was successful, at least one tile is left in the actual MDD\\
      3 && operation was successful, at least one block is left in the actual tile\\
      4 && client context not found, no tiles in the MDD object, no actual transfer collection \\
        && or nothing left in the collection\\
      \end{tabular}

      Examples of valid return value chains:
      \begin{itemize}
      \item To be transferred: 1 MDD consisting of 1 tile (which is too large)\\
      \begin{verbatim}
      3 ->...-> 3 -> 0
      \end{verbatim}
      \item To be transferred: 1 MDD consisting of 2 tiles (the first is too large)\\
      \begin{verbatim}
      3 ->...-> 3 -> 2 -> 0
      |--------------|    |
          1st tile     2nd tile
      \end{verbatim}
      \item To be transferred: 2 MDDs, each consisting of 1 tile (none too large)\\
      \begin{verbatim}
      1 -> 0
      \end{verbatim}
      \item To be transferred: 3 MDDs, the first (A) consisting of 1 tile (not too large),\\
      the second (B) consisting of 2 tiles (B1, B2, of which the first is too large),
      the third (C) consisting of 2 tiles (C1, C2, of which the second is too large),
      \begin{verbatim}
      1 -> 3 ->...-> 3 -> 2 -> 1 -> 2 -> 3 ->...-> 3 -> 0
      |    |--------------|    |    |    |--------------|
      |           B1          B2    C1          C2
      |    |-------------------|    |-------------------|
      A              B                        C
      \end{verbatim}
      \end{itemize}
    */

    /// process the client's alive signal
    virtual unsigned short endTransfer(unsigned long client);
    /**
      The method terminates a transfer session and releases all transfer structures.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successfull\\
      1 && client context not found\\
      \end{tabular}
    */

    // -----------------------------------------------------------------------------------------
    // Collection mgmt, used by the rasodmg C++ API
    // -----------------------------------------------------------------------------------------

    ///
    /// create new MDD collection
    virtual unsigned short insertColl(unsigned long callingClientId,
                                      const char *collName, const char *typeName, r_OId &oid);
    /**
      Creates a new MDD collection.

      Parameters
      \begin{tabular}{lll}
      {\tt callingClientId}  && unique client id of the calling client\\
      {\tt collName}         && name of the collection to be created\\
      {\tt typeName}         && name of the collection type\\
      {\tt oid}              && object identifier\\
      \end{tabular}

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && collection type name not found\\
      3 && collection name exists already in the db\\
      4 && failed to create due to some other error
      \end{tabular}
    */

    ///
    /// delete MDD collection
    virtual unsigned short deleteCollByName(unsigned long callingClientId,
                                            const char *collName);
    /**
      Deletes an MDD collection. The first parameter is the unique client id
      for which the collection should be deleted. The second parameter is the
      name for the collection to be deleted.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && collection with name does not exist\\
      \end{tabular}
    */

    /// delete object by oid
    virtual unsigned short deleteObjByOId(unsigned long callingClientId, r_OId &oid);
    /**
      Deletes the object with {\tt oid}.
      The first parameter is the unique client id for which the object should be
      deleted.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && object with oid does not exist\\
      \end{tabular}
    */

    ///
    /// remove object specified by oid from collection specified by name
    virtual unsigned short removeObjFromColl(unsigned long callingClientId,
            const char *collName, r_OId &oid);
    /**
      The method removes the object with {\\t oid} from collection with {\tt collName}.
      The first parameter is the unique client id for which the object should be removed.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && specified collection does not exist\\
      3 && specified object does not exist in the collection\\
      4 && general error\\
      \end{tabular}
    */

    // -----------------------------------------------------------------------------------------
    // Get collection/MDD by name or oid
    // -----------------------------------------------------------------------------------------

    ///
    /// prepare an MDD collection for transfer with getNextMDD()
    virtual unsigned short getCollByName(unsigned long callingClientId,
                                         const char *collName,
                                         char *&typeName, char *&typeStructure, r_OId &oid);
    /**
      ATTENTION: This function is not used at the moment. It hast
      to be adapted to transferData.

      Prepares an MDD collection for transfer with getNextMDD().

      Parameters
      \begin{tabular}{lll}
      {\tt callingClientId}  && unique client id of the calling client\\
      {\tt collName}         && name of the collection to be got\\
      {\tt typeName}         && returns name of the collection type\\
      {\tt typeStructure}    && returns structure of the collection type\\
      {\tt oid}              && returns oid of the collection\\
      \end{tabular}

      The first parameter is the unique client id. The second parameter is the
      name of the collection to get. {\tt typeName} returns the name of the
      collection type and {\tt typeStructure} its type structure.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful - collection has some elements\\
      1 && operation was successful - collection has no elements\\
      2 && collection is not known\\
      3 && client context not found\\
      \end{tabular}
    */

    /// prepare an MDD collection for transfer with getNextMDD()
    virtual unsigned short getCollByOId(unsigned long callingClientId,
                                        r_OId &oid, char *&typeName, char *&typeStructure, char *&collName);
    /**
      ATTENTION: This function is not used at the moment. It hast
      to be adapted to transferData.

      Prepares an MDD collection for transfer with \Ref{getNextMDD}.

      Parameters
      \begin{tabular}{lll}
      {\tt callingClientId}  && unique client id of the calling client\\
      {\tt oid}              && oid of the collection to be got\\
      {\tt typeName}         && returns name of the collection type\\
      {\tt typeStructure}    && returns structure of the collection type\\
      {\tt collName}         && returns name of collection\\
      \end{tabular}

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful - collection has some elements\\
      1 && operation was successful - collection has no elements\\
      2 && collection is not known\\
      3 && client context not found\\
      \end{tabular}
    */

    /// gets oids of the collection specified by name
    virtual unsigned short getCollOIdsByName(unsigned long callingClientId,
            const char *collName,
            char *&typeName, char *&typeStructure, r_OId &oid,
            RPCOIdEntry *&oidTable, unsigned int &oidTableSize);
    /**
      Gets the collection of oids of the collection with {\tt collName}.

      Parameters
      \begin{tabular}{lll}
      {\tt callingClientId}  && unique client id of the calling client\\
      {\tt collName}         && name of the collection to be got\\
      {\tt typeName}         && returns name of the collection type\\
      {\tt typeStructure}    && returns structure of the collection type\\
      {\tt oid}              && returns object identifier\\
      {\tt oidTable}         && returns an array of pointers to oids\\
      {\tt oidTableSize}     && returns the no of elements in the table\\
      \end{tabular}

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful - collection has some elements\\
      1 && operation was successful - collection has no elements\\
      2 && collection is not known\\
      3 && client context not found\\
      \end{tabular}
    */

    /// gets oids of the collection specified by name
    virtual unsigned short getCollOIdsByOId(unsigned long callingClientId,
                                            r_OId &oid, char *&typeName, char *&typeStructure,
                                            RPCOIdEntry *&oidTable, unsigned int &oidTableSize, char *&collName);
    /**
      Gets the collection of oids of the collection with {\tt collName}.

      Parameters
      \begin{tabular}{lll}
      {\tt callingClientId}  && unique client id of the calling client\\
      {\tt oid}              && oid of the collection to be got\\
      {\tt typeName}         && returns name of the collection type\\
      {\tt typeStructure}    && returns structure of the collection type\\
      {\tt oidTable}         && returns an array of pointers to oids\\
      {\tt oidTableSize}     && returns the no of elements in the table\\
      {\tt collName}         && returns name of collection\\
      \end{tabular}

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful - collection has some elements\\
      1 && operation was successful - collection has no elements\\
      2 && collection is not known\\
      3 && client context not found\\
      \end{tabular}
    */

    /// get an MDD by OId
    virtual unsigned short getMDDByOId(unsigned long callingClientId,
                                       r_OId &oid, r_Minterval &mddDomain,
                                       char *&typeName, char *&typeStructure, unsigned short &currentFormat);
    /**
      The Method gets an MDD by OId {\tt oid}. If the MDD is found, it is initialized as transfer
      object and can be picked up by \Ref{getNextTile} calls (tile-based transfer).

      Additionally, the method returns domain, type name, and type structure of the found MDD
      object by reference parameters.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && object with this oid not found\\
      3 && object has no tiles
      \end{tabular}

      Communication protocol
      \begin{tabular}{lll}
      \Ref{getMDDByOId} \\
      ->                && \Ref{getNextTile} \\
                        && : \\
      \Ref{endTransfer} \\
      \end{tabular}
    */

    // -----------------------------------------------------------------------------------------
    // Utility methods
    // -----------------------------------------------------------------------------------------

    ///
    /// get new object identifier
    virtual unsigned short getNewOId(unsigned long callingClientId,
                                     unsigned short objType, r_OId &oid);
    /**
      Creates a new oid and gives it back by the refernce parameter {\tt oid}.
      {\tt objType} determines the type of object for which that oid is allocated. The folowing
      values are supported: 1 = MDD,  2 = Collection.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && error while creating oid\\
      \end{tabular}
    */

    /// get type of object by oid
    virtual unsigned short getObjectType(unsigned long callingClientId,
                                         r_OId &oid, unsigned short &objType);
    /**
      Determines the type of the object indicated by {\tt oid}. The type is returned by the
      reference parameter {\tt objType}. The folowing types are supported: 1 = MDD,  2 = Collection.

      Return values:
      \begin{tabular}{lll}
      0 && operation was successful\\
      1 && client context not found\\
      2 && oid not found\\
      \end{tabular}
    */

    /// get type structure of a type name
    virtual unsigned short getTypeStructure(unsigned long callingClientId,
                                            const char *typeName, unsigned short typeType, char *&typeStructure);
    /**
      Determines the type structure of the type specified by {\tt typeName}. The type
    either can be a set type (typeType=1), an mdd type (typeType=2), or a base type
    (typeType = 3).

           Return values:
           \begin{tabular}{lll}
           0 && operation was successful\\
           1 && client context not found\\
           2 && type name not found\\
           \end{tabular}
         */

    /// set the data format used for transferring data to the client
    virtual unsigned short setTransferMode(unsigned long callingClientId,
                                           unsigned short format, const char *formatParams);
    /**
    Sets the data format used by the server to transfer data to the client to
    format which is of type r_Data_Format.

    Return values:
    \begin{tabular}{lll}
    0 && operation was successful\\
    1 && client context not found\\
    2 && unknown or unsupported data format\\
    \end{tabular}
         */

    /// set the data format for storing data into the database
    virtual unsigned short setStorageMode(unsigned long callingClientId,
                                          unsigned short format, const char *formatParams);
    /**
    return values exactly like setTransferMode()
    */
    
    void setAdmin(AdminIf *newAdmin);

    static const int RESPONSE_ERROR;
    static const int RESPONSE_MDDS;
    static const int RESPONSE_SCALARS;
    static const int RESPONSE_INT;
    static const int RESPONSE_OID;
    static const int RESPONSE_OK_NEGATIVE;
    static const int RESPONSE_OK;

    static const unsigned short EXEC_RESULT_MDDS = 0;
    static const unsigned short EXEC_RESULT_SCALARS = 1;
    static const unsigned short EXEC_RESULT_EMPTY = 2;
    static const unsigned short EXEC_RESULT_PARSE_ERROR = 4;
    static const unsigned short EXEC_RESULT_EXEC_ERROR = 5;

    static constexpr unsigned short RC_OK = 0;
    static constexpr unsigned short RC_CLIENT_NOT_FOUND = 1;
    static constexpr unsigned short RC_ERROR = 2;

    static const int ENDIAN_BIG;
    static const int ENDIAN_LITTLE;

protected:
    /// make sure a tile has the correct data format, converting if necessary
    static int ensureTileFormat(r_Data_Format &hasFmt, r_Data_Format needFmt,
                                const r_Minterval &dom, const BaseType *type, char *&data, r_Bytes &size,
                                int repack, int owner, const char *params = NULL);

    // parse the query, return true if all fine
    bool parseQuery(const char *query);

    /// init fields of res to 0
    static void resetExecuteQueryRes(ExecuteQueryRes &res);
    static void resetExecuteUpdateRes(ExecuteUpdateRes &res);
    /// free fields of res
    static void cleanExecuteQueryRes(ExecuteQueryRes &res);
    /// return type name and type structure of the first transfer element in context
    std::pair<char *, char *> getTypeNameStructure(ClientTblElt *context) const;
    unsigned short getTransferCollInfo(
        ClientTblElt *context, r_OId &oid, char *&typeName, char *&typeStructure, MDDColl *coll) const;

    /// pointer to the actual administration interface object
    AdminIf *admin{NULL};

    /// flag for active db transaction (stores the clientID of the owner of the active transaction,
    /// or 0 if none open)
    unsigned long transactionActive{0};
};

#endif
