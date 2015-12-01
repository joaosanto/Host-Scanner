#pragma once
#include "Stdafx.h"
#include "HostScanner.h"
#include <string>
#include <boost/property_tree/ptree_fwd.hpp>

/*!
* Implements a passive scanner which returns Censys data.
*/
class CensysScanner : public HostScanner
{
public:
	
	/*!
	 * API username and password to use for the requests.
	 */
	std::string auth;

	/*!
	 * API endpoint location.
	 */
	std::string endpoint = "censys.io/api/v1";

	/*!
	 * Scans a host to determine service availability.
	 * 
	 * \param host Host.
	 */
	void Scan(Host* host) override;

	/*!
	 * Scans a list of hosts to determine service availability.
	 * 
	 * \param hosts List of hosts.
	 */
	void Scan(Hosts* hosts) override;

	/*!
	 * Frees up the resources allocated during the lifetime of this instance.
	 */
	~CensysScanner() override;

private:

	/*!
	 * Gets the information available on the API for the specified host.
	 *
	 * \param host Host.
	 */
	void getHostInfo(Host* host);

	/*!
	 * Traverses the supplied property tree and tries to find a relevant service banner.
	 *
	 * \param pt Property tree to traverse.
	 *
	 * \return Service banner, if any.
	 */
	std::string findServiceBanner(boost::property_tree::ptree pt);

};