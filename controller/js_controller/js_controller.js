import Joystick from "@hkaspy/joystick-linux";
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

var artnetOptions = {
  host: '255.255.255.255',
  iface: '192.168.4.1',
  sendAll: true
}

const artnet = Artnet(artnetOptions);

const stick = new Joystick("/dev/input/js0");
console.log("xxxxx");
console.log(stick);

init();
var laserData, selectedPreset, lastButtonPressed;
var isControllerFxMode=false;
var isShift=false;

var jsPositions = {
  "left-stick":{
    "xInterval":0,
    "yInterval":0
  },
  "right-stick":{
    "xInterval":0,
    "yInterval":0
  },
  "pad":{
    "xInterval":0,
    "yInterval":0
  },
  "pad-shift":{
    "xInterval":0,
    "yInterval":0
  }
}

function init(){
  laserData={};
  selectedPreset=1;
  for(var x=1; x<17; x++){
      laserData["scene"+x]={"color":1, "gobo":1, "positionX":0, "positionY":0, "scaleX":0, "scaleY":0, "rotation":0, "zoom":0, "animation":0, "dots":0  }
  }
  // savePresets();
  
}

function recallPresets(){
  fs.readFile('/root/laserControllerImages/presets1.txt', 'utf8', (err, data) => {
    if (err) {
      console.error(err);
      return;
    }
    laserData=JSON.parse(data);
    console.log('recalled saved presets!');
    emitEvent();
  });
}

function savePresets(){
  var fileContents=JSON.stringify(laserData);
  fs.writeFile('/root/laserControllerImages/presets1.txt', fileContents, err => {
    if (err) {
      console.error(err);
    }
    console.log('saved preset data!');
  });
}

function sendArtnet(){
  var dmxValues=getDMXfromLaserData(laserData["scene"+selectedPreset]);
  // var dmxValues=[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];
  artnet.set(0,1, dmxValues, function (err, res) {
    console.log("Sent Artnet:");
    console.log(dmxValues);
  });
}

function getDMXfromLaserData(data){
  var dmxData=[];
  var colorMapping=[0,15,18,22,27,32,37,10,50,255];
  var goboMapping=[3,5,2,29,32,33,75,69,44,43,62,79,85,0,1,7];
  for(var x=1; x<25; x++){
      if(x==1 || x==1+12){
          dmxData.push(255);
      }else if( x==2 || x==(2+12) ){
          dmxData.push( colorMapping[data.color-1] );
      }else if( x==3 || x==(3+12) ){
          dmxData.push( goboMapping[data.gobo-1] );
      
      }else if( x==4 || x==(4+12) ){
          dmxData.push( data.positionY );
      }else if( x==5 || x==(5+12) ){
          dmxData.push( data.positionX );
      }else if( x==6 || x==(6+12) ){
          dmxData.push( data.scaleY );
      }else if( x==7 || x==(7+12) ){
          dmxData.push( data.scaleX );
      }else if( x==8 || x==(8+12) ){
          dmxData.push( data.rotation );
      }else if( x==9 || x==(9+12) ){
          dmxData.push( data.zoom );
      }else if( x==11 || x==(11+12) ){
          dmxData.push( data.dots );
      }else if( x==12 || x==(12+12) ){
          dmxData.push( data.animation );
      }else{
          dmxData.push(0);
      }
  }
  return dmxData;
}

function emitEvent(){
  var messagePayload={};
  messagePayload["selectedPreset"]=selectedPreset;
  messagePayload["laserData"]=laserData;
  messagePayload["lastButtonPressed"]=lastButtonPressed;
  messagePayload["isControllerFxMode"]=isControllerFxMode;
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(messagePayload));
    }
  });
  sendArtnet();
}

function extractJoystickEvents(inputEvent, label, type, number, value, comparator, action){
  if(comparator==">"){
    if(inputEvent.type == type && inputEvent.number == number && inputEvent.value > value){
      console.log(label);
      lastButtonPressed=label;
      action();
      emitEvent();
    }
  }else{
    if(inputEvent.type == type && inputEvent.number == number && inputEvent.value <= value){
      console.log(label);
      lastButtonPressed=label;
      action();
      emitEvent();
    }   
  }
}

