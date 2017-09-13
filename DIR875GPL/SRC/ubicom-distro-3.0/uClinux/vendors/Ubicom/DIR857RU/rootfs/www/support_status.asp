<html>
<head>
<link rel="STYLESHEET" type="text/css" href="css_router.css">
<script language="JavaScript" src="lingual_<% CmoGetCfg("lingual","none"); %>.js"></script>
<script language="JavaScript" src="public.js"></script>
<script language="JavaScript" src="public_msg.js"></script>
<title><% CmoGetStatus("title"); %></title>
<meta http-equiv=Content-Type content="text/html; charset=UTF8">
<meta http-equiv="REFRESH" content="<% CmoGetStatus("gui_logout"); %>">
</head>
<body topmargin="1" leftmargin="0" rightmargin="0" bgcolor="#757575">
	<table id="header_container" border="0" cellpadding="5" cellspacing="0" width="838" align="center">
      <tr>
        <td width="100%">&nbsp;&nbsp;<script>show_words(TA2)</script>: <a href="http://support.dlink.com.tw/"><% CmoGetCfg("model_number","none"); %></a></td>
		<td align="right" nowrap><script>show_words(TA3)</script>: <% CmoGetStatus("hw_version"); %> &nbsp;</td>
		<td align="right" nowrap><script>show_words(sd_FWV)</script>: <% CmoGetStatus("version"); %></td>
		<td>&nbsp;</td>
      </tr>
    </table>

<table id="topnav_container" border="0" cellpadding="0" cellspacing="0" width="838" align="center">
		<tr>
			<td align="center" valign="middle"><img src="wlan_masthead.gif" width="836" height="92"></td>
		</tr>
	</table>
<table border="0" cellpadding="2" cellspacing="1" width="838" align="center" bgcolor="#FFFFFF">
		<tr id="topnav_container">
			<td><img src="short_modnum.gif" width="125" height="25"></td>
			<td id="topnavoff"><a href="index.asp"><script>show_words(_setup)</script></a></td>
			<td id="topnavoff"><a href="adv_virtual.asp"><script>show_words(_advanced)</script></a></td>
			<td id="topnavoff"><a href="tools_admin.asp"><script>show_words(_tools)</script></a></td>
			<td id="topnavoff"><a href="st_device.asp"><script>show_words(_status)</script></a></td>
			<td id="topnavon"><a href="support_men.asp"><script>show_words(_support)</script></a></td>
		</tr>
	</table>
	<table border="1" cellpadding="2" cellspacing="0" width="838" height="100%" align="center" bgcolor="#FFFFFF" bordercolordark="#FFFFFF">
		<tr>
		  <td id="sidenav_container" valign="top" width="125" align="right">
				<table border="0" cellpadding="0" cellspacing="0">
					<tr>
						<td id="sidenav_container">
							<div id=sidenav>
							<!-- === BEGIN SIDENAV === -->
								<ul>
									<li><div><a href="support_men.asp"><script>show_words(ish_menu)</script></a></div></li>
									<li><div><a href="support_internet.asp"><script>show_words(_setup)</script></a></div></li>
									<li><div><a href="support_adv.asp"><script>show_words(_advanced)</script></a></div></li>
									<li><div><a href="support_tools.asp"><script>show_words(_tools)</script></a></div></li>
									<li><div id="sidenavoff"><script>show_words(_status)</script></div></li>
								</ul>
								<!-- === END SIDENAV === -->
							</div>
						</td>
					</tr>
				</table>
		  </td>
			<td valign="top" id="maincontent_container">
				<div id="maincontent">

				  				  <div id="box_header">
						<h1> <script>show_words(help771)</script></h1>
						<table width=75% border=0 cellspacing=0 cellpadding=0>
                          <tr>
                            <td>
                              <ul>
                               <li><a href=support_status.asp#Device_Info><script>show_words(_devinfo)</script></a></li>
                                <li><a href=support_status.asp#Logs><script>show_words(_logs)</script></a></li>
                                <li><a href=support_status.asp#Statistics><script>show_words(_stats)</script></a></li>
                                <li><a href=support_status.asp#Internet_Sessions><script>show_words(YM157)</script></a></li>
                                <li><a href=support_status.asp#Wireless><script>show_words(_wireless)</script></a></li>
                                <li><a href=support_status.asp#ipv6>IPv6</a></li>
                              </ul></td>
                          </tr>
                        </table>
				  </div>

				  <div class="box">
				  <h2><A name=Device_Info><script>show_words(_devinfo)</script></A></h2>
				  <table border=0 cellspacing=0 cellpadding=0>
				  	<tr>

              <td> <P>
                  <script>show_words(help772)</script>
                  <script>show_words(sd_FWV)</script>
                </P>

                <P class=box_alert>
                  <script>show_words(help773)</script>
                </P>

                <P>
                  <script>show_words(help774)</script>
                </P>

                <DL>
                  <DT>
                    <script>show_words(help775)</script>
                  <DD>
                    <script>show_words(help776)</script>
                  <DT>
                    <script>show_words(help777)</script>
                  <DD>
                    <script>show_words(help778)</script>
                  <!--DT>
                    <script>show_words(help779)</script>
                  <DD>
                    <script>show_words(help780)</script>-->
                  <DT>
                    <script>show_words(_sdi_staticip)</script>
                  <DD>
                    <script>show_words(KR94)</script>
                  </DD>
                </DL>

                <DL>
                  <DT>
                    <script>show_words(sd_WLAN)</script>
                  <DD>
                    <script>show_words(LT291)</script>
                  <DT>
                    <script>show_words(_LANComputers)</script>
                  <DD>
                    <script>show_words(help781)</script>
                  <DT>
                    <script>show_words(_bln_title_IGMPMemberships)</script>
                  <DD>
                    <script>show_words(_bln_title_IGMPMemberships_h)</script>
                  </DD>
                </DL>
				  		</td>
                    </tr>
				  </table>
				  </div>

				  <div class="box">
				  <h2><A name=Logs><script>show_words(_logs)</script>
            </A></h2>
				  <table border=0 cellspacing=0 cellpadding=0>
				  <tr>

              <td> <P>
                  <script>show_words(help795)</script>
                </P>

                <DL>
                  <DT>
                    <script>show_words(sl_WtV)</script>
                  <DD>
                    <script>show_words(help796)</script>
                    <UL>
                      <LI>
                        <script>show_words(help797)</script>
