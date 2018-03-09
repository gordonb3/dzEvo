/*
 * Copyright (c) 2016 Gordon Bos <gordon@bosvangennip.nl> All rights reserved.
 *
 * Demo app for connecting to Evohome and Domoticz
 *
 *
 *
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <cstring>
#include <time.h>
#include <stdlib.h>
#include "../domoticzclient/domoticzclient.h"


#ifndef CONFIGFILE
#define CONFIGFILE "dzEvo.conf"
#endif

#ifdef _WIN32
#define localtime_r(timep, result) localtime_s(result, timep)
#define gmtime_r(timep, result) gmtime_s(result, timep)
#endif

#ifndef _WIN32
#define sprintf_s(buffer, buffer_size, stringbuffer, ...) (sprintf(buffer, stringbuffer, __VA_ARGS__))
#endif



using namespace std;

// Global variables
std::string configfile;
std::vector<std::string> myinstallations;
std::map<std::string, std::string> evoconfig;

const std::string my_evo_modes[7] = {"auto", "off", "economy", "away", "holiday", "unknown", "custom"};



void usage(std::string mode)
{
	if (mode == "badparm")
	{
		cout << "Bad parameter" << endl;
		exit(1);
	}

	if (mode == "query")
	{
		cout << "Usage: dzEvo query SUBJECT" << endl;
		cout << endl;
		cout << "SUBJECT := { hardware | devices | zone | system }" << endl;
		cout << endl;
		cout << "  hardware\t\t\tlist Evohome hardware in Domoticz" << endl;
		cout << endl;
		cout << "  devices [hardware name]\tlist Evohome devices in Domoticz" << endl;
		cout << endl;
		cout << "  zone <zone name>\t\tshow a zone's status" << endl;
		cout << endl;
		cout << "  system [hardware name]\tshow system status" << endl;
		cout << endl;
		exit(0);
	}


	if (mode == "set")
	{
		cout << "Usage: dzEvo set SUBJECT" << endl;
		cout << endl;
		cout << "SUBJECT := { temperature | system }" << endl;
		cout << endl;
		cout << "  temperature <zone name> SETPOINT DURATION\tchange a zone's setpoint" << endl;
		cout << endl;
		cout << "  system [hardware name] MODE\t\t\tchange the system mode" << endl;
		cout << endl;
		exit(0);
	}

	if (mode == "settemp")
	{
		cout << "Usage: dzEvo set temperature <zone name> SETPOINT UNTIL" << endl;
		cout << endl;
		cout << "SETPOINT := { <setpoint> | auto | [+-][1..3]}" << endl;
		cout << endl;
		cout << "UNTIL := { until <ISO time>[Z] | <ISO time>[Z] | for <minutes> | +<minutes> | [auto] (default) | keep | permanent }" << endl;
		cout << endl;
		cout << "  setpoint\t\ta numerical value between 5 and 35 that will be rounded to half degree steps" << endl;
		cout << "\t\t\tdecimal sign may be either point or comma" << endl;
		cout << endl;
		cout << "  +/- 1..3\t\tincrease or decrease the current setpoint by 1, 2 or 3 degrees" << endl;
		cout << endl;
		cout << "  ISO time\t\ta datetime string with format \"YYYY-MM-DD HH:mm:ss\"" << endl;
		cout << "\t\t\tdate an time parts may be joined by a capital 'T' to avoid needing quotes" << endl;
		cout << "\t\t\ttimes are localtime, append a 'Z' to specify UTC time" << endl;
		cout << endl;
		cout << "  minutes\t\tan integer value typically larger than 5 to allow the setting to be propagated" << endl;
		cout << "\t\t\tthrough the whole system" << endl;
		cout << endl;
		cout << "  auto\t\t\tselect from Evohome schedule (if available in Domoticz)" << endl;
		cout << "\t\t\tchanging the setpoint to auto will cancel any active override" << endl;
		cout << endl;
		cout << "  keep\t\t\tkeep whatever Domoticz reports as the current end time" << endl;
		cout << "\t\t\tthis will result in a permanent override if Domoticz reports an empty value" << endl;
		cout << endl;
		cout << "  permanent\t\tset a permanent override" << endl;
		cout << endl;
		exit(0);
	}

	if (mode == "setsystem")
	{
		cout << "Usage: dzEvo set system [hardware name] MODE" << endl;
		cout << endl;
		cout << "MODE := { auto | off | economy | away | holiday | custom }" << endl;
		cout << endl;
		exit(0);
	}

//	if (mode == "general")
	{
		cout << "Usage: dzEvo COMMAND" << endl;
		cout << endl;
		cout << "COMMAND := { query | set }" << endl;
		cout << "  query\t\tquery Evohome information" << endl;
		cout << "  set\t\tchange a setpoint or system mode" << endl;
		cout << endl;
		exit(0);
	}
}


void exit_error(std::string message)
{
	cerr << message << endl;
	exit(1);
}


bool read_evoconfig(std::string configfile)
{
	ifstream myfile (configfile.c_str());
	if ( myfile.is_open() )
	{
		stringstream key,val;
		bool isKey = true;
		string line;
		unsigned int i;
		bool skipSpace = true;
		char strident = '\0';
		while ( getline(myfile,line) )
		{
			if ( (line[0] == '#') || (line[0] == ';') )
				continue;
			for (i = 0; i < line.length(); i++)
			{
				if (line[i] == 0x0d)
					continue;
				if ( skipSpace && (line[i] == ' ') )
					continue;
				if (line[i] == '=')
				{
					isKey = false;
					continue;
				}
				if (isKey)
					key << line[i];
				else
				{
					if ( (line[i] == '\'') || (line[i] == '"') )
					{
						if (strident == '\0')
						{
							strident = line[i];
							skipSpace = false;
							continue;
						}
						if (strident == line[i])
						{
							strident = '\0';
							skipSpace = true;
							continue;
						}
					}
					val << line[i];
				}
			}
			if ( ! isKey )
			{
				string skey = key.str();
				evoconfig[skey] = val.str();
				isKey = true;
				key.str("");
				val.str("");
			}
		}
		myfile.close();
		return true;
	}
	return false;
}


std::string getpath(std::string filename)
{
#ifdef _WIN32
	stringstream ss;
	unsigned int i;
	for (i = 0; i < filename.length(); i++)
	{
		if (filename[i] == '\\')
			ss << '/';
		else
			ss << filename[i];
	}
	filename = ss.str();
#endif
	std::size_t pos = filename.rfind('/');
	if (pos == std::string::npos)
		return "";
	return filename.substr(0, pos+1);
}


/*
 * Convert domoticz host settings into fully qualified url prefix
 */
