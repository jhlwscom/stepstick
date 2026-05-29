#pragma once
const char *overlayHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>StepStick Overlay</title>
  <style>
    body {
      font-family: system-ui, -apple-system, sans-serif;
      background: #0f0f14;
      color: #fff;
      display: flex;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
      margin: 0;
      padding: 24px;
      box-sizing: border-box;
      text-align: center;
    }
    .card {
      background: #1e1e1e;
      border: 1px solid #333344;
      border-radius: 12px;
      padding: 32px 28px;
      max-width: 400px;
      width: 100%;
    }
    h2 { margin: 0 0 10px; color: #00ffcc; font-size: 1.25rem; }
    p  { color: #aaaabc; margin: 0 0 20px; line-height: 1.6; font-size: 0.95rem; }
    a  {
      display: inline-block;
      background: #00ffcc;
      color: #111;
      text-decoration: none;
      padding: 12px 24px;
      border-radius: 8px;
      font-weight: 700;
      font-size: 0.95rem;
    }
    a:hover { background: #00e6b8; }
  </style>
</head>
<body>
  <div class="card">
    <h2>Themes have moved!</h2>
    <p>
      Overlays are now standalone HTML files you download from the gallery
      and add as a <strong>Local</strong> Browser Source in OBS — no mixed-content issues.
    </p>
    <a href="https://jhlwscom.github.io/stepstick/gallery/" target="_blank">
      Open Theme Gallery →
    </a>
  </div>
</body>
</html>
)rawliteral";