<LI>
                        <script>show_words(_system)</script>
                      <LI>
                        <script>show_words(sl_RStat)</script>
                      </LI>
                    </UL>
                  <DT>
                    <script>show_words(sl_VLevs)</script>
                  <DD>
                    <script>show_words(help798)</script>
                    <UL>
                      <LI>
                        <script>show_words(sl_Critical)</script>
                      <LI>
                        <script>show_words(sl_Warning)</script>
                      <LI>
                        <script>show_words(sl_Infrml)</script>
                      </LI>
                    </UL>
                  <DT>
                    <script>show_words(sl_ApplySt)</script>
                  <DD>
                    <script>show_words(help799)</script>
                  <DT>
                    <script>show_words(sl_reload)</script>
                  <DD>
                    <script>show_words(help800)</script>
                  <DT>
                    <script>show_words(_clear)</script>
                  <DD>
                    <script>show_words(help801)</script>
                  <DT>
                    <script>show_words(sl_emailLog)</script>
                  <DD>
                    <script>show_words(help802)</script>
                  <DT>
                    <script>show_words(sl_saveLog)</script>
                  <DD>
                    <script>show_words(help803)</script>
                  </DD>
                </DL>
				  	</td>
                    </tr>
				  </table>
				  </div>

				  <div class="box">
				  <h2><A name=Statistics><script>show_words(_stats)</script>
            </A></h2>
				  <table border=0 cellspacing=0 cellpadding=0>
				  	<tr>

              <td> <P>
                  <script>show_words(help804)</script>
                </P>

                <DL>
                  <DT>
                    <script>show_words(ss_Sent)</script>
                  <DD>
                    <script>show_words(help806)</script>
                  <DT>
                    <script>show_words(ss_Received)</script>
                  <DD>
                    <script>show_words(help807)</script>
                  <DT>
                    <script>show_words(ss_TXPD)</script>
                  <DD>
                    <script>show_words(help808)</script>
                  <DT>
                    <script>show_words(ss_RXPD)</script>
                  <DD>
                    <script>show_words(help809)</script>
                  <DT>
                    <script>show_words(ss_Collisions)</script>
                  <DD>
                    <script>show_words(help810)</script>
                  <DT>
                    <script>show_words(ss_Errors)</script>
                  <DD>
                    <script>show_words(help811)</script>
                  </DD>
                </DL>
				  		</td>
                    </tr>
				  </table>
				  </div>

				  <div class="box">
				  <h2><A name=Internet_Sessions><script>show_words(YM157)</script>
            </A></h2>
				  <table border=0 cellspacing=0 cellpadding=0>
				  	<tr>

              <td> <P>
                  <script>show_words(help813)</script>
                </P>

                <DL>
                  <DT>
                    <script>show_words(sa_Local)</script>
                  <DD>
                    <script>show_words(help814)</script>
                  <DT>
                    <script>show_words(sa_NAT)</script>
                  <DD>
                    <script>show_words(help817)</script>
                  <DT>
                    <script>show_words(sa_Internet)</script>
                  <DD>
                    <script>show_words(help816)</script>
                  <DT>
                    <script>show_words(_protocol)</script>
                  <DD>
                    <script>show_words(help815)</script>
                  <DT>
                    <script>show_words(sa_State)</script>
                  <DD>
                    <script>show_words(help819)</script>
                    <UL>
                      <LI>NO:
                        <script>show_words(help819_1)</script>
                      <LI>SS:
                        <script>show_words(help819_2)</script>
                      <LI>EST:
                        <script>show_words(help819_3)</script>
                      <LI>FW:
                        <script>show_words(help819_4)</script>
                      <LI>CW:
                        <script>show_words(help819_5)</script>
                      <LI>TW:
                        <script>show_words(help819_6)</script>
                      <LI>LA:
                        <script>show_words(help819_7)</script>
                      <LI>CL:
                        <script>show_words(help819_8)</script>
                      </LI>
                    </UL>
                  <DT>
                    <script>show_words(sa_Dir)</script>
