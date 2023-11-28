const { Client, Server } = require('node-osc');
const midi = require('midi');
const apcMiniInput = new midi.Input();
const apcMiniOutput = new midi.Output();
const express = require('express');
const bodyParser = require('body-parser');
const fs = require('fs');
const path = require('path');
const app = express();
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(bodyParser.raw());
const port = 3000;

init();
function init(){

oscServer = new Server(10001, '0.0.0.0', () => {
  console.log('OSC Server is listening');
});
  
colorMap= [
  [0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,255,255,250,245,240,235,230,225,220,215,210,205,200,195,190,185,180,175,170,165,160,155,150,145,140,135,130,125,120,115,110,105,100,95,90,85,80,75,70,65,60,55,50,45,40,35,30,25,20,15,10,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
  [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,255,255,250,245,240,235,230,225,220,215,210,205,200,195,190,185,180,175,170,165,160,155,150,145,140,135,130,125,120,115,110,105,100,95,90,85,80,75,70,65,60,55,50,45,40,35,30,25,20,15,10,5,0],
  [255,250,245,240,235,230,225,220,215,210,205,200,195,190,185,180,175,170,165,160,155,150,145,140,135,130,125,120,115,110,105,100,95,90,85,80,75,70,65,60,55,50,45,40,35,30,25,20,15,10,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,255]
];

  dmxChainOld=[
{
  "name":"multi_beam_1",
  "num_pixels":2,
  "pixel_channels":[
  {
  "r":"beam_r",
  "g":"beam_g",
  "b":"beam_b"
  },
  {
  "r":"wash_r",
  "g":"wash_g",
  "b":"wash_b"
  }
  ],
  "channels":{
    "x_pos":0,
    "x_fine":0,
    "y_pos":48,
    "y_fine":0,
    "spin":0,
    "brake":0,
    "dimmer":5,
    "strobe":0,
    "beam_r":0,
    "beam_g":0,
    "beam_b":0,
    "beam_w":0,
    "wash_r":0,
    "wash_g":0,
    "wash_b":0,
    "wash_w":0,
    "macro1":0,
    "macro2":0,
    "macro3":0,
    "macro4":0,
    "reset":0
  }
},
{
  "name":"multi_beam_1",
  "num_pixels":2,
  "pixel_channels":[
  {
  "r":"beam_r",
  "g":"beam_g",
  "b":"beam_b"
  },
  {
  "r":"wash_r",
  "g":"wash_g",
  "b":"wash_b"
  }
  ],
  "channels":{
    "x_pos":0,
    "x_fine":0,
    "y_pos":48,
    "y_fine":0,
    "spin":0,
    "brake":0,
    "dimmer":5,
    "strobe":0,
    "beam_r":0,
    "beam_g":0,
    "beam_b":0,
    "beam_w":0,
    "wash_r":0,
    "wash_g":0,
    "wash_b":0,
    "wash_w":0,
    "macro1":0,
    "macro2":0,
    "macro3":0,
    "macro4":0,
    "reset":0
  }
}];

var brake=240;
dmxChain=[
  {
    "name":"wash_1",
    "num_pixels":1,
    "pixel_channels":[
    {
    "r":"wash_r",
    "g":"wash_g",
    "b":"wash_b",
    "w":"wash_w"
    }
    ],
    "channels":{
      "x_pos":0,
      "y_pos":95,
      "strobe":100,
      "wash_r":100,
      "wash_g":0,
      "wash_b":0,
      "wash_w":0,
      "f":brake,
      "reset":0
    }
  },
  {
    "name":"wash_2",
    "num_pixels":1,
    "pixel_channels":[
    {
    "r":"wash_r",
    "g":"wash_g",
    "b":"wash_b",
    "w":"wash_w"
    }
    ],
    "channels":{
      "x_pos":0,
      "y_pos":95,
      "strobe":100,
      "wash_r":100,
      "wash_g":0,
      "wash_b":0,
      "wash_w":0,
      "f":brake,
      "reset":0
    }
  },
  {
    "name":"wash_3",
    "num_pixels":1,
    "pixel_channels":[
    {
    "r":"wash_r",
    "g":"wash_g",
    "b":"wash_b",
    "w":"wash_w"
    }
    ],
    "channels":{
      "x_pos":0,
      "y_pos":95,
      "strobe":100,
      "wash_r":100,
      "wash_g":0,
      "wash_b":0,
      "wash_w":0,
      "f":brake,
      "reset":0
    }
  },
  {
    "name":"wash_4",
    "num_pixels":1,
    "pixel_channels":[
    {
    "r":"wash_r",
    "g":"wash_g",
    "b":"wash_b",
    "w":"wash_w"
    }
    ],
    "channels":{
      "x_pos":0,
      "y_pos":95,
      "strobe":100,
      "wash_r":100,
      "wash_g":0,
      "wash_b":0,
      "wash_w":0,
      "f":brake,
      "reset":0
    }
  }];


  num_pixels=0;
  num_channels=0;
  for (var x in dmxChain){
    num_pixels=num_pixels+dmxChain[x].num_pixels;
    num_channels=num_channels+dmxChain[x].channels.length;
  }

  filterBuffer=[];
  for(var x=0;x<num_pixels;x++){
    filterBuffer.push([0,0,0,0]);
  }

  var artnetOptions = {
    host: '255.255.255.255',
    iface: '192.168.4.1',
    sendAll: true
  }
  artnet = require('artnet')(artnetOptions);
  patternNumber=56;
  selectedFixtures=[0,1,2,3,4];
  loopTime=1000;
  copySceneSource=null;
  copySceneTarget=null;
  copyFixtureSource=null;
  copyFixtureTarget=null;
  loopIsPlaying=false;
  apcMiniPort="None";
  fixtureBrightness=[1,1,1,1];
  fixtureMuteMode=0;
  masterBrightness=1;
  randomnessDebounce=true;
  randomnessCount=0;
  randomnessBuffer=[0,0,0,0];
  isShift=false;
  channels=[];
  loopCellsState=[];
  numFixtures=5;
  for(var x=0; x<64; x++){
    var fixtures=[]
    for(var y=0; y<numFixtures; y++){
      fixtures.push({
        "colorIndex":127,
        "colorSpread":127,
        "cycleTime":127,
        "trailLength":127,
        "trailSpread":127,
        "dir":127,
        "strobe":0,
        "brightness":127,
        "x":50,
        "y":50
      });
    }
    channels.push(fixtures);
    loopCellsState[x]=false;
  }
  for(var x=60;x<64;x++){
    loopCellsState[x]=true;
  }
  initMidiConnections();
  recall();
  setInterval(function(){
    syncArtnetToModel();
  },25);
}

function initMidiConnections(){
  for(var x=0; x<apcMiniInput.getPortCount(); x++){
    console.log(apcMiniInput.getPortName(x));
    if(apcMiniInput.getPortName(x).indexOf("APC MINI")!=-1){
      apcMiniPort=x;
      console.log("Found apcMini on port: "+x);
    }
  }
  if(apcMiniPort!="None"){
    apcMiniInput.openPort(apcMiniPort);
    apcMiniInput.ignoreTypes(true,false,true);
    apcMiniOutput.openPort(apcMiniPort);
  }else{
    console.log("A midi device is missing, exiting");
    process.exit(1);
  }
  for(var x=0;x<100;x++){
    apcMiniOutput.sendMessage([144,x,0]);  
  }
  syncLeds();
  setTimeout(function(){syncLeds();},500);
}

apcMiniInput.on('message', (deltaTime, message) => {
  //apc mini grid input
  if(message[0]==144 && message[2]==127){
    //top right button - set preset to row
    if(message[1]==82 && !isShift){
      for(var x=(Math.floor(patternNumber/4)*4); x<(Math.floor(patternNumber/4)*4)+4;x++){
        if(x!=patternNumber){
          for (var f=0;f<numFixtures;f++){
            channels[x][selectedFixtures[f]]=JSON.parse(JSON.stringify(channels[patternNumber][selectedFixtures[f]]));
          }
        }
      }
      console.log("Saved Preset To All");
    }
    //top right button shift - set preset to all
    if(message[1]==82){
      for(var x=0;x<64;x++){
        if(x!=patternNumber){
          for (var f=0;f<numFixtures;f++){
            channels[x][selectedFixtures[f]]=JSON.parse(JSON.stringify(channels[patternNumber][selectedFixtures[f]]));
          }
        }
      }
      console.log("Saved Preset To All");
    }

    // top right down 1 - Set XY To Row
    if(message[1]==83 && !isShift){
      for(var y=(Math.floor(patternNumber/4)*4); y<(Math.floor(patternNumber/4)*4)+4;y++){
        if(y!=patternNumber){
          for (var f=0;f<numFixtures;f++){
            channels[y][f].x=channels[patternNumber][f].x;
            channels[y][f].y=channels[patternNumber][f].y;
          }
        }
      }
      console.log("Set XY To Row");
    }
    // top right down 1 shift - Set XY To All
    if(message[1]==83 && isShift){
      for(var y=0;y<64;y++){
        if(y!=patternNumber){
          for (var f=0;f<numFixtures;f++){
            channels[y][f].x=channels[patternNumber][f].x;
            channels[y][f].y=channels[patternNumber][f].y;
          }
        }
      }
      console.log("Set XY To All");
    }

    if(message[1]>=0 && message[1]<64 && (message[1]%8)<=3){
      if(!isShift){
        patternNumber=message[1];
        syncArtnetToModel();
        syncLeds();
      }else{
        if(copySceneSource==null){
          copySceneSource=message[1];
          syncLeds();
        }else{
          copySceneTarget=message[1];
          copyScene(copySceneSource,copySceneTarget,[0,1,2,3,4]);
          copySceneSource=null;
          copySceneTarget=null;
          syncLeds();
        }
      }
    }
    if(message[1]>=0 && message[1]<64 && (message[1]%8)>3){
      loopCellsState[message[1]]=!loopCellsState[message[1]];
      console.log(loopCellsState[x]);
      syncLeds();
    }
    //fixture selectors
    if(message[1]>=64 && message[1]<68 && message[2]==127){
      if(!isShift){
        fixtureButtonPress(message[1]-64);
        syncLeds();
      }else{
        if(copyFixtureSource==null){
          copyFixtureSource=message[1]-64;
          syncLeds();
        }else{
          copyFixtureTarget=message[1]-64;
          copyFixture(copyFixtureSource,copyFixtureTarget);
          copyFixtureSource=null;
          copyFixtureTarget=null;
          syncLeds();
        }
      }
    }
    //start / stop
    if(message[1]==89 && message[2]==127){
      if(!loopIsPlaying){
        loopRoWStart(loopTime);
      }else{
        loopRoWStop();
      }
      syncLeds();
    }
    //save
    if(message[1]==84 && message[2]==127 && isShift){
      save();
    }
    //recall
    if(message[1]==85 && message[2]==127){
      recall();
    }
    //rotate fixtures
    if(message[1]==86 && message[2]==127){
      rotateFixtures(true);
    }
    //rotate fixtures
    if(message[1]==87 && message[2]==127){
      rotateFixtures(false);
    }
  }
  //fixture mute
  if(message[1]>=68 && message[1]<72 && !isShift){      
    if(message[0]==144){
      fixtureBrightness[message[1]-68]=fixtureMuteMode;
    }else{
      fixtureBrightness[message[1]-68]=1-fixtureMuteMode;
    }
    syncArtnetToModel();
    syncLeds();
  }
  // fixture mute mode
  if(message[1]==71 && message[0]==144 && isShift){
    fixtureMuteMode=(fixtureMuteMode+1)%2;
    for(var x=68; x<72; x++){
      fixtureBrightness[x-68]=1-fixtureMuteMode;
    }
    syncArtnetToModel();
    syncLeds();
  }
  //shift on / off
  if(message[1]==98 && message[2]==127){
    if(message[0]==144){
      isShift=true;
    }else{
      isShift=false;
    }
    copySceneSource=null;
    copySceneTarget=null;
    copyFixtureSource=null;
    copyFixtureTarget=null;
    syncLeds();
    console.log(isShift);
  }

  if(message[1]==88 && message[2]==127 && message[0]==144){
    fixtureButtonPress(4);
    syncLeds();
    console.log(selectedFixtures.toString());
  }

  //apc mini sliders input
  if(message[0]==176){
    if(message[1]>=48 && message[1]<=56 && isShift==false){
      if(message[1]==48){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].colorIndex=message[2];
        }
      }
      if(message[1]==49){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].colorSpread=message[2];
        }
      }
      if(message[1]==50){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].cycleTime=message[2];
        }
      }
      if(message[1]==51){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].trailLength=message[2];
        }
      }
      if(message[1]==52){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].trailSpread=message[2];
        }
      }
      if(message[1]==53){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].dir=message[2];
        }
      }
      if(message[1]==54){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].strobe=message[2];
        }
      }
      if(message[1]==55){
        for (var f in selectedFixtures){
          channels[patternNumber][selectedFixtures[f]].brightness=message[2];
        }
      }
      if(message[1]==56 && !isShift){
          loopTime=200+message[2]*50;
          if(loopIsPlaying){
            loopRoWStop();
            loopRoWStart(loopTime);
          }
      }
      syncArtnetToModel();
    }
    if(message[1]>=48 && message[1]<=56 && isShift==true){
      if(message[1]==48){
        channels[patternNumber][0].x=parseInt(message[2]*2);
      }
      if(message[1]==49){
        channels[patternNumber][0].y=parseInt(message[2]*2);
      }
      if(message[1]==50){
        channels[patternNumber][1].x=parseInt(message[2]*2);
      }
      if(message[1]==51){
        channels[patternNumber][1].y=parseInt(message[2]*2);
      }
      if(message[1]==52){
        channels[patternNumber][2].x=parseInt(message[2]*2);
      }
      if(message[1]==53){
        channels[patternNumber][2].y=parseInt(message[2]*2);
      }
      if(message[1]==54){
        channels[patternNumber][3].x=parseInt(message[2]*2);
      }
      if(message[1]==55){
        channels[patternNumber][3].y=parseInt(message[2]*2);
      }
      if(message[1]==56 && isShift){
        masterBrightness=message[2]/127;
      }
      syncArtnetToModel();
    }
  }
  console.log(message);
});

