/*
 * Copyright (c) 2016 Gordon Bos <gordon@bosvangennip.nl> All rights reserved.
 *
 * Json client for Domoticz
 *
 *
 * Source code subject to GNU GENERAL PUBLIC LICENSE version 3
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include "domoticzclient.h"
#include "webclient.h"
#include "jsoncpp/json.h"

#ifdef DEBUG
#define DRYRUN
#endif

#ifndef HARDWAREFILTER
#define HARDWAREFILTER "Evohome"
#endif


//const char DomoticzClient::m_szControllerMode[7][20]={"Normal","Economy","Away","Day Off","Custom","Heating Off","Unknown"};
//const char DomoticzClient::m_szWebAPIMode[7][20]={"Auto","AutoWithEco","Away","DayOff","Custom","HeatingOff","Unknown"};
//const char DomoticzClient::m_szZoneMode[7][20]={"Auto","PermanentOverride","TemporaryOverride","OpenWindow","LocalOverride","RemoteOverride","Unknown"};

//const uint8_t CEvohomeWeb::m_dczToEvoWebAPIMode[7] = { 0,2,3,4,6,1,5 };


const std::string DomoticzClient::evo_modes[7] = {"Auto", "HeatingOff", "AutoWithEco", "Away", "DayOff", "", "Custom"};



/*
 * Class construct
 */
DomoticzClient::DomoticzClient(std::string host)
{
	domoticzhost = host;
	domoticzheader.clear();
	domoticzheader.push_back("Accept: application/json, application/xml, text/json, text/x-json, text/javascript, text/xml");
	domoticzheader.push_back("content-type: application/json");
	domoticzheader.push_back("charsets: utf-8");
	init();
}

DomoticzClient::~DomoticzClient()
{
	cleanup();
}

/************************************************************************
 *									*
 *	Web client helpers						*
 *									*
 ************************************************************************/

/*
 * Initialize web client
 */
void DomoticzClient::init()
{
	hwtypes.clear();
	web_connection_init("domoticz");
}


/*
 * Cleanup web client
 */
void DomoticzClient::cleanup()
{
	web_connection_cleanup("domoticz");
}

/*
 * Execute web query
 */
std::string DomoticzClient::send_receive_data(std::string url)
{
	try
	{
		return send_receive_data(url, "");
	}
	catch (...)
	{
		throw;
	}
	return "";
}

std::string DomoticzClient::send_receive_data(std::string url, std::string postdata)
{
	std::stringstream ss_url;
	ss_url << domoticzhost << url;
	std::string s_res;
	try
	{
		s_res = web_send_receive_data("domoticz", ss_url.str(), postdata, domoticzheader);
	}
	catch (...)
	{
		throw;
	}
	return s_res;
}


/************************************************************************
 *									*
 *	Private tools							*
 *									*
 ************************************************************************/

std::string _int_to_string(const int myint)
{
	std::stringstream ss;
	ss << myint;
	return ss.str();
}


/************************************************************************
 *									*
 *	Public tools							*
 *									*
 ************************************************************************/

std::string DomoticzClient::get_hwid_by_name(const std::string hardwarename)
{
	std::string s_res;
	try
	{
		s_res = send_receive_data("/json.htm?type=hardware");
	}
	catch (...)
	{
		throw;
	}
	Json::Value j_res;
	Json::Reader jReader;
	if (!jReader.parse(s_res.c_str(), j_res) || !j_res.isMember("result") || !j_res["result"].isArray())
		return "-1";

	size_t l = j_res["result"].size();
	for (size_t i = 0; i < l; ++i)
	{
		if (j_res["result"][(int)(i)]["Name"].asString() == hardwarename)
			return j_res["result"][(int)(i)]["idx"].asString();
	}
	return "-1";
}