<DD>
                    <script>show_words(help820)</script>
                    <DL>
                      <DT>
                        <script>show_words(_Out)</script>
                      <DD>
                        <script>show_words(help821a)</script>
                      <DT>
                        <script>show_words(_In)</script>
                      <DD>
                        <script>show_words(help822a)</script>
                      </DD>
                    </DL>
                  <!--<DT>
                    <script>show_words(_priority)</script>
                  <DD>
                    <script>show_words(help818)</script>
                    -->
                  <DT>
                    <script>show_words(sa_TimeOut)</script>
                  <DD>
                    <script>show_words(help823)</script>
                    <script>show_words(help823_1)</script>
                    <DL>
                      <DT>300
                        <script>show_words(_seconds)</script>
                      <DD>
                        <script>show_words(help823_11)</script>
                      <DT>240
                        <script>show_words(_seconds)</script>
                      <DD>
                        <script>show_words(help823_13)</script>
                      <DT>7800
                        <script>show_words(help823_13)</script>
                      <DD>
                        <script>show_words(help823_17)</script>
                      </DD>
                    </DL>
                  </DD>
                </DL>
				  		</td>
                    </tr>
				  </table>
				  </div>
				  <div class="box">
				  <h2><A name=Wireless><script>show_words(_wireless)</script>
            </A></h2>
				  <table border=0 cellspacing=0 cellpadding=0>
				  	<tr>

              <td> <P>
                  <script>show_words(help782)</script>
                </P>

                <DL>
                  <DT>
                    <script>show_words(_macaddr)</script>
                  <DD>
                    <script>show_words(help783)</script>
                  <DT>
                    <script>show_words(_ipaddr)</script>
                  <DD>
                    <script>show_words(help784)</script>
                  <DT>
                    <script>show_words(_mode)</script>
                  <DD>
                    <script>show_words(help785)</script>
                  <DT>
                    <script>show_words(_rate)</script>
                  <DD>
                    <script>show_words(help786)</script>
                  <DT>
                    <script>show_words(help787)</script>
                  <DD>
                    <script>show_words(help788)</script>
                  </DD>
                </DL>
				  		</td>
                    </tr>
				  </table>
				  </div>

				  <div class="box"><h2><A name=ipv6>IPv6</A></h2>
				  <table border=0 cellspacing=0 cellpadding=0>  
				  	<tr>				  		
                      <td> <p><script>show_words(STATUS_IPV6_DESC_0)</script></p>            
                			<DT> 
                    		<script>show_words(STATUS_IPV6_DESC_1)</script>
                  			<DD> 
                            <DL>
                      			<DT>
								<script>show_words(IPV6_TEXT29a)</script>                        
                      			<DD> 
                       				<script>show_words(STATUS_IPV6_DESC_3)</script> 
								<DD> 
                      			<DT><script>show_words(IPV6_TEXT47)</script>                       
                      			<DD> 
                        			<script>show_words(STATUS_IPV6_DESC_5)</script>                       
                     			 </DD>
                    		</DL>
							
							<DT><script>show_words(TEXT072)</script>
							<DD> 
							<DL>
							<DT>
							<script>show_words(_name)</script>
							<DD>
							<script>show_words(STATUS_IPV6_DESC_2)</script>
							<DD>
							<DT>
                    		<script>show_words(IPV6_TEXT0)</script>
							<DD>
							<script>show_words(STATUS_IPV6_DESC_6)</script>
							</DD>
							</DD>
							</DL>
				  	  </td>
                    </tr>
				  </table>
				  </div>

      </div></td>
                </td>
		</tr>
	</table>
	<table id="footer_container" border="0" cellpadding="0" cellspacing="0" width="838" align="center">
		<tr>
			<td width="125" align="center">&nbsp;&nbsp;<img src="wireless_tail.gif" width="114" height="35"></td>
			<td width="10">&nbsp;</td>
			<td>&nbsp;</td>
		</tr>
	</table>
<br>
<div id="copyright"><% CmoGetStatus("copyright"); %></div>
<br>
</body>
</html>