oscServer.on('message', function (msg) {
  if(typeof accellPatternBank == 'undefined'){
    accellPatternBank=0;
  }
  if( typeof previousAccellData == 'undefined'){
    previousAccellData=[0.0,0.0,0.0];
    lastAccellTriggerTime=new Date().getTime();
  }else{
    if(  Math.abs(lastAccellTriggerTime-new Date().getTime())>100  ){
      lastAccellTriggerTime=new Date().getTime();
      oscChannelName=msg.toString().substring(msg.toString().indexOf(',')-1,msg.toString().indexOf(','));
      oscChannelValue=parseFloat(msg.toString().substring(msg.toString().indexOf(',')+1,msg.toString().length));
      var diffMagnitude=[0.0,0.0,0.0];
      var needsToRunSync=false;
      if(oscChannelName=="x"){
        diffMagnitude[0]=Math.abs(previousAccellData[0]-oscChannelValue);
        previousAccellData[0]=oscChannelValue;
      }else if(oscChannelName=="y"){
        diffMagnitude[1]=Math.abs(previousAccellData[1]-oscChannelValue);
        previousAccellData[1]=oscChannelValue;
      }else if(oscChannelName=="z"){
        diffMagnitude[2]=Math.abs(previousAccellData[2]-oscChannelValue);
        previousAccellData[2]=oscChannelValue;
      }
      if((diffMagnitude[0]+diffMagnitude[1]+diffMagnitude[2])>=.07){
        accellPatternBank=(accellPatternBank+1)%8;
        console.log(`Accell Bank Change: ${accellPatternBank}`);
        for(var x=0; x<64; x++){
          loopCellsState[x]=false;
          if( (x>=(4+accellPatternBank*8)) && (x<(8+accellPatternBank*8)) ){
            loopCellsState[x]=true;
            // console.log("loopCellsState of "+x+"is on!");
          }
        }
        needsToRunSync=true;
      }
      if((diffMagnitude[0]+diffMagnitude[1]+diffMagnitude[2])>=.02){
        console.log(`Message: ${msg}`);
        for(var x=1; x<64; x++){
          if(loopCellsState[(patternNumber+4+x)%64]==true){
            patternNumber=((patternNumber+4+x)%64)-4;
            // console.log(patternNumber);
            break;
          }
        }
        needsToRunSync=true;
      }
      if(needsToRunSync){
        syncArtnetToModel();
        syncLeds();
      }
    }
  }
});