void DomoticzClient::get_hardware_types(const std::string needle)
{
	hwtypes.clear();
	std::string s_res;
	try
	{
		s_res = send_receive_data("/json.htm?type=command&param=gethardwaretypes");
	}
	catch (...)
	{
		throw;
	}
	Json::Value j_res;
	Json::Reader jReader;
	if (!jReader.parse(s_res.c_str(), j_res) || !j_res.isMember("result") || !j_res["result"].isArray())
		return;

	size_t l = j_res["result"].size();
	for (size_t i = 0; i < l; ++i)
	{
		if (j_res["result"][(int)(i)]["name"].asString().find(needle) != std::string::npos)
		{
			std::string idx = j_res["result"][(int)(i)]["idx"].asString();
			hwtypes[idx] = j_res["result"][(int)(i)]["name"].asString();
		}
	}
	if (hwtypes.size() < 1)
		throw std::invalid_argument(std::string("could not find Evohome hardware with type filter '")+needle+"'");
}


void DomoticzClient::scan_hardware()
{
	if (hwtypes.size() == 0)
		get_hardware_types(HARDWAREFILTER);

	std::vector<hardware>().swap(installations);
	std::string s_res;
	try
	{
		s_res = send_receive_data("/json.htm?type=hardware");
	}
	catch (...)
	{
		throw;
	}
	Json::Value j_res;
	Json::Reader jReader;
	if (!jReader.parse(s_res.c_str(), j_res) || !j_res.isMember("result") || !j_res["result"].isArray())
		return;

	size_t l = j_res["result"].size();
	for (size_t i = 0; i < l; ++i)
	{
		if (j_res["result"][(int)(i)]["Enabled"].asString() == "false")
			continue;

		std::string hw_type = j_res["result"][(int)(i)]["Type"].asString();
		std::map<std::string,std::string>::iterator it = hwtypes.find(hw_type);
		if (it != hwtypes.end())
		{
			hardware newhw = hardware();

			newhw.HardwareName = j_res["result"][(int)(i)]["Name"].asString();
			newhw.HardwareType = it->second;
			newhw.HardwareTypeVal = j_res["result"][(int)(i)]["Type"].asString();
			newhw.idx = j_res["result"][(int)(i)]["idx"].asString();

			installations.push_back(newhw);
		}
	}
}


void DomoticzClient::set_hardware_by_name(const std::string hardwarename)
{
	if ((installations.size() == 1) && (installations[0].HardwareName == hardwarename))  // already selected
		return;

	if (hwtypes.size() == 0)
		get_hardware_types(HARDWAREFILTER);

	std::vector<hardware>().swap(installations);
	std::string idx = get_hwid_by_name(hardwarename);
	std::string s_res;
	try
	{
		s_res = send_receive_data("/json.htm?type=hardware");
	}
	catch (...)
	{
		throw;
	}
	Json::Value j_res;
	Json::Reader jReader;
	if (!jReader.parse(s_res.c_str(), j_res) || !j_res.isMember("result") || !j_res["result"].isArray())
		return;

	size_t l = j_res["result"].size();
	for (size_t i = 0; i < l; ++i)
	{
		if (j_res["result"][(int)(i)]["Name"].asString() == hardwarename)
		{
			hardware newhw = hardware();

			newhw.idx = j_res["result"][(int)(i)]["idx"].asString();
			newhw.HardwareName = hardwarename;
			newhw.HardwareTypeVal = j_res["result"][(int)(i)]["Type"].asString();
			newhw.HardwareType = hwtypes[newhw.HardwareTypeVal];

			installations.push_back(newhw);
		}
	}

	// clear dependend vars
	devices.clear();
}