stick.on("update", (ev) => {

  console.log(ev);

  extractJoystickEvents(ev, 'right-tab-on', 'BUTTON', 5, 0, ">", function(){
    if(!isControllerFxMode && !isShift){
      if(selectedPreset<16){
        selectedPreset=selectedPreset+1;    
      }else{
          selectedPreset=1;
      }
      console.log(selectedPreset);
    }
  });

  extractJoystickEvents(ev, 'left-tab-on', 'BUTTON', 4, 0, ">", function(){
    if(!isControllerFxMode && !isShift){
      if(selectedPreset>1){
        selectedPreset=selectedPreset-1;    
      }else{
          selectedPreset=16;
      }
      console.log(selectedPreset);
    }
  });

  extractJoystickEvents(ev, 'a-button-on', 'BUTTON', 0, 0, ">", function(){
    if(!isControllerFxMode && !isShift){
      if(laserData["scene"+selectedPreset]["color"]<10){
        laserData["scene"+selectedPreset]["color"]=laserData["scene"+selectedPreset]["color"]+1;    
      }else{
        laserData["scene"+selectedPreset]["color"]=1;
      }
    }
  });

  extractJoystickEvents(ev, 'x-button-on', 'BUTTON', 2, 0, ">", function(){
    if(!isControllerFxMode && !isShift){
      if(laserData["scene"+selectedPreset]["color"]>1){
        laserData["scene"+selectedPreset]["color"]=laserData["scene"+selectedPreset]["color"]-1;    
      }else{
        laserData["scene"+selectedPreset]["color"]=10;
      }
    }
  });

  extractJoystickEvents(ev, 'b-button-on', 'BUTTON', 1, 0, ">", function(){
    if(!isControllerFxMode && !isShift){
      if(laserData["scene"+selectedPreset]["gobo"]<16){
        laserData["scene"+selectedPreset]["gobo"]=laserData["scene"+selectedPreset]["gobo"]+1;    
      }else{
        laserData["scene"+selectedPreset]["gobo"]=1;
      }
    }
  });

  extractJoystickEvents(ev, 'y-button-on', 'BUTTON', 3, 0, ">", function(){
    if(!isControllerFxMode && !isShift){
      if(laserData["scene"+selectedPreset]["gobo"]>1){
        laserData["scene"+selectedPreset]["gobo"]=laserData["scene"+selectedPreset]["gobo"]-1;    
      }else{
        laserData["scene"+selectedPreset]["gobo"]=16;
      }
    }
  });
    
  extractJoystickEvents(ev, 'left-stick-x-axis', 'AXIS', 0, -40000, ">", function(){
    if(!isControllerFxMode && !isShift){
      parseJoystickToSlider(ev, 'left-stick', 'xInvterval', 'rotation', 0);
    }
  });

  extractJoystickEvents(ev, 'left-stick-y-axis', 'AXIS', 1, -40000, ">", function(){
    if(!isControllerFxMode && !isShift){
      parseJoystickToSlider(ev, 'left-stick', 'yInvterval', 'zoom', 0);
    }
  });

  extractJoystickEvents(ev, 'right-stick-x-axis', 'AXIS', 3, -40000, ">", function(){
    if(!isControllerFxMode && !isShift){
      parseJoystickToSlider(ev, 'right-stick', 'xInvterval', 'positionX', 0);
    }
  });

  extractJoystickEvents(ev, 'right-stick-y-axis', 'AXIS', 4, -40000, ">", function(){
    if(!isControllerFxMode && !isShift){
      parseJoystickToSlider(ev, 'right-stick', 'yInvterval', 'positionY', 0);
    }
  });

  extractJoystickEvents(ev, 'pad-x-axis', 'AXIS', 6, -40000, ">", function(){
    if(!isControllerFxMode){
      if(isShift==false){
        parseJoystickToSlider(ev, 'pad', 'xInvterval', 'dots', 0);
      }else if(isShift==true){
        parseJoystickToSlider(ev, 'pad-shift', 'xInvterval', 'scaleX', 0);
      }
    }
  });

  extractJoystickEvents(ev, 'pad-y-axis', 'AXIS', 7, -40000, ">", function(){
    if(!isControllerFxMode){
      if(isShift==false){
        parseJoystickToSlider(ev, 'pad', 'yInvterval', 'animation', 0);
      }else if(isShift==true){
        parseJoystickToSlider(ev, 'pad-shift', 'yInvterval', 'scaleY', 0);
      }
    }
  });

  extractJoystickEvents(ev, 'back-button-on', 'BUTTON', 6, 0, ">", function(){
    if(!isControllerFxMode){
      console.log("isShift True");
      isShift=true;
    }
  });

  extractJoystickEvents(ev, 'back-button-off', 'BUTTON', 6, 0, "<=", function(){
    if(!isControllerFxMode){
      console.log("isShift False");
      isShift=false;
    }
  }); 

  extractJoystickEvents(ev, 'start-button-on', 'BUTTON', 7, 0, ">", function(){
    if(!isShift){
      isControllerFxMode=!isControllerFxMode;
      console.log("isControllerFxMode: "+isControllerFxMode);
    }
  }); 

});

