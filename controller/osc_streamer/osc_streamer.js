import { Client, Server } from 'node-osc';
import Artnet from "artnet";
import express from "express"
import bodyParser from "body-parser";
import fs from "fs";
import path from "path";
import WebSocket, { WebSocketServer } from 'ws';
const wss = new WebSocketServer({ port: 3003 });
const app = express();
const port=3002
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(bodyParser.raw());
const client = new Client('192.168.0.216', 3333);
var currentSettings={};

app.listen(port, () => {
    console.log(` listening at http://localhost:${port}`)
})

app.post('/updateSettings', function (req, res) {
    client.host=req.body.ipAddress;
    client.port=req.body.port;
    currentSettings=req.body;
    var fileContents=JSON.stringify(req.body);
    fs.writeFile('/root/osc_streamer_settings.txt', fileContents, err => {
      if (err) {
        console.error(err);
      }
      console.log('saved preset data!');
      res.send("Settings Updated");
    });
});

app.get('/recallSettings', function (req, res) {
    fs.readFile('/root/osc_streamer_settings.txt', 'utf8', (err, data) => {
        if (err) {
          console.error(err);
          return;
        }
        currentSettings=JSON.parse(data);
        console.log('recalled saved presets!');
        res.send(currentSettings);
      });
});

app.get('/', function (req, res) {
    const options = {
        root: "/root/"
    };
  
    const fileName = 'osc_streamer_gui.html';
    res.sendFile(fileName, options, function (err) {
        if (err) {
            next(err);
        } else {
            console.log('Sent:', fileName);
        }
    });
});

app.get('/test', function (req, res) {
    sendOsc(currentSettings.oscAddress, req._parsedUrl.query);
    res.send(true);
});

function sendOsc(path,value){
    console.log("preping osc: "+path+": "+value);
    client.send(path, value, () => {
        console.log("sent osc: "+path+": "+value);
    });
}