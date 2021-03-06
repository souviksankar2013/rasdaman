#include <limits.h> //PATH_MAX

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <stdexcept>

#include <logging.hh>

#include "include/globals.hh"
#include "common/exceptions/missingresourceexception.hh"
#include "common/uuid/uuid.hh"

#include "constants.hh"
#include "controlcommandexecutor.hh"
#include "databasehostmanager.hh"
#include "databasemanager.hh"
#include "peermanager.hh"
#include "servermanager.hh"
#include "usermanager.hh"

#include "configurationmanager.hh"

namespace rasmgr
{
using std::runtime_error;

ConfigurationManager::ConfigurationManager(std::shared_ptr<ControlCommandExecutor> commandExecutor,
        std::shared_ptr<DatabaseHostManager> dbhManager,
        std::shared_ptr<DatabaseManager> dbManager,
        std::shared_ptr<PeerManager> peerManager,
        std::shared_ptr<ServerManager> serverManager,
        std::shared_ptr<UserManager> userManager)
    : commandExecutor_(commandExecutor),
      dbhManager_(dbhManager),
      dbManager_(dbManager),
      peerManager_(peerManager),
      serverManager_(serverManager),
      userManager_(userManager),
      rasmgrConfFilePath(std::string(CONFDIR) + "/" + std::string(RASMGR_CONF_FILE)),
      isDirty_(false)
{}

ConfigurationManager::~ConfigurationManager()
{}

void ConfigurationManager::saveConfiguration(bool backup)
{
    if (backup)
    {
        if (this->isDirty())
        {
            // Append a unique ID to the file.
            this->saveRasMgrConf(true);
            this->userManager_->saveUserInformation(true);
        }
    }
    else
    {
        this->userManager_->saveUserInformation();
        this->saveRasMgrConf();
    }

    this->setIsDirty(false);
}

void ConfigurationManager::loadConfiguration()
{
    //Will not throw an exception
    this->userManager_->loadUserInformation();

    try
    {
        this->loadRasMgrConf();
    }
    catch (std::exception &ex)
    {
        LERROR << ex.what();
    }
    catch (...)
    {
        LERROR << "Failed to load" << RASMGR_CONF_FILE;
    }

    this->setIsDirty(false);
}

bool ConfigurationManager::isDirty() const
{
    return isDirty_;
}

void ConfigurationManager::setIsDirty(bool isDirty)
{
    isDirty_ = isDirty;
}

void ConfigurationManager::loadRasMgrConf()
{
    if (rasmgrConfFilePath.length() >= PATH_MAX)
    {
        throw runtime_error("The path to the configuration file is longer than the maximum file system path.");
    }

    LDEBUG << "Opening rasmanager configuration file:" << rasmgrConfFilePath;

    std::ifstream ifs(rasmgrConfFilePath);    // open config file

    if (!ifs)
    {
        throw common::MissingResourceException(rasmgrConfFilePath);
    }
    else
    {
        char inBuffer[MAX_CONTROL_COMMAND_LENGTH];

        while (!ifs.eof())
        {
            ifs.getline(inBuffer, MAX_CONTROL_COMMAND_LENGTH);

            std::string result = commandExecutor_->sudoExecuteCommand(std::string(inBuffer));

            //Only error messages are non-empty
            if (!result.empty())
            {
                LERROR << result;
            }
        }
    }
}

void ConfigurationManager::saveRasMgrConf(bool backup)
{
    std::string confFilePath = backup ? (rasmgrConfFilePath + "." + common::UUID::generateUUID()) : rasmgrConfFilePath;

    if (confFilePath.length() >= PATH_MAX)
    {
        throw runtime_error("The path to the configuration file is longer than the maximum file system path.");
    }

    std::ofstream ofs(confFilePath);

    ofs << "# rasmgr config file (v1.1)" << std::endl;
    ofs << "# warning: do not edit this file, it may be overwritten by rasmgr!" << std::endl;
    ofs << "#" << std::endl;

    saveDatabaseHosts(ofs);
    saveDatabases(ofs);
    saveServers(ofs);
    savePeers(ofs);

    ofs.close();
}

void ConfigurationManager::saveDatabaseHosts(std::ofstream &out)
{
    DatabaseHostMgrProto dbhMgrData = this->dbhManager_->serializeToProto();
    for (int i = 0; i < dbhMgrData.database_hosts_size(); ++i)
    {
        DatabaseHostProto dbhData = dbhMgrData.database_hosts(i);

        out << "define dbh " << dbhData.host_name()
            << " -connect " << dbhData.connect_string();
        if (!dbhData.user_name().empty())
        {
            out << " -user " << dbhData.user_name();
        }
        if (!dbhData.password().empty())
        {
            out << " -passwd " << dbhData.password();
        }

        out << std::endl;
    }
}

void ConfigurationManager::saveDatabases(std::ofstream &out)
{
    DatabaseMgrProto dbManagerData = this->dbManager_->serializeToProto();

    for (int i = 0; i < dbManagerData.databases_size(); ++i)
    {
        auto databaseData = dbManagerData.databases(i);

        out << "define db " << databaseData.database().name()
            << " -dbh " << databaseData.database_host()
            << std::endl;
    }
}

void ConfigurationManager::saveServers(std::ofstream &out)
{
    ServerMgrProto serverManagerData = this->serverManager_->serializeToProto();
    for (int i = 0; i < serverManagerData.server_groups_size(); ++i)
    {
        ServerGroupProto serverGroup = serverManagerData.server_groups(i);
        out << "define srv " << serverGroup.name()
            << " -host " << serverGroup.host();

        // Store ports in an array for sorting.
        std::vector<std::int32_t> ports;
        for (int j = 0; j < serverGroup.ports_size(); ++j)
        {
            ports.push_back(serverGroup.ports(j));
        }

        //Sort the list of ports
        std::sort(ports.begin(), ports.end());

        out << " -port ";
        for (std::uint32_t j = 0; j < ports.size(); ++j)
        {
            std::uint32_t start = j;
            std::uint32_t end = j;

            for (std::uint32_t k = j + 1; k < ports.size(); ++k)
            {
                if (ports[k - 1] + 1 == ports[k])
                {
                    end++;
                }
                else
                {
                    break;
                }
            }

            j = end;

            if (start != end)
            {
                out << " " << ports[start] << "-" << ports[end];
            }
            else
            {
                out << " " << ports[start];
            }

            if (j != ports.size() - 1)
            {
                out << ",";
            }
        }

        out << " -dbh " << serverGroup.db_host()
            << " -autorestart " << (serverGroup.autorestart() ? "on" : "off")
            << " -countdown " << serverGroup.countdown();

        if (serverGroup.has_min_alive_server_no())
        {
            out << " -alive " << serverGroup.min_alive_server_no();
        }

        if (serverGroup.has_min_available_server_no())
        {
            out << " -available " << serverGroup.min_available_server_no();
        }

        if (serverGroup.has_max_idle_server_no())
        {
            out << " -idle " << serverGroup.max_idle_server_no();
        }
        if (serverGroup.has_server_options())
        {
            out << " -xp " << serverGroup.server_options();
        }

        out << std::endl;
    }
}

void ConfigurationManager::savePeers(std::ofstream &out)
{
    PeerMgrProto peerManagerData = this->peerManager_->serializeToProto();

    for (int i = 0; i < peerManagerData.outpeers_size(); ++i)
    {
        out << "define outpeer " << peerManagerData.outpeers(i).host_name()
            << " -port " << peerManagerData.outpeers(i).port()
            << std::endl;
    }

    for (int i = 0; i < peerManagerData.inpeers_size(); ++i)
    {
        out << "define inpeer " << peerManagerData.inpeers(i).host_name()
            << std::endl;
    }
}

}
