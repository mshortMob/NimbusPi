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

app.listen(port, () => {
    console.log(` listening at http://localhost:${port}`)
  })
  
app.get('/on', function (req, res) {
    client.send('/composition/master', 1.0, () => {
        console.log("sent oscTestAddress on");
    });
    res.send(true);
});

app.get('/off', function (req, res) {
    client.send('/composition/master', 0.0, () => {
        console.log("sent oscTestAddress off");
    });
    res.send(false);
});

app.get('/test', function (req, res) {
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