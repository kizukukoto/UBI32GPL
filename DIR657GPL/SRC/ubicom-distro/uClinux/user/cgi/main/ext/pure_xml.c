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
"<PresentationURL>/index.html</PresentationURL>"
"<SOAPActions>"
"<string>http://purenetworks.com/HNAP1/GetDeviceSettings</string>"
"<string>http://purenetworks.com/HNAP1/SetDeviceSettings</string>"
"<string>http://purenetworks.com/HNAP1/SetWLanSettings24</string>"
"<string>http://purenetworks.com/HNAP1/GetWLanSettings24</string>"
"<string>http://purenetworks.com/HNAP1/SetWanSettings</string>"
"<string>http://purenetworks.com/HNAP1/GetWanSettings</string>"
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
"</SOAPActions>"
"<SubDeviceURLs></SubDeviceURLs>"
"<Tasks>"
"<TaskExtension>"
"<Name>Wireless settings</Name>"
"<URL>/cgi/ssi/wireless.asp</URL>"
"<Type>Browser</Type>"
"</TaskExtension>"
"<TaskExtension>"
"<Name>Block network access</Name>"
"<URL>/cgi/ssi/adv_filters_mac.asp</URL>"
"<Type>Browser</Type>"
"</TaskExtension>"
"<TaskExtension>"											
"<Name>Parental controls</Name>"
"<URL>/cgi/ssi/adv_filters_url.asp</URL>"    
"<Type>Browser</Type>"
"</TaskExtension>"
"<TaskExtension>"
"<Name>D-Link Tech Support</Name>"
"<URL>http://support.dlink.com</URL>"
"<Type>Browser</Type>"
"</TaskExtension>"
"<TaskExtension>"
"<Name>Reboot router</Name>"
"<URL>/restart.cgi</URL>"
"<Type>Silent</Type>"
"</TaskExtension>"
"</Tasks>"
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
"<MacAddress>%s</MacAddress>"
"<MTU>%s</MTU>"
"</GetWanSettingsResponse>"
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
"<int>152</int>"
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
"<GetRouterLanSettingsResult>%s</GetRouterLanSettingsResult>"
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
"<GetNetworkStatsResult>%s</GetNetworkStatsResult>"
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
"<SetDeviceSettingsResult>OK</SetDeviceSettingsResult>" 
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
