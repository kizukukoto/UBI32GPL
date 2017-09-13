/* xml format request and reponse for pure network */

char *RebootResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<RebootResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<RebootResult>REBOOT</RebootResult>"
"</RebootResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *IsDeviceReady_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<IsDeviceReadyResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<IsDeviceReadyResult>%s</IsDeviceReadyResult>"
"</IsDeviceReadyResponse>"
"</soap:Body>"
"</soap:Envelope>";

/* 
	  Modification: DeviceSetting testting for different Model
      Modified by: ken_chiang 
      Date:2007/9/21
*/
char *GetDeviceSettingsResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetDeviceSettingsResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetDeviceSettingsResult>OK</GetDeviceSettingsResult>"
"<Type>%s</Type>"
"<DeviceName>%s</DeviceName>"
"<VendorName>%s</VendorName>"
"<ModelDescription>%s</ModelDescription>"
"<ModelName>%s</ModelName>"
"<FirmwareVersion>%s</FirmwareVersion>"
"<PresentationURL>%s</PresentationURL>"
#ifdef AUTHGRAPH
"<CAPTCHA>%s</CAPTCHA>"
#endif
"<SOAPActions>"
"<string>http://purenetworks.com/HNAP1/GetDeviceSettings</string>"
"<string>http://purenetworks.com/HNAP1/SetDeviceSettings</string>"
"<string>http://purenetworks.com/HNAP1/SetWLanSettings24</string>"
"<string>http://purenetworks.com/HNAP1/GetWLanSettings24</string>"
"<string>http://purenetworks.com/HNAP1/SetWanSettings</string>"
"<string>http://purenetworks.com/HNAP1/GetWanSettings</string>"
"<string>http://purenetworks.com/HNAP1/GetWanStatus</string>"
"<string>http://purenetworks.com/HNAP1/SetWLanSecurity</string>"
"<string>http://purenetworks.com/HNAP1/GetWLanSecurity</string>"
"<string>http://purenetworks.com/HNAP1/IsDeviceReady</string>"
"<string>http://purenetworks.com/HNAP1/AddPortMapping</string>"
"<string>http://purenetworks.com/HNAP1/DeletePortMapping</string>"
"<string>http://purenetworks.com/HNAP1/GetPortMappings</string>"
"<string>http://purenetworks.com/HNAP1/SetMACFilters2</string>"
"<string>http://purenetworks.com/HNAP1/GetMACFilters2</string>"
"<string>http://purenetworks.com/HNAP1/GetConnectedDevices</string>"
"<string>http://purenetworks.com/HNAP1/GetNetworkStats</string>"
"<string>http://purenetworks.com/HNAP1/RenewWanConnection</string>"
"<string>http://purenetworks.com/HNAP1/Reboot</string>"
"<string>http://purenetworks.com/HNAP1/GetRouterLanSettings</string>"
"<string>http://purenetworks.com/HNAP1/SetRouterLanSettings</string>"
"<string>http://purenetworks.com/HNAP1/GetWLanRadios</string>"
"<string>http://purenetworks.com/HNAP1/GetWLanRadioSettings</string>"
"<string>http://purenetworks.com/HNAP1/SetWLanRadioSettings</string>"
"<string>http://purenetworks.com/HNAP1/GetWLanRadioSecurity</string>"
"<string>http://purenetworks.com/HNAP1/SetWLanRadioSecurity</string>"
"<string>http://purenetworks.com/HNAP1/StartCableModemCloneWanMac</string>"
"<string>http://purenetworks.com/HNAP1/GetCableModemCloneWanMac</string>"
"<string>http://purenetworks.com/HNAP1/SetCableModemCloneWanMac</string>"
"</SOAPActions>"
"<SubDeviceURLs></SubDeviceURLs>"
"<Tasks />"
"</GetDeviceSettingsResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *GetWanSettingsResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetWanSettingsResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetWanSettingsResult>%s</GetWanSettingsResult>"
"<Type>%s</Type>"
"<Username>%s</Username>"
"<Password>%s</Password>"
"<MaxIdleTime>%s</MaxIdleTime>"
"<ServiceName>%s</ServiceName>"
"<AutoReconnect>%s</AutoReconnect>"
"<IPAddress>%s</IPAddress>"
"<SubnetMask>%s</SubnetMask>"
"<Gateway>%s</Gateway>"
"<DNS>"
"<Primary>%s</Primary>"
"<Secondary>%s</Secondary>"
"</DNS>"
#ifdef OPENDNS
"<OpenDNS>"
"<enable>%s</enable>"
"</OpenDNS>"
#endif
"<MacAddress>%s</MacAddress>"
"<MTU>%s</MTU>"
"</GetWanSettingsResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *GetWanStatusResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetWanStatusResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetWanStatusResult>%s</GetWanStatusResult>"
"<Status>%s</Status>"
"</GetWanStatusResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *GetEthernetPortSettingResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetEthernetPortSettingResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetEthernetPortSettingResult>%s</GetEthernetPortSettingResult>"
"<EthernetPorts>"
"<EthernetPort>"
"<port>%s</port>"
"<connected>%s</connected>"
"<speed>%s</speed>"
"<duplex>%s</duplex>"	
"</EthernetPort>"
"<EthernetPort>"
"<port>%s</port>"
"<connected>%s</connected>"
"<speed>%s</speed>"
"<duplex>%s</duplex>"	
"</EthernetPort>"
"<EthernetPort>"
"<port>%s</port>"
"<connected>%s</connected>"
"<speed>%s</speed>"
"<duplex>%s</duplex>"	
"</EthernetPort>"
"<EthernetPort>"
"<port>%s</port>"
"<connected>%s</connected>"
"<speed>%s</speed>"
"<duplex>%s</duplex>"	
"</EthernetPort>"
"<EthernetPort>"
"<port>%s</port>"
"<connected>%s</connected>"
"<speed>%s</speed>"
"<duplex>%s</duplex>"	
"</EthernetPort>"
"</EthernetPorts>"
"</GetEthernetPortSettingResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *GetWLanSecurityResult_xml= 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetWLanSecurityResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetWLanSecurityResult>%s</GetWLanSecurityResult>"
"<Enabled>%s</Enabled>"
"<Type>%s</Type>"
"<WEPKeyBits>%s</WEPKeyBits>"
"<SupportedWEPKeyBits>"
"<int>64</int>"
"<int>128</int>"
"</SupportedWEPKeyBits>"
"<Key>%s</Key>"
"<RadiusIP1>%s</RadiusIP1>"
"<RadiusPort1>%s</RadiusPort1>"
"<RadiusIP2>%s</RadiusIP2>"
"<RadiusPort2>%s</RadiusPort2>"
"</GetWLanSecurityResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *GetWLanSettings24Result_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetWLanSettings24Response xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetWLanSettings24Result>%s</GetWLanSettings24Result>"
"<Enabled>%s</Enabled>"
"<MacAddress>%s</MacAddress>"
"<SSID>%s</SSID>"
"<SSIDBroadcast>%s</SSIDBroadcast>"
"<Channel>%s</Channel>"
"</GetWLanSettings24Response>"
"</soap:Body>"
"</soap:Envelope>";	

