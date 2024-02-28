import Joystick from "@hkaspy/joystick-linux";
import Artnet from "artnet";
import express from "express"
import bodyParser from "body-parser";
import fs from "fs";
import path from "path";
import WebSocket, { WebSocketServer } from 'ws';
import {exec} from "child_process";
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

init();
var laserData, selectedPreset, lastButtonPressed;
var isControllerFxMode=false;
var isShift=false;
var isCruiseMode=false;
var cruiseSpeed=2;
var cruiseIntervalHandle=-1;
var isGateMode=false;
var isTriggerOn=false;

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

const stick = new Joystick("/dev/input/js0");
console.log("initializing joystick:");
console.log(stick);

stick.on("error", (err) => console.log('error initializing joystick!', err));

stick.on("disconnect", () => console.log('joystick disconnected'));

stick.on("update", (ev) => {

  // console.log(ev);

  extractJoystickEvents(ev, 'left-trigger-on', 'AXIS', 2, 0, ">", function(){
    if(!isControllerFxMode && !isShift){
      isGateMode=true;
      console.log("isGateMode On");
    }
  });

  extractJoystickEvents(ev, 'left-trigger-off', 'AXIS', 2, 0, "<=", function(){
    if(!isControllerFxMode && !isShift){
      isGateMode=false;
      console.log("isGateMode Off");
    }
  });

  extractJoystickEvents(ev, 'right-trigger-on', 'AXIS', 5, 0, ">", function(){
    if(!isControllerFxMode && !isShift){
      isTriggerOn=true;
      console.log("isTriggerOn true");
    }
  });

  extractJoystickEvents(ev, 'right-trigger-off', 'AXIS', 5, 0, "<=", function(){
    if(!isControllerFxMode && !isShift){
      isTriggerOn=false;
      console.log("isTriggerOn false");
    }
  });

  extractJoystickEvents(ev, 'right-tab-on', 'BUTTON', 5, 0, ">", function(){
    if(!isControllerFxMode && !isShift){
      if(selectedPreset<16){
        selectedPreset=selectedPreset+1;    
      }else{
          selectedPreset=1;
      }
      console.log(selectedPreset);
    }else if(!isControllerFxMode && isShift){
      copyPresetToNext();
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
    }else if(!isControllerFxMode && isShift){
      copyPresetToPrevious();
    }
  });

  extractJoystickEvents(ev, 'a-button-on', 'BUTTON', 0, 0, ">", function(){
    if(!isShift){
      if(laserData["scene"+selectedPreset]["color"]<10){
        laserData["scene"+selectedPreset]["color"]=laserData["scene"+selectedPreset]["color"]+1;    
      }else{
        laserData["scene"+selectedPreset]["color"]=1;
      }
    }
  });

  extractJoystickEvents(ev, 'x-button-on', 'BUTTON', 2, 0, ">", function(){
    if(!isShift){
      if(laserData["scene"+selectedPreset]["color"]>1){
        laserData["scene"+selectedPreset]["color"]=laserData["scene"+selectedPreset]["color"]-1;    
      }else{
        laserData["scene"+selectedPreset]["color"]=10;
      }
    }
  });

  extractJoystickEvents(ev, 'b-button-on', 'BUTTON', 1, 0, ">", function(){
    if(!isShift){
      if(laserData["scene"+selectedPreset]["gobo"]<25){
        laserData["scene"+selectedPreset]["gobo"]=laserData["scene"+selectedPreset]["gobo"]+1;    
      }else{
        laserData["scene"+selectedPreset]["gobo"]=1;
      }
    }
  });

  extractJoystickEvents(ev, 'y-button-on', 'BUTTON', 3, 0, ">", function(){
    if(!isShift){
      if(laserData["scene"+selectedPreset]["gobo"]>1){
        laserData["scene"+selectedPreset]["gobo"]=laserData["scene"+selectedPreset]["gobo"]-1;    
      }else{
        laserData["scene"+selectedPreset]["gobo"]=25;
      }
    }
  });
    
  extractJoystickEvents(ev, 'left-stick-x-axis', 'AXIS', 0, -40000, ">", function(){
    if(!isControllerFxMode && !isShift){
      parseJoystickToSlider(ev, 'left-stick', 'xInterval', 'rotation', 0);
    }
  });

  extractJoystickEvents(ev, 'left-stick-y-axis', 'AXIS', 1, -40000, ">", function(){
    if(!isControllerFxMode && !isShift){
      parseJoystickToSlider(ev, 'left-stick', 'yInterval', 'zoom', 0);
    }
  });

  extractJoystickEvents(ev, 'left-stick-button', 'BUTTON', 9, 0, ">", function(){
    if(!isControllerFxMode && !isShift){
      laserData["scene"+selectedPreset]["zoom"]=0;
      laserData["scene"+selectedPreset]["rotation"]=0;
      laserData["scene"+selectedPreset]["dots"]=0;
      laserData["scene"+selectedPreset]["animation"]=0;
    }
  });

  extractJoystickEvents(ev, 'right-stick-x-axis', 'AXIS', 3, -40000, ">", function(){
    if(!isControllerFxMode && !isShift){
      parseJoystickToSlider(ev, 'right-stick', 'xInterval', 'positionX', 0);
    }
    if(isControllerFxMode && !isShift){
      parseJoystickToPosition(ev, 'positionX', 'x');
    }
  });

  extractJoystickEvents(ev, 'right-stick-y-axis', 'AXIS', 4, -40000, ">", function(){
    if(!isControllerFxMode && !isShift){
      parseJoystickToSlider(ev, 'right-stick', 'yInterval', 'scaleX', 0);
    }
    if(isControllerFxMode && !isShift){
      parseJoystickToPosition(ev, 'positionY', 'y');
    }
  });

  extractJoystickEvents(ev, 'right-stick-button', 'BUTTON', 10, 0, ">", function(){
    if(!isControllerFxMode && !isShift){
      laserData["scene"+selectedPreset]["positionX"]=0;
      laserData["scene"+selectedPreset]["scaleX"]=0;
    }
  });

  extractJoystickEvents(ev, 'pad-x-axis', 'AXIS', 6, -40000, ">", function(){
    if(!isControllerFxMode){
      if(isShift==false){
        parseJoystickToSlider(ev, 'pad', 'xInterval', 'dots', 0);
      }else if(isShift==true){
        parseJoystickToSlider(ev, 'pad-shift', 'xInterval', 'positionY', 0);
      }
    }
  });

  extractJoystickEvents(ev, 'pad-y-axis', 'AXIS', 7, -40000, ">", function(){
    if(!isControllerFxMode){
      if(isShift==false){
        parseJoystickToSlider(ev, 'pad', 'yInterval', 'animation', 0);
      }else if(isShift==true){
        parseJoystickToSlider(ev, 'pad-shift', 'yInterval', 'scaleY', 0);
      }
    }
  });

  extractJoystickEvents(ev, 'back-button-on', 'BUTTON', 6, 0, ">", function(){
    isGateMode=false;
    isTriggerOn=false;
    if(!isControllerFxMode){
      console.log("isShift True");
      isShift=true;

      var sticks=["left-stick","right-stick","pad","pad-shift"];
      var intervals=["xInterval","yInterval"];
      for(var x=0; x<sticks.length; x++){
          for(var y=0; y<intervals.length; y++){
              // console.log(sticks[x]+"  "+intervals[y]+"  "+jsPositions[sticks[x]][intervals[y]]);
              clearInterval(jsPositions[sticks[x]][intervals[y]]);
              // jsPositions[sticks[x]][intervals[y]]=0;
          }
      }

    }
  });

  extractJoystickEvents(ev, 'back-button-off', 'BUTTON', 6, 0, "<=", function(){
    isGateMode=false;
    isTriggerOn=false;
    if(!isControllerFxMode){
      console.log("isShift False");
      isShift=false;

      var sticks=["left-stick","right-stick","pad","pad-shift"];
      var intervals=["xInterval","yInterval"];
      for(var x=0; x<sticks.length; x++){
          for(var y=0; y<intervals.length; y++){
              // console.log(sticks[x]+"  "+intervals[y]+"  "+jsPositions[sticks[x]][intervals[y]]);
              clearInterval(jsPositions[sticks[x]][intervals[y]]);
              // jsPositions[sticks[x]][intervals[y]]=0;
          }
      }

    }
  }); 

  extractJoystickEvents(ev, 'start-button-on', 'BUTTON', 7, 0, ">", function(){
    if(!isShift){
      // isControllerFxMode=!isControllerFxMode;
      // console.log("isControllerFxMode: "+isControllerFxMode);
      recallPresets();
    }else{
      savePresets();
    }
  }); 

});


