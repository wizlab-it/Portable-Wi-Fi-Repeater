/**
 * @package Portable WiFi Repeater
 * Configuration web server pages
 * @author WizLab.it
 * @version 20240204.019
 */


/**
 * Page layout
 */
const char configurationPortalHtmlLayout[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Portable Wi-Fi Repeater</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
* { font-family:sans-serif; font-size:14px; }
body { background-color:#328f8a; }
h1 { font-size:22px; color:#FFF; text-align:center; }
h3 { font-size:18px; text-align:center; }
input, select { outline:0; background:#f2f2f2; width:100%; border:1px solid #30ee30; margin:0 0 0px; padding:15px; box-sizing:border-box; font-size:14px; }
  input:invalid, select:invalid { border:1px solid #ee3030; }
  input[type=checkbox] { width:auto; margin-right:6px; vertical-align:middle; }
#customRepeaterWifiNetwork:not(:checked) ~ #customRepeaterWifiNetworkBox { display:none; }
#customRepeaterWifiNetwork:checked ~ #customRepeaterWifiNetworkBox { display:block; }
button { outline:0; background-color:#328f8a; width:100%; border:0; padding:15px; color:#FFF; font-size:14px; cursor:pointer; }
  button.small { width:auto; padding:3px 6px; font-size:12px; }
.tip { text-align:left; color:#666; font-size:12px; }
.box { background:#FFF; max-width:360px; margin:30px auto; padding:15px 45px; text-align:center; box-shadow:0 0 20px 0 rgba(0, 0, 0, 0.2), 0 5px 5px 0 rgba(0, 0, 0, 0.24); }
  .box .item { margin-bottom:15px; }
</style>
<script>
function openAsync(url) {
  const xhttp = new XMLHttpRequest();
  xhttp.open("GET", url, true);
  xhttp.send();
}
</script>
</head>

<body>
<h1>Portable Wi-Fi Repeater Configuration</h1>
%BODY_CONTENT%
</body>
</html>
)rawliteral";


/**
 * Configuration form
 */
const char configurationPortalHtmlConfigForm[] PROGMEM = R"rawliteral(
<form action="/" method="POST" onsubmit="return confirm('Save configuration?');">
  <div class="box">
    <h3>Select the Wi-Fi Network to repeat</h3>
    <p><button type="button" class="small" onclick="openAsync('/identify');">Identify device (20 fast blinks)</button></p>
    <div class="item">
      <select name="staSSID" required><option></option>%AVAILABLE_NETWORKS%</select>
      <div class="tip">Select the Wi-Fi Network to be repeated <button type="button" class="small" onclick="location.reload(true);">Refresh</button></div>
    </div>
    <div class="item">
      <input name="staPSK" type="password" placeholder="PSK" pattern=".{5,30}" required>
      <div class="tip">Enter the PSK</div>
    </div>
  </div>
  <div class="box">
    <input type="checkbox" id="customRepeaterWifiNetwork"><label for="customRepeaterWifiNetwork">Customize Repeater Wi-Fi Network</label>
    <div id="customRepeaterWifiNetworkBox">
      <h3>Repeater Wi-Fi Network</h3>
      <div class="item">
        <input name="apSSID" type="text" placeholder="Repeater Wi-Fi Network" pattern="[A-Za-z0-9]{8,}">
        <div class="tip">Enter the Wi-Fi Network name to be created</div>
      </div>
      <div class="item">
        <input name="apPSK" type="password" placeholder="PSK" pattern=".{8,30}">
        <div class="tip">Enter the PSK</div>
      </div>
    </div>
  </div>
  <div class="box">
    <button type="submit">Save</button>
  </div>
</form>
)rawliteral";


/**
 * Configuration saved
 */
const char configurationPortalHtmlConfigSaved[] PROGMEM = R"rawliteral(
<div class="box">
  <h3>Configuration saved!</h3>
  <p><button type="button" onclick="location.href='/reboot';">Reboot</button></p>
</div>
)rawliteral";