//NickChou add for QRS
char *SecurityInfo =
"<SecurityInfo>"
"<SecurityType>WEP-OPEN</SecurityType>"
"<Encryptions>"
"<string>WEP-64</string>"
"<string>WEP-128</string>"
"</Encryptions>"
"</SecurityInfo>"
"<SecurityInfo>"
"<SecurityType>WEP-SHARED</SecurityType>"
"<Encryptions>"
"<string>WEP-64</string>"
"<string>WEP-128</string>"
"</Encryptions>"
"</SecurityInfo>"
"<SecurityInfo>"
"<SecurityType>WPA-PSK</SecurityType>"
"<Encryptions>"
"<string>TKIP</string>"
"<string>AES</string>"
"<string>TKIPORAES</string>"
"</Encryptions>"
"</SecurityInfo>"
"<SecurityInfo>"
"<SecurityType>WPA-RADIUS</SecurityType>"
"<Encryptions>"
"<string>TKIP</string>"
"<string>AES</string>"
"<string>TKIPORAES</string>"
"</Encryptions>"
"</SecurityInfo>"
"<SecurityInfo>"
"<SecurityType>WPA2-PSK</SecurityType>"
"<Encryptions>"
"<string>TKIP</string>"
"<string>AES</string>"
"<string>TKIPORAES</string>"
"</Encryptions>"
"</SecurityInfo>"
"<SecurityInfo>"
"<SecurityType>WPA2-RADIUS</SecurityType>"
"<Encryptions>"
"<string>TKIP</string>"
"<string>AES</string>"
"<string>TKIPORAES</string>"
"</Encryptions>"
"</SecurityInfo>"
"<SecurityInfo>"
"<SecurityType>WPAORWPA2-PSK</SecurityType>"
"<Encryptions>"
"<string>TKIP</string>"
"<string>AES</string>"
"<string>TKIPORAES</string>"
"</Encryptions>"
"</SecurityInfo>"
"<SecurityInfo>"
"<SecurityType>WPAORWPA2-RADIUS</SecurityType>"
"<Encryptions>"
"<string>TKIP</string>"
"<string>AES</string>"
"<string>TKIPORAES</string>"
"</Encryptions>"
"</SecurityInfo>";

