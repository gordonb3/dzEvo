/*
 * Copyright (c) 2016-2023 Gordon Bos <gordon@bosvangennip.nl> All rights reserved.
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
#include "RESTClient.hpp"
#include "API.hpp"
#include "jsoncpp/json.h"

#ifdef DEBUG
#define DRYRUN
#endif

#ifndef HARDWAREFILTER
#define HARDWAREFILTER "Evohome"
#endif


const std::string DomoticzClient::evo_modes[7] = {"Auto", "HeatingOff", "AutoWithEco", "Away", "DayOff", "", "Custom"};


/*
 * Class construct
 */
DomoticzClient::DomoticzClient(std::string host)
{
	m_DomoticzHost = host;
	m_DomoticzHeader.clear();
	m_DomoticzHeader.push_back("Accept: application/json, application/xml, text/json, text/x-json, text/javascript, text/xml");
	m_DomoticzHeader.push_back("content-type: application/json");
	m_DomoticzHeader.push_back("charsets: utf-8");
	m_hwtypes.clear();
	m_useoldapi = false;
}

DomoticzClient::~DomoticzClient()
{
}

/************************************************************************
 *									*
 *	Web client helper						*
 *									*
 ************************************************************************/


/*
 * Execute web query
 */

bool DomoticzClient::send_receive_data(const std::string &szUrl, std::string &szResponse)
{
	std::string myUrl=m_DomoticzHost + "/json.htm?" + szUrl;
	connection::HTTP::method::value HTTP_method = (connection::HTTP::method::value)(connection::HTTP::method::HEAD | connection::HTTP::method::GET);
	std::vector<std::string> vHeaderData;
	return RESTClient::Execute(HTTP_method, myUrl, "", m_DomoticzHeader, szResponse, vHeaderData, false, -1, true);
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
	std::string szResult;
	if (m_useoldapi)
		send_receive_data(domoticz::API::hardware::list, szResult);
	else
		send_receive_data(domoticz::APIv2::hardware::list, szResult);
	Json::Value j_res;
	Json::CharReaderBuilder jBuilder;
	std::unique_ptr<Json::CharReader> jReader(jBuilder.newCharReader());
	if (!jReader->parse(szResult.c_str(), szResult.c_str() + szResult.size(), &j_res, nullptr) || !j_res.isMember("result") || !j_res["result"].isArray())
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
	m_hwtypes.clear();
	std::string szResult;
	if (m_useoldapi)
		send_receive_data(domoticz::API::hardware::gettypes, szResult);
	else
		send_receive_data(domoticz::APIv2::hardware::gettypes, szResult);
	Json::Value j_res;
	Json::CharReaderBuilder jBuilder;
	std::unique_ptr<Json::CharReader> jReader(jBuilder.newCharReader());
	if (!jReader->parse(szResult.c_str(), szResult.c_str() + szResult.size(), &j_res, nullptr) || !j_res.isMember("result") || !j_res["result"].isArray())
		return;

	size_t l = j_res["result"].size();
	for (size_t i = 0; i < l; ++i)
	{
		if (j_res["result"][(int)(i)]["name"].asString().find(needle) != std::string::npos)
		{
			std::string idx = j_res["result"][(int)(i)]["idx"].asString();
			m_hwtypes[idx] = j_res["result"][(int)(i)]["name"].asString();
		}
	}
	if (m_hwtypes.size() < 1)
		throw std::invalid_argument(std::string("could not find Evohome hardware with type filter '") + needle + "'");
}


void DomoticzClient::scan_hardware()
{
	if (m_hwtypes.size() == 0)
		get_hardware_types(HARDWAREFILTER);

	std::vector<hardware>().swap(m_evohardware);
	std::string szResult;
	if (m_useoldapi)
		send_receive_data(domoticz::API::hardware::list, szResult);
	else
		send_receive_data(domoticz::APIv2::hardware::list, szResult);
	Json::Value j_res;
	Json::CharReaderBuilder jBuilder;
	std::unique_ptr<Json::CharReader> jReader(jBuilder.newCharReader());
	if (!jReader->parse(szResult.c_str(), szResult.c_str() + szResult.size(), &j_res, nullptr) || !j_res.isMember("result") || !j_res["result"].isArray())
		return;

	size_t l = j_res["result"].size();
	for (size_t i = 0; i < l; ++i)
	{
		if (j_res["result"][(int)(i)]["Enabled"].asString() == "false")
			continue;

		std::string hw_type = j_res["result"][(int)(i)]["Type"].asString();
		std::map<std::string,std::string>::iterator it = m_hwtypes.find(hw_type);
		if (it != m_hwtypes.end())
		{
			hardware newhw = hardware();

			newhw.HardwareName = j_res["result"][(int)(i)]["Name"].asString();
			newhw.HardwareType = it->second;
			newhw.HardwareTypeVal = j_res["result"][(int)(i)]["Type"].asString();
			newhw.idx = j_res["result"][(int)(i)]["idx"].asString();

			m_evohardware.push_back(newhw);
		}
	}
}


