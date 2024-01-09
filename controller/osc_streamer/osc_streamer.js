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
var oscPath="/"
var currentSettings={};
console.log(client);

app.listen(port, () => {
    console.log(` listening at http://localhost:${port}`)
})

app.post('/updateSettings', function (req, res) {
    console.log(req.body);
    client.host=req.body.ipAddress;
    client.port=req.body.port;
    oscPath=req.body.oscAddress;
    currentSettings=req.body;
    var fileContents=JSON.stringify(req.body);
    fs.writeFile('/root/osc_streamer_settings.txt', fileContents, err => {
      if (err) {
        console.error(err);
      }
      console.log('saved preset data!');
      res.send("Settings Updated");
      console.log(client);
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

function sendOsc(path,value){
    client.send(path, value, () => {
        console.log("sent oscTestAddress on");
    });
    res.send(true);
}