function lpFilter(desiredValues){
  for(var x=0;x<num_pixels;x++){
    for(var y=0;y<4;y++){
      var delta=desiredValues[x][y]-filterBuffer[x][y];
      if(Math.abs(Math.floor(delta))<3 || parseInt(channels[patternNumber][4].cycleTime) <= 64 || y==3){
        filterBuffer[x][y]=desiredValues[x][y];
      }else{
        // filterBuffer[x][y]=filterBuffer[x][y]+delta/parseInt(1+(channels[patternNumber][4].cycleTime)/127*20);
        filterBuffer[x][y]=filterBuffer[x][y]+delta/parseInt(1+(channels[patternNumber][4].cycleTime-64)/64*20);
      }
    }
  }
  return filterBuffer;
}

function patternSynthesizer(colorIndex, colorSpread, cycleTime, trailLength, trailSpread, dir, strobe, brightness){
  var synthesizedPattern=[]
  for(var x=0;x<num_pixels;x++){
    synthesizedPattern.push([0,0,0,0]);
  }
  
  ts=new Date().getTime();
  cycle=(ts)%cycleTime;
  index=Math.floor((cycle/cycleTime)*num_pixels);
  var randomSpeed=4;
  if(dir>100){
    randomSpeed=16;
  }
  if(Math.floor(ts/(cycleTime/randomSpeed))!=randomnessCount && randomnessDebounce==false){
    randomnessCount=Math.floor(ts/(cycleTime/randomSpeed));
    randomnessDebounce=true; 
  }

  if(dir<32){
    for(var i=0; i<trailLength; i++){
      ind=(index+i*trailSpread)%num_pixels;
      synthesizedPattern[ind][0]=parseInt(colorMap[0][(colorIndex+i*colorSpread)%156]*brightness/255);
      synthesizedPattern[ind][1]=parseInt(colorMap[1][(colorIndex+i*colorSpread)%156]*brightness/255);
      synthesizedPattern[ind][2]=parseInt(colorMap[2][(colorIndex+i*colorSpread)%156]*brightness/255);
    }
  }else if(dir<64){
    for(var i=0; i<trailLength; i++){
      ind=(index+i*trailSpread)%num_pixels;
      synthesizedPattern[num_pixels-1-ind][0]=parseInt(colorMap[0][(colorIndex+i*colorSpread)%156]*brightness/255);
      synthesizedPattern[num_pixels-1-ind][1]=parseInt(colorMap[1][(colorIndex+i*colorSpread)%156]*brightness/255);
      synthesizedPattern[num_pixels-1-ind][2]=parseInt(colorMap[2][(colorIndex+i*colorSpread)%156]*brightness/255);
    }
  }else if(dir<115){
    if(randomnessDebounce==true){
      var temp=[];
      for(var i=0; i<trailLength; i++){
        temp.push(Math.floor(Math.random()*num_pixels));
      }
      randomnessBuffer=temp;
      randomnessDebounce=false;
    }
    for(var i=0; i<randomnessBuffer.length; i++){
      ind=randomnessBuffer[i];
      synthesizedPattern[ind][0]=parseInt(colorMap[0][(colorIndex+i*colorSpread)%156]*brightness/255);
      synthesizedPattern[ind][1]=parseInt(colorMap[1][(colorIndex+i*colorSpread)%156]*brightness/255);
      synthesizedPattern[ind][2]=parseInt(colorMap[2][(colorIndex+i*colorSpread)%156]*brightness/255);
    }
  }else{
    for(var i=0; i<trailLength; i++){
      ind=(index+i*trailSpread)%num_pixels;
      synthesizedPattern[ind][0]=parseInt(colorMap[0][(colorIndex+i*colorSpread)%156]*brightness/255);
      synthesizedPattern[ind][1]=parseInt(colorMap[1][(colorIndex+i*colorSpread)%156]*brightness/255);
      synthesizedPattern[ind][2]=parseInt(colorMap[2][(colorIndex+i*colorSpread)%156]*brightness/255);
      synthesizedPattern[num_pixels-1-ind][0]=parseInt(colorMap[0][(colorIndex+i*colorSpread)%156]*brightness/255);
      synthesizedPattern[num_pixels-1-ind][1]=parseInt(colorMap[1][(colorIndex+i*colorSpread)%156]*brightness/255);
      synthesizedPattern[num_pixels-1-ind][2]=parseInt(colorMap[2][(colorIndex+i*colorSpread)%156]*brightness/255);
    }
  }

  if(channels[patternNumber][4].strobe>113){
    for(var x=0;x<num_pixels;x++){
      var temp=brightness/255*150*(synthesizedPattern[x][0]/255+synthesizedPattern[x][1]/255+synthesizedPattern[x][2]/255)*3;
      if(temp>124){
        temp=124
      }
      synthesizedPattern[x][3]=temp;
      synthesizedPattern[x][0]=0;
      synthesizedPattern[x][1]=0;
      synthesizedPattern[x][2]=0;
    }
  }else{
    for(var x=0;x<num_pixels;x++){
      synthesizedPattern[x][3]=0;
    }
  }

  filteredSynthesizedPattern=lpFilter(synthesizedPattern);
  return parsePatternToDMX(filteredSynthesizedPattern, strobe);
}