void DomoticzClient::set_hardware_by_name(const std::string hardwarename)
{
	if ((m_evohardware.size() == 1) && (m_evohardware[0].HardwareName == hardwarename))  // already selected
		return;

	if (m_hwtypes.size() == 0)
		get_hardware_types(HARDWAREFILTER);

	std::vector<hardware>().swap(m_evohardware);
	std::string idx = get_hwid_by_name(hardwarename);
	if (idx == "-1")
		return;
	
	std::string szResult;
	if (m_useoldapi)
		send_receive_data(domoticz::API::hardware::list, szResult);
	else
		send_receive_data(domoticz::APIv2::hardware::list, szResult);
	Json::Value j_res;
	Json::CharReaderBuilder jBuilder;
	std::unique_ptr<Json::CharReader> jReader(jBuilder.newCharReader());
	if (!jReader->parse(szResult.c_str(), szResult.c_str() + szResult.size(), &j_res, nullptr) || !j_res.isMember("result") || !j_res["result"].isArray())
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
			newhw.HardwareType = m_hwtypes[newhw.HardwareTypeVal];

			m_evohardware.push_back(newhw);
		}
	}

	// clear dependend vars
	m_devices.clear();
}


bool DomoticzClient::get_devices()
{
	if (m_evohardware.size() < 1)
		scan_hardware();
	m_devices.clear();
	for (std::vector<std::string>::size_type i = 0; i < m_evohardware.size(); ++i)
		get_devices(m_evohardware[i].idx,1);
	return (m_devices.size() > 0);
	return false;
}
bool DomoticzClient::get_devices(const std::string hwid)
{
	return get_devices(hwid,0);
}
bool DomoticzClient::get_devices(const std::string hwid, bool concatenate)
{
	if (!concatenate)
		m_devices.clear();
	std::string szResult;
	if (m_useoldapi)
		send_receive_data(domoticz::API::devices::list, szResult);
	else
		send_receive_data(domoticz::APIv2::devices::list, szResult);
	Json::Value j_res;
	Json::CharReaderBuilder jBuilder;
	std::unique_ptr<Json::CharReader> jReader(jBuilder.newCharReader());
	if (!jReader->parse(szResult.c_str(), szResult.c_str() + szResult.size(), &j_res, nullptr) || !j_res.isMember("result") || !j_res["result"].isArray())
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
			m_devices[idx].idx = idx;
			m_devices[idx].SubType = j_res["result"][(int)(i)]["SubType"].asString();
			m_devices[idx].Name = j_res["result"][(int)(i)]["Name"].asString();

			m_devices[idx].SetPoint = j_res["result"][(int)(i)]["SetPoint"].asString();
			m_devices[idx].Status = j_res["result"][(int)(i)]["Status"].asString();
			m_devices[idx].Until = j_res["result"][(int)(i)]["Until"].asString();
			m_devices[idx].HardwareName = j_res["result"][(int)(i)]["HardwareName"].asString();
			m_devices[idx].Description = j_res["result"][(int)(i)]["Description"].asString();

			// can't trust 'Temp' value for correct number of decimals
			std::string data = j_res["result"][(int)(i)]["Data"].asString();
			m_devices[idx].Temp = data.substr(0,data.find(" "));
		}
	}
	return (m_devices.size() > 0);
}


std::string DomoticzClient::get_device_idx_by_name(const std::string devicename)
{
	if (m_devices.size() == 0)
			get_devices();
	std::map<std::string,device>::iterator it;
	for ( it = m_devices.begin(); it != m_devices.end(); it++ )
	{
		if (it->second.Name == devicename)
			return it->second.idx;
	}
	return "-1";
}


std::string DomoticzClient::get_controller()
{
	if (m_devices.size() == 0)
		get_devices();
	std::map<std::string,device>::iterator it;
	for ( it = m_devices.begin(); it != m_devices.end(); it++ )
	{
		if (it->second.SubType == "Evohome")
			return it->second.idx;
	}
	return "-1";
}
std::string DomoticzClient::get_controller(const std::string hardwarename)
{
	if (!hardwarename.empty())
		set_hardware_by_name(hardwarename);
	return get_controller();
}


std::string DomoticzClient::get_DHW()
{
	if (m_devices.size() == 0)
		get_devices();
	std::map<std::string,device>::iterator it;
	for ( it = m_devices.begin(); it != m_devices.end(); it++ )
	{
		if (it->second.SubType == "Hot Water")
			return it->second.idx;
	}
	return "-1";
}
std::string DomoticzClient::get_DHW(const std::string hardwarename)
{
	if (!hardwarename.empty())
		set_hardware_by_name(hardwarename);
	return get_DHW();
}