function parseJoystickToSlider(ev, stick, axisInterval, mappedControl, incAdjustment){
  const maxSliderInc=25;
  if(ev.value!=0){
    if(jsPositions[stick][axisInterval]!=false){
      clearInterval(jsPositions[stick][axisInterval]);
    }
    jsPositions[stick][axisInterval]=setInterval(function(){
      var sliderInc=Math.abs(Math.floor(ev.value/32767*maxSliderInc));
      if(Math.sign(ev.value)==-1){
        if( laserData["scene"+selectedPreset][mappedControl]<(255-sliderInc) ){
          laserData["scene"+selectedPreset][mappedControl]=laserData["scene"+selectedPreset][mappedControl]+(sliderInc+incAdjustment);    
        }else{
          laserData["scene"+selectedPreset][mappedControl]=255;
        }
      }else if(Math.sign(ev.value)==1){
        if( laserData["scene"+selectedPreset][mappedControl]>(sliderInc) ){
          laserData["scene"+selectedPreset][mappedControl]=laserData["scene"+selectedPreset][mappedControl]-(sliderInc+incAdjustment);    
        }else{
          laserData["scene"+selectedPreset][mappedControl]=0;
        }
      }
      emitEvent();
    },100);
  }else if(ev.value==0){
    clearInterval(jsPositions[stick][axisInterval]);
    jsPositions[stick][axisInterval]=false;
  }
}

app.listen(port, () => {
  console.log(` listening at http://localhost:${port}`)
})

app.get('/test', function (req, res) {
  const options = {
      root: "/root/"
  };

  const fileName = 'js_controller_gui.html';
  res.sendFile(fileName, options, function (err) {
      if (err) {
          next(err);
      } else {
          console.log('Sent:', fileName);
      }
  });
});

app.get('/action*', function (req, res) {
  // emitEvent("message","world");
  res.send('Hello World');
});

app.get('/laserControllerData*', function (req, res) {
  const options = {
      root: "/root/laserControllerImages"
  };

  const fileName = req.originalUrl.split('/')[2];
  res.sendFile(fileName, options, function (err) {
      if (err) {
          next(err);
      } else {
          console.log('Sent:', fileName);
      }
  });
});

app.get('/savePresets', function (req, res) {
  savePresets();
  res.send("save complete");
});

app.get('/recallPresets', function (req, res) {
  recallPresets();
  res.send("recall complete");
});

wss.on('connection', function connection(ws) {
  recallPresets();
  ws.on('message', function message(data) {
    console.log('received: %s', data);
    try{
      selectedPreset=JSON.parse(data).selectedPreset;
      laserData=JSON.parse(data).laserData;
      console.log(selectedPreset);
      sendArtnet();
    }catch{}
  });
  // ws.send('something');
});