function parsePatternToDMX(synthesizedPattern, strobe){
  var dmxPatternBuffer=[];
  var selected_pixel=0;
  for (var x in dmxChain){
    for (const [channelName, value] of Object.entries(dmxChain[x].channels)) {
      var matchFound=false;
      for(var z in dmxChain[x].pixel_channels) {
        if(dmxChain[x].pixel_channels[z].r==`${channelName}`){
          dmxPatternBuffer.push(synthesizedPattern[Math.floor(selected_pixel/4)][0]);
          selected_pixel=selected_pixel+1;
          matchFound=true;
          // console.log(`${channelName}: ${value}`);
        }else if(dmxChain[x].pixel_channels[z].g==`${channelName}`){
          dmxPatternBuffer.push(synthesizedPattern[Math.floor(selected_pixel/4)][1]);
          selected_pixel=selected_pixel+1;
          matchFound=true;
        }else if(dmxChain[x].pixel_channels[z].b==`${channelName}`){
          dmxPatternBuffer.push(synthesizedPattern[Math.floor(selected_pixel/4)][2]);
          selected_pixel=selected_pixel+1;
          matchFound=true;
        }else if(dmxChain[x].pixel_channels[z].w==`${channelName}`){
          dmxPatternBuffer.push(synthesizedPattern[Math.floor(selected_pixel/4)][3]);
          selected_pixel=selected_pixel+1;
          matchFound=true;
        }else if("x_pos"==`${channelName}`){
          dmxPatternBuffer.push(channels[patternNumber][Math.floor(selected_pixel/4)].x);
          matchFound=true;
        }else if("y_pos"==`${channelName}`){
          dmxPatternBuffer.push(channels[patternNumber][Math.floor(selected_pixel/4)].y);
          matchFound=true;
        }
      }
      if(matchFound==false){
        if(`${channelName}`=="strobe"){
          dmxPatternBuffer.push(strobe)
        }else{
          dmxPatternBuffer.push(dmxChain[x].channels[`${channelName}`]);
        }
      }
    }
  }
  return dmxPatternBuffer;
}