void DomoticzClient::set_temperature(const std::string zonename, const std::string setpoint, const std::string until)
{
	std::string idx;
	idx = get_device_idx_by_name(zonename);
	if (idx == "-1")
		throw std::invalid_argument(std::string("zone with name ")+zonename+" does not exist");

	std::string mode = (until.empty()) ? "PermanentOverride" : "TemporaryOverride";

	std::stringstream ss;
	ss << "&idx=" << idx << "&name=" << RESTClient::urlencode(zonename) << "&description=" << RESTClient::urlencode(m_devices[idx].Description)
	   << "&setpoint=" << setpoint << "&mode=" << mode;
	if (!until.empty())
		ss << "&until=" << until;
	ss << "&used=true";

#ifdef DRYRUN
	std::cout << m_DomoticzHost << ss.str() << std::endl;
#else
	std::string szResult;
	if (m_useoldapi)
		send_receive_data(domoticz::API::evohome::settemp + ss.str(), szResult);
	else
		send_receive_data(domoticz::APIv2::evohome::settemp + ss.str(), szResult);
#endif
}


void DomoticzClient::cancel_temperature_override(const std::string zonename)
{
	std::string idx;
	idx = get_device_idx_by_name(zonename);
	if (idx == "-1")
		throw std::invalid_argument(std::string("zone with name ")+zonename+" does not exist");

	std::stringstream ss;
	ss << "&idx=" << idx << "&name=" << RESTClient::urlencode(zonename) << "&description=" << RESTClient::urlencode(m_devices[idx].Description)
	   << "&setpoint=" << m_devices[idx].SetPoint << "&mode=Auto&used=true";

#ifdef DRYRUN
	std::cout << m_DomoticzHost << ss.str() << std::endl;
#else
	std::string szResult;
	if (m_useoldapi)
		send_receive_data(domoticz::API::evohome::settemp + ss.str(), szResult);
	else
		send_receive_data(domoticz::APIv2::evohome::settemp + ss.str(), szResult);
#endif
}


void DomoticzClient::set_system_mode(const std::string mode)
{
	std::string idx;
	idx = get_controller();
	if (idx == "-1")
		throw std::invalid_argument("could not find Evohome controller in Domoticz");

	uint8_t nZoneMode = 0;
	while ((mode != evo_modes[nZoneMode]) && (nZoneMode < 7))
		nZoneMode++;
	if (nZoneMode >= 7)
		throw std::invalid_argument("invalid system mode - please verify your syntax");

	std::stringstream ss;
	ss << "&idx=" << idx << "&status=" << mode << "&action=1";

#ifdef DRYRUN
	std::cout << m_DomoticzHost << ss.str() << std::endl;
#else
	std::string szResult;
	if (m_useoldapi)
		send_receive_data(domoticz::API::evohome::setsysmode + ss.str(), szResult);
	else
		send_receive_data(domoticz::APIv2::evohome::setsysmode + ss.str(), szResult);
#endif
}
void DomoticzClient::set_system_mode(const std::string mode, const std::string hardwarename)
{
	if (!hardwarename.empty())
		set_hardware_by_name(hardwarename);
	set_system_mode(mode);
}



void DomoticzClient::set_DHW_state(const std::string state, const std::string until)
{
	std::string idx;
	idx = get_DHW();
	if (idx == "-1")
		throw std::invalid_argument("could not find Hot Water device in Domoticz");

	std::string mode = (until.empty()) ? "PermanentOverride" : "TemporaryOverride";

	std::stringstream ss;
	ss << "&idx=" << idx << "&name=" << RESTClient::urlencode(m_devices[idx].Name) << "&description=" << RESTClient::urlencode(m_devices[idx].Description)
	   << "&state=" << state << "&mode=" << mode;
	if (!until.empty())
		ss << "&until=" << until;
	ss << "&used=true";

#ifdef DRYRUN
	std::cout << m_DomoticzHost << ss.str() << std::endl;
#else
	std::string szResult;
	if (m_useoldapi)
		send_receive_data(domoticz::API::evohome::settemp + ss.str(), szResult);
	else
		send_receive_data(domoticz::APIv2::evohome::settemp + ss.str(), szResult);
#endif
}
void DomoticzClient::set_DHW_state(const std::string state, const std::string until, const std::string hardwarename)
{
	if (!hardwarename.empty())
		set_hardware_by_name(hardwarename);
	set_DHW_state(state, until);
}


void DomoticzClient::set_oldapi()
{
	m_useoldapi = true;
}



