#ifndef __XML_HEADER__
#define __XML_HEADER__
char header[]="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n" \
"<plist version=\"1.0\">\n" \
"<dict>\n\x09";

char tailer[]="</dict>\n" \
"</plist>\n";

char header1[]="<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" \
"<plist version=\"1.0\"> \
<dict> \
	<key>PairRecord</key> \
	<dict> \
		<key>DeviceCertificate</key> \
		<data>";


char header2[]="		</data>" \
"		<key>HostCertificate</key>" \
"		<data>";

char header3[]="		</data>" \
"		<key>HostID</key>" \
"		<string>";


char header4[]="</string>" \
"		<key>RootCertificate</key>" \
"		<data>";

char header5[]="		</data>" \
"	</dict>" \
"	<key>ProtocolVersion</key>" \
"	<string>2</string>" \
"	<key>Request</key>" \
"	<string>ValidatePair</string>" \
"</dict>" \
"</plist>";
#endif //__XML_HEADER__