//colorIndex, colorSpread, cycleTime, trailLength, trailSpread, dir, strobe, brightness
function sendArtnet(values){
  artnet.set(1,1, values, function (err, res) {
    // console.log("Sent Artnet:");
    // console.log(values);
  });
  artnet.set(2,1, patternSynthesizer(
    Math.floor(channels[patternNumber][4].colorIndex*1.25),
    Math.floor(channels[patternNumber][4].colorSpread/2),
    Math.floor(200+channels[patternNumber][4].cycleTime*20),
    Math.floor(1+channels[patternNumber][4].trailLength/127*num_pixels*1),
    Math.floor(1+channels[patternNumber][4].trailSpread/127*num_pixels*1),
    Math.floor(channels[patternNumber][4].dir),
    Math.floor(127+channels[patternNumber][4].strobe*.925),
    Math.floor(channels[patternNumber][4].brightness*2*masterBrightness)), function (err, res) {
  // artnet.set(2,1, temp, function (err, res) {
    // console.log("Sent Artnet:");
  });
}

app.get('/test', (req, res) => {
  for(var x=0;x<64;x++){
    console.log(loopCellsState[x]);
    if((x%8)>3){
      if(loopCellsState[x]){
        apcMiniOutput.sendMessage([144,x,3]); 
      }else{
        apcMiniOutput.sendMessage([144,x,5]); 
      }
    }
  }
  res.send(loopCellsState);
})

