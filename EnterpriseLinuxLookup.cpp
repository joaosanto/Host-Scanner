#include "EnterpriseLinuxLookup.h"
#include "Utils.h"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using namespace std;
using namespace boost;

unordered_set<string> EnterpriseLinuxLookup::FindVulnerability(const string& cve, OpSys distrib, double ver)
{
	VendorVulnInfo vuln;

	if (!ValidateCVE(cve))
	{
		log(ERR, "Specified CVE identifier '" + cve + "' is not syntactically valid.");
		return vuln.Packages;
	}

	if (distrib != EnterpriseLinux && distrib != Fedora)
	{
		log(ERR, "Specified distribution is not supported by this instance.");
		return vuln.Packages;
	}

	auto resp = getURL("https://bugzilla.redhat.com/show_bug.cgi?ctype=xml&id=" + cve);

	if (get<2>(resp) != 200)
	{
		if (get<2>(resp) == -1)
		{
			log(ERR, "Failed to send HTTP request: " + get<1>(resp));
		}
		else
		{
			log(ERR, "Failed to get reply: HTTP response code was " + to_string(get<2>(resp)) + ".");
		}

		return vuln.Packages;
	}

	auto html = get<0>(resp);

	// pkg -> name of the package
	// dist -> distribution (rhel-7, rhscl-1, fedora-all, etc)
	// status -> vulnerability status (affected, notaffected)
	static regex tblrgx("(?<dist>(?:rhel|rhscl|fedora)-(?:\\d+|all))\\/(?<pkg>[^=]+)=(?<status>[^,]+)", regex::icase);

	// res -> resolution (NOTABUG, ERRATA)
	static regex stsrgx("<resolution>(?<res>[^<]+)<\\/resolution>", regex::icase);

	// lst -> if res is ERRATA, comma-separated list of version numbers which fix the vulnerability
	static regex cfirgx("<cf_fixed_in>(?<lst>[^<]+)<\\/cf_fixed_in>", regex::icase);

	sregex_iterator srit(html.begin(), html.end(), tblrgx);
	sregex_iterator end;

	auto tdist = string(distrib == EnterpriseLinux ? "rhel" : "fedora");
	auto tdall = tdist + "-all";
	auto tdver = ver != 0 ? tdist + "-" + to_string(ver) : tdall;

	for (; srit != end; ++srit)
	{
		auto m = *srit;

		auto pkg  = m["pkg"].str();
		auto dist = m["dist"].str();
		auto sts  = m["status"].str();

		if (dist != tdall && dist != tdver)
		{
			continue;
		}

		if (sts != "notaffected")
		{
			// placeholder for now

			vuln.Fixes.emplace(pkg, "");
		}

		vuln.Packages.emplace(pkg);
	}

	smatch cfism;

	if (regex_search(html, cfism, cfirgx))
	{
		auto lst = cfism["lst"].str();

		vector<string> strs;
		split(strs, lst, is_any_of(","));

		for (auto& str : strs)
		{
			trim(str);

			auto spc = str.find_last_of(" ");

			if (spc == string::npos)
			{
				continue;
			}

			auto spkg = str.substr(0, spc);
			auto sver = str.substr(spc + 1);

			for (auto& fix : vuln.Fixes)
			{
				if (fix.second != "" || fix.first != spkg)
				{
					continue;
				}

				fix.second = sver;
			}
		}
	}

	return vuln.Packages;
}

string EnterpriseLinuxLookup::GetUpgradeCommand(const unordered_set<string>& pkgs, OpSys distrib, double ver)
{
	if (pkgs.empty())
	{
		return "";
	}

	string cmd;

	if (distrib == OpSys::Fedora && ver >= 22)
	{
		cmd = "sudo dnf update";
	}
	else
	{
		cmd = "sudo yum update";
	}

	for (auto& pkg : pkgs)
	{
		cmd += " " + pkg;
	}

	return cmd;
}

EnterpriseLinuxLookup::~EnterpriseLinuxLookup()
{
}
