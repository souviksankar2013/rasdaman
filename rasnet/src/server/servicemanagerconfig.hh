/*
 * This file is part of rasdaman community.
 *
 * Rasdaman community is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Rasdaman community is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with rasdaman community.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014 Peter Baumann / rasdaman GmbH.
 *
 * For more information please see <http://www.rasdaman.org>
 * or contact Peter Baumann via <baumann@rasdaman.com>.
 */

#ifndef RASNET_SRC_SERVER_SERVICEMANAGERCONFIG_HH
#define RASNET_SRC_SERVER_SERVICEMANAGERCONFIG_HH

#include <boost/cstdint.hpp>
namespace rasnet
{
class ServiceManagerConfig
{
public:
    ServiceManagerConfig();

    boost::int32_t getAliveTimeout() const;
    void setAliveTimeout(const boost::int32_t &value);

    boost::int32_t getAliveRetryNo() const;
    void setAliveRetryNo(const boost::int32_t &value);

    boost::int32_t getIoThreadsNo() const;
    void setIoThreadsNo(const boost::int32_t &value);

    boost::uint32_t getCpuThreadsNo() const;
    void setCpuThreadsNo(const boost::uint32_t &value);

    boost::int32_t getMaxOpenSockets() const;
    void setMaxOpenSockets(const boost::int32_t &value);

private:
    boost::int32_t aliveTimeout;
    boost::int32_t aliveRetryNo;
    boost::int32_t ioThreadsNo;
    boost::uint32_t cpuThreadsNo;
    boost::int32_t maxOpenSockets;
};
}
#endif // RASNET_SRC_SERVER_SERVICEMANAGERCONFIG_HH