app.get('/test2', (req, res) => {
  res.send("Hello World!");
  syncLeds();
  sendArtnet([175,10,255,255,255,255,255,255,255,255,255]);
})

app.get('/test3', (req, res) => {
  copyFixture(0,1);
  res.send("Hello World!3");
})

app.get('/test4', (req, res) => {
  copyScene(56, 57, [0,1,2,3]);
  res.send("Hello World!4");
})

app.get('/save', (req, res) => {
  save();
  res.send(channels);
})

app.get('/recall', (req, res) => {
  recall();
  res.send(JSON.stringify(channels));
})

app.listen(port, () => {
  console.log(` listening at http://localhost:${port}`)
})

function recall(){
  fs.readFile('/root/ledControllerData.txt', 'utf8', (err, data) => {
    if (err) {
      console.error(err);
      return;
    }
    console.log("loaded presets");
    channels=JSON.parse(data);
    syncArtnetToModel();
  });
}

function save(){
  try {
    fs.writeFileSync('/root/ledControllerData.txt', JSON.stringify(channels));
    console.log('file written successfully');
  } catch (err) {
    console.error(err);
  }
}

function syncLeds(){
  for(var x=0;x<100;x++){
    if(x<64){
      if(!isShift){
        if(x==patternNumber){
          apcMiniOutput.sendMessage([144,x,3]); 
        }else{
          if((x%8)<=3){
            apcMiniOutput.sendMessage([144,x,127]); 
          }else{
            if(loopCellsState[x]){
              apcMiniOutput.sendMessage([144,x,3]); 
            }else{
              apcMiniOutput.sendMessage([144,x,5]); 
            }
          }
        }
      }else{
        if((x%8)<=3){
          if(copySceneSource==x){
            apcMiniOutput.sendMessage([144,x,127]); 
          }else{
            apcMiniOutput.sendMessage([144,x,3]); 
          }
        }else{
          if(loopCellsState[x]){
            apcMiniOutput.sendMessage([144,x,3]); 
          }else{
            apcMiniOutput.sendMessage([144,x,5]); 
          }
        }
      }
    }else{
      if(!(x>=68 && x<72) && x!=88 ){
        apcMiniOutput.sendMessage([144,x,5]);  
      }else if(x==88){
        if(selectedFixtures.indexOf((4))!=-1){
          apcMiniOutput.sendMessage([144,x,127]); 
        }else{
          apcMiniOutput.sendMessage([144,x,0]); 
        }
      }
    }
    if(x>=64 && x<68){
      if(!isShift){
        if(selectedFixtures.indexOf((x-64))!=-1){
          apcMiniOutput.sendMessage([144,x,127]); 
        }else{
          apcMiniOutput.sendMessage([144,x,0]); 
        }
      }else{
        if((x-64)==copyFixtureSource){
          apcMiniOutput.sendMessage([144,x,127]); 
        }else{
          apcMiniOutput.sendMessage([144,x,0]); 
        }
      }
    }
    if(x>=68 && x<72){
      apcMiniOutput.sendMessage([144,x,127*fixtureBrightness[x-68]]); 
    }
    if(x==89){
      if(!loopIsPlaying){
        apcMiniOutput.sendMessage([144,x,0]); 
      }else{
        apcMiniOutput.sendMessage([144,x,127]); 
      }
    }
  }
}

