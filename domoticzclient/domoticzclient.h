/*
 * Copyright (c) 2016 Gordon Bos <gordon@bosvangennip.nl> All rights reserved.
 *
 * Json client for Domoticz
 *
 *
 * Source code subject to GNU GENERAL PUBLIC LICENSE version 3
 */

#ifndef _DomoticzClient
#define _DomoticzClient

#include <map>
#include <vector>
#include <string>

class DomoticzClient
{
	private:
	std::string domoticzhost;
	std::vector<std::string> domoticzheader;

	void init();
	std::string send_receive_data(std::string url);
	std::string send_receive_data(std::string url, std::string postdata);


	public:
	static const std::string evo_modes[7];

	struct device
	{
		std::string idx;
		std::string Name;
		std::string SubType;
		std::string Temp;
		std::string SetPoint;
		std::string Status;
		std::string Until;
		std::string HardwareName;
		std::string Description;
	};

	struct hardware
	{
		std::string idx;
		std::string HardwareName;
		std::string HardwareType;
		std::string HardwareTypeVal;
	};

	std::map<std::string,std::string> hwtypes;
	std::map<std::string,device> devices;
	std::vector<hardware> installations;

	DomoticzClient(std::string host);
	~DomoticzClient();
	void cleanup();


	std::string get_hwid_by_name(const std::string hardwarename);
	bool get_devices();
	bool get_devices(const std::string hwid);
	bool get_devices(const std::string hwid, bool concatenate);

	std::string get_controller();
	std::string get_controller(const std::string hardwarename);

	std::string get_DHW();
	std::string get_DHW(const std::string hardwarename);

	std::string get_device_idx_by_name(const std::string devicename);
	void get_hardware_types(const std::string needle);
	void scan_hardware();
	void set_hardware_by_name(const std::string hardwarename);

	void set_temperature(const std::string zonename,const std::string setpoint,const std::string until);
	void cancel_temperature_override(const std::string zonename);
	void set_system_mode(const std::string mode);
	void set_system_mode(const std::string mode, const std::string hardwarename);
	void set_DHW_state(const std::string state, const std::string until);
	void set_DHW_state(const std::string state, const std::string until, const std::string hardwarename);

};

#endif
