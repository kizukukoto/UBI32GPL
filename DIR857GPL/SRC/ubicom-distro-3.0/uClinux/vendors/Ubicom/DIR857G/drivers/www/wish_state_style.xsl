<?xml version="1.0" encoding="UTF-8" ?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" xmlns="http://www.w3.org/1999/xhtml">
	<xsl:output method="html" indent="yes"/>
	<xsl:template match="/">
		<html>
			<body>
				<table border="1">
					<tr>
						<th>Expires (secs)</th>
						<th>Protocol</th>

						<th>From MAC</th>
						<th>From IP</th>
						<th>From Port</th>
						<th>To MAC</th>
						<th>To IP</th>
						<th>To Port</th>

						<th>Ext found</th>

						<th>Direction</th>
						<th>ipOS QoS</th>
						<th>Linux QoS</th>
						<th>QoS weight send</th>
						<th>QoS weight recv</th>
						<th>QoS boost count</th>
					</tr>
					<xsl:for-each select="//conn">
						<tr>
							<td>
								<xsl:if test="@expires &lt; 0">Never</xsl:if>
								<xsl:if test="@expires = '0'">Pending deletion</xsl:if>
								<xsl:if test="@expires &gt; 0"><xsl:value-of select="@expires"/></xsl:if>
							</td>
							<td>
								<xsl:choose>
									<xsl:when test="@protocol ='17'">UDP</xsl:when>
									<xsl:when test="@protocol ='6'">TCP</xsl:when>
									<xsl:when test="@protocol ='1'">ICMP</xsl:when>
									<xsl:when test="@protocol ='2'">IGMP</xsl:when>
									<xsl:when test="@protocol ='27'">RDP</xsl:when>
									<xsl:when test="@protocol ='41'">IPv6</xsl:when>
									<xsl:when test="@protocol ='46'">RSVP</xsl:when>
									<xsl:when test="@protocol ='47'">GRE</xsl:when>
									<xsl:when test="@protocol ='50'">ESP</xsl:when>
									<xsl:when test="@protocol ='58'">IPv6-ICMP</xsl:when>
									<xsl:otherwise><xsl:value-of select="@protocol"/></xsl:otherwise>
								</xsl:choose>
							</td>

							<td><xsl:value-of select="@smac_address"/></td>
							<td><xsl:value-of select="@sip_address"/></td>
							<td>
								<xsl:if test="@sport = '-1'">N/A</xsl:if>
								<xsl:if test="@sport != '-1'"><xsl:value-of select="@sport"/></xsl:if>
							</td>

							<td><xsl:value-of select="@dmac_address"/></td>
							<td><xsl:value-of select="@dip_address"/></td>
							<td>
								<xsl:if test="@dport = '-1'">N/A</xsl:if>
								<xsl:if test="@dport != '-1'"><xsl:value-of select="@dport"/></xsl:if>
							</td>

							<td>
								<xsl:if test="@ext_found = ''">N/A</xsl:if>
								<xsl:if test="@ext_found != ''"><xsl:value-of select="@ext_found"/></xsl:if>
							</td>

							<td><xsl:value-of select="@direction"/></td>

							<td><xsl:value-of select="@ipos_qos"/>
								(
								<xsl:choose>
									<xsl:when test="@ipos_qos ='0'">BE_LO</xsl:when>
									<xsl:when test="@ipos_qos ='1'">BK_LO</xsl:when>
									<xsl:when test="@ipos_qos ='2'">BK_HI</xsl:when>
									<xsl:when test="@ipos_qos ='3'">BE_HI</xsl:when>
									<xsl:when test="@ipos_qos ='4'">VI_LO</xsl:when>
									<xsl:when test="@ipos_qos ='5'">VI_HI</xsl:when>
									<xsl:when test="@ipos_qos ='6'">VO_LO</xsl:when>
									<xsl:when test="@ipos_qos ='7'">VO_HI</xsl:when>
									<xsl:otherwise>Untagged</xsl:otherwise>
								</xsl:choose>
								)
							</td>
							<td><xsl:value-of select="@linux_qos"/>
								(
								<xsl:choose>
									<xsl:when test="@linux_qos ='0'">BE</xsl:when>
									<xsl:when test="@linux_qos ='1'">BK</xsl:when>
									<xsl:when test="@linux_qos ='2'">BK</xsl:when>
									<xsl:when test="@linux_qos ='3'">BK</xsl:when>
									<xsl:when test="@linux_qos ='4'">BE</xsl:when>
									<xsl:when test="@linux_qos ='5'">BK</xsl:when>
									<xsl:when test="@linux_qos ='6'">VO</xsl:when>
									<xsl:when test="@linux_qos ='7'">VI</xsl:when>
									<xsl:when test="@linux_qos ='8'">BE</xsl:when>
									<xsl:when test="@linux_qos ='9'">BE</xsl:when>
									<xsl:when test="@linux_qos ='10'">BE</xsl:when>
									<xsl:when test="@linux_qos ='11'">BE</xsl:when>
									<xsl:when test="@linux_qos ='12'">BE</xsl:when>
									<xsl:when test="@linux_qos ='13'">BE</xsl:when>
									<xsl:when test="@linux_qos ='14'">BE</xsl:when>
									<xsl:when test="@linux_qos ='15'">BE</xsl:when>
									<xsl:otherwise>Untagged</xsl:otherwise>
								</xsl:choose>
								)
							</td>
							<td><xsl:value-of select="@qos_weight_send"/></td>
							<td><xsl:value-of select="@qos_weight_recv"/></td>
							<td><xsl:value-of select="@qos_boost_counter"/></td>
						</tr>
					</xsl:for-each>
				</table>
			</body>
		</html>
	</xsl:template>
</xsl:stylesheet>