char *WideChannels_5GHZ = 
"<WideChannel><Channel>0</Channel><SecondaryChannels><int>0</int></SecondaryChannels></WideChannel>"
"<WideChannel><Channel>44</Channel><SecondaryChannels><int>36</int><int>149</int></SecondaryChannels></WideChannel>"
"<WideChannel><Channel>48</Channel><SecondaryChannels><int>40</int><int>153</int></SecondaryChannels></WideChannel>"
"<WideChannel><Channel>149</Channel><SecondaryChannels><int>44</int><int>157</int></SecondaryChannels></WideChannel>"
"<WideChannel><Channel>153</Channel><SecondaryChannels><int>48</int><int>161</int></SecondaryChannels></WideChannel>"
"<WideChannel><Channel>157</Channel><SecondaryChannels><int>149</int><int>165</int></SecondaryChannels></WideChannel>";

char *RadioInfo_24GHZ =
"<RadioInfo>"
"<RadioID>RADIO_24GHz</RadioID>"
"<Frequency>2</Frequency>"
"<SupportedModes>"
"<string>802.11b</string>"
"<string>802.11g</string>"
"<string>802.11n</string>"
"<string>802.11bg</string>"
"<string>802.11gn</string>"
"<string>802.11bgn</string>"
"</SupportedModes>"
"<Channels>%s</Channels>"
"<WideChannels>%s</WideChannels>" 
"<SupportedSecurity>%s</SupportedSecurity>"
"</RadioInfo>";

char *RadioInfo_5GHZ =
"<RadioInfo>"
"<RadioID>RADIO_5GHz</RadioID>"
"<Frequency>5</Frequency>"
"<SupportedModes>"
"<string>802.11a</string>"
"<string>802.11n</string>"
"<string>802.11an</string>"
"</SupportedModes>"
"<Channels>%s</Channels>"
"<WideChannels>%s</WideChannels>"
"<SupportedSecurity>%s</SupportedSecurity>"
"</RadioInfo>";

