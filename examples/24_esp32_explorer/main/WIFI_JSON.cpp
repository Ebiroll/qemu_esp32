#include <stdlib.h>
#include <JSON.h>
#include <WiFi.h>
#include <GeneralUtils.h>

JsonObject WIFI_JSON() {
	JsonObject obj = JSON::createObject();
	obj.setString("mode", WiFi::getMode());
	obj.setString("staMac", WiFi::getStaMac());
	obj.setString("apMac", WiFi::getApMac());
	obj.setString("staSSID", WiFi::getStaSSID());
	obj.setString("apSSID", WiFi::getApSSID());

	JsonObject apIpInfo = JSON::createObject();
	tcpip_adapter_ip_info_t ipInfo = WiFi::getApIpInfo();
	apIpInfo.setString("ip", GeneralUtils::ipToString((uint8_t *)&(ipInfo.ip)));
	apIpInfo.setString("gw", GeneralUtils::ipToString((uint8_t *)&(ipInfo.gw)));
	apIpInfo.setString("netmask", GeneralUtils::ipToString((uint8_t *)&(ipInfo.netmask)));
	obj.setObject("apIpInfo", apIpInfo);

	JsonObject staIpInfo = JSON::createObject();
	ipInfo = WiFi::getStaIpInfo();
	staIpInfo.setString("ip", GeneralUtils::ipToString((uint8_t *)&(ipInfo.ip)));
	staIpInfo.setString("gw", GeneralUtils::ipToString((uint8_t *)&(ipInfo.gw)));
	staIpInfo.setString("netmask", GeneralUtils::ipToString((uint8_t *)&(ipInfo.netmask)));
	obj.setObject("staIpInfo", staIpInfo);
	return obj;
} // WIFI_JSON