function syncArtnetToModel(){
  var temp=[];
  for (var f=0; f<numFixtures; f++){
    temp=temp.concat([channels[patternNumber][f].colorIndex*2, channels[patternNumber][f].colorSpread*2, channels[patternNumber][f].cycleTime*2, channels[patternNumber][f].trailLength*2, channels[patternNumber][f].trailSpread*2, channels[patternNumber][f].dir*2, channels[patternNumber][f].strobe*2, masterBrightness*fixtureBrightness[f]*channels[patternNumber][f].brightness*2]);
  }
  sendArtnet(temp);
}

function fixtureButtonPress(val){
  if(selectedFixtures.indexOf(val)!=-1){
      deselectFixture(val);
  }else{
      selectFixture(val);
  }
}

function selectFixture(val){
  selectedFixtures.push(val);
  console.log(selectedFixtures);
}

function deselectFixture(val){
  var temp=[];
  for(var x in selectedFixtures){
      if(selectedFixtures[x]!=val){
          temp=temp.concat(selectedFixtures[x]);
      }
  }
  selectedFixtures=temp;
  // console.log(selectedFixtures);
}

function loopRoWStart(freq){
  intervalHandle=setInterval(function(){
    for(var x=1; x<64; x++){
      if(loopCellsState[(patternNumber+4+x)%64]==true){
        patternNumber=((patternNumber+4+x)%64)-4;
        // console.log(patternNumber);
        break;
      }
    }
    syncArtnetToModel();
    syncLeds();
  },freq);
  loopIsPlaying=true;
}