bool DomoticzClient::get_devices()
{
	try
	{
		if (installations.size() < 1)
			scan_hardware();
		devices.clear();
		for (std::vector<std::string>::size_type i = 0; i < installations.size(); ++i)
			get_devices(installations[i].idx,1);
		return (devices.size() > 0);
	}
	catch (...)
	{
		throw;
	}
	return false;
}
bool DomoticzClient::get_devices(const std::string hwid)
{
	try
	{
		return get_devices(hwid,0);
	}
	catch (...)
	{
		throw;
	}
	return false;
}
bool DomoticzClient::get_devices(const std::string hwid, bool concatenate)
{
	if (!concatenate)
		devices.clear();
	std::string s_res;
	try
	{
		s_res = send_receive_data("/json.htm?type=devices&displayhidden=1&used=all");
	}
	catch (...)
	{
		throw;
	}
	Json::Value j_res;
	Json::Reader jReader;
	if (!jReader.parse(s_res.c_str(), j_res) || !j_res.isMember("result") || !j_res["result"].isArray())
		return false;

	for (size_t i = 0; i < j_res["result"].size(); i++)
	{
		if (j_res["result"][(int)(i)]["HardwareID"].asString() == hwid)
		{
			
#ifndef DEBUG
			if (j_res["result"][(int)(i)]["Used"].asString() == "0")
				continue;
#endif
			std::string idx = j_res["result"][(int)(i)]["idx"].asString();
			devices[idx].idx = idx;
			devices[idx].SubType = j_res["result"][(int)(i)]["SubType"].asString();
			devices[idx].Name = j_res["result"][(int)(i)]["Name"].asString();

			devices[idx].SetPoint = j_res["result"][(int)(i)]["SetPoint"].asString();
			devices[idx].Status = j_res["result"][(int)(i)]["Status"].asString();
			devices[idx].Until = j_res["result"][(int)(i)]["Until"].asString();
			devices[idx].HardwareName = j_res["result"][(int)(i)]["HardwareName"].asString();
			devices[idx].Description = j_res["result"][(int)(i)]["Description"].asString();

			// can't trust 'Temp' value for correct number of decimals
			std::string data = j_res["result"][(int)(i)]["Data"].asString();
			devices[idx].Temp = data.substr(0,data.find(" "));
		}
	}
	return (devices.size() > 0);
}


std::string DomoticzClient::get_device_idx_by_name(const std::string devicename)
{
	if (devices.size() == 0)
	{
		try
		{
			get_devices();
		}
		catch (...)
		{
			throw;
		}
	}
	std::map<std::string,device>::iterator it;
	for ( it = devices.begin(); it != devices.end(); it++ )
	{
		if (it->second.Name == devicename)
			return it->second.idx;
	}
	return "-1";
}


std::string DomoticzClient::get_controller()
{
	if (devices.size() == 0)
	{
		try
		{
			get_devices();
		}
		catch (...)
		{
			throw;
		}
	}
	std::map<std::string,device>::iterator it;
	for ( it = devices.begin(); it != devices.end(); it++ )
	{
		if (it->second.SubType == "Evohome")
			return it->second.idx;
	}
	return "-1";
}
std::string DomoticzClient::get_controller(const std::string hardwarename)
{
	try
	{
		if (!hardwarename.empty())
			set_hardware_by_name(hardwarename);
		return get_controller();
	}
	catch (...)
	{
		throw;
	}
	return "-1";
}


std::string DomoticzClient::get_DHW()
{
	if (devices.size() == 0)
	{
		try
		{
			get_devices();
		}
		catch (...)
		{
			throw;
		}
	}
	std::map<std::string,device>::iterator it;
	for ( it = devices.begin(); it != devices.end(); it++ )
	{
		if (it->second.SubType == "Hot Water")
			return it->second.idx;
	}
	return "-1";
}
std::string DomoticzClient::get_DHW(const std::string hardwarename)
{
	try
	{
		if (!hardwarename.empty())
			set_hardware_by_name(hardwarename);
		return get_DHW();
	}
	catch (...)
	{
		throw;
	}
	return "-1";
}


