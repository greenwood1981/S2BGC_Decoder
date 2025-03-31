<h2>S2BGC Decoder</h2>

<h3>Installation</h3>

<h3>Operation</h3>
<p>S2BGC_Decoder can be automated using a cronjob or manually run by a user from the command line. The <b>S2BGC_PATH</b> environmental variable must be defined prior to running the decoder.</p>
<h4>Cronjob Example</h4>
<pre>
S2BGC_PATH="/data1/BGC-SOLO"
#  min hr dom mon dow command
  */15  *   *   *   * /path/BGC_Decoder 2>&1
</pre>


<h3>Packet types</h3>
<table>
<tr><th>packet id</th><th>subid</th><th>description</th><th>decoder</th></tr>
<tr><td>0</td><td>all</td><td>GPS packets</td><td>src/gps/gps.cpp GPS()</td></tr>
<tr><td>c</td><td>12</td><td>BIT OK</td><td>src/BIT/BIT()</td></tr>
<tr><td>12</td><td></td><td>BIT Beacon</td><td>src/BIT/BIT_Beacon()</td></tr>
<tr><td>13</tr><td></td><td>BIT OK</td><td>src/BIT/BIT/BIT()</td></tr>
<tr><td>14</tr><td></td><td>BIT Fail</td><td>src/BIT/BIT/BIT()</td></tr>
</table>