function loopRowOld(freq){
  intervalHandle=setInterval(function(){
    if(patternNumber+1>=(Math.floor(patternNumber/4)*4)+4){
      patternNumber=Math.floor(patternNumber/4)*4;
    }else{
      patternNumber=patternNumber+1;
    }
    syncArtnetToModel();
    syncLeds();
  },freq);
  loopIsPlaying=true;
}

function loopRoWStop(){
  clearInterval(intervalHandle);
  loopIsPlaying=false;
}

function copyScene(sourceScene, targetScene, includedFixtures){
  for(var x=0; x<numFixtures; x++){
    if(includedFixtures.indexOf(x)!=-1){
      channels[targetScene][x]=JSON.parse(JSON.stringify(channels[sourceScene][x])); 
    }
  }
  syncArtnetToModel();
}

function copyFixture(sourceFixture, targetFixture){
  var temp_x=channels[patternNumber][targetFixture].x;
  var temp_y=channels[patternNumber][targetFixture].y;
  channels[patternNumber][targetFixture]=JSON.parse(JSON.stringify(channels[patternNumber][sourceFixture])); 
  channels[patternNumber][targetFixture].x=temp_x;
  channels[patternNumber][targetFixture].y=temp_y;

  syncArtnetToModel();
}

function rotateFixtures(directionBool){
  // console.log("running rotateFixtures");
  var temp=null;
  if(directionBool){
    for(var x=3; x>=0; x--){
      var temp_x=channels[patternNumber][x].x;
      var temp_y=channels[patternNumber][x].y;
      if(x==3){
        temp=JSON.stringify(channels[patternNumber][x]);
        channels[patternNumber][x]=JSON.parse(JSON.stringify(channels[patternNumber][x-1])); 
      }
      if(x==2 || x==1){
        channels[patternNumber][x]=JSON.parse(JSON.stringify(channels[patternNumber][x-1])); 
      }
      if(x==0){
        channels[patternNumber][x]=JSON.parse(temp); 
      }
      channels[patternNumber][x].x=temp_x;
      channels[patternNumber][x].y=temp_y;
    }
  }else{
    for(var x=0; x<4; x++){
      var temp_x=channels[patternNumber][x].x;
      var temp_y=channels[patternNumber][x].y;
      if(x==0){
        temp=JSON.stringify(channels[patternNumber][x]);
        channels[patternNumber][x]=JSON.parse(JSON.stringify(channels[patternNumber][x+1])); 
      }
      if(x==2 || x==1){
        channels[patternNumber][x]=JSON.parse(JSON.stringify(channels[patternNumber][x+1])); 
      }
      if(x==3){
        channels[patternNumber][x]=JSON.parse(temp); 
      }
      channels[patternNumber][x].x=temp_x;
      channels[patternNumber][x].y=temp_y;
    }
  }
  syncArtnetToModel();
}