char *GetWLanRadiosResult_xml =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" soap:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
"<soap:Body>"
"<GetWLanRadiosResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetWLanRadiosResult>%s</GetWLanRadiosResult>"
"<RadioInfos>"
"%s"
#ifdef USER_WL_ATH_5GHZ
"%s"
#endif
"</RadioInfos>"
"</GetWLanRadiosResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *GetWLanRadioSettingsResult_xml = 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" soap:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
"<soap:Body>"
"<GetWLanRadioSettingsResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetWLanRadioSettingsResult>%s</GetWLanRadioSettingsResult>"
"<Enabled>%s</Enabled>"
"<Mode>%s</Mode>"
"<MacAddress>%s</MacAddress>"
"<SSID>%s</SSID>"
"<SSIDBroadcast>%s</SSIDBroadcast>"
"<ChannelWidth>%s</ChannelWidth>"
"<Channel>%s</Channel>"
"<SecondaryChannel>%s</SecondaryChannel>"
"<QoS>%s</QoS>"
"</GetWLanRadioSettingsResponse>"
"</soap:Body>"
"</soap:Envelope>"
;

char *GetWLanRadioSecurityResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" soap:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
"<soap:Body>"
"<GetWLanRadioSecurityResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetWLanRadioSecurityResult>%s</GetWLanRadioSecurityResult>"
"<Enabled>%s</Enabled>"
"<Type>%s</Type>"
"<Encryption>%s</Encryption>"
"<Key>%s</Key>"
"<KeyRenewal>%s</KeyRenewal>"
"<RadiusIP1>%s</RadiusIP1>"
"<RadiusPort1>%s</RadiusPort1>"
"<RadiusSecret1>%s</RadiusSecret1>"
"<RadiusIP2>%s</RadiusIP2>"
"<RadiusPort2>%s</RadiusPort2>"
"<RadiusSecret2>%s</RadiusSecret2>"
"</GetWLanRadioSecurityResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *SetWLanRadioSettingsResult_xml =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<SetWLanRadioSettingsResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<SetWLanRadioSettingsResult>%s</SetWLanRadioSettingsResult>"
"</SetWLanRadioSettingsResponse>"
"</soap:Body>"
"</soap:Envelope>";
 
char *SetWLanRadioSecurityResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"	
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"	
"<soap:Body>"  	
"<SetWLanRadioSecurityResponse xmlns=\"http://purenetworks.com/HNAP1/\">"  	 	
"<SetWLanRadioSecurityResult>%s</SetWLanRadioSecurityResult>" 	  
"</SetWLanRadioSecurityResponse>" 	
"</soap:Body>"  	
"</soap:Envelope>";  	
//NickChou add for QRS end

char *GetMACFilters2Result_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetMACFilters2Response xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetMACFilters2Result>%s</GetMACFilters2Result>"
"<Enabled>%s</Enabled>"
"<IsAllowList>%s</IsAllowList>"
"<MACList>%s</MACList>"
"</GetMACFilters2Response>"
"</soap:Body>"
"</soap:Envelope>";

char *GetPortMappingsResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetPortMappingsResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetPortMappingsResult>%s</GetPortMappingsResult>"
"<PortMappings>"
"%s"
"</PortMappings>"
"</GetPortMappingsResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *GetRouterLanSettingsResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetRouterLanSettingsResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetRouterLanSettingsResult>OK</GetRouterLanSettingsResult>"
"<RouterIPAddress>%s</RouterIPAddress>"
"<RouterSubnetMask>%s</RouterSubnetMask>"
"<DHCPServerEnabled>%s</DHCPServerEnabled>"
"</GetRouterLanSettingsResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *GetConnectedDevicesResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetConnectedDevicesResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetConnectedDevicesResult>%s</GetConnectedDevicesResult>"
"<ConnectedClients>"
"%s"
"</ConnectedClients>"
"</GetConnectedDevicesResponse>"
"</soap:Body>"
"</soap:Envelope>";
  	