function init(){
  laserData={};
  selectedPreset=1;
  for(var x=1; x<17; x++){
      laserData["scene"+x]={"color":1, "gobo":1, "positionX":0, "positionY":0, "scaleX":0, "scaleY":0, "rotation":0, "zoom":0, "animation":0, "dots":0  }
  }
  // savePresets();
  recallPresets();
}

function recallPresets(){
  fs.readFile('/root/laserControllerImages/js_controller_presets.txt', 'utf8', (err, data) => {
    if (err) {
      console.error(err);
      return;
    }
    laserData=JSON.parse(data);
    console.log('recalled saved presets!');
    emitEvent();
  });
  sendArtnet();
}

function savePresets(){
  var fileContents=JSON.stringify(laserData);
  fs.writeFile('/root/laserControllerImages/js_controller_presets.txt', fileContents, err => {
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
    // console.log("Sent Artnet");
    // console.log(dmxValues);
  });
}

function getDMXfromLaserData(data){
  var dmxData=[];
  var colorMapping=[0,15,18,22,27,32,37,10,50,255,];
  var goboMapping=[3,5,2,29,32,33,75,69,44,43,62,79,85,0,1,7,55,56,59,61,81,93,95,99,100];
  for(var x=1; x<25; x++){
      if(x==1 || x==1+12){
          dmxData.push(255);
      }else if( x==2 || x==(2+12) ){
        if(isGateMode==false){
          dmxData.push( colorMapping[data.color-1] );
        }else{
          if(isTriggerOn==true){
            dmxData.push( colorMapping[data.color-1] );
          }else{
            dmxData.push( colorMapping[0] );
          }         
        }
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
  messagePayload["isCruiseMode"]=isCruiseMode;
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

function parseJoystickToPosition(ev, mappedControl, axis){
  var positionRange={"x":45,"y":45};
  var positionCenter={"x":51,"y":51};
  laserData["scene"+selectedPreset][mappedControl]=positionCenter[axis]+positionRange[axis]*(ev.value/32768);
  console.log(positionCenter[axis]+positionRange[axis]*(ev.value/32768));
  emitEvent();
}

function parseJoystickToSlider(ev, stick, axisInterval, mappedControl, incAdjustment){
  var maxSliderInc=3;
  if(mappedControl=="positionY" || mappedControl=="scaleY"){
    var maxSliderInc=1;
  }
  if(mappedControl=="animation"){
    var maxSliderInc=4;
  }
  if(mappedControl=="dots"){
    var maxSliderInc=15;
  }
  if(mappedControl=="positionX" ||mappedControl=="rotation" || mappedControl=="dots" || mappedControl=="positionY"){
    ev.value=ev.value*-1;
  }
  if(ev.value!=0){
    if(jsPositions[stick][axisInterval]!=0){
      // console.log(jsPositions[stick][axisInterval]);
      clearInterval(jsPositions[stick][axisInterval]);
    }
    jsPositions[stick][axisInterval]=setInterval(function(){
      var sliderInc=Math.abs(Math.floor(ev.value/32767*maxSliderInc));
      // console.log('sliderInc: '+sliderInc);
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
      if(mappedControl=="scaleY" ||mappedControl=="positionY"){
        for(var x=1; x<17; x++){
          if(x!=selectedPreset){
            laserData["scene"+x][mappedControl]=laserData["scene"+selectedPreset][mappedControl];    
          }
        }
      }
      emitEvent();
    },50);
  }else if(ev.value==0){
    clearInterval(jsPositions[stick][axisInterval]);
    jsPositions[stick][axisInterval]=0;
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

app.get('/copyPresetToRow', function (req, res) {
  copyPresetToRow();
  res.send("copyPresetToRow complete");
});

app.get('/copyPresetToAll', function (req, res) {
  copyPresetToAll();
  res.send("copyPresetToAll complete");
});

function copyPresetToRow(){
  for(var x=(Math.floor(selectedPreset/4)*4+1); x<(Math.floor(selectedPreset/4)*4)+4+1;x++){
    if(x!=(selectedPreset)){
      laserData["scene"+x]=JSON.parse(JSON.stringify(laserData["scene"+selectedPreset]));
    }
  }
  emitEvent();
  sendArtnet();
}

function copyPresetToAll(){
  for(var x=1; x<17;x++){
    if(x!=(selectedPreset)){
      laserData["scene"+x]=JSON.parse(JSON.stringify(laserData["scene"+selectedPreset]));
    }
  }
  emitEvent();
  sendArtnet();
}

function copyPresetToNext(){
  var nextPreset=0;
  if(selectedPreset<16){
    nextPreset=selectedPreset+1;    
  }else{
    nextPreset=1;
  }
  laserData["scene"+nextPreset]=JSON.parse(JSON.stringify(laserData["scene"+selectedPreset]));
  selectedPreset=nextPreset;
  console.log(selectedPreset);
  emitEvent();
  sendArtnet();
}

function copyPresetToPrevious(){
  var nextPreset=0;
  if(selectedPreset>1){
    nextPreset=selectedPreset-1;    
  }else{
    nextPreset=16;
  }
  laserData["scene"+nextPreset]=JSON.parse(JSON.stringify(laserData["scene"+selectedPreset]));
  selectedPreset=nextPreset;
  console.log(selectedPreset);
  emitEvent();
  sendArtnet();
}

function increaseSelectedPreset(){
  if(selectedPreset<16){
      selectedPreset=selectedPreset+1;    
  }else{
      selectedPreset=1;
  }
  emitEvent();
  sendArtnet();
}

app.get('/toggleCruiseMode', function (req, res) {
  initCruiseMode();
  emitEvent();
  res.send(isCruiseMode);
});

function initCruiseMode(){
  if(isCruiseMode){
    isCruiseMode=false;
    console.log("clearing interval: "+ cruiseIntervalHandle);
    clearInterval(cruiseIntervalHandle);
  }else{
    isCruiseMode=true;
    cruiseIntervalHandle=setInterval(function(){
      increaseSelectedPreset();
    },(9-cruiseSpeed)*2000);
  }
}

app.get('/setCruiseSpeed', function (req, res) {
  setCruiseSpeed(parseInt(req._parsedUrl.query));
  res.send("done");
});

function setCruiseSpeed(speed){
  cruiseSpeed=speed;
  console.log("set cruiseSpeed: "+cruiseSpeed);
  if(isCruiseMode){
    clearInterval(cruiseIntervalHandle);
    cruiseIntervalHandle=setInterval(function(){
      increaseSelectedPreset();
    },(9-cruiseSpeed)*2000);
  }
}

app.get('/rebootPi', function (req, res) {
  res.send("rebooting....");
  exec('reboot', (err, stdout, stderr) => {});
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
