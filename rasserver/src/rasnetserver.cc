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

#include "rasnetserver.hh"
#include "clientmanager.hh"
#include "common/grpc/grpcutils.hh"
#include "common/exceptions/connectionfailedexception.hh"
#include "rasnet/messages/rasmgr_rassrvr_service.grpc.pb.h"
#include "servercomm/rasnetservercomm.hh"
#include "server/rasserver_entry.hh"
#include "include/globals.hh"

#include <logging.hh>

#include <iostream>
#include <chrono>
#include <grpc++/grpc++.h>

namespace rasserver
{

using std::shared_ptr;
using std::unique_ptr;
using common::GrpcUtils;
using common::HealthServiceImpl;


RasnetServer::RasnetServer(std::uint32_t listenPort1, const char* rasmgrHost1,
                           std::uint32_t rasmgrPort1, const char* serverId1):
    isRunning(false), listenPort{listenPort1},
    rasmgrPort{rasmgrPort1}, rasmgrHost{rasmgrHost1}, serverId{serverId1}
{
    shared_ptr<ClientManager> clientManager(new ClientManager());

    rasserverService = std::make_shared<RasServerServiceImpl>(clientManager);
    clientServerService = std::make_shared<RasnetServerComm>(clientManager);
    healthServiceImpl = std::make_shared<HealthServiceImpl>();
}

void RasnetServer::startRasnetServer()
{
    RasServerEntry& rasserver = RasServerEntry::getInstance();
    rasserver.connectToRasbase();
    
    std::string serverAddress = GrpcUtils::constructAddressString("0.0.0.0",  listenPort);
    
    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(rasserverService.get());
    builder.RegisterService(clientServerService.get());
    builder.RegisterService(healthServiceImpl.get());

    this->isRunning = true;
    // Finally assemble the server.

    LDEBUG << "Starting server on:" << serverAddress;
    this->server = builder.BuildAndStart();

    // Register the server
    this->registerServerWithRasmgr();

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    this->server->Wait();
}

void RasnetServer::registerServerWithRasmgr()
{
    std::string rasmgrAddress = GrpcUtils::constructAddressString(rasmgrHost, rasmgrPort);
    std::shared_ptr<grpc::Channel> channel(grpc::CreateCustomChannel(rasmgrAddress, grpc::InsecureChannelCredentials(), GrpcUtils::getDefaultChannelArguments()));

    ::rasnet::service::RasMgrRasServerService::Stub rasmgrRasserverService(channel);
    std::shared_ptr<common::HealthService::Stub> healthService(new common::HealthService::Stub(channel));

    ::rasnet::service::Void response;
    ::rasnet::service::RegisterServerReq request;
    request.set_serverid(serverId);

    if (!GrpcUtils::isServerAlive(healthService, SERVICE_CALL_TIMEOUT))
    {
        throw common::ConnectionFailedException("rasserver failed to connect to rasmgr.");
    }

    grpc::ClientContext context;
    grpc::Status status = rasmgrRasserverService.RegisterServer(&context, request, &response);

    if (!status.ok())
    {
        LERROR << "Could not register rasserver with rasmgr.";
        GrpcUtils::convertStatusToExceptionAndThrow(status);
    }
}

}
