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

#include "clientmanager.hh"

#include "common/uuid/uuid.hh"
#include "server/rasserver_entry.hh"
#include "common/exceptions/missingresourceexception.hh"
#include <logging.hh>
#include <boost/thread.hpp>

namespace rasserver
{

using std::string;
using std::pair;
using std::make_pair;
using std::map;
using std::set;
using std::unique_lock;
using std::shared_ptr;
using std::thread;
using boost::shared_mutex;
using boost::shared_lock;
using common::Timer;
using common::UUID;

const int ClientManager::ALIVE_PERIOD = 30000;

ClientManager::ClientManager()
{
    this->isThreadRunning = true;
    this->managementThread.reset(
        new thread(&ClientManager::evaluateClientStatus, this));
}

ClientManager::~ClientManager()
{
    try
    {
        {
            std::lock_guard<std::mutex> lock(this->threadMutex);
            this->isThreadRunning = false;
        }

        this->isThreadRunningCondition.notify_one();

        this->managementThread->join();
    }
    catch (std::exception& ex)
    {
        LERROR << ex.what();
    }
    catch (...)
    {
        LERROR << "ClientManager destructor has failed";
    }
}

bool ClientManager::allocateClient(std::string clientUUID, __attribute__ ((unused)) std::string sessionId)
{
    Timer timer(ALIVE_PERIOD);

    pair<map<string, Timer>::iterator, bool> result = this->clientList.insert(make_pair(clientUUID, timer));

    return result.second;
}

void ClientManager::deallocateClient(std::string clientUUID, __attribute__ ((unused)) std::string sessionId)
{
    this->clientList.erase(clientUUID);
}

bool ClientManager::isAlive(std::string clientUUID)
{
    map<string, Timer>::iterator clientIt = this->clientList.find(clientUUID);
    if (clientIt == this->clientList.end())
    {
        return false;
    }
    return !clientIt->second.hasExpired();
}

void ClientManager::resetLiveliness(std::string clientUUID)
{
    map<string, Timer>::iterator clientIt = this->clientList.find(clientUUID);
    if (clientIt != this->clientList.end())
    {
        clientIt->second.reset();
    }
}

void ClientManager::cleanQueryStreamedResult(const std::string& requestUUID)
{
    this->queryStreamedResultList.erase(requestUUID);
}

void ClientManager::addQueryStreamedResult(const std::string& requestUUID, const shared_ptr<ClientQueryStreamedResult>& streamedResult)
{
    this->queryStreamedResultList.insert(std::make_pair(requestUUID, streamedResult));
}

shared_ptr<ClientQueryStreamedResult> ClientManager::getQueryStreamedResult(const std::string& requestUUID)
{
    map<string, shared_ptr<ClientQueryStreamedResult>>::iterator queryResult = this->queryStreamedResultList.find(requestUUID);

    if (queryResult == this->queryStreamedResultList.end())
    {
        throw common::MissingResourceException("Invalid request uuid: " + requestUUID);
    }

    return queryResult->second;
}

size_t ClientManager::getClientQueueSize()
{
    return this->clientList.size();
}

void ClientManager::evaluateClientStatus()
{
    map<string, Timer>::iterator it;
    map<string, Timer>::iterator toErase;

    map<string, shared_ptr<ClientQueryStreamedResult>>::iterator resultIt;
    map<string, shared_ptr<ClientQueryStreamedResult>>::iterator toEraseResult;

    std::chrono::milliseconds timeToSleepFor{ALIVE_PERIOD};

    std::unique_lock<std::mutex> threadLock(this->threadMutex);
    while (this->isThreadRunning)
    {
        try
        {
            // Wait on the condition variable to be notified from the
            // destructor when it is time to stop the worker thread
            if (this->isThreadRunningCondition.wait_for(threadLock, timeToSleepFor) == std::cv_status::timeout)
            {
                set<string> deadClients;

                boost::upgrade_lock<boost::shared_mutex> clientsLock(this->clientMutex);
                it = this->clientList.begin();

                while (it != this->clientList.end())
                {
                    toErase = it;
                    ++it;
                    if (toErase->second.hasExpired())
                    {
                        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(clientsLock);
                        this->clientList.erase(toErase);
                        deadClients.insert(toErase->first);

                        // If the client dies, clean up
                        RasServerEntry& rasServerEntry = RasServerEntry::getInstance();
                        try {
                            rasServerEntry.abortTA();
                        } catch (...) {
                            LERROR << "Failed aborting transaction.";
                        }
                        try {
                            rasServerEntry.closeDB();
                        } catch (...) {
                            LERROR << "Failed closing database connection.";
                        }
                        try {
                            rasServerEntry.disconnectClient();
                        } catch (...) {
                            LERROR << "Failed disconnecting client.";
                        }
                    }
                }

                resultIt = this->queryStreamedResultList.begin();
                while (resultIt != this->queryStreamedResultList.end())
                {
                    toEraseResult = resultIt;
                    resultIt++;

                    if (deadClients.find(toEraseResult->second->getClientUUID()) != deadClients.end())
                    {
                        this->cleanQueryStreamedResult(resultIt->first);
                    }
                }
            }
        }
        catch (std::exception& ex)
        {
            LERROR << "Client management thread has failed, reason: " << ex.what();
        }
        catch (...)
        {
            LERROR << "Client management thread failed for unknown reason.";
        }
    }
}

}
