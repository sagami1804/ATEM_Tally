const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html { text-align: center; font-family: Arial; }
    input { padding: 8px; font-size: 16px; margin: 5px; }
    button { padding: 10px 20px; font-size: 18px; }
  </style>
</head>
<body>
  <h1>M5 Tally Setup</h1>
  <form action="/setIP" method="POST">
    <label>Network SSID:</label><br>
    <input type="text" name="SSID" size="15" required><br>
    <label>Network Password:</label><br>
    <input type="text" name="Password" size="15" required><br><br>
    <label>ATEM IP:</label><br>
    <input type="text" name="atemIP" size="15" required><br><br>
    <button type="submit">submit</button>
  </form>
</body>
</html>
)rawliteral";
