<h2>S2BGC Decoder</h2>
<p>S2BGC Decoder is a free and open-source decoder (GPL-3.0) to convert raw S2BGC telemetry into human readable .json files</p>

<h3>Installation</h3>
<p>To install, clone this repo, and run make</p>

<h3>Operation</h3>
<p>S2BGC_Decoder can be automated using a cronjob or run manually by a user from the command line.</p>

<h3>Requirements</h3>
<ul>

<li>Tested on a Linux computer running Ubuntu 24.04 and compiled with GNU g++ v11.4</li>
<li>The <b>S2BGC_PATH</b> environmental variable must be defined prior to running the decoder.</li>
</ul>

<h3>Cronjob Example</h3>
<pre>
S2BGC_PATH="/path/BGC-SOLO"
#  min hr dom mon dow command
  */15  *   *   *   * /path/BGC_Decoder 2>&1
</pre>

<h3>Code summary</h3>
<p>

<h3>Packet types</h3>
<table>
<tr><th>packet id</th><th>subid</th><th>description</th><th>decoder</th></tr>
<tr><td>0</td><td>all</td><td>GPS packets</td><td>src/gps/gps.cpp GPS()</td></tr>
<tr><td>c</td><td>12</td><td>BIT OK</td><td>src/BIT/BIT()</td></tr>
<tr><td>12</td><td></td><td>BIT Beacon</td><td>src/BIT/BIT_Beacon()</td></tr>
<tr><td>13</tr><td></td><td>BIT OK</td><td>src/BIT/BIT/BIT()</td></tr>
<tr><td>14</tr><td></td><td>BIT Fail</td><td>src/BIT/BIT/BIT()</td></tr>
</table>
