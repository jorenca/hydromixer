const express = require('express');
const fs = require('node:fs');
const { SerialPort, ReadlineParser } = require('serialport');
const _ = require('lodash');


let comPort = null;

function mixNowFn() {
  if (!comPort) return;
  comPort.write('M\n');
  comPort.flush();
}
function reportNowFn() {
  if (!comPort) return;
  comPort.write('M\n');
  comPort.flush();
}
setInterval(mixNowFn, 8*60*60*1000);
setInterval(reportNowFn, 3*60*1000);


const app = express();
const port = 4000;

app.use(express.static('public'));
app.get('/mixnow', (req, res) => { mixNowFn(); res.send('ok'); });

app.listen(port, () => console.log(`Web app running on port ${port}`));


let latestStatus = {
  timestamp: new Date(),
  tds: NaN,
  ph: NaN,
  temperature: NaN,
  tanklevel: NaN
};

function writeLatestStatus() {

  const csvStatusLine = [
    latestStatus.timestamp.toISOString(),
    latestStatus.tds,
    latestStatus.ph,
    latestStatus.temperature,
    latestStatus.tanklevel
  ].join(',');
  fs.appendFile('./public/data.csv', csvStatusLine + '\n', console.error);
}

function reportMixerStatus(statusLine) {
  if (!statusLine.startsWith('STATUS ')) {
    console.log('Mixer non-status line:', statusLine);
    return;
  }

  console.log('Reporting mixer status', statusLine);
  const newStatus = {
    timestamp: new Date()
  };
  const statusTokens = statusLine.split(' ').slice(1); // ignore STATUS token in the beginning
  for (i in statusTokens) {
    const [ tokenName, tokenVal ] = statusTokens[i].split('=');
    newStatus[tokenName.toLowerCase()] = tokenVal.trim();
  }
  //console.log('Parsed latest status as', newStatus);
  latestStatus = {
    ...latestStatus,
    ...newStatus
  };
  writeLatestStatus();
}

function connectToMixerBoard(comName) {
  console.log('Connecting to mixer board', comName);

  const comParser = new ReadlineParser();
  comPort = new SerialPort({ path: comName, baudRate: 115200 });
  comPort.pipe(comParser);
  comPort.on('error', err => {
    console.error(err);
    connectToMixerBoard(comName);
  });

  comParser.on('data', reportMixerStatus);
}


SerialPort.list()
  .then(portsList => {
    console.log('Available COM devices:', portsList);

    const comDevice = _(portsList)
      .filter({
        //vendorId: '1a86',
        productId: '7523'
      }).first();

      if (!comDevice) {
        console.error('Mixer board not connected!');
        return;
      }
      connectToMixerBoard(comDevice.path);
  });