std::string get_domoticz_url(std::string host, std::string port)
{
	stringstream ss;
	if (host.substr(0,4) != "http")
		ss << "http://";
	ss << host;
	if (port.length() > 0)
		ss << ":" << port;
	return ss.str();
}


std::string int_to_string(int myint)
{
	stringstream ss;
	ss << myint;
	return ss.str();
}


/************************************************************************
 *									*
 *	Time conversion functions					*
 *									*
 ************************************************************************/

std::string local_to_utc(std::string local_time)
{
	if (local_time.size() <  19)
		return "";
	time_t now = time(0);
	struct tm utime;
	gmtime_r(&now, &utime);
	utime.tm_isdst = -1;
	int tzoffset = (int)difftime(mktime(&utime), now);
	struct tm ltime;
	ltime.tm_year = atoi(local_time.substr(0, 4).c_str()) - 1900;
	ltime.tm_mon = atoi(local_time.substr(5, 2).c_str()) - 1;
	ltime.tm_mday = atoi(local_time.substr(8, 2).c_str());
	ltime.tm_hour = atoi(local_time.substr(11, 2).c_str());
	ltime.tm_min = atoi(local_time.substr(14, 2).c_str());
	ltime.tm_sec = atoi(local_time.substr(17, 2).c_str()) + tzoffset;
	// FIXME: Should I consider the possibility that someone might want to use this during a DST switch? 
	ltime.tm_isdst = -1;
	time_t ntime = mktime(&ltime);
	if (ntime == -1)
		throw std::invalid_argument("bad timestamp value on command line");
	char c_until[40];
	sprintf_s(c_until,40,"%04d-%02d-%02dT%02d:%02d:%02dZ",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,ltime.tm_hour,ltime.tm_min,ltime.tm_sec);
	return string(c_until);
}