void DomoticzClient::set_temperature(const std::string zonename, const std::string setpoint, const std::string until)
{
	std::string idx;
	try
	{
		idx = get_device_idx_by_name(zonename);
	}
	catch (...)
	{
		throw;
	}
	if (idx == "-1")
		throw std::invalid_argument(std::string("zone with name ")+zonename+" does not exist");

	std::string mode = (until.empty()) ? "PermanentOverride" : "TemporaryOverride";

	std::stringstream ss;
	ss << "/json.htm?type=setused&idx=" << idx << "&name=" << zonename << "&description=" << devices[idx].Description
	   << "&setpoint=" << setpoint << "&mode=" << mode;
	if (!until.empty())
		ss << "&until=" << until;
	ss << "&used=true";

#ifdef DRYRUN
	std::cout << domoticzhost << ss.str() << std::endl;
#else

	try
	{
		send_receive_data(ss.str());
	}
	catch (...)
	{
		throw;
	}
#endif
}


void DomoticzClient::cancel_temperature_override(const std::string zonename)
{
	std::string idx;
	try
	{
		idx = get_device_idx_by_name(zonename);
	}
	catch (...)
	{
		throw;
	}
	if (idx == "-1")
		throw std::invalid_argument(std::string("zone with name ")+zonename+" does not exist");

	std::stringstream ss;
	ss << "/json.htm?type=setused&idx=" << idx << "&name=" << zonename << "&description=" << devices[idx].Description
	   << "&setpoint=" << devices[idx].SetPoint << "&mode=Auto&used=true";

#ifdef DRYRUN
	std::cout << domoticzhost << ss.str() << std::endl;
#else

	try
	{
		send_receive_data(ss.str());
	}
	catch (...)
	{
		throw;
	}
#endif
}


void DomoticzClient::set_system_mode(const std::string mode)
{
	std::string idx;
	try
	{
		idx = get_controller();
	}
	catch (...)
	{
		throw;
	}
	if (idx == "-1")
		throw std::invalid_argument("could not find Evohome controller in Domoticz");

	uint8_t nZoneMode = 0;
	while ((mode != evo_modes[nZoneMode]) && (nZoneMode < 7))
		nZoneMode++;
	if (nZoneMode >= 7)
		throw std::invalid_argument("invalid system mode - please verify your syntax");


	std::stringstream ss;
	ss << "/json.htm?type=command&param=switchmodal&idx=" << idx << "&status=" << mode << "&action=1";

#ifdef DRYRUN
	std::cout << domoticzhost << ss.str() << std::endl;
#else

	try
	{
		send_receive_data(ss.str());
	}
	catch (...)
	{
		throw;
	}
#endif
}
void DomoticzClient::set_system_mode(const std::string mode, const std::string hardwarename)
{
	try
	{
		if (!hardwarename.empty())
			set_hardware_by_name(hardwarename);
		set_system_mode(mode);
	}
	catch (...)
	{
		throw;
	}
}



void DomoticzClient::set_DHW_state(const std::string state, const std::string until)
{
	std::string idx;
	try
	{
		idx = get_DHW();
	}
	catch (...)
	{
		throw;
	}
	if (idx == "-1")
		throw std::invalid_argument("could not find Hot Water device in Domoticz");

	std::string mode = (until.empty()) ? "PermanentOverride" : "TemporaryOverride";

	std::stringstream ss;
	ss << "/json.htm?type=setused&idx=" << idx << "&name=" << devices[idx].Name << "&description=" << devices[idx].Description
	   << "&state=" << state << "&mode=" << mode << "&used=true";


#ifdef DRYRUN
	std::cout << domoticzhost << ss.str() << std::endl;
#else

	try
	{
		send_receive_data(ss.str());
	}
	catch (...)
	{
		throw;
	}
#endif
}
void DomoticzClient::set_DHW_state(const std::string state, const std::string until, const std::string hardwarename)
{
	try
	{
		if (!hardwarename.empty())
			set_hardware_by_name(hardwarename);
		set_DHW_state(state, until);
	}
	catch (...)
	{
		throw;
	}
}












