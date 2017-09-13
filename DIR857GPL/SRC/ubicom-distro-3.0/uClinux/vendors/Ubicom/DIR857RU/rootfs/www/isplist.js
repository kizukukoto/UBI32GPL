function display_isplist(){
var isp_temp = get_by_id("subISP");

if(get_by_id("country").value == "0" && get_by_id("usb3g_isp").value == "0"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"</select>";										
}
if(get_by_id("country").value == "999999" && get_by_id("usb3g_isp").value == "0"){
		isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+"<option value=\"0\" selected>-- None --</option>"
										+"</select>";										
}
if(get_by_id("country").value == "61" && get_by_id("usb3g_isp").value == "1"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:telstra.internet::\" selected>Telstra</option>"
										+"<option value=\"*99#:Internet::\">Optus</option>"
										+"<option value=\"*99**#:telstra.bigpond::\">Bigpond</option>"
										+"<option value=\"*99***1#:3netaccess::\">Hutchison 3G</option>"
										+"<option value=\"*99***1#:vfinternet.au::\">Vodafone</option>"
										+"</select>";
}
if(get_by_id("country").value == "61" && get_by_id("usb3g_isp").value == "2"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:telstra.internet::\" >Telstra</option>"
										+"<option value=\"*99#:Internet::\" selected>Optus</option>"
										+"<option value=\"*99**#:telstra.bigpond::\">Bigpond</option>"
										+"<option value=\"*99***1#:3netaccess::\">Hutchison 3G</option>"
										+"<option value=\"*99***1#:vfinternet.au::\">Vodafone</option>"
										+"</select>";
}
if(get_by_id("country").value == "61" && get_by_id("usb3g_isp").value == "3"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:telstra.internet::\" >Telstra</option>"
										+"<option value=\"*99#:Internet::\" >Optus</option>"
										+"<option value=\"*99**#:telstra.bigpond::\" selected>Bigpond</option>"
										+"<option value=\"*99***1#:3netaccess::\">Hutchison 3G</option>"
										+"<option value=\"*99***1#:vfinternet.au::\">Vodafone</option>"
										+"</select>";
}
if(get_by_id("country").value == "61" && get_by_id("usb3g_isp").value == "4"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:telstra.internet::\" >Telstra</option>"
										+"<option value=\"*99#:Internet::\" >Optus</option>"
										+"<option value=\"*99**#:telstra.bigpond::\" >Bigpond</option>"
										+"<option value=\"*99***1#:3netaccess::\" selected>Hutchison 3G</option>"
										+"<option value=\"*99***1#:vfinternet.au::\">Vodafone</option>"
										+"</select>";
}
if(get_by_id("country").value == "61" && get_by_id("usb3g_isp").value == "5"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:telstra.internet::\" >Telstra</option>"
										+"<option value=\"*99#:Internet::\" >Optus</option>"
										+"<option value=\"*99**#:telstra.bigpond::\" >Bigpond</option>"
										+"<option value=\"*99***1#:3netaccess::\" >Hutchison 3G</option>"
										+"<option value=\"*99***1#:vfinternet.au::\" selected>Vodafone</option>"
										+"</select>";
}
if(get_by_id("country").value == "2" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":internet.comcel.com.co:comcelweb:comcelweb\" selected>Rogers</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "57" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.com:wapuser1:wap\" selected>Comcel</option>"
										+"</select>";
										
}


if(get_by_id("country").value == "208" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:websfr::\" selected>SFR</option>"
										+"</select>";
										
}

if(get_by_id("country").value == "66" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet::\" selected>AIS</option>"
										+"</select>";
										
}

if(get_by_id("country").value == "52" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:Internet.itelcel.com:webgprs:webgprs2002\" selected>Mexico</option>"
										+"</select>";
										
}

if(get_by_id("country").value == "617" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":::\" selected>EMTEL</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "639" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.com:saf:data\" selected>Safaricom</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "49" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:web.vodafone.de::ADMIN\" selected>Vodafone</option>"
										+"</select>";
										
}

if(get_by_id("country").value == "45" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:data.tre.dk::\" selected>3</option>"
										+"<option value=\"*99***1#:internet::\">TDC</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "45" && get_by_id("usb3g_isp").value == "2"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:data.tre.dk::\" >3</option>"
										+"<option value=\"*99***1#:internet::\" selected>TDC</option>"
										+"</select>";
										
}


if(get_by_id("country").value == "47" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.netcom.no::\" selected>Netcom</option>"
										+"<option value=\"*99#:telenor::\">Telenor</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "47" && get_by_id("usb3g_isp").value == "2"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.netcom.no::\" >Netcom</option>"
										+"<option value=\"*99##:telenor::\" selected>Telenor</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "972" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internetg::\" selected>Cellcom</option>"
										+"<option value=\"*99#:uinternet::\">Orange</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "972" && get_by_id("usb3g_isp").value == "2"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internetg::\" >Cellcom</option>"
										+"<option value=\"*99#:uinternet::\" selected>Orange</option>"
										+"</select>";
										
}

if(get_by_id("country").value == "1" && get_by_id("usb3g_isp").value == "1"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:proxy::\" selected>AT&T</option>"
										+"<option value=\"*99***1#:isp.cingular::\">AT&T</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "1" && get_by_id("usb3g_isp").value == "2"){		
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:proxy::\" >AT&T</option>"
										+"<option value=\"*99***1#:isp.cingular::\" selected>AT&T</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "39" && get_by_id("usb3g_isp").value == "1"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":Telecom Italia Mobile::\" selected>Italy</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "351" && get_by_id("usb3g_isp").value == "1"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":Optimus::\" selected>Portugal</option>"
										+"</select>";
										
}

if(get_by_id("country").value == "358" && get_by_id("usb3g_isp").value == "1"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\" selected>ODNA / Finnet</option>"
										+"<option value=\"*99***1#:internet::\">Elisa</option>"
										+"<option value=\"*99***1#:internet.saunalahti::\">Saunalahti</option>"
										+"<option value=\"*99***1#:prointernet::\">Sonera</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "358" && get_by_id("usb3g_isp").value == "2"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\" >ODNA / Finnet</option>"
										+"<option value=\"*99***1#:internet::\" selected>Elisa</option>"
										+"<option value=\"*99***1#:internet.saunalahti::\">Saunalahti</option>"
										+"<option value=\"*99***1#:prointernet::\">Sonera</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "358" && get_by_id("usb3g_isp").value == "3"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\" >ODNA / Finnet</option>"
										+"<option value=\"*99***1#:internet::\" >Elisa</option>"
										+"<option value=\"*99***1#:internet.saunalahti::\" selected>Saunalahti</option>"
										+"<option value=\"*99***1#:prointernet::\">Sonera</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "358" && get_by_id("usb3g_isp").value == "4"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\" >ODNA / Finnet</option>"
										+"<option value=\"*99***1#:internet::\" >Elisa</option>"
										+"<option value=\"*99***1#:internet.saunalahti::\" >Saunalahti</option>"
										+"<option value=\"*99***1#:prointernet::\" selected>Sonera</option>"
										+"</select>";
										
}
//
if(get_by_id("country").value == "46" && get_by_id("usb3g_isp").value == "1"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.tele2.se::\" selected>Tele2/Comviq</option>"
										+"<option value=\"*99#:internet.telenor.se::\">Telenor</option>"
										+"<option value=\"*99#:online.telia.se::\">Telia</option>"
										+"<option value=\"*99#:bredband.tre.se::\">Tre</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "46" && get_by_id("usb3g_isp").value == "2"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.tele2.se::\" >Tele2/Comviq</option>"
										+"<option value=\"*99#:internet.telenor.se::\" selected>Telenor</option>"
										+"<option value=\"*99#:online.telia.se::\">Telia</option>"
										+"<option value=\"*99#:bredband.tre.se::\">Tre</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "46" && get_by_id("usb3g_isp").value == "3"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.tele2.se::\" >Tele2/Comviq</option>"
										+"<option value=\"*99#:internet.telenor.se::\" >Telenor</option>"
										+"<option value=\"*99#:online.telia.se::\" selected>Telia</option>"
										+"<option value=\"*99#:bredband.tre.se::\">Tre</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "46" && get_by_id("usb3g_isp").value == "4"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.tele2.se::\" >Tele2/Comviq</option>"
										+"<option value=\"*99#:internet.telenor.se::\" >Telenor</option>"
										+"<option value=\"*99#:online.telia.se::\" >Telia</option>"
										+"<option value=\"*99#:bredband.tre.se::\" selected>Tre</option>"
										+"</select>";
										
}
//
if(get_by_id("country").value == "44" && get_by_id("usb3g_isp").value == "1"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":orangenet::\" selected>Orangenet</option>"
										+"<option value=\":::\">Vodafone</option>"
										+"<option value=\":::\">O2</option>"
										+"<option value=\":::\">T-mobile</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "44" && get_by_id("usb3g_isp").value == "2"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":orangenet::\" >Orangenet</option>"
										+"<option value=\":::\" selected>Vodafone</option>"
										+"<option value=\":::\">O2</option>"
										+"<option value=\":::\">T-mobile</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "44" && get_by_id("usb3g_isp").value == "3"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":orangenet::\" >Orangenet</option>"
										+"<option value=\":::\" >Vodafone</option>"
										+"<option value=\":::\" selected>O2</option>"
										+"<option value=\":::\">T-mobile</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "44" && get_by_id("usb3g_isp").value == "4"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":orangenet::\" >Orangenet</option>"
										+"<option value=\":::\" >Vodafone</option>"
										+"<option value=\":::\" >O2</option>"
										+"<option value=\":::\" selected>T-mobile</option>"
										+"</select>";
										
}

if(get_by_id("country").value == "62" && get_by_id("usb3g_isp").value == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:indosatm2::\" selected>IM2</option>"
										+"<option value=\"*99#:indosat3g:indosat:indosat\">INDOSAT</option>"
										+"<option value=\"*99#:www.xlgprs.net :xlgprs:proxl\">XL</option>"
										+"<option value=\"*99#:flash::\">Telkomsel Flash</option>"
										+"<option value=\"*99***1#:3gprs:3gprs:3gprs\">3</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "62" && get_by_id("usb3g_isp").value == "2"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:indosatm2::\" >IM2</option>"
										+"<option value=\"*99#:indosat3g:indosat:indosat\" selected>INDOSAT</option>"
										+"<option value=\"*99#:www.xlgprs.net :xlgprs:proxl\">XL</option>"
										+"<option value=\"*99#:flash::\">Telkomsel Flash</option>"
										+"<option value=\"*99***1#:3gprs:3gprs:3gprs\">3</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "62" && get_by_id("usb3g_isp").value == "3"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:indosatm2::\" >IM2</option>"
										+"<option value=\"*99#:indosat3g:indosat:indosat\" >INDOSAT</option>"
										+"<option value=\"*99#:www.xlgprs.net :xlgprs:proxl\" selected>XL</option>"
										+"<option value=\"*99#:flash::\">Telkomsel Flash</option>"
										+"<option value=\"*99***1#:3gprs:3gprs:3gprs\">3</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "62" && get_by_id("usb3g_isp").value == "4"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:indosatm2::\" >IM2</option>"
										+"<option value=\"*99#:indosat3g:indosat:indosat\" >INDOSAT</option>"
										+"<option value=\"*99#:www.xlgprs.net :xlgprs:proxl\" >XL</option>"
										+"<option value=\"*99#:flash::\" selected>Telkomsel Flash</option>"
										+"<option value=\"*99***1#:3gprs:3gprs:3gprs\">3</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "62" && get_by_id("usb3g_isp").value == "5"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:indosatm2::\" >IM2</option>"
										+"<option value=\"*99#:indosat3g:indosat:indosat\" >INDOSAT</option>"
										+"<option value=\"*99#:www.xlgprs.net :xlgprs:proxl\" >XL</option>"
										+"<option value=\"*99#:flash::\" >Telkomsel Flash</option>"
										+"<option value=\"*99***1#:3gprs:3gprs:3gprs\" selected>3</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "60" && get_by_id("usb3g_isp").value == "1"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:celcom3g::\" selected>Celcom</option>"
										+"<option value=\"*99***1#:unet:maxis:wap\">Maxis</option>"
										+"<option value=\"*99#:redtonei::\">Red Tone</option>"
										+"<option value=\"*99#:my3g::\">U-Mobile</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "60" && get_by_id("usb3g_isp").value == "2"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:celcom3g::\" >Celcom</option>"
										+"<option value=\"*99***1#:unet:maxis:wap\" selected>Maxis</option>"
										+"<option value=\"*99#:redtone::\">Red Tone</option>"
										+"<option value=\"*99#:my3g::\">U-Mobile</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "60" && get_by_id("usb3g_isp").value == "3"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:celcom3g::\" >Celcom</option>"
										+"<option value=\"*99***1#:unet:maxis:wap\" >Maxis</option>"
										+"<option value=\"*99#:redtone::\" selected>Red Tone</option>"
										+"<option value=\"*99#:my3g::\">U-Mobile</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "60" && get_by_id("usb3g_isp").value == "4"){
	isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:celcom3g::\" >Celcom</option>"
										+"<option value=\"*99***1#:unet:maxis:wap\" >Maxis</option>"
										+"<option value=\"*99#:redtone::\" >Red Tone</option>"
										+"<option value=\"*99#:my3g::\" selected>U-Mobile</option>"
										+"</select>";
}
if(get_by_id("country").value == "744" && get_by_id("usb3g_isp").value == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:gprs.claro.com.py:clarogprs:clarogprs999\" selected>Claro PY</option>"
										+"<option value=\"*99#:internet.ctimovil.com.py:ctigprs:ctigprs999\">Claro/CTI</option>"
										+"<option value=\"*99***1#:broadband.tigo.py:tigo:tigo\">Telecel</option>"
										+"</select>";
}
if(get_by_id("country").value == "744" && get_by_id("usb3g_isp").value == "2"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:gprs.claro.com.py:clarogprs:clarogprs999\" >Claro PY</option>"
										+"<option value=\"*99#:internet.ctimovil.com.py:ctigprs:ctigprs999\" selected>Claro/CTI</option>"
										+"<option value=\"*99***1#:broadband.tigo.py:tigo:tigo\">Telecel</option>"
										+"</select>";
}
if(get_by_id("country").value == "744" && get_by_id("usb3g_isp").value == "3"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:gprs.claro.com.py:clarogprs:clarogprs999\" >Claro PY</option>"
										+"<option value=\"*99#:internet.ctimovil.com.py:ctigprs:ctigprs999\" >Claro/CTI</option>"
										+"<option value=\"*99***1#:broadband.tigo.py:tigo:tigo\" selected>Telecel</option>"
										+"</select>";
}
//
if(get_by_id("country").value == "48" && get_by_id("usb3g_isp").value == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:erainternet:erainternet:erainternet\" selected>Era</option>"
										+"<option value=\"*99#:internet:internet:internet\">Orange</option>"
										+"<option value=\"*99#:internet::\">Plus</option>"
										+"</select>";
}
if(get_by_id("country").value == "48" && get_by_id("usb3g_isp").value == "2"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:erainternet:erainternet:erainternet\" >Era</option>"
										+"<option value=\"*99#:internet:internet:internet\" selected>Orange</option>"
										+"<option value=\"*99#:internet::\">Plus</option>"
										+"</select>";
}
if(get_by_id("country").value == "48" && get_by_id("usb3g_isp").value == "3"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:erainternet:erainternet:erainternet\" >Era</option>"
										+"<option value=\"*99#:internet:internet:internet\" >Orange</option>"
										+"<option value=\"*99#:internet::\" selected>Plus</option>"
										+"</select>";
}
//
if(get_by_id("country").value == "65" && get_by_id("usb3g_isp").value == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:sunsurf::\" selected>M1</option>"
										+"<option value=\"*99***1#:e-ideas::\">Singtel</option>"
										+"<option value=\"*99***1#:::\">StarHub</option>"
										+"<option value=\"*99***1#:::\">Power Grid</option>"
										+"</select>";
}
if(get_by_id("country").value == "65" && get_by_id("usb3g_isp").value == "2"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:sunsurf::\" >M1</option>"
										+"<option value=\"*99***1#:e-ideas::\" selected>Singtel</option>"
										+"<option value=\"*99***1#:::\">StarHub</option>"
										+"<option value=\"*99***1#:::\">Power Grid</option>"
										+"</select>";
}
if(get_by_id("country").value == "65" && get_by_id("usb3g_isp").value == "3"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:sunsurf::\" >M1</option>"
										+"<option value=\"*99***1#:e-ideas::\" >Singtel</option>"
										+"<option value=\"*99***1#:::\" selected>StarHub</option>"
										+"<option value=\"*99***1#:::\">Power Grid</option>"
										+"</select>";
}
if(get_by_id("country").value == "65" && get_by_id("usb3g_isp").value == "4"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:sunsurf::\" >M1</option>"
										+"<option value=\"*99***1#:e-ideas::\" >Singtel</option>"
										+"<option value=\"*99***1#:::\" >StarHub</option>"
										+"<option value=\"*99***1#:::\" selected>Power Grid</option>"
										+"</select>";
}
if(get_by_id("country").value == "63" && get_by_id("usb3g_isp").value == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet.globe.com.ph::\" selected>Globe</option>"
										+"</select>";
}
if(get_by_id("country").value == "27" && get_by_id("usb3g_isp").value == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":::\" selected>Vodacom</option>"
										+"<option value=\":::\">MTN</option>"
										+"</select>";
}
if(get_by_id("country").value == "27" && get_by_id("usb3g_isp").value == "2"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":::\" >Vodacom</option>"
										+"<option value=\":::\" selected>MTN</option>"
										+"</select>";
}

if(get_by_id("country").value == "852" && get_by_id("usb3g_isp").value == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\" selected>SmarTone-Vodafone</option>"
										+"<option value=\"*99***1#:mobile.three.com.hk::\">3 Hong Kong</option>"
										+"<option value=\"*99***1#:internet::\">One2Free</option>"
										+"<option value=\"*99#:pccw::\">PCCW mobile</option>"
										+"<option value=\"*99***1#:::\">All 3G ISP support</option>"
										+"</select>";
}
if(get_by_id("country").value == "852" && get_by_id("usb3g_isp").value == "2"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\" >SmarTone-Vodafone</option>"
										+"<option value=\"*99***1#:mobile.three.com.hk::\" selected>3 Hong Kong</option>"
										+"<option value=\"*99***1#:internet::\">One2Free</option>"
										+"<option value=\"*99#:pccw::\">PCCW mobile</option>"
										+"<option value=\"*99***1#:::\">All 3G ISP support</option>"
										+"</select>";
}
if(get_by_id("country").value == "852" && get_by_id("usb3g_isp").value == "3"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\" >SmarTone-Vodafone</option>"
										+"<option value=\"*99***1#:mobile.three.com.hk::\" >3 Hong Kong</option>"
										+"<option value=\"*99***1#:internet::\" selected>One2Free</option>"
										+"<option value=\"*99#:pccw::\">PCCW mobile</option>"
										+"<option value=\"*99***1#:::\">All 3G ISP support</option>"
										+"</select>";
}
if(get_by_id("country").value == "852" && get_by_id("usb3g_isp").value == "4"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\" >SmarTone-Vodafone</option>"
										+"<option value=\"*99***1#:mobile.three.com.hk::\" >3 Hong Kong</option>"
										+"<option value=\"*99***1#:internet::\" >One2Free</option>"
										+"<option value=\"*99#:pccw::\" selected>PCCW mobile</option>"
										+"<option value=\"*99***1#:::\">All 3G ISP support</option>"
										+"</select>";
}
if(get_by_id("country").value == "852" && get_by_id("usb3g_isp").value == "5"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\" >SmarTone-Vodafone</option>"
										+"<option value=\"*99***1#:mobile.three.com.hk::\" >3 Hong Kong</option>"
										+"<option value=\"*99***1#:internet::\" >One2Free</option>"
										+"<option value=\"*99#:pccw::\" >PCCW mobile</option>"
										+"<option value=\"*99***1#:::\" selected>All 3G ISP support</option>"
										+"</select>";
}
if(get_by_id("country").value == "886" && get_by_id("usb3g_isp").value == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet::\" selected>Far Eastern</option>"
										+"<option value=\"*99***1#:internet::\">Chunghwa Telecom</option>"
										+"<option value=\"*99#:internet::\">Taiwan Mobile</option>"
										+"<option value=\"*99#:vibo::\">Vibo</option>"
										+"</select>";
}
if(get_by_id("country").value == "886" && get_by_id("usb3g_isp").value == "2"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet::\" >Far Eastern</option>"
										+"<option value=\"*99***1#:internet::\" selected>Chunghwa Telecom</option>"
										+"<option value=\"*99#:internet::\">Taiwan Mobile</option>"
										+"<option value=\"*99#:vibo::\">Vibo</option>"
										+"</select>";
}
if(get_by_id("country").value == "886" && get_by_id("usb3g_isp").value == "3"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet::\" >Far Eastern</option>"
										+"<option value=\"*99***1#:internet::\" >Chunghwa Telecom</option>"
										+"<option value=\"*99#:internet::\" selected>Taiwan Mobile</option>"
										+"<option value=\"*99#:vibo::\">Vibo</option>"
										+"</select>";
}
if(get_by_id("country").value == "886" && get_by_id("usb3g_isp").value == "4"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet::\" >Far Eastern</option>"
										+"<option value=\"*99***1#:internet::\" >Chunghwa Telecom</option>"
										+"<option value=\"*99#:internet::\" >Taiwan Mobile</option>"
										+"<option value=\"*99#:vibo::\" selected>Vibo</option>"
										+"</select>";
}
if(get_by_id("country").value == "20" && get_by_id("usb3g_isp").value == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\" selected>Etisalat</option>"
										+"</select>";
}
if(get_by_id("country").value == "370" && get_by_id("usb3g_isp").value == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.ideasclaro.com.do:claro:claro\" selected>Telmex</option>"
										+"</select>";
										
}
if(get_by_id("country").value == "706" && get_by_id("usb3g_isp").value == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.ideasclaro::\" selected>Telmex</option>"
										+"</select>";
										
		}
if(get_by_id("country").value == "55" && get_by_id("usb3g_isp").value == "1"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:brt.br:brt:brt\" selected>Brasil Telecom</option>"
										+"<option value=\"*99#:ctbc.br:ctbc:ctbc\">CTBC</option>"
										+"<option value=\"*99***1#:telstra.bigpond::\">Claro</option>"
										+"<option value=\"*99#:gprs.oi.com.br::\">Oi</option>"
										+"<option value=\"*99***1#:sercomtel.com.br:sercomtel:sercomtel\">Sercomtel</option>"
										+"<option value=\"*99#:tim.br:tim:tim\" >TIM</option>"
										+"<option value=\"*99#:zap.vivo.com.br:vivo:vivo\" selected>Vivo</option>"										
										+"</select>";
}
if(get_by_id("country").value == "55" && get_by_id("usb3g_isp").value == "2"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:brt.br:brt:brt\" selected>Brasil Telecom</option>"
										+"<option value=\"*99#:ctbc.br:ctbc:ctbc\">CTBC</option>"
										+"<option value=\"*99***1#:telstra.bigpond::\">Claro</option>"
										+"<option value=\"*99#:gprs.oi.com.br::\">Oi</option>"
										+"<option value=\"*99***1#:sercomtel.com.br:sercomtel:sercomtel\">Sercomtel</option>"
										+"<option value=\"*99#:tim.br:tim:tim\" >TIM</option>"
										+"<option value=\"*99#:zap.vivo.com.br:vivo:vivo\" selected>Vivo</option>"	
										+"</select>";
}
if(get_by_id("country").value == "55" && get_by_id("usb3g_isp").value == "3"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:brt.br:brt:brt\" selected>Brasil Telecom</option>"
										+"<option value=\"*99#:ctbc.br:ctbc:ctbc\">CTBC</option>"
										+"<option value=\"*99***1#:telstra.bigpond::\">Claro</option>"
										+"<option value=\"*99#:gprs.oi.com.br::\">Oi</option>"
										+"<option value=\"*99***1#:sercomtel.com.br:sercomtel:sercomtel\">Sercomtel</option>"
										+"<option value=\"*99#:tim.br:tim:tim\" >TIM</option>"
										+"<option value=\"*99#:zap.vivo.com.br:vivo:vivo\" selected>Vivo</option>"									
										+"</select>";
}
if(get_by_id("country").value == "55" && get_by_id("usb3g_isp").value == "4"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:brt.br:brt:brt\" selected>Brasil Telecom</option>"
										+"<option value=\"*99#:ctbc.br:ctbc:ctbc\">CTBC</option>"
										+"<option value=\"*99***1#:telstra.bigpond::\">Claro</option>"
										+"<option value=\"*99#:gprs.oi.com.br::\">Oi</option>"
										+"<option value=\"*99***1#:sercomtel.com.br:sercomtel:sercomtel\">Sercomtel</option>"
										+"<option value=\"*99#:tim.br:tim:tim\" >TIM</option>"
										+"<option value=\"*99#:zap.vivo.com.br:vivo:vivo\" selected>Vivo</option>"										
										+"</select>";
}
if(get_by_id("country").value == "55" && get_by_id("usb3g_isp").value == "5"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:brt.br:brt:brt\" selected>Brasil Telecom</option>"
										+"<option value=\"*99#:ctbc.br:ctbc:ctbc\">CTBC</option>"
										+"<option value=\"*99***1#:telstra.bigpond::\">Claro</option>"
										+"<option value=\"*99#:gprs.oi.com.br::\">Oi</option>"
										+"<option value=\"*99***1#:sercomtel.com.br:sercomtel:sercomtel\">Sercomtel</option>"
										+"<option value=\"*99#:tim.br:tim:tim\" >TIM</option>"
										+"<option value=\"*99#:zap.vivo.com.br:vivo:vivo\" selected>Vivo</option>"										
										+"</select>";
}
if(get_by_id("country").value == "55" && get_by_id("usb3g_isp").value == "6"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:brt.br:brt:brt\" selected>Brasil Telecom</option>"
										+"<option value=\"*99#:ctbc.br:ctbc:ctbc\">CTBC</option>"
										+"<option value=\"*99***1#:telstra.bigpond::\">Claro</option>"
										+"<option value=\"*99#:gprs.oi.com.br::\">Oi</option>"
										+"<option value=\"*99***1#:sercomtel.com.br:sercomtel:sercomtel\">Sercomtel</option>"
										+"<option value=\"*99#:tim.br:tim:tim\" >TIM</option>"
										+"<option value=\"*99#:zap.vivo.com.br:vivo:vivo\" selected>Vivo</option>"									
										+"</select>";
}
if(get_by_id("country").value == "55" && get_by_id("usb3g_isp").value == "7"){		
	get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:brt.br:brt:brt\" selected>Brasil Telecom</option>"
										+"<option value=\"*99#:ctbc.br:ctbc:ctbc\">CTBC</option>"
										+"<option value=\"*99***1#:telstra.bigpond::\">Claro</option>"
										+"<option value=\"*99#:gprs.oi.com.br::\">Oi</option>"
										+"<option value=\"*99***1#:sercomtel.com.br:sercomtel:sercomtel\">Sercomtel</option>"
										+"<option value=\"*99#:tim.br:tim:tim\" >TIM</option>"
										+"<option value=\"*99#:zap.vivo.com.br:vivo:vivo\" selected>Vivo</option>"									
										+"</select>";
}
}

 function CountryList(){
		var country_temp =get_by_id("country").value;
		var isp_temp = get_by_id("subISP");
		if(country_temp == "0"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"</select>";										
		}
		if(country_temp == "999999"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+"<option value=\"0\">-- None --</option>"
										+"</select>";										
		}
		if(country_temp == "61"){
			get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:telstra.internet::\">Telstra</option>"
										+"<option value=\"*99#:Internet::\">Optus</option>"
										+"<option value=\"*99**#:telstra.bigpond::\">Bigpond</option>"
										+"<option value=\"*99***1#:3netaccess::\">Hutchison 3G</option>"
										+"<option value=\"*99***1#:vfinternet.au::\">Vodafone</option>"
										+"</select>";
										
		}
		if(country_temp == "2"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":internet.com:wapuser1:wap\">Rogers</option>"
										+"</select>";
										
		}
		if(country_temp == "57"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.comcel.com.co:comcelweb:comcelweb\">Comcel</option>"
										+"</select>";
										
		}
		if(country_temp == "208"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:websfr::\">SFR</option>"
										+"</select>";
										
		}		
		if(country_temp == "66"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet::\">AIS</option>"
										+"</select>";
										
		}		
		if(country_temp == "52"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:Internet.itelcel.com:webgprs:webgprs2002\">Telcel</option>"
										+"</select>";
										
		}		
		if(country_temp == "617"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":::\">EMTEL</option>"
										+"</select>";
										
		}		
		if(country_temp == "639"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:safaricom:saf:data\">Safaricom</option>"
										+"</select>";
										
		}		
		if(country_temp == "49"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:web.vodafone.de::ADMIN\">Vodafone</option>"
										+"</select>";
										
		}
				
		if(country_temp == "1"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:proxy::\">AT&T</option>"
										+"<option value=\"*99***1#:isp.cingular::\">AT&T</option>"
										+"</select>";
										
		}
		if(country_temp == "45"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:data.tre.dk::\">3</option>"
										+"<option value=\"*99***1#:internet::\">TDC</option>"
										+"</select>";
										
		}
		if(country_temp == "47"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.netcom.no::\">Netcom</option>"
										+"<option value=\"*99#:telenor::\">Telenor</option>"
										+"</select>";
										
		}		
		if(country_temp == "972"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internetg::\">Cellcom</option>"
										+"<option value=\"*99#:uinternet::\">Orange</option>"
										+"</select>";
										
		}				
		if(country_temp == "39"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":Telecom Italia Mobile::\">Italy</option>"
										+"</select>";
										
		}
		if(country_temp == "351"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":Optimus::\">Portugal</option>"
										+"</select>";
										
		}
		if(country_temp == "44"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":orangenet::\">Orangenet</option>"
										+"<option value=\":::\">Vodafone</option>"
										+"<option value=\":::\">O2</option>"
										+"<option value=\":::\">T-mobile</option>"
										+"</select>";
										
		}
		if(country_temp == "358"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\">DNA / Finnet</option>"
										+"<option value=\"*99***1#:internet::\">Elisa</option>"
										+"<option value=\"*99***1#:internet.saunalahti::\">Saunalahti</option>"
										+"<option value=\"*99***1#:prointernet::\">Sonera</option>"
										+"</select>";
										
		}
		if(country_temp == "46"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.tele2.se::\">Tele2/Comviq</option>"
										+"<option value=\"*99#:internet.telenor.se::\">Telenor</option>"
										+"<option value=\"*99#:online.telia.se::\">Telia</option>"
										+"<option value=\"*99#:bredband.tre.se::\">Tre</option>"
										+"</select>";
										
		}				
		if(country_temp == "62"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:indosatm2::\">IM2</option>"
										+"<option value=\"*99#:indosat3g:indosat:indosat\">INDOSAT</option>"
										+"<option value=\"*99#:www.xlgprs.net :xlgprs:proxl\">XL</option>"
										+"<option value=\"*99#:flash::\">Telkomsel Flash</option>"
										+"<option value=\"*99***1#:3gprs:3gprs:3gprs\">3</option>"
										+"</select>";
										
		}
		if(country_temp == "60"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:celcom3g::\">Celcom</option>"
										+"<option value=\"*99***1#:unet:maxis:wap\">Maxis</option>"
										+"<option value=\"*99#:redtone::\">Red Tone</option>"
										+"<option value=\"*99#:my3g::\">U-Mobile</option>"
										+"</select>";
										
		}
		if(country_temp == "744"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:gprs.claro.com.py:clarogprs:clarogprs999\">Claro PY</option>"
										+"<option value=\"*99#:internet.ctimovil.com.py:ctigprs:ctigprs999\">Claro/CTI</option>"
										+"<option value=\"*99***1#:broadband.tigo.py:tigo:tigo\">Telecel</option>"
										+"</select>";
										
		}
		if(country_temp == "48"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:erainternet:erainternet:erainternet\">Era</option>"
										+"<option value=\"*99#:internet:internet:internet\">Orange</option>"
										+"<option value=\"*99#:internet::\">Plus</option>"
										+"</select>";
										
		}				
		if(country_temp == "65"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:sunsurf::\">M1</option>"
										+"<option value=\"*99***1#:e-ideas::\">Singtel</option>"
										+"<option value=\"*99***1#:::\">StarHub</option>"
										+"<option value=\"*99***1#:::\">Power Grid</option>"
										+"</select>";
										
		}
		if(country_temp == "63"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet.globe.com.ph::\">Globe</option>"
										+"</select>";
										
		}
		if(country_temp == "27"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\":::\">Vodacom</option>"
										+"<option value=\":::\">MTN</option>"
										+"</select>";
										
		}
		if(country_temp == "852"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\">SmarTone-Vodafone</option>"
										+"<option value=\"*99***1#:mobile.three.com.hk::\">3 Hong Kong</option>"
										+"<option value=\"*99***1#:internet::\">One2Free</option>"
										+"<option value=\"*99#:pccw::\">PCCW mobile</option>"
										+"<option value=\"*99***1#:::\">All 3G ISP support</option>"
										+"</select>";
										
		}
		if(country_temp == "886"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet::\">Far Eastern</option>"
										+"<option value=\"*99***1#:internet::\">Chunghwa Telecom</option>"
										+"<option value=\"*99#:internet::\">Taiwan Mobile</option>"
										+"<option value=\"*99#:vibo::\">Vibo</option>"
										+"</select>";
										
		}
		if(country_temp == "20"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:internet::\">Etisalat</option>"
										+"</select>";
										
		}
		if(country_temp == "370"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.ideasclaro.com.do:claro:claro\">Telmex</option>"
										+"</select>";
										
		}
		if(country_temp == "706"){
			isp_temp.innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99#:internet.ideasclaro::\">Telmex</option>"
										+"</select>";
										
		}
		if(country_temp == "55"){
			get_by_id("subISP").innerHTML = "<select id=\"ispList\" name=\"ispList\" onChange=\"copy_data(this)\">"
										+'<option value=\"0\">-- '+_select_ISP +'--</option>'
										+"<option value=\"*99***1#:brt.br:brt:brt\">Brasil Telecom</option>"
										+"<option value=\"*99#:ctbc.br:ctbc:ctbc\">CTBC</option>"
										+"<option value=\"*99***1#:telstra.bigpond::\">Claro</option>"
										+"<option value=\"*99#:gprs.oi.com.br::\">Oi</option>"
										+"<option value=\"*99***1#:sercomtel.com.br:sercomtel:sercomtel\">Sercomtel</option>"
										+"<option value=\"*99#:tim.br:tim:tim\" >TIM</option>"
										+"<option value=\"*99#:zap.vivo.com.br:vivo:vivo\" selected>Vivo</option>"									
										+"</select>";
		}
		}
	
	function copy_data(obj){
		var get_data =get_by_id("get_data").value;
		var data = obj.options[obj.selectedIndex].value;
		var isp_val = data.split(":");		
		get_by_id("usb3g_dial_num").value = isp_val[0];
		get_by_id("usb3g_apn_name").value = isp_val[1];
		get_by_id("usb3g_username").value = isp_val[2];
		get_by_id("password").value = isp_val[3];
		get_by_id("get_data").value = obj.options.selectedIndex;
	}

