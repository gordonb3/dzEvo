/*
 * Copyright (c) 2023 Gordon Bos <gordon@bosvangennip.nl> All rights reserved.
 *
 * Json client for Domoticz API
 *
 *
 * Source code subject to GNU GENERAL PUBLIC LICENSE version 3
 */

#pragma once
#include <string>


namespace domoticz {

  namespace API {

    namespace hardware {
	static const std::string list = "type=hardware";
	static const std::string gettypes = "type=command&param=gethardwaretypes";
    }; // namespace hardware

    namespace devices {
	static const std::string list = "type=devices&displayhidden=1&used=all";
    }; // namespace devices

    namespace evohome {
	static const std::string setsysmode = "type=command&param=switchmodal";
	static const std::string settemp = "type=setused";
    }; // namespace evohome

  }; // namespace API

  namespace APIv2 {

    namespace hardware {
	static const std::string list = "type=command&gethardware";
	static const std::string gettypes = "type=command&param=gethardwaretypes";
    }; // namespace hardware

    namespace devices {
	static const std::string list = "type=command&getdevices&displayhidden=1&used=all";
    }; // namespace devices

    namespace evohome {
	static const std::string setsysmode = "type=command&param=switchmodal";
	static const std::string settemp = "type=command&setused";
    }; // namespace evohome

  }; // namespace APIv2

}; // namespace domoticz