char *GetNetworkStatsResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetNetworkStatsResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetNetworkStatsResult>OK</GetNetworkStatsResult>"
"<Stats>"
"<NetworkStats>"
"<PortName>LAN</PortName>"
"<PacketsReceived>%s</PacketsReceived>"
"<PacketsSent>%s</PacketsSent>"
"<BytesReceived>%s</BytesReceived>"
"<BytesSent>%s</BytesSent>"
"</NetworkStats>"
"<NetworkStats>"
"<PortName>WLAN 802.11g</PortName>"
"<PacketsReceived>%s</PacketsReceived>"
"<PacketsSent>%s</PacketsSent>"
"<BytesReceived>%s</BytesReceived>"
"<BytesSent>%s</BytesSent>"
"</NetworkStats>"
"</Stats>"
"</GetNetworkStatsResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *SetDeviceSettingsResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"	
"<soap:Body>"
"<SetDeviceSettingsResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<SetDeviceSettingsResult>%s</SetDeviceSettingsResult>" 
"</SetDeviceSettingsResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *SetWLanSettings24Result_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<SetWLanSettings24Response xmlns=\"http://purenetworks.com/HNAP1/\">"
"<SetWLanSettings24Result>%s</SetWLanSettings24Result>"
"</SetWLanSettings24Response>"
"</soap:Body>"
"</soap:Envelope>";

char *SetWanSettingsResult_xml = 				
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"		
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"		
"<soap:Body>"  	
"<SetWanSettingsResponse xmlns=\"http://purenetworks.com/HNAP1/\">"  	 
"<SetWanSettingsResult>%s</SetWanSettingsResult>"  			
"</SetWanSettingsResponse>"  	
"</soap:Body>"  	
"</soap:Envelope>";

char *SetMACFilters2Result_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"		
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"		
"<soap:Body>" 
"<SetMACFilters2Response xmlns=\"http://purenetworks.com/HNAP1/\">"
"<SetMACFilters2Result>%s</SetMACFilters2Result>"
"</SetMACFilters2Response>"
"</soap:Body>"
"</soap:Envelope>";

char *SetWLanSecurityResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"	
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"	
"<soap:Body>"  	
"<SetWLanSecurityResponse xmlns=\"http://purenetworks.com/HNAP1/\">"  	 	
"<SetWLanSecurityResult>%s</SetWLanSecurityResult>" 	  
"</SetWLanSecurityResponse>" 	
"</soap:Body>"  	
"</soap:Envelope>";  	

char *SetRouterLanSettingsResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<SetRouterLanSettingsResponse xmlns=\"http://purenetworks.com/HNAP1/\">" 			
"<SetRouterLanSettingsResult>%s</SetRouterLanSettingsResult>"    		    
"</SetRouterLanSettingsResponse>"  
"</soap:Body>"  
"</soap:Envelope>";

char *AddPortMappingResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<AddPortMappingResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<AddPortMappingResult>%s</AddPortMappingResult>"
"</AddPortMappingResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *SetAccessPointModeResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"  
"<SetAccessPointModeResponse xmlns=\"http://purenetworks.com/HNAP1/\">"	
"<SetAccessPointModeResult>OK</SetAccessPointModeResult>"	  
"</SetAccessPointModeResponse>"  
"</soap:Body>"  
"</soap:Envelope>";

char *RenewWanConnectionResult_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"  
"<RenewWanConnectionResponse xmlns=\"http://purenetworks.com/HNAP1/\">"  
"<RenewWanConnectionResult>%s</RenewWanConnectionResult>" 	    
"</RenewWanConnectionResponse>"  
"</soap:Body>"  
"</soap:Envelope>";  

char *DeletePortMappingResult_xml =     
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<DeletePortMappingResponse xmlns=\"http://purenetworks.com/HNAP1/\">" 	
"<DeletePortMappingResult>%s</DeletePortMappingResult>"	
"</DeletePortMappingResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *MACInfo_xml = 
"<MACInfo>"
"<MacAddress>%s</MacAddress>"
"<DeviceName>%s</DeviceName>"
"</MACInfo>";