std::string utc_to_local(std::string utc_time)
{
	if (utc_time.size() <  19)
		return "";
	time_t now = time(0);
	struct tm utime;
	gmtime_r(&now, &utime);
	utime.tm_isdst = -1;
	int tzoffset = (int)difftime(mktime(&utime), now);
	struct tm ltime;
	ltime.tm_year = atoi(utc_time.substr(0, 4).c_str()) - 1900;
	ltime.tm_mon = atoi(utc_time.substr(5, 2).c_str()) - 1;
	ltime.tm_mday = atoi(utc_time.substr(8, 2).c_str());
	ltime.tm_hour = atoi(utc_time.substr(11, 2).c_str());
	ltime.tm_min = atoi(utc_time.substr(14, 2).c_str());
	ltime.tm_sec = atoi(utc_time.substr(17, 2).c_str()) - tzoffset;
	// FIXME: Should I consider the possibility that someone might want to use this during a DST switch? 
	ltime.tm_isdst = -1;
	time_t ntime = mktime(&ltime);
	if (ntime == -1)
		throw std::invalid_argument("bad timestamp value on command line");
	char c_until[40];
	sprintf_s(c_until,40,"%04d-%02d-%02d %02d:%02d:%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,ltime.tm_hour,ltime.tm_min,ltime.tm_sec);
	return string(c_until);
}


std::string format_time(std::string local_time)
{
	struct tm ltime;
	if (local_time[0] == '+')
	{
		int minutes = atoi(local_time.substr(1).c_str());
		time_t now = time(0);
		localtime_r(&now, &ltime);
		ltime.tm_min += minutes;
		
	}
	else if (local_time.length() < 19)
		throw std::invalid_argument("bad timestamp value on command line");
	else
	{
		ltime.tm_year = atoi(local_time.substr(0, 4).c_str()) - 1900;
		ltime.tm_mon = atoi(local_time.substr(5, 2).c_str()) - 1;
		ltime.tm_mday = atoi(local_time.substr(8, 2).c_str());
		ltime.tm_hour = atoi(local_time.substr(11, 2).c_str());
		ltime.tm_min = atoi(local_time.substr(14, 2).c_str());
		ltime.tm_sec = atoi(local_time.substr(17, 2).c_str());
	}
	// FIXME: Should I consider the possibility that someone might want to use this during a DST switch? 
	ltime.tm_isdst = -1;
	time_t ntime = mktime(&ltime);
	if (ntime == -1)
		throw std::invalid_argument("bad timestamp value on command line");
	char c_until[40];
	sprintf_s(c_until,40,"%04d-%02d-%02dT%02d:%02d:%02d",ltime.tm_year+1900,ltime.tm_mon+1,ltime.tm_mday,ltime.tm_hour,ltime.tm_min,ltime.tm_sec);
	return string(c_until);
}



/************************************************************************
 *									*
 *	Common functions						*
 *									*
 ************************************************************************/

void scan_for_evohome_hardware(DomoticzClient &dclient)
{
	std::vector<std::string>().swap(myinstallations);
	dclient.scan_hardware();
	if (dclient.installations.size() == 0)
		throw std::invalid_argument("No active Evohome installation found");

	for (std::vector<DomoticzClient::hardware>::size_type i = 0; i < dclient.installations.size(); ++i)
		myinstallations.push_back(dclient.get_hwid_by_name(dclient.installations[i].HardwareName));
}



/************************************************************************
 *									*
 *	Output functions - query information				*
 *									*
 ************************************************************************/

void query_evohome_hardware(DomoticzClient &dclient)
{
	dclient.scan_hardware();
	if (dclient.installations.size() == 0)
		throw std::invalid_argument("No active Evohome installation found");

	cout << "ID\tName\t\t\tHardware Type" << endl;
	for (std::vector<DomoticzClient::hardware>::size_type i = 0; i < dclient.installations.size(); ++i)
	{
		cout << dclient.get_hwid_by_name(dclient.installations[i].HardwareName) << "\t"
		     << dclient.installations[i].HardwareName << "\t";
		if (dclient.installations[i].HardwareName.length() < 16)
			cout << "\t";
		if (dclient.installations[i].HardwareName.length() < 8)
			cout << "\t";
		cout << dclient.installations[i].HardwareType << endl;
	}
}


void query_evohome_devices(DomoticzClient &dclient, const std::string hwname="")
{
	if (hwname.empty())
		scan_for_evohome_hardware(dclient);
	else
	{
		std::vector<std::string>().swap(myinstallations);
		std::string hwid = dclient.get_hwid_by_name(hwname);
		if (hwid == "-1")
			throw std::invalid_argument(std::string("Evohome installation with name '")+hwname+"' does not exist");
		myinstallations.push_back(hwid);
	}

	cout << "ID\tName\t\t\tDevice Type" << endl;
	dclient.devices.clear();
	for (std::vector<std::string>::size_type i = 0; i < myinstallations.size(); ++i)
		dclient.get_devices(myinstallations[i],1);

	std::map<std::string,DomoticzClient::device>::iterator it;
	for ( it = dclient.devices.begin(); it != dclient.devices.end(); it++ )
	{
		cout << it->second.idx << "\t" << it->second.Name << "\t";
		if (it->second.Name.length() < 16)
			cout << "\t";
		if (it->second.Name.length() < 8)
			cout << "\t";
		cout << it->second.SubType << endl;
	}
}


void query_evohome_zone_status(DomoticzClient &dclient, const std::string zonename)
{
	if (myinstallations.size() < 1)
		scan_for_evohome_hardware(dclient);
	dclient.devices.clear();
	for (std::vector<std::string>::size_type i = 0; i < myinstallations.size(); ++i)
		dclient.get_devices(myinstallations[i],1);


	std::string idx = dclient.get_device_idx_by_name(zonename);
	if ((idx == "-1") || (dclient.devices[idx].SubType != "Zone"))
		throw std::invalid_argument(std::string("Evohome zone with name '")+zonename+"' does not exist");

	cout << "ID\tTemp\tSetPoint\tStatus\t\t\tUntil" << endl;
	cout << idx << "\t" << dclient.devices[idx].Temp << "\t" << dclient.devices[idx].SetPoint << "\t\t"
	     << dclient.devices[idx].Status << "\t\t";
		if (dclient.devices[idx].Status.length() < 8)
			cout << "\t";
	cout << utc_to_local(dclient.devices[idx].Until) << endl;
}


void query_evohome_system_mode(DomoticzClient &dclient, const std::string hardwarename="")
{
	if (myinstallations.size() < 1)
		scan_for_evohome_hardware(dclient);
	dclient.devices.clear();
	for (std::vector<std::string>::size_type i = 0; i < myinstallations.size(); ++i)
		dclient.get_devices(myinstallations[i],1);

	std::string idx = "";
	std::map<std::string,DomoticzClient::device>::iterator it;
	for ( it = dclient.devices.begin(); it != dclient.devices.end(); it++ )
	{
		if (it->second.SubType == "Evohome")
		{
			if ((hardwarename != "") && (it->second.HardwareName != hardwarename))
				continue;

			if (idx.empty())
				cout << "ID\tHardware Name\t\tStatus" << endl;

			idx = it->second.idx;
			cout << idx << "\t" << it->second.HardwareName << "\t";
			if (it->second.HardwareName.length() < 16)
				cout << "\t";
			if (it->second.HardwareName.length() < 8)
				cout << "\t";
			cout << it->second.Status << endl;
		}
	}
	if (idx.empty())
	{
		if (hardwarename.empty())
			throw std::invalid_argument("Evohome system could not be found");
		else
			throw std::invalid_argument(std::string("Evohome system with hardware name '")+hardwarename+"' does not exist");
	}
}


int query_main(int argc, char** argv, DomoticzClient &dclient)
{
	if (argc <= 2)
		usage("query");

	std::string what;

	what = "hardware";
	if (what.find(argv[2]) == 0)
	{
		query_evohome_hardware(dclient);
		return 0;
	}

	if (!evoconfig["hwtype"].empty())
		dclient.get_hardware_types(evoconfig["hwtype"]);

	what = "devices";
	if (what.find(argv[2]) == 0)
	{
		std::string hwname = (argc >= 4) ? argv[3] : evoconfig["hwname"];
		query_evohome_devices(dclient, hwname);
		return 0;
	}

	what = "zone";
	if (what.find(argv[2]) == 0)
	{
		if (argc < 4)
			usage("query");

		query_evohome_zone_status(dclient, argv[3]);
		return 0;
	}

	what = "system";
	if (what.find(argv[2]) == 0)
	{
		std::string hwname = (argc >= 4) ? argv[3] : evoconfig["hwname"];
		query_evohome_system_mode(dclient, hwname);
		return 0;
	}

	usage("query");
	return 0;
}



/************************************************************************
 *									*
 *	Command functions - change Evohome settings			*
 *									*
 ************************************************************************/

void cmd_set_temperature(DomoticzClient &dclient, const std::string zonename, const std::string setpoint, const std::string until="")
{
	std::string idx = "-1";
	std::string s_setpoint;

	if (setpoint == "auto")
	{
		if (idx == "-1")
			idx = dclient.get_device_idx_by_name(zonename);
		if ((dclient.devices[idx].Status != "Auto") && (dclient.devices[idx].Status != "OpenWindow"))
			dclient.cancel_temperature_override(zonename);
		else
			cerr << "WARNING: zone is already set to follow schedule" << endl;
		return;
	}

	if ((setpoint.length() == 2) && ((setpoint[0] == '-') || (setpoint[0] == '+')))
	{
		// adjust current setpoint
		if ((setpoint[1] == '1') || (setpoint[1] == '2') || (setpoint[1] == '3'))
		{
			if (idx == "-1")
				idx = dclient.get_device_idx_by_name(zonename);
			stringstream ss;
			ss << dclient.devices[idx].SetPoint;
			float f_setpoint;
			ss >> f_setpoint;
			char adj = setpoint[1] - 0x30;
			stringstream ss2;
			if (setpoint[0] == '-')
			{
				float f_setpoint2 = f_setpoint - adj;
				if (f_setpoint2 < 5)
					f_setpoint2 = 5;
				ss2 << f_setpoint2;
			}
			else
			{
				float f_setpoint2 = f_setpoint + adj;
				if (f_setpoint2 > 35)
					f_setpoint2 = 35;
				ss2 << f_setpoint2;
			}
			s_setpoint = ss2.str();
		}
		else
			throw std::invalid_argument("adjust setpoint is limited to 3⁰C");


	}
	else
	{
		// sanity checks on setpoint input
		if (setpoint.find_first_not_of("0123456789.,") != string::npos)
			throw std::invalid_argument("setpoint is not a number");
		if ((setpoint.find_first_of(".") != string::npos) && (setpoint.find_first_of(",") != string::npos))
			throw std::invalid_argument("can't have thousands separator in setpoint");
		size_t founddecimal = 1024;
		stringstream ss;
		for (size_t i = 0; i < setpoint.length(); i++)
		{
			if ((setpoint[i] == ',') || (setpoint[i] == '.'))
			{
				if (founddecimal != 1024)
					throw std::invalid_argument("can't have multiple decimal signs in setpoint");
				ss << '.';
				founddecimal = i;
			}
			else
				ss << setpoint[i];
		}
		if (founddecimal == 0)
			throw std::invalid_argument("setpoint cannot start with a decimal sign");
		float f_setpoint, f_setpoint2;
		ss >> f_setpoint;
		stringstream ss2;
		ss2 << f_setpoint * 2 + 0.5;
		s_setpoint = ss2.str();
		stringstream ss3;
		for (size_t i = 0; ((i < s_setpoint.length()) && (s_setpoint[i] != '.')); i++)
			ss3 << s_setpoint[i];
		ss3 >> f_setpoint2;
		if (f_setpoint2 < 10)
			throw std::invalid_argument("setpoint cannot be below 5⁰C");
		if (f_setpoint2 > 70)
			throw std::invalid_argument("setpoint cannot be above 35⁰C");
		stringstream ss4;
		ss4 << f_setpoint2 / 2;
		s_setpoint = ss4.str();
		if (f_setpoint != (f_setpoint2 / 2))
			cerr << "WARNING: setpoint " << setpoint << " was rounded to its nearest half degree value (" << s_setpoint << ")" << endl;
	}

	std::string s_until;
	if (until[0] == '+')
	{
		if (until.substr(1).find_first_not_of("0123456789") != string::npos)
			throw std::invalid_argument("for <minutes> must be an integer value");
		s_until = local_to_utc(format_time(until));
	}
	else if ((until.length() > 19) && (until[19] == 'Z'))
		s_until = until;
	else if (until.length() >= 19)
		s_until = local_to_utc(until);
	else
	{
		if (!until.empty())
		{
			std::string try_until;
			if (until[0] == 'a')
				try_until = "auto";
			else if (until[0] == 'p')
				try_until = "permanent";
			else if (until[0] == 'k')
				try_until = "keep";
			if (try_until.find(until) != 0)
				usage("settemp");
		}

		if ((!until.empty()) && (until[0] == 'p'))
			s_until = "";
		else
		{
			if (idx == "-1")
				idx = dclient.get_device_idx_by_name(zonename);

			if (until[0] == 'a')
			{
				if ((dclient.devices[idx].Status == "Auto") || (dclient.devices[idx].Status == "OpenWindow"))
					s_until = dclient.devices[idx].Until;
				else
					throw std::invalid_argument("Can't select next schedule switch point when zone is in override mode");
			}
			else
				s_until = dclient.devices[idx].Until;
			if (s_until.empty())
			{
				if (until.empty())
					cerr << "WARNING: Domoticz does not supply schedule information - override will be permanent" << endl;
				else
					throw std::invalid_argument("Can't select next schedule switch point as Domoticz does not supply that information");
			}
		}
	}

	dclient.set_temperature(zonename, s_setpoint, s_until);
}


void cmd_set_evohome_system_mode(DomoticzClient &dclient, const std::string systemmode, const std::string hardwarename="")
{
	// convert systemmode to lowercase
	stringstream ss;
	for (size_t i = 0; i < systemmode.length(); i++)
		ss << (char)(systemmode[i] | 0x20);
	std::string lo_systemmode = ss.str();
	uint8_t nZoneMode = 0;
	while ((lo_systemmode != my_evo_modes[nZoneMode].substr(0,lo_systemmode.length())) && (nZoneMode < 7))
		nZoneMode++;
	if (nZoneMode >= 7)
		usage("setsystem");

	if (!hardwarename.empty())
		dclient.set_hardware_by_name(hardwarename);

	dclient.set_system_mode(dclient.evo_modes[nZoneMode]);

}


void cmd_set_DHW_state(DomoticzClient &dclient, const std::string state, const std::string until, const std::string hardwarename="")
{
	std::string idx = "-1";

	// convert systemmode to lowercase
	stringstream ss;
	for (size_t i = 0; i < state.length(); i++)
		ss << (char)(state[i] | 0x20);
	std::string lo_state = ss.str();
	if ((lo_state != "auto") && (lo_state != "on") && (lo_state != "off"))
		usage("setDHW");

	std::string s_until;
	if (until[0] == '+')
	{
		if (until.substr(1).find_first_not_of("0123456789") != string::npos)
			throw std::invalid_argument("for <minutes> must be an integer value");
		s_until = local_to_utc(format_time(until));
	}
	else if ((until.length() > 19) && (until[19] == 'Z'))
		s_until = until;
	else if (until.length() >= 19)
		s_until = local_to_utc(until);
	else
	{
		if (!until.empty())
		{
			std::string try_until;
			if (until[0] == 'a')
				try_until = "auto";
			else if (until[0] == 'p')
				try_until = "permanent";
			else if (until[0] == 'k')
				try_until = "keep";
			if (try_until.find(until) != 0)
				usage("settemp");
		}

		if ((!until.empty()) && (until[0] == 'p'))
			s_until = "";
		else
		{
			if (idx == "-1")
				idx = dclient.get_DHW(hardwarename);

			if (until[0] == 'a')
			{
				if (dclient.devices[idx].Status == "Auto")
					s_until = dclient.devices[idx].Until;
				else
					throw std::invalid_argument("Can't select next schedule switch point when device is in override mode");
			}
			else
				s_until = dclient.devices[idx].Until;
			if (s_until.empty())
			{
				if (until.empty())
					cerr << "WARNING: Domoticz does not supply schedule information - override will be permanent" << endl;
				else
					throw std::invalid_argument("Can't select next schedule switch point as Domoticz does not supply that information");
			}
		}
	}
}


int cmd_set_main(int argc, char** argv, DomoticzClient &dclient)
{
	if (argc <= 2)
		usage("set");

	std::string what;

	what = "temperature";
	if (what.find(argv[2]) == 0)		// set temperature <zone> <setpoint> [until <time> | for <nn> [minutes] | <time> | +<nn> [minutes]]
	{
		if (argc < 5)
			usage("settemp");
		std::string until;
		if (argc < 6)
			until = "";
		else if (argc == 6)
			until = argv[5];
		else // if (argc > 6)
		{
			until = argv[5];
			if (until[0] == 'u')
				what = "until";
			else if (until[0] == 'f')
				what = "for";
			else
				what = "";
			if (what.find(argv[5]) == 0)
				until = argv[6];
			if ((what == "for") && (until[0] != '+'))
			{
				stringstream ss;
				ss << '+' << until;
				until = ss.str();
			}
		}
		cmd_set_temperature(dclient, argv[3], argv[4], until);
		return 0;
	}

	what = "system";
	if (what.find(argv[2]) == 0)
	{
		if (argc < 4)
			usage("setsystem");
		if (argc >= 5)	
			cmd_set_evohome_system_mode(dclient, argv[4], argv[3]);
		else
			cmd_set_evohome_system_mode(dclient, argv[3], "");
		return 0;
	}

	usage("set");
	return 0;
}


/************************************************************************
 *									*
 *	Program entry point						*
 *									*
 ************************************************************************/

int main(int argc, char** argv)
{

	std::string menu;
	menu = "help";
	if ((argc <= 1) || (menu.find(argv[1]) == 0))
		usage("general");

	// set defaults
	configfile = CONFIGFILE;

	if ( ( ! read_evoconfig(configfile) ) && (getpath(configfile) == "") )
	{
		// try to find evoconfig in the application path
		stringstream ss;
		ss << getpath(argv[0]) << configfile;
		if ( ! read_evoconfig(ss.str()) )
			exit_error("ERROR: can't read config file");
	}

	std::string url;
	if (evoconfig["url"].empty())
		url = get_domoticz_url(evoconfig["host"], evoconfig["port"]);
	else
		url = evoconfig["url"];

	// initialise connection to Domoticz server
	DomoticzClient dclient = DomoticzClient(url);

	try
	{
		menu = "query";
		if (menu.find(argv[1]) == 0)
		{
			return query_main(argc, argv, dclient);
		}

		if (!evoconfig["hwtype"].empty())
			dclient.get_hardware_types(evoconfig["hwtype"]);

		if (!evoconfig["hwname"].empty())
			dclient.set_hardware_by_name(evoconfig["hwname"]);

		menu = "set";
		if (menu.find(argv[1]) == 0)
			return cmd_set_main(argc, argv, dclient);


	}
	catch (exception& e)
	{
		dclient.cleanup();
		cerr << "ERROR: " << e.what() << endl;
		return 1;
	}

	// cleanup
	dclient.cleanup();

	usage("general");
	return 0;
}

