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
  refresh: 300,
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
var joystickPolarity=false;

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
    if(!isControllerFxMode && isShift){
      joystickPolarity=!joystickPolarity;
      console.log("Swap JS polarity");
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
      laserData["scene"+selectedPreset]["zoom"]=130;
      laserData["scene"+selectedPreset]["rotation"]=130;
      laserData["scene"+selectedPreset]["dots"]=0;
      laserData["scene"+selectedPreset]["animation"]=130;
    }
    if(!isControllerFxMode && isShift){
      laserData["scene"+selectedPreset]["positionY"]=130;
      laserData["scene"+selectedPreset]["scaleY"]=0;
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
      laserData["scene"+selectedPreset]["positionX"]=130;
      laserData["scene"+selectedPreset]["scaleX"]=130;
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
        const positionYMap=[19,19,19,19,20,20,20,20,21,21,21,21,22,22,22,22,23,23,23,23,24,24,24,24,25,25,25,25,26,26,26,26,27,27,27,27,28,28,28,28,29,29,29,29,30,30,30,30,31,31,31,31,32,32,32,32,33,33,33,33,34,34,34,34,35,35,35,35,36,36,36,36,37,37,37,37,38,38,38,38,39,39,39,39,40,40,40,40,41,41,41,41,42,42,42,42,43,43,43,43,44,44,44,44,45,45,45,45,46,46,46,46,47,47,47,47,48,48,48,48,49,49,49,49,50,50,50,50,50,50,50,50,50,50,50,50,51,51,51,51,52,52,52,52,53,53,53,53,54,54,54,54,55,55,55,55,56,56,56,56,57,57,57,57,58,58,58,58,59,59,59,59,60,60,60,60,61,61,61,61,62,62,62,62,63,63,63,63,64,64,64,64,65,65,65,65,66,66,66,66,67,67,67,67,68,68,68,68,69,69,69,69,70,70,70,70,71,71,71,71,72,72,72,72,73,73,73,73,74,74,74,74,75,75,75,75,76,76,76,76,77,77,77,77,78,78,78,78,79,79,79,79,80,80,80,80];
        dmxData.push( positionYMap[data.positionY] );
      }else if( x==5 || x==(5+12) ){
        const positionXMap=[182,174,170,166,164,162,160,159,158,158,158,158,157,157,157,157,156,156,156,156,155,155,155,155,154,154,154,154,153,153,153,153,152,152,152,152,151,151,151,151,150,150,150,150,149,149,149,149,148,148,148,148,147,147,147,147,146,146,146,146,145,145,145,145,144,144,144,144,143,143,143,143,142,142,142,142,141,141,141,141,140,140,140,140,139,139,139,139,138,138,138,138,137,137,137,137,136,136,136,136,135,135,135,135,134,134,134,134,133,133,133,133,132,132,132,132,131,131,131,131,130,130,130,130,0,0,0,0,0,0,0,0,0,0,0,0,192,192,192,192,193,193,193,193,194,194,194,194,195,195,195,195,196,196,196,196,197,197,197,197,198,198,198,198,199,199,199,199,200,200,200,200,201,201,201,201,202,202,202,202,203,203,203,203,204,204,204,204,205,205,205,205,206,206,206,206,207,207,207,207,208,208,208,208,209,209,209,209,210,210,210,210,211,211,211,211,212,212,212,212,213,213,213,213,214,214,214,214,215,215,215,215,216,216,216,216,217,217,217,217,218,218,218,218,219,219,219,219,220,222,224,228,234,238,242,246];
        if(joystickPolarity){
          dmxData.push( positionXMap[255-data.positionX] );
        }else{
          dmxData.push( positionXMap[data.positionX] );
        }
      }else if( x==6 || x==(6+12) ){
        const scaleYMap=[0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,15,15,15,15,16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,19,20,20,20,20,21,21,21,21,22,22,22,22,23,23,23,23,24,24,24,24,25,25,25,25,26,26,26,26,27,27,27,27,28,28,28,28,29,29,29,29,30,30,30,30,31,31,31,31,32,32,32,32,33,33,33,33,34,34,34,34,35,35,35,35,36,36,36,36,37,37,37,37,38,38,38,38,39,39,39,39,40,40,40,40,41,41,41,41,42,42,42,42,43,43,43,43,44,44,44,44,45,45,45,45,46,46,46,46,47,47,47,47,48,48,48,48,49,49,49,49,50,50,50,50,51,51,51,51,52,52,52,52,53,53,53,53,54,54,54,54,55,55,55,55,56,56,56,56,57,57,57,57,58,58,58,58,59,59,59,59,60,60,60,60,61,61,61,61,62,62,62,62,63,63,63,63];
        dmxData.push( scaleYMap[data.scaleY] );
      }else if( x==7 || x==(7+12) ){
        const scaleXMap=[176,168,164,160,159,159,159,159,158,158,158,158,157,157,157,157,156,156,156,156,155,155,155,155,154,154,154,154,153,153,153,153,152,152,152,152,151,151,151,151,150,150,150,150,149,149,149,149,148,148,148,148,147,147,147,147,146,146,146,146,145,145,145,145,144,144,144,144,143,143,143,143,142,142,142,142,141,141,141,141,140,140,140,140,139,139,139,139,138,138,138,138,137,137,137,137,136,136,136,136,135,135,135,135,134,134,134,134,133,133,133,133,132,132,132,132,131,131,131,131,130,130,130,130,129,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,129,129,129,129,130,130,130,130,131,131,131,131,132,132,132,132,133,133,133,133,134,134,134,134,135,135,135,135,136,136,136,136,137,137,137,137,138,138,138,138,139,139,139,139,140,140,140,140,141,141,141,141,142,142,142,142,143,143,143,143,144,144,144,144,145,145,145,145,146,146,146,146,147,147,147,147,148,148,148,148,149,149,149,149,150,150,150,150,151,151,151,151,152,152,152,152,153,153,153,153,154,154,154,154,155,155,155,155,156,156,156,156,157,164,168,176];
        dmxData.push( scaleXMap[data.scaleX]);
      }else if( x==8 || x==(8+12) ){
        const rotationMap=[238,230,228,226,224,222,220,220,219,219,219,219,218,218,218,218,217,217,217,217,216,216,216,216,215,215,215,215,214,214,214,214,213,213,213,213,212,212,212,212,211,211,211,211,210,210,210,210,209,209,209,209,208,208,208,208,207,207,207,207,206,206,206,206,205,205,205,205,204,204,204,204,203,203,203,203,202,202,202,202,201,201,201,201,200,200,200,200,199,199,199,199,198,198,198,198,197,197,197,197,196,196,196,196,195,195,195,195,194,194,194,194,193,193,193,193,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,128,128,128,129,129,129,129,130,130,130,130,131,131,131,131,132,132,132,132,133,133,133,133,134,134,134,134,135,135,135,135,136,136,136,136,137,137,137,137,138,138,138,138,139,139,139,139,140,140,140,140,141,141,141,141,142,142,142,142,143,143,143,143,144,144,144,144,145,145,145,145,146,146,146,146,147,147,147,147,148,148,148,148,149,149,149,149,150,150,150,150,151,151,151,151,152,152,152,152,153,153,153,153,154,154,154,154,155,155,155,155,156,158,160,162,164,166,168,170,172,176,182];  
        if(joystickPolarity){
          dmxData.push( rotationMap[data.rotation] );
        }else{
          dmxData.push( rotationMap[255-data.rotation] );
        }
        
      }else if( x==9 || x==(9+12) ){
        const zoomMap=[118,118,118,118,117,117,117,117,116,116,116,116,115,115,115,115,114,114,114,114,113,113,113,113,112,112,112,112,111,111,111,111,110,110,110,110,109,109,109,109,108,108,108,108,107,107,107,107,106,106,106,106,105,105,105,105,104,104,104,104,103,103,103,103,102,102,102,102,101,101,101,101,100,100,100,100,99,99,99,99,98,98,98,98,97,97,97,97,96,96,96,96,95,95,95,95,94,94,94,94,93,93,93,93,92,92,92,92,91,91,91,91,90,90,90,90,89,89,89,89,88,88,88,88,87,87,87,87,0,0,0,0,0,0,0,0,8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,15,15,15,15,16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,19,20,20,20,20,21,21,21,21,22,22,22,22,23,23,23,23,24,24,24,24,25,25,25,25,26,26,26,26,27,27,27,27,28,28,28,28,29,29,29,29,30,30,30,30,31,31,31,31,32,32,32,32,33,33,33,33,34,34,34,34,35,35,35,35,36,36,36,36,37,37,37,37];
        dmxData.push( zoomMap[data.zoom] );
      }else if( x==11 || x==(11+12) ){
          dmxData.push( data.dots );
      }else if( x==12 || x==(12+12) ){
        const animationMap=[118,118,118,118,117,117,117,117,116,116,116,116,115,115,115,115,114,114,114,114,113,113,113,113,112,112,112,112,111,111,111,111,110,110,110,110,109,109,109,109,108,108,108,108,107,107,107,107,106,106,106,106,105,105,105,105,104,104,104,104,103,103,103,103,102,102,102,102,101,101,101,101,100,100,100,100,99,99,99,99,98,98,98,98,97,97,97,97,96,96,96,96,95,95,95,95,94,94,94,94,93,93,93,93,92,92,92,92,91,91,91,91,90,90,90,90,89,89,89,89,88,88,88,88,87,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,15,15,15,15,16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,19,20,20,20,20,21,21,21,21,22,22,22,22,23,23,23,23,24,24,24,24,25,25,25,25,26,26,26,26,27,27,27,27,28,28,28,28,29,29,29,29,30,30,30,30,31,31,31,31,32,32,32,32,33,33,33,33,34,34,34,34,35,35,35,35,36,36,36,36,37,37,37,37];
        dmxData.push( animationMap[data.animation] );
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
  var maxLimit=255;
  var minLimit=0;
  if(mappedControl=="positionY"){
    var maxSliderInc=3;
    // var maxLimit=81;
    // var minLimit=19;
  }
  if(mappedControl=="scaleY"){
    var maxSliderInc=3;
    // var maxLimit=64;
    // var minLimit=0;
  }
  if(mappedControl=="animation"){
    var maxSliderInc=5;
  }
  if(mappedControl=="dots"){
    var maxSliderInc=30;
  }
  if(mappedControl=="rotation"){
    var maxSliderInc=3;
    var maxLimit=255;
    var minLimit=0;
  }
  if(mappedControl=="positionX" ||mappedControl=="rotation" || mappedControl=="dots" || mappedControl=="positionY"){
    ev.value=ev.value*-1;
  }
  if(ev.value!=0){
    if(jsPositions[stick][axisInterval]!=0){
      clearInterval(jsPositions[stick][axisInterval]);
    }
    jsPositions[stick][axisInterval]=setInterval(function(){
      var sliderInc=Math.abs(Math.floor(ev.value/32767*maxSliderInc));
      if(Math.sign(ev.value)==-1){
        if( laserData["scene"+selectedPreset][mappedControl]<(maxLimit-sliderInc) ){
          laserData["scene"+selectedPreset][mappedControl]=laserData["scene"+selectedPreset][mappedControl]+(sliderInc+incAdjustment);
        }else{
          laserData["scene"+selectedPreset][mappedControl]=maxLimit;
        }
        // if(mappedControl=="rotation" && laserData["scene"+selectedPreset][mappedControl]<=(minLimit)){
        //   laserData["scene"+selectedPreset][mappedControl]=minLimit;
        // }
      }else if(Math.sign(ev.value)==1){
        if( laserData["scene"+selectedPreset][mappedControl]>(minLimit+sliderInc) ){
          laserData["scene"+selectedPreset][mappedControl]=laserData["scene"+selectedPreset][mappedControl]-(sliderInc+incAdjustment);    
        }else{
          // if(mappedControl=="rotation"){
          //   laserData["scene"+selectedPreset][mappedControl]=0;
          // }else{
            laserData["scene"+selectedPreset][mappedControl]=minLimit;
          // }
        }
      }
      if(mappedControl=="scaleY" || mappedControl=="positionY"){
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
