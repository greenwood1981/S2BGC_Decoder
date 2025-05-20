<h1>S2BGC Decoder</h1>
<p>S2BGC Decoder is a free and open-source decoder (GPL-3.0) to convert raw S2BGC telemetry into human readable .json files</p>

<h2>Requirements</h2>
<ul>
  <li>A Linux operating system is recommended</li>
  <li>Compile with GNU g++ v11.4 or newer</li>
  <li>The <b>S2BGC_PATH</b> environmental variable must be defined prior to running the decoder.</li>
  <li>Modify config/default_meta.json to match your institution's information</li>
</ul>

<h2>Installation</h2>
<p>To install, clone this repo, and run make. g++ v11.4 or newer is recommended</p>

<h2>Operation</h2>
<p>S2BGC_Decoder can be automated using a cronjob or run manually by a user from the command line.</p>

<h2>Cronjob Example</h2>
<pre>
S2BGC_PATH="/path/BGC-SOLO"
#  min hr dom mon dow command
  */15  *   *   *   * /path/BGC_Decoder 2>&1
</pre>

<h2>Code summary</h2>

<h2>File Structure</h2>
<pre>
S2BGC/
├─ config/
│  ├─ config.json         (User configurable parameters - path definitions, etc..)
│  ├─ default_meta.json   (Default meta info assigned to a float)
├─ data/
│  ├─ 4001/
│  │  ├─ 4001_meta.json   (float specific meta info added near the top of each .json file)
│  │  ├─ hex
│  │  │  ├─ 4001_000.hex  (hex files are placed in this subdirectory after being processed)
│  │  │  ├─ 4001_001.hex
│  │  ├─ json
│  │  │  ├─ 4001_000.json (decoded .json file)
│  │  │  ├─ 4001_001.json
│  ├─ 40xx/
│  │  ├─ 40xx_meta.json
│  │  ├─ hex
│  │  ├─ json
├─ incoming/
│  ├─ 4001_002.hex        (unprocessed hex files)
│  ├─ 4001_003.hex
├─ log/                   (daily log files are generated and placed in this subdirectory)
├─ src/                   (Decoder c++ source code)
</pre>

<h2>JSON output</h2>
<p>A sample .json file is provided. See <a href="https://github.com/greenwood1981/S2BGC_Decoder/blob/master/example/4008_018.json">4008_018.json</a></p>

<h3>Float Specific metadata</h3>
<p>Each S2BGC .json file begins with a collection of float specific metadata. This information is meant to be updated by the user and is placed in a file in a float's subdiectory <b>data/40xx/40xx_meta.json</b></p>

```javascript
{
  "FILE_CREATION_DATE": "2025-05-19T12:20:01Z",
  "DECODER_VERSION": "0.87",
  "SCHEMA_VERSION": "0.1",
  "INTERNAL ID NUMBER": 4008,
  "WMO_ID NUMBER": 9999999,
  "TRANSMISSION ID NUMBER": 4008,
  "INSTRUMENT TYPE": "SOLO_BGC_MRV",
  "PI": "NICHOLSON, WIJFFELS",
  "OPERATING_INSTITUTION": "WHOI, Woods Hole, MA",
  "PROJECT_NAME": "GO-BGC, WHOI",
  "PROFILE_NUMBER": 0
}
```
<h3>hex_summary</h3>
<p>A summary of raw telemetered data is included after the float metadata. Timestamps and telemetry statistics are summarized in a single line for each RUDICS file and SBD message.</p>
```javascript
{
  "hex_summary": [
    { "PID":  0, "source": "RUDICS", "TIME": "2025-02-26T14:47:43Z", "momsn": 141, "size": 1102, "sensor_ids": [2,0,33,33,33,4,4,4,33,33,33,4] },
    { "PID":  1, "source": "RUDICS", "TIME": "2025-02-26T14:47:53Z", "momsn": 141, "size": 1862, "sensor_ids": [35,35,35,36,36,36,36,36,34,34] },
    { "PID":  2, "source": "RUDICS", "TIME": "2025-02-26T14:48:00Z", "momsn": 141, "size":  155, "sensor_ids": [35,35,35,4] }
  ]
}
```

<h3>packet_info</h3>
<p>Each S2BGC RUDICS session or SBD binary attachment may contain multiple packets of information. This section highlights each type of packets that have been received in a human readable format</p>
```javascript
{
  "packet_info": {
    "packet_count": 102,
    "packet_bytes": 3083,
    "packet_type": [
      { "id": "000100", "bytes":   27, "packets":  1, "description": "GPS start-dive" },
      { "id": "000200", "bytes":   27, "packets":  1, "description": "GPS end-dive" },
      { "id": "020300", "bytes":   40, "packets":  1, "description": "Argo Mission" },
      { "id": "040000", "bytes":   31, "packets":  1, "description": "Engineering Fall" },
      { "id": "040001", "bytes":  126, "packets":  1, "description": "Engineering Rise" },
      { "id": "040400", "bytes":   40, "packets":  1, "description": "Engineering Pump" },
      { "id": "040B08", "bytes":  111, "packets":  1, "description": "Engineering parameters" },
      { "id": "050200", "bytes":   23, "packets":  1, "description": "BIST Seabird CTD" },
      { "id": "050300", "bytes":   33, "packets":  1, "description": "BIST Dissolved Oxygen" },
      { "id": "050400", "bytes":   33, "packets":  1, "description": "BIST pH" },
      { "id": "050500", "bytes":   23, "packets":  1, "description": "BIST ECO" },
      { "id": "050600", "bytes":   33, "packets":  1, "description": "BIST OCR" },
      { "id": "05070C", "bytes":   59, "packets":  1, "description": "BIST Nitrate Engineering" },
      { "id": "050800", "bytes":   93, "packets":  1, "description": "BIST Nitrate spectrum" },
      { "id": "050900", "bytes":  125, "packets":  1, "description": "BIST Nitrate ascii" },
      { "id": "210000", "bytes":   32, "packets":  1, "description": "CTD Binned PRES" },
      { "id": "210020", "bytes":  204, "packets":  1, "description": "CTD Discrete PRES" },
      { "id": "210033", "bytes":   31, "packets":  1, "description": "CTD Drift PRES" }
}
```

<h3>S2BGC Packet types</h3>
<table>
<tr><th>packet id</th><th>subid</th><th>description</th><th>decoder</th></tr>
<tr><td>0</td><td>all</td><td>GPS packets</td><td>src/gps/gps.cpp GPS()</td></tr>
<tr><td>c</td><td>12</td><td>BIT OK</td><td>src/BIT/BIT()</td></tr>
<tr><td>12</td><td></td><td>BIT Beacon</td><td>src/BIT/BIT_Beacon()</td></tr>
<tr><td>13</tr><td></td><td>BIT OK</td><td>src/BIT/BIT/BIT()</td></tr>
<tr><td>14</tr><td></td><td>BIT Fail</td><td>src/BIT/BIT/BIT()</td></tr>
</table>