char *PortMapping_xml = 
"<PortMapping>"
"<PortMappingDescription>%s</PortMappingDescription>"
"<InternalClient>%s</InternalClient>"
"<PortMappingProtocol>%s</PortMappingProtocol>"
"<ExternalPort>%s</ExternalPort>"
"<InternalPort>%s</InternalPort>"
"</PortMapping>";

char *CntedClient_xml = 
"<ConnectedClient>"
"<ConnectTime>%s</ConnectTime>"
"<MacAddress>%s</MacAddress>"
"<DeviceName>%s</DeviceName>"
"<PortName>%s</PortName>"
"<Wireless>%s</Wireless>"
"<Active>%s</Active>"
"</ConnectedClient>";

char *UnkownCode_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>" 
"<soap:Fault>" 
"<faultcode>soap:Client</faultcode>"
"<faultstring>Error unknow</faultstring>"
"</soap:Fault>" 
"</soap:Body>"  
"</soap:Envelope>";  

char *StartCableModemCloneWanMac_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>" 
"<StartCableModemCloneWanMacResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<StartCableModemCloneWanMacResult>%s</StartCableModemCloneWanMacResult>"
"</StartCableModemCloneWanMacResponse>"
"</soap:Body>"  
"</soap:Envelope>";

char *GetCableModemCloneWanMac_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>" 
"<GetCableModemCloneWanMacResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetCableModemCloneWanMacResult>%s</GetCableModemCloneWanMacResult>"
"<CableModemCloneWanMacAddr>%s</CableModemCloneWanMacAddr>"
"</GetCableModemCloneWanMacResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *SetCableModemCloneWanMac_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<SetCableModemCloneWanMacResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
	"<SetCableModemCloneWanMacResult>%s</SetCableModemCloneWanMacResult>"
"</SetCableModemCloneWanMacResponse>"
"</soap:Body>"
"</soap:Envelope>";


#ifdef OPENDNS
char *GetOpenDNS_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<GetOpenDNSResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<GetOpenDNSResult>%s</GetOpenDNSResult>"
"<EnableOpenDNS>%s</EnableOpenDNS>"
"<OpenDNSMode>%s</OpenDNSMode>"
"<OpenDNSDeviceID>%s</OpenDNSDeviceID>"
"<OpenDNSDeviceKey>%s</OpenDNSDeviceKey>"
"</GetOpenDNSResponse>"
"</soap:Body>"
"</soap:Envelope>";

char *SetOpenDNS_xml = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
"<soap:Body>"
"<SetOpenDNSResponse xmlns=\"http://purenetworks.com/HNAP1/\">"
"<SetOpenDNSResult>%s</SetOpenDNSResult>"
"</SetOpenDNSResponse>"
"</soap:Body>"
"</soap:Envelope>";
#endif

char *SetMultipleActionResult_xml = 				
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"		
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"		
"<soap:Body>"  	
"<SetMultipleActionsResponse xmlns=\"http://purenetworks.com/HNAP1/\">"  	 
"<SetMultipleActionsResult>%s</SetMultipleActionsResult>"
	"<SetDeviceSettingsResult>%s</SetDeviceSettingsResult>"
	"<SetWanSettingsResult>%s</SetWanSettingsResult>"
	"<SetWLanRadioSettingsResult>%s</SetWLanRadioSettingsResult>"
	"<SetWLanRadioSecurityResult>%s</SetWLanRadioSecurityResult>"		
"</SetMultipleActionsResponse>"  	
"</soap:Body>"  	
"</soap:Envelope>";

char *SetTriggerADICResult_xml = 				
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"		
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"		
"<soap:Body>"  	
"<SetTriggerADICResponse xmlns=\"http://purenetworks.com/HNAP1/\">"  	 
"<SetTriggerADICResult>%s</SetTriggerADICResult>"	
"</SetTriggerADICResponse>"  	
"</soap:Body>"  	
"</soap:Envelope>";